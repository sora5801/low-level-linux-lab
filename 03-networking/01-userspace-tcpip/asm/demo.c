/* ===========================================================================
 * demo.c — the Internet Checksum, extracted SELF-CONTAINED for assembly study.
 * ===========================================================================
 *
 * This is the single most instructive pure-logic routine in the stack: the
 * ones'-complement 16-bit sum with end-around carry fold (RFC 1071) that IP,
 * ICMP, UDP, and TCP all use. It is lifted out of checksum.c with NO system
 * headers and its own integer typedefs, so `clang -S` produces clean assembly
 * with nothing from libc in the way — exactly what asm/demo.annotated.s
 * dissects.
 *
 * Generate the committed assembly (from this project's directory) with:
 *   clang --target=x86_64-pc-linux-gnu -S -O0 -fno-asynchronous-unwind-tables \
 *         -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
 *   clang --target=x86_64-pc-linux-gnu -S -O1 -fno-asynchronous-unwind-tables \
 *         -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
 *   clang --target=x86_64-pc-linux-gnu -S -O2 -fno-asynchronous-unwind-tables \
 *         -fno-jump-tables asm/demo.c -o asm/demo.O2.s
 *
 * See the top of ../checksum.c for the full "why ones' complement / why it is
 * byte-order independent" derivation. The short version:
 *   - sum 16-bit words into a 32-bit accumulator so carries survive,
 *   - fold the high half back into the low half until it is empty,
 *   - return the bitwise-NOT of the result.
 * ========================================================================= */

/* Our own fixed-width types — no <stdint.h>. On the x86-64 SysV target these
 * widths are guaranteed: int is 32-bit, short is 16-bit, char is 8-bit, and
 * unsigned long is 64-bit (LP64). We use these so the file needs no headers. */
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef unsigned long  usize;   /* 64-bit on LP64; holds a size/pointer diff  */

/* ---------------------------------------------------------------------------
 * csum_accumulate — add `len` bytes at `data` into the running 32-bit sum.
 *
 * We assemble each 16-bit word big-endian from two bytes: word = b0<<8 | b1.
 * (The library version uses memcpy to read in host order; here, with no
 * headers, we spell out the byte assembly, which makes the compiled loop easy
 * to read in the assembly. The numeric result differs only by a byte swap that
 * the ones'-complement sum is provably invariant to — see checksum.c.)
 * ------------------------------------------------------------------------- */
u32 csum_accumulate(const void *data, usize len, u32 sum)
{
    const u8 *p = (const u8 *)data;

    /* Consume complete 16-bit words. Adding into a 32-bit accumulator keeps
     * every carry-out (bit 16) alive for the fold instead of dropping it. */
    while (len >= 2) {
        sum += (u32)((p[0] << 8) | p[1]);
        p   += 2;
        len -= 2;
    }

    /* A single trailing byte is the high byte of a final word padded with 0. */
    if (len == 1)
        sum += (u32)(p[0] << 8);

    return sum;
}

/* ---------------------------------------------------------------------------
 * csum_fold — collapse the 32-bit accumulator to the final 16-bit checksum.
 * The `while (sum >> 16)` loop performs the end-around carry: it folds the
 * accumulated carries in the high half back into the low half until none
 * remain, then returns the ones'-complement.
 * ------------------------------------------------------------------------- */
u16 csum_fold(u32 sum)
{
    while (sum >> 16)
        sum = (sum & 0xffff) + (sum >> 16);
    return (u16)(~sum & 0xffff);
}

/* ---------------------------------------------------------------------------
 * inet_checksum — the whole thing for one contiguous buffer.
 * This is the function asm/demo.annotated.s walks through instruction by
 * instruction: at -O1 clang inlines both helpers into this one body.
 * ------------------------------------------------------------------------- */
u16 inet_checksum(const void *data, usize len)
{
    return csum_fold(csum_accumulate(data, len, 0));
}
