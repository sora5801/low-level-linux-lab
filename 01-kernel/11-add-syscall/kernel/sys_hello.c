// SPDX-License-Identifier: GPL-2.0
/* ===========================================================================
 * kernel/sys_hello.c — the implementation of a brand-new Linux system call.
 * ===========================================================================
 *
 * WHERE THIS FILE LIVES
 * ---------------------
 * Copy this file into the ROOT of a Linux kernel source tree as
 *     kernel/sys_hello.c
 * and wire it in with the three edits in ../patches/ (syscall table, prototype,
 * and kernel/Makefile). Then rebuild the kernel and boot it in QEMU. You cannot
 * add a syscall from a loadable module: the syscall dispatch table
 * (`sys_call_table[]`) is a fixed array baked into the kernel image at build
 * time, so a new entry means recompiling the kernel — that is the whole point
 * of this project, and why there is no `obj-m` here.
 *
 * WHAT A SYSCALL *IS*
 * -------------------
 * A system call is the one sanctioned doorway from unprivileged userspace (ring
 * 3) into the kernel (ring 0). On x86-64 userspace loads the call NUMBER into
 * %rax, the arguments into %rdi,%rsi,%rdx,%r10,%r8,%r9, and executes the
 * `syscall` instruction. The CPU jumps to the kernel's `entry_SYSCALL_64`
 * trampoline, which looks up `sys_call_table[nr]` and calls the C function
 * registered there. THIS FILE provides that C function for our new number.
 *
 * The kernel↔user boundary is a TRUST boundary. Userspace pointers are
 * completely untrusted: they may be NULL, point into the kernel, point at an
 * unmapped page, or be flipped to a hostile value by another thread the instant
 * after we check them. Therefore the kernel must NEVER dereference a user
 * pointer directly — it must go through copy_to_user()/copy_from_user(), which
 * perform the access with fault-handling armed so a bad address returns an error
 * instead of crashing the kernel. Getting this wrong is how CVEs are born.
 * ===========================================================================
 */

/* linux/syscalls.h gives us the SYSCALL_DEFINEn() macro family and the
 * `asmlinkage long sys_*` prototype conventions. It is the single most
 * important include for a file that defines a syscall. */
#include <linux/syscalls.h>
/* linux/uaccess.h declares copy_to_user()/copy_from_user() and the __user
 * sparse annotation. Anything that crosses the user/kernel memory boundary
 * needs this header. */
#include <linux/uaccess.h>
/* linux/errno.h: the negative error codes (-EFAULT, -EINVAL, ...) that a
 * syscall returns to signal failure. The convention is "return -Efoo"; glibc
 * turns a small-negative return into errno for the caller. */
#include <linux/errno.h>
/* linux/kernel.h: min(), and general kernel helpers. */
#include <linux/kernel.h>
/* linux/printk.h: pr_info() and friends, for the kernel log (dmesg). */
#include <linux/printk.h>
/* linux/types.h: size_t and the fixed-width kernel types. */
#include <linux/types.h>

/* ---------------------------------------------------------------------------
 * SYSCALL_DEFINE2(hello, char __user *, buf, size_t, len)
 *
 * Read that macro as: "define a 2-argument syscall named hello whose arguments
 * are (char __user *buf, size_t len)". The trailing digit MUST equal the number
 * of arguments; each argument is written as a (type, name) PAIR because the
 * macro pastes them into a generated prototype.
 *
 * Why a macro instead of a plain function? Since Linux 4.17 every x86-64 syscall
 * is reached through a thin wrapper that takes a single `struct pt_regs *` (the
 * saved userspace register file) and unpacks the arguments from it. That change
 * closed a class of Spectre-v1 gadgets where the CPU could speculate on
 * attacker-controlled register values leaking past the syscall boundary.
 * SYSCALL_DEFINE2 expands to THREE functions so you don't have to see any of it:
 *
 *   __x64_sys_hello(struct pt_regs *regs)   // the ABI entry the table points at;
 *                                            // extracts di/si from *regs ...
 *   __se_sys_hello(long, long)              // sign-extension shim, then calls ...
 *   __do_sys_hello(char __user *, size_t)   // <-- THE BODY YOU WRITE BELOW.
 *
 * The name in arch/x86/entry/syscalls/syscall_64.tbl is written as `sys_hello`;
 * the build machinery adds the `__x64_` prefix to reach __x64_sys_hello. That is
 * why the .tbl entry point column can stay the historical `sys_hello` spelling.
 * --------------------------------------------------------------------------- */
SYSCALL_DEFINE2(hello, char __user *, buf, size_t, len)
{
	/* The payload. `static const` so it lives once in the kernel's read-only
	 * data, not rebuilt on the stack every call. sizeof() on a char[] literal
	 * INCLUDES the trailing NUL the compiler appends, so `glen` counts the
	 * '\n' AND the '\0' — we deliberately copy the NUL so userspace receives a
	 * ready-to-print C string. */
	static const char greeting[] = "Hello from a real Linux syscall!\n";
	size_t glen = sizeof(greeting);

	/* One line in the kernel log per call. KERN_INFO-level. Useful the first
	 * time you boot the patched kernel: `dmesg | tail` proves the call landed.
	 * Real hot-path syscalls do NOT printk on every invocation (it is slow and
	 * floods the log) — this is here purely because it is a teaching syscall. */
	pr_info("sys_hello: called with buf=%p len=%zu\n", buf, len);

	/* -------- Argument validation, BEFORE touching user memory ----------
	 * A robust syscall checks its arguments first. A zero-length buffer can
	 * hold nothing, so there is nothing sensible to do — reject it with
	 * -EINVAL (invalid argument). Returning early here also means the
	 * copy_to_user() below can assume len >= 1. */
	if (len == 0)
		return -EINVAL;

	/* -------- Clamp to the caller-provided size (THE key invariant) -----
	 * `len` is the size of the user's buffer, and it is controlled by
	 * untrusted userspace. We must NEVER write more than `len` bytes, or we
	 * smash memory the caller didn't offer us — a kernel-writes-past-buffer
	 * bug that becomes a privilege-escalation primitive. So the number of
	 * bytes we copy is min(greeting size, user buffer size). If the greeting
	 * doesn't fit, the user gets a truncated (possibly non-NUL-terminated)
	 * result and the returned count tells them how much we wrote — exactly the
	 * contract read(2) offers. */
	if (glen > len)
		glen = len;

	/* -------- Cross the boundary: copy_to_user(to, from, n) -------------
	 * This is the ONLY correct way to move bytes from kernel memory (greeting)
	 * into a userspace address (buf). Under the hood it flips on the CPU's
	 * fault-tolerant access mode (STAC/CLAC + the exception fixup tables) so
	 * that if `buf` is bad — unmapped, read-only, or a kernel address a
	 * malicious caller passed to trick us into writing there — the access
	 * FAULTS SAFELY and copy_to_user returns the count of bytes it could not
	 * copy instead of oopsing the kernel.
	 *
	 * Return-value semantics are the classic footgun: copy_to_user returns the
	 * number of bytes NOT copied. ZERO means complete success; any nonzero
	 * means the user address was (partially) unusable, which we surface as
	 * -EFAULT (bad address), the same errno a userspace write to a bad pointer
	 * would produce. We must test it — silently ignoring a short copy would
	 * hand userspace uninitialized or partial data. */
	if (copy_to_user(buf, greeting, glen))
		return -EFAULT;

	/* -------- Success return ---------------------------------------------
	 * Return the number of bytes we placed in the buffer, like read(2). The
	 * syscall return travels back in %rax; glibc's wrapper treats a value in
	 * the range [-4095, -1] as -errno and everything else as success, so a
	 * small non-negative byte count reads through cleanly as the return of
	 * syscall(__NR_hello, ...). */
	return (long)glen;
}
