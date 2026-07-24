/* ===========================================================================
 * chacha20poly1305.c — ChaCha20 stream cipher + Poly1305 MAC + the AEAD that
 *                      combines them, exactly per RFC 8439.
 * ===========================================================================
 *
 * Three layers, bottom-up:
 *   1. ChaCha20  — a stream cipher. It turns (key, counter, nonce) into a
 *      pseudo-random 64-byte keystream block; XOR keystream with plaintext to
 *      encrypt. It is built from ONE operation, the "quarter round", repeated:
 *      pure 32-bit Add / Rotate / Xor (an "ARX" cipher). No S-boxes, no tables,
 *      so it is naturally constant-time. (asm/demo.c extracts exactly this.)
 *   2. Poly1305 — a one-time authenticator. Given a one-time 32-byte key it maps
 *      a message to a 16-byte tag by evaluating a polynomial modulo the prime
 *      2^130 - 5. "One-time" is literal: the key must be fresh per message, which
 *      is why the AEAD derives it from the cipher itself (ChaCha block 0).
 *   3. AEAD      — glue: derive the Poly1305 key from ChaCha block 0, encrypt the
 *      plaintext with blocks 1.., then authenticate AAD+ciphertext+lengths.
 *
 * The Poly1305 field arithmetic here is the well-known public-domain
 * "poly1305-donna" 32-bit reference (5 limbs of 26 bits, 64-bit products),
 * transcribed with commentary. The X25519 code (x25519.c) is likewise a
 * transcription of TweetNaCl. Reusing vetted reference math is deliberate: hand-
 * rolling bignum reductions is exactly where crypto code goes subtly wrong.
 * =========================================================================== */

#include "chacha20poly1305.h"

/* ===========================================================================
 * 1. ChaCha20 (RFC 8439 §2.3–2.4)
 * =========================================================================== */

/* The quarter round: the entire cryptographic core of ChaCha. It mixes four
 * 32-bit state words with Add-Rotate-Xor so that flipping any input bit
 * avalanches across all four outputs. The rotate distances 16/12/8/7 are the
 * design's fixed constants. This is the routine asm/demo.c isolates. */
#define QR(a, b, c, d)                       \
    do {                                     \
        a += b;  d ^= a;  d = rotl32(d, 16); \
        c += d;  b ^= c;  b = rotl32(b, 12); \
        a += b;  d ^= a;  d = rotl32(d, 8);  \
        c += d;  b ^= c;  b = rotl32(b, 7);  \
    } while (0)

void chacha20_block(u8 out[64], const u8 key[AEAD_KEY_LEN],
                    u32 counter, const u8 nonce[AEAD_NONCE_LEN])
{
    /* The 4x4 matrix of 32-bit words that IS the ChaCha state:
     *   row 0: four constants "expand 32-byte k" (defends against all-zero keys)
     *   rows 1-2: the 256-bit key, as 8 little-endian words
     *   row 3: the 32-bit block counter, then the 96-bit nonce (3 words)
     * The constants are the ASCII of "expa" "nd 3" "2-by" "te k". */
    u32 s[16];
    s[0] = 0x61707865u; s[1] = 0x3320646eu;   /* "expa" "nd 3"                 */
    s[2] = 0x79622d32u; s[3] = 0x6b206574u;   /* "2-by" "te k"                 */
    for (int i = 0; i < 8; i++)               /* key words 4..11               */
        s[4 + i] = load_le32(key + 4 * i);
    s[12] = counter;                          /* block counter                 */
    s[13] = load_le32(nonce + 0);             /* nonce words 13..15            */
    s[14] = load_le32(nonce + 4);
    s[15] = load_le32(nonce + 8);

    /* Working copy: we permute `x` for 20 rounds, then ADD the original `s`
     * back in. That feed-forward add is what makes the permutation one-way. */
    u32 x[16];
    for (int i = 0; i < 16; i++) x[i] = s[i];

    /* 20 rounds = 10 "double rounds". Each double round is 4 quarter rounds on
     * the COLUMNS followed by 4 on the DIAGONALS, so every word is touched
     * twice per double round. */
    for (int i = 0; i < 10; i++) {
        QR(x[0], x[4], x[8],  x[12]);   /* column 0 */
        QR(x[1], x[5], x[9],  x[13]);   /* column 1 */
        QR(x[2], x[6], x[10], x[14]);   /* column 2 */
        QR(x[3], x[7], x[11], x[15]);   /* column 3 */
        QR(x[0], x[5], x[10], x[15]);   /* diagonal 0 */
        QR(x[1], x[6], x[11], x[12]);   /* diagonal 1 */
        QR(x[2], x[7], x[8],  x[13]);   /* diagonal 2 */
        QR(x[3], x[4], x[9],  x[14]);   /* diagonal 3 */
    }

    /* out = (x + s) serialised little-endian. The wrap-around add mod 2^32 is
     * intentional; it is the feed-forward that prevents inverting the rounds. */
    for (int i = 0; i < 16; i++)
        store_le32(out + 4 * i, x[i] + s[i]);
}

/* Encrypt/decrypt `len` bytes by XORing with the ChaCha keystream, starting at
 * block `counter`. Symmetric: the same function decrypts (XOR is its own
 * inverse). `in` and `out` may alias. */
static void chacha20_xor(u8 *out, const u8 *in, usize len,
                         const u8 key[AEAD_KEY_LEN], u32 counter,
                         const u8 nonce[AEAD_NONCE_LEN])
{
    u8 block[64];
    usize done = 0;
    while (done < len) {
        chacha20_block(block, key, counter, nonce);
        usize n = len - done;
        if (n > 64) n = 64;                 /* last block may be partial       */
        for (usize i = 0; i < n; i++)
            out[done + i] = in[done + i] ^ block[i];
        done += n;
        counter++;                          /* next 64-byte keystream block    */
    }
    secure_zero(block, sizeof block);       /* keystream is as sensitive as key */
}

/* ===========================================================================
 * 2. Poly1305 (RFC 8439 §2.5) — poly1305-donna, 32-bit reference.
 *
 * The accumulator h and the key part r are held as 5 limbs of 26 bits (radix
 * 2^26), so a limb*limb product fits in 64 bits and five of them sum without
 * overflow. Reduction modulo 2^130-5 is cheap: a bit at position 130 folds back
 * to position 0 with weight 5 (because 2^130 ≡ 5 (mod 2^130-5)).
 * =========================================================================== */

struct poly1305 {
    u32   r[5];        /* clamped key multiplier, 5x26-bit limbs               */
    u32   h[5];        /* accumulator                                          */
    u32   pad[4];      /* the "s" half of the key, added at the very end       */
    u8    buffer[16];  /* holds a partial (<16 byte) final block               */
    usize leftover;    /* bytes currently in `buffer`                          */
    int   final;       /* set once the padding bit must be suppressed          */
};

static void poly1305_init(struct poly1305 *st, const u8 key[32])
{
    /* Clamp r: mask off the bits the spec forbids (r &= 0x0ffffffc0ffffffc...).
     * Clamping guarantees the products stay in range for the 26-bit limb code
     * and removes weak keys. Each limb pulls 26 bits out of the little-endian
     * key, shifted so the limbs tile the 128-bit value. */
    st->r[0] = (load_le32(key +  0)     ) & 0x03ffffffu;
    st->r[1] = (load_le32(key +  3) >> 2) & 0x03ffff03u;
    st->r[2] = (load_le32(key +  6) >> 4) & 0x03ffc0ffu;
    st->r[3] = (load_le32(key +  9) >> 6) & 0x03f03fffu;
    st->r[4] = (load_le32(key + 12) >> 8) & 0x000fffffu;

    for (int i = 0; i < 5; i++) st->h[i] = 0;         /* accumulator starts 0  */

    st->pad[0] = load_le32(key + 16);   /* the second 16 bytes of the one-time */
    st->pad[1] = load_le32(key + 20);   /* key are added to the final result;  */
    st->pad[2] = load_le32(key + 24);   /* this is the "s" in tag = (poly + s). */
    st->pad[3] = load_le32(key + 28);

    st->leftover = 0;
    st->final = 0;
}

/* Absorb whole 16-byte blocks: h = (h + block) * r  (mod 2^130-5). */
static void poly1305_blocks(struct poly1305 *st, const u8 *m, usize bytes)
{
    /* hibit adds the 2^128 bit that every full block carries per the spec; for
     * the final (possibly short) block the caller sets st->final and we drop it
     * because the padding byte 0x01 already supplied that high bit. */
    const u32 hibit = st->final ? 0 : (1u << 24);
    u32 r0 = st->r[0], r1 = st->r[1], r2 = st->r[2], r3 = st->r[3], r4 = st->r[4];
    /* s_i = 5 * r_i, precomputed: these are the "wrapped" limbs used when a
     * product lands above limb 4 and must fold back with weight 5. */
    u32 s1 = r1 * 5, s2 = r2 * 5, s3 = r3 * 5, s4 = r4 * 5;
    u32 h0 = st->h[0], h1 = st->h[1], h2 = st->h[2], h3 = st->h[3], h4 = st->h[4];

    while (bytes >= 16) {
        /* h += m: unpack this block into 26-bit limbs and add to the accumulator. */
        h0 += (load_le32(m +  0)     ) & 0x03ffffffu;
        h1 += (load_le32(m +  3) >> 2) & 0x03ffffffu;
        h2 += (load_le32(m +  6) >> 4) & 0x03ffffffu;
        h3 += (load_le32(m +  9) >> 6) & 0x03ffffffu;
        h4 += (load_le32(m + 12) >> 8) | hibit;

        /* h *= r  — schoolbook multiply of two 5-limb numbers. Terms above limb
         * 4 (which carry weight 2^130 and higher) fold down multiplied by 5 via
         * the s_i, implementing the (mod 2^130-5) reduction inside the multiply. */
        u64 d0 = (u64)h0*r0 + (u64)h1*s4 + (u64)h2*s3 + (u64)h3*s2 + (u64)h4*s1;
        u64 d1 = (u64)h0*r1 + (u64)h1*r0 + (u64)h2*s4 + (u64)h3*s3 + (u64)h4*s2;
        u64 d2 = (u64)h0*r2 + (u64)h1*r1 + (u64)h2*r0 + (u64)h3*s4 + (u64)h4*s3;
        u64 d3 = (u64)h0*r3 + (u64)h1*r2 + (u64)h2*r1 + (u64)h3*r0 + (u64)h4*s4;
        u64 d4 = (u64)h0*r4 + (u64)h1*r3 + (u64)h2*r2 + (u64)h3*r1 + (u64)h4*r0;

        /* Carry-propagate the five 64-bit partial sums back into 26-bit limbs. */
        u32 c;
        c = (u32)(d0 >> 26); h0 = (u32)d0 & 0x03ffffffu;
        d1 += c; c = (u32)(d1 >> 26); h1 = (u32)d1 & 0x03ffffffu;
        d2 += c; c = (u32)(d2 >> 26); h2 = (u32)d2 & 0x03ffffffu;
        d3 += c; c = (u32)(d3 >> 26); h3 = (u32)d3 & 0x03ffffffu;
        d4 += c; c = (u32)(d4 >> 26); h4 = (u32)d4 & 0x03ffffffu;
        h0 += c * 5;                          /* fold the top carry back (x5)   */
        c = h0 >> 26; h0 &= 0x03ffffffu;
        h1 += c;

        m += 16;
        bytes -= 16;
    }

    st->h[0] = h0; st->h[1] = h1; st->h[2] = h2; st->h[3] = h3; st->h[4] = h4;
}

/* Feed arbitrary-length data, buffering partial trailing bytes across calls so
 * the AEAD can stream AAD, padding, ciphertext, padding, and the length block. */
static void poly1305_update(struct poly1305 *st, const u8 *m, usize bytes)
{
    /* Drain any buffered leftover first, completing it to a full 16-byte block. */
    if (st->leftover) {
        usize want = 16 - st->leftover;
        if (want > bytes) want = bytes;
        for (usize i = 0; i < want; i++) st->buffer[st->leftover + i] = m[i];
        bytes -= want;
        m += want;
        st->leftover += want;
        if (st->leftover < 16) return;        /* still not a full block          */
        poly1305_blocks(st, st->buffer, 16);
        st->leftover = 0;
    }
    /* Process as many whole blocks as we can directly from the input. */
    if (bytes >= 16) {
        usize want = bytes & ~(usize)15;      /* round down to a multiple of 16  */
        poly1305_blocks(st, m, want);
        m += want;
        bytes -= want;
    }
    /* Stash the remaining <16 bytes for next time (or for finish()). */
    for (usize i = 0; i < bytes; i++) st->buffer[i] = m[i];
    st->leftover = bytes;
}

static void poly1305_finish(struct poly1305 *st, u8 mac[16])
{
    /* Pad and absorb the final partial block: append 0x01 (the high bit that a
     * full block would have carried), zero-fill, and process with final=1. */
    if (st->leftover) {
        usize i = st->leftover;
        st->buffer[i++] = 1;
        for (; i < 16; i++) st->buffer[i] = 0;
        st->final = 1;
        poly1305_blocks(st, st->buffer, 16);
    }

    u32 h0 = st->h[0], h1 = st->h[1], h2 = st->h[2], h3 = st->h[3], h4 = st->h[4];
    u32 c;
    /* Fully carry h so each limb is a clean 26 bits. */
    c = h1 >> 26; h1 &= 0x03ffffffu;
    h2 += c; c = h2 >> 26; h2 &= 0x03ffffffu;
    h3 += c; c = h3 >> 26; h3 &= 0x03ffffffu;
    h4 += c; c = h4 >> 26; h4 &= 0x03ffffffu;
    h0 += c * 5; c = h0 >> 26; h0 &= 0x03ffffffu;
    h1 += c;

    /* Compute h + -p = h - (2^130-5) = h + 5 - 2^130 as g, so we can select
     * min(h, h-p) in constant time (h is fully reduced only after this). */
    u32 g0 = h0 + 5; c = g0 >> 26; g0 &= 0x03ffffffu;
    u32 g1 = h1 + c; c = g1 >> 26; g1 &= 0x03ffffffu;
    u32 g2 = h2 + c; c = g2 >> 26; g2 &= 0x03ffffffu;
    u32 g3 = h3 + c; c = g3 >> 26; g3 &= 0x03ffffffu;
    u32 g4 = h4 + c - (1u << 26);      /* borrow lands in bit 31 iff h < p      */

    /* Branchless select: if the subtraction borrowed (h < p) keep h, else use g.
     * mask = 0 when bit31 set (h<p), else all-ones. */
    u32 mask = (g4 >> 31) - 1;
    g0 &= mask; g1 &= mask; g2 &= mask; g3 &= mask; g4 &= mask;
    mask = ~mask;
    h0 = (h0 & mask) | g0; h1 = (h1 & mask) | g1; h2 = (h2 & mask) | g2;
    h3 = (h3 & mask) | g3; h4 = (h4 & mask) | g4;

    /* Collapse the 26-bit limbs back into four 32-bit words (mod 2^128). */
    h0 = (h0      ) | (h1 << 26);
    h1 = (h1 >>  6) | (h2 << 20);
    h2 = (h2 >> 12) | (h3 << 14);
    h3 = (h3 >> 18) | (h4 <<  8);

    /* tag = (h + s) mod 2^128, propagating the add carry across the words. */
    u64 f;
    f = (u64)h0 + st->pad[0];              h0 = (u32)f;
    f = (u64)h1 + st->pad[1] + (f >> 32);  h1 = (u32)f;
    f = (u64)h2 + st->pad[2] + (f >> 32);  h2 = (u32)f;
    f = (u64)h3 + st->pad[3] + (f >> 32);  h3 = (u32)f;

    store_le32(mac +  0, h0);
    store_le32(mac +  4, h1);
    store_le32(mac +  8, h2);
    store_le32(mac + 12, h3);
}

/* ===========================================================================
 * 3. The AEAD (RFC 8439 §2.8): tie ChaCha20 and Poly1305 together.
 * =========================================================================== */

/* Poly1305 must authenticate 16-byte-aligned regions; pad16 feeds the zero bytes
 * needed to round the running length up to the next multiple of 16. */
static void poly1305_pad16(struct poly1305 *st, usize len)
{
    static const u8 zero[16] = {0};
    usize rem = len & 15;
    if (rem) poly1305_update(st, zero, 16 - rem);
}

/* Compute the AEAD tag over: AAD ‖ pad ‖ ciphertext ‖ pad ‖ le64(aad_len) ‖
 * le64(ct_len). Binding the two lengths at the end stops an attacker from
 * shifting bytes between the AAD and the ciphertext. */
static void aead_tag(u8 tag[AEAD_TAG_LEN], const u8 key[AEAD_KEY_LEN],
                     const u8 nonce[AEAD_NONCE_LEN],
                     const u8 *ad, usize ad_len,
                     const u8 *ct, usize ct_len)
{
    /* One-time Poly1305 key = first 32 bytes of ChaCha20 keystream block 0.
     * Deriving it from the same key+nonce guarantees it is fresh per message
     * (the nonce is unique), which is exactly what "one-time" MAC requires. */
    u8 block0[64];
    chacha20_block(block0, key, 0, nonce);

    struct poly1305 st;
    poly1305_init(&st, block0);       /* uses block0[0..31] as (r ‖ s)          */

    if (ad_len) { poly1305_update(&st, ad, ad_len); poly1305_pad16(&st, ad_len); }
    poly1305_update(&st, ct, ct_len);  poly1305_pad16(&st, ct_len);

    u8 lengths[16];
    store_le64(lengths + 0, (u64)ad_len);
    store_le64(lengths + 8, (u64)ct_len);
    poly1305_update(&st, lengths, 16);

    poly1305_finish(&st, tag);

    secure_zero(block0, sizeof block0);   /* wipe the derived one-time key       */
    secure_zero(&st, sizeof st);
}

void aead_seal(u8 *out, const u8 key[AEAD_KEY_LEN],
               const u8 nonce[AEAD_NONCE_LEN],
               const u8 *ad, usize ad_len,
               const u8 *plaintext, usize plaintext_len)
{
    /* Encrypt with keystream starting at block 1 (block 0 was spent on the MAC
     * key). Then append the tag over the resulting ciphertext. Order matters:
     * we authenticate the CIPHERTEXT (encrypt-then-MAC), the only order that is
     * generically secure. */
    chacha20_xor(out, plaintext, plaintext_len, key, 1, nonce);
    aead_tag(out + plaintext_len, key, nonce, ad, ad_len, out, plaintext_len);
}

/* Constant-time 16-byte compare: returns 0 iff equal. We OR together the XOR of
 * every byte pair; the result is zero only if all bytes matched. Crucially the
 * running time does NOT depend on WHERE the first mismatch is, so an attacker
 * cannot time-probe the tag byte by byte to forge it. */
static int ct_equal16(const u8 *a, const u8 *b)
{
    u8 diff = 0;
    for (int i = 0; i < AEAD_TAG_LEN; i++) diff |= a[i] ^ b[i];
    return diff;    /* 0 == equal */
}

int aead_open(u8 *out, const u8 key[AEAD_KEY_LEN],
              const u8 nonce[AEAD_NONCE_LEN],
              const u8 *ad, usize ad_len,
              const u8 *ciphertext, usize ciphertext_len)
{
    if (ciphertext_len < AEAD_TAG_LEN) return -1;    /* too short to hold a tag */
    usize ct_len = ciphertext_len - AEAD_TAG_LEN;
    const u8 *tag = ciphertext + ct_len;

    /* Recompute the tag over the received ciphertext and compare BEFORE trusting
     * anything. This is the authentication half of the AEAD. */
    u8 expected[AEAD_TAG_LEN];
    aead_tag(expected, key, nonce, ad, ad_len, ciphertext, ct_len);
    int bad = ct_equal16(expected, tag);
    secure_zero(expected, sizeof expected);
    if (bad) return -1;                              /* forgery / corruption    */

    /* Tag verified: safe to decrypt (same XOR, starting at block 1). */
    chacha20_xor(out, ciphertext, ct_len, key, 1, nonce);
    return 0;
}
