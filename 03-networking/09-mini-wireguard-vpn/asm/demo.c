/* ===========================================================================
 * asm/demo.c — the ChaCha20 quarter round + block, extracted and self-contained.
 * ===========================================================================
 *
 * This is the pure-logic HEART of the tunnel's data path: ChaCha20, the stream
 * cipher that turns (key, counter, nonce) into keystream. Everything encrypted —
 * every handshake field, every packet — is XORed with the output of this code.
 *
 * ChaCha is an "ARX" cipher: its ONLY operations are 32-bit Add, Rotate, and
 * Xor. There are no S-boxes and no lookup tables, which is *why* it is naturally
 * constant-time (no data-dependent memory addresses or branches) — a property
 * you can literally confirm in the generated assembly: you will see nothing but
 * add / xor / rol on registers. That is the lesson of reading this .s.
 *
 * It is deliberately freestanding — NO system headers, its own integer types —
 * so `clang -S` produces clean assembly containing only these routines. See
 * asm/demo.annotated.s for the line-by-line SysV AMD64 ABI walkthrough.
 *
 * TWO functions are exported so the assembly shows both scales:
 *   chacha_quarter_round : the ARX core in isolation (tiny, fully annotated).
 *   chacha20_block       : the full 20-round block (shows the optimizer unroll
 *                          the ARX and, at -O2, vectorize it).
 * =========================================================================== */

/* Freestanding fixed-width types (every LP64/ILP32-int target clang supports has
 * these widths). No <stdint.h> so the translation unit needs nothing external. */
typedef unsigned char  u8;
typedef unsigned int   u32;

/* 32-bit left rotate — the "R" in ARX. Written as (v<<c)|(v>>(32-c)); the
 * compiler pattern-matches this idiom to a single `rol` instruction. With c a
 * nonzero literal (16/12/8/7 below) the 32-shift UB corner is never reached. */
static u32 rotl32(u32 v, unsigned c)
{
    return (v << c) | (v >> (32 - c));
}

/* ---------------------------------------------------------------------------
 * chacha_quarter_round — mix four 32-bit state words in place.
 *
 * Given the 16-word state `s` and four indices, apply the ChaCha quarter round.
 * Each of the four ARX steps flips bits in one word based on two others, so
 * after the round every output word depends on every input word (diffusion).
 * The rotate distances 16, 12, 8, 7 are fixed constants of the design.
 * --------------------------------------------------------------------------- */
void chacha_quarter_round(u32 *s, unsigned a, unsigned b, unsigned c, unsigned d)
{
    s[a] += s[b];  s[d] ^= s[a];  s[d] = rotl32(s[d], 16);
    s[c] += s[d];  s[b] ^= s[c];  s[b] = rotl32(s[b], 12);
    s[a] += s[b];  s[d] ^= s[a];  s[d] = rotl32(s[d], 8);
    s[c] += s[d];  s[b] ^= s[c];  s[b] = rotl32(s[b], 7);
}

/* Load/store 32-bit little-endian, byte-wise (endianness- and alignment-safe). */
static u32 load_le32(const u8 *p)
{
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}
static void store_le32(u8 *p, u32 v)
{
    p[0] = (u8)v;         p[1] = (u8)(v >> 8);
    p[2] = (u8)(v >> 16); p[3] = (u8)(v >> 24);
}

/* The same quarter round as above, but as a macro so chacha20_block does not
 * depend on the standalone function (lets the optimizer show its unrolling). */
#define QR(a, b, c, d)                       \
    do {                                     \
        a += b;  d ^= a;  d = rotl32(d, 16); \
        c += d;  b ^= c;  b = rotl32(b, 12); \
        a += b;  d ^= a;  d = rotl32(d, 8);  \
        c += d;  b ^= c;  b = rotl32(b, 7);  \
    } while (0)

/* ---------------------------------------------------------------------------
 * chacha20_block — produce one 64-byte keystream block (RFC 8439 §2.3).
 *
 * Build the 4x4 state (constants, key, counter, nonce), run 20 rounds (10
 * column+diagonal double rounds), then add the original state back and
 * serialise little-endian. The final ADD is the feed-forward that makes the
 * permutation one-way.
 * --------------------------------------------------------------------------- */
void chacha20_block(u8 out[64], const u8 key[32], u32 counter, const u8 nonce[12])
{
    u32 s[16], x[16];

    s[0] = 0x61707865u; s[1] = 0x3320646eu;   /* "expa" "nd 3" */
    s[2] = 0x79622d32u; s[3] = 0x6b206574u;   /* "2-by" "te k" */
    for (int i = 0; i < 8; i++) s[4 + i] = load_le32(key + 4 * i);
    s[12] = counter;
    s[13] = load_le32(nonce + 0);
    s[14] = load_le32(nonce + 4);
    s[15] = load_le32(nonce + 8);

    for (int i = 0; i < 16; i++) x[i] = s[i];

    for (int i = 0; i < 10; i++) {
        QR(x[0], x[4], x[8],  x[12]);   /* columns   */
        QR(x[1], x[5], x[9],  x[13]);
        QR(x[2], x[6], x[10], x[14]);
        QR(x[3], x[7], x[11], x[15]);
        QR(x[0], x[5], x[10], x[15]);   /* diagonals */
        QR(x[1], x[6], x[11], x[12]);
        QR(x[2], x[7], x[8],  x[13]);
        QR(x[3], x[4], x[9],  x[14]);
    }

    for (int i = 0; i < 16; i++) store_le32(out + 4 * i, x[i] + s[i]);
}
