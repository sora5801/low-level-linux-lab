/* ===========================================================================
 * demo.c — SELF-CONTAINED extraction of this project's two pure-logic hearts:
 *          the DHCP option TLV walker and the UDP pseudo-header checksum.
 * ===========================================================================
 *
 * WHY A SEPARATE FILE. The real sources (dhcp_common.c, dhcp_client.c,
 * dhcp_server.c) all pull in Linux headers (<sys/socket.h>, <linux/if_packet.h>,
 * …) so they cannot be compiled to assembly on a non-Linux host. The teaching
 * assembly must come from something that compiles ANYWHERE clang runs, so we
 * copy the two routines that are the intellectual core of the project into this
 * header-free file — no #include, our own fixed-width typedefs — and generate
 * the committed .s from it. The logic is byte-for-byte the same as the real
 * ip_checksum/udp_checksum/dhcp_opt_find in dhcp_common.c.
 *
 * Generate the assembly (exactly the commands in the project README):
 *   clang --target=x86_64-pc-linux-gnu -S -O0 -fno-asynchronous-unwind-tables \
 *         -fno-jump-tables -fno-omit-frame-pointer demo.c -o asm/demo.O0.s
 *   clang --target=x86_64-pc-linux-gnu -S -O1 -fno-asynchronous-unwind-tables \
 *         -fno-jump-tables -fno-omit-frame-pointer demo.c -o asm/demo.s
 *   clang --target=x86_64-pc-linux-gnu -S -O2 -fno-asynchronous-unwind-tables \
 *         -fno-jump-tables demo.c -o asm/demo.O2.s
 *
 * asm/demo.annotated.s hand-annotates the -O1 output (asm/demo.s).
 * ===========================================================================
 */

/* Our own fixed-width types — no <stdint.h>. On the x86-64 SysV (LP64) target
 * these widths hold: char=1, short=2, int=4, long=8. `usize` is the natural
 * 64-bit word used for indices and lengths (what size_t would be). */
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef unsigned long  usize;

/* ---------------------------------------------------------------------------
 * dhcp_opt_find — walk a DHCP options area and return the first option's value.
 *
 * The options area is a flat sequence of Type-Length-Value triples, with two
 * one-byte specials: PAD (0) has no length/value, END (255) terminates. This is
 * exactly the kind of length-prefixed, attacker-supplied data where a missing
 * bounds check becomes a remote over-read, so the three guards below are the
 * whole lesson:
 *   (1) reading the length byte requires i+1 to be in range,
 *   (2) the value span [i+2, i+2+l) must not exceed opts_len,
 *   (3) END stops us even if more bytes follow.
 *
 * Returns a pointer to the matched option's value bytes (and its length via
 * out_len), or a null pointer if the option is absent or the buffer malformed.
 * --------------------------------------------------------------------------- */
const u8 *dhcp_opt_find(const u8 *opts, usize opts_len, u8 code, u8 *out_len)
{
    usize i = 0;
    while (i < opts_len) {
        u8 t = opts[i];

        if (t == 0) {              /* PAD: a single byte, no length follows      */
            i++;
            continue;
        }
        if (t == 255)              /* END: nothing valid after this              */
            break;

        if (i + 1 >= opts_len)     /* guard (1): need a length byte              */
            break;
        u8 l = opts[i + 1];

        if (i + 2 + (usize)l > opts_len)  /* guard (2): value must fit           */
            break;

        if (t == code) {           /* match: hand back value pointer + length    */
            if (out_len)
                *out_len = l;
            return &opts[i + 2];
        }

        i += 2 + (usize)l;         /* advance past type+len+value                 */
    }
    return (const u8 *)0;          /* not found                                  */
}

/* ---------------------------------------------------------------------------
 * udp_checksum — the RFC 768 UDP checksum over the IPv4 pseudo-header.
 *
 * The checksum covers a 12-byte synthetic pseudo-header (src addr, dst addr, a
 * zero byte, the protocol number 17, and the UDP length) followed by the UDP
 * header and the payload. Including the addresses/protocol lets a receiver
 * detect a datagram misrouted to it. We sum 16-bit big-endian words into a
 * 32-bit accumulator, fold the end-around carry twice, and take the ones-
 * complement — the classic RFC 1071 loop, written so it is endianness-neutral
 * (each word is assembled explicitly as (hi<<8)|lo).
 *
 *   saddr, daddr : source/dest IPv4, already in NETWORK byte order.
 *   udp          : the 8-byte UDP header; udp[4],udp[5] are its length field,
 *                  and its own checksum bytes udp[6],udp[7] must be 0 on entry.
 *   payload      : the DHCP bytes; payload_len their count.
 *
 * Returns the network-order checksum, mapping a computed 0 to 0xFFFF (0 is
 * reserved to mean "no checksum" in UDP).
 * --------------------------------------------------------------------------- */
u16 udp_checksum(u32 saddr, u32 daddr, const u8 *udp,
                 const u8 *payload, usize payload_len)
{
    u32 sum = 0;

    /* View the addresses as raw bytes so the word assembly is host-neutral. */
    const u8 *sa = (const u8 *)&saddr;
    const u8 *da = (const u8 *)&daddr;

    /* --- pseudo-header --- */
    sum += ((u32)sa[0] << 8) | sa[1];      /* src high half                      */
    sum += ((u32)sa[2] << 8) | sa[3];      /* src low half                       */
    sum += ((u32)da[0] << 8) | da[1];      /* dst high half                      */
    sum += ((u32)da[2] << 8) | da[3];      /* dst low half                       */
    sum += (u32)17;                        /* zero byte + protocol 17 (UDP)      */
    sum += ((u32)udp[4] << 8) | udp[5];    /* UDP length (also in pseudo-header) */

    /* --- the UDP header itself (8 bytes; checksum field is 0 here) --- */
    sum += ((u32)udp[0] << 8) | udp[1];    /* source port                        */
    sum += ((u32)udp[2] << 8) | udp[3];    /* dest port                          */
    sum += ((u32)udp[4] << 8) | udp[5];    /* length                             */
    sum += ((u32)udp[6] << 8) | udp[7];    /* checksum field (== 0)              */

    /* --- payload --- */
    usize n = payload_len;
    const u8 *p = payload;
    while (n > 1) {
        sum += ((u32)p[0] << 8) | p[1];    /* one big-endian 16-bit word         */
        p += 2;
        n -= 2;
    }
    if (n == 1)                            /* odd tail byte: low byte implicit 0 */
        sum += (u32)p[0] << 8;

    /* Fold carries out of the high 16 bits back into the low 16, twice. */
    sum = (sum & 0xFFFF) + (sum >> 16);
    sum = (sum & 0xFFFF) + (sum >> 16);

    u16 c = (u16)(~sum & 0xFFFF);          /* ones-complement, 16-bit            */
    return c == 0 ? (u16)0xFFFF : c;       /* 0 -> 0xFFFF (UDP special case)     */
}
