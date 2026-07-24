/* ===========================================================================
 * aes_ct.c — CONSTANT-TIME, TABLE-FREE software AES-128 / AES-256.
 * ===========================================================================
 *
 * THE THREAT THIS DEFENDS AGAINST
 * -------------------------------
 * The "textbook fast" software AES precomputes the S-box (and the combined
 * SubBytes+MixColumns "T-tables") as 256- or 1024-entry lookup tables, then
 * indexes them with *secret* bytes:  s = sbox[ state ^ key ].  The index is a
 * function of the key, so which cache line gets touched is a function of the
 * key. An attacker sharing the machine (another VM, another process, hyper-
 * thread sibling) can measure, via cache timing (PRIME+PROBE / FLUSH+RELOAD),
 * *which* table lines you touched and recover the key. This is not theoretical:
 * Bernstein (2005) and Osvik-Shamir-Tromer (2006) extracted full AES keys this
 * way. The defense is to make the memory access pattern and the branch pattern
 * INDEPENDENT of secrets.
 *
 * HOW THIS FILE STAYS CONSTANT-TIME
 * ---------------------------------
 * We never index memory with a secret and never branch on a secret. Instead we
 * compute the S-box arithmetically:
 *
 *   S(x) = Affine( x^{-1} in GF(2^8) )                     [FIPS-197 §5.1.1]
 *
 * The multiplicative inverse is x^254 (because the multiplicative group has
 * order 255, so x^255 = 1 and x^{-1} = x^254; and 0^254 = 0, matching the AES
 * convention that S maps 0 through the inverse-of-0 := 0). We raise to the 254th
 * power by square-and-multiply. The exponent 254 is a PUBLIC constant, so the
 * "if this bit is set, multiply" decisions are on public data, not secret data —
 * we can even keep them as straight-line code. Every GF multiply is branch-free
 * (fixed 8 iterations, masks instead of `if`). Result: the number of executed
 * instructions, the memory addresses touched, and the branch outcomes are all
 * independent of the key and the plaintext.
 *
 * COST.  This is dramatically slower than AES-NI or than a T-table AES — that is
 * the price of the side-channel guarantee, and it is exactly why you should
 * reach for AES-NI (also constant-time, in hardware) when the CPU has it. This
 * software path is the portable, safe fallback for when it does not.
 *
 * STATE LAYOUT.  We keep the 16-byte block as a flat array `s[0..15]` where
 * s[row + 4*col] is the AES state cell at (row, col) — the standard column-major
 * order, so a round key block XORs in byte-for-byte (see aes.h).
 * ===========================================================================
 */
#include "aes.h"

/* ---------------------------------------------------------------------------
 * GF(2^8) multiply, the AES field with reduction polynomial
 *   m(x) = x^8 + x^4 + x^3 + x + 1   (0x11B; the reduction byte is 0x1B).
 *
 * Russian-peasant multiply, made BRANCH-FREE:
 *   - "add a into the product if bit i of b is set" becomes an AND with a mask
 *     that is all-ones (0xFF) or all-zeros, derived by negating the bit. No `if`.
 *   - "xtime" (multiply a by x, i.e. shift left and conditionally reduce) uses
 *     the same mask trick on the high bit. No `if`.
 * The loop runs a FIXED 8 times regardless of operands, so timing is constant.
 * `mask = (uint8_t)-bit` relies on two's complement: -(1) = 0xFF, -(0) = 0x00.
 * --------------------------------------------------------------------------- */
static uint8_t gf_mul(uint8_t a, uint8_t b)
{
    uint8_t p = 0;
    for (int i = 0; i < 8; i++) {
        uint8_t add_mask = (uint8_t)(0u - (b & 1u));   /* 0xFF if b0 set else 0 */
        p ^= (uint8_t)(a & add_mask);                  /* conditional add       */

        uint8_t hi_mask  = (uint8_t)(0u - (a >> 7));   /* 0xFF if a7 set else 0 */
        a = (uint8_t)((a << 1) ^ (0x1B & hi_mask));    /* xtime with reduction  */
        b >>= 1;
    }
    return p;
}

/* Multiplicative inverse in GF(2^8): x^{-1} = x^254, by square-and-multiply.
 * The exponent bits (254 = 0b1111_1110) are compile-time constants, so the
 * control flow is fixed and secret-independent. 0 maps to 0 (gf_mul by 0). */
static uint8_t gf_inv(uint8_t x)
{
    uint8_t r = 1;
    /* bits of 254 from MSB to LSB: 1,1,1,1,1,1,1,0 */
    r = gf_mul(r, r); r = gf_mul(r, x);   /* bit 1 */
    r = gf_mul(r, r); r = gf_mul(r, x);   /* bit 1 */
    r = gf_mul(r, r); r = gf_mul(r, x);   /* bit 1 */
    r = gf_mul(r, r); r = gf_mul(r, x);   /* bit 1 */
    r = gf_mul(r, r); r = gf_mul(r, x);   /* bit 1 */
    r = gf_mul(r, r); r = gf_mul(r, x);   /* bit 1 */
    r = gf_mul(r, r); r = gf_mul(r, x);   /* bit 1 */
    r = gf_mul(r, r);                     /* bit 0 (final square, no multiply) */
    return r;                             /* r == x^254 == x^{-1}  (and 0->0)  */
}

/* Rotate an 8-bit value left by n (n in 1..7); used by the AES affine map. */
static inline uint8_t rotl8(uint8_t x, int n)
{
    return (uint8_t)((x << n) | (x >> (8 - n)));
}

/* The AES S-box, computed (not looked up):
 *   S(x) = y ^ rotl(y,1) ^ rotl(y,2) ^ rotl(y,3) ^ rotl(y,4) ^ 0x63,  y = x^{-1}
 * This bit-rotation form is the AES affine transformation written over a byte
 * (FIPS-197 §5.1.1). Constant-time: gf_inv is, and the rest is pure arithmetic. */
static uint8_t sbox(uint8_t x)
{
    uint8_t y = gf_inv(x);
    return (uint8_t)(y ^ rotl8(y,1) ^ rotl8(y,2) ^ rotl8(y,3) ^ rotl8(y,4) ^ 0x63);
}

/* The inverse S-box: apply the INVERSE affine map, then invert in the field.
 *   inv_affine(x) = rotl(x,1) ^ rotl(x,3) ^ rotl(x,6) ^ 0x05,  then  ^{-1}. */
static uint8_t inv_sbox(uint8_t x)
{
    uint8_t y = (uint8_t)(rotl8(x,1) ^ rotl8(x,3) ^ rotl8(x,6) ^ 0x05);
    return gf_inv(y);
}

/* ===========================================================================
 * Key expansion (FIPS-197 §5.2), byte-oriented, constant-time.
 *
 * Words are 4 bytes. Nk = key words (4 for AES-128, 8 for AES-256). For each new
 * word i >= Nk: temp = w[i-1]; at the start of each Nk-block, temp goes through
 * RotWord+SubWord and XOR with the round constant; AES-256 additionally applies
 * a bare SubWord at the i%Nk==4 position. Then w[i] = w[i-Nk] ^ temp. The S-box
 * here is our constant-time `sbox`, so expanding a SECRET key leaks nothing.
 * ===========================================================================
 */
void aes_ct_expand(aes_key *k, const uint8_t *user_key, int key_bits)
{
    int Nk = (key_bits == 256) ? 8 : 4;      /* key length in 32-bit words     */
    k->nr = (key_bits == 256) ? 14 : 10;
    int total_words = (k->nr + 1) * 4;       /* 44 for AES-128, 60 for AES-256 */

    uint8_t *w = k->rk;                       /* view the schedule as bytes     */

    /* Round constant, low byte of Rcon[j]; the rest of Rcon is zero. rc doubles
     * in GF(2^8): 01,02,04,...,80, then 1b,36 (the x^8 wrap). It is public. */
    uint8_t rc = 1;

    /* w[0..Nk-1] = the raw key words. */
    for (int i = 0; i < Nk * 4; i++)
        w[i] = user_key[i];

    for (int i = Nk; i < total_words; i++) {
        uint8_t t[4];
        int prev = (i - 1) * 4;               /* byte offset of word i-1        */
        t[0] = w[prev + 0]; t[1] = w[prev + 1];
        t[2] = w[prev + 2]; t[3] = w[prev + 3];

        if (i % Nk == 0) {
            /* RotWord: cyclic left shift of the 4 bytes. */
            uint8_t tmp = t[0]; t[0] = t[1]; t[1] = t[2]; t[2] = t[3]; t[3] = tmp;
            /* SubWord. */
            t[0] = sbox(t[0]); t[1] = sbox(t[1]);
            t[2] = sbox(t[2]); t[3] = sbox(t[3]);
            /* XOR the round constant into the first byte, then advance rc. */
            t[0] ^= rc;
            rc = gf_mul(rc, 0x02);            /* rc *= x in GF(2^8)             */
        } else if (Nk > 6 && i % Nk == 4) {
            /* AES-256 only: an extra SubWord with NO RotWord and NO Rcon. */
            t[0] = sbox(t[0]); t[1] = sbox(t[1]);
            t[2] = sbox(t[2]); t[3] = sbox(t[3]);
        }

        int cur  = i * 4;
        int back = (i - Nk) * 4;
        w[cur + 0] = (uint8_t)(w[back + 0] ^ t[0]);
        w[cur + 1] = (uint8_t)(w[back + 1] ^ t[1]);
        w[cur + 2] = (uint8_t)(w[back + 2] ^ t[2]);
        w[cur + 3] = (uint8_t)(w[back + 3] ^ t[3]);
    }
}

/* ===========================================================================
 * The round transformations.  All operate in place on the 16-byte state `s`.
 * ===========================================================================
 */

/* AddRoundKey: XOR in the 16 bytes of round key `round`. */
static void add_round_key(uint8_t s[16], const uint8_t *rk, int round)
{
    const uint8_t *k = rk + round * 16;
    for (int i = 0; i < 16; i++) s[i] ^= k[i];
}

/* SubBytes / InvSubBytes: apply the (computed) S-box to every cell. Note the
 * index `i` is a loop counter, never a secret — the secret is the *value* s[i],
 * which is fed to arithmetic, not used as an address. That is the whole trick. */
static void sub_bytes(uint8_t s[16])     { for (int i = 0; i < 16; i++) s[i] = sbox(s[i]); }
static void inv_sub_bytes(uint8_t s[16]) { for (int i = 0; i < 16; i++) s[i] = inv_sbox(s[i]); }

/* ShiftRows: cyclically left-shift row r by r cells. With flat index row+4*col,
 * new (r,c) = old (r, (c+r) mod 4). Row 0 is unchanged. */
static void shift_rows(uint8_t s[16])
{
    uint8_t t[16];
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
            t[r + 4 * c] = s[r + 4 * ((c + r) & 3)];
    for (int i = 0; i < 16; i++) s[i] = t[i];
}

/* InvShiftRows: cyclically RIGHT-shift row r by r cells. */
static void inv_shift_rows(uint8_t s[16])
{
    uint8_t t[16];
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
            t[r + 4 * c] = s[r + 4 * ((c - r) & 3)];
    for (int i = 0; i < 16; i++) s[i] = t[i];
}

/* MixColumns: multiply each column by the fixed MDS matrix
 *   [2 3 1 1; 1 2 3 1; 1 1 2 3; 3 1 1 2]  over GF(2^8). gf_mul is constant-time. */
static void mix_columns(uint8_t s[16])
{
    for (int c = 0; c < 4; c++) {
        uint8_t *col = s + 4 * c;
        uint8_t a0 = col[0], a1 = col[1], a2 = col[2], a3 = col[3];
        col[0] = (uint8_t)(gf_mul(a0,2) ^ gf_mul(a1,3) ^ a2 ^ a3);
        col[1] = (uint8_t)(a0 ^ gf_mul(a1,2) ^ gf_mul(a2,3) ^ a3);
        col[2] = (uint8_t)(a0 ^ a1 ^ gf_mul(a2,2) ^ gf_mul(a3,3));
        col[3] = (uint8_t)(gf_mul(a0,3) ^ a1 ^ a2 ^ gf_mul(a3,2));
    }
}

/* InvMixColumns: the inverse MDS matrix with coefficients {0e,0b,0d,09}. */
static void inv_mix_columns(uint8_t s[16])
{
    for (int c = 0; c < 4; c++) {
        uint8_t *col = s + 4 * c;
        uint8_t a0 = col[0], a1 = col[1], a2 = col[2], a3 = col[3];
        col[0] = (uint8_t)(gf_mul(a0,14) ^ gf_mul(a1,11) ^ gf_mul(a2,13) ^ gf_mul(a3, 9));
        col[1] = (uint8_t)(gf_mul(a0, 9) ^ gf_mul(a1,14) ^ gf_mul(a2,11) ^ gf_mul(a3,13));
        col[2] = (uint8_t)(gf_mul(a0,13) ^ gf_mul(a1, 9) ^ gf_mul(a2,14) ^ gf_mul(a3,11));
        col[3] = (uint8_t)(gf_mul(a0,11) ^ gf_mul(a1,13) ^ gf_mul(a2, 9) ^ gf_mul(a3,14));
    }
}

/* ===========================================================================
 * Public single-block encrypt / decrypt. Straight FIPS-197 round structure.
 * ===========================================================================
 */
void aes_ct_encrypt(const aes_key *k, const uint8_t in[16], uint8_t out[16])
{
    uint8_t s[16];
    for (int i = 0; i < 16; i++) s[i] = in[i];

    add_round_key(s, k->rk, 0);               /* initial whitening              */
    for (int round = 1; round < k->nr; round++) {
        sub_bytes(s);
        shift_rows(s);
        mix_columns(s);
        add_round_key(s, k->rk, round);
    }
    /* Final round omits MixColumns. */
    sub_bytes(s);
    shift_rows(s);
    add_round_key(s, k->rk, k->nr);

    for (int i = 0; i < 16; i++) out[i] = s[i];
}

void aes_ct_decrypt(const aes_key *k, const uint8_t in[16], uint8_t out[16])
{
    uint8_t s[16];
    for (int i = 0; i < 16; i++) s[i] = in[i];

    /* Inverse cipher (FIPS-197 §5.3), walking the SAME schedule backwards. */
    add_round_key(s, k->rk, k->nr);
    for (int round = k->nr - 1; round >= 1; round--) {
        inv_shift_rows(s);
        inv_sub_bytes(s);
        add_round_key(s, k->rk, round);
        inv_mix_columns(s);
    }
    inv_shift_rows(s);
    inv_sub_bytes(s);
    add_round_key(s, k->rk, 0);

    for (int i = 0; i < 16; i++) out[i] = s[i];
}
