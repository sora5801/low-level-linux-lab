/* ===========================================================================
 * asm/demo.c — SELF-CONTAINED extraction of mini-git's two hottest pure-logic
 *              routines: the SHA-1 compression round, and hex encoding.
 * ===========================================================================
 *
 * This file has NO #includes and declares its own types, so a bare cross-
 * compiler turns it into x86-64 System V assembly with no libc or kernel headers
 * present. Both routines here are exactly the ones the real program runs
 * (sha1.c / util.c), reproduced in isolation so every instruction they compile
 * to can be annotated (see demo.annotated.s).
 *
 * WHY THESE TWO ROUTINES
 * ----------------------
 * They are the endpoints of git's identity pipeline:
 *
 *     file bytes --[sha1_block x N]--> 20 raw id bytes --[hex_encode]--> 40 chars
 *
 * sha1_block is 100% integer arithmetic — shifts, rotates, ANDs, XORs, adds —
 * which makes uncommonly clean assembly and shows the compiler turning the C
 * rotate idiom `(x<<n)|(x>>(32-n))` into a single `rol`. hex_encode is a tight
 * branch-free table lookup: each byte becomes two `movzbl`+indexed-load pairs.
 * Watching both is the point of the exercise.
 * ===========================================================================
 */

/* --- our own fixed-width types (no <stdint.h>) ----------------------------
 * On every Linux x86-64 target `unsigned int` is exactly 32 bits (SHA-1 needs
 * the mod-2^32 wraparound) and `unsigned long long` is 64. */
typedef unsigned int       u32;
typedef unsigned char      u8;

/* Rotate-left: C has no rotate operator, so we OR two shifts. Because u32 is 32
 * bits, the bits shifted out the top of (x<<n) vanish and reappear via
 * (x>>(32-n)). clang recognizes the pattern and emits `rol`. n is always 1..31
 * here, so (32-n) is never an undefined shift-by-32. */
#define ROTL32(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

/* SHA-1's four "nothing-up-my-sleeve" round constants (RFC 3174 §5). */
#define K0 0x5A827999u
#define K1 0x6ED9EBA1u
#define K2 0x8F1BBCDCu
#define K3 0xCA62C1D6u

/* ---------------------------------------------------------------------------
 * sha1_block — fold ONE 64-byte block into the five-word state h[0..4].
 *
 * A pure function of (state, 64 input bytes) -> new state. This is the SHA round
 * the project spec highlights as excellent assembly. Identical to sha1.c's copy.
 * --------------------------------------------------------------------------- */
void sha1_block(u32 h[5], const u8 block[64])
{
    u32 w[80];
    int t;

    /* Load 16 big-endian words (SHA-1 is defined MSB-first; we assemble each
     * word from four bytes so the code is endian-correct on any host). */
    for (t = 0; t < 16; t++)
        w[t] = ((u32)block[t*4+0] << 24) | ((u32)block[t*4+1] << 16)
             | ((u32)block[t*4+2] <<  8) | ((u32)block[t*4+3] <<  0);

    /* Expand to 80 words: each mixes four earlier words and rotates left 1. */
    for (t = 16; t < 80; t++)
        w[t] = ROTL32(w[t-3] ^ w[t-8] ^ w[t-14] ^ w[t-16], 1);

    u32 a = h[0], b = h[1], c = h[2], d = h[3], e = h[4];

    /* 80 rounds in four quarters; only f and K change between them. */
    for (t = 0; t < 20; t++) {
        u32 f = (b & c) | (~b & d);                 /* choose                    */
        u32 T = ROTL32(a,5) + f + e + w[t] + K0;
        e=d; d=c; c=ROTL32(b,30); b=a; a=T;
    }
    for (t = 20; t < 40; t++) {
        u32 f = b ^ c ^ d;                          /* parity                    */
        u32 T = ROTL32(a,5) + f + e + w[t] + K1;
        e=d; d=c; c=ROTL32(b,30); b=a; a=T;
    }
    for (t = 40; t < 60; t++) {
        u32 f = (b & c) | (b & d) | (c & d);        /* majority                  */
        u32 T = ROTL32(a,5) + f + e + w[t] + K2;
        e=d; d=c; c=ROTL32(b,30); b=a; a=T;
    }
    for (t = 60; t < 80; t++) {
        u32 f = b ^ c ^ d;                          /* parity                    */
        u32 T = ROTL32(a,5) + f + e + w[t] + K3;
        e=d; d=c; c=ROTL32(b,30); b=a; a=T;
    }

    /* Feed the block's result back with 32-bit modular addition. */
    h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d; h[4]+=e;
}

/* ---------------------------------------------------------------------------
 * hex_encode — 20 raw id bytes -> 40 lowercase hex chars (+ NUL).
 *
 * Each byte splits into two 4-bit nibbles indexing a 16-char table. It is
 * branch-free: no conditionals, just shifts, masks, and indexed loads — which is
 * why it vectorizes/pipelines so well and reads cleanly in asm.
 * --------------------------------------------------------------------------- */
void hex_encode(const u8 *raw, int n, char *out)
{
    static const char d[16] = { '0','1','2','3','4','5','6','7',
                                '8','9','a','b','c','d','e','f' };
    for (int i = 0; i < n; i++) {
        out[2*i]   = d[raw[i] >> 4];       /* high nibble (bits 7..4)           */
        out[2*i+1] = d[raw[i] & 0x0f];     /* low  nibble (bits 3..0)           */
    }
    out[2*n] = '\0';
}

/* ---------------------------------------------------------------------------
 * demo_run — a complete, self-contained exercise so the unit links and nothing
 * is optimized away when compiled alone.
 *
 * It hand-builds the SINGLE padded SHA-1 block for the message "abc" (the
 * canonical test vector) and runs sha1_block once from the standard init state.
 * The result must be the well-known digest a9993e364706816aba3e25717850c26c9cd0
 * d89d. We hex-encode it and return the first character ('a' == 0x61 == 97), a
 * value you can check by eye. No libc, no I/O.
 * --------------------------------------------------------------------------- */
int demo_run(void)
{
    /* The padded 512-bit block for "abc": the 3 message bytes, then 0x80, then
     * zeros, then the 64-bit big-endian bit length (24 == 0x18) in the last 8
     * bytes. Building it by hand keeps the demo free of the padding machinery. */
    u8 block[64];
    for (int i = 0; i < 64; i++) block[i] = 0;
    block[0] = 'a'; block[1] = 'b'; block[2] = 'c';
    block[3] = 0x80;                     /* the mandatory trailing 1 bit         */
    block[63] = 24;                      /* message length in BITS (3 bytes * 8) */

    /* Standard SHA-1 initial state (RFC 3174 §6.1). */
    u32 h[5] = { 0x67452301u, 0xEFCDAB89u, 0x98BADCFEu, 0x10325476u, 0xC3D2E1F0u };
    sha1_block(h, block);

    /* Serialize the state big-endian into 20 raw bytes, then hex-encode. */
    u8 raw[20];
    for (int i = 0; i < 5; i++) {
        raw[i*4+0] = (u8)(h[i] >> 24);
        raw[i*4+1] = (u8)(h[i] >> 16);
        raw[i*4+2] = (u8)(h[i] >>  8);
        raw[i*4+3] = (u8)(h[i] >>  0);
    }
    char hex[41];
    hex_encode(raw, 20, hex);
    return (int)(unsigned char)hex[0];   /* expect 'a' (97) for "abc"            */
}
