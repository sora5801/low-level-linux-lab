/* ===========================================================================
 * demo.c — the DNS name decoder with compression-pointer following.
 * ===========================================================================
 *
 * This is a SELF-CONTAINED extraction of the single most instructive routine in
 * the project: decoding a domain name from a DNS message, including the 0xC0
 * compression pointers, WITH a guard against pointer loops. It defines its own
 * integer types and includes NO system headers, so the assembly clang emits for
 * it (see demo.s / demo.O0.s / demo.O2.s, and the hand-annotated
 * demo.annotated.s) is pure, self-contained logic — nothing but this code.
 *
 * ----------------------------------------------------------------------------
 * THE FORMAT (RFC 1035 §4.1.4)
 * ----------------------------------------------------------------------------
 * A name is a sequence of LABELS. Each label is a length byte L followed by L
 * bytes. A length byte of 0 ends the name. To save space, a length byte whose
 * top two bits are set (byte & 0xC0 == 0xC0) is not a label but a COMPRESSION
 * POINTER: that byte's low 6 bits and the next byte together form a 14-bit
 * offset from the start of the message, and decoding CONTINUES from there.
 *
 *     message bytes:   ... 3 w w w  C0 0C ...
 *                          |-------| |----- pointer to offset 0x0C (=12)
 *
 * WHY 0xC0 IS SAFE AS A FLAG. A label length is at most 63 (a label is <= 63
 * bytes). 63 = 0x3F, so the two high bits of a real length byte are always 0.
 * That left the 0b11 combination free to mean "pointer", and 0b01 / 0b10
 * reserved.
 *
 * THE TWO CLASSIC BUGS THIS GUARDS AGAINST
 *   1. A pointer that points at itself, or two pointers that point at each
 *      other, form an INFINITE LOOP. We cap the number of pointer hops: a valid
 *      name cannot follow more pointers than the message has bytes, so once we
 *      exceed that we declare a loop and fail. (`hops` / `max_hops` below.)
 *   2. A pointer offset that lands outside the buffer would read wild memory.
 *      Every access is bounds-checked against `msg_len` first.
 *
 * Returns the number of characters written to `out` (not counting the trailing
 * NUL) on success, or -1 on any malformed input.
 * ===========================================================================
 */

/* Our own fixed-width types — no <stdint.h>. On the x86-64 SysV target these
 * widths are: unsigned char = 1 byte, unsigned int = 4 bytes. */
typedef unsigned char u8;
typedef unsigned int  u32;

/* Presentation/length limits, spelled out so we need no headers. */
#define MAX_NAME  255   /* a whole name is at most 255 octets           */
#define MAX_LABEL 63    /* a single label is at most 63 octets          */
#define PTR_MASK  0xC0  /* top two bits set => this byte is a pointer   */

int dns_decode_name(const u8 *msg, u32 msg_len, u32 start,
                    char *out, u32 out_cap)
{
    u32 pos     = start;   /* walk cursor; may jump when we follow a pointer  */
    u32 out_len = 0;       /* characters written to `out` so far             */
    u32 hops    = 0;       /* number of compression pointers followed        */
    /* Loop guard: no acyclic pointer chain is longer than the message, so
     * more hops than there are bytes means we are in a cycle. */
    u32 max_hops = msg_len + 1;

    if (out_cap == 0)
        return -1;
    out[0] = '\0';

    for (;;) {
        /* The length/pointer byte must be inside the message. */
        if (pos >= msg_len)
            return -1;
        u8 lenb = msg[pos];

        if ((lenb & PTR_MASK) == PTR_MASK) {
            /* ---- compression pointer ------------------------------------ */
            if (pos + 2 > msg_len)          /* need the second pointer byte   */
                return -1;
            /* 14-bit offset: low 6 bits of lenb are the high bits; next byte
             * is the low 8 bits. */
            u32 target = ((u32)(lenb & 0x3F) << 8) | (u32)msg[pos + 1];

            if (++hops > max_hops)          /* too many hops => pointer loop  */
                return -1;
            if (target >= msg_len)          /* pointer must stay in bounds    */
                return -1;
            pos = target;                   /* continue decoding there        */
            continue;
        }

        if ((lenb & PTR_MASK) != 0)         /* 0b01/0b10 are reserved/illegal */
            return -1;

        if (lenb == 0) {                    /* the root label ends the name   */
            break;
        }

        /* ---- an ordinary label of `lenb` bytes ------------------------- */
        if (lenb > MAX_LABEL)               /* label too long                 */
            return -1;
        if (pos + 1 + (u32)lenb > msg_len)  /* label body must fit           */
            return -1;

        /* Presentation form puts a '.' between labels (not before the first). */
        if (out_len != 0) {
            if (out_len + 1 >= out_cap)
                return -1;
            out[out_len++] = '.';
        }

        /* Enforce the whole-name cap and the output capacity, then copy the
         * label bytes one at a time (no memcpy — we include no headers). */
        if (out_len + (u32)lenb >= out_cap)
            return -1;
        if (out_len + (u32)lenb > MAX_NAME)
            return -1;
        for (u32 i = 0; i < (u32)lenb; i++)
            out[out_len + i] = (char)msg[pos + 1 + i];
        out_len += (u32)lenb;

        pos += 1u + (u32)lenb;              /* advance past length + label    */
    }

    out[out_len] = '\0';                    /* NUL-terminate the result       */
    return (int)out_len;
}
