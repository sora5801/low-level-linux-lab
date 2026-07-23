// SPDX-License-Identifier: GPL-2.0
/* ===========================================================================
 * hello_lkm.c — a Loadable Kernel Module (LKM), done properly.
 * ===========================================================================
 *
 * This is the "hello world" of kernel programming, but written the way a real
 * driver is written rather than the three-line toy you usually see. The point
 * is to meet, in the smallest possible module, the machinery that EVERY Linux
 * driver depends on:
 *
 *   - module_init / module_exit  — how code enters and leaves the kernel
 *   - MODULE_LICENSE / *_AUTHOR / *_DESCRIPTION  — the module's metadata contract
 *   - module_param / module_param_cb  — tunables exposed under /sys/module/...
 *   - pr_info / pr_err  — logging at the correct printk severity levels
 *
 * WHERE THIS CODE RUNS. This is NOT a userspace program. After `insmod`, this
 * object is linked into the running kernel's address space and executes in
 * ring 0 with no libc, no stdio, no malloc-you-know, and no memory protection
 * between you and the rest of the machine. A wild pointer here panics the box,
 * not just the process. That is why the invariants in the comments below matter:
 * in userspace a mistake is a SIGSEGV; here it is a dead kernel.
 *
 * It CANNOT be built or loaded on the Windows host this repo lives on. Build it
 * inside a Linux VM (QEMU/KVM is ideal — a throwaway kernel you can crash). See
 * the README for the exact QEMU + insmod/rmmod workflow.
 * ===========================================================================
 */

/* <linux/module.h> pulls in the core LKM framework: the MODULE_* macros, the
 * module_init/module_exit registration hooks, and struct module itself. Any
 * file that wants to *be* a module includes this first. */
#include <linux/module.h>

/* <linux/kernel.h> gives us pr_info/pr_err, the printk() family, and helpers
 * like ARRAY_SIZE. In the kernel there is no <stdio.h>; printk is the only way
 * to emit text, and it writes to the kernel ring buffer (read via `dmesg`). */
#include <linux/kernel.h>

/* <linux/init.h> defines the __init and __exit section attributes. Code marked
 * __init is placed in a special .init.text section that the kernel FREES once
 * initialization finishes — so init-only code costs zero resident memory after
 * load. __exit code is discarded entirely if the module is built into the
 * kernel image (you cannot unload a built-in module, so its exit path is dead).*/
#include <linux/init.h>

/* <linux/moduleparam.h> declares module_param, module_param_cb, and
 * struct kernel_param_ops — the plumbing that projects a C variable onto a file
 * under /sys/module/<name>/parameters/. It is pulled in transitively by
 * module.h, but we include it explicitly because we lean on it heavily. */
#include <linux/moduleparam.h>

/* <linux/kstrtox.h> provides kstrtoint(): the *safe* string-to-integer parser
 * the kernel uses instead of the overflow-prone atoi/simple_strtol. We need it
 * to parse a value a user echoes into our sysfs parameter file. */
#include <linux/kstrtox.h>

/* <linux/sysfs.h> declares sysfs_emit(): the bounds-safe way to format into the
 * PAGE_SIZE buffer sysfs hands a read handler. It is normally pulled in
 * transitively, but we include it explicitly since count_get() depends on it. */
#include <linux/sysfs.h>

/* ---------------------------------------------------------------------------
 * MODULE METADATA
 * ---------------------------------------------------------------------------
 * These macros do NOT generate code; they emit strings into dedicated ELF
 * sections (.modinfo) that `modinfo hello_lkm.ko` reads and that the kernel
 * inspects at load time.
 */

/* MODULE_LICENSE is not decoration — it is load-bearing. The kernel reads this
 * string when the module loads. Anything the kernel does not recognize as a
 * GPL-compatible license sets the TAINT_PROPRIETARY_MODULE flag ("kernel
 * tainted"), which bug reporters and maintainers use to ignore your report.
 * More concretely: symbols exported with EXPORT_SYMBOL_GPL are *invisible* to a
 * non-GPL module — the linker will refuse to resolve them. "GPL v2" here keeps
 * us in the club so we can call GPL-only kernel APIs. */
MODULE_LICENSE("GPL v2");

/* Free-form metadata, surfaced by `modinfo`. Purely informational. */
MODULE_AUTHOR("low-level-linux-lab");
MODULE_DESCRIPTION("A properly-structured hello-world LKM: params, callbacks, printk levels");
MODULE_VERSION("1.0");

/* ---------------------------------------------------------------------------
 * PURE-LOGIC HELPERS
 * ---------------------------------------------------------------------------
 * These two functions contain NO kernel API calls. They are exactly the code
 * extracted (verbatim in spirit) into asm/demo.c so we can compile them to
 * standalone assembly on any host — kernel C cannot be compiled to .s without a
 * full kernel build tree. Keeping the "brain" of the module in dependency-free
 * helpers is also just good style: logic you can unit-test without a VM.
 */

/* The greeting count is a tunable, and a user can write ANY int into its sysfs
 * file. We refuse nonsense rather than clamp it silently, but we still expose
 * the clamp as the canonical "what is the legal range" predicate. */
#define COUNT_MIN 1
#define COUNT_MAX 10

/* clamp_count — force an arbitrary integer into [COUNT_MIN, COUNT_MAX].
 * Branchy on purpose so the generated assembly (see asm/) shows the compiler
 * turning these comparisons into conditional moves (cmov) at -O1/-O2. */
static int clamp_count(int count)
{
	if (count < COUNT_MIN)
		return COUNT_MIN;
	if (count > COUNT_MAX)
		return COUNT_MAX;
	return count;
}

/* fnv1a32 — the 32-bit FNV-1a hash. We use it purely as a cheap, deterministic
 * "tag" for a given `who` string so each greeting run carries a stable ID you
 * can grep for in dmesg. It is a textbook example of a hot pure-logic loop
 * (xor-then-multiply per byte), which makes its assembly instructive: watch how
 * the optimizer implements the ×16777619 multiply (clang keeps a single imul). */
static unsigned int fnv1a32(const char *s)
{
	/* FNV offset basis and prime — fixed constants from the FNV spec. */
	unsigned int h = 2166136261u;

	while (*s) {
		h ^= (unsigned char)*s++; /* mix in the next byte (unsigned to avoid
					   * sign-extension of bytes >= 0x80)      */
		h *= 16777619u;           /* diffuse: the FNV prime               */
	}
	return h;
}

/* ---------------------------------------------------------------------------
 * MODULE PARAMETER 1: `who` (a string) — the plain, classic form.
 * ---------------------------------------------------------------------------
 * `charp` is the kernel's "char pointer" parameter type: the module framework
 * owns a `char *` and, when set, points it at a kstrdup'd copy of the value.
 * We give it a compile-time default so the module works with no arguments.
 *
 * Storage note: a module parameter's backing variable must have static storage
 * duration (it lives for the whole module lifetime and is touched from sysfs
 * context), so it is a file-scope static, never a stack local.
 */
static char *who = "world";

/* module_param(name, type, perm):
 *   name — must match the C variable's name; the sysfs file takes this name.
 *   type — one of the framework's known types (int, uint, charp, bool, ...).
 *   perm — the mode bits of /sys/module/hello_lkm/parameters/who.
 *          0644 = owner rw, group/other r. A perm of 0 hides the file entirely.
 *          The build FAILS (BUILD_BUG) if you pass world-writable bits, because
 *          a globally writable kernel tunable is a security hole.
 * A `charp` is only meaningfully settable at load time (insmod hello_lkm.ko
 * who=sora); writing the sysfs file later just swaps the pointer, which is why
 * we keep it simple here and reserve the runtime-reactive path for `count`. */
module_param(who, charp, 0644);
MODULE_PARM_DESC(who, "Name to greet (string). Default: \"world\".");

/* ---------------------------------------------------------------------------
 * MODULE PARAMETER 2: `count` (an int) — the runtime-tunable, callback form.
 * ---------------------------------------------------------------------------
 * This is the "done properly" part. A plain `module_param(count, int, 0644)`
 * would let a user write any integer into the sysfs file with NO validation and
 * NO reaction — the value would just change silently. Real tunables need to
 * validate input and often act on the change. module_param_cb() lets us supply
 * our own get/set operations via a struct kernel_param_ops.
 */
static int count = 3;

/* `ready` gates the runtime "react to the change" behavior. It matters because
 * of a real init-ordering fact: when you pass count=N on the insmod line, the
 * module framework parses parameters — and therefore calls count_set() below —
 * BEFORE hello_lkm_init() runs. At that early moment `who` may not be parsed
 * yet and the module isn't really "up", so re-emitting the greeting from the
 * setter would be premature and could use a stale name. We flip `ready` to true
 * at the very end of init; until then, count_set() only validates and stores,
 * and init does the first greeting itself. This keeps load-time output clean and
 * runtime writes reactive. */
static bool ready;

/* count_set — invoked by the kernel every time `count` is assigned: once at
 * insmod (if count=N is passed) and again on every `echo N > .../parameters/
 * count` at runtime. THIS is the callback that "reacts to the change."
 *
 * Contract (struct kernel_param_ops::set):
 *   @val — the NUL-terminated string the user wrote (e.g. "5\n"). It is a
 *          kernel-owned buffer; we only read it.
 *   @kp  — describes the parameter; kp->arg points at our `count` variable.
 *   returns 0 on success, or a negative errno that propagates back to the
 *   write(2) syscall so the shell's `echo` reports the failure.
 *
 * We run in process context (the writing task), so sleeping/printk is fine. */
static int count_set(const char *val, const struct kernel_param *kp)
{
	int new_count;
	int ret;

	/* kstrtoint parses the whole string as a base-10 int, tolerating a
	 * trailing newline, and returns -EINVAL/-ERANGE on garbage or overflow.
	 * This is the safe replacement for atoi(): it never overflows silently
	 * and it reports *why* it failed. */
	ret = kstrtoint(val, 10, &new_count);
	if (ret < 0) {
		/* KERN_ERR level: an operator did something wrong and should see
		 * it. pr_err writes to the ring buffer at severity 3. */
		pr_err("hello_lkm: '%s' is not a valid integer\n", val);
		return ret; /* propagate -EINVAL; the echo fails, count unchanged */
	}

	/* Enforce our policy range. We REJECT out-of-range writes rather than
	 * clamp silently, so the operator gets honest feedback — but we phrase
	 * the legal bounds through clamp_count() so there is a single source of
	 * truth for the range (and so its logic shows up in the asm deliverable).*/
	if (clamp_count(new_count) != new_count) {
		pr_err("hello_lkm: count=%d out of range [%d,%d]\n",
		       new_count, COUNT_MIN, COUNT_MAX);
		return -EINVAL;
	}

	/* Commit the validated value. kp->arg is a void* aliasing our `count`;
	 * writing through it is how the framework expects a custom setter to
	 * store. (We could also assign `count = new_count;` directly since we
	 * own the variable, but going through kp->arg is the idiomatic pattern
	 * that also works for generic, reusable ops.) */
	*(int *)kp->arg = new_count;

	/* Only "react" once the module is fully initialized. During insmod the
	 * framework calls us before init (see the `ready` comment above), so we
	 * store silently and let init own the first greeting. */
	if (!ready)
		return 0;

	/* React to the change: re-emit the greeting so the effect is visible in
	 * dmesg immediately. KERN_INFO level (severity 6) — normal operation. */
	pr_info("hello_lkm: count updated to %d; greeting %s again:\n",
		new_count, who);
	{
		int i;
		unsigned int tag = fnv1a32(who);

		for (i = 0; i < new_count; i++)
			pr_info("hello_lkm: [%08x] Hello, %s! (%d/%d)\n",
				tag, who, i + 1, new_count);
	}

	return 0; /* success: the write(2) returns the byte count to the shell */
}

/* count_get — invoked when a user READS the sysfs file (cat .../count). We must
 * render the current value into @buffer, which is a single PAGE_SIZE scratch
 * page the kernel hands us, and return the number of bytes written (like a
 * snprintf). sysfs convention is to end with a newline.
 *
 * We use sysfs_emit(), the modern, overflow-safe helper: it knows @buffer is
 * page-aligned and PAGE_SIZE long, so it can never write past the page — the
 * kind of bounds guarantee that matters doubly in kernel space. */
static int count_get(char *buffer, const struct kernel_param *kp)
{
	return sysfs_emit(buffer, "%d\n", *(int *)kp->arg);
}

/* Bundle our two operations into the ops table the framework calls through.
 * `const` because it never changes and belongs in .rodata (a stray write to a
 * function-pointer table is a favorite exploit primitive; keeping it read-only
 * lets the kernel's memory protections catch that). */
static const struct kernel_param_ops count_ops = {
	.set = count_set,
	.get = count_get,
};

/* module_param_cb(name, ops, arg, perm): like module_param but routes get/set
 * through our `count_ops` instead of the framework's built-in int handlers.
 * `&count` becomes kp->arg inside the callbacks. 0644 exposes a readable +
 * owner-writable sysfs file, which is what makes runtime tuning possible. */
module_param_cb(count, &count_ops, &count, 0644);
MODULE_PARM_DESC(count, "Times to greet, validated to [1,10]; live-tunable via sysfs.");

/* ---------------------------------------------------------------------------
 * INIT: runs once, at `insmod` time.
 * ---------------------------------------------------------------------------
 * __init marks this for the freeable .init.text section (see init.h note up
 * top). The function's return value is the insmod contract: return 0 for
 * success (module stays loaded) or a negative errno for failure (module is torn
 * down and insmod prints the error). Returning nonzero is how a driver says
 * "the hardware isn't here, don't load me."
 */
static int __init hello_lkm_init(void)
{
	int i;
	unsigned int tag;

	/* Defensive validation of the load-time `count`. A user can pass
	 * count=999 on the insmod line; that path does NOT go through count_set
	 * (module_param_cb's setter only fires for values actually present on
	 * the command line, and older kernels' ordering quirks make belt-and-
	 * suspenders checking here the safe habit). If it's out of range, refuse
	 * to load — a driver should never come up in an invalid configuration. */
	if (clamp_count(count) != count) {
		pr_err("hello_lkm: refusing to load, count=%d out of [%d,%d]\n",
		       count, COUNT_MIN, COUNT_MAX);
		return -EINVAL; /* insmod fails with "Invalid argument" */
	}

	/* Tag this greeting run with a hash of `who`, so repeated loads with
	 * different names are easy to tell apart in a busy dmesg. */
	tag = fnv1a32(who);

	pr_info("hello_lkm: loaded. greeting %s %d time(s) [tag %08x]\n",
		who, count, tag);

	/* The actual "hello world", `count` times, at KERN_INFO. */
	for (i = 0; i < count; i++)
		pr_info("hello_lkm: [%08x] Hello, %s! (%d/%d)\n",
			tag, who, i + 1, count);

	/* Module is fully up: from here on, sysfs writes to `count` should react.
	 * This is the last thing init does, so no reader can observe a "live"
	 * module that hasn't finished its own setup. */
	ready = true;

	return 0; /* 0 == success: the module is now live */
}

/* ---------------------------------------------------------------------------
 * EXIT: runs once, at `rmmod` time.
 * ---------------------------------------------------------------------------
 * __exit marks this for a section dropped when the module is built statically
 * into the kernel (a built-in can't be removed, so its cleanup is unreachable).
 * The exit function takes no arguments and returns nothing: by the time it runs
 * the kernel has already checked the module's reference count is zero, so
 * teardown cannot fail in a way that keeps the module loaded.
 *
 * A real driver frees everything init allocated here — in strict reverse order
 * of acquisition — to avoid leaking kernel memory (which never gets reclaimed
 * until reboot). We allocated nothing, so we just say goodbye. */
static void __exit hello_lkm_exit(void)
{
	/* Stop reacting to any last-moment sysfs write racing with teardown. (A
	 * production driver would need real locking here, not just a flag — see
	 * the README's "Going further" note; this flag documents the intent.) */
	ready = false;
	pr_info("hello_lkm: unloaded. goodbye, %s.\n", who);
}

/* Register our two hooks. These macros place function pointers into the
 * module's init/exit tables so the loader (`insmod`) and unloader (`rmmod`)
 * know what to call. Without module_init the kernel would load the code but
 * never run any of it. */
module_init(hello_lkm_init);
module_exit(hello_lkm_exit);
