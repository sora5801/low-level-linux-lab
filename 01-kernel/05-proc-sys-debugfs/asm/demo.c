/* ===========================================================================
 * demo.c — the seq_file number-formatting CORE, extracted to compile standalone.
 * ===========================================================================
 *
 * WHY THIS FILE EXISTS
 * --------------------
 * The real project (../proc_sys_debugfs.c) is a Linux kernel module. Kernel C
 * cannot be compiled to assembly on this host: it needs the linux/ headers, the kernel
 * build system, and a freestanding target. So — exactly as CONVENTIONS.md §4
 * prescribes — we lift out the project's most instructive PURE-LOGIC helper into
 * this self-contained file, which any clang can turn into real assembly.
 *
 * WHAT WE EXTRACTED
 * -----------------
 * Every one of the three interfaces ultimately turns integers into text:
 * seq_printf("%llu", jiffies), sysfs_emit("%u\n", threshold), and so on. Under
 * all that formatting machinery sits one humble routine — convert an unsigned
 * integer to its decimal ASCII digits. The kernel's own vsnprintf() has exactly
 * this core (lib/vsprintf.c: number() / put_dec()). We reproduce a clean version
 * of it here, plus a fixed-width right-justified field formatter that mirrors the
 * "  %-4lld  %20llu\n" column layout the /proc seq_file emits.
 *
 * There are NO system headers below: we declare our own fixed-width types so the
 * file is hermetic and the generated asm depends on nothing but the C.
 *
 * THE ASM LESSON TO WATCH FOR
 * ---------------------------
 * `u64_to_dec` divides by 10 in a loop. At -O0 clang emits a real `div`/`divq`.
 * At -O1 and -O2 it replaces division-by-constant-10 with a MULTIPLY by a magic
 * reciprocal constant plus a shift (the well-known "division by invariant
 * integers using multiplication" trick). Diff demo.O0.s against demo.s to see a
 * `divq` become a `mulq $0xcccccccccccccccd` + `shr`. That transformation is the
 * single best thing to point a learner at here — see asm/demo.annotated.s.
 * =========================================================================== */

/* Our own types — no <stdint.h>, no <linux/types.h>. On the LP64 model clang
 * uses for x86-64 Linux, these widths are exact. */
typedef unsigned long long u64;   /* 64-bit unsigned                            */
typedef unsigned int       u32;   /* 32-bit unsigned                            */

/* ---------------------------------------------------------------------------
 * u64_to_dec — write the decimal digits of `v` into `out`, return the length.
 *
 * This is the beating heart of every "%u"/"%llu" the module prints. The classic
 * algorithm produces digits least-significant first (v % 10), so they come out
 * BACKWARDS; we write them into a small temp buffer and then reverse into `out`.
 * `out` must have room for up to 20 digits (2^64-1 == 18446744073709551615) plus
 * a NUL terminator: 21 bytes. Returns the number of digit characters written
 * (not counting the NUL), just like an snprintf would report.
 *
 * Pure logic: no allocation, no globals, no I/O. Perfect for reading as asm.
 * --------------------------------------------------------------------------- */
unsigned u64_to_dec(u64 v, char *out)
{
	char tmp[20];          /* holds digits in reverse; 20 = max decimal digits  */
	unsigned n = 0;        /* count of digits emitted into tmp                  */
	unsigned i;

	/* Special-case zero: the loop below would emit nothing for v == 0, but "0"
	 * is one digit, so handle it explicitly. */
	if (v == 0) {
		out[0] = '0';
		out[1] = '\0';
		return 1;
	}

	/* Peel off the low digit repeatedly. `v % 10` is the digit, `v / 10` shifts
	 * right one decimal place. THIS is the division the optimizer will turn into
	 * a magic multiply. `'0' + digit` maps 0..9 to their ASCII codes '0'..'9'. */
	while (v != 0) {
		u64 digit = v % 10;
		tmp[n++] = (char)('0' + digit);
		v /= 10;
	}

	/* Reverse tmp[0..n-1] into out[]. tmp holds the number least-significant
	 * digit first; the printed form is most-significant first. */
	for (i = 0; i < n; i++)
		out[i] = tmp[n - 1 - i];

	out[n] = '\0';         /* NUL-terminate so `out` is a valid C string         */
	return n;
}

/* ---------------------------------------------------------------------------
 * format_field — right-justify `v`'s decimal form in a `width`-wide column.
 *
 * This mirrors seq_printf(m, "%20llu", x): render the number, then pad on the
 * LEFT with spaces so the whole field is exactly `width` characters (unless the
 * number is already wider, in which case it is printed in full and overflows the
 * column, which is what printf does too). Returns the number of characters
 * written to `out` (excluding the NUL).
 *
 * `out` must have room for max(width, digits) + 1 bytes.
 * --------------------------------------------------------------------------- */
unsigned format_field(u64 v, unsigned width, char *out)
{
	char digits[21];                        /* up to 20 digits + NUL            */
	unsigned ndigits = u64_to_dec(v, digits);
	unsigned pad;
	unsigned i;
	unsigned pos = 0;

	/* How many pad spaces? If the number already fills or exceeds the column,
	 * pad is zero. Computing it without going negative matters because these are
	 * UNSIGNED: `width - ndigits` would wrap to a huge value if ndigits > width. */
	pad = (width > ndigits) ? (width - ndigits) : 0;

	/* Emit the leading spaces. */
	for (i = 0; i < pad; i++)
		out[pos++] = ' ';

	/* Copy the rendered digits after the padding. */
	for (i = 0; i < ndigits; i++)
		out[pos++] = digits[i];

	out[pos] = '\0';
	return pos;
}

/* ---------------------------------------------------------------------------
 * checksum8 — a tiny 8-bit additive checksum over `len` bytes.
 *
 * A second, even simpler pure-logic routine, included so the annotated asm can
 * contrast a trivial accumulate-loop (which -O2 will happily vectorize) against
 * the division trick above. Sums bytes mod 256 — the kind of throwaway integrity
 * check a debugfs dump might print next to a buffer. Returns the low 8 bits.
 * --------------------------------------------------------------------------- */
u32 checksum8(const unsigned char *buf, u32 len)
{
	u32 sum = 0;
	u32 i;

	for (i = 0; i < len; i++)
		sum += buf[i];          /* wrap is intentional; we mask to 8 bits below */

	return sum & 0xffu;             /* keep only the low byte                    */
}

/* ---------------------------------------------------------------------------
 * render_line — compose the helpers, mirroring one /proc row: "<idx> <value>".
 *
 * Included as the "top-level driver" so the optimizer has something to INLINE
 * u64_to_dec and format_field INTO. Comparing demo.O0.s (real calls) with
 * demo.s / demo.O2.s (inlined, magic-multiply division) is the whole point of
 * shipping three optimization levels. Writes "<idx> " then the right-justified
 * value, NUL-terminated; returns total length.
 * --------------------------------------------------------------------------- */
unsigned render_line(u32 idx, u64 value, char *out)
{
	unsigned pos = u64_to_dec(idx, out);    /* the row index, left-justified     */

	out[pos++] = ' ';                       /* a single separating space         */

	/* Right-justify the value in a 20-wide column, exactly like the module's
	 * "%20llu". format_field appends and returns its own length. */
	pos += format_field(value, 20, out + pos);

	return pos;
}
