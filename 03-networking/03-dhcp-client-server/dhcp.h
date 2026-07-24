/* ===========================================================================
 * dhcp.h — wire formats and shared helpers for a from-scratch DHCP client/server
 * ===========================================================================
 *
 * WHY THIS FILE EXISTS
 * --------------------
 * A DHCP client is a chicken-and-egg problem: it needs an IP address, but the
 * whole point of DHCP is that it does NOT have one yet. The normal socket layer
 * (AF_INET / SOCK_DGRAM) refuses to send a UDP datagram from a host with no
 * configured source address, and the kernel would have no route for the
 * 255.255.255.255 limited-broadcast destination either. So we drop *below* the
 * IP stack and use an AF_PACKET raw socket: we hand the kernel a fully-formed
 * Ethernet frame — Ethernet + IPv4 + UDP + BOOTP/DHCP, checksums and all — and
 * it just puts the bytes on the wire. That means WE are the IP stack for these
 * packets, which is exactly what makes this a good teaching exercise.
 *
 * This header declares the four nested wire structures and the pure helper
 * routines (checksums, TLV option walk/append) shared by the client and server.
 * Everything here is Linux-only; see README.md for the capabilities required
 * (CAP_NET_RAW) and the exact run commands.
 *
 * BYTE ORDER
 * ----------
 * Every multi-byte field that travels on the wire is BIG-ENDIAN ("network byte
 * order"), because RFC 1700 fixed that as the on-the-wire convention for the
 * whole Internet protocol suite back when big-endian hosts were common. x86-64
 * is little-endian, so any u16/u32 we *interpret* must be run through
 * ntohs/ntohl, and any we *emit* through htons/htonl. Fields we treat as opaque
 * byte arrays (MACs, IPv4 addresses stored as uint32_t we never do math on) we
 * keep in network order end-to-end and never byte-swap — that is a deliberate
 * simplification the comments call out at each use site.
 * ===========================================================================
 */
#ifndef DHCP_H
#define DHCP_H

#include <stdint.h>   /* uintN_t fixed-width types — wire formats need exact widths */
#include <stddef.h>   /* size_t */

/* ---------------------------------------------------------------------------
 * Host<->network byte order, spelled out.
 *
 * The standard htons/htonl live in <arpa/inet.h>, but we define our own so this
 * header stays dependency-free and, more importantly, so the SWAP is visible.
 * "Network order" is big-endian: the most significant byte travels first. On a
 * big-endian host these are the identity; on little-endian x86-64 they reverse
 * the bytes. We branch on the compiler's __BYTE_ORDER__ so the code is correct
 * on either, though every target in this lab is little-endian x86-64.
 * --------------------------------------------------------------------------- */
static inline uint16_t htons_(uint16_t x)
{
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return x;                                   /* already network order        */
#else
    return (uint16_t)((x << 8) | (x >> 8));     /* swap the two bytes           */
#endif
}
static inline uint32_t htonl_(uint32_t x)
{
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return x;
#else
    return ((x & 0x000000FFu) << 24) |          /* byte0 -> byte3               */
           ((x & 0x0000FF00u) << 8)  |          /* byte1 -> byte2               */
           ((x & 0x00FF0000u) >> 8)  |          /* byte2 -> byte1               */
           ((x & 0xFF000000u) >> 24);           /* byte3 -> byte0               */
#endif
}
/* ntoh* are the same bit operation as hton* (swap is its own inverse). */
static inline uint16_t ntohs_(uint16_t x) { return htons_(x); }
static inline uint32_t ntohl_(uint32_t x) { return htonl_(x); }

/* ---------------------------------------------------------------------------
 * Packed wire structures.
 *
 * `__attribute__((packed))` is LOAD-BEARING here, not cosmetic. Without it the
 * compiler is free to insert padding between fields to satisfy each type's
 * natural alignment, and the struct would no longer match the byte layout the
 * protocol mandates. Packed forces "one byte after another, offset == sum of
 * prior field sizes," so `sizeof` and `offsetof` equal the on-wire offsets.
 * The cost is that member accesses may be unaligned; x86-64 tolerates that in
 * hardware, and these structs are only touched on the (rare) packet path.
 * --------------------------------------------------------------------------- */

/* Ethernet II header — 14 bytes. This is what a NIC prepends/strips in HW; with
 * AF_PACKET/SOCK_RAW we build it ourselves. */
struct eth_hdr {
    uint8_t  dst[6];     /* destination MAC. For DHCP DISCOVER/REQUEST this is
                          *   the broadcast address ff:ff:ff:ff:ff:ff so every
                          *   station (incl. the not-yet-known DHCP server) sees
                          *   it. Offset 0. */
    uint8_t  src[6];     /* our own MAC, read from the NIC via SIOCGIFHWADDR.
                          *   Offset 6. */
    uint16_t ethertype;  /* 0x0800 = IPv4 (see ETHERTYPE_IPV4). Network order.
                          *   Offset 12. Tells the receiver's stack which L3
                          *   protocol the payload is. */
} __attribute__((packed));

/* IPv4 header — 20 bytes (no options; IHL fixed at 5). RFC 791. */
struct ip_hdr {
    uint8_t  ver_ihl;    /* high nibble = version (4); low nibble = header length
                          *   in 32-bit WORDS (5 => 5*4 = 20 bytes). Packed into
                          *   one byte, hence the manual (4<<4)|5. Offset 0. */
    uint8_t  tos;        /* type of service / DSCP+ECN. 0 = best effort. Off 1. */
    uint16_t tot_len;    /* total length = IP header + UDP header + DHCP payload,
                          *   in bytes, network order. Off 2. */
    uint16_t id;         /* identification, for reassembling fragments. We don't
                          *   fragment, so any value works; we use a constant. */
    uint16_t frag_off;   /* flags (3 bits) + fragment offset (13 bits). 0 here:
                          *   not fragmented, DF clear. Off 6. */
    uint8_t  ttl;        /* time to live; each router decrements it. 64 is the
                          *   conventional start (Linux default). Off 8. */
    uint8_t  proto;      /* L4 protocol number. 17 = UDP (see IPPROTO_UDP). Off 9. */
    uint16_t check;      /* header checksum (ones-complement over the 20 header
                          *   bytes with this field taken as 0). Off 10. */
    uint32_t saddr;      /* source IPv4, network order. 0.0.0.0 for a client that
                          *   has no address yet. Off 12. */
    uint32_t daddr;      /* destination IPv4, network order. 255.255.255.255
                          *   (limited broadcast) for DISCOVER/REQUEST. Off 16. */
} __attribute__((packed));

/* UDP header — 8 bytes. RFC 768. */
struct udp_hdr {
    uint16_t sport;      /* source port. Client uses 68, server uses 67. Net order. */
    uint16_t dport;      /* destination port. 67 (server) or 68 (client). */
    uint16_t len;        /* UDP header (8) + DHCP payload length, network order. */
    uint16_t check;      /* checksum computed over a PSEUDO-HEADER + UDP header +
                          *   data (see udp_checksum). 0 would mean "no checksum";
                          *   we always compute a real one so tcpdump shows "ok". */
} __attribute__((packed));

/* BOOTP/DHCP fixed message — 236 bytes, then the 4-byte magic cookie, then a
 * variable-length options area. RFC 951 (BOOTP) + RFC 2131 (DHCP). DHCP reuses
 * the BOOTP frame and stuffs its real semantics into the options. */
struct dhcp_msg {
    uint8_t  op;         /* 1 = BOOTREQUEST (client->server),
                          *   2 = BOOTREPLY   (server->client). Off 0. */
    uint8_t  htype;      /* hardware type. 1 = Ethernet (ARP HRD value). Off 1. */
    uint8_t  hlen;       /* hardware address length. 6 for a MAC. Off 2. */
    uint8_t  hops;       /* set to 0 by the client; a relay agent increments it. */
    uint32_t xid;        /* transaction ID: a random 32-bit token the client
                          *   picks once and echoes through the whole DORA
                          *   exchange, so it can match replies to its request
                          *   and ignore other clients' traffic. Net order. Off 4. */
    uint16_t secs;       /* seconds since the client began acquiring. Off 8. */
    uint16_t flags;      /* bit 15 = BROADCAST flag. We set 0x8000 so the server
                          *   replies via broadcast (a client with no IP cannot
                          *   yet answer a unicast ARP). Net order. Off 10. */
    uint32_t ciaddr;     /* client IP — filled only if the client already owns a
                          *   lease it is renewing. 0 during initial DORA. Off 12. */
    uint32_t yiaddr;     /* "your" IP — the address the SERVER assigns; the field
                          *   the client is really after. Net order. Off 16. */
    uint32_t siaddr;     /* IP of the next server in a bootstrap (TFTP). Off 20. */
    uint32_t giaddr;     /* relay agent (gateway) IP; 0 when no relay. Off 24. */
    uint8_t  chaddr[16]; /* client hardware address: the 6-byte MAC then 10 zero
                          *   pad bytes. The server keys leases off this. Off 28. */
    uint8_t  sname[64];  /* optional server host name, NUL-padded. Off 44. */
    uint8_t  file[128];  /* optional boot file name, NUL-padded. Off 108. */
    /* Off 236: the options area begins with the 4-byte magic cookie below. */
} __attribute__((packed));

/* The magic cookie that separates the BOOTP vendor area from DHCP options.
 * RFC 2131 §3: the first four octets of the options field are 99.130.83.99,
 * i.e. 0x63825363. A parser uses it to confirm "these are DHCP options, not
 * legacy BOOTP vendor data." Stored/compared in network byte order. */
#define DHCP_MAGIC_COOKIE 0x63825363u

/* op-field values */
#define BOOTREQUEST 1
#define BOOTREPLY   2

/* htype */
#define HTYPE_ETHERNET 1

/* The BROADCAST flag (bit 15 of dhcp_msg.flags), pre-swapped to network order.
 * htons(0x8000) == 0x0080 on a little-endian host; we store the network-order
 * constant directly to keep the builders branch-free. */
#define DHCP_FLAG_BROADCAST 0x8000

/* ---- DHCP option codes (the TLV "type" byte) — RFC 2132 --------------------
 * Options are Type-Length-Value triples EXCEPT for two 1-byte specials: PAD (0)
 * and END (255) carry no length/value. Everything the DORA dance needs rides in
 * these options rather than in fixed header fields. */
#define DHCP_OPT_PAD             0   /* single 0 byte, used only for alignment */
#define DHCP_OPT_SUBNET_MASK     1   /* value: 4-byte netmask */
#define DHCP_OPT_ROUTER          3   /* value: 4-byte default gateway */
#define DHCP_OPT_DNS             6   /* value: one or more 4-byte DNS servers */
#define DHCP_OPT_REQUESTED_IP    50  /* client asks for a specific IP (in REQUEST) */
#define DHCP_OPT_LEASE_TIME      51  /* value: 4-byte lease duration in seconds */
#define DHCP_OPT_MSG_TYPE        53  /* value: 1 byte, the DHCP message type below */
#define DHCP_OPT_SERVER_ID       54  /* value: 4-byte IP identifying the server */
#define DHCP_OPT_PARAM_REQ_LIST  55  /* client's wish-list of option codes */
#define DHCP_OPT_END             255 /* single 255 byte: end of options */

/* ---- DHCP message types (the value of option 53) — the DORA state machine --
 *   DISCOVER -> OFFER -> REQUEST -> ACK  is one successful lease acquisition. */
#define DHCPDISCOVER 1  /* client broadcasts: "any DHCP server out there?"      */
#define DHCPOFFER    2  /* server -> client: "you may have <yiaddr>"            */
#define DHCPREQUEST  3  /* client broadcasts: "I formally request <that IP>"    */
#define DHCPDECLINE  4  /* client: "that IP is already in use (ARP clash)"      */
#define DHCPACK      5  /* server: "confirmed, the lease is yours"              */
#define DHCPNAK      6  /* server: "no — that request is invalid, start over"   */
#define DHCPRELEASE  7  /* client: "I'm done, reclaim the lease"                */
#define DHCPINFORM   8  /* client with a static IP wants only config options    */

/* Well-known UDP ports (RFC 2131). Fixed by IANA; both are privileged (<1024). */
#define DHCP_SERVER_PORT 67
#define DHCP_CLIENT_PORT 68

/* EtherType and IP protocol numbers we hardcode into the headers. */
#define ETHERTYPE_IPV4 0x0800
#define IPPROTO_UDP_   17

/* Sizes used all over the builders/parsers. */
#define ETH_HDR_LEN   14
#define IP_HDR_LEN    20
#define UDP_HDR_LEN   8
#define DHCP_FIXED_LEN 236                 /* op..file, before the magic cookie */
#define MAC_LEN       6

/* A generous cap on the whole frame. A DHCP message is small; 1500 (the classic
 * Ethernet MTU) is far more than we ever need but keeps buffers simple. */
#define FRAME_BUF_MAX 1500

/* ===========================================================================
 * Shared pure-logic helpers (implemented in dhcp_common.c). These contain NO
 * syscalls — they only manipulate buffers — which is why the assembly demo is
 * carved out of exactly this code (see asm/demo.c).
 * ===========================================================================
 */

/* RFC 1071 ones-complement checksum over `len` bytes at `data`. Used for the
 * IPv4 header. Returns the value already in network byte order, ready to drop
 * into ip_hdr.check. See dhcp_common.c for the folding math. */
uint16_t ip_checksum(const void *data, size_t len);

/* UDP checksum: RFC 768 ones-complement over the 12-byte IPv4 pseudo-header
 * (saddr, daddr, zero, proto, udp_len) followed by the UDP header and payload.
 * `saddr`/`daddr` are in network order; `udp` points at the already-filled UDP
 * header (its own check field must be 0 on entry); `payload`/`payload_len` are
 * the DHCP bytes. Returns the network-order checksum (never 0 — a computed 0 is
 * transmitted as 0xFFFF, since 0 means "no checksum" in UDP). */
uint16_t udp_checksum(uint32_t saddr, uint32_t daddr,
                      const struct udp_hdr *udp,
                      const void *payload, size_t payload_len);

/* Append one option to the options buffer at offset `off`, returning the new
 * offset. `opts` points at the start of the options area (just past the magic
 * cookie); `code` is the option type; `val`/`len` are the value bytes. The
 * caller guarantees room (options are tiny and FRAME_BUF_MAX is large). */
size_t dhcp_opt_append(uint8_t *opts, size_t off,
                       uint8_t code, const void *val, uint8_t len);

/* Convenience wrappers: append a 1-byte or 4-byte option. */
size_t dhcp_opt_append_u8(uint8_t *opts, size_t off, uint8_t code, uint8_t v);
size_t dhcp_opt_append_u32(uint8_t *opts, size_t off, uint8_t code, uint32_t v_net);

/* Write the single END (255) marker; returns the new (final) length. */
size_t dhcp_opt_end(uint8_t *opts, size_t off);

/* THE TLV WALKER. Scan the options area [opts, opts+opts_len) for the first
 * option whose type == `code`. On a match, return a pointer to its VALUE bytes
 * and store the value length in *out_len (if non-NULL). Return NULL if absent.
 * Correctly skips PAD (0, no length byte), stops at END (255), and never reads
 * past opts_len even if a malicious length byte points beyond the buffer — this
 * bounds-checking is the crux of safe option parsing and the star of asm/demo.c. */
const uint8_t *dhcp_opt_find(const uint8_t *opts, size_t opts_len,
                             uint8_t code, uint8_t *out_len);

/* Assemble a complete Ethernet+IPv4+UDP frame around a DHCP payload. Fills the
 * three headers, computes both checksums, and copies `dhcp`/`dhcp_len` into
 * place. Returns the total frame length (ETH+IP+UDP+dhcp_len). `frame` must
 * have room for FRAME_BUF_MAX. All addresses are in network byte order. */
size_t dhcp_build_frame(uint8_t *frame,
                        const uint8_t dst_mac[MAC_LEN],
                        const uint8_t src_mac[MAC_LEN],
                        uint32_t saddr, uint32_t daddr,
                        uint16_t sport, uint16_t dport,
                        const void *dhcp, size_t dhcp_len);

#endif /* DHCP_H */
