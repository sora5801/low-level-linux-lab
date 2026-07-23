/* ===========================================================================
 * latency_hist.h — a lock-free power-of-two ("log2") latency histogram.
 * ===========================================================================
 *
 * WHY A LOG2 HISTOGRAM?
 * ---------------------
 * Latencies span many orders of magnitude: an openat() that hits the dentry
 * cache returns in ~200 ns, one that faults in from spinning rust can take
 * tens of milliseconds — a spread of ~10^5. A linear histogram would need
 * millions of buckets to keep resolution at both ends. The classic trick,
 * used by bpftrace's `@ = hist()`, BCC, and `perf`, is to bucket by the
 * *number of significant bits* of the value, i.e. floor(log2(v)) + 1. Each
 * bucket then covers a power-of-two-wide band [2^(b-1), 2^b - 1], so you get
 * constant *relative* resolution across the whole range with only ~64 slots.
 *
 * THE INVARIANT THAT MAKES THIS FILE SAFE TO SHARE
 * ------------------------------------------------
 * A kprobe / kretprobe / ftrace handler can fire:
 *   - on ANY CPU, concurrently with itself (two opens racing on two cores), and
 *   - in a non-preemptible, may-be-interrupt context where you MUST NOT sleep.
 * That rules out any lock that can sleep (mutex) and makes even a spinlock a
 * poor choice on the hot path. So the counters are `atomic64_t` and we bump
 * them with `atomic64_inc`, which lowers to a single `lock xadd` on x86-64 —
 * a CPU-atomic read-modify-write that cannot lose an increment no matter how
 * many cores hit the same bucket at once. We only ever *increment* and, at
 * teardown, *read*; there is no compound invariant across slots, so relaxed
 * atomic increments are all the ordering we need (we never publish a pointer
 * or gate other memory on these counts).
 *
 * The bucketing math itself (`lat_bucket`) is pure integer logic with no
 * kernel dependency at all — that is exactly why it is the routine extracted
 * into asm/demo.c and annotated: it is the most instructive standalone core.
 * ===========================================================================
 */
#ifndef LATENCY_HIST_H
#define LATENCY_HIST_H

#include <linux/types.h>   /* u64, u32                                        */
#include <linux/atomic.h>  /* atomic64_t, atomic64_inc, atomic64_read         */
#include <linux/kernel.h>  /* pr_info                                         */

/* A u64 has at most 64 significant bits, so bucket indices run 0..64. We size
 * the array to 65 and, defensively, clamp anything at or past the top into the
 * last slot. 2^63 ns is ~292 years, so in practice only the low buckets fill —
 * but a corrupt/huge delta must never index out of bounds (that would be an
 * out-of-array write in kernel space: memory corruption, not a wrong number). */
#define HIST_SLOTS 65

/* ---------------------------------------------------------------------------
 * lat_bucket — map a value to its log2 bucket = count of significant bits.
 *
 *   v == 0            -> 0        (the "zero" bucket)
 *   v in [1,1]        -> 1        (2^0)
 *   v in [2,3]        -> 2        (2^1 .. 2^2-1)
 *   v in [4,7]        -> 3
 *   v in [2^(b-1), 2^b - 1] -> b
 *
 * This is deliberately the naive shift loop rather than the kernel's `fls64()`
 * intrinsic. Reason: this same function is copied verbatim into asm/demo.c so
 * we can compile it standalone and watch the optimizer. At -O2 clang recognizes
 * the loop and replaces it with a single `bsr`/`lzcnt` bit-scan instruction —
 * which is precisely what `fls64()` expands to. Seeing that transformation is
 * the whole point of the assembly deliverable, so we keep the readable loop
 * here and let the annotation explain the machine-code shortcut.
 * --------------------------------------------------------------------------- */
static inline u32 lat_bucket(u64 v)
{
	u32 b = 0;
	while (v) {          /* each iteration consumes one bit from the bottom  */
		b++;         /* ...and counts it                                 */
		v >>= 1;
	}
	return b;            /* b == number of significant bits == log2 bucket  */
}

/* Inclusive lower bound (in the same unit as the recorded value, here ns) of
 * the band a bucket covers. Bucket 0 is the single value 0; bucket b>=1 starts
 * at 2^(b-1). Used only for the human-readable dump, never on the hot path. */
static inline u64 bucket_low(u32 b)
{
	if (b == 0)
		return 0;
	return (u64)1 << (b - 1);
}

/* Inclusive upper bound of a bucket's band: 2^b - 1 for b>=1. */
static inline u64 bucket_high(u32 b)
{
	if (b == 0)
		return 0;
	return ((u64)1 << b) - 1;
}

/* The histogram itself: one atomic counter per bucket. `atomic64_t` is a
 * struct wrapper around a 64-bit counter; ATOMIC64_INIT(0) zero-initializes it
 * at compile time. An array initializer of one element value-initializes the
 * rest, so this zeroes all 65 slots. */
struct lat_hist {
	atomic64_t slot[HIST_SLOTS];
};

/* Record one sample. Called from probe context (atomic, any CPU), so it must
 * not sleep and must be reentrancy-safe — both guaranteed because the only
 * shared write is a single atomic increment. */
static inline void hist_record(struct lat_hist *h, u64 value)
{
	u32 b = lat_bucket(value);

	/* Clamp is a safety invariant, not an expected path: b can only exceed
	 * the array if `value` had more bits than HIST_SLOTS-1, which for ns
	 * latencies cannot happen — but we refuse to trust that and index safely
	 * rather than corrupt adjacent kernel memory. */
	if (b >= HIST_SLOTS)
		b = HIST_SLOTS - 1;

	atomic64_inc(&h->slot[b]);   /* lock xadd $1 — cannot lose a race        */
}

/* Pretty-print the histogram to the kernel log. Called from process context at
 * module teardown (NOT from a probe), so pr_info — which can sleep on console
 * locks — is fine here. We read each counter once with atomic64_read; a racing
 * increment during teardown would simply be reflected or not, which is
 * harmless for a diagnostic dump. */
static inline void hist_dump(const struct lat_hist *h, const char *title)
{
	int i;
	u64 total = 0;

	for (i = 0; i < HIST_SLOTS; i++)
		total += (u64)atomic64_read(&h->slot[i]);

	pr_info("%s: %llu samples\n", title, total);
	pr_info("      nsec range            : count\n");

	for (i = 0; i < HIST_SLOTS; i++) {
		u64 c = (u64)atomic64_read(&h->slot[i]);

		if (c == 0)          /* skip empty rows to keep dmesg readable   */
			continue;

		/* [lo, hi] is the closed ns interval this bucket represents. */
		pr_info("  [%14llu, %14llu] : %llu\n",
			bucket_low((u32)i), bucket_high((u32)i), c);
	}
}

#endif /* LATENCY_HIST_H */
