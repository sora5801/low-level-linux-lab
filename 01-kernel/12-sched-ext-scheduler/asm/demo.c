/* ===========================================================================
 * demo.c — the pure-arithmetic HEART of the scx_fifo scheduler, extracted so it
 * can be compiled to standalone assembly and studied.
 * ===========================================================================
 *
 * WHY THIS FILE EXISTS
 * --------------------
 * scx_fifo.bpf.c is a BPF program: it #includes <scx/common.bpf.h> and
 * vmlinux.h, references `struct task_struct`, and is compiled with `clang
 * -target bpf` against a live kernel's BTF. You cannot compile it to ordinary
 * x86-64 assembly on a normal host — it has no standalone form. So, per the
 * lab's assembly rule, we lift out its most instructive PURE-LOGIC core — the
 * virtual-time / weight arithmetic from .enqueue(), .stopping() and
 * vtime_before() — into this dependency-free file. It declares its own integer
 * types, includes NO headers, and computes exactly what the scheduler computes.
 * The committed asm/demo.{O0,,O2}.s and the hand-annotated asm/demo.annotated.s
 * are generated from THIS file. See the README's "Assembly notes."
 *
 * The interesting machine-level lessons hiding in a few lines of C:
 *   - a wrap-safe timestamp comparison that becomes a single sign test;
 *   - a 64-bit multiply-then-divide (the CPU's `mul`/`div` on rdx:rax) — and
 *     WHY the multiply must come before the divide;
 *   - how -O2 turns the divide-by-a-constant weight into a reciprocal multiply
 *     when it can, and inlines the whole update into one straight-line body.
 * =========================================================================== */

/* We are freestanding here: no <stdint.h>, so we spell the kernel's integer
 * types ourselves. On the x86-64 LP64 model these widths are exact matches for
 * the u64/s64/u32 the real scheduler uses. */
typedef unsigned long long u64;   /* 64-bit unsigned: vtime, slices (ns)      */
typedef signed   long long s64;   /* 64-bit signed: the wrap-safe difference  */
typedef unsigned int       u32;   /* 32-bit unsigned: a task's weight         */

/* The default time slice: 20 milliseconds expressed in nanoseconds, matching
 * SCX_SLICE_DFL in the kernel. A task is granted this much CPU before it can be
 * preempted; whatever it does not use is refunded to its virtual clock. */
#define SCX_SLICE_DFL   20000000ULL

/* The nominal weight (nice level 0). weight/100 is a task's share multiplier:
 * weight 200 gets twice the CPU of weight 100, weight 50 gets half. */
#define NICE0_WEIGHT    100ULL

/* ---------------------------------------------------------------------------
 * vtime_before — wrap-safe "is virtual time `a` earlier than `b`?"
 *
 * Comparing two u64 timestamps with a<b is WRONG once they straddle the point
 * where the 64-bit counter wraps: the smaller-looking value may actually be the
 * later one. The fix is to subtract (which wraps consistently in modular
 * arithmetic) and interpret the result as SIGNED: if a is behind b by less than
 * half the range, (s64)(a-b) is negative. Watch the assembly collapse this to a
 * subtract and a sign-bit test — no branch on the magnitudes at all.
 * --------------------------------------------------------------------------- */
int vtime_before(u64 a, u64 b)
{
	return (s64)(a - b) < 0;
}

/* ---------------------------------------------------------------------------
 * vtime_charge — advance a task's virtual clock for the CPU time it just used.
 *
 * `slice_remaining` is what was left of its 20ms grant when it stopped, so
 * (SCX_SLICE_DFL - slice_remaining) is the real nanoseconds consumed. We scale
 * that by NICE0_WEIGHT/weight: a heavier (higher-priority) task is charged LESS
 * virtual time for the same real time, so its clock lags and it keeps sorting
 * to the front of the run queue. This is the exact line from .stopping().
 *
 * ORDER OF OPERATIONS IS LOAD-BEARING: we multiply by 100 BEFORE dividing by
 * weight. Do the divide first and small slices would round to zero and the
 * task would accrue no virtual time at all — a fairness bug. In the assembly
 * you will see the 64-bit `mul` land in rdx:rax, then a 64-bit `div` by weight.
 * --------------------------------------------------------------------------- */
u64 vtime_charge(u64 vtime, u64 slice_remaining, u32 weight)
{
	u64 used = SCX_SLICE_DFL - slice_remaining;   /* ns actually consumed  */
	return vtime + used * NICE0_WEIGHT / weight;  /* weight-scaled charge  */
}

/* ---------------------------------------------------------------------------
 * vtime_clamp — bound how much "credit" a long-sleeping task may carry.
 *
 * A task that slept keeps its old (small) vtime; on waking it would otherwise
 * dominate the CPU until its clock caught up to everyone else's. We refuse to
 * let any task be more than one slice ahead of the global frontier `vtime_now`:
 * if it is further behind than that, snap it up to the floor. This is the
 * anti-hoarding clamp from .enqueue(). Note the wrap-safe compare is reused.
 * --------------------------------------------------------------------------- */
u64 vtime_clamp(u64 vtime, u64 vtime_now)
{
	u64 floor = vtime_now - SCX_SLICE_DFL;        /* one slice of credit   */
	if (vtime_before(vtime, floor))
		vtime = floor;
	return vtime;
}

/* ---------------------------------------------------------------------------
 * vtime_on_stop — the whole .stopping() virtual-time update in one function.
 *
 * Charge for the time used, then clamp against the frontier. Written as a
 * composite so the -O2 assembly can show the optimizer INLINING vtime_charge
 * and vtime_clamp into a single straight-line body and folding the two
 * constant subtractions together. Compare demo.O0.s (three real `call`s) with
 * demo.O2.s (one flat function) to see the inliner at work.
 * --------------------------------------------------------------------------- */
u64 vtime_on_stop(u64 vtime, u64 slice_remaining, u32 weight, u64 vtime_now)
{
	vtime = vtime_charge(vtime, slice_remaining, weight);
	vtime = vtime_clamp(vtime, vtime_now);
	return vtime;
}
