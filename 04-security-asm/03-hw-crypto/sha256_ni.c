/* ===========================================================================
 * sha256_ni.c — SHA-256 block compression using the x86 SHA extensions.
 * ===========================================================================
 *
 * The Intel SHA extensions (CPUID.(EAX=7).EBX[29], on AMD since Zen and Intel
 * since Goldmont/Ice Lake) add three instructions that collapse SHA-256's inner
 * loop from ~64 scalar rounds into a handful of 128-bit ops:
 *
 *   sha256rnds2 xmm1, xmm2, xmm0 : perform TWO SHA-256 rounds. xmm1/xmm2 carry
 *                                  the eight state words packed as {A,B,E,F} and
 *                                  {C,D,G,H}; xmm0's low two dwords supply the
 *                                  two (W_t + K_t) round inputs.
 *   sha256msg1  xmm1, xmm2       : first half of the message-schedule update
 *                                  (the sigma0 mixing for four future words).
 *   sha256msg2  xmm1, xmm2       : second half (the sigma1 mixing), completing
 *                                  four new W words at once.
 *
 * So the schedule and the rounds that sha256_soft.c does word-by-word happen
 * four-words / two-rounds at a time in hardware, with fixed latency and no
 * data-dependent memory access. This is the canonical Gulley/Walton structure
 * (public domain), reorganized and commented for teaching.
 *
 * DATA LAYOUT GYMNASTICS.  The instructions want the state as {A,B,E,F} and
 * {C,D,G,H}, but FIPS/our driver keep it as {A,B,C,D,E,F,G,H}. And SHA-256 reads
 * message words big-endian while x86 loads them little-endian. The shuffles at
 * the top/bottom convert state order once per call, and BYTE_MASK byte-swaps
 * each 16-byte message load. Getting these permutations exactly right is the
 * whole difficulty of using these instructions — hence the heavy comments.
 *
 * Compile with `-msha -msse4.1 -mssse3`. Only call after cpuid confirms SHA.
 * ===========================================================================
 */
#include "sha256.h"
#include <immintrin.h>   /* _mm_sha256* live here (pulls in the SSE headers)   */

void sha256_ni_transform(uint32_t state[8], const uint8_t *data, size_t nblocks)
{
    __m128i STATE0, STATE1;                       /* {A,B,E,F} and {C,D,G,H}    */
    __m128i MSG, TMP;
    __m128i MSG0, MSG1, MSG2, MSG3;               /* rolling message schedule   */
    __m128i ABEF_SAVE, CDGH_SAVE;                 /* feed-forward across a block */

    /* Byte-swap mask: turn each little-endian 32-bit lane of a loaded 16-byte
     * chunk into big-endian, since SHA-256 defines W words big-endian. Applied
     * with pshufb (_mm_shuffle_epi8). */
    const __m128i BYTE_MASK =
        _mm_set_epi64x((long long)0x0c0d0e0f08090a0bULL,
                       (long long)0x0405060700010203ULL);

    /* --- Load and RE-ORDER the incoming state into {A,B,E,F},{C,D,G,H}. -----
     * state[] is A B C D E F G H in memory. We need STATE0={A,B,E,F} and
     * STATE1={C,D,G,H}. The shuffles/alignr/blend below perform that once. */
    TMP    = _mm_loadu_si128((const __m128i *)&state[0]); /* A B C D            */
    STATE1 = _mm_loadu_si128((const __m128i *)&state[4]); /* E F G H            */

    TMP    = _mm_shuffle_epi32(TMP,    0xB1);   /* C D A B                      */
    STATE1 = _mm_shuffle_epi32(STATE1, 0x1B);   /* H G F E -> as words: E F G H reversed => {H,G,F,E}? see note */
    STATE0 = _mm_alignr_epi8(TMP, STATE1, 8);   /* A B E F                      */
    STATE1 = _mm_blend_epi16(STATE1, TMP, 0xF0);/* C D G H                      */

    while (nblocks--) {
        /* Feed-forward: SHA-2 adds the pre-block state back after 64 rounds. */
        ABEF_SAVE = STATE0;
        CDGH_SAVE = STATE1;

        /* Rounds 0-3. Load 16 message bytes, byte-swap to big-endian (MSG0),
         * add K[0..3], run two double-rounds. _mm_shuffle_epi32(MSG,0x0E) slides
         * the high two (W+K) dwords down for the second sha256rnds2. */
        MSG0 = _mm_shuffle_epi8(_mm_loadu_si128((const __m128i *)(data +  0)), BYTE_MASK);
        MSG  = _mm_add_epi32(MSG0, _mm_set_epi64x((long long)0xE9B5DBA5B5C0FBCFULL,(long long)0x71374491428A2F98ULL));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        MSG    = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);

        /* Rounds 4-7. sha256msg1 begins mixing MSG0 with MSG1 for future words. */
        MSG1 = _mm_shuffle_epi8(_mm_loadu_si128((const __m128i *)(data + 16)), BYTE_MASK);
        MSG  = _mm_add_epi32(MSG1, _mm_set_epi64x((long long)0xAB1C5ED5923F82A4ULL,(long long)0x59F111F13956C25BULL));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        MSG    = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
        MSG0   = _mm_sha256msg1_epu32(MSG0, MSG1);

        /* Rounds 8-11. */
        MSG2 = _mm_shuffle_epi8(_mm_loadu_si128((const __m128i *)(data + 32)), BYTE_MASK);
        MSG  = _mm_add_epi32(MSG2, _mm_set_epi64x((long long)0x550C7DC3243185BEULL,(long long)0x12835B01D807AA98ULL));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        MSG    = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
        MSG1   = _mm_sha256msg1_epu32(MSG1, MSG2);

        /* Rounds 12-15. From here each group also runs sha256msg2 to FINISH the
         * four words that sha256msg1 started, using a palignr of the two most
         * recent message vectors as the "W[t-7]" contribution. */
        MSG3 = _mm_shuffle_epi8(_mm_loadu_si128((const __m128i *)(data + 48)), BYTE_MASK);
        MSG  = _mm_add_epi32(MSG3, _mm_set_epi64x((long long)0xC19BF1749BDC06A7ULL,(long long)0x80DEB1FE72BE5D74ULL));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        TMP    = _mm_alignr_epi8(MSG3, MSG2, 4);
        MSG0   = _mm_add_epi32(MSG0, TMP);
        MSG0   = _mm_sha256msg2_epu32(MSG0, MSG3);
        MSG    = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
        MSG2   = _mm_sha256msg1_epu32(MSG2, MSG3);

        /* Rounds 16-19. */
        MSG  = _mm_add_epi32(MSG0, _mm_set_epi64x((long long)0x240CA1CC0FC19DC6ULL,(long long)0xEFBE4786E49B69C1ULL));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        TMP    = _mm_alignr_epi8(MSG0, MSG3, 4);
        MSG1   = _mm_add_epi32(MSG1, TMP);
        MSG1   = _mm_sha256msg2_epu32(MSG1, MSG0);
        MSG    = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
        MSG3   = _mm_sha256msg1_epu32(MSG3, MSG0);

        /* Rounds 20-23. */
        MSG  = _mm_add_epi32(MSG1, _mm_set_epi64x((long long)0x76F988DA5CB0A9DCULL,(long long)0x4A7484AA2DE92C6FULL));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        TMP    = _mm_alignr_epi8(MSG1, MSG0, 4);
        MSG2   = _mm_add_epi32(MSG2, TMP);
        MSG2   = _mm_sha256msg2_epu32(MSG2, MSG1);
        MSG    = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
        MSG0   = _mm_sha256msg1_epu32(MSG0, MSG1);

        /* Rounds 24-27. */
        MSG  = _mm_add_epi32(MSG2, _mm_set_epi64x((long long)0xBF597FC7B00327C8ULL,(long long)0xA831C66D983E5152ULL));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        TMP    = _mm_alignr_epi8(MSG2, MSG1, 4);
        MSG3   = _mm_add_epi32(MSG3, TMP);
        MSG3   = _mm_sha256msg2_epu32(MSG3, MSG2);
        MSG    = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
        MSG1   = _mm_sha256msg1_epu32(MSG1, MSG2);

        /* Rounds 28-31. */
        MSG  = _mm_add_epi32(MSG3, _mm_set_epi64x((long long)0x1429296706CA6351ULL,(long long)0xD5A79147C6E00BF3ULL));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        TMP    = _mm_alignr_epi8(MSG3, MSG2, 4);
        MSG0   = _mm_add_epi32(MSG0, TMP);
        MSG0   = _mm_sha256msg2_epu32(MSG0, MSG3);
        MSG    = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
        MSG2   = _mm_sha256msg1_epu32(MSG2, MSG3);

        /* Rounds 32-35. */
        MSG  = _mm_add_epi32(MSG0, _mm_set_epi64x((long long)0x53380D134D2C6DFCULL,(long long)0x2E1B213827B70A85ULL));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        TMP    = _mm_alignr_epi8(MSG0, MSG3, 4);
        MSG1   = _mm_add_epi32(MSG1, TMP);
        MSG1   = _mm_sha256msg2_epu32(MSG1, MSG0);
        MSG    = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
        MSG3   = _mm_sha256msg1_epu32(MSG3, MSG0);

        /* Rounds 36-39. */
        MSG  = _mm_add_epi32(MSG1, _mm_set_epi64x((long long)0x92722C8581C2C92EULL,(long long)0x766A0ABB650A7354ULL));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        TMP    = _mm_alignr_epi8(MSG1, MSG0, 4);
        MSG2   = _mm_add_epi32(MSG2, TMP);
        MSG2   = _mm_sha256msg2_epu32(MSG2, MSG1);
        MSG    = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
        MSG0   = _mm_sha256msg1_epu32(MSG0, MSG1);

        /* Rounds 40-43. */
        MSG  = _mm_add_epi32(MSG2, _mm_set_epi64x((long long)0xC76C51A3C24B8B70ULL,(long long)0xA81A664BA2BFE8A1ULL));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        TMP    = _mm_alignr_epi8(MSG2, MSG1, 4);
        MSG3   = _mm_add_epi32(MSG3, TMP);
        MSG3   = _mm_sha256msg2_epu32(MSG3, MSG2);
        MSG    = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
        MSG1   = _mm_sha256msg1_epu32(MSG1, MSG2);

        /* Rounds 44-47. */
        MSG  = _mm_add_epi32(MSG3, _mm_set_epi64x((long long)0x106AA070F40E3585ULL,(long long)0xD6990624D192E819ULL));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        TMP    = _mm_alignr_epi8(MSG3, MSG2, 4);
        MSG0   = _mm_add_epi32(MSG0, TMP);
        MSG0   = _mm_sha256msg2_epu32(MSG0, MSG3);
        MSG    = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
        MSG2   = _mm_sha256msg1_epu32(MSG2, MSG3);

        /* Rounds 48-51. */
        MSG  = _mm_add_epi32(MSG0, _mm_set_epi64x((long long)0x34B0BCB52748774CULL,(long long)0x1E376C0819A4C116ULL));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        TMP    = _mm_alignr_epi8(MSG0, MSG3, 4);
        MSG1   = _mm_add_epi32(MSG1, TMP);
        MSG1   = _mm_sha256msg2_epu32(MSG1, MSG0);
        MSG    = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
        MSG3   = _mm_sha256msg1_epu32(MSG3, MSG0);

        /* Rounds 52-55. */
        MSG  = _mm_add_epi32(MSG1, _mm_set_epi64x((long long)0x682E6FF35B9CCA4FULL,(long long)0x4ED8AA4A391C0CB3ULL));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        TMP    = _mm_alignr_epi8(MSG1, MSG0, 4);
        MSG2   = _mm_add_epi32(MSG2, TMP);
        MSG2   = _mm_sha256msg2_epu32(MSG2, MSG1);
        MSG    = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);

        /* Rounds 56-59. */
        MSG  = _mm_add_epi32(MSG2, _mm_set_epi64x((long long)0x8CC7020884C87814ULL,(long long)0x78A5636F748F82EEULL));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        TMP    = _mm_alignr_epi8(MSG2, MSG1, 4);
        MSG3   = _mm_add_epi32(MSG3, TMP);
        MSG3   = _mm_sha256msg2_epu32(MSG3, MSG2);
        MSG    = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);

        /* Rounds 60-63: last group, no more schedule updates needed. */
        MSG  = _mm_add_epi32(MSG3, _mm_set_epi64x((long long)0xC67178F2BEF9A3F7ULL,(long long)0xA4506CEB90BEFFFAULL));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        MSG    = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);

        /* Feed-forward add (the "+= working vars" of the Davies-Meyer step). */
        STATE0 = _mm_add_epi32(STATE0, ABEF_SAVE);
        STATE1 = _mm_add_epi32(STATE1, CDGH_SAVE);

        data += SHA256_BLOCK_BYTES;
    }

    /* --- Un-shuffle {A,B,E,F},{C,D,G,H} back to A B C D E F G H and store. --- */
    TMP    = _mm_shuffle_epi32(STATE0, 0x1B);   /* F E B A                      */
    STATE1 = _mm_shuffle_epi32(STATE1, 0xB1);   /* D C H G                      */
    STATE0 = _mm_blend_epi16(TMP, STATE1, 0xF0);/* D C B A -> as words A B C D  */
    STATE1 = _mm_alignr_epi8(STATE1, TMP, 8);   /* H G F E -> as words E F G H  */

    _mm_storeu_si128((__m128i *)&state[0], STATE0);
    _mm_storeu_si128((__m128i *)&state[4], STATE1);
}
