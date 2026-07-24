/* ===========================================================================
 * sha1.c — SHA-1 (RFC 3174), from scratch, with ZERO libc dependency.
 * ===========================================================================
 *
 * This file includes only "sha1.h" and calls no library function — not even
 * memcpy (we copy bytes with explicit loops). That is what lets a bare
 * cross-compiler emit its assembly with no headers present, and it keeps the
 * round function's arithmetic completely visible in the generated code.
 *
 * THE ALGORITHM IN ONE PARAGRAPH
 * ------------------------------
 * SHA-1 keeps a 160-bit state (five 32-bit words h0..h4). It consumes the
 * message in 512-bit blocks. For each block it expands the 16 input words into
 * 80 words (the "message schedule" W[]), then runs 80 rounds of
 *
 *       T = ROTL(a,5) + f_t(b,c,d) + e + W[t] + K_t
 *       e = d;  d = c;  c = ROTL(b,30);  b = a;  a = T
 *
 * and finally folds a..e back into h0..h4 with 32-bit modular addition. All of
 * it is integer shifts, rotates, ANDs, XORs and adds — no memory tricks, no
 * tables beyond four constants — which is exactly why it makes such clean,
 * instructive assembly (see asm/demo.annotated.s).
 * ===========================================================================
 */
#include "sha1.h"

/* ---------------------------------------------------------------------------
 * ROTL32 — rotate a 32-bit word left by n bits.
 *
 * A rotate is NOT a shift: the bits that fall off the top re-enter at the
 * bottom. C has no rotate operator, so we synthesize it from two shifts ORed
 * together. `(x << n) | (x >> (32 - n))` is the idiom; because x is exactly 32
 * bits (sha1_u32), the high bits shifted out of `x << n` are dropped by the
 * type width and reappear via `x >> (32 - n)`. Compilers RECOGNIZE this pattern
 * and emit a single `rol` instruction — watch for it in the annotated asm.
 *
 * Caveat encoded here: n is always 1..31 in SHA-1, so (32 - n) is 1..31 and we
 * never hit the undefined-behavior shift-by-32. We never call ROTL32(x, 0). */
#define ROTL32(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

/* SHA-1's four round constants K_t, one per 20-round quarter (RFC 3174 §5).
 * They are the fractional parts of sqrt(2), sqrt(3), sqrt(5), sqrt(10) — chosen
 * to be "nothing-up-my-sleeve" numbers, i.e. obviously not backdoored. */
#define K0 0x5A827999u   /* rounds  0..19 */
#define K1 0x6ED9EBA1u   /* rounds 20..39 */
#define K2 0x8F1BBCDCu   /* rounds 40..59 */
#define K3 0xCA62C1D6u   /* rounds 60..79 */

/* ---------------------------------------------------------------------------
 * sha1_block — the compression function: fold ONE 64-byte block into h[0..4].
 *
 * This is "the SHA round" the project spec calls out as excellent assembly. It
 * is a pure function of (current state, 64 input bytes) -> new state. The exact
 * same routine is reproduced, standalone, in asm/demo.c and annotated there.
 * --------------------------------------------------------------------------- */
static void sha1_block(sha1_u32 h[5], const unsigned char block[64])
{
    sha1_u32 w[80];   /* the message schedule: 16 loaded, 64 derived           */
    int t;

    /* --- Load the 16 big-endian input words -------------------------------
     * SHA-1 is defined big-endian: byte 0 is the most significant byte of
     * word 0. x86-64 is little-endian, so we assemble each word by hand from
     * four bytes rather than reinterpreting memory — that keeps the code
     * endian-correct on any host and makes the byte order explicit. */
    for (t = 0; t < 16; t++) {
        w[t] = ((sha1_u32)block[t * 4 + 0] << 24)
             | ((sha1_u32)block[t * 4 + 1] << 16)
             | ((sha1_u32)block[t * 4 + 2] <<  8)
             | ((sha1_u32)block[t * 4 + 3] <<  0);
    }

    /* --- Extend to 80 words -----------------------------------------------
     * Each later word mixes four earlier ones and rotates left by 1. The
     * rotate (added in FIPS 180-1, the fix that turned "SHA-0" into "SHA-1")
     * is what breaks up linear structure in the schedule. */
    for (t = 16; t < 80; t++) {
        sha1_u32 x = w[t - 3] ^ w[t - 8] ^ w[t - 14] ^ w[t - 16];
        w[t] = ROTL32(x, 1);
    }

    /* --- Initialize the working variables from the running state ---------- */
    sha1_u32 a = h[0], b = h[1], c = h[2], d = h[3], e = h[4];

    /* --- 80 rounds --------------------------------------------------------
     * The only thing that changes across the four quarters is the mixing
     * function f and the constant K. We spell the quarters out separately
     * rather than branching inside the loop, so each round is straight-line
     * integer arithmetic (and the compiler can schedule it freely).
     *
     *   Ch(b,c,d)     = (b & c) | (~b & d)          "choose"     rounds  0-19
     *   Parity(b,c,d) = b ^ c ^ d                   "xor"        rounds 20-39
     *   Maj(b,c,d)    = (b&c) | (b&d) | (c&d)       "majority"   rounds 40-59
     *   Parity again                                             rounds 60-79
     */
    for (t = 0; t < 20; t++) {
        sha1_u32 f = (b & c) | (~b & d);
        sha1_u32 T = ROTL32(a, 5) + f + e + w[t] + K0;
        e = d; d = c; c = ROTL32(b, 30); b = a; a = T;
    }
    for (t = 20; t < 40; t++) {
        sha1_u32 f = b ^ c ^ d;
        sha1_u32 T = ROTL32(a, 5) + f + e + w[t] + K1;
        e = d; d = c; c = ROTL32(b, 30); b = a; a = T;
    }
    for (t = 40; t < 60; t++) {
        sha1_u32 f = (b & c) | (b & d) | (c & d);
        sha1_u32 T = ROTL32(a, 5) + f + e + w[t] + K2;
        e = d; d = c; c = ROTL32(b, 30); b = a; a = T;
    }
    for (t = 60; t < 80; t++) {
        sha1_u32 f = b ^ c ^ d;
        sha1_u32 T = ROTL32(a, 5) + f + e + w[t] + K3;
        e = d; d = c; c = ROTL32(b, 30); b = a; a = T;
    }

    /* --- Fold the block's result back into the running state --------------
     * The `+=` here is 32-bit modular addition (sha1_u32 wraps). This feedback
     * is what makes SHA-1 a one-way "compression" of arbitrary length into 160
     * bits, and why you cannot run it backwards. */
    h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e;
}

/* ---------------------------------------------------------------------------
 * sha1_init — load the standard initial state (RFC 3174 §6.1). These five magic
 * words are the published starting constants; every SHA-1 in the world begins
 * here so that identical input yields identical output everywhere. */
void sha1_init(sha1_ctx *c)
{
    c->h[0] = 0x67452301u;
    c->h[1] = 0xEFCDAB89u;
    c->h[2] = 0x98BADCFEu;
    c->h[3] = 0x10325476u;
    c->h[4] = 0xC3D2E1F0u;
    c->nbits = 0;
    c->blen  = 0;
}

/* ---------------------------------------------------------------------------
 * sha1_update — feed `len` more bytes into the hash, buffering across calls.
 *
 * We copy incoming bytes into the 64-byte block buffer; every time it fills we
 * run sha1_block and reset. The running bit-length is bumped up front so
 * sha1_final can emit it. No libc: the copy is an explicit byte loop, which is
 * also the honest thing to show in the assembly. */
void sha1_update(sha1_ctx *c, const void *data, sha1_size len)
{
    const unsigned char *p = (const unsigned char *)data;

    /* Track the TOTAL message length in bits — SHA-1's final block encodes it,
     * and that length is what makes "abc" and "abc\0" hash differently. */
    c->nbits += (sha1_u64)len << 3;

    while (len > 0) {
        /* How many bytes still fit in the current partial block? */
        unsigned space = 64u - c->blen;
        unsigned take  = (space < len) ? space : (unsigned)len;

        for (unsigned i = 0; i < take; i++)
            c->block[c->blen + i] = p[i];

        c->blen += take;
        p       += take;
        len     -= take;

        if (c->blen == 64) {          /* a full 512-bit block is ready         */
            sha1_block(c->h, c->block);
            c->blen = 0;
        }
    }
}

/* ---------------------------------------------------------------------------
 * sha1_final — append the SHA-1 padding and emit the 20-byte digest.
 *
 * The padding rule (RFC 3174 §4) is exact and worth understanding: append a
 * single 1 bit (the byte 0x80), then 0 bits until the length is 56 mod 64, then
 * the original message length as a 64-bit BIG-ENDIAN integer. That fills the
 * final block to a 64-byte boundary and makes the total length self-describing,
 * so no two messages of different lengths can share a padded form. */
void sha1_final(sha1_ctx *c, unsigned char out[20])
{
    /* Snapshot the true message length NOW, before the padding bytes we are
     * about to feed in bump c->nbits (those bumps are harmless — we ignore the
     * counter afterward — but the length we EMIT must be the pre-padding one). */
    sha1_u64 nbits = c->nbits;

    unsigned char one = 0x80;         /* the mandatory 1 bit, byte-aligned     */
    sha1_update(c, &one, 1);

    /* Zero-pad until exactly 8 bytes remain in the block for the length field.
     * Feeding one zero byte at a time keeps this trivial and reuses the block-
     * flushing logic in sha1_update (it wraps correctly across a block edge). */
    unsigned char zero = 0x00;
    while (c->blen != 56)
        sha1_update(c, &zero, 1);

    /* The 64-bit length, big-endian: most significant byte first. */
    unsigned char lenbuf[8];
    for (int i = 0; i < 8; i++)
        lenbuf[i] = (unsigned char)(nbits >> (56 - 8 * i));
    sha1_update(c, lenbuf, 8);        /* this completes and flushes the block  */

    /* Serialize h[0..4] big-endian into the 20 output bytes. This byte order is
     * the canonical digest and is exactly what git writes into tree entries. */
    for (int i = 0; i < 5; i++) {
        out[i * 4 + 0] = (unsigned char)(c->h[i] >> 24);
        out[i * 4 + 1] = (unsigned char)(c->h[i] >> 16);
        out[i * 4 + 2] = (unsigned char)(c->h[i] >>  8);
        out[i * 4 + 3] = (unsigned char)(c->h[i] >>  0);
    }
}
