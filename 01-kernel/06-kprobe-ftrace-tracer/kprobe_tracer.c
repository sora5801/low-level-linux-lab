// SPDX-License-Identifier: GPL-2.0
/* ===========================================================================
 * kprobe_tracer.c — trace a kernel function with a kprobe + kretprobe pair,
 * logging arguments on entry and measuring per-call latency on exit,
 * WITHOUT recompiling or rebooting into a modified kernel.
 * ===========================================================================
 *
 * WHAT A KPROBE ACTUALLY IS
 * -------------------------
 * `register_kprobe()` asks the kprobes subsystem (kernel/kprobes.c) to make a
 * dynamic breakpoint at an arbitrary kernel instruction. On x86-64 the kernel
 * saves the original byte at the target address and overwrites it with `int3`
 * (0xCC, the one-byte breakpoint trap). When the CPU executes that byte it
 * takes a #BP exception; kprobes' die-notifier catches it, runs OUR
 * `.pre_handler`, single-steps the saved original instruction out-of-line,
 * runs `.post_handler`, and resumes. So we get to run code at any kernel
 * address the same way a debugger would — but in-kernel and per-hit-cheap.
 *
 * WHY ALSO A KRETPROBE
 * --------------------
 * A plain kprobe fires at ONE instruction. To measure how long a *function*
 * takes we need to run code at both its entry and its return. A kretprobe does
 * this by hijacking the return address: on entry it saves the caller's real
 * return address and substitutes a "trampoline"; when the function returns it
 * lands in the trampoline, which runs our `.handler` and then jumps to the real
 * return address. Crucially, each in-flight call gets its own `kretprobe_
 * instance` from a preallocated pool (size `.maxactive`), carrying `.data_size`
 * bytes of scratch. THAT is what lets us stash the entry timestamp and match it
 * to the correct exit even when the function recurses or runs concurrently on
 * many CPUs. If the pool is exhausted (more concurrent calls than `.maxactive`)
 * the kretprobe is simply skipped for that call and `krp.nmissed` counts it —
 * an honest, bounded failure mode rather than corruption.
 *
 * TARGET FUNCTION
 * ---------------
 * We hook `do_sys_openat2()`, the internal worker behind the openat2/openat/
 * open syscalls (fs/open.c):
 *     long do_sys_openat2(int dfd, const char __user *filename,
 *                         struct open_how *how);
 * Hooking the *internal* function (not the `__x64_sys_*` wrapper) means the
 * arguments arrive in the normal SysV kernel calling convention — dfd in rdi,
 * filename in rsi, how in rdx — so we can read them straight out of pt_regs.
 *
 * WHERE THIS RUNS
 * ---------------
 * A kernel module. It cannot be built or loaded on this Windows host; it builds
 * against real Linux kernel headers and must be exercised inside a Linux VM
 * (QEMU/KVM) so a mistake panics a throwaway guest, not your machine. See the
 * README for the exact QEMU recipe.
 * ===========================================================================
 */

#include <linux/module.h>    /* MODULE_*, module_init/exit                    */
#include <linux/kernel.h>    /* pr_info, pr_err                               */
#include <linux/init.h>      /* __init / __exit section hints                 */
#include <linux/kprobes.h>   /* struct kprobe, kretprobe, register_*          */
#include <linux/ktime.h>     /* ktime_get, ktime_sub, ktime_to_ns             */
#include <linux/ptrace.h>    /* pt_regs, regs_get_kernel_argument, ...        */
#include <linux/uaccess.h>   /* strncpy_from_user_nofault                     */
#include <linux/string.h>    /* strscpy                                        */
#include <linux/limits.h>    /* PATH_MAX                                       */
#include <linux/ratelimit.h> /* printk_ratelimited                            */

#include "latency_hist.h"    /* our lock-free log2 latency histogram          */

/* The symbol to trace. Exposed as a module parameter so you can retarget the
 * tracer at load time (e.g. `insmod kprobe_tracer.ko symbol=vfs_read`) without
 * recompiling — a small demonstration of how flexible dynamic tracing is.
 * 0444 = world-readable in /sys/module/kprobe_tracer/parameters/symbol. */
static char *symbol = "do_sys_openat2";
module_param(symbol, charp, 0444);
MODULE_PARM_DESC(symbol, "kernel symbol to trace (default: do_sys_openat2)");

/* The one histogram all exit handlers feed. `static` = file-local; the counters
 * inside are atomic, so concurrent probe hits are safe (see latency_hist.h). */
static struct lat_hist openat_latency;

/* ===========================================================================
 * PART 1 — the plain kprobe: log arguments on ENTRY.
 * ===========================================================================
 *
 * `.pre_handler` runs after the int3 trap, before the traced instruction is
 * single-stepped. Context rules that shape everything below:
 *   - Preemption is DISABLED and we may be deep in the syscall path: we must
 *     not sleep, must not call anything that might sleep.
 *   - `regs` is the full register file captured at the breakpoint, so the
 *     function's arguments are exactly where the ABI put them.
 */

/* Read the Nth (0-based) integer/pointer argument portably. On x86-64 this just
 * returns regs->di, regs->si, regs->dx, ... but `regs_get_kernel_argument()`
 * hides the arch detail, so the same code reads args on arm64 etc. */
static int trace_pre(struct kprobe *p, struct pt_regs *regs)
{
	/* arg0 = dfd (a directory fd, or AT_FDCWD == -100 for "current dir"). */
	int dfd = (int)regs_get_kernel_argument(regs, 0);

	/* arg1 = filename, a POINTER INTO USER SPACE. We must not dereference it
	 * with a plain load: it may be paged out, and taking a page fault here —
	 * with preemption disabled inside a probe — is exactly the kind of sleep
	 * we are forbidden from doing. The correct primitive is the *_nofault
	 * copy, which walks the page tables without ever registering a fault
	 * handler: if the page is not resident it returns -EFAULT instead of
	 * sleeping. This is the same reason eBPF exposes bpf_probe_read_user_str.*/
	const char __user *ufname =
		(const char __user *)regs_get_kernel_argument(regs, 1);

	char buf[64];  /* small on-stack copy; probe stacks are precious, keep it
	                * modest rather than PATH_MAX (4096) which risks overflow. */
	long n = strncpy_from_user_nofault(buf, ufname, sizeof(buf));

	if (n < 0)
		/* Page not resident / bad pointer: report the fact, not garbage. */
		strscpy(buf, "<unavailable>", sizeof(buf));

	/* printk from a hot syscall path would flood the log and can itself be
	 * costly, so rate-limit it. This is a *diagnostic* of entry events; the
	 * quantitative signal is the latency histogram, not these lines. */
	printk_ratelimited(KERN_INFO
		"kprobe_tracer: %s(dfd=%d, file=\"%s\")\n",
		symbol, dfd, buf);

	return 0;  /* 0 = "continue normally"; nonzero would be a special resume */
}

static struct kprobe entry_kp = {
	.symbol_name = NULL,   /* filled in from `symbol` at init (see below)    */
	.pre_handler = trace_pre,
	/* No .post_handler: we don't need to run anything after the instruction
	 * single-steps. Leaving it NULL keeps the fast path fast. */
};

/* ===========================================================================
 * PART 2 — the kretprobe: measure ENTRY->EXIT latency.
 * ===========================================================================
 *
 * Per-call scratch carried in each kretprobe_instance. `.data_size` below tells
 * the pool how many bytes to reserve per in-flight call; `ri->data` points at
 * ours. Storing the entry timestamp here (rather than in a shared variable) is
 * what makes latency correct under recursion and SMP concurrency: instance A's
 * t0 can never be clobbered by instance B.
 */
struct call_scratch {
	ktime_t t0;  /* monotonic timestamp taken at function entry             */
};

/* `.entry_handler` runs on function entry (before the body). We use the
 * MONOTONIC clock (ktime_get) — never the wall clock — because we are timing a
 * duration and wall time can jump backward on NTP steps/settimeofday, which
 * would produce negative or absurd latencies. */
static int lat_entry(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	struct call_scratch *s = (struct call_scratch *)ri->data;

	s->t0 = ktime_get();
	return 0;  /* returning nonzero would SKIP the return handler for this
	            * call — a documented way to filter, which we don't use.     */
}

/* `.handler` runs at function return, in the trampoline. `regs` here holds the
 * return state, so regs_return_value(regs) is the function's return value
 * (rax on x86-64) — for do_sys_openat2 that is the new fd or a negative errno. */
static int lat_return(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	struct call_scratch *s = (struct call_scratch *)ri->data;
	ktime_t now = ktime_get();

	/* ktime_sub yields a signed ns delta; it is non-negative because the
	 * monotonic clock only moves forward and entry strictly precedes exit. */
	s64 delta_ns = ktime_to_ns(ktime_sub(now, s->t0));
	long ret = regs_return_value(regs);

	if (delta_ns < 0)
		delta_ns = 0;  /* paranoia: clamp a nonsense delta to bucket 0     */

	/* Feed the log2 histogram. One atomic increment; safe on any CPU. */
	hist_record(&openat_latency, (u64)delta_ns);

	/* Surface slow calls at INFO but rate-limited, so a storm of opens can't
	 * turn the tracer into the bottleneck. The histogram already captured it. */
	if (delta_ns > 1000000 /* 1 ms */)
		printk_ratelimited(KERN_INFO
			"kprobe_tracer: %s ret=%ld took %lld ns (SLOW)\n",
			symbol, ret, delta_ns);

	return 0;
}

static struct kretprobe krp = {
	.entry_handler = lat_entry,
	.handler       = lat_return,
	.data_size     = sizeof(struct call_scratch),
	/* Size of the instance pool = max concurrent in-flight calls we can time.
	 * 0 would let the kernel pick a small default (num_online_cpus()); we ask
	 * for a generous 64 so a burst of parallel opens is not under-counted.
	 * Overshoot shows up in krp.nmissed, which we print at teardown. */
	.maxactive     = 64,
	/* .kp is an embedded struct kprobe; we set its symbol_name at init. */
};

/* ===========================================================================
 * Module lifecycle.
 * ===========================================================================
 */
static int __init tracer_init(void)
{
	int ret;

	/* Point both probes at the requested symbol. kprobes resolves the name to
	 * an address via kallsyms internally, so we never need the unexported
	 * kallsyms_lookup_name() ourselves. */
	entry_kp.symbol_name = symbol;
	krp.kp.symbol_name   = symbol;

	/* Register the entry-logging kprobe first. Failure here is usually -EINVAL
	 * (symbol not found / blacklisted — some functions like the kprobe int3
	 * path itself are un-probeable to avoid infinite recursion). */
	ret = register_kprobe(&entry_kp);
	if (ret < 0) {
		pr_err("kprobe_tracer: register_kprobe(%s) failed: %d\n",
		       symbol, ret);
		return ret;
	}

	/* Then the latency kretprobe. If this fails we must unwind the kprobe we
	 * already installed — leaving a half-registered probe would keep an int3
	 * patched into live kernel text after our module is gone: an instant crash
	 * on the next hit. Ordered cleanup is not optional in the kernel. */
	ret = register_kretprobe(&krp);
	if (ret < 0) {
		pr_err("kprobe_tracer: register_kretprobe(%s) failed: %d\n",
		       symbol, ret);
		unregister_kprobe(&entry_kp);
		return ret;
	}

	pr_info("kprobe_tracer: armed on '%s' (kprobe@%p, kretprobe maxactive=%d)\n",
		symbol, entry_kp.addr, krp.maxactive);
	return 0;
}

static void __exit tracer_exit(void)
{
	/* Unregister in the reverse order of registration. Both unregister calls
	 * restore the original instruction bytes and synchronize so that no CPU is
	 * still executing inside a handler before we return — that barrier is what
	 * makes it safe for our module's code (the handlers) to be unloaded next. */
	unregister_kretprobe(&krp);
	unregister_kprobe(&entry_kp);

	/* Report the pool-exhaustion counter: how many calls we could not time
	 * because more than `maxactive` were in flight at once. Nonzero here is the
	 * signal to raise `.maxactive`. */
	pr_info("kprobe_tracer: %s: kretprobe missed %d calls (pool exhausted)\n",
		symbol, krp.nmissed);

	/* Dump the accumulated latency distribution to the kernel log. */
	hist_dump(&openat_latency, "kprobe_tracer latency");

	pr_info("kprobe_tracer: disarmed\n");
}

module_init(tracer_init);
module_exit(tracer_exit);

MODULE_LICENSE("GPL");            /* kprobes symbols are GPL-only exports.     */
MODULE_AUTHOR("low-level-linux-lab");
MODULE_DESCRIPTION("kprobe + kretprobe tracer: log args, measure latency, log2 histogram");
