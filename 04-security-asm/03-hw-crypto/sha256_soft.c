/* ===========================================================================
 * sha256_soft.c — the portable SHA-256 block compression (FIPS 180-4 §6.2).
 * ===========================================================================
 *
 * This is the plain 32-bit-arithmetic reference. It is the fallback when the CPU
 * has no SHA extensions, and — just as important here — it is the ORACLE we
 * check the hardware path against: main.c hashes the same inputs with this and
 * with sha256_ni_transform and asserts they agree, and both agree with the NIST
 * vectors. If a hardware instruction behaved differently, this catches it.
 *
 * There are no lookup tables indexed by message bytes — SHA-256 is all shifts,
 * rotates, ANDs and adds over PUBLIC round constants — so this is naturally
 * constant-time. (Contrast aes_ct.c, where avoiding tables took real effort.)
 * ===========================================================================
 */
#include "sha256.h"

/* The 64 round constants K: fractional parts of the cube roots of the first 64
 * primes (FIPS 180-4 §4.2.2). Fixed and public — never indexed by a secret. */
static const uint32_t K[64] = {
    0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
    0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
    0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
    0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
    0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
    0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
    0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
    0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u
};

/* Circular right rotate — the core diffusion primitive of SHA-2. */
static inline uint32_t rotr(uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }

/* The four SHA-256 mixing functions (FIPS 180-4 §4.1.2). */
static inline uint32_t Ch (uint32_t x,uint32_t y,uint32_t z){ return (x & y) ^ (~x & z); }
static inline uint32_t Maj(uint32_t x,uint32_t y,uint32_t z){ return (x & y) ^ (x & z) ^ (y & z); }
static inline uint32_t BSIG0(uint32_t x){ return rotr(x,2) ^ rotr(x,13) ^ rotr(x,22); }
static inline uint32_t BSIG1(uint32_t x){ return rotr(x,6) ^ rotr(x,11) ^ rotr(x,25); }
static inline uint32_t SSIG0(uint32_t x){ return rotr(x,7) ^ rotr(x,18) ^ (x >> 3);  }
static inline uint32_t SSIG1(uint32_t x){ return rotr(x,17)^ rotr(x,19) ^ (x >> 10); }

void sha256_soft_transform(uint32_t state[8], const uint8_t *data, size_t nblocks)
{
    for (size_t b = 0; b < nblocks; b++) {
        const uint8_t *p = data + b * SHA256_BLOCK_BYTES;

        /* 1. Build the 64-word message schedule W. The first 16 words are the
         *    block, read BIG-ENDIAN; the rest are derived by the recurrence. */
        uint32_t W[64];
        for (int t = 0; t < 16; t++)
            W[t] = ((uint32_t)p[4*t] << 24) | ((uint32_t)p[4*t+1] << 16)
                 | ((uint32_t)p[4*t+2] << 8) |  (uint32_t)p[4*t+3];
        for (int t = 16; t < 64; t++)
            W[t] = SSIG1(W[t-2]) + W[t-7] + SSIG0(W[t-15]) + W[t-16];

        /* 2. Initialize the eight working variables from the current state. */
        uint32_t a=state[0],bb=state[1],c=state[2],d=state[3];
        uint32_t e=state[4],f=state[5],g=state[6],h=state[7];

        /* 3. 64 rounds of compression. */
        for (int t = 0; t < 64; t++) {
            uint32_t T1 = h + BSIG1(e) + Ch(e,f,g) + K[t] + W[t];
            uint32_t T2 = BSIG0(a) + Maj(a,bb,c);
            h=g; g=f; f=e; e=d+T1; d=c; c=bb; bb=a; a=T1+T2;
        }

        /* 4. Add the working variables back into the state (Davies-Meyer). */
        state[0]+=a; state[1]+=bb; state[2]+=c; state[3]+=d;
        state[4]+=e; state[5]+=f;  state[6]+=g; state[7]+=h;
    }
}
