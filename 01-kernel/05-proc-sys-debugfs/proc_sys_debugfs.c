// SPDX-License-Identifier: GPL-2.0
/* ===========================================================================
 * proc_sys_debugfs.c — one driver's state, exported THREE different ways.
 * ===========================================================================
 *
 * WHAT THIS TEACHES
 * -----------------
 * The Linux kernel gives a driver three separate virtual filesystems to expose
 * its internal state to userspace, and choosing the wrong one is a real design
 * mistake that shows up in code review. This single module wires the *same*
 * piece of driver state into all three, side by side, so you can compare the
 * mechanics and — more importantly — the *social contract* of each:
 *
 *   /proc/psd_demo         (procfs)   — the legacy, human-oriented dump. We use
 *                                       a seq_file so a read of any size is safe
 *                                       even when the output is bigger than the
 *                                       reader's buffer. Historically process-
 *                                       centric (/proc/<pid>/...); driver state
 *                                       here is tolerated but discouraged today.
 *
 *   /sys/kernel/psd_demo/  (sysfs)    — the STABLE ABI surface. One value per
 *                                       file, machine-parseable, documented in
 *                                       Documentation/ABI/. This is what tools,
 *                                       udev rules and libraries are allowed to
 *                                       depend on. Breaking it breaks userspace.
 *
 *   /sys/kernel/debug/psd_demo/ (debugfs) — the UNSTABLE debugging surface. No
 *                                       ABI promise whatsoever: files may appear,
 *                                       change format, or vanish between kernel
 *                                       releases. Perfect for raw internals you
 *                                       do NOT want anyone building a tool on.
 *
 * The rule of thumb the code embodies:
 *   sysfs  = "one value per file, and I promise not to change it"
 *   proc   = "a formatted, possibly large, human report" (legacy for new state)
 *   debugfs= "throwaway knobs and dumps for kernel developers, no promises"
 *
 * WHERE THIS RUNS
 * ---------------
 * This is real kernel code. It compiles against Linux kernel headers with the
 * Kbuild system and loads with insmod. It CANNOT run on Windows and cannot be
 * built here on this host — do it inside a Linux/QEMU VM (see README.md). The
 * host-portable, compilable teaching artifact is asm/demo.c, which extracts the
 * pure-logic number formatting core so we can still show real annotated asm.
 *
 * MEMORY / CONCURRENCY MODEL
 * --------------------------
 * There is exactly one instance of this "device", so its state lives in a single
 * file-scope struct (psd) rather than being kmalloc'd per-open. That is an
 * honest simplification for a teaching module; a real multi-instance driver
 * would embed struct psd_state in its per-device object and pass it through
 * ->private_data / seq_file->private / the kobject container_of. The one shared
 * mutable region (the event ring + the label) is guarded by a spinlock; every
 * reader/writer path documents which lock it needs and why.
 * ===========================================================================
 */

#include <linux/module.h>      /* MODULE_*, module_init/exit, THIS_MODULE       */
#include <linux/kernel.h>      /* pr_info(), container_of()                     */
#include <linux/init.h>        /* __init / __exit section annotations           */
#include <linux/fs.h>          /* struct inode, struct file                     */
#include <linux/proc_fs.h>     /* proc_create(), proc_remove(), struct proc_ops */
#include <linux/seq_file.h>    /* seq_operations, seq_printf(), SEQ_START_TOKEN  */
#include <linux/kobject.h>     /* kobject, kobject_create_and_add()             */
#include <linux/sysfs.h>       /* kobj_attribute, sysfs_create_group, __ATTR    */
#include <linux/debugfs.h>     /* debugfs_create_dir/_u32/_atomic_t/_file       */
#include <linux/slab.h>        /* (not strictly needed here, but idiomatic)     */
#include <linux/atomic.h>      /* atomic_t, atomic_read/inc                      */
#include <linux/spinlock.h>    /* spinlock_t, spin_lock/unlock                   */
#include <linux/string.h>      /* strscpy()                                     */
#include <linux/jiffies.h>     /* jiffies — a monotonic-ish tick counter        */
#include <linux/kstrtox.h>     /* kstrtou32() for parsing user input safely     */
#include <linux/version.h>     /* LINUX_VERSION_CODE, for the proc_ops note     */

/* --------------------------------------------------------------------------- */
/* Tunable sizes. Small on purpose so the whole state fits on a screen.        */
/* --------------------------------------------------------------------------- */
#define PSD_NAME       "psd_demo"   /* the directory/file basename in all 3 FSes */
#define PSD_RING_SIZE  16           /* how many recent events we remember        */
#define PSD_LABEL_MAX  32           /* bytes for the user-settable label         */

/* ===========================================================================
 * THE DRIVER STATE — the single source of truth all three interfaces read.
 * ===========================================================================
 *
 * `hits` is an atomic_t rather than a plain int because the "trigger" path can
 * be entered concurrently from multiple CPUs (two processes writing the sysfs
 * file at once), and we increment it OUTSIDE the spinlock in one path to show
 * that atomics are the right tool for a lock-free counter. atomic_read/inc are
 * single-instruction (LOCK-prefixed) on x86-64, so no torn reads.
 *
 * `threshold` is a plain u32. It is deliberately exposed to debugfs_create_u32()
 * by ADDRESS (&psd.threshold), which reads/writes it with no locking at all —
 * that is fine for debugfs (a debugging knob) and is itself the lesson: debugfs
 * trades safety/stability for zero ceremony.
 *
 * The event ring (`ring`, `ring_count`, `dropped`) plus `label` are the only
 * fields that need mutual exclusion, because they are multi-word and a reader
 * (the /proc seq_file) walks them element by element. `lock` guards exactly
 * those. Invariant: never sleep while holding `lock` (it is a spinlock), and
 * the seq_file iterator below holds it across ->start .. ->stop, so ->show must
 * not sleep either — seq_printf() only formats into a preallocated page, so it
 * doesn't. Violate that and you deadlock or trigger "scheduling while atomic".
 * =========================================================================== */
struct psd_state {
	atomic_t     hits;                 /* total events recorded (lock-free)     */
	u32          threshold;            /* a config knob, shared by sysfs+debugfs */
	u32          dropped;              /* events lost because the ring was full  */
	unsigned int ring_count;           /* number of valid entries in ring[]      */
	u64          ring[PSD_RING_SIZE];  /* recent event timestamps (jiffies)      */
	char         label[PSD_LABEL_MAX]; /* a user-settable name, for demonstration */
	spinlock_t   lock;                 /* guards ring/ring_count/dropped/label    */
};

/* The single instance. `.lock` is initialised at load time (spin_lock_init). */
static struct psd_state psd = {
	.hits       = ATOMIC_INIT(0),
	.threshold  = 100,
	.label      = "default",
};

/* Handles we must remember so the exit path can tear everything down cleanly.
 * Leaking these on rmmod would leave dangling files pointing at freed code —
 * an instant kernel oops the next time someone reads them. */
static struct proc_dir_entry *psd_proc_entry;   /* /proc/psd_demo               */
static struct kobject        *psd_kobj;         /* /sys/kernel/psd_demo         */
static struct dentry         *psd_debug_dir;    /* /sys/kernel/debug/psd_demo   */

/* ---------------------------------------------------------------------------
 * psd_record_event — append "now" to the ring and bump the hit counter.
 *
 * Called from the sysfs "trigger" store path. We take `lock` because the ring
 * is multi-word state a concurrent /proc reader is walking; without it a reader
 * could observe ring_count incremented before ring[] was written (a torn view).
 * `hits` is atomic and updated outside the critical section on purpose, to make
 * the point that a pure counter does not need the heavier lock.
 * --------------------------------------------------------------------------- */
static void psd_record_event(struct psd_state *st)
{
	unsigned long flags;

	/* atomic_inc is a LOCK-prefixed RMW on x86-64: no lost updates even under
	 * concurrency, and no need to hold `st->lock` just for this. */
	atomic_inc(&st->hits);

	/* spin_lock_irqsave: disable local IRQs and take the lock. We use the
	 * _irqsave variant because a future version might record events from an
	 * interrupt handler; taking the plain spin_lock() in process context while
	 * an IRQ handler also grabs it would deadlock the CPU. Saving/restoring
	 * flags keeps this correct no matter who our caller is. */
	spin_lock_irqsave(&st->lock, flags);
	if (st->ring_count < PSD_RING_SIZE) {
		/* Linear, saturating log: we keep the FIRST PSD_RING_SIZE events, then
		 * count the rest as "dropped". A production driver would usually keep
		 * the most RECENT N with a circular head index; we chose the linear
		 * form so the seq_file iterator below stays about the seq_file contract
		 * and not about modular arithmetic. The `dropped` counter turns the
		 * lost data into a visible, honest number instead of silence. */
		st->ring[st->ring_count++] = get_jiffies_64();
	} else {
		st->dropped++;
	}
	spin_unlock_irqrestore(&st->lock, flags);
}

/* ===========================================================================
 * INTERFACE 1 — procfs via a seq_file iterator.
 * ===========================================================================
 *
 * WHY seq_file? A naive ->read() that snprintf()s everything into the user's
 * buffer is a classic kernel bug generator: if the output is larger than the
 * buffer, or the reader reads in small chunks (as `dd bs=1` or a slow pipe
 * does), you must correctly resume from an arbitrary byte offset. seq_file
 * solves this once, correctly: you describe your data as an ITERATOR (start,
 * next, stop, show) over logical records, and the seq_file core handles buffer
 * sizing, the 4 KiB page, overflow-and-retry, and partial reads. This is the
 * "safe large reads" property the spec asks us to demonstrate.
 *
 * THE ITERATOR CONTRACT (read this once and the four callbacks make sense):
 *   ->start(m, pos): position the cursor at logical record *pos. Return a
 *                    non-NULL "token" for that record, NULL to end. May return
 *                    the special SEQ_START_TOKEN to ask for a one-time header.
 *   ->show (m, v)  : format the record `v` into the seq buffer with seq_printf.
 *                    If the buffer is full it silently overflows; seq_file then
 *                    throws away this pass, allocates a bigger buffer, and calls
 *                    ->start again from the same *pos. So ->show MUST be a pure
 *                    function of `v` (no side effects) — it can be re-run.
 *   ->next (m, v, pos): advance *pos, return the next token or NULL.
 *   ->stop (m, v)  : called once when iteration ends OR pauses. The place to
 *                    drop whatever ->start acquired.
 *
 * LOCKING: we take psd.lock in ->start and drop it in ->stop, holding it across
 * the whole walk. That is safe here because the ring is tiny and ->show never
 * sleeps. For a large or sleepable dataset you would instead snapshot under the
 * lock in ->start, or use a sleepable mutex and an iterator that can sleep.
 * =========================================================================== */

static void *psd_seq_start(struct seq_file *m, loff_t *pos)
{
	/* Acquire the lock guarding the ring for the entire iteration. Paired with
	 * the spin_unlock in psd_seq_stop(). Note: plain spin_lock (not _irqsave)
	 * is fine on the read side here because we are always in process context
	 * (a syscall read) and no IRQ handler takes this lock in this teaching
	 * build; the write path's _irqsave is the belt-and-suspenders version. */
	spin_lock(&psd.lock);

	/* *pos == 0 is the very first call: ask seq to print a header row first by
	 * returning the SEQ_START_TOKEN sentinel. This is the exact idiom used by
	 * /proc/slabinfo and friends. */
	if (*pos == 0)
		return SEQ_START_TOKEN;

	/* Otherwise logical data row index = *pos - 1 (row 0 was "used up" by the
	 * header token). If we've walked past the valid entries, signal end. */
	if (*pos - 1 >= psd.ring_count)
		return NULL;

	/* Return a stable pointer to the record. seq_file treats this as an opaque
	 * token and just hands it back to ->show / ->next. */
	return &psd.ring[*pos - 1];
}

static void *psd_seq_next(struct seq_file *m, void *v, loff_t *pos)
{
	/* Advance the cursor. seq_file relies on us to move *pos forward; returning
	 * the same position forever would spin. */
	(*pos)++;

	if (*pos - 1 >= psd.ring_count)
		return NULL;                 /* no more records -> end iteration       */

	return &psd.ring[*pos - 1];
}

static void psd_seq_stop(struct seq_file *m, void *v)
{
	/* Release the lock taken in ->start. seq_file guarantees ->stop is called
	 * for every ->start, including on the terminating NULL and on error, so the
	 * lock can never leak. */
	spin_unlock(&psd.lock);
}

static int psd_seq_show(struct seq_file *m, void *v)
{
	u64 *slot;
	loff_t idx;

	/* The header pass. seq_printf appends to the seq buffer and returns void in
	 * modern kernels; if the buffer is full it sets an overflow flag and the
	 * core retries the whole pass with a bigger buffer — which is exactly why
	 * ->show must have no side effects. */
	if (v == SEQ_START_TOKEN) {
		seq_printf(m,
			   "# %s driver state (via /proc — seq_file)\n"
			   "# hits=%d threshold=%u dropped=%u label=\"%s\"\n"
			   "# %-4s  %-20s\n",
			   PSD_NAME,
			   atomic_read(&psd.hits), psd.threshold, psd.dropped,
			   psd.label,
			   "idx", "jiffies_at_event");
		return 0;
	}

	/* A data row. `v` is the &psd.ring[i] we returned; recover i by pointer
	 * arithmetic against the array base. This is why linear (non-circular)
	 * storage keeps the iterator clean. */
	slot = v;
	idx = slot - psd.ring;
	seq_printf(m, "  %-4lld  %20llu\n", idx, *slot);
	return 0;
}

/* Bundle the four callbacks. seq_open() below stores a pointer to this. */
static const struct seq_operations psd_seq_ops = {
	.start = psd_seq_start,
	.next  = psd_seq_next,
	.stop  = psd_seq_stop,
	.show  = psd_seq_show,
};

/* open() for the proc file: hand the seq core our iterator. seq_open allocates
 * the struct seq_file and wires file->private_data to it. From here on the
 * generic seq_read/seq_lseek/seq_release do all the heavy lifting. */
static int psd_proc_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &psd_seq_ops);
}

/* Since kernel 5.6, /proc files use `struct proc_ops` instead of the general
 * `struct file_operations`. This shrank the per-file footprint and decoupled
 * proc from the VFS op table. On a pre-5.6 kernel you would populate a
 * struct file_operations with .owner/.open/.read/.llseek/.release instead. */
static const struct proc_ops psd_proc_ops = {
	.proc_open    = psd_proc_open,   /* our seq_open wrapper                    */
	.proc_read    = seq_read,        /* generic: formats records into the user's
					  *          buffer, handling any read size */
	.proc_lseek   = seq_lseek,       /* generic: seek within the virtual file   */
	.proc_release = seq_release,     /* generic: free the struct seq_file       */
};

/* ===========================================================================
 * INTERFACE 2 — sysfs via a kobject + an attribute group.
 * ===========================================================================
 *
 * sysfs is the STABLE, one-value-per-file ABI. Each attribute below becomes a
 * single file under /sys/kernel/psd_demo/. The golden rules sysfs enforces by
 * convention:
 *   - one file exposes exactly one value (no multi-line tables — that's proc);
 *   - show() must emit at most PAGE_SIZE bytes (the buffer is one page);
 *   - the format is an ABI: once shipped, tools depend on it, so you don't get
 *     to change "42\n" into "value: 42\n" later.
 *
 * We attach the files to a kobject created under kernel_kobj, which is the
 * /sys/kernel/ directory. kobject_create_and_add() both allocates the kobject
 * (refcount = 1) and links it into sysfs. On teardown kobject_put() drops that
 * reference; when it hits zero the kobject and its sysfs dir are freed.
 * =========================================================================== */

/* show() for the read-only "hits" file. The signature (kobj, attr, buf) is the
 * kobj_attribute variant used for freestanding kobjects (as opposed to the
 * device_attribute variant used under struct device). We format ONE value plus
 * a newline, and return the byte count — sysfs uses that as the read length.
 * sysfs_emit() is the modern, bounds-checked helper that guarantees we never
 * write past the one-page `buf`; it is the correct replacement for a raw
 * scnprintf(buf, PAGE_SIZE, ...). */
static ssize_t hits_show(struct kobject *kobj, struct kobj_attribute *attr,
			 char *buf)
{
	return sysfs_emit(buf, "%d\n", atomic_read(&psd.hits));
}

/* show()/store() for the read-write "threshold" file. store() receives the raw
 * bytes the user wrote and the count; we parse them with kstrtou32(), which
 * safely rejects non-numeric or out-of-range input (returning -EINVAL/-ERANGE)
 * instead of the classic simple_strtoul() foot-gun that silently accepts junk.
 * On success we MUST return `count` (bytes consumed) or userspace's write()
 * loops forever thinking it made no progress. */
static ssize_t threshold_show(struct kobject *kobj, struct kobj_attribute *attr,
			      char *buf)
{
	return sysfs_emit(buf, "%u\n", psd.threshold);
}

static ssize_t threshold_store(struct kobject *kobj, struct kobj_attribute *attr,
			       const char *buf, size_t count)
{
	u32 val;
	int ret = kstrtou32(buf, 0, &val);   /* base 0 => accept 0x.. and decimal   */

	if (ret)
		return ret;                  /* propagate -EINVAL/-ERANGE to userspace */

	/* A plain u32 store; a torn read against a concurrent debugfs reader is
	 * harmless here (aligned 32-bit stores are atomic on x86-64). We note but
	 * do not lock it, matching how trivial scalar knobs are handled in tree. */
	psd.threshold = val;
	return count;
}

/* show()/store() for the "label" string file. The string lives in shared state
 * that the /proc reader also touches, so both sides take psd.lock. strscpy() is
 * the safe, always-NUL-terminating copy (unlike strncpy, which can leave the
 * destination unterminated); it also returns -E2BIG on truncation, which we let
 * pass silently here since we intentionally cap the label length. */
static ssize_t label_show(struct kobject *kobj, struct kobj_attribute *attr,
			  char *buf)
{
	ssize_t n;
	unsigned long flags;

	spin_lock_irqsave(&psd.lock, flags);
	n = sysfs_emit(buf, "%s\n", psd.label);
	spin_unlock_irqrestore(&psd.lock, flags);
	return n;
}

static ssize_t label_store(struct kobject *kobj, struct kobj_attribute *attr,
			   const char *buf, size_t count)
{
	unsigned long flags;

	spin_lock_irqsave(&psd.lock, flags);
	/* strscpy copies at most sizeof(label)-1 bytes and always NUL-terminates.
	 * sysfs guarantees `buf` is NUL-terminated, so treating it as a C string is
	 * safe. A trailing newline from `echo` will be copied verbatim; a real ABI
	 * would strip it, which is left as a one-line exercise. */
	strscpy(psd.label, buf, sizeof(psd.label));
	spin_unlock_irqrestore(&psd.lock, flags);
	return count;
}

/* write-only "trigger" file: writing anything records one event. This is the
 * mutating action that makes `hits` and the /proc table change, so you can
 * watch all three interfaces move together. Mode 0200 = write-only; store-only
 * attributes with no show() are legal and appear as --w------- in `ls -l`. */
static ssize_t trigger_store(struct kobject *kobj, struct kobj_attribute *attr,
			     const char *buf, size_t count)
{
	psd_record_event(&psd);
	return count;
}

/* __ATTR(name, mode, show, store) expands to a struct kobj_attribute whose
 * embedded struct attribute has .name = "name". __ATTR_RO / __ATTR_WO are the
 * read-only / write-only shorthands that also force the correct mode bits. */
static struct kobj_attribute hits_attr      = __ATTR_RO(hits);            /* 0444 */
static struct kobj_attribute threshold_attr = __ATTR(threshold, 0644,
						     threshold_show,
						     threshold_store);
static struct kobj_attribute label_attr     = __ATTR(label, 0644,
						     label_show, label_store);
static struct kobj_attribute trigger_attr   = __ATTR_WO(trigger);         /* 0200 */

/* A NULL-terminated array of the raw `struct attribute *` pointers. Each
 * kobj_attribute embeds a struct attribute as its first member, so &X.attr is
 * that inner object. The group creates all of them atomically (and removes them
 * atomically), which is cleaner and race-free compared to N sysfs_create_file
 * calls. */
static struct attribute *psd_attrs[] = {
	&hits_attr.attr,
	&threshold_attr.attr,
	&label_attr.attr,
	&trigger_attr.attr,
	NULL,                            /* sentinel: the array is NULL-terminated  */
};

static const struct attribute_group psd_attr_group = {
	.attrs = psd_attrs,
	/* .name is NULL, so the files land directly in the kobject's directory
	 * rather than in a named sub-directory. */
};

/* ===========================================================================
 * INTERFACE 3 — debugfs.
 * ===========================================================================
 *
 * debugfs is the "no promises" interface for kernel developers. Mounted at
 * /sys/kernel/debug (root-only by default). Its create helpers are gloriously
 * terse: for a scalar you hand debugfs the ADDRESS of the variable and it wires
 * up read+parse+write for you with zero boilerplate. The flip side is that
 * there is NO stable ABI and NO locking — you get raw, racy, convenient access,
 * which is exactly the right trade-off for debugging internals.
 *
 * Note we DELIBERATELY don't check the return values of the debugfs_create_*
 * calls. Since ~2019 they return an ERR_PTR-or-dentry that is safe to ignore:
 * if debugfs is disabled in the kernel config, the functions become no-ops and
 * later debugfs_remove_recursive(NULL/err) does nothing. Kernel style now says
 * driver logic must NOT depend on debugfs succeeding — another way debugfs
 * signals "I am optional and untrustworthy."
 * =========================================================================== */

/* A second look at the SAME state as /proc, but as the raw internal dump that a
 * kernel dev wants: everything on a few lines, no ABI, using single_open — the
 * seq_file shorthand for "my whole output fits comfortably in one pass, just
 * call my show() once." Contrast with the full iterator we wrote for /proc. */
static int psd_debug_state_show(struct seq_file *m, void *v)
{
	unsigned long flags;
	unsigned int i;

	seq_printf(m, "hits=%d\n",      atomic_read(&psd.hits));
	seq_printf(m, "threshold=%u\n", psd.threshold);
	seq_printf(m, "dropped=%u\n",   psd.dropped);

	spin_lock_irqsave(&psd.lock, flags);
	seq_printf(m, "ring_count=%u\n", psd.ring_count);
	seq_printf(m, "label=%s\n",      psd.label);
	for (i = 0; i < psd.ring_count; i++)
		seq_printf(m, "ring[%u]=%llu\n", i, psd.ring[i]);
	spin_unlock_irqrestore(&psd.lock, flags);
	return 0;
}

static int psd_debug_state_open(struct inode *inode, struct file *file)
{
	/* single_open packages a one-shot show() as a seq_file; single_release
	 * frees it. This is the minimal-effort path when you are certain the output
	 * is small — which for a debug dump it always is. */
	return single_open(file, psd_debug_state_show, NULL);
}

static const struct file_operations psd_debug_state_fops = {
	.owner   = THIS_MODULE,
	.open    = psd_debug_state_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

/* ===========================================================================
 * MODULE LIFECYCLE — create everything on load, tear it ALL down on unload.
 * ===========================================================================
 *
 * Ordering discipline: register each interface only after the previous one
 * succeeded, and on ANY failure unwind in exact reverse order. A half-created
 * module that returns an error but leaves a /proc file behind is a guaranteed
 * oops the moment someone cat's it after the module text is freed. Every error
 * path below jumps to the correct unwind label.
 * =========================================================================== */
static int __init psd_init(void)
{
	int ret;

	/* Initialise the spinlock before anything can be reached that takes it.
	 * (A static spinlock could use DEFINE_SPINLOCK, but the struct lives inside
	 * psd, so we init at runtime.) */
	spin_lock_init(&psd.lock);

	/* --- Interface 1: /proc/psd_demo -------------------------------------- */
	/* proc_create(name, mode, parent, ops). parent = NULL => directly under
	 * /proc. Mode 0444 = world-readable. Returns NULL on failure. */
	psd_proc_entry = proc_create(PSD_NAME, 0444, NULL, &psd_proc_ops);
	if (!psd_proc_entry) {
		pr_err("%s: proc_create failed\n", PSD_NAME);
		return -ENOMEM;
	}

	/* --- Interface 2: /sys/kernel/psd_demo/ ------------------------------- */
	/* kernel_kobj is the exported kobject for /sys/kernel. Creating our kobject
	 * under it puts the directory at /sys/kernel/psd_demo. */
	psd_kobj = kobject_create_and_add(PSD_NAME, kernel_kobj);
	if (!psd_kobj) {
		pr_err("%s: kobject_create_and_add failed\n", PSD_NAME);
		ret = -ENOMEM;
		goto err_proc;
	}

	/* Populate the directory with all four attribute files atomically. */
	ret = sysfs_create_group(psd_kobj, &psd_attr_group);
	if (ret) {
		pr_err("%s: sysfs_create_group failed (%d)\n", PSD_NAME, ret);
		goto err_kobj;
	}

	/* --- Interface 3: /sys/kernel/debug/psd_demo/ ------------------------- */
	/* No error handling by design (see the big comment above): if debugfs is
	 * off, these are no-ops and the dir handle is an ERR_PTR that
	 * debugfs_remove_recursive() safely ignores. */
	psd_debug_dir = debugfs_create_dir(PSD_NAME, NULL);

	/* Scalar knobs by address: debugfs owns the read/parse/write plumbing.
	 * threshold aliases the SAME u32 that sysfs exposes — change it here and
	 * the sysfs file reflects it, proving both are views of one variable. */
	debugfs_create_u32("threshold", 0644, psd_debug_dir, &psd.threshold);

	/* atomic_t has its own helper so the read is a proper atomic_read(). */
	debugfs_create_atomic_t("hits", 0444, psd_debug_dir, &psd.hits);

	debugfs_create_u32("dropped", 0444, psd_debug_dir, &psd.dropped);

	/* The raw internal dump via single_open seq_file. */
	debugfs_create_file("state", 0444, psd_debug_dir, NULL,
			    &psd_debug_state_fops);

	pr_info("%s: loaded — see /proc/%s, /sys/kernel/%s/, /sys/kernel/debug/%s/\n",
		PSD_NAME, PSD_NAME, PSD_NAME, PSD_NAME);
	return 0;

/* --- error unwind, reverse order of creation ------------------------------ */
err_kobj:
	kobject_put(psd_kobj);           /* drop our reference -> frees the kobject */
err_proc:
	proc_remove(psd_proc_entry);     /* remove /proc/psd_demo                   */
	return ret;
}

static void __exit psd_exit(void)
{
	/* Tear down in strict reverse order. Each remove severs userspace's path to
	 * this module's code BEFORE the module text is unmapped, closing the
	 * use-after-free window. */

	/* debugfs: one recursive remove frees the dir and every file under it. It
	 * accepts NULL/ERR_PTR, so it is safe even if creation was a no-op. */
	debugfs_remove_recursive(psd_debug_dir);

	/* sysfs: remove the group first, then release our kobject reference. */
	sysfs_remove_group(psd_kobj, &psd_attr_group);
	kobject_put(psd_kobj);

	/* proc: remove the entry. After this returns, no new open() can find it and
	 * the proc core has waited out in-flight readers. */
	proc_remove(psd_proc_entry);

	pr_info("%s: unloaded\n", PSD_NAME);
}

module_init(psd_init);
module_exit(psd_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("low-level-linux-lab");
MODULE_DESCRIPTION("One driver state exported via procfs (seq_file), sysfs (kobject group), and debugfs — a side-by-side comparison of the three interfaces.");
MODULE_VERSION("1.0");
