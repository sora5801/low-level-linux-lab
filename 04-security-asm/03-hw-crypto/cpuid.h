/* ===========================================================================
 * cpuid.h — runtime CPU feature detection via the `cpuid` instruction.
 * ===========================================================================
 *
 * WHY THIS FILE EXISTS
 * --------------------
 * This project ships THREE code paths for the same math:
 *
 *   - AES via AES-NI    (aesenc/aesenclast/aeskeygenassist)     — needs AES-NI
 *   - SHA-256 via SHA-NI(sha256rnds2/sha256msg1/sha256msg2)     — needs SHA
 *   - a constant-time, table-free *software* AES               — needs nothing
 *
 * The hardware instructions are NOT present on every x86-64 CPU. AES-NI arrived
 * with Westmere (2010); the SHA extensions arrived far later (Goldmont 2016 on
 * Atom, Zen 2017 on AMD, and only Ice Lake / 2019+ on mainstream Intel Core).
 * If we *emit* `aesenc` and the CPU lacks it, the instruction is not decoded and
 * the CPU raises #UD — an "invalid opcode" fault that the kernel turns into
 * SIGILL and your process dies. So a program that wants to use these must ASK
 * the CPU, at runtime, whether they exist, and fall back if they don't. That
 * question is exactly what the `cpuid` instruction answers.
 *
 * This is the same "ISA dispatch" every real crypto library performs (OpenSSL's
 * `OPENSSL_ia32cap`, BoringSSL's `CRYPTO_is_*`, glibc's ifunc resolvers). It is
 * also the honest way to ship SIMD/crypto code: detect, then dispatch.
 * ===========================================================================
 */
#ifndef HWCRYPTO_CPUID_H
#define HWCRYPTO_CPUID_H

#include <stdint.h>

/* ---------------------------------------------------------------------------
 * cpuid_raw — execute one `cpuid` and return all four result registers.
 *
 * THE INSTRUCTION.  `cpuid` takes its "leaf" selector in EAX and (for some
 * leaves) a "subleaf" in ECX, then overwrites EAX/EBX/ECX/EDX with a packed
 * table of capability bits. It is a serializing instruction (it drains the
 * pipeline), which is why it is cheap-but-not-free and why we cache the result.
 *
 * THE rbx CAVEAT.  On 32-bit x86 position-independent code, EBX is the reserved
 * GOT pointer and clobbering it inside inline asm is a hard error — the classic
 * fix is to save/restore rbx by hand. On x86-64 (our target) rbx is an ordinary
 * callee-saved register and the compiler is perfectly happy to let `cpuid`
 * write it as an output operand, so we can name "=b" directly. We still mark it
 * as an output (not a clobber) so the compiler spills anything it had in rbx.
 *
 * We pass the leaf in "a"(leaf) and subleaf in "c"(subleaf): the "a"/"c" operand
 * letters pin those C values into EAX/ECX before the instruction runs, exactly
 * as the ISA requires.
 * --------------------------------------------------------------------------- */
static inline void cpuid_raw(uint32_t leaf, uint32_t subleaf,
                             uint32_t *eax, uint32_t *ebx,
                             uint32_t *ecx, uint32_t *edx)
{
    __asm__ volatile(
        "cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx) /* out: all four regs */
        : "a"(leaf), "c"(subleaf)                        /* in : leaf, subleaf */
        : /* no extra clobbers: every written reg is an output above */
    );
}

/* A tiny bag of the capabilities this project cares about. `int` flags (0/1)
 * because we only ever branch on "present?" — see hwc_detect() below. */
typedef struct {
    int aesni;   /* AES-NI:            CPUID.(EAX=1).ECX bit 25                */
    int sse41;   /* SSE4.1:            CPUID.(EAX=1).ECX bit 19 (pblendw etc.) */
    int ssse3;   /* SSSE3:             CPUID.(EAX=1).ECX bit  9 (pshufb/palignr)*/
    int sha;     /* SHA extensions:    CPUID.(EAX=7,ECX=0).EBX bit 29         */
} hwc_features;

/* Bit positions, spelled out so the detection reads like the Intel SDM tables.
 * (Intel SDM Vol.2A, "CPUID — CPU Identification", leaf 01H and leaf 07H.) */
#define HWC_ECX1_SSSE3  (1u << 9)   /* leaf 1, ECX[9]  = SSSE3                 */
#define HWC_ECX1_SSE41  (1u << 19)  /* leaf 1, ECX[19] = SSE4.1                */
#define HWC_ECX1_AESNI  (1u << 25)  /* leaf 1, ECX[25] = AES-NI               */
#define HWC_EBX7_SHA    (1u << 29)  /* leaf 7 subleaf 0, EBX[29] = SHA        */

/* ---------------------------------------------------------------------------
 * hwc_detect — fill an hwc_features from two CPUID leaves.
 *
 * Leaf 0 ("maximum standard leaf") tells us the highest leaf the CPU supports.
 * We must check it before reading leaf 7: on an ancient CPU that predates leaf
 * 7, executing `cpuid` with EAX=7 returns *undefined* garbage rather than zero,
 * so blindly testing EBX[29] could produce a false positive. Guarding on the
 * max-leaf is the SDM-mandated way to probe.
 * --------------------------------------------------------------------------- */
static inline hwc_features hwc_detect(void)
{
    hwc_features f = (hwc_features){0, 0, 0, 0};
    uint32_t a, b, c, d;

    /* Leaf 0: EAX = highest supported standard leaf number. */
    cpuid_raw(0, 0, &a, &b, &c, &d);
    uint32_t max_leaf = a;

    /* Leaf 1: the classic feature bits live in ECX/EDX. */
    cpuid_raw(1, 0, &a, &b, &c, &d);
    f.ssse3 = (c & HWC_ECX1_SSSE3) != 0;
    f.sse41 = (c & HWC_ECX1_SSE41) != 0;
    f.aesni = (c & HWC_ECX1_AESNI) != 0;

    /* Leaf 7, subleaf 0: extended features. Only valid if the CPU advertises
     * it via leaf 0. The SHA bit is EBX[29]. */
    if (max_leaf >= 7) {
        cpuid_raw(7, 0, &a, &b, &c, &d);
        f.sha = (b & HWC_EBX7_SHA) != 0;
    }
    return f;
}

#endif /* HWCRYPTO_CPUID_H */
