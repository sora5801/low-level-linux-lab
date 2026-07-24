/* ===========================================================================
 * dispatch.c — pick the fastest implementation at RUNTIME (the IFUNC pattern).
 * ===========================================================================
 *
 * You cannot bake "use AVX2" into a binary you want to ship: a user's CPU might
 * be a decade old. The standard fix, used by glibc for memcpy/strlen/memchr, is
 * to detect the CPU ONCE and route every later call through a function pointer
 * aimed at the best variant that CPU can run. glibc does this with the ELF
 * "IFUNC" mechanism (the dynamic linker calls a resolver at load time and
 * patches the PLT); we do the same idea by hand with plain function pointers.
 *
 * Cost model: after simd_init(), each call is one indirect branch. The CPU's
 * indirect-branch predictor nails it after the first call (the target never
 * changes), so the overhead is effectively zero — you pay a load and a
 * perfectly-predicted jump, and in return you run code tuned for THIS CPU.
 * ===========================================================================
 */
#include "simd_primitives.h"

/* The public pointers. Before simd_init() they point at the always-correct
 * scalar reference, so calling one "too early" is safe, just not fast. */
size_t (*strlen_best)(const char *s)                          = strlen_scalar;
void  *(*memchr_best)(const void *s, int c, size_t n)         = memchr_scalar;
void  *(*memcpy_best)(void *dst, const void *src, size_t n)   = memcpy_scalar;
int    (*utf8_validate_best)(const uint8_t *data, size_t len) = utf8_validate_scalar;

void simd_init(void)
{
    cpu_features_t f = simd_detect_features();

    /* strlen: AVX2 hand-asm is the best we ship; then SSE2 intrinsics; then
     * the scalar fallback for the (theoretical) CPU without SSE2. Note we
     * prefer the HAND-ASM AVX2 over the intrinsics AVX2 purely to exercise the
     * star artifact — in practice they perform within noise of each other. */
    if (f.avx2)
        strlen_best = strlen_avx2_asm;
    else if (f.sse2)
        strlen_best = strlen_sse2_intrin;
    /* else: stays strlen_scalar */

    /* memchr: our vector path is SSE2 (16 bytes), which covers every x86-64. */
    if (f.sse2)
        memchr_best = memchr_sse2_asm;

    /* memcpy: on ERMS-class hardware `rep movsb` is the throughput king, so we
     * pick the hand-asm ERMS routine whenever we have SSE2 (a proxy for "a real
     * x86-64, therefore modern microcode"). A more precise dispatch would test
     * CPUID.07H:EBX.ERMS[bit 9] and fall back to the SSE copy loop otherwise;
     * we keep the ERMS routine as the default and expose the SSE one for the
     * benchmark's A/B comparison. */
    if (f.sse2)
        memcpy_best = memcpy_erms_asm;

    /* UTF-8: the SSE ASCII fast path needs only SSE2. */
    if (f.sse2)
        utf8_validate_best = utf8_validate_simd;
}
