/* ===========================================================================
 * simd_intrin.c — the same four primitives in SSE2/AVX2 intrinsics.
 * ===========================================================================
 *
 * <immintrin.h> exposes the SSE/AVX instruction set as C "intrinsic" functions:
 * each `_mm_*` call maps (almost) 1:1 to a machine instruction, but the compiler
 * still allocates registers and schedules. This is the sweet spot for most SIMD
 * work — you get the vector ISA without hand-writing a prologue.
 *
 * THE ONE TRICK that powers strlen and memchr here:
 *
 *     pcmpeqb   xmmA, xmmB     ; per-byte compare: lane = 0xFF if equal, else 0
 *     pmovmskb  xmmA -> eax    ; gather the HIGH BIT of each byte into an int
 *
 * After pcmpeqb, a matching byte is 0xFF (high bit set) and a non-match is 0x00
 * (high bit clear). pmovmskb then packs those 16 (SSE) or 32 (AVX2) high bits
 * into a scalar integer, so a single `tzcnt`/`bsf` finds the FIRST match's byte
 * index. That is how you locate a NUL or a target byte 16/32 bytes at a time.
 *
 * THE ONE HAZARD: a 16/32-byte load can touch bytes past the caller's data. For
 * strlen (no length is given) we must never read across a page boundary into an
 * unmapped page. We avoid it by ALIGNING THE POINTER DOWN to the vector width
 * and masking off the bytes before the string: an aligned N-byte load, where N
 * divides the 4096-byte page size, can never straddle a page boundary. For
 * memchr/memcpy the length IS known, so we vectorize the full 16-byte chunks
 * and finish the <16-byte tail scalar-ly — again never over-reading.
 * ===========================================================================
 */
#include "simd_primitives.h"
#include <immintrin.h>   /* SSE2..AVX2 intrinsics                             */
#include <stdint.h>

/* WHY THE __attribute__((target(...))) ON EACH FUNCTION:
 * We compile this whole file at the x86-64 baseline (SSE2). The per-function
 * `target("avx2")` attribute lets ONLY that function use VEX-encoded AVX2
 * instructions, and — crucially — stops the compiler from sprinkling AVX
 * encodings into the SSE2 functions. That way a binary built here still runs on
 * a pre-AVX2 CPU, and the avx2 code is reached only after CPUID says it's safe.
 * This is the compiler-supported form of "function multiversioning". */

/* ---------------------------------------------------------------------------
 * strlen, SSE2 — 16 bytes per iteration.
 * --------------------------------------------------------------------------- */
__attribute__((target("sse2")))
size_t strlen_sse2_intrin(const char *s)
{
    const __m128i zero = _mm_setzero_si128();      /* compare target = NUL     */

    /* Split the start address into an aligned base and a misalignment. */
    uintptr_t addr     = (uintptr_t)s;
    size_t    misalign = addr & 15;                /* 0..15 bytes into a block */
    const char *aligned = (const char *)(addr & ~(uintptr_t)15);

    /* First (possibly partial) block: aligned 16B load is page-safe. */
    __m128i blk  = _mm_load_si128((const __m128i *)aligned);
    unsigned m   = (unsigned)_mm_movemask_epi8(_mm_cmpeq_epi8(blk, zero));
    m >>= misalign;                                /* discard bytes before s   */
    if (m)                                         /* NUL in the first block?  */
        return (size_t)__builtin_ctz(m);           /* offset from s            */

    /* Steady state: full aligned 16-byte loads until a zero byte appears. */
    const char *p = aligned + 16;
    for (;;) {
        blk = _mm_load_si128((const __m128i *)p);
        m   = (unsigned)_mm_movemask_epi8(_mm_cmpeq_epi8(blk, zero));
        if (m)
            return (size_t)(p - s) + (size_t)__builtin_ctz(m);
        p += 16;
    }
}

/* ---------------------------------------------------------------------------
 * strlen, AVX2 — 32 bytes per iteration (twice the throughput of SSE2).
 * Identical shape; wider vectors. _mm256_movemask_epi8 yields a 32-bit mask.
 * --------------------------------------------------------------------------- */
__attribute__((target("avx2")))
size_t strlen_avx2_intrin(const char *s)
{
    const __m256i zero = _mm256_setzero_si256();

    uintptr_t addr     = (uintptr_t)s;
    size_t    misalign = addr & 31;                /* 0..31                    */
    const char *aligned = (const char *)(addr & ~(uintptr_t)31);

    __m256i blk = _mm256_load_si256((const __m256i *)aligned);
    unsigned m  = (unsigned)_mm256_movemask_epi8(_mm256_cmpeq_epi8(blk, zero));
    m >>= misalign;
    if (m)
        return (size_t)__builtin_ctz(m);

    const char *p = aligned + 32;
    for (;;) {
        blk = _mm256_load_si256((const __m256i *)p);
        m   = (unsigned)_mm256_movemask_epi8(_mm256_cmpeq_epi8(blk, zero));
        if (m)
            return (size_t)(p - s) + (size_t)__builtin_ctz(m);
        p += 32;
    }
    /* Note: a production AVX2 strlen ends with `vzeroupper` (the compiler
     * inserts it here) to avoid the SSE/AVX transition stall on later legacy-
     * SSE code. Our hand-asm version does it explicitly — see simd_asm.S. */
}

/* ---------------------------------------------------------------------------
 * memchr, SSE2 — 16 bytes per iteration, scalar tail. Length is known, so no
 * page-safety dance: we simply never load past the last full 16-byte chunk.
 * --------------------------------------------------------------------------- */
__attribute__((target("sse2")))
void *memchr_sse2_intrin(const void *s, int c, size_t n)
{
    const unsigned char *p = (const unsigned char *)s;
    unsigned char target   = (unsigned char)c;     /* memchr compares a BYTE   */
    __m128i needle = _mm_set1_epi8((char)target);   /* broadcast c to 16 lanes */

    while (n >= 16) {
        __m128i blk = _mm_loadu_si128((const __m128i *)p);  /* unaligned load  */
        unsigned m  = (unsigned)_mm_movemask_epi8(_mm_cmpeq_epi8(blk, needle));
        if (m)
            return (void *)(p + __builtin_ctz(m));  /* first matching byte     */
        p += 16;
        n -= 16;
    }
    /* 0..15 leftover bytes: scalar, so we never read past s+n. */
    while (n--) {
        if (*p == target) return (void *)p;
        p++;
    }
    return NULL;
}

/* ---------------------------------------------------------------------------
 * memcpy, SSE2 — 16 bytes per iteration with unaligned load/store, scalar tail.
 * This is the "no ERMS" teaching path; the hand-asm file shows the rep-movsb
 * ERMS path glibc actually prefers on modern CPUs. memcpy has no overlap
 * contract, so a straight forward copy is correct.
 * --------------------------------------------------------------------------- */
__attribute__((target("sse2")))
void *memcpy_sse2_intrin(void *dst, const void *src, size_t n)
{
    unsigned char       *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;

    while (n >= 16) {
        __m128i blk = _mm_loadu_si128((const __m128i *)s);
        _mm_storeu_si128((__m128i *)d, blk);
        d += 16; s += 16; n -= 16;
    }
    while (n--) *d++ = *s++;         /* copy the <16-byte tail                 */
    return dst;
}

/* ---------------------------------------------------------------------------
 * UTF-8 validation, SIMD-accelerated ASCII fast path.
 *
 * Real-world UTF-8 (source code, JSON, HTML, English/most-of-the-web text) is
 * overwhelmingly ASCII. So the cheap win is: scan 16 bytes at once, and if the
 * whole block has every high bit clear it is 16 valid 1-byte codepoints — skip
 * it. Only when a block contains a >=0x80 byte do we fall back to the fully-
 * checked scalar decoder (scalar.c) for exactly one codepoint, then resume.
 *
 * WHY THIS IS CORRECT AT BLOCK BOUNDARIES: an all-ASCII block advances i by a
 * multiple of 16, and every ASCII byte is a self-contained codepoint, so we
 * always land on a codepoint boundary. When we drop to scalar we consume one
 * WHOLE codepoint (1..4 bytes, possibly straddling the next block), with the
 * decoder's i+k<len checks preventing any over-read. The vector step is a pure
 * accelerator; the scalar path remains the source of truth.
 *
 * SCOPE (honest): this is NOT a full-vector validator. The state-of-the-art
 * (Lemire/Keiser "simdutf") validates continuation-byte structure and ranges
 * entirely in vectors with no scalar fallback. That is a large, subtle
 * algorithm; here we teach the fast-path idea and the pmovmskb structure and
 * point at simdutf in the README for the complete version.
 * --------------------------------------------------------------------------- */
__attribute__((target("sse2")))
int utf8_validate_simd(const uint8_t *data, size_t len)
{
    size_t i = 0;

    while (i + 16 <= len) {
        __m128i blk = _mm_loadu_si128((const __m128i *)(data + i));
        /* movemask gathers each byte's HIGH BIT. For raw bytes that high bit
         * is exactly "byte >= 0x80", i.e. "not ASCII". mask==0 => all ASCII. */
        int mask = _mm_movemask_epi8(blk);
        if (mask == 0) {
            i += 16;                 /* 16 clean ASCII bytes — skip wholesale  */
            continue;
        }
        /* Non-ASCII somewhere in this block: hand exactly one codepoint to the
         * proven scalar decoder, which validates ranges/overlongs/surrogates. */
        size_t step = utf8_decode_one(data, len, i);
        if (step == 0) return 0;
        i += step;
    }

    /* Tail (< 16 bytes) — all scalar. */
    while (i < len) {
        size_t step = utf8_decode_one(data, len, i);
        if (step == 0) return 0;
        i += step;
    }
    return 1;
}
