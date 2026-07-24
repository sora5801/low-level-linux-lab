/* ===========================================================================
 * net.c — DNS over UDP and TCP, with poll(2)-based timeouts and retries.
 * ===========================================================================
 *
 * This file is where the wire bytes actually meet the kernel. Every syscall is
 * annotated with its argument order and the errors we handle, because the error
 * paths ARE the lesson: a resolver that ignores EINTR, EAGAIN, or partial I/O
 * is subtly broken in ways that only show up under load or packet loss.
 * ===========================================================================
 */
#include "net.h"
#include "dns.h"

#include <string.h>       /* memcpy, memset                                    */
#include <errno.h>        /* errno, EINTR, EAGAIN                              */
#include <unistd.h>       /* close, read, write                               */
#include <poll.h>         /* poll(2) — our timeout mechanism                   */
#include <arpa/inet.h>    /* inet_pton                                         */
#include <netinet/in.h>   /* sockaddr_in, sockaddr_in6, IPPROTO_*             */
#include <sys/socket.h>   /* socket, sendto, recvfrom, connect               */
#include <time.h>         /* clock_gettime for the TCP deadline               */

/* ---------------------------------------------------------------------------
 * dns_addr_from_ip — turn "1.2.3.4" or "2001:db8::1" into a sockaddr.
 *
 * inet_pton(af, src, dst) parses a NUMERIC address of family `af` into the
 * raw network-order bytes at `dst`. It returns 1 on success, 0 if the string
 * is not a valid address of that family, -1 if `af` is unsupported. We try v4
 * first, then v6, so callers can pass either.
 * ------------------------------------------------------------------------- */
int dns_addr_from_ip(const char *ip, uint16_t port,
                     struct sockaddr_storage *ss, socklen_t *slen)
{
    memset(ss, 0, sizeof(*ss));

    /* Try IPv4. */
    struct sockaddr_in *v4 = (struct sockaddr_in *)ss;
    if (inet_pton(AF_INET, ip, &v4->sin_addr) == 1) {
        v4->sin_family = AF_INET;
        /* Port is a 16-bit field stored in NETWORK byte order in the sockaddr,
         * so htons() converts our host-order port. This is the same big-endian
         * discipline as the DNS wire format, just applied by the C library. */
        v4->sin_port = htons(port);
        *slen = sizeof(*v4);
        return 0;
    }

    /* Try IPv6. */
    struct sockaddr_in6 *v6 = (struct sockaddr_in6 *)ss;
    if (inet_pton(AF_INET6, ip, &v6->sin6_addr) == 1) {
        v6->sin6_family = AF_INET6;
        v6->sin6_port = htons(port);
        *slen = sizeof(*v6);
        return 0;
    }

    return -1;   /* neither family parsed it */
}

/* ---------------------------------------------------------------------------
 * poll_one — wait up to `timeout_ms` for `events` on `fd`, restarting on EINTR.
 *
 * poll(2): poll(struct pollfd *fds, nfds_t n, int timeout_ms). It returns the
 * number of ready fds (>0), 0 on timeout, or -1 with errno on error. A signal
 * can interrupt it (errno==EINTR); the correct behavior is to resume waiting
 * for the REMAINING time, not to fail. We recompute the remaining timeout from
 * a monotonic clock so repeated EINTRs cannot extend the deadline forever.
 * Returns 1 if ready, 0 on timeout, -1 on real error.
 * ------------------------------------------------------------------------- */
static int poll_one(int fd, short events, int timeout_ms)
{
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);   /* MONOTONIC: immune to clock set */

    for (;;) {
        struct pollfd pfd;
        pfd.fd     = fd;
        pfd.events = events;
        pfd.revents = 0;

        int n = poll(&pfd, 1, timeout_ms);
        if (n > 0) {
            /* Even when poll says "ready", the fd can be ready for ERROR
             * (POLLERR) or HANGUP (POLLHUP) rather than the event we asked
             * for. Treat those as failure so the caller doesn't spin. */
            if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) return -1;
            return 1;
        }
        if (n == 0) return 0;                 /* timed out                     */

        if (errno == EINTR) {
            /* Interrupted by a signal. Recompute how much of the budget is
             * left and poll again for only that long. */
            struct timespec now;
            clock_gettime(CLOCK_MONOTONIC, &now);
            long elapsed_ms = (now.tv_sec - start.tv_sec) * 1000L +
                              (now.tv_nsec - start.tv_nsec) / 1000000L;
            long remaining = (long)timeout_ms - elapsed_ms;
            if (remaining <= 0) return 0;     /* budget exhausted -> timeout   */
            timeout_ms = (int)remaining;
            continue;
        }
        return -1;                            /* a genuine poll error          */
    }
}

/* ===========================================================================
 * dns_send_udp — one UDP exchange.
 * ===========================================================================
 * socket(family, SOCK_DGRAM, 0)  -> a connectionless UDP socket.
 * We connect(2) it to the server so that (a) sendto/recv need no address and
 * (b) the kernel filters incoming datagrams to those from THIS peer — a cheap
 * defense against off-path answers from a wrong source. Then poll for the
 * reply with a timeout and recv it.
 * =========================================================================== */
int dns_send_udp(const struct sockaddr_storage *server, socklen_t slen,
                 const uint8_t *query, size_t qlen,
                 uint8_t *resp, size_t resp_cap, size_t *resp_len,
                 int timeout_ms)
{
    /* socket(): args (domain, type, protocol). Returns an fd or -1/errno. */
    int fd = socket(server->ss_family, SOCK_DGRAM, IPPROTO_UDP);
    if (fd < 0) return -1;

    int rc = -1;   /* pessimistic default; every success path sets rc = 0     */

    /* connect() a datagram socket: no handshake, just records the peer so the
     * kernel drops datagrams from anyone else. Returns 0 or -1/errno. */
    if (connect(fd, (const struct sockaddr *)server, slen) < 0)
        goto out;

    /* send(): like write() but for sockets. On a connected UDP socket the whole
     * datagram goes in one call or not at all (no partial datagrams), so we do
     * not loop. Returns bytes sent or -1/errno. EINTR is possible before any
     * data is queued; retry once via the loop. */
    for (;;) {
        ssize_t s = send(fd, query, qlen, 0);
        if (s == (ssize_t)qlen) break;        /* whole datagram queued         */
        if (s < 0 && errno == EINTR) continue;/* interrupted before send; retry*/
        goto out;                             /* short/failed send: give up    */
    }

    /* Wait for the reply (or time out). */
    int ready = poll_one(fd, POLLIN, timeout_ms);
    if (ready != 1) goto out;                 /* 0 = timeout, -1 = error       */

    /* recv() the reply datagram. A single recv returns one whole datagram;
     * anything beyond resp_cap is silently discarded by the kernel (which is
     * fine — a too-big UDP answer should have set TC and we fall back to TCP). */
    for (;;) {
        ssize_t got = recv(fd, resp, resp_cap, 0);
        if (got >= 0) { *resp_len = (size_t)got; rc = 0; break; }
        if (errno == EINTR) continue;         /* interrupted; try again        */
        goto out;                             /* real recv error               */
    }

out:
    /* Always close the fd — leaking sockets is how a long-running resolver
     * runs out of file descriptors. close() can itself be interrupted, but on
     * Linux the fd is released regardless, so we do not loop on EINTR. */
    close(fd);
    return rc;
}

/* Read exactly `n` bytes from a TCP stream into `buf`, honoring a deadline.
 * TCP is a byte stream: a single read() may return fewer bytes than asked, so
 * large reads MUST loop. Returns 0 on success, -1 on error/timeout/EOF. */
static int read_full(int fd, uint8_t *buf, size_t n, int timeout_ms)
{
    size_t off = 0;
    while (off < n) {
        int ready = poll_one(fd, POLLIN, timeout_ms);
        if (ready != 1) return -1;            /* timeout or error              */

        ssize_t got = read(fd, buf + off, n - off);
        if (got > 0) { off += (size_t)got; continue; }
        if (got == 0) return -1;              /* peer closed early = truncated */
        if (errno == EINTR) continue;         /* interrupted; retry            */
        return -1;                            /* real read error               */
    }
    return 0;
}

/* Write exactly `n` bytes to a TCP stream. write() may accept fewer than asked
 * (kernel send buffer full), so we loop until all bytes are handed off. */
static int write_full(int fd, const uint8_t *buf, size_t n, int timeout_ms)
{
    size_t off = 0;
    while (off < n) {
        int ready = poll_one(fd, POLLOUT, timeout_ms);
        if (ready != 1) return -1;

        ssize_t put = write(fd, buf + off, n - off);
        if (put > 0) { off += (size_t)put; continue; }
        if (put < 0 && errno == EINTR) continue;
        return -1;
    }
    return 0;
}

/* ===========================================================================
 * dns_send_tcp — one TCP exchange with 2-byte length framing.
 * ===========================================================================
 * Over TCP a DNS message is prefixed by its length as a 16-bit big-endian
 * integer (RFC 1035 §4.2.2), because a stream has no message boundaries. We
 * send [len_hi len_lo query...], then read the 2-byte length of the reply,
 * then exactly that many bytes.
 * =========================================================================== */
int dns_send_tcp(const struct sockaddr_storage *server, socklen_t slen,
                 const uint8_t *query, size_t qlen,
                 uint8_t *resp, size_t resp_cap, size_t *resp_len,
                 int timeout_ms)
{
    if (qlen > 0xFFFF) return -1;             /* length must fit the 16-bit hdr */

    int fd = socket(server->ss_family, SOCK_STREAM, IPPROTO_TCP);
    if (fd < 0) return -1;

    int rc = -1;

    /* connect() performs the TCP 3-way handshake. It can block; for a teaching
     * core we accept a blocking connect (the resolver's overall attempt budget
     * bounds total time). Returns 0 or -1/errno. */
    if (connect(fd, (const struct sockaddr *)server, slen) < 0)
        goto out;

    /* Frame and send: 2-byte length prefix (big-endian) then the message. */
    uint8_t prefix[2];
    prefix[0] = (uint8_t)(qlen >> 8);
    prefix[1] = (uint8_t)(qlen & 0xFF);
    if (write_full(fd, prefix, 2, timeout_ms) < 0)      goto out;
    if (write_full(fd, query, qlen, timeout_ms) < 0)    goto out;

    /* Read the reply's 2-byte length prefix, then the body. */
    uint8_t rlen_buf[2];
    if (read_full(fd, rlen_buf, 2, timeout_ms) < 0)     goto out;
    size_t rlen = ((size_t)rlen_buf[0] << 8) | rlen_buf[1];
    if (rlen == 0 || rlen > resp_cap)                   goto out;

    if (read_full(fd, resp, rlen, timeout_ms) < 0)      goto out;
    *resp_len = rlen;
    rc = 0;

out:
    close(fd);
    return rc;
}

/* ===========================================================================
 * dns_exchange — UDP first (with retries), TCP fallback on truncation.
 * ===========================================================================
 * This mirrors what a real resolver does per RFC 1035 §4.2.1 and §7.3:
 *   1. Try UDP up to `retries` times (UDP can silently drop packets, so a
 *      resolver ALWAYS retries before declaring a server dead).
 *   2. If a reply comes back with the TC (truncated) bit set, the answer did
 *      not fit the datagram; re-issue the SAME query over TCP, which has no
 *      size limit to speak of.
 * =========================================================================== */
int dns_exchange(const struct sockaddr_storage *server, socklen_t slen,
                 const uint8_t *query, size_t qlen,
                 uint8_t *resp, size_t resp_cap, size_t *resp_len,
                 int timeout_ms, int retries)
{
    for (int attempt = 0; attempt < retries; attempt++) {
        if (dns_send_udp(server, slen, query, qlen,
                         resp, resp_cap, resp_len, timeout_ms) < 0)
            continue;                         /* timeout/drop: try again        */

        /* A valid reply is at least a 12-byte header. Read the flags word
         * (bytes 2..3) directly to check the TC bit. */
        if (*resp_len < 12) continue;         /* runt reply; retry              */
        uint16_t flags = (uint16_t)((resp[2] << 8) | resp[3]);

        if (flags & DNS_FLAG_TC) {
            /* Truncated: the authoritative signal to switch to TCP. */
            if (dns_send_tcp(server, slen, query, qlen,
                             resp, resp_cap, resp_len, timeout_ms) < 0)
                return -1;                    /* TCP also failed: real failure  */
            return 0;
        }
        return 0;                             /* good UDP answer                */
    }
    return -1;                                /* every UDP attempt failed       */
}
