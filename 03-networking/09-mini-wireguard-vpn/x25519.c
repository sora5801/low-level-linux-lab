/* ===========================================================================
 * x25519.c — Curve25519 scalar multiplication.
 * ===========================================================================
 *
 * This is a transcription of Daniel J. Bernstein et al.'s **TweetNaCl**
 * `crypto_scalarmult` (public domain), with commentary. TweetNaCl is chosen on
 * purpose: it is tiny, constant-time, and one of the most audited pieces of
 * crypto C in existence. Re-deriving Curve25519 field arithmetic by hand is a
 * classic way to ship a subtle, exploitable bug, so we stand on vetted code and
 * spend our effort EXPLAINING it.
 *
 * REPRESENTATION
 * --------------
 * A field element mod p = 2^255 - 19 is held as `gf` = 16 signed 64-bit limbs,
 * each nominally a 16-bit "digit" (radix 2^16): value = sum limb[i] * 2^(16i).
 * Using 64-bit limbs for 16-bit digits leaves ~48 bits of headroom, so we can
 * add and multiply several times before carrying (`car25519`) back into range.
 *
 * THE LADDER
 * ----------
 * scalarmult walks the 255 scalar bits from high to low running a Montgomery
 * ladder: at each bit it does a constant set of field add/sub/mul on two
 * candidate points and conditionally swaps them (`sel25519`) based on the bit —
 * branchlessly, so the timing never depends on the secret scalar.
 * =========================================================================== */

#include "x25519.h"

typedef long long   i64;
typedef i64         gf[16];    /* a field element: 16 limbs, radix 2^16          */

/* The curve constant (486662 - 2)/4 = 121665, used once per ladder step. */
static const gf gf_121665 = { 0xDB41, 1 };

/* Conditional constant-time swap of p and q when b == 1. We build a full-width
 * mask from b (0 -> 0x000…0, 1 -> 0xFFF…F) and XOR-swap under it. No branch on
 * the secret bit -> constant time. */
static void sel25519(gf p, gf q, int b)
{
    i64 t, c = ~(b - 1);           /* c = 0 if b==0, all-ones if b==1            */
    for (int i = 0; i < 16; i++) {
        t = c & (p[i] ^ q[i]);
        p[i] ^= t;
        q[i] ^= t;
    }
}

/* Carry-propagate a field element back to 16-bit limbs, folding the overflow
 * out of the top limb back into limb 0 with weight 38 (because 2^256 ≡ 38
 * (mod p): 2^255 ≡ 19, so 2^256 ≡ 38). This is the reduction mod 2^255-19. */
static void car25519(gf o)
{
    i64 c;
    for (int i = 0; i < 16; i++) {
        o[i] += (i64)1 << 16;                       /* bias so >>16 rounds toward 0 */
        c = o[i] >> 16;
        o[(i + 1) * (i < 15)] += c - 1 + 37 * (c - 1) * (i == 15);
        o[i] -= c << 16;
    }
}

static void A(gf o, const gf a, const gf b) { for (int i=0;i<16;i++) o[i]=a[i]+b[i]; } /* add */
static void Z(gf o, const gf a, const gf b) { for (int i=0;i<16;i++) o[i]=a[i]-b[i]; } /* sub */

/* Field multiply: schoolbook 16x16 into a 31-limb product, then fold limbs
 * 16..30 back down with weight 38 (2^256 ≡ 38), then carry twice to normalise. */
static void M(gf o, const gf a, const gf b)
{
    i64 t[31];
    for (int i = 0; i < 31; i++) t[i] = 0;
    for (int i = 0; i < 16; i++)
        for (int j = 0; j < 16; j++)
            t[i + j] += a[i] * b[j];
    for (int i = 0; i < 15; i++) t[i] += 38 * t[i + 16];   /* reduce top half     */
    for (int i = 0; i < 16; i++) o[i] = t[i];
    car25519(o);
    car25519(o);
}

static void S(gf o, const gf a) { M(o, a, a); }            /* square = self-mul   */

/* Field inversion by Fermat: a^(p-2) mod p, via a fixed square-and-multiply
 * chain over the 255-bit exponent (skipping bits 2 and 4, which are 0 in p-2).
 * Constant time: the exponent is public (p-2), not a secret. */
static void inv25519(gf o, const gf i)
{
    gf c;
    for (int a = 0; a < 16; a++) c[a] = i[a];
    for (int a = 253; a >= 0; a--) {
        S(c, c);
        if (a != 2 && a != 4) M(c, c, i);
    }
    for (int a = 0; a < 16; a++) o[a] = c[a];
}

/* Load a 32-byte little-endian u-coordinate into a field element, masking off
 * bit 255 (Curve25519 u-coordinates are only 255 bits). */
static void unpack25519(gf o, const u8 *n)
{
    for (int i = 0; i < 16; i++) o[i] = n[2 * i] + ((i64)n[2 * i + 1] << 8);
    o[15] &= 0x7fff;
}

/* Fully reduce a field element mod p and serialise it little-endian. The two
 * conditional subtractions bring a value in [0, 2p) down to [0, p). */
static void pack25519(u8 *o, const gf n)
{
    gf m, t;
    for (int i = 0; i < 16; i++) t[i] = n[i];
    car25519(t); car25519(t); car25519(t);
    for (int j = 0; j < 2; j++) {
        m[0] = t[0] - 0xffed;                       /* subtract p, limb by limb   */
        for (int i = 1; i < 15; i++) {
            m[i] = t[i] - 0xffff - ((m[i - 1] >> 16) & 1);
            m[i - 1] &= 0xffff;
        }
        m[15] = t[15] - 0x7fff - ((m[14] >> 16) & 1);
        int b = (m[15] >> 16) & 1;                  /* borrow => t < p, keep t    */
        m[14] &= 0xffff;
        sel25519(t, m, 1 - b);                      /* pick t or (t-p) branchlessly */
    }
    for (int i = 0; i < 16; i++) {
        o[2 * i]     = t[i] & 0xff;
        o[2 * i + 1] = t[i] >> 8;
    }
}

void x25519(u8 out[X25519_KEY_LEN],
            const u8 scalar[X25519_KEY_LEN],
            const u8 point[X25519_KEY_LEN])
{
    u8 z[32];
    i64 x[80], r;
    gf a, b, c, d, e, f;

    /* Clamp the scalar: clear the low 3 bits (cofactor 8), force bit 254 set and
     * bit 255 clear. This constrains the scalar to a fixed range and multiple of
     * 8, defeating small-subgroup attacks and keeping the ladder well-defined. */
    for (int i = 0; i < 31; i++) z[i] = scalar[i];
    z[31] = (scalar[31] & 127) | 64;
    z[0] &= 248;

    unpack25519(x, point);

    /* Ladder init: (a,c) = point-at-infinity-ish (1,0), (b,d) = (x,1). */
    for (int i = 0; i < 16; i++) { b[i] = x[i]; d[i] = a[i] = c[i] = 0; }
    a[0] = d[0] = 1;

    /* Montgomery ladder over scalar bits 254..0, high to low. Every iteration is
     * the SAME sequence of field ops on both candidate points; the only data-
     * dependence is the branchless conditional swap keyed on the current bit. */
    for (int i = 254; i >= 0; --i) {
        r = (z[i >> 3] >> (i & 7)) & 1;   /* the i-th scalar bit                 */
        sel25519(a, b, r);                /* swap in the bit's candidate         */
        sel25519(c, d, r);
        A(e, a, c);  Z(a, a, c);
        A(c, b, d);  Z(b, b, d);
        S(d, e);     S(f, a);
        M(a, c, a);  M(c, b, e);
        A(e, a, c);  Z(a, a, c);
        S(b, a);     Z(c, d, f);
        M(a, c, gf_121665);
        A(a, a, d);  M(c, c, a);
        M(a, d, f);  M(d, b, x);
        S(b, e);
        sel25519(a, b, r);                /* swap back                           */
        sel25519(c, d, r);
    }
    /* Recover the affine u-coordinate: result = a / c = a * c^(p-2). */
    for (int i = 0; i < 16; i++) { x[i+16]=a[i]; x[i+32]=c[i]; x[i+48]=b[i]; x[i+64]=d[i]; }
    inv25519(x + 32, x + 32);
    M(x + 16, x + 16, x + 32);
    pack25519(out, x + 16);

    secure_zero(z, sizeof z);
    secure_zero(x, sizeof x);
}

void x25519_base(u8 out[X25519_KEY_LEN], const u8 scalar[X25519_KEY_LEN])
{
    /* The basepoint G has u-coordinate 9. pub = scalar * 9. */
    static const u8 basepoint[32] = { 9 };
    x25519(out, scalar, basepoint);
}
