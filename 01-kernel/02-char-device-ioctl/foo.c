// SPDX-License-Identifier: GPL-2.0
/* ===========================================================================
 * foo.c — a /dev/foo misc character device: a blocking byte FIFO with ioctl,
 *         poll(), llseek, and an mmap zero-copy view of its backing store.
 * ===========================================================================
 *
 * WHAT THIS IS
 * ------------
 * A single in-kernel ring buffer (a byte FIFO, like a named pipe) exposed at
 * /dev/foo. Writers append bytes; readers drain them. When the FIFO is empty a
 * blocking reader SLEEPS on a wait queue until a writer supplies data; when it
 * is full a blocking writer sleeps until a reader frees space. poll()/select()
 * report readiness, an ioctl set inspects and tweaks the device, and mmap()
 * hands userspace a direct window onto the kernel's backing pages.
 *
 * THE SUBSYSTEMS IT TOUCHES (and why each matters)
 * ------------------------------------------------
 *   misc_register()  — the cheapest way to get a real /dev node. Instead of
 *                       allocating a whole major number with alloc_chrdev_region
 *                       + cdev_add, a "misc" device borrows major 10 and just
 *                       needs a name + file_operations. The kernel + udev create
 *                       /dev/foo for us on registration.
 *   file_operations  — the vtable the VFS calls for every syscall on our fd:
 *                       .open .read .write .llseek .release .unlocked_ioctl
 *                       .poll .mmap. This IS the driver's public surface.
 *   copy_to/from_user— the ONLY sanctioned way to move bytes across the
 *                       user/kernel boundary. A raw memcpy to a user pointer is
 *                       a security hole (no access_ok check) and can oops on a
 *                       bad address; these helpers validate and fault safely.
 *   wait queues      — wait_event_interruptible() puts a reader to sleep until a
 *                       condition holds; wake_up_interruptible() from the writer
 *                       side re-runs the condition and wakes sleepers. This is
 *                       how blocking I/O is built.
 *   poll             — poll_wait() parks the caller on our queues without
 *                       sleeping, then we return a readiness bitmask; this is the
 *                       machinery behind select/poll/epoll.
 *
 * CONCURRENCY MODEL
 * -----------------
 * ONE global device shared by every open() (a FIFO has a single stream of
 * bytes, so per-open state would make no sense). A single MUTEX serialises all
 * access to the ring indices and buffer. We deliberately use a mutex, not a
 * spinlock, because read()/write() call copy_to/from_user, which may PAGE-FAULT
 * and therefore SLEEP — and you must never sleep holding a spinlock. Holding a
 * (sleepable) mutex across a copy is fine and is the classic char-driver idiom.
 *
 * THE RING BUFFER INDEX TRICK (the pure logic extracted into asm/demo.c)
 * ---------------------------------------------------------------------
 * head (producer) and tail (consumer) are FREE-RUNNING unsigned counters that
 * are never wrapped to the buffer size — they just keep incrementing and wrap
 * naturally at 2^32. Two consequences make the code branch-free and race-light:
 *   count = head - tail                (unsigned subtraction; correct across the
 *                                       2^32 wrap as long as count <= capacity)
 *   physical offset = index & MASK     (MASK = capacity-1; valid ONLY because
 *                                       capacity is a power of two)
 * The single "empty vs full" ambiguity that plagues naive ring buffers (where
 * head==tail could mean either) simply cannot happen here: empty is count==0 and
 * full is count==capacity, two distinct values. This is exactly how the kernel's
 * own kfifo works. capacity is one PAGE so the same buffer is mmap-able.
 * =========================================================================== */

#include <linux/module.h>	/* MODULE_*, module_init/exit                     */
#include <linux/kernel.h>	/* pr_info, container_of, min_t                   */
#include <linux/init.h>		/* __init / __exit section annotations           */
#include <linux/fs.h>		/* struct file, file_operations, SEEK_*          */
#include <linux/miscdevice.h>	/* miscdevice, misc_register/misc_deregister     */
#include <linux/uaccess.h>	/* copy_to_user, copy_from_user, get/put_user    */
#include <linux/wait.h>		/* wait_queue_head_t, wait_event_interruptible   */
#include <linux/sched.h>	/* signal handling used by *_interruptible       */
#include <linux/poll.h>		/* poll_table, poll_wait, EPOLL* masks           */
#include <linux/mutex.h>	/* struct mutex, mutex_lock_interruptible        */
#include <linux/vmalloc.h>	/* vmalloc_user, vfree, remap_vmalloc_range      */
#include <linux/mm.h>		/* struct vm_area_struct for mmap                */
#include <linux/slab.h>		/* (not strictly needed, kept for clarity)       */
#include <linux/types.h>

#include "foo.h"		/* the shared ioctl contract                     */

/* Capacity == one page keeps two invariants at once:
 *   - it is a power of two, so MASK arithmetic below is valid, and
 *   - it is a whole number of pages, so mmap() can map it with no slack.
 * PAGE_SIZE is 4096 on x86-64; MASK is therefore 0xFFF. */
#define FOO_CAP   ((unsigned int)PAGE_SIZE)
#define FOO_MASK  (FOO_CAP - 1u)

/* -------------------------------------------------------------------------
 * The one and only device instance. Everything the driver owns hangs off here.
 * ------------------------------------------------------------------------- */
struct foo_dev {
	struct mutex		lock;	/* serialises ALL access to the fields below   */
	wait_queue_head_t	readq;	/* readers sleep here while the FIFO is empty   */
	wait_queue_head_t	writeq;	/* writers sleep here while the FIFO is full    */

	unsigned char	       *buf;	/* backing store, FOO_CAP bytes, page-aligned   */
	unsigned int		head;	/* producer index (free-running, wraps at 2^32) */
	unsigned int		tail;	/* consumer index (free-running, wraps at 2^32) */

	unsigned int		lowat;	/* poll() read low-water mark, in bytes         */
	unsigned long long	consumed;/* total bytes ever consumed = stream position  */
};

static struct foo_dev foo;	/* zero-initialised in BSS; fields set up in _init */

/* ---- ring-buffer predicates (the pure logic; see asm/demo.c) --------------
 * These read head/tail with READ_ONCE. WHY READ_ONCE: the wait_event condition
 * below is evaluated OUTSIDE our mutex (that is how wait_event works — it tests
 * the condition, and only sleeps if false). READ_ONCE forbids the compiler from
 * hoisting the load out of the wait loop or tearing it, which would let a thread
 * spin forever on a stale cached value. The AUTHORITATIVE decision is always
 * re-made under the mutex after we wake; these are just the wakeup trigger. */
static inline unsigned int foo_count(struct foo_dev *d)
{
	return READ_ONCE(d->head) - READ_ONCE(d->tail);   /* bytes readable */
}
static inline unsigned int foo_space(struct foo_dev *d)
{
	return FOO_CAP - foo_count(d);                     /* bytes writable */
}

/* ===========================================================================
 * open / release — attach and detach the shared device to a struct file.
 * ===========================================================================
 * With a single global FIFO there is no per-open allocation to do; we simply
 * stash a pointer so the other handlers can reach the device via file->private_data
 * instead of touching the global directly (cleaner, and the shape you would keep
 * if you later grew to one device per minor). */
static int foo_open(struct inode *inode, struct file *file)
{
	file->private_data = &foo;
	/* We deliberately do NOT call nonseekable_open(): that would make the VFS
	 * reject every lseek() with -ESPIPE before our handler ran, but we want to
	 * offer the one seek that IS meaningful on a stream — a forward skip (see
	 * foo_llseek). So we just record the device on the struct file and return. */
	return 0;
}

static int foo_release(struct inode *inode, struct file *file)
{
	/* Nothing to free: the FIFO outlives individual opens (like a named pipe).
	 * A per-open design would drop a refcount / free buffers here. */
	return 0;
}

/* ===========================================================================
 * read — drain up to `len` bytes, BLOCKING until at least one byte exists.
 * ===========================================================================
 * VFS contract: return >0 = bytes read, 0 = EOF (never, for a FIFO), or a
 * negative errno. Partial reads are legal and expected. */
static ssize_t foo_read(struct file *file, char __user *ubuf,
			size_t len, loff_t *ppos)
{
	struct foo_dev *d = file->private_data;
	unsigned int count, off, first;
	ssize_t ret;

	if (len == 0)
		return 0;

	/* mutex_lock_interruptible: if a fatal signal arrives while we wait for the
	 * lock, bail out with -ERESTARTSYS so the syscall can be restarted or the
	 * process can die, instead of hanging uninterruptibly. */
	if (mutex_lock_interruptible(&d->lock))
		return -ERESTARTSYS;

	/* Sleep until there is something to read. Note the DROP-then-WAIT dance:
	 * we release the lock before sleeping (otherwise no writer could ever run
	 * to make the condition true — instant deadlock), then re-acquire and
	 * re-test in the loop. wait_event_interruptible itself re-checks the
	 * condition atomically w.r.t. wake_up, closing the lost-wakeup race. */
	while (foo_count(d) == 0) {
		mutex_unlock(&d->lock);

		/* Non-blocking callers get -EAGAIN instead of sleeping. This is the
		 * O_NONBLOCK contract that select/poll loops rely on. */
		if (file->f_flags & O_NONBLOCK)
			return -EAGAIN;

		/* Sleep until foo_count(d) != 0. Returns nonzero only if a signal
		 * interrupted the sleep, in which case we surrender the syscall. */
		if (wait_event_interruptible(d->readq, foo_count(d) != 0))
			return -ERESTARTSYS;

		if (mutex_lock_interruptible(&d->lock))
			return -ERESTARTSYS;
	}

	/* Lock is held and count > 0. Clamp the request to what's available. */
	count = foo_count(d);
	if (len > count)
		len = count;

	/* Split the copy at the wrap boundary. `off` is the physical position of
	 * the consumer; `first` is how many bytes sit contiguously before the
	 * buffer end. If len spills past the end we do a second copy from offset 0.
	 * This exact arithmetic is what asm/demo.c isolates and annotates. */
	off   = d->tail & FOO_MASK;
	first = min_t(unsigned int, len, FOO_CAP - off);

	/* copy_to_user may fault (and thus sleep) — legal here because we hold a
	 * MUTEX, not a spinlock. On failure it returns the count NOT copied and we
	 * must report -EFAULT without advancing the consumer (no bytes consumed). */
	if (copy_to_user(ubuf, d->buf + off, first)) {
		ret = -EFAULT;
		goto out;
	}
	if (len > first) {
		if (copy_to_user(ubuf + first, d->buf, len - first)) {
			ret = -EFAULT;
			goto out;
		}
	}

	/* Publish consumption. Advancing tail is what frees space for writers. */
	d->tail     += len;
	d->consumed += len;
	*ppos        = d->consumed;   /* keep the stream position coherent for llseek */
	ret          = len;

out:
	mutex_unlock(&d->lock);

	/* We freed space, so wake any writer blocked on a full FIFO and any poller
	 * waiting for POLLOUT. wake_up_interruptible re-evaluates their conditions;
	 * spurious wakeups are harmless because they re-test under the lock. */
	if (ret > 0)
		wake_up_interruptible(&d->writeq);

	return ret;
}

/* ===========================================================================
 * write — append up to `len` bytes, BLOCKING until at least one byte fits.
 * =========================================================================== */
static ssize_t foo_write(struct file *file, const char __user *ubuf,
			 size_t len, loff_t *ppos)
{
	struct foo_dev *d = file->private_data;
	unsigned int space, off, first;
	ssize_t ret;

	if (len == 0)
		return 0;

	if (mutex_lock_interruptible(&d->lock))
		return -ERESTARTSYS;

	/* Mirror of the read loop: wait for room, dropping the lock while asleep so
	 * a reader can drain and make space. */
	while (foo_space(d) == 0) {
		mutex_unlock(&d->lock);

		if (file->f_flags & O_NONBLOCK)
			return -EAGAIN;

		if (wait_event_interruptible(d->writeq, foo_space(d) != 0))
			return -ERESTARTSYS;

		if (mutex_lock_interruptible(&d->lock))
			return -ERESTARTSYS;
	}

	space = foo_space(d);
	if (len > space)
		len = space;              /* accept a partial write; caller loops */

	/* Same wrap-split as read, but at the PRODUCER index this time. */
	off   = d->head & FOO_MASK;
	first = min_t(unsigned int, len, FOO_CAP - off);

	if (copy_from_user(d->buf + off, ubuf, first)) {
		ret = -EFAULT;
		goto out;
	}
	if (len > first) {
		if (copy_from_user(d->buf, ubuf + first, len - first)) {
			ret = -EFAULT;
			goto out;
		}
	}

	/* Advancing head is what makes the new bytes visible to readers. */
	d->head += len;
	ret      = len;

out:
	mutex_unlock(&d->lock);

	/* New data is available: wake a blocked reader and any POLLIN poller. */
	if (ret > 0)
		wake_up_interruptible(&d->readq);

	return ret;
}

/* ===========================================================================
 * llseek — forward-skip (discard) bytes from the stream. A FIFO is not randomly
 *          seekable, so we implement only the operations that make sense.
 * ===========================================================================
 * We model the file position as `consumed` = total bytes the reader side has
 * dropped (via read() or by skipping here). Seeking FORWARD discards buffered
 * bytes (like fast-forwarding a pipe); seeking backward is impossible (the bytes
 * are gone) and SEEK_END is meaningless (a live stream has no fixed end). */
static loff_t foo_llseek(struct file *file, loff_t off, int whence)
{
	struct foo_dev *d = file->private_data;
	loff_t target;
	unsigned int skip, avail, drop;

	if (mutex_lock_interruptible(&d->lock))
		return -ERESTARTSYS;

	switch (whence) {
	case SEEK_SET:
		target = off;                                  /* absolute stream position */
		break;
	case SEEK_CUR:
		target = (loff_t)d->consumed + off;            /* relative to where we are  */
		break;
	case SEEK_END:
	default:
		mutex_unlock(&d->lock);
		return -ESPIPE;   /* "illegal seek": the canonical errno for a pipe/stream */
	}

	/* You cannot un-consume bytes that were already handed out or dropped. */
	if (target < (loff_t)d->consumed) {
		mutex_unlock(&d->lock);
		return -EINVAL;
	}

	/* Clamp the skip to what is actually buffered right now: we can only drop
	 * bytes that exist. Requesting to skip past the end of current data simply
	 * drops everything available (the position will reflect what really moved). */
	skip  = (unsigned int)(target - d->consumed);
	avail = foo_count(d);
	drop  = min_t(unsigned int, skip, avail);

	d->tail     += drop;
	d->consumed += drop;
	file->f_pos  = d->consumed;

	mutex_unlock(&d->lock);

	if (drop)
		wake_up_interruptible(&d->writeq);   /* skipping freed space */

	return d->consumed;
}

/* ===========================================================================
 * unlocked_ioctl — the out-of-band control channel.
 * ===========================================================================
 * "unlocked" means the caller does NOT hold the old Big Kernel Lock (long dead);
 * we do our own locking. `cmd` is the packed number from foo.h; `arg` is an
 * opaque unsigned long that, per the command's direction bits, is either a
 * scalar or a __user pointer we must validate with get_user/put_user/copy_*. */
static long foo_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct foo_dev *d = file->private_data;
	void __user *uarg = (void __user *)arg;
	int val;

	/* First line of defense: reject anything not addressed to THIS driver, and
	 * any ordinal out of range, with -ENOTTY (the ioctl-specific "not
	 * understood"). This also stops us from acting on another driver's number
	 * that happens to collide numerically. */
	if (_IOC_TYPE(cmd) != FOO_IOC_MAGIC)
		return -ENOTTY;
	if (_IOC_NR(cmd) > FOO_IOC_MAXNR)
		return -ENOTTY;

	switch (cmd) {
	case FOO_IOC_RESET:
		/* Drop everything. Take the lock so we don't race a read/write mid-copy.
		 * Setting tail = head makes count == 0 (empty) atomically w.r.t. readers. */
		mutex_lock(&d->lock);
		d->tail = d->head;
		mutex_unlock(&d->lock);
		wake_up_interruptible(&d->writeq);  /* the whole buffer is free now */
		return 0;

	case FOO_IOC_GET_LEVEL:
		/* Report bytes readable. put_user writes ONE int to *(int *)arg safely,
		 * validating the address; -EFAULT if userspace passed a bad pointer. */
		val = (int)foo_count(d);
		if (put_user(val, (int __user *)uarg))
			return -EFAULT;
		return 0;

	case FOO_IOC_SET_LOWAT:
		/* Pull the new low-water mark in from userspace, then clamp it into
		 * [1, FOO_CAP] so poll() can never wait for more bytes than can exist. */
		if (get_user(val, (int __user *)uarg))
			return -EFAULT;
		if (val < 1)
			val = 1;
		if (val > (int)FOO_CAP)
			val = FOO_CAP;
		WRITE_ONCE(d->lowat, (unsigned int)val);
		/* A lower threshold may make the device readable now — re-poll. */
		wake_up_interruptible(&d->readq);
		return 0;

	case FOO_IOC_GET_INFO: {
		/* Assemble a consistent snapshot UNDER the lock so head/tail/count agree
		 * with each other, then copy the whole struct out in one shot. */
		struct foo_info info;

		mutex_lock(&d->lock);
		info.capacity = FOO_CAP;
		info.count    = foo_count(d);
		info.head     = d->head;
		info.tail     = d->tail;
		mutex_unlock(&d->lock);

		if (copy_to_user(uarg, &info, sizeof(info)))
			return -EFAULT;
		return 0;
	}

	default:
		return -ENOTTY;
	}
}

/* ===========================================================================
 * poll — the readiness engine behind select/poll/epoll.
 * ===========================================================================
 * poll_wait() does NOT sleep. It registers `file` on each wait queue with the
 * caller's poll_table so that a later wake_up on that queue re-runs poll. We
 * then return a mask of which conditions are TRUE right now. The kernel combines
 * this with the events the caller asked for. Registering on BOTH queues means a
 * writer's wake (readable) or a reader's wake (writable) both re-poll us. */
static __poll_t foo_poll(struct file *file, poll_table *wait)
{
	struct foo_dev *d = file->private_data;
	__poll_t mask = 0;
	unsigned int count;

	poll_wait(file, &d->readq, wait);
	poll_wait(file, &d->writeq, wait);

	mutex_lock(&d->lock);
	count = foo_count(d);
	/* Readable only once we have at least `lowat` bytes — the SET_LOWAT knob.
	 * EPOLLRDNORM is the "normal data" companion to EPOLLIN that epoll wants. */
	if (count >= READ_ONCE(d->lowat))
		mask |= EPOLLIN | EPOLLRDNORM;
	/* Writable whenever any space remains. */
	if (count < FOO_CAP)
		mask |= EPOLLOUT | EPOLLWRNORM;
	mutex_unlock(&d->lock);

	return mask;
}

/* ===========================================================================
 * mmap — the STRETCH goal: map the FIFO's backing pages into userspace so it can
 *        read the raw bytes with ZERO copy_to_user calls.
 * ===========================================================================
 * The buffer is allocated with vmalloc_user(), which (a) zero-fills it and
 * (b) lays it out so its pages can be handed to userspace. remap_vmalloc_range()
 * walks the (physically discontiguous) vmalloc pages and installs a PTE for each
 * into the caller's VMA — one syscall, no per-byte copying thereafter.
 *
 * IMPORTANT teaching caveat, stated honestly: this shares the STORAGE, not a
 * synchronised protocol. head/tail still live in the kernel and move under the
 * mutex, so a userspace mmap reader sees live bytes but must still learn where
 * valid data is via FOO_IOC_GET_INFO (or read()). A true lock-free shared ring
 * (à la io_uring) would also place head/tail in the shared page with acquire/
 * release ordering; that is the "going further" note in the README. */
static int foo_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct foo_dev *d = file->private_data;
	unsigned long size = vma->vm_end - vma->vm_start;

	/* Refuse to map more than we own, or at a non-zero file offset: our object
	 * is exactly one FOO_CAP-sized region starting at byte 0. */
	if (vma->vm_pgoff != 0)
		return -EINVAL;
	if (size > FOO_CAP)
		return -EINVAL;

	/* remap_vmalloc_range maps `d->buf` starting at vmalloc page index 0 into
	 * the VMA. It sets the right page protections and VMA flags; on error the
	 * caller gets the negative errno and no partial mapping is left behind. */
	return remap_vmalloc_range(vma, d->buf, 0);
}

/* The vtable. Order does not matter; naming the fields (C99 designated
 * initialisers) makes it robust to struct changes across kernel versions.
 * .owner = THIS_MODULE bumps our module refcount while any fd is open, so the
 * module cannot be rmmod'd out from under a live user — a use-after-free guard. */
static const struct file_operations foo_fops = {
	.owner		= THIS_MODULE,
	.open		= foo_open,
	.release	= foo_release,
	.read		= foo_read,
	.write		= foo_write,
	.llseek		= foo_llseek,
	.unlocked_ioctl	= foo_ioctl,
	.poll		= foo_poll,
	.mmap		= foo_mmap,
};

/* misc device descriptor. MISC_DYNAMIC_MINOR asks the core to pick a free minor
 * under major 10; .name becomes /dev/foo (udev creates the node). .mode makes it
 * world read/write for easy demoing — a REAL driver would restrict this and/or
 * rely on udev rules, since 0666 lets any user drive the device. */
static struct miscdevice foo_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = "foo",
	.fops  = &foo_fops,
	.mode  = 0666,
};

/* ===========================================================================
 * module init / exit
 * =========================================================================== */
static int __init foo_init(void)
{
	int err;

	/* Initialise the synchronisation objects BEFORE the device is registered:
	 * once misc_register returns, userspace can open() and race straight into
	 * read()/write(), so every lock and queue must already be valid. */
	mutex_init(&foo.lock);
	init_waitqueue_head(&foo.readq);
	init_waitqueue_head(&foo.writeq);
	foo.head = foo.tail = 0;
	foo.consumed = 0;
	foo.lowat = 1;   /* default: readable as soon as a single byte arrives */

	/* vmalloc_user gives page-aligned, zeroed memory suitable for mmap into
	 * userspace (unlike kmalloc, whose slab pages you must not hand out). */
	foo.buf = vmalloc_user(FOO_CAP);
	if (!foo.buf)
		return -ENOMEM;

	/* Register LAST: this is the point the device becomes visible. If it fails
	 * we must undo the allocation to avoid a leak on the error path. */
	err = misc_register(&foo_misc);
	if (err) {
		vfree(foo.buf);
		foo.buf = NULL;
		return err;
	}

	pr_info("foo: registered /dev/foo (minor %d), FIFO capacity %u bytes\n",
		foo_misc.minor, FOO_CAP);
	return 0;
}

static void __exit foo_exit(void)
{
	/* Deregister FIRST so no new open() can start; the module refcount held via
	 * .owner guarantees there are no open fds still in our fops at this point. */
	misc_deregister(&foo_misc);
	vfree(foo.buf);
	foo.buf = NULL;
	pr_info("foo: unregistered /dev/foo\n");
}

module_init(foo_init);
module_exit(foo_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("low-level-linux-lab");
MODULE_DESCRIPTION("Blocking byte-FIFO misc char device with ioctl, poll, llseek, mmap");
MODULE_VERSION("1.0");
