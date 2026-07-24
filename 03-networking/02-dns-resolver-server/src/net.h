/* ===========================================================================
 * net.h — DNS transport: UDP with a TCP fallback, timeouts, and retries.
 * ===========================================================================
 *
 * DNS runs on port 53 over BOTH transports:
 *   - UDP is the default: one datagram out, one datagram back. Cheap, but a
 *     UDP answer that exceeds the client's buffer comes back with the TC
 *     (truncated) header bit set and empty-ish — the signal to "ask again over
 *     TCP".
 *   - TCP is used for large answers (zone transfers, big RRsets) and whenever
 *     TC was set. Over TCP each message is framed with a 2-byte big-endian
 *     length prefix, because a stream has no datagram boundaries.
 *
 * All addresses are passed as a sockaddr_storage (big enough for IPv4 or IPv6)
 * plus its length, so the same code path serves both families.
 * =========================================================================== */
#ifndef NET_H
#define NET_H

#include <stdint.h>
#include <stddef.h>
#include <sys/socket.h>   /* struct sockaddr_storage, socklen_t                */

/* Parse a numeric IPv4 or IPv6 address string into a sockaddr with `port`.
 * Numeric only (no name lookup — this is the thing that DOES the lookups).
 * Returns 0 on success, -1 on a malformed address. */
int dns_addr_from_ip(const char *ip, uint16_t port,
                     struct sockaddr_storage *ss, socklen_t *slen);

/* One UDP request/response exchange with a millisecond timeout.
 * Returns 0 and fills resp/resp_len on success; -1 on error or timeout. */
int dns_send_udp(const struct sockaddr_storage *server, socklen_t slen,
                 const uint8_t *query, size_t qlen,
                 uint8_t *resp, size_t resp_cap, size_t *resp_len,
                 int timeout_ms);

/* One TCP request/response exchange (with 2-byte length framing) + timeout. */
int dns_send_tcp(const struct sockaddr_storage *server, socklen_t slen,
                 const uint8_t *query, size_t qlen,
                 uint8_t *resp, size_t resp_cap, size_t *resp_len,
                 int timeout_ms);

/* High-level exchange used by the resolver: send over UDP with `retries`
 * attempts; if the reply is truncated (TC bit) transparently retry over TCP.
 * Returns 0 on success, -1 if every attempt failed. */
int dns_exchange(const struct sockaddr_storage *server, socklen_t slen,
                 const uint8_t *query, size_t qlen,
                 uint8_t *resp, size_t resp_cap, size_t *resp_len,
                 int timeout_ms, int retries);

#endif /* NET_H */
