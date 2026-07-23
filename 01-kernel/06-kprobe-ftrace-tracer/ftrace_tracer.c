// SPDX-License-Identifier: GPL-2.0
/* ===========================================================================
 * ftrace_tracer.c — the SAME "hook a kernel function" idea, but via ftrace_ops
 * (the fentry mechanism) instead of a kprobe's int3 breakpoint.
 * ===========================================================================
 *
 * TWO WAYS TO HOOK, AND WHY THIS ONE IS FASTER
 * --------------------------------------------
 * kprobe_tracer.c patched an `int3` breakpoint into the target. That is fully
 * general (it can hook ANY instruction) but every hit takes a trap: an
 * exception, a mode switch, single-stepping. ftrace uses a different lever that
 * only works at *function entry*, but is far cheaper:
 *
 *   The kernel is compiled with -pg/-mfentry, so the compiler emits a
 *   `call __fentry__` as the very first instruction of (almost) every function.
 *   With CONFIG_DYNAMIC_FTRACE those calls are patched to 5-byte NOPs at boot,
 *   costing nothing. When you attach an ftrace_ops to a function, ftrace patches
 *   that NOP back into a call to a trampoline that invokes YOUR `.func`. So the
 *   overhead is a direct call, not a trap. This is the exact same machinery
 *   BPF `fentry/` programs, live-patching (klp), and the function tracer use.
 *
 * WHAT THIS VARIANT DOES
 * ----------------------
 * It hooks the function ENTRY only (that is all fentry offers) and logs the
 * first argument. It deliberately does NOT measure return latency: a bare
 * ftrace_ops has no exit hook. To time a function with ftrace you use the
 * *function graph* tracer (which patches the return address, much like a
 * kretprobe) — noted in the README's "going further". Keeping this module
 * entry-only makes the contrast with the kretprobe crisp: kretprobe = latency,
 * ftrace_ops = cheapest possible entry hook.
 *
 * Build/run: a Linux kernel module; exercise it in a QEMU VM. Requires a kernel
 * with CONFIG_DYNAMIC_FTRACE and, for register access, CONFIG_DYNAMIC_FTRACE_
 * WITH_REGS (true on essentially all modern x86-64 distro kernels).
 * ===========================================================================
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/ftrace.h>    /* ftrace_ops, register_ftrace_function, ...     */
#include <linux/ptrace.h>    /* pt_regs, regs_get_kernel_argument             */
#include <linux/uaccess.h>   /* strncpy_from_user_nofault                     */
#include <linux/string.h>    /* strscpy, strlen                               */
#include <linux/ratelimit.h> /* printk_ratelimited                            */
#include <linux/atomic.h>    /* atomic64_t call counter                       */

static char *symbol = "do_sys_openat2";
module_param(symbol, charp, 0444);
MODULE_PARM_DESC(symbol, "kernel symbol to hook via ftrace (default: do_sys_openat2)");

/* A single atomic counter of how many times the hooked function was entered.
 * Atomic for the same reason as the kprobe histogram: the callback fires on any
 * CPU concurrently and in atomic context. */
static atomic64_t hit_count = ATOMIC64_INIT(0);

/* ---------------------------------------------------------------------------
 * The ftrace callback. Modern signature (kernel >= 5.11):
 *   ip        — address of the traced function (its entry).
 *   parent_ip — return address, i.e. who called it.
 *   op        — the ftrace_ops that owns this callback (lets one func serve
 *               several ops; we have one).
 *   fregs     — an opaque register bundle. With FTRACE_OPS_FL_SAVE_REGS set,
 *               ftrace_get_regs(fregs) hands back a full struct pt_regs.
 *
 * RECURSION IS THE TRAP TO AVOID
 * ------------------------------
 * Our handler calls into the kernel (printk, user-copy helpers). If any of
 * those were themselves traced, they would re-enter this callback, which would
 * call them again... an unbounded recursion that overflows the stack instantly.
 * ftrace_test_recursion_trylock() sets a per-CPU "I am already inside a tracer"
 * bit and returns < 0 if it was already set. We bail out in that case. This is
 * the required idiom for any non-trivial ftrace_ops callback.
 * --------------------------------------------------------------------------- */
static void notrace openat_ftrace_cb(unsigned long ip, unsigned long parent_ip,
				     struct ftrace_ops *op,
				     struct ftrace_regs *fregs)
{
	struct pt_regs *regs;
	const char __user *ufname;
	char buf[64];
	long n;
	int bit;

	/* Take the recursion guard FIRST, before touching anything traceable. A
	 * negative return means we are re-entering; get out without doing work. */
	bit = ftrace_test_recursion_trylock(ip, parent_ip);
	if (bit < 0)
		return;

	/* Pull the saved registers. Without FTRACE_OPS_FL_SAVE_REGS this could be
	 * NULL, so we check — dereferencing a NULL pt_regs in the kernel is an
	 * immediate oops. */
	regs = ftrace_get_regs(fregs);
	if (!regs)
		goto out;

	atomic64_inc(&hit_count);

	/* arg1 = filename (user pointer). Same *_nofault rule as the kprobe: we
	 * are in atomic context and must not fault/sleep, so copy without a fault
	 * handler and accept -EFAULT for a non-resident page. */
	ufname = (const char __user *)regs_get_kernel_argument(regs, 1);
	n = strncpy_from_user_nofault(buf, ufname, sizeof(buf));
	if (n < 0)
		strscpy(buf, "<unavailable>", sizeof(buf));

	printk_ratelimited(KERN_INFO
		"ftrace_tracer: %s entered from %pS, file=\"%s\"\n",
		symbol, (void *)parent_ip, buf);

out:
	/* Release the guard with the exact bit we took. Balanced lock/unlock is
	 * mandatory: leaking the bit would permanently wedge tracing on this CPU. */
	ftrace_test_recursion_unlock(bit);
}

/* The ftrace_ops that ties our callback to a filter set of functions.
 *   .func  — the callback above.
 *   .flags — SAVE_REGS: give the callback a full pt_regs (needs
 *            CONFIG_DYNAMIC_FTRACE_WITH_REGS). Without it we'd get no args.
 * We intentionally do NOT set FTRACE_OPS_FL_RECURSION (which would mean "I
 * handle recursion myself and want the fast path"): we DO handle it, via the
 * trylock above, but keeping the flag unset also keeps ftrace's own safety net,
 * which is the conservative choice for a teaching module. */
static struct ftrace_ops ops = {
	.func  = openat_ftrace_cb,
	.flags = FTRACE_OPS_FL_SAVE_REGS,
};

static int __init ftrace_tracer_init(void)
{
	int ret;

	/* ftrace_set_filter() resolves the symbol NAME to its ftrace record for
	 * us — no kallsyms_lookup_name(), no manual address math. The last arg
	 * `reset=1` clears any prior filter so `ops` matches exactly this one
	 * function. It matches against the set of functions ftrace knows are
	 * patch-able (i.e. have the fentry nop); a typo or an un-traceable symbol
	 * yields an empty set and register_ftrace_function() then hooks nothing. */
	ret = ftrace_set_filter(&ops, symbol, strlen(symbol), 1);
	if (ret) {
		pr_err("ftrace_tracer: ftrace_set_filter(%s) failed: %d\n",
		       symbol, ret);
		return ret;
	}

	/* Arm it: patch the fentry nop in `symbol` into a call to our trampoline.
	 * After this returns, every entry to the function runs openat_ftrace_cb. */
	ret = register_ftrace_function(&ops);
	if (ret) {
		pr_err("ftrace_tracer: register_ftrace_function failed: %d\n", ret);
		/* Clear the filter we set so `ops` is left in a clean state. */
		ftrace_set_filter(&ops, NULL, 0, 1);
		return ret;
	}

	pr_info("ftrace_tracer: hooked '%s' via ftrace_ops (fentry)\n", symbol);
	return 0;
}

static void __exit ftrace_tracer_exit(void)
{
	/* Unregister patches the call site back to a nop and, importantly,
	 * synchronizes RCU/tracing so no CPU is still inside our callback before
	 * returning. Only then is it safe for the module text to be freed. */
	unregister_ftrace_function(&ops);
	ftrace_set_filter(&ops, NULL, 0, 1);  /* drop the filter set             */

	pr_info("ftrace_tracer: '%s' entered %lld times; unhooked\n",
		symbol, (long long)atomic64_read(&hit_count));
}

module_init(ftrace_tracer_init);
module_exit(ftrace_tracer_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("low-level-linux-lab");
MODULE_DESCRIPTION("ftrace_ops/fentry entry hook variant of the function tracer");
