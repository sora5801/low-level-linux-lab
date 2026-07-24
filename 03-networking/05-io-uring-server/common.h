/* ===========================================================================
 * common.h — the two bits of plumbing both servers share: a TCP listener and
 *            a fatal-error helper. Deliberately header-only (static inline) so
 *            each server stays a single self-contained translation unit.
 * ===========================================================================
 *
 * Nothing here is io_uring- or epoll-specific: it is the "get me a listening
 * socket" boilerplate that every network server repeats. We factor it out so the
 * interesting files (echo_uring.c, epoll_echo.c) start at the event loop, which
 * is the part worth reading. Every syscall is commented with its number, the
 * argument order the kernel expects, and the errors we actually care about.
 * ===========================================================================
 */
#ifndef IO_URING_LAB_COMMON_H
#define IO_URING_LAB_COMMON_H

#include <stdio.h>       /* fprintf, perror                                    */
#include <stdlib.h>      /* exit                                               */
#include <string.h>      /* memset, strerror                                   */
#include <errno.h>       /* errno, E* constants                                */
#include <unistd.h>      /* close                                             */
#include <fcntl.h>       /* fcntl, O_NONBLOCK                                  */
#include <netinet/in.h>  /* sockaddr_in, htons, INADDR_ANY                    */
#include <netinet/tcp.h> /* TCP_NODELAY                                        */
#include <sys/socket.h>  /* socket, bind, listen, setsockopt, SO_REUSEADDR    */
#include <arpa/inet.h>   /* htons / htonl (host<->network byte order)         */

/* die() — print context + the errno string and abort. We use it only for
 * setup-time failures (socket/bind/listen), where there is nothing sane to do
 * but stop. The event loops below NEVER call die() on a per-connection error —
 * one broken client must not take the server down — they log and close instead. */
static inline void die(const char *what)
{
    /* strerror(errno) turns the numeric errno the failed syscall left behind
     * into text ("Address already in use", etc). We print to stderr (fd 2). */
    fprintf(stderr, "fatal: %s: %s\n", what, strerror(errno));
    exit(1);
}

/* set_nonblocking(fd) — flip O_NONBLOCK on an existing fd.
 *
 * WHY: an epoll/event-loop server must never block in read()/write()/accept().
 * A blocking accept() on a "ready" socket can still stall if the client RSTs
 * between the readiness notification and our syscall (a classic thundering-herd
 * race); a blocking read() on a slow client would freeze the whole single-
 * threaded loop. O_NONBLOCK makes those calls return -1/EAGAIN instead of
 * sleeping, which is the contract the event loop is built around.
 *
 * fcntl(2): fcntl(fd, F_GETFL) reads the current status flags; we OR in
 * O_NONBLOCK and write them back with F_SETFL. We read-modify-write rather than
 * blindly setting O_NONBLOCK so we do not clobber other flags (e.g. O_APPEND). */
static inline int set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0)
        return -1;                       /* caller decides how fatal this is    */
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
        return -1;
    return 0;
}

/* make_listener(port, nonblock) — create, bind, and listen on a TCP socket.
 *
 * Returns the listening fd, or calls die() (setup failure is unrecoverable).
 * `nonblock` lets the caller choose: the epoll server wants a non-blocking
 * listener (it accept()s in a loop until EAGAIN); the io_uring server is happy
 * with a blocking listener because it never calls accept() itself — the kernel
 * performs the accept asynchronously for a multishot ACCEPT SQE. */
static inline int make_listener(int port, int nonblock)
{
    /* socket(2): domain=AF_INET (IPv4), type=SOCK_STREAM (TCP), protocol=0
     * (let the kernel pick TCP for a STREAM/INET socket). Returns a new fd or
     * -1/errno. This fd is just an endpoint; it is not yet bound to any port. */
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd < 0)
        die("socket");

    /* SO_REUSEADDR: allow bind() to succeed even while a previous incarnation of
     * this port lingers in TIME_WAIT. Without it, restarting the server within
     * ~60s of a busy shutdown fails with EADDRINUSE. setsockopt takes the option
     * value by pointer + length, hence &one/sizeof(one). */
    int one = 1;
    if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0)
        die("setsockopt(SO_REUSEADDR)");

    /* Build the bind address. sockaddr_in is the IPv4 flavour of sockaddr.
     *   sin_family = AF_INET                    (must match the socket domain)
     *   sin_port   = htons(port)   <-- BYTE ORDER: the wire (and the kernel's
     *                socket layer) expect the port as a 16-bit BIG-ENDIAN value.
     *                htons ("host to network short") byte-swaps on little-endian
     *                x86 so that port 8080 (0x1F90) is laid down as 1F 90, the
     *                order a peer and every RFC read it in. Forgetting htons is
     *                the canonical "why is it listening on port 36895?" bug.
     *   sin_addr   = htonl(INADDR_ANY) = 0.0.0.0 = bind every local interface. */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));      /* zero sin_zero padding too           */
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons((unsigned short)port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    /* bind(2): attach the socket to (0.0.0.0:port). Fails with EADDRINUSE if the
     * port is taken, EACCES for privileged ports (<1024) without CAP_NET_BIND. */
    if (bind(lfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        die("bind");

    /* listen(2): mark the socket passive and size the accept backlog. The
     * backlog bounds the completed-handshake queue the kernel holds for us
     * between our accept()s; SOMAXCONN (typically 4096) is the sane ceiling. */
    if (listen(lfd, 512) < 0)
        die("listen");

    if (nonblock && set_nonblocking(lfd) < 0)
        die("fcntl(O_NONBLOCK) on listener");

    return lfd;
}

/* set_nodelay(fd) — disable Nagle's algorithm on a connected TCP socket.
 *
 * Nagle coalesces small writes to cut header overhead, but it deliberately
 * holds a sub-MSS segment until the previous one is ACKed. For a request/reply
 * echo workload that adds a whole RTT of latency per exchange and wrecks the
 * throughput number. TCP_NODELAY turns it off so each echo goes out immediately.
 * Best-effort: a failure here is not fatal, so callers may ignore the result. */
static inline int set_nodelay(int fd)
{
    int one = 1;
    return setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
}

#endif /* IO_URING_LAB_COMMON_H */
