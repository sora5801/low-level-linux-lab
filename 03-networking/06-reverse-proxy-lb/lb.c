/* ===========================================================================
 * lb.c — a single-threaded, epoll-driven L4 reverse proxy / load balancer.
 * ===========================================================================
 *
 * WHAT THIS PROGRAM DOES
 * ----------------------
 * It listens on a TCP port, and for every client that connects it:
 *   1. picks a backend by the configured policy (round-robin / least-conns /
 *      consistent-hash-on-client-IP),
 *   2. opens (or reuses a pre-warmed) TCP connection to that backend, and
 *   3. splices bytes in both directions with ZERO userspace copies until one
 *      side closes, then relays the half-close and tears the pair down.
 * Meanwhile a timerfd drives periodic health probes that eject/re-admit
 * backends, an AF_UNIX control socket lets an admin drain a backend gracefully,
 * and SIGINT/SIGTERM start a graceful whole-process drain.
 *
 * WHY splice() (the zero-copy idea at the heart of the data path)
 * --------------------------------------------------------------
 * The obvious proxy loop is `n = read(src, buf, N); write(dst, buf, n);`. That
 * copies every byte TWICE across the user/kernel boundary: kernel socket buffer
 * -> our `buf` (copy 1, a read), then `buf` -> kernel socket buffer (copy 2, a
 * write). For a byte-shovel that inspects nothing, those copies are pure waste
 * of CPU and memory bandwidth.
 *
 * splice(2) moves data between a file descriptor and a PIPE entirely inside the
 * kernel by handing PAGE REFERENCES around (struct pipe_buffer points at the
 * same pages the socket already holds); the bytes are never mapped into this
 * process. To move socket->socket we stage through a pipe:
 *
 *     splice(src_socket, NULL, pipe_wr, NULL, len, ...)   // socket -> pipe
 *     splice(pipe_rd,   NULL, dst_socket, NULL, len, ...) // pipe   -> socket
 *
 * The data sits in the pipe's kernel buffers the whole time; our address space
 * never sees it and the CPU never memcpy's it. That is the same mechanism nginx,
 * HAProxy, and the kernel's own sendfile() use. (sendfile() is the file->socket
 * special case; splice() is the general fd<->pipe form, which is what a proxy
 * needs because both ends are sockets.)
 *
 * WHY epoll (the concurrency model)
 * ---------------------------------
 * One thread cannot afford to block on any single connection, so every fd is
 * non-blocking and we ask ONE epoll instance "which of my thousands of fds are
 * ready?" epoll returns only the ready ones in O(ready), not O(total) like
 * select()/poll(). The hot per-connection fds use EDGE-TRIGGERED mode (EPOLLET):
 * the kernel signals readiness only on the 0->ready transition, so we MUST drain
 * each fd until EAGAIN or we would stall waiting for an edge that never comes.
 * The low-rate control fds (listener, timerfd, signalfd, control socket) use the
 * simpler level-triggered mode. There are no locks anywhere because there is no
 * second thread — this is I/O concurrency, not thread concurrency.
 * ===========================================================================
 */
#define _GNU_SOURCE                 /* splice, accept4, pipe2, SOCK_*, timerfd... */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>                  /* splice, fcntl, pipe2, O_NONBLOCK          */
#include <signal.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sys/signalfd.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>             /* inet_ntop                                 */
#include <netdb.h>                 /* getaddrinfo                               */

#include "lb.h"

/* ===========================================================================
 * Small utilities
 * ===========================================================================
 */

/* Timestamped diagnostic to stderr. Not on the data path, so printf cost is fine. */
static void logmsg(const char *fmt, ...)
{
    char ts[16];
    time_t t = time(NULL);
    struct tm tm;
    localtime_r(&t, &tm);
    strftime(ts, sizeof(ts), "%H:%M:%S", &tm);
    fprintf(stderr, "[%s] ", ts);

    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
}

/* Fatal setup error (used only before the event loop starts). */
static void die(const char *msg)
{
    fprintf(stderr, "fatal: %s: %s\n", msg, errno ? strerror(errno) : "");
    exit(1);
}

/* Close an fd if open and poison the slot so we never double-close. Double-close
 * is a real bug in fd-heavy servers: a stale close() can reap a fd number that a
 * newer connection has since been assigned, silently killing the wrong socket. */
static void close_fd(int *fd)
{
    if (*fd >= 0) {
        close(*fd);
        *fd = -1;
    }
}

/* Create a non-blocking, close-on-exec IPv4 TCP socket. SOCK_NONBLOCK|SOCK_CLOEXEC
 * are OR'd into the type (a Linux extension) so we avoid the extra fcntl() calls
 * and, crucially, the race window where a forked child could inherit the fd
 * between socket() and a separate FD_CLOEXEC fcntl(). */
static int new_tcp_socket(void)
{
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (fd < 0)
        logmsg("socket(): %s", strerror(errno));
    return fd;
}

/* Fetch and clear a socket's pending error (SO_ERROR). Used to discover the
 * result of a non-blocking connect() once epoll reports the socket writable:
 * the connect's success/errno is delivered here, not by connect() itself. */
static int socket_error(int fd)
{
    int err = 0;
    socklen_t len = sizeof(err);
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) < 0)
        err = errno;
    return err;
}

/* Split "host:port" at the LAST ':' (so bracketless IPv4 and hostnames work).
 * Returns 0 on success. */
static int split_hostport(const char *spec, char *host, size_t hostsz,
                          char *port, size_t portsz)
{
    const char *colon = strrchr(spec, ':');
    if (!colon || colon == spec)
        return -1;                         /* need "host:port" with a nonempty host */
    size_t hlen = (size_t)(colon - spec);
    if (hlen >= hostsz)
        return -1;
    memcpy(host, spec, hlen);
    host[hlen] = '\0';
    if (strlen(colon + 1) >= portsz)
        return -1;
    strcpy(port, colon + 1);
    return 0;
}

/* Resolve "host" + "port" to a single IPv4 sockaddr. We restrict to AF_INET to
 * keep the teaching core focused; extending to IPv6 is just AF_UNSPEC + storing
 * the sockaddr_in6 (the rest of the proxy is already address-family agnostic
 * because it carries sockaddr_storage + socklen everywhere). */
static int resolve_ipv4(const char *host, const char *port,
                        struct sockaddr_storage *ss, socklen_t *slen)
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo *res = NULL;
    int rc = getaddrinfo(host, port, &hints, &res);
    if (rc != 0) {
        logmsg("resolve %s:%s: %s", host, port, gai_strerror(rc));
        return -1;
    }
    memcpy(ss, res->ai_addr, res->ai_addrlen);
    *slen = res->ai_addrlen;
    freeaddrinfo(res);
    return 0;
}

/* ===========================================================================
 * epoll registration helpers. Every fd carries a struct io_handle* in
 * data.ptr; the dispatcher recovers "what is this fd" from handle->kind.
 * ===========================================================================
 */
static int ep_add(struct lb_ctx *c, int fd, uint32_t events, struct io_handle *h)
{
    struct epoll_event ev;
    ev.events   = events;
    ev.data.ptr = h;
    if (epoll_ctl(c->epfd, EPOLL_CTL_ADD, fd, &ev) < 0) {
        logmsg("epoll add fd %d: %s", fd, strerror(errno));
        return -1;
    }
    return 0;
}

static void ep_del(struct lb_ctx *c, int fd)
{
    /* NULL event is fine on modern kernels. We ignore the return: deleting a fd
     * that was never added (ENOENT) is expected on some cleanup paths. */
    epoll_ctl(c->epfd, EPOLL_CTL_DEL, fd, NULL);
}

/* ===========================================================================
 * Backend selection
 * ===========================================================================
 */

/* Rebuild the consistent-hash ring after any eligibility change. Only needed for
 * policy "hash"; RR and LC consult be_eligible() directly and need no ring. */
static void on_eligibility_change(struct lb_ctx *c)
{
    if (c->policy == POL_HASH)
        ring_build(&c->ring, c->be, c->nbe, RING_VNODES);
}

/* Choose a backend index for a new client, or -1 if none is eligible.
 * `client_ip` is the peer's IPv4 address in NETWORK byte order (the raw
 * s_addr) — we hash its 4 bytes directly, which is endianness-independent and
 * keeps a given client pinned to a backend under policy "hash". */
static int select_backend(struct lb_ctx *c, uint32_t client_ip)
{
    switch (c->policy) {
    case POL_RR: {
        /* Walk the ring of backends starting just past last time; take the first
         * eligible one and park the cursor after it. Skips down/draining ones. */
        for (int k = 0; k < c->nbe; k++) {
            int i = (c->rr_cursor + k) % c->nbe;
            if (be_eligible(&c->be[i])) {
                c->rr_cursor = (i + 1) % c->nbe;
                return i;
            }
        }
        return -1;
    }
    case POL_LC: {
        /* Fewest in-flight connections wins; ties keep the earliest index. This
         * is the same logic as least_conn_pick() in asm/demo.c. */
        int best = -1, bestc = 0;
        for (int i = 0; i < c->nbe; i++) {
            if (!be_eligible(&c->be[i]))
                continue;
            if (best < 0 || c->be[i].active < bestc) {
                best  = i;
                bestc = c->be[i].active;
            }
        }
        return best;
    }
    case POL_HASH: {
        /* Consistent hash: hash the client IP into ring space, walk clockwise to
         * the owning backend. The ring only contains eligible backends, so this
         * never returns a down/draining one. */
        uint32_t key = lb_hash(&client_ip, sizeof(client_ip));
        return ring_lookup(&c->ring, key);
    }
    }
    return -1;
}

/* ===========================================================================
 * Warm connection pool.
 *
 * For an L4 splice proxy each client consumes one backend TCP connection for its
 * whole lifetime, so "pooling" here means PRE-CONNECTING: keep a few idle,
 * already-handshaked sockets to each backend so a new client can start splicing
 * without paying the connect() round-trip. (True request-multiplexed pooling —
 * reusing one backend connection for many client requests — is an L7/HTTP
 * feature and is out of scope for a byte-transparent L4 proxy; see the README.)
 *
 * Idle pooled fds are deliberately NOT registered in epoll: while parked they
 * carry no conn to react to. The tradeoff is that a backend which closes an idle
 * pooled connection goes unnoticed until we pop it and the first splice fails —
 * at which point that conn is torn down normally. A production pool would also
 * watch idle fds for EPOLLRDHUP and validate age; we keep it simple and comment
 * the gap.
 * ===========================================================================
 */
static void pool_refill(struct lb_ctx *c, struct backend *b);

/* Pop a ready-to-use connected fd, or -1 if the pool is empty. */
static int pool_pop(struct backend *b)
{
    if (b->idle_n > 0)
        return b->idle[--b->idle_n];
    return -1;
}

/* Discard the whole pool (called when a backend becomes down/draining). */
static void pool_drain(struct lb_ctx *c, struct backend *b)
{
    while (b->idle_n > 0)
        close_fd(&b->idle[--b->idle_n]);   /* idle fds are not in epoll         */
    if (b->pool_fd >= 0) {
        ep_del(c, b->pool_fd);             /* an in-flight connect IS in epoll  */
        close_fd(&b->pool_fd);
    }
}

/* Start at most one asynchronous pool connect toward `b`, if we are below the
 * warm target and none is already in flight. One-at-a-time keeps a single inline
 * io_handle per backend; the pool fills at one socket per health tick, which is
 * plenty for a teaching build. */
static void pool_refill(struct lb_ctx *c, struct backend *b)
{
    if (c->warm <= 0)                 return;   /* pooling disabled              */
    if (!be_eligible(b))              return;   /* never pre-warm a dead backend */
    if (b->pool_fd >= 0)              return;   /* a connect is already pending  */
    if (b->idle_n >= c->warm)         return;   /* pool already full             */

    int fd = new_tcp_socket();
    if (fd < 0)
        return;

    int r = connect(fd, (struct sockaddr *)&b->addr, b->addrlen);
    if (r == 0) {
        /* Rare: connected synchronously (loopback). Park it as idle right away. */
        if (b->idle_n < POOL_MAX)
            b->idle[b->idle_n++] = fd;
        else
            close_fd(&fd);
        return;
    }
    if (errno == EINPROGRESS) {
        /* Normal: the handshake is underway; epoll will report the socket
         * writable when it finishes (or errored — checked via SO_ERROR). */
        b->pool_fd     = fd;
        b->pool_h.kind = EV_POOL;
        b->pool_h.obj  = b;
        if (ep_add(c, fd, EPOLLOUT | EPOLLET, &b->pool_h) < 0) {
            close_fd(&b->pool_fd);
        }
        return;
    }
    /* Immediate hard failure (e.g. ECONNREFUSED): drop it, let the health
     * checker notice the backend is unwell. */
    close_fd(&fd);
}

/* A pool connect completed (or failed). Finalize it and try to top up further. */
static void on_pool_event(struct lb_ctx *c, struct backend *b, uint32_t events)
{
    int err = socket_error(b->pool_fd);
    ep_del(c, b->pool_fd);

    if (err == 0 && !(events & (EPOLLERR | EPOLLHUP))) {
        if (b->idle_n < POOL_MAX)
            b->idle[b->idle_n++] = b->pool_fd; /* now a ready idle connection    */
        else
            close(b->pool_fd);
        b->pool_fd = -1;
        pool_refill(c, b);                     /* keep filling toward the target */
    } else {
        close_fd(&b->pool_fd);                 /* connect failed; health handles */
    }
}

/* ===========================================================================
 * Health checking: a periodic non-blocking TCP connect probe per backend, with
 * hysteresis so a single blip does not flap a backend in and out of rotation.
 * (An HTTP probe would additionally send "GET /health" and check for "200"; the
 * connect probe is the L4 core and the natural fit for an L4 proxy.)
 * ===========================================================================
 */
static void launch_probe(struct lb_ctx *c, struct backend *b);

/* Apply a probe result, crossing the rise/fall thresholds to flip UP<->DOWN. */
static void probe_result(struct lb_ctx *c, struct backend *b, bool ok)
{
    if (ok) {
        b->fail_count = 0;
        if (b->state == BE_DOWN) {
            if (++b->ok_count >= HEALTH_RISE) {   /* enough good probes in a row */
                b->state    = BE_UP;
                b->ok_count = 0;
                on_eligibility_change(c);
                logmsg("backend %s: UP (re-added to rotation)", b->name);
            }
        } else {
            b->ok_count = 0;
        }
    } else {
        b->ok_count = 0;
        if (b->state == BE_UP) {
            if (++b->fail_count >= HEALTH_FALL) { /* enough bad probes in a row  */
                b->state      = BE_DOWN;
                b->fail_count = 0;
                pool_drain(c, b);                 /* stale warm conns are useless */
                on_eligibility_change(c);
                logmsg("backend %s: DOWN (ejected from rotation)", b->name);
            }
        } else {
            b->fail_count = 0;
        }
    }
}

/* A probe socket became writable (or errored): read the connect result. */
static void on_probe_event(struct lb_ctx *c, struct backend *b, uint32_t events)
{
    int err = socket_error(b->probe_fd);
    bool ok = (err == 0) && !(events & (EPOLLERR | EPOLLHUP));
    ep_del(c, b->probe_fd);
    close_fd(&b->probe_fd);
    probe_result(c, b, ok);
}

/* Kick off one probe toward `b` (no-op if one is already in flight). */
static void launch_probe(struct lb_ctx *c, struct backend *b)
{
    if (b->probe_fd >= 0)
        return;                                  /* previous probe still running */

    int fd = new_tcp_socket();
    if (fd < 0)
        return;

    int r = connect(fd, (struct sockaddr *)&b->addr, b->addrlen);
    if (r == 0) {
        probe_result(c, b, true);                /* connected immediately: OK     */
        close_fd(&fd);
        return;
    }
    if (errno == EINPROGRESS) {
        b->probe_fd     = fd;
        b->probe_h.kind = EV_HEALTH;
        b->probe_h.obj  = b;
        if (ep_add(c, fd, EPOLLOUT | EPOLLET, &b->probe_h) < 0) {
            close_fd(&b->probe_fd);
            probe_result(c, b, false);
        }
        return;
    }
    /* connect() failed outright (ECONNREFUSED, ENETUNREACH, ...): a failed probe. */
    close_fd(&fd);
    probe_result(c, b, false);
}

/* The timerfd fired: run a probe against every backend and top up warm pools. */
static void on_timer(struct lb_ctx *c)
{
    /* Drain the expiration counter or the level-triggered timerfd keeps firing. */
    uint64_t expirations;
    ssize_t r = read(c->timer_fd, &expirations, sizeof(expirations));
    (void)r;                                     /* value unused; the read rearms */

    for (int i = 0; i < c->nbe; i++) {
        launch_probe(c, &c->be[i]);
        pool_refill(c, &c->be[i]);
    }
}

/* ===========================================================================
 * The splice data path
 * ===========================================================================
 */

/* Allocate the two staging pipes and wire the pumps' fds. Returns 0 on success.
 * pipe2(O_NONBLOCK) so splice() with SPLICE_F_NONBLOCK never blocks on a full or
 * empty pipe; O_CLOEXEC so a child never inherits these kernel buffers. */
static int setup_pumps(struct conn *c)
{
    int c2b[2], b2c[2];
    if (pipe2(c2b, O_NONBLOCK | O_CLOEXEC) < 0) {
        logmsg("pipe2: %s", strerror(errno));
        return -1;
    }
    if (pipe2(b2c, O_NONBLOCK | O_CLOEXEC) < 0) {
        logmsg("pipe2: %s", strerror(errno));
        close(c2b[0]);
        close(c2b[1]);
        return -1;
    }

    /* client -> backend: read client, write backend, stage in c2b pipe. */
    c->c2b.src_fd  = c->client_fd;
    c->c2b.dst_fd  = c->backend_fd;
    c->c2b.pipe_rd = c2b[0];
    c->c2b.pipe_wr = c2b[1];

    /* backend -> client: read backend, write client, stage in b2c pipe. */
    c->b2c.src_fd  = c->backend_fd;
    c->b2c.dst_fd  = c->client_fd;
    c->b2c.pipe_rd = b2c[0];
    c->b2c.pipe_wr = b2c[1];
    return 0;
}

/* ---------------------------------------------------------------------------
 * pump — move as many bytes as possible src -> pipe -> dst for one direction.
 *
 * Called whenever EITHER endpoint of the direction becomes ready. Because the
 * hot fds are edge-triggered, we loop until neither half makes progress: draining
 * the pipe into dst frees room, which may let us pull more from src, and vice
 * versa. Stopping early would strand data with no future edge to wake us.
 *
 * The two splice() calls:
 *   (a) splice(src_socket -> pipe_wr): pulls readable socket bytes into the pipe
 *       by page reference (no copy). EAGAIN here means "socket has no more data
 *       right now, OR the pipe is full" — either way we stop and wait for the
 *       next edge (readable on src, or writable on dst which then drains).
 *   (b) splice(pipe_rd -> dst_socket): pushes staged bytes into the destination
 *       socket. EAGAIN means the dst socket's send buffer is full — apply
 *       backpressure: leave the bytes in the pipe and wait for dst to become
 *       writable (its EPOLLOUT edge re-runs this pump).
 *
 * Flags: SPLICE_F_MOVE asks the kernel to move pages rather than copy (a hint;
 * modern kernels effectively always do). SPLICE_F_NONBLOCK makes both splices
 * honour non-blocking semantics regardless of the fds' own flags. SPLICE_F_MORE
 * on the pipe->socket splice is the TCP "more data coming" hint (like MSG_MORE /
 * TCP_CORK): while src is still open we expect to send again soon, so the kernel
 * may coalesce rather than emit a small segment; we drop it once src hits EOF so
 * the final bytes flush promptly.
 * --------------------------------------------------------------------------- */
static void pump(struct pump *p)
{
    if (p->broken)
        return;

    ssize_t moved_total;
    do {
        moved_total = 0;

        /* (a) src socket -> pipe, while src is open and the pipe has room. */
        while (!p->src_closed && p->inpipe < (size_t)PIPE_LEN) {
            size_t room = PIPE_LEN - p->inpipe;
            ssize_t n = splice(p->src_fd, NULL, p->pipe_wr, NULL, room,
                               SPLICE_F_MOVE | SPLICE_F_NONBLOCK);
            if (n > 0) {
                p->inpipe   += (size_t)n;
                moved_total += n;
            } else if (n == 0) {
                p->src_closed = true;            /* orderly EOF from src (FIN)    */
                break;
            } else {
                if (errno == EINTR)
                    continue;                    /* interrupted: just retry       */
                if (errno == EAGAIN)
                    break;                       /* src dry or pipe full: stop     */
                p->broken = true;                /* real error (ECONNRESET, ...)  */
                return;
            }
        }

        /* (b) pipe -> dst socket, while anything is staged. */
        while (p->inpipe > 0) {
            unsigned flags = SPLICE_F_MOVE | SPLICE_F_NONBLOCK;
            if (!p->src_closed)
                flags |= SPLICE_F_MORE;          /* more will follow: coalesce    */
            ssize_t n = splice(p->pipe_rd, NULL, p->dst_fd, NULL, p->inpipe, flags);
            if (n > 0) {
                p->inpipe   -= (size_t)n;
                moved_total += n;
            } else {
                if (errno == EINTR)
                    continue;
                if (errno == EAGAIN)
                    break;                       /* dst send buffer full: backpressure */
                p->broken = true;                /* EPIPE/ECONNRESET: peer gone   */
                return;
            }
        }
    } while (moved_total > 0);                    /* keep cycling while progress   */

    /* EOF propagation: once src is closed AND the pipe is fully flushed, relay the
     * half-close to dst so it learns this direction is finished. We only shut the
     * WRITE side, leaving the other direction (a separate pump) free to keep
     * flowing — TCP half-close, exactly what a transparent proxy must preserve. */
    if (p->src_closed && p->inpipe == 0 && !p->shut_done) {
        shutdown(p->dst_fd, SHUT_WR);            /* sends FIN on dst              */
        p->shut_done = true;
    }
}

/* Forward decls for teardown/reap (used by check_conn). */
static void conn_shutdown(struct lb_ctx *c, struct conn *cn);

/* Decide whether a conn is finished and, if so, tear it down. Returns true if it
 * closed (caller must not use the conn's flow state afterward — memory stays
 * valid until the post-batch reap, but the conn is logically gone). */
static bool check_conn(struct lb_ctx *c, struct conn *cn)
{
    if (cn->c2b.broken || cn->b2c.broken) {
        conn_shutdown(c, cn);                    /* hard error either direction   */
        return true;
    }
    if (cn->c2b.shut_done && cn->b2c.shut_done) {
        conn_shutdown(c, cn);                    /* both directions relayed EOF   */
        return true;
    }
    return false;
}

/* ===========================================================================
 * Connection lifecycle
 * ===========================================================================
 */

/* Tear down a conn: unlink, unregister & close every fd, update accounting, and
 * PARK it on the dead list (freed after the current epoll batch — see the note
 * on struct conn). Idempotent via the `closed` flag. */
static void conn_shutdown(struct lb_ctx *c, struct conn *cn)
{
    if (cn->closed)
        return;
    cn->closed = true;

    /* Unlink from the live list. */
    if (cn->prev) cn->prev->next = cn->next;
    else          c->conns       = cn->next;
    if (cn->next) cn->next->prev = cn->prev;

    /* Remove both fds from epoll before closing them (fd still valid here). */
    ep_del(c, cn->client_fd);
    ep_del(c, cn->backend_fd);

    /* Close the two sockets and all four pipe ends. */
    close_fd(&cn->client_fd);
    close_fd(&cn->backend_fd);
    close_fd(&cn->c2b.pipe_rd);
    close_fd(&cn->c2b.pipe_wr);
    close_fd(&cn->b2c.pipe_rd);
    close_fd(&cn->b2c.pipe_wr);

    /* Backend accounting; announce a completed drain. */
    struct backend *b = &c->be[cn->backend_idx];
    if (b->active > 0)
        b->active--;
    if (b->draining && b->active == 0)
        logmsg("backend %s: fully drained (0 connections)", b->name);

    c->nconns--;

    /* Park for deferred free. */
    cn->dead_next = c->dead;
    c->dead       = cn;
}

/* Free everything parked on the dead list. Called once per epoll batch, AFTER
 * all events in the batch have been dispatched, so no queued event can still
 * point at a conn we are about to free. */
static void reap_dead(struct lb_ctx *c)
{
    struct conn *d = c->dead;
    while (d) {
        struct conn *next = d->dead_next;
        free(d);
        d = next;
    }
    c->dead = NULL;
}

/* Finish a non-blocking backend connect() that epoll just reported writable. */
static void finalize_connect(struct lb_ctx *c, struct conn *cn, uint32_t events)
{
    int err = socket_error(cn->backend_fd);
    if (err != 0 || (events & (EPOLLERR | EPOLLHUP))) {
        logmsg("connect to backend %s failed: %s",
               c->be[cn->backend_idx].name, strerror(err ? err : ECONNREFUSED));
        conn_shutdown(c, cn);
        /* A production LB would retry another backend here; the teaching core
         * simply drops the client so the failure path stays legible. */
        return;
    }
    cn->connecting = false;

    /* The backend is live. Pump both directions now: the client may have sent
     * data during the handshake (we deliberately did not drain it while the
     * backend was still connecting), and there is no pending edge to rely on. */
    pump(&cn->c2b);
    pump(&cn->b2c);
    check_conn(c, cn);
}

/* Dispatch an epoll event on one of a conn's two fds. */
static void on_conn_event(struct lb_ctx *c, struct conn *cn, enum side side,
                          uint32_t events)
{
    if (cn->closed)
        return;                                  /* freed-but-not-yet-reaped: skip */

    if (cn->connecting) {
        /* Still handshaking to the backend. The only event we act on is the
         * backend socket finishing (writable) or erroring; a client-side event
         * is left alone — its data waits in the socket and finalize_connect()
         * will pump it once the backend is up. */
        if (side == SIDE_BACKEND)
            finalize_connect(c, cn, events);
        return;
    }

    /* Normal splicing. Run BOTH directions regardless of which fd woke us: an
     * event on either endpoint can unblock either the read or the write half of
     * either flow, and pump() is cheap and idempotent when there is nothing to
     * do. This keeps the edge-triggered bookkeeping trivially correct. */
    pump(&cn->c2b);
    pump(&cn->b2c);

    /* A hangup or error on a fd means that endpoint is unusable; after the final
     * pump attempt above (to flush any last readable bytes), force teardown. */
    if (events & (EPOLLERR | EPOLLHUP)) {
        cn->c2b.broken = true;
        cn->b2c.broken = true;
    }

    check_conn(c, cn);
}

/* Accept every pending client (edge-triggered listener: drain until EAGAIN). */
static void on_accept(struct lb_ctx *c)
{
    for (;;) {
        struct sockaddr_in peer;
        socklen_t plen = sizeof(peer);
        int cfd = accept4(c->listen_fd, (struct sockaddr *)&peer, &plen,
                          SOCK_NONBLOCK | SOCK_CLOEXEC);
        if (cfd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;                           /* drained the accept queue      */
            if (errno == EINTR)
                continue;
            if (errno == ECONNABORTED)
                continue;                        /* client vanished pre-accept    */
            logmsg("accept4: %s", strerror(errno));
            break;
        }

        /* During graceful shutdown we stop serving new clients. (The listener is
         * normally already closed by then; this guards the race where an event
         * was queued just before we closed it.) */
        if (c->shutting_down) {
            close(cfd);
            continue;
        }

        /* Pick a backend using the client's source IP (network-order s_addr). */
        uint32_t client_ip = peer.sin_addr.s_addr;
        int bidx = select_backend(c, client_ip);
        if (bidx < 0) {
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &peer.sin_addr, ip, sizeof(ip));
            logmsg("no eligible backend for client %s; dropping", ip);
            close(cfd);
            continue;
        }
        struct backend *b = &c->be[bidx];

        /* Obtain a backend fd: reuse a pre-warmed idle connection if we have one,
         * otherwise start a fresh non-blocking connect(). */
        bool connecting = false;
        int bfd = pool_pop(b);
        if (bfd >= 0) {
            connecting = false;                  /* pooled socket already handshook */
        } else {
            bfd = new_tcp_socket();
            if (bfd < 0) {
                close(cfd);
                continue;
            }
            int r = connect(bfd, (struct sockaddr *)&b->addr, b->addrlen);
            if (r == 0) {
                connecting = false;              /* connected synchronously        */
            } else if (errno == EINPROGRESS) {
                connecting = true;               /* handshake underway             */
            } else {
                logmsg("connect to backend %s: %s", b->name, strerror(errno));
                close(bfd);
                close(cfd);
                continue;
            }
        }

        /* Allocate and wire the conn. calloc zeros the flags/counters. */
        struct conn *cn = calloc(1, sizeof(*cn));
        if (!cn) {
            close(cfd);
            close(bfd);
            continue;
        }
        cn->client_fd  = cfd;
        cn->backend_fd = bfd;
        cn->backend_idx = bidx;
        cn->connecting = connecting;

        if (setup_pumps(cn) < 0) {
            close(cfd);
            close(bfd);
            free(cn);
            continue;
        }

        cn->h_client  = (struct io_handle){ EV_CONN, cn, SIDE_CLIENT };
        cn->h_backend = (struct io_handle){ EV_CONN, cn, SIDE_BACKEND };

        /* Link into the live list and bump accounting BEFORE registering epoll,
         * so that if a registration fails, conn_shutdown() runs the single unified
         * cleanup path (unlink, close all fds, decrement active/nconns, park). */
        cn->next = c->conns;
        if (c->conns)
            c->conns->prev = cn;
        c->conns = cn;
        c->nconns++;
        b->active++;

        /* Register both fds edge-triggered. We arm EPOLLOUT permanently: under ET
         * that yields one initial writable edge then further edges only after a
         * write hits EAGAIN, which is exactly the backpressure signal pump wants.
         * EPOLLRDHUP tells us the peer half-closed even while we still have data
         * to send it. */
        uint32_t evmask = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET;
        if (ep_add(c, cfd, evmask, &cn->h_client) < 0) {
            conn_shutdown(c, cn);
            continue;
        }
        if (ep_add(c, bfd, evmask, &cn->h_backend) < 0) {
            conn_shutdown(c, cn);
            continue;
        }

        /* Refill the pool we may have drawn from. */
        pool_refill(c, b);

        /* If the backend is already connected, start moving bytes immediately;
         * otherwise wait for the connect to finish (finalize_connect). */
        if (!connecting) {
            pump(&cn->c2b);
            pump(&cn->b2c);
            if (check_conn(c, cn))
                continue;
        }
    }
}

/* ===========================================================================
 * Signals: graceful whole-process drain on SIGINT/SIGTERM.
 * ===========================================================================
 */
static void begin_shutdown(struct lb_ctx *c)
{
    if (c->shutting_down)
        return;
    c->shutting_down = true;

    /* Stop accepting new clients: unregister and close the listener. Existing
     * conns keep splicing until they finish on their own — that IS the drain. */
    if (c->listen_fd >= 0) {
        ep_del(c, c->listen_fd);
        close_fd(&c->listen_fd);
    }
    logmsg("graceful shutdown: listener closed, draining %d live connection(s)",
           c->nconns);
}

static void on_signal(struct lb_ctx *c)
{
    struct signalfd_siginfo si;
    /* signalfd is level-triggered here; drain all pending signals. */
    while (read(c->sig_fd, &si, sizeof(si)) == (ssize_t)sizeof(si)) {
        logmsg("received signal %u; beginning graceful shutdown", si.ssi_signo);
        begin_shutdown(c);
    }
}

/* ===========================================================================
 * Admin control socket (AF_UNIX): a tiny line protocol for graceful per-backend
 * drain. Commands (one per connection):  status | drain N | undrain N
 * ===========================================================================
 */
static void handle_control_client(struct lb_ctx *c, int fd)
{
    char buf[256];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    if (n <= 0)
        return;
    buf[n] = '\0';

    char cmd[32] = {0};
    int idx = -1;
    int matched = sscanf(buf, "%31s %d", cmd, &idx);
    if (matched < 1)
        return;

    if (strcmp(cmd, "status") == 0) {
        dprintf(fd, "policy=%s warm=%d conns=%d\n",
                c->policy == POL_RR ? "rr" : c->policy == POL_LC ? "lc" : "hash",
                c->warm, c->nconns);
        for (int i = 0; i < c->nbe; i++) {
            struct backend *b = &c->be[i];
            dprintf(fd, "  [%d] %-24s %-5s%s active=%d idle=%d\n",
                    i, b->name,
                    b->state == BE_UP ? "UP" : "DOWN",
                    b->draining ? " DRAINING" : "",
                    b->active, b->idle_n);
        }
    } else if (strcmp(cmd, "drain") == 0 && idx >= 0 && idx < c->nbe) {
        struct backend *b = &c->be[idx];
        b->draining = true;               /* no NEW conns; existing ones finish  */
        pool_drain(c, b);                 /* pre-warmed sockets no longer wanted  */
        on_eligibility_change(c);         /* drop it from the hash ring           */
        dprintf(fd, "ok: draining backend %d (%s), %d conn(s) still open\n",
                idx, b->name, b->active);
        logmsg("admin: draining backend %s (%d live conns)", b->name, b->active);
    } else if (strcmp(cmd, "undrain") == 0 && idx >= 0 && idx < c->nbe) {
        struct backend *b = &c->be[idx];
        b->draining = false;
        on_eligibility_change(c);         /* re-add to the hash ring              */
        dprintf(fd, "ok: undrained backend %d (%s)\n", idx, b->name);
        logmsg("admin: undrained backend %s", b->name);
    } else {
        dprintf(fd, "err: usage: status | drain <idx> | undrain <idx>\n");
    }
}

static void on_control(struct lb_ctx *c)
{
    for (;;) {
        int fd = accept4(c->ctl_fd, NULL, NULL, SOCK_NONBLOCK | SOCK_CLOEXEC);
        if (fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            if (errno == EINTR)
                continue;
            break;
        }
        handle_control_client(c, fd);            /* one request/response per conn */
        close(fd);
    }
}

/* ===========================================================================
 * Setup helpers
 * ===========================================================================
 */
static int make_listener(struct sockaddr_storage *ss, socklen_t slen)
{
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (fd < 0)
        die("listener socket");

    int one = 1;
    /* SO_REUSEADDR lets us rebind the port immediately after a restart instead of
     * waiting out the previous socket's TIME_WAIT. */
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0)
        die("setsockopt SO_REUSEADDR");
    if (bind(fd, (struct sockaddr *)ss, slen) < 0)
        die("bind");
    if (listen(fd, SOMAXCONN) < 0)
        die("listen");
    return fd;
}

static int make_timerfd(int interval_ms)
{
    int fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (fd < 0)
        die("timerfd_create");

    struct itimerspec its;
    its.it_interval.tv_sec  = interval_ms / 1000;
    its.it_interval.tv_nsec = (long)(interval_ms % 1000) * 1000000L;
    its.it_value = its.it_interval;              /* first fire after one interval */
    if (timerfd_settime(fd, 0, &its, NULL) < 0)
        die("timerfd_settime");
    return fd;
}

static int make_signalfd(void)
{
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    /* Block normal delivery so the signals queue onto the signalfd instead of
     * interrupting us at an arbitrary instruction. Now they are just another
     * readable fd in the epoll set — no async-signal-safety minefield. */
    if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0)
        die("sigprocmask");

    int fd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
    if (fd < 0)
        die("signalfd");
    return fd;
}

static int make_control_socket(const char *path)
{
    int fd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (fd < 0)
        die("control socket");

    struct sockaddr_un un;
    memset(&un, 0, sizeof(un));
    un.sun_family = AF_UNIX;
    if (strlen(path) >= sizeof(un.sun_path)) {
        fprintf(stderr, "control socket path too long: %s\n", path);
        exit(1);
    }
    strcpy(un.sun_path, path);

    unlink(path);                                /* remove a stale socket file    */
    if (bind(fd, (struct sockaddr *)&un, sizeof(un)) < 0)
        die("control bind");
    if (listen(fd, 8) < 0)
        die("control listen");
    return fd;
}

static void usage(const char *argv0)
{
    fprintf(stderr,
        "usage: %s -b host:port [-b host:port ...] [options]\n"
        "  -l host:port   listen address           (default 0.0.0.0:8080)\n"
        "  -b host:port   add a backend            (repeatable, required)\n"
        "  -p policy      rr | lc | hash           (default rr)\n"
        "  -w N           warm pool size / backend  (default 0 = off)\n"
        "  -i ms          health-check interval     (default 2000)\n"
        "  -c path        admin control socket path (default /tmp/lb.ctl)\n"
        "  -h             this help\n"
        "\n"
        "policies: rr=round-robin  lc=least-connections  hash=consistent-hash(client IP)\n"
        "control:  socat - UNIX-CONNECT:/tmp/lb.ctl   then: status | drain N | undrain N\n",
        argv0);
}

/* ===========================================================================
 * main
 * ===========================================================================
 */
int main(int argc, char **argv)
{
    struct lb_ctx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.policy    = POL_RR;
    ctx.warm      = 0;
    ctx.health_ms = 2000;
    ctx.ctl_path  = "/tmp/lb.ctl";
    ctx.listen_fd = ctx.timer_fd = ctx.sig_fd = ctx.ctl_fd = ctx.epfd = -1;

    const char *listen_spec = "0.0.0.0:8080";
    const char *backend_specs[MAX_BACKENDS];
    int         nbackends = 0;

    /* --- parse CLI --- */
    int opt;
    while ((opt = getopt(argc, argv, "l:b:p:w:i:c:h")) != -1) {
        switch (opt) {
        case 'l':
            listen_spec = optarg;
            break;
        case 'b':
            if (nbackends >= MAX_BACKENDS) {
                fprintf(stderr, "too many backends (max %d)\n", MAX_BACKENDS);
                return 1;
            }
            backend_specs[nbackends++] = optarg;
            break;
        case 'p':
            if      (strcmp(optarg, "rr")   == 0) ctx.policy = POL_RR;
            else if (strcmp(optarg, "lc")   == 0) ctx.policy = POL_LC;
            else if (strcmp(optarg, "hash") == 0) ctx.policy = POL_HASH;
            else { fprintf(stderr, "unknown policy '%s'\n", optarg); return 1; }
            break;
        case 'w':
            ctx.warm = atoi(optarg);
            if (ctx.warm < 0)        ctx.warm = 0;
            if (ctx.warm > POOL_MAX) ctx.warm = POOL_MAX;
            break;
        case 'i':
            ctx.health_ms = atoi(optarg);
            if (ctx.health_ms < 100) ctx.health_ms = 100;   /* floor: avoid a busy loop */
            break;
        case 'c':
            ctx.ctl_path = optarg;
            break;
        case 'h':
        default:
            usage(argv[0]);
            return opt == 'h' ? 0 : 1;
        }
    }
    if (nbackends == 0) {
        fprintf(stderr, "error: at least one -b backend is required\n\n");
        usage(argv[0]);
        return 1;
    }

    /* --- resolve backends --- */
    ctx.be  = calloc((size_t)nbackends, sizeof(*ctx.be));
    if (!ctx.be)
        die("calloc backends");
    ctx.nbe = nbackends;
    for (int i = 0; i < nbackends; i++) {
        struct backend *b = &ctx.be[i];
        char host[128], port[16];
        if (split_hostport(backend_specs[i], host, sizeof(host),
                           port, sizeof(port)) < 0) {
            fprintf(stderr, "bad backend '%s' (want host:port)\n", backend_specs[i]);
            return 1;
        }
        if (resolve_ipv4(host, port, &b->addr, &b->addrlen) < 0)
            return 1;
        /* Store the ORIGINAL spec as the name: it is both the log id and the
         * consistent-hash label, so the ring is stable across restarts. */
        snprintf(b->name, sizeof(b->name), "%s", backend_specs[i]);
        b->state    = BE_UP;              /* optimistic; health check will correct */
        b->probe_fd = -1;
        b->pool_fd  = -1;
    }

    /* --- resolve listen address --- */
    struct sockaddr_storage lss;
    socklen_t lslen;
    {
        char host[128], port[16];
        if (split_hostport(listen_spec, host, sizeof(host), port, sizeof(port)) < 0) {
            fprintf(stderr, "bad listen address '%s'\n", listen_spec);
            return 1;
        }
        if (resolve_ipv4(host, port, &lss, &lslen) < 0)
            return 1;
    }

    /* Ignore SIGPIPE process-wide: writing (or splicing) to a socket whose peer
     * has closed its read side would otherwise kill us with SIGPIPE. We want the
     * error reported inline as EPIPE from splice() so pump() can tear that one
     * connection down and keep serving everyone else. */
    signal(SIGPIPE, SIG_IGN);

    /* --- build epoll set and all the fds it multiplexes --- */
    ctx.epfd = epoll_create1(EPOLL_CLOEXEC);
    if (ctx.epfd < 0)
        die("epoll_create1");

    ctx.listen_fd = make_listener(&lss, lslen);
    ctx.timer_fd  = make_timerfd(ctx.health_ms);
    ctx.sig_fd    = make_signalfd();
    ctx.ctl_fd    = make_control_socket(ctx.ctl_path);

    ctx.h_listen  = (struct io_handle){ EV_LISTEN,  NULL, 0 };
    ctx.h_timer   = (struct io_handle){ EV_TIMER,   NULL, 0 };
    ctx.h_signal  = (struct io_handle){ EV_SIGNAL,  NULL, 0 };
    ctx.h_control = (struct io_handle){ EV_CONTROL, NULL, 0 };

    /* Control fds are level-triggered (low rate; simplest correct behaviour).
     * The client listener is level-triggered too — on_accept still drains fully,
     * so it works either way, and LT avoids a lost-accept edge case. */
    if (ep_add(&ctx, ctx.listen_fd, EPOLLIN, &ctx.h_listen)  < 0) die("epoll add listener");
    if (ep_add(&ctx, ctx.timer_fd,  EPOLLIN, &ctx.h_timer)   < 0) die("epoll add timer");
    if (ep_add(&ctx, ctx.sig_fd,    EPOLLIN, &ctx.h_signal)  < 0) die("epoll add signalfd");
    if (ep_add(&ctx, ctx.ctl_fd,    EPOLLIN, &ctx.h_control) < 0) die("epoll add control");

    if (ctx.policy == POL_HASH)
        ring_build(&ctx.ring, ctx.be, ctx.nbe, RING_VNODES);

    logmsg("listening on %s, %d backend(s), policy=%s, warm=%d, health=%dms",
           listen_spec, ctx.nbe,
           ctx.policy == POL_RR ? "rr" : ctx.policy == POL_LC ? "lc" : "hash",
           ctx.warm, ctx.health_ms);

    /* --- the event loop --- */
    struct epoll_event evs[MAX_EVENTS];
    for (;;) {
        /* Graceful shutdown completes once every drained connection is gone. */
        if (ctx.shutting_down && ctx.nconns == 0)
            break;

        int n = epoll_wait(ctx.epfd, evs, MAX_EVENTS, -1);
        if (n < 0) {
            if (errno == EINTR)
                continue;                        /* a stray signal: just re-wait  */
            logmsg("epoll_wait: %s", strerror(errno));
            break;
        }

        for (int i = 0; i < n; i++) {
            struct io_handle *h = (struct io_handle *)evs[i].data.ptr;
            uint32_t          e = evs[i].events;
            switch (h->kind) {
            case EV_LISTEN:  on_accept(&ctx);                                    break;
            case EV_TIMER:   on_timer(&ctx);                                     break;
            case EV_SIGNAL:  on_signal(&ctx);                                    break;
            case EV_CONTROL: on_control(&ctx);                                   break;
            case EV_HEALTH:  on_probe_event(&ctx, (struct backend *)h->obj, e);  break;
            case EV_POOL:    on_pool_event(&ctx, (struct backend *)h->obj, e);   break;
            case EV_CONN:    on_conn_event(&ctx, (struct conn *)h->obj, h->side, e); break;
            }
        }

        /* Free conns closed during this batch, now that no queued event in the
         * batch can still reference them. */
        reap_dead(&ctx);
    }

    /* --- shutdown cleanup: close everything, unlink the control socket --- */
    logmsg("shutting down");
    while (ctx.conns)
        conn_shutdown(&ctx, ctx.conns);          /* force-close any stragglers    */
    reap_dead(&ctx);

    for (int i = 0; i < ctx.nbe; i++) {
        pool_drain(&ctx, &ctx.be[i]);
        if (ctx.be[i].probe_fd >= 0) {
            ep_del(&ctx, ctx.be[i].probe_fd);
            close_fd(&ctx.be[i].probe_fd);
        }
    }
    close_fd(&ctx.listen_fd);
    close_fd(&ctx.timer_fd);
    close_fd(&ctx.sig_fd);
    close_fd(&ctx.ctl_fd);
    unlink(ctx.ctl_path);
    close_fd(&ctx.epfd);
    ring_free(&ctx.ring);
    free(ctx.be);
    return 0;
}
