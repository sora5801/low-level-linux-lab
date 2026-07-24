/* ===========================================================================
 * dhcp_common.c — checksums, DHCP option TLV walk/append, and frame assembly.
 * ===========================================================================
 *
 * These are the *pure* routines shared by the client and server: they read and
 * write byte buffers and never touch the kernel. Because they are so self-
 * contained, the two most instructive ones — the ones-complement checksum and
 * the option TLV walker — are lifted almost verbatim into asm/demo.c so we can
 * study the compiler's output. Read those alongside asm/demo.annotated.s.
 * ===========================================================================
 */
#include "dhcp.h"
#include <string.h>   /* memcpy, memset — plain byte moves, no endianness games */

/* ---------------------------------------------------------------------------
 * ip_checksum — the RFC 1071 "Internet checksum".
 *
 * THE ALGORITHM. Treat the data as a sequence of 16-bit big-endian words, add
 * them all into a wide (32-bit) accumulator, then FOLD the carry bits that
 * overflowed past bit 15 back into the low 16 bits — repeatedly, until nothing
 * is left above bit 15 — and finally take the ones-complement (bitwise NOT).
 *
 * WHY IT IS ENDIANNESS-NEUTRAL. We assemble each word explicitly as
 * (hi << 8) | lo from two individual bytes, so the code computes the same
 * mathematical sum whether it runs on x86-64 (little-endian) or a big-endian
 * box. The result is *stored* big-endian because we build the word high-byte-
 * first; that is precisely what the wire wants, so the caller drops the return
 * value straight into the header with no htons().
 *
 * WHY ONES-COMPLEMENT. End-around carry makes the sum independent of where you
 * start, and the receiver can verify by summing the whole header *including*
 * the checksum field and checking the result is 0xFFFF — cheap and symmetric.
 * --------------------------------------------------------------------------- */
uint16_t ip_checksum(const void *data, size_t len)
{
    const uint8_t *p = (const uint8_t *)data;
    uint32_t sum = 0;                 /* 32 bits so carries can pile up safely */

    /* Sum complete 16-bit words, MSB first (network order). */
    while (len > 1) {
        sum += ((uint32_t)p[0] << 8) | p[1];  /* one big-endian 16-bit word    */
        p   += 2;
        len -= 2;
    }
    /* If the length is odd, the last lone byte is the HIGH byte of a final word
     * whose low byte is an implicit 0 (RFC 1071's "pad with a zero octet"). */
    if (len == 1)
        sum += (uint32_t)p[0] << 8;

    /* Fold any carry out of the low 16 bits back in, twice (a single fold can
     * itself produce one more carry). After this, sum fits in 16 bits. */
    sum = (sum & 0xFFFF) + (sum >> 16);
    sum = (sum & 0xFFFF) + (sum >> 16);

    /* Ones-complement, truncated to 16 bits. The value is already big-endian
     * because we built each word high-byte-first, so no byte swap is needed. */
    return (uint16_t)(~sum & 0xFFFF);
}

/* ---------------------------------------------------------------------------
 * udp_checksum — RFC 768 checksum, which covers a synthetic PSEUDO-HEADER.
 *
 * UDP's checksum deliberately includes fields copied from the IP header — the
 * source and destination addresses, the protocol number, and the UDP length —
 * so that a datagram delivered to the wrong host or protocol is detected. Those
 * fields are not transmitted again; they are prepended only for the math. The
 * 12-byte IPv4 pseudo-header is:
 *
 *     +--------+--------+--------+--------+
 *     |          source  address         |   (4 bytes, network order)
 *     +--------+--------+--------+--------+
 *     |        destination address       |   (4 bytes, network order)
 *     +--------+--------+--------+--------+
 *     |  zero  |proto=17|    UDP length   |   (1 + 1 + 2 bytes)
 *     +--------+--------+--------+--------+
 *
 * We accumulate the pseudo-header, then the real UDP header, then the payload,
 * using the same big-endian-word / end-around-carry scheme as ip_checksum.
 * --------------------------------------------------------------------------- */
uint16_t udp_checksum(uint32_t saddr, uint32_t daddr,
                      const struct udp_hdr *udp,
                      const void *payload, size_t payload_len)
{
    uint32_t sum = 0;

    /* saddr/daddr are already in network byte order; view them as raw bytes so
     * the (hi<<8)|lo assembly below stays endianness-neutral regardless of host. */
    const uint8_t *sa = (const uint8_t *)&saddr;
    const uint8_t *da = (const uint8_t *)&daddr;

    /* Pseudo-header, as 16-bit big-endian words. */
    sum += ((uint32_t)sa[0] << 8) | sa[1];   /* source addr, high half         */
    sum += ((uint32_t)sa[2] << 8) | sa[3];   /* source addr, low half          */
    sum += ((uint32_t)da[0] << 8) | da[1];   /* dest addr, high half           */
    sum += ((uint32_t)da[2] << 8) | da[3];   /* dest addr, low half            */
    sum += (uint32_t)IPPROTO_UDP_;           /* zero byte (0) + proto (17)      */
    /* udp->len is already the network-order UDP length; add it as a value. It
     * is the same number for both the pseudo-header "length" and the header. */
    {
        const uint8_t *ul = (const uint8_t *)&udp->len;
        sum += ((uint32_t)ul[0] << 8) | ul[1];
    }

    /* The 8-byte UDP header itself. Its own `check` field must be 0 on entry so
     * it contributes nothing here (adding 0 is a no-op). We walk its bytes. */
    {
        const uint8_t *u = (const uint8_t *)udp;
        sum += ((uint32_t)u[0] << 8) | u[1]; /* sport */
        sum += ((uint32_t)u[2] << 8) | u[3]; /* dport */
        sum += ((uint32_t)u[4] << 8) | u[5]; /* len   */
        sum += ((uint32_t)u[6] << 8) | u[7]; /* check (==0)                     */
    }

    /* The payload (the DHCP message + options). */
    {
        const uint8_t *p = (const uint8_t *)payload;
        size_t n = payload_len;
        while (n > 1) {
            sum += ((uint32_t)p[0] << 8) | p[1];
            p += 2;
            n -= 2;
        }
        if (n == 1)                         /* odd trailing byte, low byte = 0  */
            sum += (uint32_t)p[0] << 8;
    }

    /* Fold carries, twice, exactly as in ip_checksum. */
    sum = (sum & 0xFFFF) + (sum >> 16);
    sum = (sum & 0xFFFF) + (sum >> 16);

    uint16_t c = (uint16_t)(~sum & 0xFFFF);

    /* RFC 768 special case: a computed checksum of zero is transmitted as all
     * ones (0xFFFF), because the value 0 is reserved to mean "sender computed
     * no checksum." The two are equal in ones-complement arithmetic. */
    return c == 0 ? 0xFFFF : c;
}

/* ---------------------------------------------------------------------------
 * Option builders. The options area is a flat run of TLV triples; we append by
 * writing type, then length, then the value bytes, and bump the offset.
 * --------------------------------------------------------------------------- */
size_t dhcp_opt_append(uint8_t *opts, size_t off,
                       uint8_t code, const void *val, uint8_t len)
{
    opts[off++] = code;                 /* Type byte                            */
    opts[off++] = len;                  /* Length byte (value length, 0..255)   */
    if (len && val) {
        memcpy(&opts[off], val, len);   /* Value bytes, copied verbatim         */
        off += len;
    }
    return off;                         /* caller continues appending from here */
}

size_t dhcp_opt_append_u8(uint8_t *opts, size_t off, uint8_t code, uint8_t v)
{
    return dhcp_opt_append(opts, off, code, &v, 1);
}

/* Append a 4-byte option whose value is ALREADY in network byte order (e.g. an
 * IP address or an htonl'd lease time). We copy the four bytes unchanged. */
size_t dhcp_opt_append_u32(uint8_t *opts, size_t off, uint8_t code, uint32_t v_net)
{
    return dhcp_opt_append(opts, off, code, &v_net, 4);
}

size_t dhcp_opt_end(uint8_t *opts, size_t off)
{
    opts[off++] = DHCP_OPT_END;         /* 255 — no length, no value            */
    return off;
}

/* ---------------------------------------------------------------------------
 * dhcp_opt_find — THE TLV WALKER (mirrored in asm/demo.c).
 *
 * Parsing attacker-controlled length-prefixed data is where real stacks get
 * CVEs, so the bounds checks here are the whole point:
 *   - PAD (0) is a bare byte: advance by 1, no length follows.
 *   - END (255) terminates the options: stop immediately.
 *   - Any other code is TLV: byte[i]=type, byte[i+1]=length, then `length`
 *     value bytes. Before trusting `length` we verify the length byte itself is
 *     in-bounds, and that the value range [i+2, i+2+length) does not run past
 *     the buffer. A crafted option claiming length 200 in a 30-byte buffer must
 *     NOT cause an over-read.
 * --------------------------------------------------------------------------- */
const uint8_t *dhcp_opt_find(const uint8_t *opts, size_t opts_len,
                             uint8_t code, uint8_t *out_len)
{
    size_t i = 0;
    while (i < opts_len) {
        uint8_t t = opts[i];

        if (t == DHCP_OPT_PAD) {        /* PAD: skip this single byte           */
            i++;
            continue;
        }
        if (t == DHCP_OPT_END)          /* END: nothing valid follows           */
            break;

        /* From here it is a TLV; we must be able to read the length byte. */
        if (i + 1 >= opts_len)          /* truncated: type with no length       */
            break;
        uint8_t l = opts[i + 1];

        /* Reject a length that would read value bytes past the buffer end. */
        if (i + 2 + (size_t)l > opts_len)
            break;

        if (t == code) {                /* match: hand back the value pointer   */
            if (out_len)
                *out_len = l;
            return &opts[i + 2];
        }

        i += 2 + (size_t)l;             /* skip type+len+value, on to the next  */
    }
    return NULL;                        /* option absent (or buffer malformed)  */
}

/* ---------------------------------------------------------------------------
 * dhcp_build_frame — stitch the four layers together and checksum them.
 *
 * Layout in `frame`:
 *   [0 .. 13]      Ethernet header
 *   [14 .. 33]     IPv4 header
 *   [34 .. 41]     UDP header
 *   [42 .. ]       DHCP payload (dhcp_len bytes)
 *
 * ORDER MATTERS for the checksums: the UDP checksum covers the payload and the
 * (source/dest) IP addresses, so those must be set before we compute it; the IP
 * checksum covers only the IP header and must be computed with its own check
 * field zeroed. We therefore fill everything, zero both check fields, then
 * compute UDP first and IP second.
 * --------------------------------------------------------------------------- */
size_t dhcp_build_frame(uint8_t *frame,
                        const uint8_t dst_mac[MAC_LEN],
                        const uint8_t src_mac[MAC_LEN],
                        uint32_t saddr, uint32_t daddr,
                        uint16_t sport, uint16_t dport,
                        const void *dhcp, size_t dhcp_len)
{
    /* Carve typed views over the flat byte buffer at the right offsets. Because
     * the structs are packed, these pointers alias the exact wire bytes. */
    struct eth_hdr *eth = (struct eth_hdr *)(frame + 0);
    struct ip_hdr  *ip  = (struct ip_hdr  *)(frame + ETH_HDR_LEN);
    struct udp_hdr *udp = (struct udp_hdr *)(frame + ETH_HDR_LEN + IP_HDR_LEN);
    uint8_t        *pl  = frame + ETH_HDR_LEN + IP_HDR_LEN + UDP_HDR_LEN;

    size_t ip_total  = IP_HDR_LEN + UDP_HDR_LEN + dhcp_len;
    size_t udp_total = UDP_HDR_LEN + dhcp_len;

    /* ---- Ethernet ---- */
    memcpy(eth->dst, dst_mac, MAC_LEN);
    memcpy(eth->src, src_mac, MAC_LEN);
    eth->ethertype = htons_(ETHERTYPE_IPV4);  /* 0x0800, byte-swapped to wire   */

    /* ---- IPv4 ---- */
    ip->ver_ihl  = (4 << 4) | 5;              /* version 4, IHL 5 words (20 B)   */
    ip->tos      = 0;
    ip->tot_len  = htons_((uint16_t)ip_total);
    ip->id       = htons_(0x0000);            /* no fragmentation -> id unused   */
    ip->frag_off = htons_(0x0000);            /* DF=0, MF=0, offset 0            */
    ip->ttl      = 64;                        /* plenty; broadcast stays on-link */
    ip->proto    = IPPROTO_UDP_;              /* 17                              */
    ip->check    = 0;                         /* zero BEFORE checksumming header */
    ip->saddr    = saddr;                     /* already network order           */
    ip->daddr    = daddr;

    /* ---- UDP ---- */
    udp->sport = htons_(sport);
    udp->dport = htons_(dport);
    udp->len   = htons_((uint16_t)udp_total);
    udp->check = 0;                           /* zero BEFORE checksumming        */

    /* ---- DHCP payload ---- */
    memcpy(pl, dhcp, dhcp_len);

    /* ---- Checksums (UDP needs the addresses + payload already in place) ---- */
    udp->check = udp_checksum(ip->saddr, ip->daddr, udp, pl, dhcp_len);
    ip->check  = ip_checksum(ip, IP_HDR_LEN);

    return ETH_HDR_LEN + ip_total;            /* full frame length              */
}
