/* ===========================================================================
 * ftrace_helper.h — install/remove a function hook using the ftrace subsystem.
 * ===========================================================================
 *
 * WHY ftrace (and not writing into the syscall table)?
 * ----------------------------------------------------
 * The oldest "rootkit" trick is to overwrite an entry in sys_call_table with
 * your own pointer. On a modern kernel that table is READ-ONLY (it lives in
 * .rodata and CONFIG_STRICT_KERNEL_RWX / CR0.WP enforce it), so you would first
 * have to disable write protection — loud, fragile, and trivially detected.
 * ftrace gives a cleaner primitive: the compiler already planted a patchable
 * NOP at the top of nearly every kernel function (the -pg / __fentry__ site),
 * and ftrace lets a module say "call me whenever execution reaches function X."
 * We use that to redirect X to our replacement.
 *
 * THE MECHANISM, END TO END
 * -------------------------
 *   1. ftrace patches function X's fentry NOP into a call to a trampoline.
 *   2. The trampoline calls OUR callback (fh_ftrace_thunk) with the saved
 *      register state, because we set FTRACE_OPS_FL_SAVE_REGS.
 *   3. Our callback rewrites the saved instruction pointer (regs->ip) to point
 *      at our replacement. When the trampoline returns, the CPU "resumes" at
 *      our function instead of the real X. Rewriting regs->ip is only allowed
 *      because we set FTRACE_OPS_FL_IPMODIFY — that flag is what turns ftrace
 *      from a passive tracer into a hook.
 *   4. Our replacement runs, then calls the saved `original` pointer to invoke
 *      the real X. That re-enters the trampoline, but now parent_ip is inside
 *      OUR module, so we DON'T redirect — otherwise we would recurse forever.
 *      That within_module() guard is the load-bearing anti-recursion invariant.
 *
 * WHY THE DETECTOR CARES
 * ----------------------
 * Every step above leaves a fingerprint: the fentry NOP at X becomes a call,
 * so the first bytes of X change. ../detector.c hashes those bytes at load time
 * and re-hashes on demand; this hook is exactly what it is built to catch. Red
 * and blue share this header so the two halves stay honest about the mechanism.
 *
 * This file is a heavily-commented distillation of the widely-used ftrace-hook
 * helper pattern (see the References in README.md). It is EDUCATIONAL and for a
 * throwaway VM only.
 * ===========================================================================
 */
#ifndef FTRACE_HELPER_H
#define FTRACE_HELPER_H

#include <linux/ftrace.h>     /* struct ftrace_ops, register_ftrace_function   */
#include <linux/linkage.h>    /* asmlinkage                                    */
#include <linux/version.h>    /* LINUX_VERSION_CODE, KERNEL_VERSION            */
#include <linux/kprobes.h>    /* kprobe trick to resolve kallsyms_lookup_name  */
#include <linux/module.h>     /* within_module, THIS_MODULE                    */
#include <linux/kallsyms.h>   /* kallsyms_lookup_name (via the kprobe below)   */

/* ---------------------------------------------------------------------------
 * kallsyms_lookup_name has been UN-exported to modules since Linux 5.7
 * (commit 0bd476e6c671). The blessed replacement is a one-shot kprobe: register
 * a kprobe on the symbol "kallsyms_lookup_name", read the resolved address out
 * of kp.addr, then unregister. A kprobe can attach to any kernel text symbol by
 * name, so this bootstraps us from "I know a name" to "I have the address" — the
 * exact capability the export removal was meant to make inconvenient.
 * --------------------------------------------------------------------------- */
typedef unsigned long (*kallsyms_lookup_name_t)(const char *name);

static kallsyms_lookup_name_t fh_kallsyms_lookup_name;

/* Resolve and cache kallsyms_lookup_name once. Returns 0 on success.
 * Call this before any fh_install_hook(). Safe to call multiple times. */
static int fh_init_kallsyms(void)
{
	struct kprobe kp = { .symbol_name = "kallsyms_lookup_name" };
	int ret;

	if (fh_kallsyms_lookup_name)
		return 0;                       /* already resolved                  */

	/* register_kprobe plants a breakpoint at the symbol and, as a side
	 * effect, fills kp.addr with the symbol's address. We never actually
	 * want the breakpoint to fire — we only want kp.addr — so we unregister
	 * immediately. This is the canonical post-5.7 bootstrap. */
	ret = register_kprobe(&kp);
	if (ret < 0)
		return ret;                     /* symbol missing / kprobes disabled */
	fh_kallsyms_lookup_name = (kallsyms_lookup_name_t)kp.addr;
	unregister_kprobe(&kp);
	return fh_kallsyms_lookup_name ? 0 : -ENOENT;
}

/* ---------------------------------------------------------------------------
 * struct ftrace_hook — one function we intercept. The caller statically fills
 * {name, function, original}; fh_install_hook fills {address, ops}.
 *   name     : symbol to hook, e.g. "__x64_sys_getdents64".
 *   function : our replacement (same prototype as the target).
 *   original : ADDRESS OF a function pointer where we stash the real target, so
 *              the replacement can still call through to it.
 * --------------------------------------------------------------------------- */
struct ftrace_hook {
	const char *name;
	void *function;
	void *original;

	unsigned long address;          /* resolved entry address of the target  */
	struct ftrace_ops ops;          /* ftrace registration state             */
};

/* Resolve hook->address and publish the original pointer for the replacement. */
static int fh_resolve_hook_address(struct ftrace_hook *hook)
{
	hook->address = fh_kallsyms_lookup_name(hook->name);
	if (!hook->address) {
		pr_debug("ftrace_helper: unresolved symbol: %s\n", hook->name);
		return -ENOENT;
	}
	/* Publish the real target so hook->function can call the original. We
	 * treat hook->original as "pointer to (function-pointer variable)". */
	*((unsigned long *)hook->original) = hook->address;
	return 0;
}

/* ---------------------------------------------------------------------------
 * fh_ftrace_thunk — the ftrace callback. `notrace` keeps ftrace from tracing
 * OUR callback (which would be immediate recursion). The 4th argument's type
 * changed in 5.11: it used to be `struct pt_regs *`, now it is the opaque
 * `struct ftrace_regs *` you unwrap with ftrace_get_regs(). We handle both.
 * --------------------------------------------------------------------------- */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
static void notrace fh_ftrace_thunk(unsigned long ip, unsigned long parent_ip,
				    struct ftrace_ops *ops,
				    struct ftrace_regs *fregs)
{
	struct pt_regs *regs = ftrace_get_regs(fregs);
	struct ftrace_hook *hook = container_of(ops, struct ftrace_hook, ops);
#else
static void notrace fh_ftrace_thunk(unsigned long ip, unsigned long parent_ip,
				    struct ftrace_ops *ops,
				    struct pt_regs *regs)
{
	struct ftrace_hook *hook = container_of(ops, struct ftrace_hook, ops);
#endif
	/* THE anti-recursion invariant: only redirect when the call did NOT come
	 * from inside our own module. When our replacement calls the original,
	 * parent_ip lands in our .text, so within_module() is true and we fall
	 * through, letting the REAL function run. Violate this and the first call
	 * to the original re-enters here, redirects again, and you livelock the
	 * CPU inside the trampoline — a classic ftrace-hook footgun. */
	if (!within_module(parent_ip, THIS_MODULE))
		regs->ip = (unsigned long)hook->function;
}

/* ---------------------------------------------------------------------------
 * fh_install_hook — resolve, configure ftrace_ops, and arm the hook.
 *
 * Flag choices (each is load-bearing):
 *   FTRACE_OPS_FL_SAVE_REGS : hand our thunk a full pt_regs. We need it both to
 *       read the target's arguments and to WRITE regs->ip.
 *   FTRACE_OPS_FL_IPMODIFY  : promise ftrace we will change regs->ip. Only one
 *       IPMODIFY user is allowed per function, which is why two hooks on the
 *       same target conflict — and why the detector can reason about it.
 *   FTRACE_OPS_FL_RECURSION(_SAFE) : we do our own recursion guard above, so we
 *       opt out of ftrace's. The flag was renamed in 5.11 (see the #if).
 * --------------------------------------------------------------------------- */
static int fh_install_hook(struct ftrace_hook *hook)
{
	int err;

	err = fh_resolve_hook_address(hook);
	if (err)
		return err;

	hook->ops.func = fh_ftrace_thunk;
	hook->ops.flags = FTRACE_OPS_FL_SAVE_REGS
			| FTRACE_OPS_FL_IPMODIFY
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
			| FTRACE_OPS_FL_RECURSION;
#else
			| FTRACE_OPS_FL_RECURSION_SAFE;
#endif

	/* Restrict this ftrace_ops to exactly one address — the function we want.
	 * Without this filter, ftrace would call us for EVERY traced function. */
	err = ftrace_set_filter_ip(&hook->ops, hook->address, 0, 0);
	if (err) {
		pr_debug("ftrace_helper: ftrace_set_filter_ip failed: %d\n", err);
		return err;
	}

	/* Arm it: from here on, calls to hook->address route through our thunk. */
	err = register_ftrace_function(&hook->ops);
	if (err) {
		pr_debug("ftrace_helper: register_ftrace_function failed: %d\n", err);
		/* Undo the filter so we leave no partial state behind. */
		ftrace_set_filter_ip(&hook->ops, hook->address, 1, 0);
		return err;
	}
	return 0;
}

/* Disarm a single hook: unregister first (stop new redirects), THEN drop the
 * filter. Order matters — reversing it would briefly leave an armed ops with no
 * filter. */
static void fh_remove_hook(struct ftrace_hook *hook)
{
	unregister_ftrace_function(&hook->ops);
	ftrace_set_filter_ip(&hook->ops, hook->address, 1, 0);
}

/* Convenience: install/remove an array of hooks, unwinding cleanly on error so
 * we never leave half the hooks armed if the Nth one fails. */
static int fh_install_hooks(struct ftrace_hook *hooks, size_t count)
{
	int err;
	size_t i;

	for (i = 0; i < count; i++) {
		err = fh_install_hook(&hooks[i]);
		if (err)
			goto unwind;
	}
	return 0;

unwind:
	while (i-- > 0)                 /* remove the ones we already armed       */
		fh_remove_hook(&hooks[i]);
	return err;
}

static void fh_remove_hooks(struct ftrace_hook *hooks, size_t count)
{
	size_t i;

	for (i = 0; i < count; i++)
		fh_remove_hook(&hooks[i]);
}

#endif /* FTRACE_HELPER_H */
