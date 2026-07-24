/* ===========================================================================
 * server.c — the single-threaded epoll event loop, networking, and main().
 * ===========================================================================
 *
 * THE REACTOR PATTERN. Redis is (famously) single-threaded for command
 * execution: one thread multiplexes thousands of connections with epoll(7).
 * There are no per-command locks because there is no concurrency at the command
 * boundary — a command runs to completion before the next one starts. That is
 * why the dict, the keyspace, and the buffers in the rest of this project need
 * no synchronization. The cost model is "never block the loop": every socket is
 * non-blocking and every syscall that could block is driven by readiness
 * notifications from epoll.
 *
 * ONE ITERATION of the loop:
 *   1. epoll_wait() sleeps until a socket is readable/writable or a timeout.
 *   2. For each ready fd: accept new clients (listen fd), read+parse+execute
 *      commands (client readable), or flush queued replies (client writable).
 *   3. Roughly every 100ms, serverCron() runs the periodic housekeeping:
 *      sampled key expiration, incremental rehash progress, reaping a finished
 *      BGSAVE child, and flushing/fsyncing the AOF.
 *
 * PLATFORM: Linux (epoll, accept4, fork CoW). Build with the provided Makefile
 * on Linux or WSL2. The teaching assembly under asm/ is host-portable.
 * =========================================================================== */
#define _GNU_SOURCE            /* accept4, EPOLL_CLOEXEC, SOCK_NONBLOCK          */
#include "server.h"
#include "zmalloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>           /* strcasecmp (option parsing)                    */
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>       /* TCP_NODELAY                                    */
#include <sys/epoll.h>
#include <sys/wait.h>

/* The one and only server instance. */
struct redisServer server;

/* Size of the transient read buffer per readable event. */
#define READ_CHUNK 16384

/* How often serverCron runs, in milliseconds (Redis's default hz is 10 == every
 * 100ms). */
#define CRON_PERIOD_MS 100

/* ---------------------------------------------------------------------------
 * Time and logging.
 * ------------------------------------------------------------------------- */

/* Wall-clock time in milliseconds. Expiration deadlines are absolute values on
 * this clock, so a TTL survives across commands (though not across a wall-clock
 * change — a known trade-off of using CLOCK_REALTIME, which we accept because
 * TTLs are semantically wall-clock). */
long long mstime(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

void serverLog(const char *fmt, ...)
{
    char   ts[32];
    time_t now = time(NULL);
    struct tm tmv;
    localtime_r(&now, &tmv);
    strftime(ts, sizeof(ts), "%d %b %H:%M:%S", &tmv);

    fprintf(stdout, "%d:M %s ", (int)getpid(), ts);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    va_end(ap);
    fputc('\n', stdout);
    fflush(stdout);
}

/* ---------------------------------------------------------------------------
 * Socket helpers.
 * ------------------------------------------------------------------------- */

/* Put a fd into non-blocking mode so no read/write can ever stall the loop. */
static int setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

/* Create, bind, and listen on a TCP socket for `port`. Fatal on error — a
 * server that cannot listen has nothing to do. */
static int createListenSocket(int port)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) { serverLog("socket: %s", strerror(errno)); exit(1); }

    /* SO_REUSEADDR lets us rebind immediately after a restart instead of
     * waiting out the TIME_WAIT state of the previous listener's sockets. */
    int yes = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
        serverLog("setsockopt(SO_REUSEADDR): %s", strerror(errno));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);        /* bind all interfaces       */
    addr.sin_port        = htons((uint16_t)port);    /* network byte order        */

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        serverLog("bind(%d): %s", port, strerror(errno)); exit(1);
    }
    if (listen(fd, SOMAXCONN) == -1) {               /* backlog of pending SYNs   */
        serverLog("listen: %s", strerror(errno)); exit(1);
    }
    if (setNonBlocking(fd) == -1) {
        serverLog("setNonBlocking(listen): %s", strerror(errno)); exit(1);
    }
    return fd;
}

/* Modify a client fd's epoll interest set. */
static void epollMod(client *c, uint32_t events)
{
    struct epoll_event ev;
    ev.events   = events;
    ev.data.ptr = c;
    if (epoll_ctl(server.epoll_fd, EPOLL_CTL_MOD, c->fd, &ev) == -1)
        serverLog("epoll_ctl(MOD, fd=%d): %s", c->fd, strerror(errno));
}

/* ---------------------------------------------------------------------------
 * Client lifecycle.
 * ------------------------------------------------------------------------- */
static client *clientCreate(int fd)
{
    client *c = zcalloc(sizeof(*c));                 /* zero-init all fields      */
    c->fd              = fd;
    c->querybuf        = sdsempty();
    c->bulklen         = -1;
    c->pubsub_channels = dictCreate(&keyptrDictType);

    /* Watch for readability. data.ptr = c lets the event loop recover the
     * client in O(1) without an fd->client map. */
    struct epoll_event ev;
    ev.events   = EPOLLIN;
    ev.data.ptr = c;
    if (epoll_ctl(server.epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        serverLog("epoll_ctl(ADD): %s", strerror(errno));
        close(fd);
        dictRelease(c->pubsub_channels);
        sdsfree(c->querybuf);
        zfree(c);
        return NULL;
    }

    /* Push onto the head of the intrusive client list. */
    c->prev = NULL;
    c->next = server.clients;
    if (server.clients) server.clients->prev = c;
    server.clients = c;
    server.numclients++;
    return c;
}

void freeClient(client *c)
{
    pubsubUnsubscribeAll(c);                         /* drop channel subscriptions*/
    dictRelease(c->pubsub_channels);

    /* Deregister and close the socket. Passing a non-NULL event for DEL keeps us
     * portable to pre-2.6.9 kernels; NULL is accepted on modern ones. */
    epoll_ctl(server.epoll_fd, EPOLL_CTL_DEL, c->fd, NULL);
    close(c->fd);

    sdsfree(c->querybuf);
    resetClientCommand(c);                           /* frees argv               */
    zfree(c->buf);

    /* Unlink from the client list. */
    if (c->prev) c->prev->next = c->next; else server.clients = c->next;
    if (c->next) c->next->prev = c->prev;
    server.numclients--;
    zfree(c);
}

/* Arm the fd for EPOLLOUT when there is unsent reply data and it is not already
 * armed. Used by PUBLISH to schedule writes to clients other than the current
 * one. A guard for fd < 0 makes it safe on the AOF-load fake client. */
void clientInstallWriteHandler(client *c)
{
    if (c->fd < 0) return;
    if (c->flags & CLIENT_PENDING_WRITE) return;     /* already watching EPOLLOUT */
    if (c->bufpos <= c->sentlen) return;             /* nothing queued            */
    epollMod(c, EPOLLIN | EPOLLOUT);
    c->flags |= CLIENT_PENDING_WRITE;
}

/* ---------------------------------------------------------------------------
 * Reply flushing. Returns 0 normally, -1 if the client was freed.
 * ------------------------------------------------------------------------- */
static int writeToClient(client *c)
{
    while (c->sentlen < c->bufpos) {
        ssize_t n = write(c->fd, c->buf + c->sentlen, c->bufpos - c->sentlen);
        if (n == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                /* Kernel send buffer is full: wait for EPOLLOUT and resume. */
                clientInstallWriteHandler(c);
                return 0;
            }
            if (errno == EINTR) continue;            /* retry after a signal      */
            freeClient(c);                           /* real error: drop client   */
            return -1;
        }
        c->sentlen += (size_t)n;                     /* partial writes are normal */
    }

    /* Everything queued has been sent. Reset the buffer offsets. */
    c->sentlen = 0;
    c->bufpos  = 0;

    /* If we were watching EPOLLOUT, stop — there is nothing left to write. */
    if (c->flags & CLIENT_PENDING_WRITE) {
        epollMod(c, EPOLLIN);
        c->flags &= ~CLIENT_PENDING_WRITE;
    }
    /* Deferred close (QUIT, protocol error) happens once the reply has drained. */
    if (c->flags & CLIENT_CLOSE_AFTER_REPLY) { freeClient(c); return -1; }
    return 0;
}

/* Read available bytes, parse+execute complete commands, flush replies.
 * Returns 0 normally, -1 if the client was freed. */
static int readFromClient(client *c)
{
    char buf[READ_CHUNK];
    ssize_t n = read(c->fd, buf, sizeof(buf));
    if (n == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) return 0; /* spurious wakeup */
        if (errno == EINTR) return 0;
        freeClient(c);                               /* connection error          */
        return -1;
    }
    if (n == 0) { freeClient(c); return -1; }        /* orderly peer shutdown     */

    c->querybuf = sdscatlen(c->querybuf, buf, (size_t)n);
    processInputBuffer(c);                           /* run everything complete   */

    /* Push out whatever the commands queued. */
    if (c->bufpos > c->sentlen)
        return writeToClient(c);                     /* may free the client       */
    if (c->flags & CLIENT_CLOSE_AFTER_REPLY) { freeClient(c); return -1; }
    return 0;
}

/* Accept as many pending connections as the backlog holds (drain the listen
 * socket while it is readable). accept4 sets O_NONBLOCK + CLOEXEC atomically,
 * avoiding a separate fcntl and a fd-leak-across-exec race. */
static void acceptHandler(void)
{
    for (;;) {
        int cfd = accept4(server.listen_fd, NULL, NULL,
                          SOCK_NONBLOCK | SOCK_CLOEXEC);
        if (cfd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break; /* drained        */
            if (errno == EINTR) continue;
            serverLog("accept4: %s", strerror(errno));
            break;
        }
        /* Disable Nagle: reply latency matters more than packet efficiency for a
         * request/response protocol. */
        int yes = 1;
        setsockopt(cfd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes));
        if (clientCreate(cfd) == NULL) close(cfd);
    }
}

/* ---------------------------------------------------------------------------
 * Command dispatch.
 * ------------------------------------------------------------------------- */
void processCommand(client *c)
{
    struct redisCommand *cmd = lookupCommand(c->argv[0]);
    if (cmd == NULL) {
        addReplyError(c, "ERR unknown command");
        return;
    }
    /* Arity: exact if positive, minimum if negative. */
    if ((cmd->arity > 0 && cmd->arity != c->argc) ||
        (cmd->arity < 0 && c->argc < -cmd->arity)) {
        addReplyError(c, "ERR wrong number of arguments for command");
        return;
    }

    /* Run the handler, then decide whether to propagate to the AOF. We log a
     * write command only if it ACTUALLY changed the dataset (server.dirty moved)
     * — a SET that fails validation, or a DEL of a missing key, must not bloat
     * the log or alter replay. */
    long long dirty_before = server.dirty;
    cmd->proc(c);
    if ((cmd->flags & CMD_WRITE) && server.dirty != dirty_before)
        aofFeed(c->argv, c->argc);
}

/* ---------------------------------------------------------------------------
 * Periodic housekeeping.
 * ------------------------------------------------------------------------- */
static void serverCron(void)
{
    server.mstime = mstime();

    /* 1. Reclaim a sample of expired keys (active expiration). */
    activeExpireCycle();

    /* 2. Make rehash progress even with no traffic to piggy-back on, so a resize
     *    triggered by a burst still finishes during a quiet period. */
    if (dictIsRehashing(server.db.dict))    dictRehash(server.db.dict, 100);
    if (dictIsRehashing(server.db.expires)) dictRehash(server.db.expires, 100);

    /* 3. Reap a finished background save without blocking (WNOHANG). */
    if (server.rdb_child_pid != -1) {
        int status;
        pid_t pid = waitpid(server.rdb_child_pid, &status, WNOHANG);
        if (pid == server.rdb_child_pid) {
            int ok = WIFEXITED(status) && WEXITSTATUS(status) == 0;
            backgroundSaveDoneHandler(ok);
        }
    }

    /* 4. Flush buffered AOF and fsync per policy (everysec fires here). */
    flushAppendOnlyFile(0);
}

/* ---------------------------------------------------------------------------
 * The event loop.
 * ------------------------------------------------------------------------- */
static void eventLoop(void)
{
    server.cron_last_ms = mstime();
    while (!server.shutdown_asap) {
        int n = epoll_wait(server.epoll_fd, server.events,
                           server.maxevents, CRON_PERIOD_MS);
        if (n == -1) {
            if (errno == EINTR) { /* a signal (maybe SIGTERM) — loop and re-check */
                if (server.shutdown_asap) break;
                continue;
            }
            serverLog("epoll_wait: %s", strerror(errno));
            break;
        }

        for (int i = 0; i < n; i++) {
            struct epoll_event *ev = &server.events[i];

            /* The listen fd is registered with data.ptr == NULL. */
            if (ev->data.ptr == NULL) {
                if (ev->events & EPOLLIN) acceptHandler();
                continue;
            }

            client *c = (client *)ev->data.ptr;

            /* A hangup or socket error: drop the client. */
            if (ev->events & (EPOLLERR | EPOLLHUP)) { freeClient(c); continue; }

            /* Readable: read+parse+execute. This may free the client (EOF /
             * close-after-reply), in which case we must not touch it again. */
            if (ev->events & EPOLLIN) {
                if (readFromClient(c) == -1) continue;
            }
            /* Writable: resume a previously-blocked flush. */
            if (ev->events & EPOLLOUT) {
                writeToClient(c);
            }
        }

        /* Run periodic tasks if the period elapsed. */
        long long now = mstime();
        if (now - server.cron_last_ms >= CRON_PERIOD_MS) {
            serverCron();
            server.cron_last_ms = now;
        }
    }
}

/* ---------------------------------------------------------------------------
 * Signals.
 * ------------------------------------------------------------------------- */

/* SIGTERM/SIGINT: ask the loop to stop at the next safe point. We only set a
 * flag here — doing real work in a handler is unsafe (it can interrupt malloc). */
static void sigShutdownHandler(int sig)
{
    (void)sig;
    server.shutdown_asap = 1;
}

static void setupSignals(void)
{
    /* Writing to a socket the peer closed raises SIGPIPE by default, which would
     * kill the process. We ignore it and handle the EPIPE errno from write. */
    signal(SIGPIPE, SIG_IGN);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigShutdownHandler;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT,  &sa, NULL);
}

/* ---------------------------------------------------------------------------
 * Initialization and configuration.
 * ------------------------------------------------------------------------- */
static void initServerDefaults(void)
{
    server.port          = 6379;
    server.rdb_filename  = zstrdup("dump.rdb");
    server.aof_filename  = zstrdup("appendonly.aof");
    server.aof_enabled   = 0;
    server.aof_fsync     = AOF_FSYNC_EVERYSEC;
    server.aof_fd        = -1;
    server.rdb_child_pid = -1;
    server.dirty         = 0;
    server.maxevents     = 1024;
    server.shutdown_asap = 0;
}

static void initServer(void)
{
    server.mstime      = mstime();
    server.db.dict     = dictCreate(&dbDictType);
    server.db.expires  = dictCreate(&keyptrDictType);
    server.clients     = NULL;
    server.numclients  = 0;
    pubsubInit();

    server.epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (server.epoll_fd == -1) { serverLog("epoll_create1: %s", strerror(errno)); exit(1); }
    server.events = zmalloc(sizeof(struct epoll_event) * server.maxevents);

    server.listen_fd = createListenSocket(server.port);

    /* Register the listen fd with a NULL cookie so the loop recognizes it. */
    struct epoll_event ev;
    ev.events   = EPOLLIN;
    ev.data.ptr = NULL;
    if (epoll_ctl(server.epoll_fd, EPOLL_CTL_ADD, server.listen_fd, &ev) == -1) {
        serverLog("epoll_ctl(ADD listen): %s", strerror(errno)); exit(1);
    }
}

/* A tiny option parser: positional port, plus a few --flags mirroring Redis. */
static void parseArgs(int argc, char **argv)
{
    int i = 1;
    /* redis-server style: a bare first number is the port. */
    if (i < argc && argv[i][0] != '-') {
        server.port = atoi(argv[i]);
        i++;
    }
    for (; i < argc; i++) {
        if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            server.port = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--appendonly") == 0 && i + 1 < argc) {
            server.aof_enabled = (strcasecmp(argv[++i], "yes") == 0);
        } else if (strcmp(argv[i], "--appendfsync") == 0 && i + 1 < argc) {
            const char *p = argv[++i];
            if      (strcasecmp(p, "always")   == 0) server.aof_fsync = AOF_FSYNC_ALWAYS;
            else if (strcasecmp(p, "everysec") == 0) server.aof_fsync = AOF_FSYNC_EVERYSEC;
            else                                     server.aof_fsync = AOF_FSYNC_NO;
        } else if (strcmp(argv[i], "--dbfilename") == 0 && i + 1 < argc) {
            zfree(server.rdb_filename);
            server.rdb_filename = zstrdup(argv[++i]);
        } else if (strcmp(argv[i], "--appendfilename") == 0 && i + 1 < argc) {
            zfree(server.aof_filename);
            server.aof_filename = zstrdup(argv[++i]);
        } else {
            serverLog("ignoring unknown option: %s", argv[i]);
        }
    }
}

int main(int argc, char **argv)
{
    initServerDefaults();
    parseArgs(argc, argv);

    /* Seed the dict hash with a per-run random value to defeat hash-flooding,
     * and seed rand() (used by the expiry sampler) from the same entropy. */
    server.hashseed = (uint64_t)time(NULL) ^ ((uint64_t)getpid() << 32);
    dictSetHashSeed(server.hashseed);
    srand((unsigned)server.hashseed);

    setupSignals();
    initServer();
    aofInit();

    /* Restore state: the AOF is authoritative when enabled (it is the more
     * recent, finer-grained log); otherwise fall back to the RDB snapshot. */
    if (server.aof_enabled) {
        if (loadAppendOnlyFile(server.aof_filename) != 0)
            serverLog("warning: AOF load reported an error");
    } else {
        if (rdbLoad(server.rdb_filename) != 0)
            serverLog("warning: RDB load reported an error");
    }

    serverLog("redis-clone ready to accept connections on port %d "
              "(AOF %s, fsync %s)", server.port,
              server.aof_enabled ? "on" : "off",
              server.aof_fsync == AOF_FSYNC_ALWAYS   ? "always"   :
              server.aof_fsync == AOF_FSYNC_EVERYSEC ? "everysec" : "no");

    eventLoop();

    /* Graceful shutdown: flush+fsync the AOF so nothing buffered is lost. */
    serverLog("shutting down");
    flushAppendOnlyFile(1);
    if (server.aof_fd >= 0) close(server.aof_fd);
    return 0;
}
