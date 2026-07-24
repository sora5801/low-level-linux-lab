/* ===========================================================================
 * cpuid.c — "what can THIS CPU do?", the correct way.
 * ===========================================================================
 *
 * Every SIMD dispatch starts here. The CPUID instruction is x86's self-
 * description mechanism: you put a "leaf" number in EAX (and sometimes a
 * "subleaf" in ECX), execute `cpuid`, and the CPU fills EAX/EBX/ECX/EDX with
 * feature bits. We read two leaves:
 *
 *     leaf 1  (basic features):  EDX bit 26 = SSE2, ECX bit 20 = SSE4.2,
 *                                ECX bit 27 = OSXSAVE, ECX bit 28 = AVX
 *     leaf 7, subleaf 0 (ext.):  EBX bit  5 = AVX2
 *
 * THE SUBTLE PART — why a CPUID bit is not enough for AVX.
 * -------------------------------------------------------
 * AVX/AVX2 use the 256-bit YMM registers. Those registers are only preserved
 * across a context switch if the OS turned on XSAVE management of that state.
 * If we run a VEX-encoded AVX instruction on a kernel that does NOT save YMM,
 * the upper halves silently get clobbered by any other thread — a heisenbug.
 * Intel's documented protocol is therefore a THREE-part check:
 *
 *     1. CPUID.1:ECX.OSXSAVE[27] == 1   (the OS enabled XSAVE / the XCR0 reg)
 *     2. CPUID.1:ECX.AVX[28]     == 1   (the CPU has AVX)
 *     3. XGETBV(0) low bits & 0x6 == 0x6 (OS is saving BOTH XMM(bit1)+YMM(bit2))
 *
 * Only if all three hold may we use AVX. AVX2 additionally needs leaf-7 bit 5.
 * Skipping step 3 is the classic "works on my machine, corrupts on theirs" bug,
 * so we do it properly below.
 * ===========================================================================
 */
#include "simd_primitives.h"

/* Thin wrapper over the CPUID instruction.
 *
 * ABI notes that matter here:
 *   - CPUID clobbers EAX/EBX/ECX/EDX. In position-independent code EBX is also
 *     the GOT base register, so the compiler must be free to save/restore it;
 *     letting the "=b" output constraint own EBX handles that for us.
 *   - ECX is an INPUT too (the subleaf), which is why we use the "count" form
 *     and pass `subleaf` in "c". Leaf 7 genuinely depends on ECX; leaf 1
 *     ignores it, so passing 0 there is harmless.
 */
static inline void cpuid_count(uint32_t leaf, uint32_t subleaf,
                               uint32_t *a, uint32_t *b,
                               uint32_t *c, uint32_t *d)
{
    __asm__ volatile ("cpuid"
        : "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d)  /* out: the four result regs */
        : "a"(leaf), "c"(subleaf));               /* in : EAX=leaf, ECX=subleaf*/
}

/* Read extended control register 0 (XCR0) via XGETBV.
 *
 * XGETBV takes the register index in ECX (0 = XCR0) and returns the 64-bit
 * value in EDX:EAX. We only need the low 32 bits (XMM=bit1, YMM=bit2). Note
 * XGETBV itself is only legal once OSXSAVE is set, so callers MUST gate this
 * behind the OSXSAVE check — executing it otherwise faults with #UD.
 */
static inline uint64_t xgetbv0(void)
{
    uint32_t eax, edx;
    /* ".byte" spelling of `xgetbv` keeps this assembling on older toolchains
     * that don't recognize the mnemonic; ECX=0 selects XCR0. */
    __asm__ volatile (".byte 0x0f, 0x01, 0xd0"    /* xgetbv */
        : "=a"(eax), "=d"(edx)
        : "c"(0));
    return ((uint64_t)edx << 32) | eax;
}

cpu_features_t simd_detect_features(void)
{
    cpu_features_t f = {0, 0, 0, 0};
    uint32_t a, b, c, d;

    /* How many basic CPUID leaves does this CPU support? Leaf 0 returns the
     * max leaf in EAX. If it can't even report leaf 1, it is far too old to be
     * an x86-64 chip, but we check anyway rather than trust the hardware. */
    cpuid_count(0, 0, &a, &b, &c, &d);
    uint32_t max_leaf = a;
    if (max_leaf < 1)
        return f;                    /* no feature info at all — all flags 0   */

    /* ---- leaf 1: the baseline feature word ------------------------------- */
    cpuid_count(1, 0, &a, &b, &c, &d);
    f.sse2  = (d >> 26) & 1;         /* EDX[26] — guaranteed set on any x86-64 */
    f.sse42 = (c >> 20) & 1;         /* ECX[20]                                */

    int osxsave = (c >> 27) & 1;     /* ECX[27] — OS has enabled XSAVE/XCR0    */
    int avx_cpu = (c >> 28) & 1;     /* ECX[28] — silicon supports AVX         */

    /* AVX is usable only if the OS is actually preserving YMM state. */
    if (osxsave && avx_cpu) {
        uint64_t xcr0 = xgetbv0();
        /* bit1 = XMM saved, bit2 = YMM saved; need BOTH for legal AVX use. */
        if ((xcr0 & 0x6) == 0x6) {
            f.avx = 1;

            /* ---- leaf 7, subleaf 0: the "structured extended" word ------ */
            if (max_leaf >= 7) {
                cpuid_count(7, 0, &a, &b, &c, &d);
                f.avx2 = (b >> 5) & 1;   /* EBX[5] — but only meaningful given */
            }                            /*   the YMM-save guarantee above.    */
        }
    }
    return f;
}
