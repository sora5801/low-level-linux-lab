/* ===========================================================================
 * asm/demo.c — the tracer's most instructive PURE-LOGIC core, extracted so it
 * can be compiled to assembly on ANY host.
 * ===========================================================================
 *
 * WHY THIS FILE EXISTS
 * --------------------
 * The real code lives in kernel modules (kprobe_tracer.c, ftrace_tracer.c) that
 * can only be compiled against Linux kernel headers inside a build tree — you
 * cannot `clang -S` them standalone, so they cannot produce didactic assembly
 * on this host. But the single most interesting routine in the whole project is
 * pure integer arithmetic with NO kernel dependency at all: the log2 histogram
 * bucketing from latency_hist.h. We copy it here verbatim, give it its own
 * tiny type definitions (no #include of anything), and compile THIS to asm.
 *
 * The generated files are:
 *   asm/demo.O0.s   -O0, the literal statement-by-statement mapping
 *   asm/demo.s      -O1, the baseline we hand-annotate
 *   asm/demo.O2.s   -O2, where the optimizer earns its keep
 *   asm/demo.annotated.s   hand-written, every instruction explained
 *
 * THE LESSON TO WATCH FOR
 * -----------------------
 * `lat_bucket()` is written as a naive "shift right until zero, counting bits"
 * loop. That is intentionally the *readable* form. At -O2, clang recognizes the
 * idiom and replaces the entire loop with ONE bit-scan instruction
 * (`bsr`/`lzcnt`) — which is exactly what the kernel's `fls64()` intrinsic
 * would have emitted. Diffing demo.s against demo.O2.s makes that
 * loop-to-single-instruction transformation visible, which is the point.
 * ===========================================================================
 */

/* Our own fixed-width types — no <stdint.h>, no kernel headers. On the
 * x86-64 System V LP64 model these widths are exact: int is 32-bit, long long
 * is 64-bit. Mirroring the kernel's u32/u64 so the asm matches the module. */
typedef unsigned int       u32;
typedef unsigned long long u64;

/* Must match HIST_SLOTS in latency_hist.h. A u64 has at most 64 significant
 * bits, so buckets are 0..64; 65 slots hold them all. */
#define HIST_SLOTS 65

/* ---------------------------------------------------------------------------
 * lat_bucket — number of significant bits of v == floor(log2 v) + 1 (v>0).
 *
 *   v == 0             -> 0
 *   v in [2^(b-1), 2^b - 1] -> b
 *
 * Non-static (external linkage) so the compiler MUST emit it even with no
 * caller in this file — otherwise -O1/-O2 would delete an unused static.
 * --------------------------------------------------------------------------- */
u32 lat_bucket(u64 v)
{
	u32 b = 0;

	while (v) {      /* loop runs once per set-or-lower bit remaining        */
		b++;     /* count this bit position                              */
		v >>= 1; /* logical shift: drop the low bit, feed the next       */
	}
	return b;
}

/* Inclusive lower bound of bucket b's value band: 0 for b==0, else 2^(b-1).
 * The `1ULL << (b-1)` is why this shows up in asm as a variable-count `shl`. */
u64 bucket_low(u32 b)
{
	if (b == 0)
		return 0;
	return (u64)1 << (b - 1);
}

/* Inclusive upper bound of bucket b's band: 0 for b==0, else 2^b - 1. */
u64 bucket_high(u32 b)
{
	if (b == 0)
		return 0;
	return ((u64)1 << b) - 1;
}

/* ---------------------------------------------------------------------------
 * hist_record — clamp the bucket into range, then bump that slot's count.
 *
 * In the kernel the slot is an atomic64_t bumped with `lock xadd`; here, with
 * no atomics header, we use a plain `u64 *` and `++`. The interesting part for
 * the assembly is identical either way: compute the index, BOUNDS-CHECK it, and
 * index the array. The clamp is a real safety invariant — an out-of-range index
 * would be an out-of-bounds write, i.e. memory corruption in kernel space — so
 * we keep it in the extracted core to show what the check compiles to.
 * --------------------------------------------------------------------------- */
void hist_record(u64 *slots, u64 value)
{
	u32 b = lat_bucket(value);

	if (b >= HIST_SLOTS)     /* saturate rather than run off the array end   */
		b = HIST_SLOTS - 1;

	slots[b]++;              /* the (bounds-checked) increment               */
}
