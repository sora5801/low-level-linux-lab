/* ===========================================================================
 * asm/demo.c — the pure-logic core of printf, extracted for the asm study.
 * ===========================================================================
 *
 * This is a SELF-CONTAINED slice of ../src/printf.c: the integer-to-ASCII
 * conversion (`%u` and `%d`). No #includes, no libc, its own fixed-width types
 * — so it compiles standalone and its assembly is meaningful on any host that
 * has clang cross-targeting Linux.
 *
 * Why this routine? It is where the interesting instruction lives: division.
 * The C source divides by 10 in a loop, but the optimizer refuses to emit the
 * slow `div` instruction for a constant divisor. Instead it multiplies by a
 * magic reciprocal (0xCCCC...CCCD) and shifts — the classic "division by
 * invariant integers using multiplication" trick (Granlund & Montgomery). The
 * annotated assembly in demo.annotated.s walks exactly that transformation, so
 * you can SEE `/10` become `mul` + `shr`.
 *
 * Generate the three compiler outputs and hand-annotate the -O1 one:
 *   clang --target=x86_64-pc-linux-gnu -S -O0 -fno-asynchronous-unwind-tables \
 *         -fno-jump-tables -fno-omit-frame-pointer demo.c -o demo.O0.s
 *   clang --target=x86_64-pc-linux-gnu -S -O1 -fno-asynchronous-unwind-tables \
 *         -fno-jump-tables -fno-omit-frame-pointer demo.c -o demo.s
 *   clang --target=x86_64-pc-linux-gnu -S -O2 -fno-asynchronous-unwind-tables \
 *         -fno-jump-tables demo.c -o demo.O2.s
 * ===========================================================================
 */

/* Our own types: no <stdint.h>. On the LP64 target these widths hold. */
typedef unsigned long u64;   /* 64-bit unsigned                              */
typedef unsigned int  u32;   /* 32-bit unsigned                              */
typedef long          i64;   /* 64-bit signed                                */

/* ---------------------------------------------------------------------------
 * u64_to_dec — write `v` as decimal ASCII into `out`, return the digit count.
 *
 * Contract: `out` must have room for up to 20 bytes (the decimal length of
 * 2^64-1 = 18446744073709551615). No NUL is written; the caller knows the
 * length from the return value. This is exactly ob_uint(base=10) from printf.
 *
 * The two-phase shape — emit least-significant digit first into `tmp`, then
 * reverse into `out` — is inherent to base conversion by repeated division:
 * you extract the low digit first but must print the high digit first.
 * --------------------------------------------------------------------------- */
u32 u64_to_dec(u64 v, char *out)
{
	char tmp[20];                     /* scratch: max decimal digits of u64    */
	u32 i = 0;

	if (v == 0) {                     /* 0 has no digits via the loop below    */
		out[0] = '0';
		return 1;
	}

	/* Extract digits low-to-high. Each iteration: q = v/10, r = v - q*10 is the
	 * current least-significant digit. The compiler replaces `v / 10` with a
	 * multiply-high by a magic constant plus a shift; `r` then falls out of a
	 * subtract, avoiding a second division. */
	while (v != 0) {
		u64 q = v / 10;
		u64 r = v - q * 10;           /* r == v % 10, computed without a 2nd div */
		tmp[i++] = (char)('0' + r);   /* digit value -> ASCII ('0' is 0x30)    */
		v = q;
	}

	/* Reverse tmp[0..i) into out[0..i): most-significant digit first. */
	for (u32 j = 0; j < i; j++)
		out[j] = tmp[i - 1 - j];

	return i;
}

/* ---------------------------------------------------------------------------
 * i64_to_dec — signed wrapper: emit '-' for negatives, then the magnitude.
 *
 * The magnitude is computed as -(u64)v, NOT -v. For v = INT64_MIN there is no
 * positive i64 that represents |v|, so negating in signed arithmetic is
 * undefined behavior; casting to u64 first makes the negation well-defined
 * modular arithmetic that yields the correct magnitude bit-pattern. This is
 * the same subtlety ob_int() guards against in printf.c.
 * --------------------------------------------------------------------------- */
u32 i64_to_dec(i64 v, char *out)
{
	if (v < 0) {
		out[0] = '-';
		return 1 + u64_to_dec(-(u64)v, out + 1);
	}
	return u64_to_dec((u64)v, out);
}
