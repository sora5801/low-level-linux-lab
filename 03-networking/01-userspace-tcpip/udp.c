/* ===========================================================================
 * udp.c — UDP datagrams and the pseudo-header checksum (RFC 768).
 * ===========================================================================
 *
 * UDP is almost nothing: 8 bytes of header (source port, dest port, length,
 * checksum) wrapped around a payload, with no state, ordering, or reliability.
 * Its one subtlety is the checksum, which is computed over a PSEUDO-HEADER
 * (borrowed IP src/dst/proto/length) followed by the real UDP header+data —
 * the same construction TCP uses. Including the IP addresses lets the receiver
 * catch a datagram that was delivered to the wrong host.
 *
 * In IPv4 the UDP checksum is OPTIONAL: a sender that sends checksum 0 is saying
 * "I didn't compute one." If our computed checksum happens to be 0, we must send
 * 0xFFFF instead (an equivalent ones'-complement value) so the receiver doesn't
 * read it as "absent."
 *
 * This teaching core wires up a trivial ECHO service on UDP port 7: any datagram
 * to port 7 is bounced straight back to its sender. That is enough to exercise
 * the whole path with `nc -u 10.0.0.2 7`.
 * ========================================================================= */

#include "udp.h"
#include "ip.h"
#include "checksum.h"

#include <string.h>

#define UDP_ECHO_PORT 7

/* ---------------------------------------------------------------------------
 * udp_checksum — sum(pseudo-header) then sum(UDP header + data), fold.
 *
 * We seed the accumulator with the pseudo-header, then chain the real segment
 * into the SAME running sum (csum_accumulate takes and returns the accumulator).
 * The UDP header's own checksum field must be 0 during this computation.
 * ------------------------------------------------------------------------- */
static u16 udp_checksum(u32 src_ip, u32 dst_ip,
                        const struct udp_hdr *udp, size_t udp_len)
{
    struct pseudo_hdr ph;
    ph.src    = src_ip;                 /* network order already               */
    ph.dst    = dst_ip;
    ph.zero   = 0;
    ph.proto  = IPPROTO_UDP_;
    ph.length = htons((u16)udp_len);    /* UDP header + data length            */

    u32 sum = csum_accumulate(&ph, sizeof(ph), 0);   /* seed with pseudo-hdr   */
    sum = csum_accumulate(udp, udp_len, sum);        /* then the real segment  */
    return csum_fold(sum);
}

/* ---------------------------------------------------------------------------
 * udp_input — validate and (for port 7) echo.
 * ------------------------------------------------------------------------- */
void udp_input(struct netif *nif, const struct ip_hdr *ip,
               u8 *payload, size_t len)
{
    if (len < UDP_HDR_LEN) {
        LOGF("udp: runt (%zu)\n", len);
        return;
    }
    struct udp_hdr *udp = (struct udp_hdr *)payload;

    size_t udp_len = ntohs(udp->len);
    if (udp_len < UDP_HDR_LEN || udp_len > len) {
        LOGF("udp: bad length %zu\n", udp_len);
        return;
    }

    /* Verify the checksum unless the sender opted out with 0. A correct
     * datagram (pseudo-header + segment) sums to 0xFFFF, so recomputing yields
     * 0. We recompute rather than compare because the field is embedded in the
     * summed region. */
    if (udp->checksum != 0) {
        u16 saved = udp->checksum;
        udp->checksum = 0;
        u16 calc = udp_checksum(ip->src, ip->dst, udp, udp_len);
        udp->checksum = saved;          /* restore (we don't own this buffer's
                                         *   meaning beyond this call, but be
                                         *   tidy) */
        if (calc != saved) {
            LOGF("udp: bad checksum\n");
            return;
        }
    }

    u16 sport = ntohs(udp->src_port);
    u16 dport = ntohs(udp->dst_port);
    u8    *data    = payload + UDP_HDR_LEN;
    size_t datalen = udp_len - UDP_HDR_LEN;

    LOGF("udp: %u bytes %u -> %u\n", (unsigned)datalen, sport, dport);

    /* The one "application": echo port 7. Swap ports and send it right back. */
    if (dport == UDP_ECHO_PORT)
        udp_output(nif, ip->src, dport, sport, data, datalen);
}

/* ---------------------------------------------------------------------------
 * udp_output — build a UDP datagram and hand it to IP.
 * ------------------------------------------------------------------------- */
int udp_output(struct netif *nif, u32 dst_ip,
               u16 src_port, u16 dst_port, const void *data, size_t len)
{
    u8 buf[IP_MTU - IP_HDR_MIN_LEN];   /* room for UDP header + data under MTU */
    size_t udp_len = UDP_HDR_LEN + len;
    if (udp_len > sizeof(buf)) {
        LOGF("udp: payload too big (%zu)\n", len);
        return -1;
    }

    struct udp_hdr *udp = (struct udp_hdr *)buf;
    udp->src_port = htons(src_port);
    udp->dst_port = htons(dst_port);
    udp->len      = htons((u16)udp_len);
    udp->checksum = 0;                            /* zero before computing      */
    memcpy(buf + UDP_HDR_LEN, data, len);

    /* Compute over pseudo-header + segment. nif->ip is our source address. */
    u16 c = udp_checksum(nif->ip, dst_ip, udp, udp_len);
    /* In IPv4 a computed 0 must be transmitted as 0xFFFF so it isn't read as
     * "no checksum present." */
    udp->checksum = (c == 0) ? 0xffff : c;

    return ip_output(nif, dst_ip, IPPROTO_UDP_, buf, udp_len);
}
