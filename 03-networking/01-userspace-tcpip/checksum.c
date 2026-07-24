/* ===========================================================================
 * checksum.c — the Internet Checksum (RFC 1071), in exhausting detail.
 * ===========================================================================
 *
 * WHY ONES' COMPLEMENT
 * --------------------
 * Every IP/ICMP/UDP/TCP header carries a 16-bit checksum. The algorithm is:
 * treat the data as a sequence of 16-bit integers, add them with ONES'-
 * COMPLEMENT addition (whenever the 16-bit sum overflows, add the carry bit
 * back into the low bit), then store the ONES'-COMPLEMENT (bitwise NOT) of that
 * sum. The receiver sums the same span INCLUDING the stored checksum; a valid
 * packet sums to 0xFFFF, whose complement is 0 — a cheap "== 0" verify.
 *
 * THE ENDIANNESS TRICK (the part everyone gets wrong first)
 * ---------------------------------------------------------
 * We add the 16-bit words in the HOST's native byte order (we read each word
 * with memcpy, which is exactly a native load but with no alignment
 * requirement). We then store the folded result straight into the packet's
 * network-order checksum field with a plain assignment — NO htons(). Why is that
 * correct even though the field is "network order"?
 *
 * RFC 1071 proves the checksum is byte-order independent: the ones'-complement
 * sum of a set of byte-SWAPPED words equals the byte-swap of the ones'-
 * complement sum (carry folding wraps around, so swapping every input's bytes
 * just swaps the output's bytes). Consequence: whatever endianness THIS host
 * uses to read and to write the checksum word cancels out. A little-endian
 * sender and a big-endian receiver each compute internally consistent values,
 * and the "sum including checksum == 0xFFFF" test still holds on the wire.
 * That is why real stacks (lwIP, the kernel) accumulate in host order and store
 * the u16 directly — and so do we.
 *
 * THE CARRY FOLD
 * --------------
 * We accumulate into a 32-bit `sum` so carries pile up in the high 16 bits
 * instead of being lost. At the end we fold: sum = (sum & 0xffff) + (sum >> 16),
 * repeated until the high half is empty. Two folds always suffice for real
 * packet sizes, but we loop to be obviously correct.
 * ========================================================================= */

#include <string.h>   /* memcpy — an alignment-safe native-order 16-bit load  */
#include "checksum.h"

/* ---------------------------------------------------------------------------
 * csum_accumulate — add `len` bytes into the ones'-complement running sum.
 *
 * `sum` is a 32-bit accumulator carried across calls, so a pseudo-header and a
 * segment can be summed with two chained calls (this is how TCP/UDP fold in the
 * pseudo-header without copying it next to the payload).
 * ------------------------------------------------------------------------- */
u32 csum_accumulate(const void *data, size_t len, u32 sum)
{
    const u8 *p = (const u8 *)data;
    u16 word;

    /* Consume complete 16-bit words. memcpy reads two bytes in host order with
     * no alignment fault (packet buffers can start at odd addresses); adding
     * into a 32-bit accumulator keeps every carry-out (bit 16) alive for the
     * later fold rather than silently dropping it. */
    while (len >= 2) {
        memcpy(&word, p, 2);
        sum += word;
        p   += 2;
        len -= 2;
    }

    /* One leftover byte (odd-length data, e.g. an odd ICMP payload). It is the
     * low-address byte of a final word whose other byte is an implicit 0. We
     * zero `word` first so the pad byte is exactly that 0, then copy the single
     * real byte into the low-address slot — the same placement the receiver
     * will use, so both sides agree. */
    if (len == 1) {
        word = 0;
        memcpy(&word, p, 1);
        sum += word;
    }

    return sum;
}

/* ---------------------------------------------------------------------------
 * csum_fold — collapse the 32-bit accumulator to the final 16-bit checksum.
 * ------------------------------------------------------------------------- */
u16 csum_fold(u32 sum)
{
    /* Fold the accumulated carries (high 16 bits) back into the low 16 bits.
     * Each fold can itself carry, so loop until the top half is 0. For real
     * packet sizes this iterates at most twice. */
    while (sum >> 16)
        sum = (sum & 0xffff) + (sum >> 16);

    /* Ones'-complement. The narrow store is the checksum ready for the header
     * field (see the endianness note at the top: no htons needed). */
    return (u16)(~sum & 0xffff);
}

/* Full checksum of a single contiguous buffer with no pseudo-header seed. */
u16 inet_checksum(const void *data, size_t len)
{
    return csum_fold(csum_accumulate(data, len, 0));
}
