/* ===========================================================================
 * serial.h — the ONE definition of the username -> serial transform.
 * ===========================================================================
 *
 * Both the crackme (the target you reverse) and the keygen (the solution that
 * mints valid serials) include this header, so by construction they agree on
 * the algorithm. That is the whole trick of a keygen-me: the "secret" is not a
 * stored password — it is a *pure function* of the username, recomputable by
 * anyone who recovers the function. Recovering it from the binary (statically
 * with objdump, dynamically with gdb) is the exercise; re-implementing it is
 * the keygen. There is nothing to brute-force and nothing to leak: the serial
 * for a name is fully determined the moment the name is chosen.
 *
 * DESIGN OF THE TRANSFORM
 * -----------------------
 * We want a check that is "non-trivial but reversible": non-trivial so a plain
 * `strings` / `strcmp` won't reveal the answer, reversible so a diligent reader
 * can reconstruct it. We build it from three primitives the spec calls out —
 * rotates, xor, and modular arithmetic — arranged as a hash with an avalanche:
 *
 *   per byte:   h ^= byte;               (xor-in the message byte)
 *               h *= FNV_PRIME;          (multiply mod 2^64 — an ODD constant,
 *                                         so it is INVERTIBLE modulo 2^64; this
 *                                         is the "modular arithmetic" leg)
 *               h  = rotl64(h, 7);       (rotate — diffuses the high bits that
 *                                         the multiply just churned into the low
 *                                         bits, so later bytes mix everywhere)
 *               h ^= XOR_CONST;          (a constant xor — cheap extra diffusion)
 *
 *   finalizer:  MurmurHash3's fmix64 avalanche, so flipping any input bit flips
 *               ~half the output bits. This is what makes the serial look random
 *               and makes "guess the next digit" hopeless.
 *
 * WHY THESE ARE THE RIGHT TEACHING PRIMITIVES
 *   * multiply-by-odd mod 2^64 is a bijection on 64-bit words — the classic
 *     reason FNV/Murmur multipliers are odd. (A reader who tries to *invert*
 *     the whole thing learns modular inverses; a keygen author just re-runs it
 *     forward, which is easier and is why keygen-mes exist.)
 *   * `rotl` shows up as a single `rolq $7, %reg` in the disassembly — a nice,
 *     recognizable landmark when you are reading the asm (see asm/demo.*.s).
 *
 * OUTPUT FORMAT
 * -------------
 * The 64-bit key is printed as four 16-bit groups of uppercase hex joined by
 * dashes:   GGGG-GGGG-GGGG-GGGG   (19 characters + a NUL = 20-byte buffer).
 * ===========================================================================
 */
#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>   /* uint64_t/uint32_t: exact-width so the math is portable */
#include <stddef.h>   /* size_t for the constant-time compare length            */

/* The hash constants, named so the disassembly-reader can grep for them. These
 * exact 64-bit immediates appear verbatim in the compiled code (as `movabsq`
 * loads), which is how you *find* this routine when reversing a stripped binary:
 * search for 0x100000001B3 (the FNV prime) or the fmix64 constants below. */
#define SERIAL_FNV_OFFSET 0xCBF29CE484222325ULL /* FNV-1a 64-bit offset basis    */
#define SERIAL_FNV_PRIME  0x00000100000001B3ULL /* FNV-1a 64-bit prime (odd!)     */
#define SERIAL_XOR_CONST  0x000000005DEECE66DULL /* Java LCG multiplier, reused    */
#define SERIAL_FMIX_C1    0xFF51AFD7ED558CCDULL  /* MurmurHash3 fmix64 constant 1 */
#define SERIAL_FMIX_C2    0xC4CEB9FE1A85EC53ULL  /* MurmurHash3 fmix64 constant 2 */

/* rotl64 — rotate a 64-bit word left by r (0 < r < 64).
 *
 * A rotate is NOT a shift: bits that fall off the top re-enter at the bottom.
 * We synthesize it from two shifts + an OR, which is the idiom every compiler
 * pattern-matches into the hardware `rol` instruction. Precondition 0<r<64 is
 * important: `x >> (64 - r)` with r==0 would be a 64-bit shift, which is
 * Undefined Behavior in C. We only ever call it with r==7, so we are safe. */
static inline uint64_t rotl64(uint64_t x, unsigned r)
{
    return (x << r) | (x >> (64 - r));
}

/* key_from_name — the serial-derivation transform. name is a NUL-terminated
 * username; the return value is the 64-bit key that the serial encodes.
 *
 * `static inline` so it costs nothing to share between translation units; the
 * standalone asm/demo.c mirrors this byte-for-byte so the committed assembly is
 * exactly the codegen a reverser would stare at. */
static inline uint64_t key_from_name(const char *name)
{
    uint64_t h = SERIAL_FNV_OFFSET;

    /* Walk the bytes as *unsigned* char: a name may contain bytes >= 0x80, and
     * we must fold in the same numeric value the keygen would, regardless of
     * whether plain `char` is signed on the platform. Sign-extension here would
     * silently produce different serials on different compilers — a real bug in
     * naive keygen-mes. */
    for (const unsigned char *p = (const unsigned char *)name; *p; ++p) {
        h ^= (uint64_t)*p;         /* fold in the message byte (xor)            */
        h *= SERIAL_FNV_PRIME;     /* mix: multiply mod 2^64 by an odd prime    */
        h  = rotl64(h, 7);         /* diffuse the churned high bits downward    */
        h ^= SERIAL_XOR_CONST;     /* extra constant diffusion                  */
    }

    /* fmix64 avalanche (MurmurHash3). Each step is individually invertible, but
     * together they scramble the bits so the output has no visible structure —
     * exactly what stops a reader from pattern-matching name->serial by eye. */
    h ^= h >> 33;
    h *= SERIAL_FMIX_C1;
    h ^= h >> 29;
    h *= SERIAL_FMIX_C2;
    h ^= h >> 33;
    return h;
}

/* format_serial — render the 64-bit key as GGGG-GGGG-GGGG-GGGG (uppercase hex).
 *
 * out MUST be at least 20 bytes: 4 groups * 4 hex digits + 3 dashes = 19, plus
 * the terminating NUL. We do the hex conversion by hand (no snprintf) so the
 * standalone asm/demo.c can be identical and header-free, and so the codegen you
 * reverse is a clean table lookup rather than a call into libc's printf. */
static inline void format_serial(uint64_t key, char out[20])
{
    static const char HEX[] = "0123456789ABCDEF";
    unsigned oi = 0;                       /* write cursor into out[]           */

    for (unsigned gi = 0; gi < 4; ++gi) {
        /* Slice out group gi as a 16-bit field, most-significant group first:
         * gi=0 -> bits 63..48, gi=1 -> 47..32, gi=2 -> 31..16, gi=3 -> 15..0. */
        unsigned group = (unsigned)((key >> (48 - 16 * gi)) & 0xFFFFu);

        out[oi++] = HEX[(group >> 12) & 0xF]; /* nibble 3 (most significant)    */
        out[oi++] = HEX[(group >>  8) & 0xF]; /* nibble 2                       */
        out[oi++] = HEX[(group >>  4) & 0xF]; /* nibble 1                       */
        out[oi++] = HEX[ group        & 0xF]; /* nibble 0 (least significant)   */

        if (gi != 3)                          /* dash between groups, not after */
            out[oi++] = '-';
    }
    out[oi] = '\0';                            /* NUL-terminate (oi == 19 here)  */
}

/* ct_equal — constant-time byte comparison of a[0..n) vs b[0..n).
 *
 * DEFENSE LESSON (why not just strcmp?). A naive `strcmp`/`memcmp` returns as
 * soon as it finds the first mismatching byte. That early-out is a TIMING
 * ORACLE: an attacker who can measure how long the check takes learns *how many
 * leading bytes were correct*, and can recover a secret one byte at a time
 * (O(256*len) tries instead of O(256^len)). The same class of bug has broken
 * real HMAC and password checks. The fix is to look at *every* byte regardless
 * of outcome and fold the differences into one accumulator, so the running time
 * depends only on n, never on the data. Returns 1 iff all n bytes are equal. */
static inline int ct_equal(const char *a, const char *b, size_t n)
{
    unsigned diff = 0;                     /* stays 0 only if every byte matches */
    for (size_t i = 0; i < n; ++i)
        diff |= (unsigned)((unsigned char)a[i] ^ (unsigned char)b[i]);
    /* Branch-free "is diff zero?" — the standard constant-time reduction that
     * works for ANY 32-bit diff (not just byte-bounded ones): for every nonzero
     * value, `diff | (0 - diff)` has bit 31 set (the value or its two's-complement
     * negation is negative), so `>> 31` is 1; for diff==0 it is 0. We XOR with 1
     * to turn "any difference" into 0 and "no difference" into 1. The result
     * depends only on n, never on where a mismatch occurred. */
    return (int)(1u ^ ((diff | (0u - diff)) >> 31));
}

#endif /* SERIAL_H */
