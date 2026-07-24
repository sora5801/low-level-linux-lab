/* ===========================================================================
 * aes_ni.c — AES-128 / AES-256 using the AES-NI instruction set.
 * ===========================================================================
 *
 * AES-NI adds six instructions (Westmere, 2010). Five matter here:
 *
 *   aesenc          xmm1, xmm2   : one FULL middle round of AES on xmm1 =
 *                                  ShiftRows -> SubBytes -> MixColumns ->
 *                                  AddRoundKey(xmm2).  ~4 cycle latency.
 *   aesenclast      xmm1, xmm2   : the LAST round (no MixColumns).
 *   aesdec / aesdeclast          : the inverse-cipher counterparts.
 *   aeskeygenassist xmm1, imm8   : does SubWord+RotWord and XORs the round
 *                                  constant `imm8`, the heavy lifting of the
 *                                  key schedule.
 *   aesimc          xmm1, xmm2   : InvMixColumns, used to turn an *encryption*
 *                                  round key into the form `aesdec` expects.
 *
 * WHY THIS IS ALSO A SIDE-CHANNEL WIN.  Every one of these instructions computes
 * the S-box in hardware with a fixed latency and NO memory access — there is no
 * lookup table in the cache to probe. So AES-NI is not just faster than a table
 * driven software AES, it is immune to the cache-timing attacks that plague it.
 * That is the "defense" half of this project: prefer the hardware instruction;
 * where you can't, use the constant-time software path in aes_ct.c.
 *
 * These intrinsics live in <wmmintrin.h> (AES) and <emmintrin.h> (SSE2 for
 * __m128i loads/xors). This translation unit MUST be compiled with `-maes
 * -msse4.1`; the Makefile does exactly that, and main.c only ever *calls* these
 * functions after cpuid says AES-NI is present.
 * ===========================================================================
 */
#include "aes.h"
#include <wmmintrin.h>   /* aesenc / aesenclast / aeskeygenassist / aesimc     */
#include <emmintrin.h>   /* _mm_loadu_si128, _mm_xor_si128, ...                */
#include <smmintrin.h>   /* SSE4.1 (kept for parity with build flags)          */

/* Load/store helpers. `loadu`/`storeu` are UNALIGNED moves (movdqu): AES inputs
 * come from arbitrary caller buffers, so we must not assume 16-byte alignment. */
static inline __m128i load128(const uint8_t *p)  { return _mm_loadu_si128((const __m128i *)p); }
static inline void    store128(uint8_t *p, __m128i v) { _mm_storeu_si128((__m128i *)p, v); }

/* ===========================================================================
 * AES-128 key expansion.
 *
 * Rijndael's key schedule fills round key i from round key i-1. aeskeygenassist
 * does SubWord(RotWord(word)) and XORs the round constant, but only for one
 * word; the "assist" helper below propagates it across the four words of a
 * 128-bit round key with three shift-and-XOR steps. This is Intel's canonical
 * sequence (AES-NI white paper, Shay Gueron).
 * ===========================================================================
 */

/* Combine the previous round key `key` with the aeskeygenassist output `gen`
 * to produce the next 128-bit round key.
 *
 *   _mm_shuffle_epi32(gen, 0xFF) : broadcast the top 32-bit word (the one that
 *                                  holds SubWord(RotWord(w3)) ^ Rcon) into all
 *                                  four lanes — that is the word the schedule
 *                                  wants to fold in.
 *   the three (slli_si128 by 4 ; xor) steps : compute the running XOR
 *      w4 = w0 ^ t
 *      w5 = w1 ^ w4 = w0 ^ w1 ^ t
 *      w6 = w2 ^ w5 = w0 ^ w1 ^ w2 ^ t   ... exactly the Rijndael recurrence,
 *   done in parallel across the 128-bit register by shifting the whole vector
 *   left one 32-bit word (slli_si128 shifts by BYTES, so 4 bytes = one word). */
static inline __m128i aes128_assist(__m128i key, __m128i gen)
{
    gen = _mm_shuffle_epi32(gen, 0xFF);          /* broadcast g3 to all lanes  */
    key = _mm_xor_si128(key, _mm_slli_si128(key, 4));
    key = _mm_xor_si128(key, _mm_slli_si128(key, 4));
    key = _mm_xor_si128(key, _mm_slli_si128(key, 4));
    return _mm_xor_si128(key, gen);
}

static void aes128_expand(const uint8_t *user_key, __m128i rk[11])
{
    /* Round key 0 is the raw user key. */
    rk[0]  = load128(user_key);
    /* Rounds 1..10: each uses the next Rcon (01,02,04,...,1b,36). The Rcon is
     * the imm8 to aeskeygenassist; it is PUBLIC (part of the standard), so this
     * whole schedule is constant-time regardless of the secret key bytes. */
    rk[1]  = aes128_assist(rk[0],  _mm_aeskeygenassist_si128(rk[0],  0x01));
    rk[2]  = aes128_assist(rk[1],  _mm_aeskeygenassist_si128(rk[1],  0x02));
    rk[3]  = aes128_assist(rk[2],  _mm_aeskeygenassist_si128(rk[2],  0x04));
    rk[4]  = aes128_assist(rk[3],  _mm_aeskeygenassist_si128(rk[3],  0x08));
    rk[5]  = aes128_assist(rk[4],  _mm_aeskeygenassist_si128(rk[4],  0x10));
    rk[6]  = aes128_assist(rk[5],  _mm_aeskeygenassist_si128(rk[5],  0x20));
    rk[7]  = aes128_assist(rk[6],  _mm_aeskeygenassist_si128(rk[6],  0x40));
    rk[8]  = aes128_assist(rk[7],  _mm_aeskeygenassist_si128(rk[7],  0x80));
    rk[9]  = aes128_assist(rk[8],  _mm_aeskeygenassist_si128(rk[8],  0x1B)); /* Rcon wraps in GF(2^8): x^8 = 0x1B */
    rk[10] = aes128_assist(rk[9],  _mm_aeskeygenassist_si128(rk[9],  0x36));
}

/* ===========================================================================
 * AES-256 key expansion.
 *
 * AES-256 has a 32-byte key = two 128-bit halves = eight 32-bit words, and it
 * generates a fresh round key every FOUR words. So the schedule alternates two
 * assist steps: one that uses aeskeygenassist+Rcon (word index % 8 == 0) and one
 * that only applies SubWord with NO rotate and NO Rcon (word index % 8 == 4) —
 * the extra SubWord that only AES-256 performs. This is Intel's canonical pair
 * of helpers.
 * ===========================================================================
 */

/* The Rcon-bearing step: fold `gen` (broadcast of its word 3) into `t1`. */
static inline void aes256_assist1(__m128i *t1, __m128i gen)
{
    __m128i t;
    gen = _mm_shuffle_epi32(gen, 0xFF);          /* word3 -> all lanes         */
    t   = _mm_slli_si128(*t1, 4);  *t1 = _mm_xor_si128(*t1, t);
    t   = _mm_slli_si128(t,   4);  *t1 = _mm_xor_si128(*t1, t);
    t   = _mm_slli_si128(t,   4);  *t1 = _mm_xor_si128(*t1, t);
    *t1 = _mm_xor_si128(*t1, gen);
}

/* The SubWord-only step (the AES-256-specific extra SubBytes on the key words).
 * Note imm8=0: we want SubWord WITHOUT a round constant, and we take word 2 of
 * the result via the 0xAA broadcast (SubWord lands there, no RotWord applied). */
static inline void aes256_assist2(__m128i t1, __m128i *t3)
{
    __m128i gen = _mm_aeskeygenassist_si128(t1, 0x00);
    __m128i t;
    gen = _mm_shuffle_epi32(gen, 0xAA);          /* word2 (SubWord, no rotate) */
    t   = _mm_slli_si128(*t3, 4);  *t3 = _mm_xor_si128(*t3, t);
    t   = _mm_slli_si128(t,   4);  *t3 = _mm_xor_si128(*t3, t);
    t   = _mm_slli_si128(t,   4);  *t3 = _mm_xor_si128(*t3, t);
    *t3 = _mm_xor_si128(*t3, gen);
}

static void aes256_expand(const uint8_t *user_key, __m128i rk[15])
{
    __m128i t1 = load128(user_key);       /* first 16 key bytes  -> rk[0]      */
    __m128i t3 = load128(user_key + 16);  /* second 16 key bytes -> rk[1]      */
    rk[0] = t1;
    rk[1] = t3;
    /* Each Rcon feeds one assist1 (produces the even round key) followed by one
     * assist2 (produces the odd round key), until we have 15 round keys. The
     * last group needs only assist1 (rk[14]); we stop there. */
    __m128i g;
    g = _mm_aeskeygenassist_si128(t3, 0x01); aes256_assist1(&t1, g); rk[2]  = t1; aes256_assist2(t1, &t3); rk[3]  = t3;
    g = _mm_aeskeygenassist_si128(t3, 0x02); aes256_assist1(&t1, g); rk[4]  = t1; aes256_assist2(t1, &t3); rk[5]  = t3;
    g = _mm_aeskeygenassist_si128(t3, 0x04); aes256_assist1(&t1, g); rk[6]  = t1; aes256_assist2(t1, &t3); rk[7]  = t3;
    g = _mm_aeskeygenassist_si128(t3, 0x08); aes256_assist1(&t1, g); rk[8]  = t1; aes256_assist2(t1, &t3); rk[9]  = t3;
    g = _mm_aeskeygenassist_si128(t3, 0x10); aes256_assist1(&t1, g); rk[10] = t1; aes256_assist2(t1, &t3); rk[11] = t3;
    g = _mm_aeskeygenassist_si128(t3, 0x20); aes256_assist1(&t1, g); rk[12] = t1; aes256_assist2(t1, &t3); rk[13] = t3;
    g = _mm_aeskeygenassist_si128(t3, 0x40); aes256_assist1(&t1, g); rk[14] = t1;
}

/* ===========================================================================
 * Public key-expansion entry point.
 *
 * We build the encryption schedule with the helpers above, then derive the
 * DECRYPTION schedule for `aesdec`. `aesdec` performs InvSubBytes + InvShiftRows
 * + InvMixColumns + AddRoundKey, so the round keys it consumes must already have
 * InvMixColumns applied (the "equivalent inverse cipher", FIPS-197 §5.3.5). We
 * apply `aesimc` (InvMixColumns) to the interior round keys and reverse their
 * order; the first/last keys are used as-is by aesdeclast / the initial XOR.
 * ===========================================================================
 */
void aes_ni_expand(aes_key *k, const uint8_t *user_key, int key_bits)
{
    __m128i enc[15];

    if (key_bits == 256) { k->nr = 14; aes256_expand(user_key, enc); }
    else                 { k->nr = 10; aes128_expand(user_key, enc); }

    int nr = k->nr;

    /* Stash the encryption schedule as bytes. */
    for (int i = 0; i <= nr; i++)
        store128(k->rk + i * 16, enc[i]);

    /* Decryption schedule: dec[0] = enc[nr]; dec[nr] = enc[0];
     * dec[i] = aesimc(enc[nr-i]) for the interior rounds. */
    store128(k->dec_rk + 0,        enc[nr]);
    for (int i = 1; i < nr; i++)
        store128(k->dec_rk + i * 16, _mm_aesimc_si128(enc[nr - i]));
    store128(k->dec_rk + nr * 16,  enc[0]);
}

/* ===========================================================================
 * Single-block encrypt / decrypt.  This is the hot loop the README dissects and
 * the asm/aes_round.annotated.s file annotates instruction-by-instruction.
 * ===========================================================================
 */
void aes_ni_encrypt(const aes_key *k, const uint8_t in[16], uint8_t out[16])
{
    const __m128i *rk = (const __m128i *)k->rk;
    __m128i m = load128(in);
    int nr = k->nr;

    m = _mm_xor_si128(m, rk[0]);               /* initial AddRoundKey (whitening)*/
    for (int i = 1; i < nr; i++)
        m = _mm_aesenc_si128(m, rk[i]);        /* nr-1 full rounds               */
    m = _mm_aesenclast_si128(m, rk[nr]);       /* final round: no MixColumns     */

    store128(out, m);
}

void aes_ni_decrypt(const aes_key *k, const uint8_t in[16], uint8_t out[16])
{
    const __m128i *dk = (const __m128i *)k->dec_rk;
    __m128i m = load128(in);
    int nr = k->nr;

    m = _mm_xor_si128(m, dk[0]);               /* AddRoundKey with last enc key  */
    for (int i = 1; i < nr; i++)
        m = _mm_aesdec_si128(m, dk[i]);        /* inverse rounds (imc'd keys)    */
    m = _mm_aesdeclast_si128(m, dk[nr]);       /* final inverse round            */

    store128(out, m);
}
