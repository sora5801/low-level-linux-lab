// SPDX-License-Identifier: GPL-2.0
/* ===========================================================================
 * detector.c — BLUE TEAM: a kernel-integrity checker for LKM rootkits.
 * ===========================================================================
 *
 * This is the half the lab is really about. A rootkit's job is to lie to you;
 * a detector's job is to find a source of truth the rootkit did not think to
 * edit, and diff it against the lie. This module builds a KNOWN-GOOD BASELINE
 * of kernel state at load time and re-checks it on demand, reporting anything
 * that drifted. Read /proc/rkdetect to run the checks:
 *
 *     # cat /proc/rkdetect
 *
 * WHAT IT CHECKS (and which attack each check catches)
 * ----------------------------------------------------
 *   [A] Syscall-table fingerprint.  We FNV-hash the whole sys_call_table at load
 *       and re-hash on demand. A mismatch means an entry pointer was rewritten —
 *       the classic "overwrite sys_call_table[__NR_x]" attack. region_first_diff
 *       then localizes WHICH syscall number changed.  (See asm/demo.c: this
 *       hash+diff is the routine we extracted to annotated assembly.)
 *
 *   [B] Syscall-table bounds.  Every legitimate entry points into the core
 *       kernel .text segment. We flag any entry that points OUTSIDE [_stext,
 *       _etext) — e.g. into a module's memory. This catches a table hook even if
 *       it was installed BEFORE we loaded (check [A] can't, since our baseline
 *       would already contain it).
 *
 *   [C] Function-prologue integrity.  For a watch-list of commonly hooked
 *       functions (getdents64, kill, tcp4_seq_show, ...) we do two things:
 *         C1) fingerprint the first bytes at load and re-check (catches an
 *             ftrace/inline hook installed after us), and
 *         C2) decode the fentry site NOW and report if it has been redirected to
 *             a trampoline OUTSIDE kernel text (catches an ftrace hook installed
 *             before us — this is exactly what ../rootkit.c leaves behind).
 *
 *   [D] Process cross-view.  We print the authoritative PID list from the
 *       scheduler's task list (for_each_process). A DKOM/getdents rootkit can
 *       hide a PID from `ls /proc` and `ps`, but not from the task list; compare
 *       this to `ls /proc` to reveal a hidden process.
 *
 * HONEST SCOPE (teaching core): [A] and [C1] are baseline-relative — they catch
 * tampering that happens AFTER this module loads, so load the detector FIRST on
 * a clean VM. [B] and [C2] are absolute (baseline-free) and catch some
 * pre-existing hooks, at the cost of possible false positives from *legitimate*
 * ftrace users (a live tracer also redirects an fentry site). A production tool
 * would additionally walk ftrace's own ops list and validate against on-disk
 * symbol hashes; see "Going further" in README.md.
 * ===========================================================================
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>            /* kmalloc_array / kfree                    */
#include <linux/proc_fs.h>        /* proc_create, struct proc_ops            */
#include <linux/seq_file.h>       /* seq_printf report                       */
#include <linux/kprobes.h>        /* kallsyms bootstrap                      */
#include <linux/sched.h>          /* struct task_struct                      */
#include <linux/sched/signal.h>   /* for_each_process                        */
#include <linux/uaccess.h>        /* copy_from_kernel_nofault                */
#include <linux/version.h>
#include <asm/unistd.h>           /* NR_syscalls on x86-64                    */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("low-level-linux-lab");
MODULE_DESCRIPTION("Educational LKM-rootkit integrity detector (blue team)");
MODULE_VERSION("0.1");

/* NR_syscalls is provided by <asm/unistd.h> on x86-64. Guard just in case a
 * given arch/config doesn't expose it; 512 is a safe upper bound (real tables
 * are ~450). Over-counting only risks reading a few adjacent .rodata words,
 * which we handle by skipping obviously-non-text entries in check [B]. */
#ifndef NR_syscalls
#define NR_syscalls 512
#endif

/* Bytes of each watched function we fingerprint / scan for a redirected call. */
#define PROLOGUE_LEN 32

/* =====================================================================
 * PURE-LOGIC CORE — the same routine that asm/demo.c extracts for the
 * assembly deliverable. Kept here in kernel form so the detector is
 * self-contained. See asm/demo.annotated.s for the instruction-level tour.
 * ===================================================================== */

/* FNV-1a 64-bit hash of a byte region. Non-cryptographic on purpose: we only
 * need to notice a changed word in a small table on a kernel-safe hot path. */
static u64 fnv1a64(const u8 *data, size_t len)
{
	u64 hash = 0xcbf29ce484222325ULL;   /* FNV-64 offset basis              */
	size_t i;

	for (i = 0; i < len; i++) {
		hash ^= (u64)data[i];       /* mix in the next byte...          */
		hash *= 0x100000001b3ULL;   /* ...then diffuse it via the prime */
	}
	return hash;
}

/* First byte offset where a[] and b[] differ, or -1 if identical. Turns "the
 * table hash changed" into "syscall number offset/8 changed." */
static long region_first_diff(const u8 *a, const u8 *b, size_t len)
{
	size_t i;

	for (i = 0; i < len; i++)
		if (a[i] != b[i])
			return (long)i;
	return -1;
}

/* =====================================================================
 * State captured at module load (the "known good" baseline).
 * ===================================================================== */

typedef unsigned long (*kallsyms_lookup_name_t)(const char *name);
static kallsyms_lookup_name_t kln;         /* resolved once at init          */

static unsigned long **sys_call_table;     /* the live table                 */
static unsigned long *baseline_table;      /* our load-time copy of it       */
static u64 baseline_table_fp;              /* fingerprint of that copy       */

static unsigned long text_start, text_end; /* [_stext, _etext): kernel .text */

/* A function we expect a rootkit to hook. We record its address and the
 * fingerprint of its prologue at load; check [C1] re-fingerprints later. */
struct watched_fn {
	const char *name;
	unsigned long addr;
	u64 prologue_fp;    /* fnv1a64 of the first PROLOGUE_LEN bytes at load */
	bool resolved;
};

/* The watch-list. These are the syscalls/handlers rootkits classically hook to
 * hide files, processes, module presence, and network connections. */
static struct watched_fn watched[] = {
	{ .name = "__x64_sys_getdents64" },  /* file & /proc (PID) hiding       */
	{ .name = "__x64_sys_getdents"   },  /* legacy readdir path             */
	{ .name = "__x64_sys_kill"       },  /* signal interception / rootshell */
	{ .name = "__x64_sys_read"       },  /* content filtering               */
	{ .name = "tcp4_seq_show"        },  /* /proc/net/tcp port hiding       */
};

/* ---------------------------------------------------------------------------
 * Safe kernel read. copy_from_kernel_nofault returns 0 on success and -EFAULT
 * if `src` faults, so we can probe possibly-tampered text without oopsing. The
 * function was renamed from probe_kernel_read() in 5.8; handle both.
 * --------------------------------------------------------------------------- */
static int rk_read_kernel(void *dst, const void *src, size_t n)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0)
	return copy_from_kernel_nofault(dst, src, n);
#else
	return probe_kernel_read(dst, src, n);
#endif
}

/* Bootstrap kallsyms_lookup_name via a throwaway kprobe (it is un-exported
 * since 5.7 — same technique the rootkit uses, documented in ftrace_helper.h). */
static int resolve_kallsyms(void)
{
	struct kprobe kp = { .symbol_name = "kallsyms_lookup_name" };
	int ret = register_kprobe(&kp);

	if (ret < 0)
		return ret;
	kln = (kallsyms_lookup_name_t)kp.addr;
	unregister_kprobe(&kp);
	return kln ? 0 : -ENOENT;
}

/* Is `addr` inside the core kernel .text segment? Legit syscall entries and
 * un-redirected fentry targets are; a hook trampoline in module/vmalloc space
 * is not. This one predicate powers checks [B] and [C2]. */
static bool in_kernel_text(unsigned long addr)
{
	return addr >= text_start && addr < text_end;
}

/* =====================================================================
 * The checks. Each appends a human-readable report line via seq_file and
 * returns the number of anomalies it found.
 * ===================================================================== */

/* [A] Re-hash the live syscall table and compare to the baseline fingerprint.
 * On mismatch, localize the first changed entry with region_first_diff. */
static int check_syscall_table_fp(struct seq_file *m)
{
	u64 live_fp = fnv1a64((const u8 *)sys_call_table,
			      NR_syscalls * sizeof(unsigned long));

	/* A single 64-bit compare vets the whole table: if even one of the ~450
	 * entry pointers was overwritten, the FNV hash avalanches to a different
	 * value. Cheap enough to run on every read of /proc/rkdetect. */
	if (live_fp == baseline_table_fp) {
		seq_printf(m, "[ OK ] [A] syscall table fingerprint unchanged (0x%016llx)\n",
			   live_fp);
		return 0;
	}

	/* Something moved. Diff the raw bytes to find the first changed word. */
	{
		long off = region_first_diff((const u8 *)baseline_table,
					     (const u8 *)sys_call_table,
					     NR_syscalls * sizeof(unsigned long));
		long nr = off / (long)sizeof(unsigned long);

		seq_printf(m, "[WARN] [A] syscall table CHANGED: baseline=0x%016llx live=0x%016llx\n",
			   baseline_table_fp, live_fp);
		if (off >= 0)
			seq_printf(m, "         first changed entry: __NR_%ld  (was %pS, now %pS)\n",
				   nr, (void *)baseline_table[nr],
				   (void *)sys_call_table[nr]);
	}
	return 1;
}

/* [B] Every entry must point into kernel .text. Flag out-of-text entries and
 * name the owning module if there is one (a strong sign of a table hook). */
static int check_syscall_table_bounds(struct seq_file *m)
{
	int anomalies = 0;
	int i;

	for (i = 0; i < NR_syscalls; i++) {
		unsigned long entry = (unsigned long)sys_call_table[i];
		struct module *owner;

		if (!entry)
			continue;                   /* holes exist; skip NULLs      */
		if (in_kernel_text(entry))
			continue;                   /* normal: points into .text    */

		/* Out of .text. If a module owns it, that is almost certainly a
		 * hook; if nothing owns it, it may be an over-read past the real
		 * table end (see the NR_syscalls note) — report but stay calm. */
		owner = __module_address(entry);
		anomalies++;
		seq_printf(m, "[WARN] [B] __NR_%d entry 0x%lx is OUTSIDE kernel .text%s%s\n",
			   i, entry,
			   owner ? " — owned by module " : "",
			   owner ? owner->name : "");
	}
	if (!anomalies)
		seq_printf(m, "[ OK ] [B] all %d syscall entries point into kernel .text\n",
			   NR_syscalls);
	return anomalies;
}

/* [C2] Scan a function's prologue for a `call rel32` (opcode 0xe8) whose target
 * lands OUTSIDE kernel text — the signature of an ftrace hook whose fentry site
 * was redirected to a dedicated trampoline in module space. Returns 1 if found.
 *
 * The decode: at the 0xe8 byte, the next 4 bytes are a little-endian signed
 * displacement; the call target is (address_of_next_insn + disp32), i.e.
 * (site + 1 + 4 + disp32). This is the x86 relative-call encoding, and reading
 * it by hand is the whole point of a low-level lab. */
static int check_prologue_fentry(struct seq_file *m, struct watched_fn *w,
				 const u8 *buf)
{
	int i;

	for (i = 0; i + 5 <= PROLOGUE_LEN; i++) {
		s32 disp;
		unsigned long site, target;

		if (buf[i] != 0xe8)             /* not a relative CALL          */
			continue;

		/* Assemble the signed 32-bit displacement from 4 LE bytes. */
		disp = (s32)((u32)buf[i + 1]        |
			     ((u32)buf[i + 2] << 8)  |
			     ((u32)buf[i + 3] << 16) |
			     ((u32)buf[i + 4] << 24));
		site   = w->addr + i;           /* address of the 0xe8 byte     */
		target = site + 5 + disp;       /* call destination             */

		if (!in_kernel_text(target)) {
			struct module *owner = __module_address(target);

			seq_printf(m, "[WARN] [C2] %s fentry redirected: call at +%d -> 0x%lx (%pS)%s%s\n",
				   w->name, i, target, (void *)target,
				   owner ? " in module " : " (out of .text)",
				   owner ? owner->name : "");
			return 1;
		}
	}
	return 0;
}

/* [C1]+[C2] over the whole watch-list. */
static int check_watched_functions(struct seq_file *m)
{
	int anomalies = 0;
	size_t k;

	for (k = 0; k < ARRAY_SIZE(watched); k++) {
		struct watched_fn *w = &watched[k];
		u8 buf[PROLOGUE_LEN];
		u64 fp;

		if (!w->resolved)
			continue;                   /* symbol absent on this kernel */

		if (rk_read_kernel(buf, (void *)w->addr, PROLOGUE_LEN)) {
			seq_printf(m, "[WARN] [C] %s: prologue unreadable (faulted)\n",
				   w->name);
			anomalies++;
			continue;
		}

		/* C1: baseline-relative prologue integrity. A changed hash means
		 * the first PROLOGUE_LEN bytes were patched since load. We kept
		 * only the hash (not the original bytes), so we report the drift;
		 * C2 below usually pinpoints exactly where the redirect goes. */
		fp = fnv1a64(buf, PROLOGUE_LEN);
		if (fp != w->prologue_fp) {
			seq_printf(m, "[WARN] [C1] %s prologue CHANGED since load (fp 0x%016llx -> 0x%016llx)\n",
				   w->name, w->prologue_fp, fp);
			anomalies++;
		}

		/* C2: absolute fentry-redirect scan (baseline-free). */
		anomalies += check_prologue_fentry(m, w, buf);
	}

	if (!anomalies)
		seq_printf(m, "[ OK ] [C] watched-function prologues intact, no fentry redirects\n");
	return anomalies;
}

/* [D] Print the scheduler's authoritative PID list for a cross-view diff vs
 * `ls /proc`. rcu_read_lock() protects the task list while we walk it. */
static void report_process_crossview(struct seq_file *m)
{
	struct task_struct *task;
	int count = 0;

	seq_printf(m, "[INFO] [D] authoritative PID list (compare to `ls /proc`):\n         ");
	/* for_each_process walks the scheduler's task list — the source of truth a
	 * getdents/DKOM rootkit hides FROM but cannot hide itself, since the kernel
	 * must keep every runnable task on this list to schedule it. rcu_read_lock
	 * pins the list so a task can't be freed out from under our walk. */
	rcu_read_lock();
	for_each_process(task) {
		seq_printf(m, "%d ", task->pid);
		count++;
	}
	rcu_read_unlock();
	seq_printf(m, "\n         %d tasks in the scheduler's list. A PID here that is\n"
		      "         MISSING from `ls /proc` is being hidden.\n", count);
}

/* /proc/rkdetect read handler: run every check and total the anomalies. */
static int rkdetect_show(struct seq_file *m, void *v)
{
	int total = 0;

	seq_printf(m, "== rkdetect: kernel integrity report ==\n");
	seq_printf(m, "kernel .text: [0x%lx, 0x%lx)  sys_call_table: 0x%lx  NR_syscalls: %d\n\n",
		   text_start, text_end, (unsigned long)sys_call_table, NR_syscalls);

	/* Skip [A]/[B] if we never located the table (hardened kernel without it
	 * in kallsyms); the other checks still run so the report stays useful. */
	if (sys_call_table) {
		total += check_syscall_table_fp(m);
		total += check_syscall_table_bounds(m);
	} else {
		seq_printf(m, "[SKIP] [A]/[B] sys_call_table was not resolvable at load\n");
	}
	total += check_watched_functions(m);
	seq_putc(m, '\n');
	report_process_crossview(m);

	seq_printf(m, "\n== %d anomal%s flagged ==\n",
		   total, total == 1 ? "y" : "ies");
	return 0;
}

static int rkdetect_open(struct inode *inode, struct file *file)
{
	return single_open(file, rkdetect_show, NULL);
}

/* struct proc_ops replaced file_operations for /proc handlers in 5.6. */
static const struct proc_ops rkdetect_pops = {
	.proc_open    = rkdetect_open,
	.proc_read    = seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
};

/* =====================================================================
 * Baseline capture at load, then expose /proc/rkdetect.
 * ===================================================================== */
static int __init detector_init(void)
{
	size_t k;
	int err;

	err = resolve_kallsyms();
	if (err) {
		pr_err("rkdetect: cannot resolve kallsyms_lookup_name: %d\n", err);
		return err;
	}

	/* Kernel .text bounds. _stext/_etext are linker symbols not exported to
	 * modules, so we resolve them by name — the same lookup an attacker uses,
	 * turned to defensive ends. */
	text_start = kln("_stext");
	text_end   = kln("_etext");
	if (!text_start || !text_end) {
		pr_err("rkdetect: could not resolve _stext/_etext\n");
		return -ENOENT;
	}

	/* Locate the live syscall table. On some hardened kernels this symbol is
	 * absent from kallsyms (needs CONFIG_KALLSYMS_ALL); then checks [A]/[B]
	 * are skipped. That is an honest limitation, reported below. */
	sys_call_table = (unsigned long **)kln("sys_call_table");
	if (sys_call_table) {
		/* Take a private, immutable copy of the table. We diff the LIVE
		 * table against this copy, so it must live in memory the attacker
		 * has no reason to touch — our own kmalloc'd buffer, not a second
		 * pointer INTO the table (which a hook would change too). */
		baseline_table = kmalloc_array(NR_syscalls,
					       sizeof(unsigned long), GFP_KERNEL);
		if (!baseline_table)
			return -ENOMEM;
		memcpy(baseline_table, sys_call_table,
		       NR_syscalls * sizeof(unsigned long));
		/* Fold the copy down to one 64-bit fingerprint for the fast path. */
		baseline_table_fp = fnv1a64((const u8 *)baseline_table,
					    NR_syscalls * sizeof(unsigned long));
	} else {
		pr_warn("rkdetect: sys_call_table not in kallsyms; [A]/[B] disabled\n");
	}

	/* Resolve each watched function and fingerprint its prologue NOW, while we
	 * still trust the kernel — this snapshot is the yardstick for check [C1]. */
	for (k = 0; k < ARRAY_SIZE(watched); k++) {
		u8 buf[PROLOGUE_LEN];

		watched[k].addr = kln(watched[k].name);
		if (!watched[k].addr)
			continue;                   /* not present on this kernel   */
		if (rk_read_kernel(buf, (void *)watched[k].addr, PROLOGUE_LEN))
			continue;
		watched[k].prologue_fp = fnv1a64(buf, PROLOGUE_LEN);
		watched[k].resolved = true;
	}

	if (!proc_create("rkdetect", 0444, NULL, &rkdetect_pops)) {
		kfree(baseline_table);
		return -ENOMEM;
	}

	pr_info("rkdetect: baseline captured. read /proc/rkdetect to run checks.\n");
	return 0;
}

static void __exit detector_exit(void)
{
	remove_proc_entry("rkdetect", NULL);
	kfree(baseline_table);
	pr_info("rkdetect: unloaded\n");
}

module_init(detector_init);
module_exit(detector_exit);
