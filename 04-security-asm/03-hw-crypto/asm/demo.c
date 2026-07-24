/* ===========================================================================
 * asm/demo.c — the constant-time byte-equality / conditional-select primitives,
 *              extracted self-contained so we can read their branchless codegen.
 * ===========================================================================
 *
 * This is the "pure-logic routine" the CONVENTIONS ask each project to isolate
 * for annotation. It is the beating heart of constant-time crypto glue: how do
 * you compare two secrets, or pick one of two values, WITHOUT a branch or a
 * memory lookup whose timing an attacker could measure?
 *
 * The classic bug this prevents:
 *
 *     if (memcmp(mac, expected, 16) == 0) accept();     // TIMING ORACLE!
 *
 * `memcmp` returns as soon as it finds the first differing byte, so an attacker
 * who can time it learns HOW MANY leading bytes of a forged MAC were correct,
 * and can forge a valid tag byte-by-byte. The fix is a comparison that always
 * inspects every byte and whose result is computed with arithmetic, not a
 * branch — which is exactly what ct_eq / ct_memeq / ct_select below do.
 *
 * NO SYSTEM HEADERS. We define our own fixed-width types so this file compiles
 * standalone to clean assembly (the point is to READ the asm — see
 * asm/demo.annotated.s). The three generated files are:
 *     demo.O0.s  — literal, spill-everything mapping
 *     demo.s     — clang -O1, the baseline we hand-annotate
 *     demo.O2.s  — clang -O2, watch it become cmov/setcc/sbb with no jumps
 *
 * The lesson to look for in the asm: there are NO conditional JUMPS over the
 * secret data. clang lowers these idioms to `sbb`/`neg`/`setne`/`cmov`, all of
 * which run in the same number of cycles regardless of the values. THAT is what
 * "constant time" looks like at the instruction level.
 * ===========================================================================
 */

/* Our own types: on the LP64 targets we generate for, these widths hold. */
typedef unsigned char      u8;
typedef unsigned int       u32;
typedef unsigned long long u64;

/* ---------------------------------------------------------------------------
 * ct_eq — return 0xFFFFFFFF if x == y, else 0x00000000, with no branch.
 *
 * q  = x ^ y                is 0 exactly when x == y.
 * For any 32-bit q, the expression (q | (0 - q)) has its TOP bit set iff q != 0:
 *   - if q == 0, both q and -q are 0, so the top bit is 0.
 *   - if q != 0, then either q or its two's-complement negation -q has the top
 *     bit set (a nonzero value and its negation can't both have bit31 clear).
 * Shifting that top bit down gives nz in {0,1}; nz - 1 spreads it to a full mask:
 *   nz == 0 (equal)     -> 0u - 1 = 0xFFFFFFFF
 *   nz == 1 (differ)    -> 1  - 1 = 0x00000000
 * --------------------------------------------------------------------------- */
u32 ct_eq(u32 x, u32 y)
{
    u32 q  = x ^ y;
    u32 nz = (q | (0u - q)) >> 31;   /* 1 if x != y, else 0 */
    return nz - 1u;                  /* all-ones if equal, else all-zeros */
}

/* ---------------------------------------------------------------------------
 * ct_select — return (mask ? a : b) where `mask` is all-ones or all-zeros.
 *
 * (a & mask) keeps a when mask is all-ones, else 0.
 * (b & ~mask) keeps b when mask is all-zeros, else 0.
 * Their OR is `a` or `b` with no branch — a data-flow multiplexer. Callers pass
 * a mask produced by ct_eq (or similar), so the selection never becomes a jump.
 * --------------------------------------------------------------------------- */
u32 ct_select(u32 mask, u32 a, u32 b)
{
    return (a & mask) | (b & ~mask);
}

/* ---------------------------------------------------------------------------
 * ct_memeq — constant-time buffer equality (the MAC/tag comparison).
 *
 * We OR together the XOR of every byte pair. `diff` becomes nonzero if ANY byte
 * differs, but — crucially — the loop ALWAYS runs the full length: it never
 * returns early, so its timing reveals nothing about where (or whether) the
 * buffers first diverged. The final ct_eq turns "diff == 0?" into a 0/all-ones
 * answer without a branch.
 * --------------------------------------------------------------------------- */
u32 ct_memeq(const u8 *a, const u8 *b, u64 n)
{
    u8 diff = 0;
    for (u64 i = 0; i < n; i++)
        diff |= (u8)(a[i] ^ b[i]);   /* accumulate; do NOT branch/return here */
    return ct_eq(diff, 0);
}
