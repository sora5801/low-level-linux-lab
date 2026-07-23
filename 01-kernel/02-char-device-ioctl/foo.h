/* ===========================================================================
 * foo.h — the ioctl CONTRACT shared verbatim by the kernel module and userspace.
 * ===========================================================================
 *
 * WHY A SHARED HEADER?
 * --------------------
 * An ioctl() command number is not just an integer — it is a tiny struct packed
 * into a 32-bit word by the _IO/_IOR/_IOW/_IOWR macros. That word encodes four
 * things the kernel checks:
 *
 *     bits 31..30  direction  (none / read / write / read+write)  <- from _IOx
 *     bits 29..16  size       (sizeof of the argument type)
 *     bits 15..8   "magic"    (a per-driver namespace byte, here 'F')
 *     bits  7..0   ordinal    (which command within this driver: 0,1,2,3...)
 *
 * Because BOTH sides compute the command number from the SAME macros over the
 * SAME argument types, they are guaranteed to agree. If userspace and the driver
 * ever disagreed about, say, the size of the argument, the encoded size bits
 * would differ and the driver's switch would simply not match — a built-in
 * mismatch detector. That is the entire reason ioctl numbers are structured and
 * the entire reason this header is shared instead of duplicated.
 *
 * The "direction" is expressed from USERSPACE's point of view:
 *   _IOR  = userspace READS  data back  (kernel -> user; driver does copy_to_user)
 *   _IOW  = userspace WRITES data in    (user -> kernel; driver does copy_from_user)
 *   _IO   = no argument payload at all (the arg word, if any, is a bare scalar)
 * =========================================================================== */
#ifndef _UAPI_FOO_H
#define _UAPI_FOO_H

/* The macros (_IO, _IOR, _IOW) and the fixed-width kernel types (__u32) live in
 * different headers on each side, so include the right ones for whoever is
 * compiling us. Both ultimately resolve to the SAME asm-generic definitions, so
 * the numbers computed below are identical in kernel and user builds. */
#ifdef __KERNEL__
#  include <linux/ioctl.h>   /* _IOR/_IOW/_IO for in-tree module code          */
#  include <linux/types.h>   /* __u32, the ABI-stable fixed-width integer type */
#else
#  include <sys/ioctl.h>     /* the userspace ioctl() prototype + the _IOx macros */
#  include <linux/types.h>   /* __u32 from the installed kernel-headers package  */
#endif

/* The "magic" byte namespaces THIS driver's commands so they can't be confused
 * with another driver's. Convention (see Documentation/userspace-api/ioctl/):
 * pick a letter and register it; 'F' stands in for "foo" here. */
#define FOO_IOC_MAGIC  'F'

/* Snapshot of the FIFO's internal state, returned by FOO_IOC_GET_INFO.
 *
 * We use __u32 (never `unsigned int`, `size_t`, or `long`) on purpose: this
 * struct crosses the kernel/user boundary, so its layout must be FIXED and
 * identical for a 32-bit userspace talking to a 64-bit kernel. __u32 is exactly
 * 4 bytes on every ABI; `long` would be 4 bytes for a 32-bit process and 8 for
 * the kernel, silently corrupting the copy. All fields are naturally aligned and
 * the struct has no padding, so there are no uninitialised holes to leak. */
struct foo_info {
	__u32 capacity;   /* ring buffer size in bytes (a power of two)          */
	__u32 count;      /* bytes currently readable (producer - consumer)      */
	__u32 head;       /* free-running producer index (write cursor)          */
	__u32 tail;       /* free-running consumer index (read cursor)           */
};

/* ---- The command set -------------------------------------------------------
 * Ordinals are dense (0..3) so the driver can range-check with FOO_IOC_MAXNR. */

/* _IO: no payload. Empty the FIFO (drop all buffered bytes, reset cursors). */
#define FOO_IOC_RESET      _IO(FOO_IOC_MAGIC, 0)

/* _IOR(int): kernel writes the current fill level (bytes readable now) back to
 * *(int *)arg via put_user. Direction bit says user is READING. */
#define FOO_IOC_GET_LEVEL  _IOR(FOO_IOC_MAGIC, 1, int)

/* _IOW(int): userspace passes in a new poll low-water mark. poll()/select() will
 * only report the device readable once at least this many bytes are buffered.
 * Direction bit says user is WRITING into the kernel. */
#define FOO_IOC_SET_LOWAT  _IOW(FOO_IOC_MAGIC, 2, int)

/* _IOR(struct foo_info): kernel fills the whole struct and copies it out. */
#define FOO_IOC_GET_INFO   _IOR(FOO_IOC_MAGIC, 3, struct foo_info)

/* Highest valid ordinal; the driver uses it to reject bogus commands early with
 * -ENOTTY (the canonical "this fd doesn't understand that ioctl" errno). */
#define FOO_IOC_MAXNR      3

#endif /* _UAPI_FOO_H */
