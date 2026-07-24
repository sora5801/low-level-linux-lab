/* ===========================================================================
 * blake2s.c — BLAKE2s (RFC 7693) and the HMAC/HKDF built on top of it.
 * ===========================================================================
 *
 * BLAKE2s hashes a message in 64-byte blocks. Each block runs a compression
 * function G-mixing 16 message words into a 16-word working state over 10
 * rounds, using the same Add/Rotate/Xor moves as ChaCha (rotations 16/12/8/7).
 * A per-block byte counter `t` and a "last block" flag `f` are folded in so the
 * hash of a prefix can never collide with the hash of a longer message.
 *
 * This is a straight transcription of the RFC 7693 reference with commentary;
 * we use it only with a 32-byte digest and no keying (Noise keys the hash via
 * HMAC, not BLAKE2's native key input).
 * =========================================================================== */

#include "blake2s.h"

/* The initialization vector: the fractional parts of sqrt of the first 8 primes
 * (identical to SHA-256's IV). */
static const u32 BLAKE2S_IV[8] = {
    0x6A09E667u, 0xBB67AE85u, 0x3C6EF372u, 0xA54FF53Au,
    0x510E527Fu, 0x9B05688Cu, 0x1F83D9ABu, 0x5BE0CD19u
};

/* The message-word schedule: round r permutes which of the 16 message words
 * feed each G. This is what gives every message bit a different influence in
 * every round. */
static const u8 BLAKE2S_SIGMA[10][16] = {
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15 },
    {14,10, 4, 8, 9,15,13, 6, 1,12, 0, 2,11, 7, 5, 3 },
    {11, 8,12, 0, 5, 2,15,13,10,14, 3, 6, 7, 1, 9, 4 },
    { 7, 9, 3, 1,13,12,11,14, 2, 6, 5,10, 4, 0,15, 8 },
    { 9, 0, 5, 7, 2, 4,10,15,14, 1,11,12, 6, 8, 3,13 },
    { 2,12, 6,10, 0,11, 8, 3, 4,13, 7, 5,15,14, 1, 9 },
    {12, 5, 1,15,14,13, 4,10, 0, 7, 6, 3, 9, 2, 8,11 },
    {13,11, 7,14,12, 1, 3, 9, 5, 0,15, 4, 8, 6, 2,10 },
    { 6,15,14, 9,11, 3, 0, 8,12, 2,13, 7, 1, 4,10, 5 },
    {10, 2, 8, 4, 7, 6, 1, 5,15,11, 9,14, 3,12,13, 0 }
};

/* The G mixing function: two ARX "half rounds" that stir a 32-bit message word
 * into four state words. Rotations are BLAKE2s-specific (16/12/8/7). */
#define G(a, b, c, d, x, y)                     \
    do {                                        \
        a = a + b + (x); d = rotr32(d ^ a, 16); \
        c = c + d;       b = rotr32(b ^ c, 12); \
        a = a + b + (y); d = rotr32(d ^ a, 8);  \
        c = c + d;       b = rotr32(b ^ c, 7);  \
    } while (0)

static void blake2s_compress(struct blake2s *s, const u8 block[BLAKE2S_BLOCK_LEN])
{
    u32 m[16], v[16];
    for (int i = 0; i < 16; i++) m[i] = load_le32(block + 4 * i);   /* message  */

    for (int i = 0; i < 8; i++) v[i]     = s->h[i];     /* v[0..7]  = state     */
    for (int i = 0; i < 8; i++) v[8 + i] = BLAKE2S_IV[i]; /* v[8..15] = IV        */
    v[12] ^= s->t[0];   /* mix in the byte counter (low, high)                  */
    v[13] ^= s->t[1];
    v[14] ^= s->f[0];   /* and the "last block" flag (all-ones on the final one) */
    v[15] ^= s->f[1];

    for (int r = 0; r < 10; r++) {
        const u8 *sig = BLAKE2S_SIGMA[r];
        /* Four column mixes then four diagonal mixes — like ChaCha's structure. */
        G(v[0], v[4], v[ 8], v[12], m[sig[ 0]], m[sig[ 1]]);
        G(v[1], v[5], v[ 9], v[13], m[sig[ 2]], m[sig[ 3]]);
        G(v[2], v[6], v[10], v[14], m[sig[ 4]], m[sig[ 5]]);
        G(v[3], v[7], v[11], v[15], m[sig[ 6]], m[sig[ 7]]);
        G(v[0], v[5], v[10], v[15], m[sig[ 8]], m[sig[ 9]]);
        G(v[1], v[6], v[11], v[12], m[sig[10]], m[sig[11]]);
        G(v[2], v[7], v[ 8], v[13], m[sig[12]], m[sig[13]]);
        G(v[3], v[4], v[ 9], v[14], m[sig[14]], m[sig[15]]);
    }

    /* Feed-forward: new state = old state XOR both halves of the working vector. */
    for (int i = 0; i < 8; i++) s->h[i] ^= v[i] ^ v[8 + i];
}

void blake2s_init(struct blake2s *s, usize outlen)
{
    for (int i = 0; i < 8; i++) s->h[i] = BLAKE2S_IV[i];
    /* Fold the parameter block into h[0]: digest length in the low byte, key
     * length (0 for us) in the next, fanout=1 depth=1 in 0x01010000. Different
     * output lengths therefore give unrelated hashes of the same input. */
    s->h[0] ^= 0x01010000u ^ (u32)outlen;
    s->t[0] = s->t[1] = 0;
    s->f[0] = s->f[1] = 0;
    s->buflen = 0;
    s->outlen = outlen;
}

/* Advance the 64-bit byte counter by `inc`, carrying into the high word. */
static void blake2s_inc(struct blake2s *s, u32 inc)
{
    s->t[0] += inc;
    s->t[1] += (s->t[0] < inc);   /* carry: t[0] wrapped iff it is now < inc     */
}

void blake2s_update(struct blake2s *s, const void *data, usize len)
{
    const u8 *in = (const u8 *)data;
    if (len == 0) return;

    usize left = s->buflen;
    usize fill = BLAKE2S_BLOCK_LEN - left;
    /* We only ever compress a FULL buffer when we know more bytes follow, so the
     * genuinely-final block is always handled by blake2s_final (which sets f[0]).
     * That "is this the last block?" bookkeeping is intrinsic to BLAKE2. */
    if (len > fill) {
        s->buflen = 0;
        for (usize i = 0; i < fill; i++) s->buf[left + i] = in[i];   /* top up  */
        blake2s_inc(s, BLAKE2S_BLOCK_LEN);
        blake2s_compress(s, s->buf);
        in += fill;
        len -= fill;
        while (len > BLAKE2S_BLOCK_LEN) {           /* whole blocks, no copy     */
            blake2s_inc(s, BLAKE2S_BLOCK_LEN);
            blake2s_compress(s, in);
            in += BLAKE2S_BLOCK_LEN;
            len -= BLAKE2S_BLOCK_LEN;
        }
    }
    for (usize i = 0; i < len; i++) s->buf[s->buflen + i] = in[i];   /* buffer  */
    s->buflen += len;
}

void blake2s_final(struct blake2s *s, void *out, usize outlen)
{
    blake2s_inc(s, (u32)s->buflen);   /* count the final partial block's bytes   */
    s->f[0] = 0xFFFFFFFFu;            /* set the "last block" flag               */
    for (usize i = s->buflen; i < BLAKE2S_BLOCK_LEN; i++) s->buf[i] = 0; /* pad  */
    blake2s_compress(s, s->buf);

    u8 full[32];
    for (int i = 0; i < 8; i++) store_le32(full + 4 * i, s->h[i]);
    for (usize i = 0; i < outlen; i++) ((u8 *)out)[i] = full[i];
    secure_zero(full, sizeof full);
    secure_zero(s, sizeof *s);        /* the chaining value can reveal the input */
}

void blake2s256(u8 out[BLAKE2S_HASH_LEN], const void *data, usize len)
{
    struct blake2s s;
    blake2s_init(&s, BLAKE2S_HASH_LEN);
    blake2s_update(&s, data, len);
    blake2s_final(&s, out, BLAKE2S_HASH_LEN);
}

/* ---------------------------------------------------------------------------
 * HMAC-BLAKE2s(key, data): the standard HMAC construction
 *     HMAC(K, m) = HASH( (K ^ opad) ‖ HASH( (K ^ ipad) ‖ m ) )
 * with block size 64 (BLAKE2s's block). Keys longer than a block are first
 * hashed; ours are always 32 bytes so the long-key branch is rarely taken, but
 * we handle it for correctness.
 * --------------------------------------------------------------------------- */
void hmac_blake2s(u8 out[BLAKE2S_HASH_LEN],
                  const u8 *key, usize key_len,
                  const u8 *data, usize data_len)
{
    u8 k[BLAKE2S_BLOCK_LEN];
    for (int i = 0; i < BLAKE2S_BLOCK_LEN; i++) k[i] = 0;
    if (key_len > BLAKE2S_BLOCK_LEN)
        blake2s256(k, key, key_len);           /* K = HASH(K) if oversized       */
    else
        for (usize i = 0; i < key_len; i++) k[i] = key[i];

    u8 ipad[BLAKE2S_BLOCK_LEN], opad[BLAKE2S_BLOCK_LEN];
    for (int i = 0; i < BLAKE2S_BLOCK_LEN; i++) {
        ipad[i] = k[i] ^ 0x36;                 /* inner pad                      */
        opad[i] = k[i] ^ 0x5c;                 /* outer pad                      */
    }

    u8 inner[BLAKE2S_HASH_LEN];
    struct blake2s s;
    blake2s_init(&s, BLAKE2S_HASH_LEN);
    blake2s_update(&s, ipad, BLAKE2S_BLOCK_LEN);
    blake2s_update(&s, data, data_len);
    blake2s_final(&s, inner, BLAKE2S_HASH_LEN);

    blake2s_init(&s, BLAKE2S_HASH_LEN);
    blake2s_update(&s, opad, BLAKE2S_BLOCK_LEN);
    blake2s_update(&s, inner, BLAKE2S_HASH_LEN);
    blake2s_final(&s, out, BLAKE2S_HASH_LEN);

    secure_zero(k, sizeof k);
    secure_zero(ipad, sizeof ipad);
    secure_zero(opad, sizeof opad);
    secure_zero(inner, sizeof inner);
}

/* ---------------------------------------------------------------------------
 * HKDF (RFC 5869), the Noise KDF. "Extract" mixes the chaining key and the new
 * DH secret into a pseudorandom `temp` key; "Expand" then stretches it into up
 * to three independent 32-byte outputs by feeding a running counter byte:
 *     temp = HMAC(chaining_key, ikm)
 *     out1 = HMAC(temp, 0x01)
 *     out2 = HMAC(temp, out1 ‖ 0x02)
 *     out3 = HMAC(temp, out2 ‖ 0x03)
 * Each output is unpredictable without knowing the DH secret, which is the whole
 * point: an attacker who lacks even one of the four handshake DH results cannot
 * derive the session keys.
 * --------------------------------------------------------------------------- */
void hkdf(u8 *out1, u8 *out2, u8 *out3, int num,
          const u8 chaining_key[BLAKE2S_HASH_LEN],
          const u8 *ikm, usize ikm_len)
{
    u8 temp[BLAKE2S_HASH_LEN];
    hmac_blake2s(temp, chaining_key, BLAKE2S_HASH_LEN, ikm, ikm_len);  /* extract */

    /* Expand step 1: HMAC over the single byte 0x01. */
    u8 c = 1;
    hmac_blake2s(out1, temp, BLAKE2S_HASH_LEN, &c, 1);

    if (num >= 2) {
        /* Step 2: HMAC over (out1 ‖ 0x02). We build the input in a small buffer. */
        u8 in[BLAKE2S_HASH_LEN + 1];
        for (int i = 0; i < BLAKE2S_HASH_LEN; i++) in[i] = out1[i];
        in[BLAKE2S_HASH_LEN] = 2;
        hmac_blake2s(out2, temp, BLAKE2S_HASH_LEN, in, sizeof in);

        if (num >= 3) {
            for (int i = 0; i < BLAKE2S_HASH_LEN; i++) in[i] = out2[i];
            in[BLAKE2S_HASH_LEN] = 3;
            hmac_blake2s(out3, temp, BLAKE2S_HASH_LEN, in, sizeof in);
        }
        secure_zero(in, sizeof in);
    }
    secure_zero(temp, sizeof temp);
}
