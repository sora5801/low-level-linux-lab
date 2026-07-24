/* ===========================================================================
 * common.h — shared vocabulary for the whole userspace TCP/IP stack.
 * ===========================================================================
 *
 * WHAT THIS FILE IS
 * -----------------
 * A tiny networking stack is really a pile of byte layouts (headers) plus the
 * arithmetic that ties them together (checksums, sequence numbers, byte order).
 * This header collects the vocabulary every module shares:
 *
 *   - fixed-width integer types (so a "16-bit port" is exactly 16 bits),
 *   - the on-the-wire header structs, byte-for-byte, in NETWORK byte order,
 *   - byte-order helpers and WHY the network is big-endian,
 *   - a couple of debug/util macros.
 *
 * A header on the wire is not a C struct in the abstract — it is a fixed run of
 * bytes at fixed offsets. We model each one with `__attribute__((packed))` so
 * the compiler inserts ZERO padding: the struct's memory image is exactly the
 * bytes that travel on the link. Read a frame into a buffer, cast the buffer to
 * `struct eth_hdr *`, and field access becomes offset arithmetic. That is the
 * whole trick behind every parser here.
 * ===========================================================================
 */
#ifndef USERSPACE_TCPIP_COMMON_H
#define USERSPACE_TCPIP_COMMON_H

#include <stdint.h>   /* uint8_t/uint16_t/uint32_t: exact widths matter on the
                       *   wire — a field is N bits because the RFC says so.   */
#include <stddef.h>   /* size_t                                               */
#include <arpa/inet.h>/* htons/htonl/ntohs/ntohl: the ONE place we borrow the
                       *   host's byte-swap intrinsics (see note below).       */

/* --- Short type aliases. The RFCs speak in "octets"; so do we. ------------ */
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

/* ---------------------------------------------------------------------------
 * NETWORK BYTE ORDER — the single most common source of stack bugs.
 * ---------------------------------------------------------------------------
 * The Internet is big-endian ("network byte order"): the most significant byte
 * of a multi-byte field is sent FIRST. x86-64 is little-endian: the least
 * significant byte sits at the lowest address. So a 16-bit port 0x0050 (80,
 * HTTP) is stored in our memory as the bytes {0x50,0x00} but must appear on the
 * wire as {0x00,0x50}. htons() ("host TO network, short") performs that swap on
 * a little-endian host and is a no-op on a big-endian one — which is exactly
 * why we ALWAYS call it instead of hardcoding a swap: the code stays correct on
 * either kind of CPU.
 *
 * Rule we follow everywhere: values live in HOST order in local variables and
 * arithmetic, and are converted to/from NETWORK order only at the moment they
 * are written into / read out of a header struct.
 * ------------------------------------------------------------------------- */

/* EtherTypes (the 16-bit "what's inside this Ethernet frame" tag), host order.
 * We htons() these when we compare against a header field. */
#define ETH_P_IP   0x0800   /* payload is an IPv4 packet                      */
#define ETH_P_ARP  0x0806   /* payload is an ARP message                      */

#define ETH_ALEN   6        /* a MAC address is 6 octets                      */
#define ETH_HDR_LEN 14      /* dst(6) + src(6) + ethertype(2)                 */

/* IANA IP protocol numbers (the IPv4 header's "proto" byte). */
#define IPPROTO_ICMP_ 1
#define IPPROTO_TCP_  6
#define IPPROTO_UDP_  17

/* ---------------------------------------------------------------------------
 * Ethernet II header — 14 bytes, prepended to every frame on a TAP link.
 * A NIC matches the 6-byte destination MAC against its own address (or the
 * broadcast ff:ff:ff:ff:ff:ff) to decide whether to accept the frame; then the
 * `ethertype` selects the upper-layer handler (IP vs ARP).
 * ------------------------------------------------------------------------- */
struct eth_hdr {
    u8  dst[ETH_ALEN];   /* offset  0: destination MAC                        */
    u8  src[ETH_ALEN];   /* offset  6: source MAC                             */
    u16 ethertype;       /* offset 12: ETH_P_IP / ETH_P_ARP, NETWORK order    */
    u8  payload[];       /* offset 14: the encapsulated packet                */
} __attribute__((packed));

/* ---------------------------------------------------------------------------
 * ARP message (for IPv4-over-Ethernet) — 28 bytes.
 * ARP answers "I have IP X, who has the MAC?" so we can address the Ethernet
 * frame. `oper` is 1 (request) or 2 (reply). sha/spa = sender hw/proto addr,
 * tha/tpa = target hw/proto addr. All multi-byte fields are network order.
 * ------------------------------------------------------------------------- */
struct arp_hdr {
    u16 htype;           /* 0: hardware type; 1 = Ethernet                    */
    u16 ptype;           /* 2: protocol type; 0x0800 = IPv4                   */
    u8  hlen;            /* 4: hardware addr length = 6                       */
    u8  plen;            /* 5: protocol addr length = 4                       */
    u16 oper;            /* 6: operation: 1 request, 2 reply                  */
    u8  sha[ETH_ALEN];   /* 8:  sender MAC                                     */
    u8  spa[4];          /* 14: sender IPv4 (kept as raw bytes, network order)*/
    u8  tha[ETH_ALEN];   /* 18: target MAC (zero in a request)                */
    u8  tpa[4];          /* 24: target IPv4                                    */
} __attribute__((packed));

#define ARP_HTYPE_ETH   1
#define ARP_OP_REQUEST  1
#define ARP_OP_REPLY    2

/* ---------------------------------------------------------------------------
 * IPv4 header — 20 bytes when there are no options (the common case).
 * `ver_ihl` packs two 4-bit fields; `frag_off` packs 3 flag bits + a 13-bit
 * fragment offset. We extract those with masks (see ip.c) rather than bitfields
 * because C bitfield ORDERING is implementation-defined and would not match the
 * wire portably.
 * ------------------------------------------------------------------------- */
struct ip_hdr {
    u8  ver_ihl;     /* 0:  version(high nibble)=4 | IHL(low nibble)=words     */
    u8  tos;         /* 1:  type of service / DSCP+ECN                         */
    u16 total_len;   /* 2:  header + payload length, in bytes, NETWORK order   */
    u16 id;          /* 4:  identification (groups fragments of one datagram)  */
    u16 frag_off;    /* 6:  flags(3) | fragment offset(13), NETWORK order      */
    u8  ttl;         /* 8:  time to live; decremented per hop, 0 => drop       */
    u8  proto;       /* 9:  IPPROTO_* — selects ICMP/TCP/UDP handler           */
    u16 checksum;    /* 10: header-only ones'-complement checksum             */
    u32 src;         /* 12: source IPv4 address, NETWORK order                 */
    u32 dst;         /* 16: destination IPv4 address, NETWORK order            */
    u8  data[];      /* 20: payload (or options, if IHL > 5)                   */
} __attribute__((packed));

#define IP_HDR_MIN_LEN   20
#define IP_VERSION_4     4
#define IP_RF     0x8000 /* frag_off: "reserved" bit                          */
#define IP_DF     0x4000 /* frag_off: Don't Fragment                          */
#define IP_MF     0x2000 /* frag_off: More Fragments (set on all but the last)*/
#define IP_OFFMASK 0x1fff/* frag_off: the 13-bit offset field                 */

/* ICMP header (echo request/reply share this shape). */
struct icmp_hdr {
    u8  type;        /* 0: 8 = echo request, 0 = echo reply                    */
    u8  code;        /* 1: 0 for echo                                          */
    u16 checksum;    /* 2: ones'-complement checksum over ICMP hdr + data      */
    u16 id;          /* 4: echo identifier (ping uses it to match replies)     */
    u16 seq;         /* 6: echo sequence number                                */
    u8  data[];      /* 8: opaque payload — we must echo it back verbatim      */
} __attribute__((packed));

#define ICMP_ECHO_REQUEST 8
#define ICMP_ECHO_REPLY   0

/* UDP header — 8 bytes. `len` covers header+payload; `checksum` is optional in
 * IPv4 (0 means "not computed") but we always fill it in. */
struct udp_hdr {
    u16 src_port;    /* 0: source port,      NETWORK order                     */
    u16 dst_port;    /* 2: destination port, NETWORK order                     */
    u16 len;         /* 4: UDP header + data length, NETWORK order             */
    u16 checksum;    /* 6: checksum over pseudo-header + UDP hdr + data         */
    u8  data[];      /* 8: payload                                             */
} __attribute__((packed));

#define UDP_HDR_LEN 8

/* TCP header — 20 bytes without options. `data_off` high nibble is the header
 * length in 32-bit words (>=5). `flags` is the control-bit byte. */
struct tcp_hdr {
    u16 src_port;    /* 0:  source port,      NETWORK order                    */
    u16 dst_port;    /* 2:  destination port, NETWORK order                    */
    u32 seq;         /* 4:  sequence number of the first data octet (net order)*/
    u32 ack;         /* 8:  next seq the sender expects (valid iff ACK set)     */
    u8  data_off;    /* 12: data offset(high nibble, in 32-bit words) | reserved*/
    u8  flags;       /* 13: control bits (see TCP_* below)                     */
    u16 window;      /* 14: receive window the sender advertises (net order)   */
    u16 checksum;    /* 16: checksum over pseudo-header + TCP segment          */
    u16 urg_ptr;     /* 18: urgent pointer (only meaningful if URG set)        */
    u8  data[];      /* 20: options (if data_off>5) then payload               */
} __attribute__((packed));

#define TCP_HDR_MIN_LEN 20
/* Control flags, one bit each in the `flags` octet. */
#define TCP_FIN 0x01   /* no more data from sender; begin teardown            */
#define TCP_SYN 0x02   /* synchronize sequence numbers (opens a connection)   */
#define TCP_RST 0x04   /* reset — abort the connection                        */
#define TCP_PSH 0x08   /* push: deliver buffered data to the app promptly     */
#define TCP_ACK 0x10   /* the ack field is meaningful                         */
#define TCP_URG 0x20   /* urgent pointer is meaningful                        */

/* ---------------------------------------------------------------------------
 * Pseudo-header — NOT sent on the wire. TCP and UDP checksums are computed over
 * this 12-byte prefix PLUS the real segment. Including the IP src/dst/proto/len
 * lets the receiver detect a datagram that was misdelivered to the wrong host
 * or protocol (the checksum won't match). This is the historical wart where the
 * transport layer peeks at network-layer fields.
 * ------------------------------------------------------------------------- */
struct pseudo_hdr {
    u32 src;         /* 0:  IPv4 source,      NETWORK order                    */
    u32 dst;         /* 4:  IPv4 destination, NETWORK order                    */
    u8  zero;        /* 8:  always 0                                           */
    u8  proto;       /* 9:  IPPROTO_TCP_ / IPPROTO_UDP_                        */
    u16 length;      /* 10: TCP/UDP length (header+data), NETWORK order        */
} __attribute__((packed));

/* --- Small logging helper. Everything goes to stderr so stdout stays clean
 * for any real data an app might print. Compiled out is not needed; ping/TCP
 * tracing is the whole point of a teaching stack. -------------------------- */
#include <stdio.h>
#define LOGF(...) do { fprintf(stderr, __VA_ARGS__); } while (0)

/* Number of elements in a fixed C array. */
#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

#endif /* USERSPACE_TCPIP_COMMON_H */
