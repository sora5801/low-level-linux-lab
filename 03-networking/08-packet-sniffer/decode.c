/* ===========================================================================
 * decode.c — protocol dissection: Ethernet / IPv4 / IPv6 / ARP / TCP/UDP/ICMP.
 * ===========================================================================
 *
 * THE ONE RULE: NEVER TRUST A LENGTH.
 * -----------------------------------
 * A captured buffer can end at ANY byte: the kernel may have snapped it short
 * (PACKET_MMAP with a small frame size), or a malicious sender may have lied
 * about a header length. So every layer here does the same dance: check that
 * enough bytes remain BEFORE reading a field, and clamp any length taken from
 * the packet to what we actually captured. A packet dissector that trusts an
 * on-wire length is a textbook out-of-bounds read (this is how real CVEs in
 * tcpdump/Wireshark happened).
 *
 * ENDIANNESS.
 * -----------
 * Every multi-byte field in these headers is in NETWORK byte order (big-endian,
 * MSB first) — RFC 791/793/768 define the wire format that way, so a value read
 * on a little-endian x86 must be byte-swapped with ntohs()/ntohl() before it is
 * a meaningful integer. We call those out at each use.
 * ===========================================================================
 */

#include "decode.h"

#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>          /* ntohs, ntohl, inet_ntop                     */
#include <netinet/in.h>         /* IPPROTO_* constants                         */

/* We define header structs by hand (rather than pulling <netinet/ip.h>, which
 * varies by libc) so the byte offsets are explicit and visible. Everything is
 * __attribute__((packed)) so the compiler never inserts alignment padding that
 * would desync a field from its on-wire byte offset. */

/* ---- Ethernet II header: 14 bytes ---------------------------------------- */
struct eth_hdr {
    uint8_t  dst[6];            /* destination MAC                             */
    uint8_t  src[6];            /* source MAC                                  */
    uint16_t ethertype;        /* payload protocol, big-endian (0x0800 = IPv4)*/
} __attribute__((packed));

#define ETYPE_IP   0x0800
#define ETYPE_IPV6 0x86dd
#define ETYPE_ARP  0x0806

/* ---- IPv4 header: 20 bytes minimum, up to 60 with options ---------------- */
struct ip4_hdr {
    uint8_t  ver_ihl;          /* high nibble = version(4); low nibble = IHL   */
    uint8_t  tos;              /* type of service / DSCP+ECN                   */
    uint16_t tot_len;          /* total length (header+data), big-endian       */
    uint16_t id;               /* fragment identifier                          */
    uint16_t frag_off;         /* flags (3 bits) + fragment offset (13 bits)   */
    uint8_t  ttl;              /* time to live (hop limit)                     */
    uint8_t  proto;            /* L4 protocol (6=TCP,17=UDP,1=ICMP)            */
    uint16_t check;            /* header checksum, big-endian                  */
    uint32_t saddr;            /* source address, network order                */
    uint32_t daddr;            /* destination address, network order           */
} __attribute__((packed));

/* ---- IPv6 header: fixed 40 bytes ----------------------------------------- */
struct ip6_hdr_min {
    uint32_t ver_tc_fl;        /* version(4) + traffic class(8) + flow label   */
    uint16_t payload_len;      /* length after this 40-byte header             */
    uint8_t  next_hdr;         /* next header (acts like IPv4 `proto`)         */
    uint8_t  hop_limit;        /* == TTL                                       */
    uint8_t  saddr[16];        /* source address                               */
    uint8_t  daddr[16];        /* destination address                          */
} __attribute__((packed));

/* ---- TCP header: 20 bytes minimum ---------------------------------------- */
struct tcp_hdr {
    uint16_t sport;            /* source port, big-endian                      */
    uint16_t dport;            /* destination port, big-endian                 */
    uint32_t seq;              /* sequence number                              */
    uint32_t ack;              /* acknowledgement number                       */
    uint8_t  off_rsvd;         /* high nibble = data offset in 32-bit words    */
    uint8_t  flags;            /* CWR ECE URG ACK PSH RST SYN FIN              */
    uint16_t window;           /* receive window, big-endian                   */
    uint16_t check;            /* checksum                                     */
    uint16_t urg;              /* urgent pointer                               */
} __attribute__((packed));

/* ---- UDP header: fixed 8 bytes ------------------------------------------- */
struct udp_hdr {
    uint16_t sport;
    uint16_t dport;
    uint16_t len;              /* header+data length, big-endian               */
    uint16_t check;
} __attribute__((packed));

/* ---- ICMP header: first 4 bytes ------------------------------------------ */
struct icmp_hdr {
    uint8_t  type;             /* 8=echo request, 0=echo reply, 3=unreachable  */
    uint8_t  code;
    uint16_t check;
} __attribute__((packed));

/* ---------------------------------------------------------------------------
 * ip_checksum — the Internet checksum (RFC 1071), verified for teaching.
 *
 * The algorithm: sum the data as a sequence of big-endian 16-bit words using
 * ONES-COMPLEMENT addition (every carry out of bit 15 is folded back into bit
 * 0), then take the ones-complement of the result. A correct header sums to
 * 0xFFFF; equivalently, computing the checksum over a header whose `check`
 * field is included yields 0. We return the folded 16-bit value so the caller
 * can print "ok"/"bad".
 *
 * Why ones-complement? It is endianness-neutral (the sum is the same whether
 * you load words big- or little-endian, as long as you're consistent) and
 * cheap, which mattered enormously in 1981. We accumulate in a 32-bit sum and
 * fold carries at the end — mathematically identical to folding each add. --- */
static uint16_t ip_checksum(const void *buf, size_t len)
{
    const uint8_t *p = (const uint8_t *)buf;
    uint32_t sum = 0;

    /* Add complete 16-bit big-endian words. */
    while (len > 1) {
        sum += (uint32_t)((p[0] << 8) | p[1]);   /* MSB first: network order   */
        p   += 2;
        len -= 2;
    }
    /* A trailing odd byte is padded with a zero low byte. */
    if (len == 1)
        sum += (uint32_t)(p[0] << 8);

    /* Fold the carries (the bits above 15) back in until none remain. */
    while (sum >> 16)
        sum = (sum & 0xffff) + (sum >> 16);

    return (uint16_t)~sum;                       /* ones-complement of the sum */
}

/* Format a MAC address into caller storage "aa:bb:cc:dd:ee:ff". */
static void fmt_mac(const uint8_t m[6], char out[18])
{
    snprintf(out, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
             m[0], m[1], m[2], m[3], m[4], m[5]);
}

/* Decode the L4 payload common to IPv4 and IPv6. `proto` is the next-protocol
 * number, `l4` points at the transport header, `l4len` is how many captured
 * bytes remain from there. Prints " TCP 12345 > 80 [SYN] ..." style tails. */
static void decode_l4(uint8_t proto, const uint8_t *l4, uint32_t l4len)
{
    switch (proto) {
    case IPPROTO_TCP: {
        if (l4len < sizeof(struct tcp_hdr)) { printf(" TCP (truncated)\n"); return; }
        const struct tcp_hdr *t = (const struct tcp_hdr *)l4;
        /* Ports are big-endian on the wire -> ntohs before printing. */
        unsigned sp = ntohs(t->sport), dp = ntohs(t->dport);
        /* Decode the 8 flag bits into a compact [S.AP...] string. Order matches
         * the bit layout: bit0=FIN .. bit5=URG (bits 6/7 = ECE/CWR). */
        char fl[16]; int fi = 0;
        if (t->flags & 0x02) fl[fi++] = 'S';   /* SYN */
        if (t->flags & 0x10) fl[fi++] = 'A';   /* ACK */
        if (t->flags & 0x08) fl[fi++] = 'P';   /* PSH */
        if (t->flags & 0x01) fl[fi++] = 'F';   /* FIN */
        if (t->flags & 0x04) fl[fi++] = 'R';   /* RST */
        if (t->flags & 0x20) fl[fi++] = 'U';   /* URG */
        if (fi == 0) fl[fi++] = '.';
        fl[fi] = '\0';
        printf(" TCP %u > %u [%s] seq %u win %u\n",
               sp, dp, fl, ntohl(t->seq), ntohs(t->window));
        return;
    }
    case IPPROTO_UDP: {
        if (l4len < sizeof(struct udp_hdr)) { printf(" UDP (truncated)\n"); return; }
        const struct udp_hdr *u = (const struct udp_hdr *)l4;
        printf(" UDP %u > %u len %u\n",
               ntohs(u->sport), ntohs(u->dport), ntohs(u->len));
        return;
    }
    case IPPROTO_ICMP: {
        if (l4len < sizeof(struct icmp_hdr)) { printf(" ICMP (truncated)\n"); return; }
        const struct icmp_hdr *ic = (const struct icmp_hdr *)l4;
        const char *name = (ic->type == 8) ? "echo request" :
                           (ic->type == 0) ? "echo reply"   :
                           (ic->type == 3) ? "dest unreachable" :
                           (ic->type == 11) ? "time exceeded" : "type";
        printf(" ICMP %s (type %u code %u)\n", name, ic->type, ic->code);
        return;
    }
    case IPPROTO_ICMPV6:
        printf(" ICMPv6\n");
        return;
    default:
        printf(" ip-proto %u\n", proto);
        return;
    }
}

/* Decode an IPv4 datagram. `ip` points at the IPv4 header; `len` is captured
 * bytes from there onward. */
static void decode_ipv4(const uint8_t *ip, uint32_t len)
{
    if (len < sizeof(struct ip4_hdr)) { printf(" IPv4 (truncated)\n"); return; }
    const struct ip4_hdr *h = (const struct ip4_hdr *)ip;

    /* IHL (Internet Header Length) is in 32-bit words; *4 -> bytes. It can be
     * 20..60. A value < 20 is malformed; clamp so we never index backwards. */
    unsigned ihl = (h->ver_ihl & 0x0f) * 4u;
    if (ihl < sizeof(struct ip4_hdr)) { printf(" IPv4 (bad IHL)\n"); return; }
    if (ihl > len) ihl = len;              /* options may be snapped off       */

    /* Verify the header checksum over exactly `ihl` bytes. A correct header
     * yields 0x0000 when the stored checksum is included in the sum. */
    uint16_t ck = ip_checksum(h, ihl);
    const char *ckstr = (ck == 0) ? "ok" : "BAD";

    char s[INET_ADDRSTRLEN], d[INET_ADDRSTRLEN];
    /* saddr/daddr are already network order — inet_ntop expects that. */
    inet_ntop(AF_INET, &h->saddr, s, sizeof s);
    inet_ntop(AF_INET, &h->daddr, d, sizeof d);

    /* tot_len is big-endian; ntohs it. The L4 header begins after `ihl` bytes. */
    unsigned tot = ntohs(h->tot_len);
    printf("IPv4 %s > %s ttl %u len %u cksum %s |",
           s, d, h->ttl, tot, ckstr);

    /* If this is a non-first fragment (offset != 0), there is no L4 header here. */
    unsigned frag = ntohs(h->frag_off) & 0x1fff;
    if (frag != 0) { printf(" fragment offset %u\n", frag * 8); return; }

    uint32_t l4len = (len > ihl) ? len - ihl : 0;
    decode_l4(h->proto, ip + ihl, l4len);
}

/* Decode an IPv6 datagram (base header only; we don't walk extension headers,
 * we just report the first Next Header). */
static void decode_ipv6(const uint8_t *ip, uint32_t len)
{
    if (len < sizeof(struct ip6_hdr_min)) { printf(" IPv6 (truncated)\n"); return; }
    const struct ip6_hdr_min *h = (const struct ip6_hdr_min *)ip;

    char s[INET6_ADDRSTRLEN], d[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, h->saddr, s, sizeof s);
    inet_ntop(AF_INET6, h->daddr, d, sizeof d);

    printf("IPv6 %s > %s hlim %u plen %u |",
           s, d, h->hop_limit, ntohs(h->payload_len));

    uint32_t l4len = len - sizeof(struct ip6_hdr_min);
    decode_l4(h->next_hdr, ip + sizeof(struct ip6_hdr_min), l4len);
}

/* Decode an ARP message just enough to say who is asking about whom. */
static void decode_arp(const uint8_t *arp, uint32_t len)
{
    /* ARP for IPv4-over-Ethernet: 28 bytes. Fields we use:
     *   [6]=opcode(be16, 1=request 2=reply), [14..17]=sender IP,
     *   [24..27]=target IP. */
    if (len < 28) { printf("ARP (truncated)\n"); return; }
    unsigned op = (arp[6] << 8) | arp[7];        /* big-endian opcode          */
    char sip[INET_ADDRSTRLEN], tip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, arp + 14, sip, sizeof sip);
    inet_ntop(AF_INET, arp + 24, tip, sizeof tip);
    if (op == 1) printf("ARP who-has %s tell %s\n", tip, sip);
    else if (op == 2) printf("ARP reply %s\n", sip);
    else printf("ARP opcode %u\n", op);
}

void decode_frame(const uint8_t *data, uint32_t caplen, uint32_t wirelen)
{
    if (caplen < sizeof(struct eth_hdr)) {
        printf("[runt frame, %u bytes]\n", caplen);
        return;
    }
    const struct eth_hdr *e = (const struct eth_hdr *)data;
    char sm[18], dm[18];
    fmt_mac(e->src, sm);
    fmt_mac(e->dst, dm);

    /* ethertype is big-endian; ntohs it to compare against host constants. */
    uint16_t et = ntohs(e->ethertype);

    /* Length banner (wire length may exceed caplen when snapped short). */
    printf("%s > %s ", sm, dm);
    if (wirelen != caplen) printf("(%u of %u) ", caplen, wirelen);

    const uint8_t *payload = data + sizeof(struct eth_hdr);
    uint32_t plen = caplen - sizeof(struct eth_hdr);

    switch (et) {
    case ETYPE_IP:   decode_ipv4(payload, plen); break;
    case ETYPE_IPV6: decode_ipv6(payload, plen); break;
    case ETYPE_ARP:  decode_arp(payload, plen);  break;
    default:
        /* Values <= 1500 are actually an 802.3 length, not an ethertype. */
        if (et <= 1500) printf("802.3 len %u\n", et);
        else            printf("ethertype 0x%04x len %u\n", et, wirelen);
        break;
    }
}
