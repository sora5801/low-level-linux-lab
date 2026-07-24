/* ===========================================================================
 * simd_primitives.h — one API, four primitives, four implementations each.
 * ===========================================================================
 *
 * This header is the contract shared by every translation unit in the project.
 * For each of the four primitives (strlen, memchr, memcpy, UTF-8 validation)
 * we ship, and let the reader diff, up to FOUR implementations:
 *
 *     scalar   — the obvious byte-at-a-time reference (scalar.c). Correct by
 *                inspection; the yardstick every SIMD version must match.
 *     intrin   — SSE2/AVX2 written with <immintrin.h> intrinsics (simd_intrin.c).
 *                Portable C, but the compiler picks registers/scheduling.
 *     asm      — hand-written GNU assembly (simd_asm.S), SysV AMD64 ABI. The
 *                "star artifact": every instruction is commented.
 *     best     — a function pointer resolved ONCE at startup from CPUID, the way
 *                glibc's IFUNC resolvers pick a memcpy for your exact CPU.
 *
 * PLATFORM: the intrinsics and scalar code are portable C, but simd_asm.S uses
 * the System V AMD64 calling convention (args in rdi, rsi, rdx). So the FULL
 * program links and runs on Linux / WSL x86-64 only. See README.md.
 * ===========================================================================
 */
#ifndef SIMD_PRIMITIVES_H
#define SIMD_PRIMITIVES_H

#include <stddef.h>   /* size_t                                              */
#include <stdint.h>   /* uint8_t, uint32_t, uint64_t                          */

/* ---------------------------------------------------------------------------
 * CPU feature detection (cpuid.c)
 *
 * A single struct of yes/no flags, filled from the CPUID instruction. AVX/AVX2
 * additionally require the OS to have enabled the YMM register state (XSAVE),
 * which is why avx/avx2 are gated on an XGETBV check, not just CPUID bits. If
 * you skip the XGETBV check you will happily execute a VEX-encoded instruction
 * on a kernel that never saves the upper 128 bits of your YMM registers across
 * a context switch — a rare but real corruption bug. cpuid.c does it right.
 * --------------------------------------------------------------------------- */
typedef struct {
    int sse2;    /* CPUID.01H:EDX.SSE2  [bit 26] — baseline on all x86-64      */
    int sse42;   /* CPUID.01H:ECX.SSE4.2[bit 20] — gives us the fast POPCNT era*/
    int avx;     /* CPUID.01H:ECX.AVX   [bit 28] AND OS XSAVE of YMM enabled   */
    int avx2;    /* CPUID.07H:EBX.AVX2  [bit  5] AND OS XSAVE of YMM enabled   */
} cpu_features_t;

cpu_features_t simd_detect_features(void);

/* ---------------------------------------------------------------------------
 * Scalar reference implementations (scalar.c). Deliberately simple.
 * --------------------------------------------------------------------------- */
size_t strlen_scalar(const char *s);
void  *memchr_scalar(const void *s, int c, size_t n);
void  *memcpy_scalar(void *dst, const void *src, size_t n);
int    utf8_validate_scalar(const uint8_t *data, size_t len);   /* 1=valid,0=not*/

/* Validate the ONE codepoint starting at data[i]; returns its length (1..4) or
 * 0 if malformed / truncated. Shared: the SIMD validator calls this whenever it
 * leaves the ASCII fast path, so its i+k<len bounds checks protect both paths. */
size_t utf8_decode_one(const uint8_t *data, size_t len, size_t i);

/* ---------------------------------------------------------------------------
 * Intrinsics implementations (simd_intrin.c). SSE2 baseline + AVX2 upgrades.
 * --------------------------------------------------------------------------- */
size_t strlen_sse2_intrin(const char *s);
size_t strlen_avx2_intrin(const char *s);
void  *memchr_sse2_intrin(const void *s, int c, size_t n);
void  *memcpy_sse2_intrin(void *dst, const void *src, size_t n);
int    utf8_validate_simd(const uint8_t *data, size_t len);     /* SSE fast path*/

/* ---------------------------------------------------------------------------
 * Hand-written assembly (simd_asm.S). SysV AMD64 ABI — Linux only.
 * These are the "star artifact"; read simd_asm.S alongside these prototypes.
 * --------------------------------------------------------------------------- */
size_t strlen_avx2_asm(const char *s);
void  *memchr_sse2_asm(const void *s, int c, size_t n);
void  *memcpy_erms_asm(void *dst, const void *src, size_t n);   /* rep movsb    */
void  *memcpy_sse_asm(void *dst, const void *src, size_t n);    /* movdqu loop  */

/* ---------------------------------------------------------------------------
 * Runtime dispatch (dispatch.c) — the IFUNC pattern.
 *
 * Call simd_init() once at program start. It runs CPUID and points each of the
 * four *_best pointers at the fastest implementation your CPU can actually run.
 * After that, calling through the pointer costs one indirect branch (predicted
 * perfectly after the first call), which is exactly how glibc dispatches its
 * hand-tuned memcpy/strlen/memchr the very first time you call them.
 * --------------------------------------------------------------------------- */
void simd_init(void);

extern size_t (*strlen_best)(const char *s);
extern void  *(*memchr_best)(const void *s, int c, size_t n);
extern void  *(*memcpy_best)(void *dst, const void *src, size_t n);
extern int    (*utf8_validate_best)(const uint8_t *data, size_t len);

#endif /* SIMD_PRIMITIVES_H */
