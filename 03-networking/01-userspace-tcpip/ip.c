/* ===========================================================================
 * ip.c — IPv4 datagram parsing, validation, and emission (RFC 791).
 * ===========================================================================
 *
 * INBOUND we must, in order:
 *   1. confirm there is a full 20-byte header,
 *   2. confirm version == 4 and IHL >= 5,
 *   3. verify the header checksum (corruption in the addresses is fatal),
 *   4. confirm the packet is actually for us,
 *   5. hand the payload to the right transport by protocol number.
 *
 * OUTBOUND we fill every header field, set the checksum to 0, compute it over
 * the header, store it, resolve the destination MAC, and send.
 *
 * FRAGMENTATION (teaching note, not implemented): if an IP payload exceeds the
 * link MTU (1500), IP may split it into fragments that share the `id` field and
 * carry a `frag_off`; the MF bit marks all-but-the-last. The receiver holds
 * fragments until offset 0..end are all present, then reassembles. We instead
 * keep every datagram <= MTU and set DF, so no fragmentation logic is needed.
 * Modern stacks avoid fragmentation anyway (PMTU discovery) because a single
 * lost fragment forces retransmission of the whole datagram.
 * ========================================================================= */

#include "ip.h"
#include "checksum.h"
#include "arp.h"
#include "icmp.h"
#include "udp.h"
#include "tcp.h"

#include <string.h>

/* A monotonically increasing identification counter. The `id` field only needs
 * to be unique enough to group fragments of one datagram; since we never
 * fragment, any increasing value is fine. */
static u16 g_ip_id = 1;

/* ---------------------------------------------------------------------------
 * ip_input — validate and demultiplex an inbound IPv4 datagram.
 * ------------------------------------------------------------------------- */
void ip_input(struct netif *nif, u8 *packet, size_t len)
{
    if (len < IP_HDR_MIN_LEN) {
        LOGF("ip: runt (%zu)\n", len);
        return;
    }
    struct ip_hdr *ip = (struct ip_hdr *)packet;

    /* version is the HIGH nibble of ver_ihl; IHL (header length in 32-bit
     * words) is the LOW nibble. We mask each out instead of using C bitfields,
     * whose bit ordering is not portable to the wire layout. */
    u8 version = ip->ver_ihl >> 4;
    u8 ihl     = ip->ver_ihl & 0x0f;

    if (version != IP_VERSION_4) {
        LOGF("ip: not v4 (v%u)\n", version);
        return;
    }
    /* IHL must be at least 5 words (20 bytes) and fit inside what we received. */
    size_t hdr_len = (size_t)ihl * 4;
    if (ihl < 5 || hdr_len > len) {
        LOGF("ip: bad IHL %u\n", ihl);
        return;
    }

    /* Header checksum: sum the header words; a correct header sums to 0xFFFF,
     * so inet_checksum over the header (with the stored checksum included)
     * yields 0. Anything else means the addressing fields are corrupt — drop,
     * because delivering to the wrong socket/host is worse than a lost packet. */
    if (inet_checksum(ip, hdr_len) != 0) {
        LOGF("ip: bad header checksum\n");
        return;
    }

    /* total_len covers header+payload; it may be shorter than the frame if the
     * link padded a small frame up to the 60-byte Ethernet minimum. Trust
     * total_len, but never let it exceed what we actually received. */
    size_t total_len = ntohs(ip->total_len);
    if (total_len < hdr_len || total_len > len) {
        LOGF("ip: bad total_len %zu (have %zu)\n", total_len, len);
        return;
    }

    /* Is it for us? We are a host, not a router: silently drop anything not
     * addressed to our unicast IP. (A broadcast/multicast-aware stack would
     * also accept 255.255.255.255 and the subnet broadcast — out of scope.) */
    if (ip->dst != nif->ip) {
        /* Not an error, just not ours. Common on a shared segment. */
        return;
    }

    /* If this datagram is a fragment (MF set, or a nonzero offset), we cannot
     * reassemble it — say so honestly and drop. */
    u16 frag = ntohs(ip->frag_off);
    if ((frag & IP_MF) || (frag & IP_OFFMASK)) {
        LOGF("ip: fragment received but reassembly unimplemented\n");
        return;
    }

    /* Hand the transport payload up. Offsets: data starts after the (possibly
     * option-bearing) header; its length is total_len - hdr_len. */
    u8    *payload = packet + hdr_len;
    size_t paylen  = total_len - hdr_len;

    switch (ip->proto) {
    case IPPROTO_ICMP_:
        icmp_input(nif, ip, payload, paylen);
        break;
    case IPPROTO_UDP_:
        udp_input(nif, ip, payload, paylen);
        break;
    case IPPROTO_TCP_:
        tcp_input(nif, ip, payload, paylen);
        break;
    default:
        /* We could send an ICMP "protocol unreachable" here; a teaching core
         * just drops. */
        LOGF("ip: unhandled proto %u\n", ip->proto);
        break;
    }
}

/* ---------------------------------------------------------------------------
 * ip_output — build an IPv4 header around `payload` and transmit it.
 * ------------------------------------------------------------------------- */
int ip_output(struct netif *nif, u32 dst_ip, u8 proto,
               const void *payload, size_t len)
{
    u8 packet[IP_MTU];

    size_t total = IP_HDR_MIN_LEN + len;
    if (total > IP_MTU) {
        /* Would require fragmentation, which we don't do. TCP avoids this by
         * respecting the MSS; UDP/ICMP callers must keep payloads small. */
        LOGF("ip: datagram too big (%zu > MTU), would fragment\n", total);
        return -1;
    }

    struct ip_hdr *ip = (struct ip_hdr *)packet;
    ip->ver_ihl   = (IP_VERSION_4 << 4) | 5;      /* v4, 5-word (20-byte) hdr   */
    ip->tos       = 0;                             /* best-effort                */
    ip->total_len = htons((u16)total);
    ip->id        = htons(g_ip_id++);              /* unique-ish per datagram    */
    ip->frag_off  = htons(IP_DF);                  /* Don't Fragment             */
    ip->ttl       = 64;                            /* the conventional default   */
    ip->proto     = proto;
    ip->checksum  = 0;                             /* MUST be 0 while summing    */
    ip->src       = nif->ip;                       /* already network order      */
    ip->dst       = dst_ip;                        /* already network order      */

    /* Checksum is over the HEADER ONLY (unlike TCP/UDP). Compute with the field
     * zeroed, then store the result directly (see checksum.c: no htons). */
    ip->checksum  = inet_checksum(ip, IP_HDR_MIN_LEN);

    /* Copy the transport message in after the header. */
    memcpy(packet + IP_HDR_MIN_LEN, payload, len);

    /* Resolve the next hop's MAC. On this flat /24 link the destination is the
     * next hop (no router). Cache hit is the normal case, because to send us a
     * packet the peer had to ARP us first, and arp_input learned its mapping. */
    u8 dst_mac[ETH_ALEN];
    if (arp_lookup(dst_ip, dst_mac) < 0) {
        /* Miss: fire off a request and DROP this datagram. This mirrors real
         * stacks (which queue a packet or two); relying on the upper layer's
         * retransmission — TCP's RTO, or a repeated ping — to try again once
         * the reply populates the cache keeps the teaching core simple. */
        LOGF("ip: ARP miss for dst, requesting\n");
        arp_request(nif, dst_ip);
        return -1;
    }

    return eth_output(nif, dst_mac, ETH_P_IP, packet, total);
}
