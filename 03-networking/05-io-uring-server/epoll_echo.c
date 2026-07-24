/* ===========================================================================
 * epoll_echo.c — the SAME TCP echo server, written the classic epoll way.
 * ===========================================================================
 *
 * This exists to be the CONTROL in the experiment. It speaks the identical
 * protocol as echo_uring.c (read bytes, write them back), so the same
 * bench_client.c drives both, and `strace -c` on each reveals the difference
 * that io_uring is *about*: syscalls per message.
 *
 * The fundamental epoll contract: epoll only reports READINESS. It never moves
 * bytes. For every notification you still issue the read()/write()/accept()
 * syscall yourself. So a single echo exchange costs, at minimum:
 *
 *     epoll_wait  (learn the fd is readable)
 *     accept/read (actually get the bytes)         <- a syscall
 *     write       (send them back)                  <- a syscall
 *
 * io_uring collapses that: the kernel performs the read AND the write and
 * reports both results, and multishot means you do not even re-arm. The README
 * tallies the counts; this file is the baseline those counts are measured
 * against.
 *
 * DESIGN: single-threaded, LEVEL-triggered epoll (the default). Level-triggered
 * keeps this file short and obviously correct — if a fd stays readable, epoll
 * keeps telling us, so we may read one chunk per wakeup instead of draining in a
 * loop. To avoid a busy-spin while a slow client's socket buffer is full, we
 * flip our interest between EPOLLIN and EPOLLOUT rather than polling a ready fd
 * we cannot make progress on. Edge-triggered (EPOLLET) would cut epoll_wait
 * wakeups further but REQUIRES draining each fd to EAGAIN every time; that
 * tradeoff is discussed in the README.
 *
 * PLATFORM: Linux (epoll is Linux-specific). No privilege needed.
 * ===========================================================================
 */
#define _GNU_SOURCE      /* expose accept4() + SOCK_NONBLOCK from <sys/socket.h> */
#include "common.h"
#include <sys/epoll.h>   /* epoll_create1, epoll_ctl, epoll_wait, struct epoll_event */
#include <stdint.h>

#define MAX_EVENTS   256      /* epoll_wait batch size                          */
#define CHUNK        2048     /* bytes we read/echo at a time                   */
#define MAX_FDS      65536    /* size of our fd-indexed connection table        */

/* Per-connection state. We index this table directly by fd (fds are small dense
 * integers the kernel hands out from the lowest free slot, so an array beats a
 * hash map). A connection has at most ONE CHUNK of pending output at a time —
 * that bound is what makes the memory usage predictable (see the read logic). */
struct conn {
    int    in_use;             /* is this slot an open connection?              */
    int    want_out;           /* are we currently registered for EPOLLOUT?     */
    size_t out_len;            /* bytes of echo still owed to the client        */
    size_t out_off;            /* how many of them we have already written      */
    char   out[CHUNK];         /* the pending echo bytes                        */
};

static struct conn conns[MAX_FDS];  /* ~130 MiB of BSS; zero-init, never freed  */
static int epfd;                    /* the epoll instance fd                    */

/* mod_interest — reprogram which readiness events we want for `fd`.
 * epoll_ctl(EPOLL_CTL_MOD) rewrites the interest mask. We stash the fd in
 * ev.data.fd so epoll_wait hands it straight back to us. EPOLLIN = "readable",
 * EPOLLOUT = "writable"; we toggle EPOLLOUT on only while we owe output, so a
 * fd with a full send buffer does not spin epoll_wait returning EPOLLIN we
 * cannot service. */
static void mod_interest(int fd, uint32_t events)
{
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events   = events;
    ev.data.fd  = fd;
    if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) < 0)
        perror("epoll_ctl(MOD)");   /* per-fd: log, do not kill the server      */
}

/* close_conn — tear a connection down and forget its state.
 * epoll_ctl(EPOLL_CTL_DEL) removes it from the interest set (the kernel also
 * auto-removes a fd when it is closed, but being explicit is clearer), then
 * close(2) releases the fd. */
static void close_conn(int fd)
{
    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);   /* NULL event ok for DEL        */
    close(fd);
    conns[fd].in_use   = 0;
    conns[fd].want_out = 0;
    conns[fd].out_len  = 0;
    conns[fd].out_off  = 0;
}

/* flush_out — try to push the pending echo bytes to the client.
 * Returns 0 if all pending output drained, 1 if bytes remain (socket buffer
 * full), -1 on a fatal socket error (caller should close). */
static int flush_out(int fd)
{
    struct conn *c = &conns[fd];
    while (c->out_off < c->out_len) {
        /* write(2): push [out_off, out_len) to the socket. On a non-blocking
         * socket a full kernel send buffer yields -1/EAGAIN (a.k.a. EWOULDBLOCK
         * — identical value on Linux), which is NOT an error: it means "come
         * back when EPOLLOUT fires." A partial write (n < remaining) is normal
         * and we simply advance out_off. */
        ssize_t n = write(fd, c->out + c->out_off, c->out_len - c->out_off);
        if (n > 0) {
            c->out_off += (size_t)n;
            continue;                        /* maybe more room; loop again      */
        }
        if (n < 0) {
            if (errno == EINTR)
                continue;                    /* interrupted by a signal; retry   */
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return 1;                    /* buffer full: wait for EPOLLOUT    */
            return -1;                        /* EPIPE/ECONNRESET/...: fatal      */
        }
        /* n == 0 from write() does not happen for a stream socket; treat defensively. */
        return 1;
    }
    /* Drained. Reset so the slot can hold the next chunk. */
    c->out_off = 0;
    c->out_len = 0;
    return 0;
}

/* handle_readable — a client fd reported EPOLLIN. Read ONE chunk and echo it.
 *
 * We only read when we have no pending output (out_len == 0). If output is
 * still owed, we have already switched interest to EPOLLOUT-only, so EPOLLIN
 * will not even fire — but we guard anyway. Reading one chunk per wakeup (rather
 * than looping to EAGAIN) is the level-triggered idiom: leftover readable data
 * simply triggers another epoll_wait return. */
static void handle_readable(int fd)
{
    struct conn *c = &conns[fd];
    if (c->out_len != 0)
        return;                              /* still draining a prior echo      */

    /* read(2): pull up to CHUNK bytes into our per-conn buffer.
     *   n  > 0 : that many bytes of data.
     *   n == 0 : orderly EOF (peer sent FIN) -> close.
     *   n  < 0 : EAGAIN (nothing left right now, not an error) or a real error. */
    ssize_t n = read(fd, c->out, CHUNK);
    if (n == 0) {
        close_conn(fd);
        return;
    }
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
            return;                          /* spurious wakeup / retry later    */
        perror("read");
        close_conn(fd);
        return;
    }

    /* We have n bytes to echo. Try to write them immediately — the common case
     * is the socket has room and this one write() completes the echo with no
     * EPOLLOUT round trip at all. */
    c->out_len = (size_t)n;
    c->out_off = 0;
    int fr = flush_out(fd);
    if (fr < 0) {
        close_conn(fd);
    } else if (fr == 1) {
        /* Could not send it all. Register for EPOLLOUT and DROP EPOLLIN: we must
         * not read more until this chunk drains (that is what bounds memory to
         * one CHUNK per connection), and dropping EPOLLIN stops epoll_wait from
         * spinning on the still-readable socket. */
        c->want_out = 1;
        mod_interest(fd, EPOLLOUT);
    }
    /* fr == 0: fully echoed in-line; interest stays EPOLLIN, nothing to do. */
}

/* handle_writable — the socket reported EPOLLOUT: send buffer has room again. */
static void handle_writable(int fd)
{
    struct conn *c = &conns[fd];
    int fr = flush_out(fd);
    if (fr < 0) {
        close_conn(fd);
        return;
    }
    if (fr == 0 && c->want_out) {
        /* Output drained: go back to waiting for input only. */
        c->want_out = 0;
        mod_interest(fd, EPOLLIN);
    }
    /* fr == 1: still more to send; stay armed for EPOLLOUT. */
}

/* handle_accept — the listener is readable: drain the accept backlog.
 * Because the listener is EDGE-ish here (we loop until EAGAIN) we accept every
 * pending connection in one go, then register each for EPOLLIN. */
static void handle_accept(int listen_fd)
{
    for (;;) {
        /* accept4(2) with SOCK_NONBLOCK sets O_NONBLOCK on the new fd atomically,
         * saving the extra fcntl() a plain accept()+set_nonblocking() would cost.
         * Returns the new fd, or -1/EAGAIN once the backlog is drained. */
        int cfd = accept4(listen_fd, NULL, NULL, SOCK_NONBLOCK);
        if (cfd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;                       /* backlog empty: done accepting    */
            if (errno == EINTR)
                continue;
            if (errno == ECONNABORTED)
                continue;                    /* client vanished mid-handshake    */
            perror("accept4");
            break;
        }
        if (cfd >= MAX_FDS) {
            /* Our fd-indexed table cannot hold this fd. A real server would grow
             * the table; here we refuse rather than index out of bounds. */
            fprintf(stderr, "fd %d exceeds MAX_FDS; dropping\n", cfd);
            close(cfd);
            continue;
        }

        (void)set_nodelay(cfd);              /* fair comparison with echo_uring  */

        conns[cfd].in_use   = 1;
        conns[cfd].want_out = 0;
        conns[cfd].out_len  = 0;
        conns[cfd].out_off  = 0;

        /* Register the new connection for readability. */
        struct epoll_event ev;
        memset(&ev, 0, sizeof(ev));
        ev.events  = EPOLLIN;
        ev.data.fd = cfd;
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev) < 0) {
            perror("epoll_ctl(ADD)");
            close(cfd);
            conns[cfd].in_use = 0;
        }
    }
}

int main(int argc, char **argv)
{
    int port = (argc > 1) ? atoi(argv[1]) : 8080;

    /* Non-blocking listener: handle_accept() loops accept4() until EAGAIN. */
    int listen_fd = make_listener(port, 1);

    /* epoll_create1(2): create an epoll instance. EPOLL_CLOEXEC closes it across
     * exec(); flags=0 works too. Returns the epoll fd. */
    epfd = epoll_create1(EPOLL_CLOEXEC);
    if (epfd < 0)
        die("epoll_create1");

    /* Register the listener for EPOLLIN ("a connection is waiting to be
     * accepted" presents as the listen socket becoming readable). */
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events  = EPOLLIN;
    ev.data.fd = listen_fd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev) < 0)
        die("epoll_ctl(ADD listener)");

    fprintf(stderr, "epoll_echo: listening on :%d  (epoll, level-triggered)\n", port);

    struct epoll_event events[MAX_EVENTS];
    for (;;) {
        /* epoll_wait(2): block until at least one registered fd is ready (or a
         * signal arrives). Returns the number of ready events, filling `events`.
         * timeout = -1 means "wait indefinitely". This is the epoll analogue of
         * io_uring_submit_and_wait — but note it ONLY reports readiness; every
         * ready fd below still costs us its own read()/write() syscall. */
        int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (n < 0) {
            if (errno == EINTR)
                continue;                    /* signal; just re-enter            */
            die("epoll_wait");
        }

        for (int i = 0; i < n; i++) {
            int fd = events[i].data.fd;
            uint32_t re = events[i].events;

            if (fd == listen_fd) {
                handle_accept(listen_fd);
                continue;
            }

            /* EPOLLERR / EPOLLHUP: the connection errored or the peer hung up
             * (half-close of both directions). These can arrive alongside or
             * instead of EPOLLIN. The cheapest correct action is to close. */
            if (re & (EPOLLERR | EPOLLHUP)) {
                close_conn(fd);
                continue;
            }
            if (re & EPOLLOUT)
                handle_writable(fd);
            if (re & EPOLLIN)
                handle_readable(fd);
        }
    }

    /* Unreachable in the normal loop; present for completeness. */
    close(epfd);
    close(listen_fd);
    return 0;
}
