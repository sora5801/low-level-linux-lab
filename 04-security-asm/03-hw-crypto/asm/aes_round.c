/* ===========================================================================
 * asm/aes_round.c — a minimal one-block AES-128 encryption, isolated so the
 *                   `aesenc` round is easy to see in the generated assembly.
 * ===========================================================================
 *
 * The point of this file is the DISASSEMBLY, not the C. It contains exactly the
 * AES-128 hot path — an initial key XOR, nine `aesenc` rounds, and one
 * `aesenclast` — with nothing else around it, so clang emits a clean run of AES
 * instructions. asm/aes_round.annotated.s walks that output line by line and is
 * the file that fulfills the "annotate the aesenc round in asm" requirement.
 *
 * Generate the raw asm with (the Makefile's `asm` target does this):
 *   clang --target=x86_64-pc-linux-gnu -S -O1 -maes -msse4.1 \
 *         -fno-asynchronous-unwind-tables -fno-jump-tables aes_round.c -o aes_round.s
 *
 * Each `aesenc xmm, xmm` performs a WHOLE AES round in one instruction:
 * ShiftRows, then SubBytes (the S-box, in hardware — no table in the cache),
 * then MixColumns, then XOR with the round key operand. `aesenclast` is the same
 * minus MixColumns (the final round). Must be compiled with `-maes`.
 * ===========================================================================
 */
#include <wmmintrin.h>   /* __m128i, _mm_aesenc_si128, _mm_aesenclast_si128    */
#include <emmintrin.h>   /* _mm_loadu_si128, _mm_storeu_si128, _mm_xor_si128   */

/* in[16] plaintext, out[16] ciphertext, rk = 11 expanded round keys (176 bytes).
 * The round keys are assumed already expanded (see aes_ni.c). */
void aes128_encrypt_block(const unsigned char in[16],
                          unsigned char out[16],
                          const unsigned char rk[176])
{
    const __m128i *k = (const __m128i *)rk;
    __m128i m = _mm_loadu_si128((const __m128i *)in);

    m = _mm_xor_si128(m, k[0]);          /* round 0: AddRoundKey (whitening)    */
    m = _mm_aesenc_si128(m, k[1]);       /* round 1                             */
    m = _mm_aesenc_si128(m, k[2]);       /* round 2                             */
    m = _mm_aesenc_si128(m, k[3]);       /* round 3                             */
    m = _mm_aesenc_si128(m, k[4]);       /* round 4                             */
    m = _mm_aesenc_si128(m, k[5]);       /* round 5                             */
    m = _mm_aesenc_si128(m, k[6]);       /* round 6                             */
    m = _mm_aesenc_si128(m, k[7]);       /* round 7                             */
    m = _mm_aesenc_si128(m, k[8]);       /* round 8                             */
    m = _mm_aesenc_si128(m, k[9]);       /* round 9                             */
    m = _mm_aesenclast_si128(m, k[10]);  /* round 10: no MixColumns             */

    _mm_storeu_si128((__m128i *)out, m);
}
