/* ===========================================================================
 * scalar.c — the reference implementations. Correct by inspection.
 * ===========================================================================
 *
 * These are the yardstick. Every SIMD version (intrinsics or hand-asm) must
 * return BIT-IDENTICAL results to these for every input. They are written to
 * be obviously correct, not fast: one byte at a time, no cleverness. When a
 * fuzzing differential in bench.c disagrees, THIS file is presumed right.
 * ===========================================================================
 */
#include "simd_primitives.h"

/* strlen: walk until the NUL terminator, return the byte distance. O(n). */
size_t strlen_scalar(const char *s)
{
    const char *p = s;
    while (*p) p++;              /* stop at the first 0x00 byte               */
    return (size_t)(p - s);      /* pointer difference = length, NUL excluded */
}

/* memchr: find the first byte equal to (unsigned char)c in the first n bytes.
 * Returns a pointer to it, or NULL. Note c is truncated to a byte — that is
 * the C standard's contract, and getting it wrong (comparing full ints) is a
 * classic bug the SIMD versions must also avoid. */
void *memchr_scalar(const void *s, int c, size_t n)
{
    const unsigned char *p = (const unsigned char *)s;
    unsigned char target = (unsigned char)c;   /* compare as a byte           */
    for (size_t i = 0; i < n; i++)
        if (p[i] == target)
            return (void *)(p + i);
    return NULL;                                /* not found in n bytes        */
}

/* memcpy: copy n bytes forward. UB (and here simply unhandled) if the regions
 * overlap — that is memmove's job. Returns dst, per the C contract. */
void *memcpy_scalar(void *dst, const void *src, size_t n)
{
    unsigned char       *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    for (size_t i = 0; i < n; i++)
        d[i] = s[i];
    return dst;
}

/* ---------------------------------------------------------------------------
 * UTF-8 validation — the fully-checked reference.
 *
 * UTF-8 is not "any bytes with the right high bits". A *valid* stream forbids:
 *   - overlong encodings   (e.g. 0xC0 0x80 for U+0000 — must be 1 byte)
 *   - surrogates           (U+D800..U+DFFF are illegal scalar values)
 *   - out-of-range         (> U+10FFFF)
 *   - stray continuations  (a 0x80..0xBF byte with no lead)
 *   - truncated sequences  (a lead byte with too few continuations)
 *
 * The canonical way to enforce all of that WITHOUT a giant if-tree is the
 * "well-formed UTF-8 byte sequences" table from the Unicode standard (Table
 * 3-7). It constrains the SECOND byte's range based on the lead, which is
 * exactly what rejects overlongs and surrogates:
 *
 *   lead        2nd byte     3rd/4th
 *   00..7F      —            —            (ASCII, 1 byte)
 *   C2..DF      80..BF       —            (2 bytes)
 *   E0          A0..BF       80..BF       (E0 80..9F would be overlong)
 *   E1..EC      80..BF       80..BF
 *   ED          80..9F       80..BF       (ED A0..BF would be a surrogate)
 *   EE..EF      80..BF       80..BF
 *   F0          90..BF       80..BF 80..BF(F0 80..8F would be overlong)
 *   F1..F3      80..BF       80..BF 80..BF
 *   F4          80..8F       80..BF 80..BF(F4 90.. would exceed U+10FFFF)
 *   else                                  (C0,C1,F5..FF and lone 80..BF: bad)
 * --------------------------------------------------------------------------- */

/* A continuation byte is exactly 10xxxxxx == 0x80..0xBF. */
static inline int is_cont(uint8_t b) { return (b & 0xC0) == 0x80; }

/* Validate ONE sequence starting at data[i], with `len` the total buffer size.
 * Returns the number of bytes the codepoint occupies (1..4), or 0 if the bytes
 * at data[i] are not the start of a well-formed sequence (or run off the end).
 *
 * This is the workhorse the SIMD validator (simd_intrin.c) also calls whenever
 * it hits a non-ASCII block — so its bounds checks (i+k < len) are what keep
 * the vector path from ever reading past the buffer. */
size_t utf8_decode_one(const uint8_t *data, size_t len, size_t i);
size_t utf8_decode_one(const uint8_t *data, size_t len, size_t i)
{
    uint8_t b0 = data[i];

    /* 1-byte: plain ASCII. */
    if (b0 < 0x80)
        return 1;

    /* 2-byte: C2..DF  80..BF. (C0/C1 are overlong leads — rejected here.) */
    if (b0 >= 0xC2 && b0 <= 0xDF) {
        if (i + 1 >= len) return 0;               /* truncated                */
        if (!is_cont(data[i + 1])) return 0;
        return 2;
    }

    /* 3-byte: E0..EF, with a lead-specific range on the 2nd byte. */
    if (b0 >= 0xE0 && b0 <= 0xEF) {
        if (i + 2 >= len) return 0;               /* need 2 continuations     */
        uint8_t b1 = data[i + 1], b2 = data[i + 2];
        if (!is_cont(b2)) return 0;
        /* second-byte lower/upper bounds keyed by the lead: */
        uint8_t lo = 0x80, hi = 0xBF;
        if (b0 == 0xE0) lo = 0xA0;                /* reject overlong           */
        else if (b0 == 0xED) hi = 0x9F;           /* reject surrogates D800..  */
        if (b1 < lo || b1 > hi) return 0;
        return 3;
    }

    /* 4-byte: F0..F4, again with a lead-specific 2nd-byte range. */
    if (b0 >= 0xF0 && b0 <= 0xF4) {
        if (i + 3 >= len) return 0;               /* need 3 continuations     */
        uint8_t b1 = data[i + 1], b2 = data[i + 2], b3 = data[i + 3];
        if (!is_cont(b2) || !is_cont(b3)) return 0;
        uint8_t lo = 0x80, hi = 0xBF;
        if (b0 == 0xF0) lo = 0x90;                /* reject overlong           */
        else if (b0 == 0xF4) hi = 0x8F;           /* cap at U+10FFFF           */
        if (b1 < lo || b1 > hi) return 0;
        return 4;
    }

    /* Everything else — C0, C1, F5..FF, and any lone 80..BF — is invalid. */
    return 0;
}

int utf8_validate_scalar(const uint8_t *data, size_t len)
{
    size_t i = 0;
    while (i < len) {
        size_t step = utf8_decode_one(data, len, i);
        if (step == 0)
            return 0;               /* malformed at offset i                  */
        i += step;                  /* advance past the whole codepoint       */
    }
    return 1;                       /* consumed every byte cleanly            */
}
