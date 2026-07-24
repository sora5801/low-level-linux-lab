/* ===========================================================================
 * aes.h — a tiny AES-128/256 API with TWO interchangeable back ends.
 * ===========================================================================
 *
 * The whole point of this project is that the same block cipher can be computed
 * two very different ways, and the choice has *security* consequences:
 *
 *   1. aes_ni_*   — uses the CPU's AES-NI instructions. One `aesenc` does a whole
 *                   round (SubBytes + ShiftRows + MixColumns + AddRoundKey) in a
 *                   handful of cycles, in hardware, with NO data-dependent memory
 *                   access. It is fast AND constant-time by construction.
 *
 *   2. aes_ct_*   — a pure-software implementation that is deliberately written
 *                   to be CONSTANT-TIME and TABLE-FREE. The naive software AES
 *                   uses 256-byte S-box lookup tables indexed by *secret* bytes;
 *                   that leaks the key through the data cache (the classic
 *                   Bernstein / Osvik-Shamir-Tromer cache-timing attacks). Ours
 *                   computes the S-box arithmetically in GF(2^8) with a fixed
 *                   sequence of AND/XOR/shift — no tables, no secret branches —
 *                   so its timing is independent of the key and the data.
 *
 * Both back ends implement the identical mathematics (FIPS-197) and MUST produce
 * byte-identical output; main.c proves that against the NIST vectors.
 *
 * SCOPE / HONESTY.  This is a teaching core: it implements the raw single-block
 * cipher (ECB of one 16-byte block) for AES-128 and AES-256, encrypt and decrypt.
 * A real library would add a mode of operation (CTR/GCM), never expose raw ECB,
 * and zeroize key material. The README says exactly what is and is not here.
 * ===========================================================================
 */
#ifndef HWCRYPTO_AES_H
#define HWCRYPTO_AES_H

#include <stdint.h>
#include <stddef.h>

/* AES always operates on a 128-bit (16-byte) block, regardless of key size. */
#define AES_BLOCK_BYTES 16

/* Number of rounds is a function of key length (FIPS-197 Table).
 *   AES-128: 10 rounds -> 11 round keys
 *   AES-256: 14 rounds -> 15 round keys
 * A round key is one 16-byte block, so the expanded schedule is
 * (rounds+1) * 16 bytes.  15 is the max (AES-256). */
#define AES_MAX_ROUNDS      14
#define AES_MAX_ROUND_KEYS  (AES_MAX_ROUNDS + 1)          /* 15                */
#define AES_MAX_RK_BYTES    (AES_MAX_ROUND_KEYS * AES_BLOCK_BYTES) /* 240      */

/* ---------------------------------------------------------------------------
 * Key schedule context, shared by both back ends.
 *
 * We keep the expanded round keys as a flat byte array in the SAME column-major
 * byte order the AES state uses, so a round key block can be XORed into the
 * state byte-for-byte. The AES-NI code reinterprets 16-byte groups as __m128i;
 * the software code indexes bytes. `nr` records how many rounds (10 or 14) so
 * one struct serves both key sizes.
 *
 * For decryption the AES-NI path needs a SEPARATE schedule (the round keys run
 * through `aesimc`, the inverse MixColumns, so `aesdec` can consume them). The
 * software path reuses the *same* schedule and simply walks it backwards, so it
 * has no `dec_rk`.
 * --------------------------------------------------------------------------- */
typedef struct {
    uint8_t rk[AES_MAX_RK_BYTES];      /* encryption round keys (both back ends)*/
    uint8_t dec_rk[AES_MAX_RK_BYTES];  /* AES-NI decryption round keys (aesimc) */
    int     nr;                        /* number of rounds: 10 (128) or 14 (256)*/
} aes_key;

/* ------------------------------ AES-NI back end --------------------------- */
/* These functions dereference AES-NI instructions and MUST only be called after
 * cpuid confirms AES-NI (see cpuid.h). Calling them on a CPU without AES-NI is
 * undefined-opcode -> SIGILL. `key_bits` is 128 or 256. */
void aes_ni_expand (aes_key *k, const uint8_t *user_key, int key_bits);
void aes_ni_encrypt(const aes_key *k, const uint8_t in[16], uint8_t out[16]);
void aes_ni_decrypt(const aes_key *k, const uint8_t in[16], uint8_t out[16]);

/* --------------------- constant-time software back end -------------------- */
/* Always safe to call on any CPU. Slower than AES-NI, but table-free and
 * branch-free so its execution time does not depend on the key or the data. */
void aes_ct_expand (aes_key *k, const uint8_t *user_key, int key_bits);
void aes_ct_encrypt(const aes_key *k, const uint8_t in[16], uint8_t out[16]);
void aes_ct_decrypt(const aes_key *k, const uint8_t in[16], uint8_t out[16]);

#endif /* HWCRYPTO_AES_H */
