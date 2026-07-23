// SPDX-License-Identifier: GPL-2.0
/* ===========================================================================
 * user/hello_user.c — the userspace half: call our new syscall by number.
 * ===========================================================================
 *
 * This program runs in ORDINARY userspace and invokes the `hello` syscall we
 * added to the kernel. It is fully working C — it compiles and runs on any
 * Linux host today — but it will only SUCCEED when run on a kernel that was
 * built with our patch. On a stock kernel the number 463 is an unassigned slot,
 * so the kernel returns -ENOSYS ("Function not implemented") and you will see
 * that error. That is the correct, expected outcome on an unpatched machine and
 * is itself instructive: it proves the number reached the dispatch table.
 *
 * WHY syscall(2) INSTEAD OF A NICE WRAPPER
 * ----------------------------------------
 * glibc ships a hand-written C wrapper for every *known* syscall (that is what
 * `read()`, `write()`, `openat()` are). Our syscall is brand new, so glibc has
 * no wrapper and no `__NR_hello` in its headers yet. `syscall()` is glibc's
 * generic escape hatch: you hand it the number and the raw arguments and it
 * loads %rax + the argument registers and executes the `syscall` instruction
 * for you. This is exactly how you call any syscall that predates its libc
 * wrapper — the same trick real code uses for e.g. gettid() on old glibc.
 * ===========================================================================
 */

#include <stdio.h>      /* printf, perror                                      */
#include <string.h>     /* strlen (to size our print)                          */
#include <errno.h>      /* errno, set by the syscall() wrapper on failure      */
#include <unistd.h>     /* syscall() prototype, ssize_t                        */
#include <sys/syscall.h>/* __NR_* constants for the syscalls glibc DOES know   */

/* ---------------------------------------------------------------------------
 * The syscall number. This MUST match the number we chose in
 * arch/x86/entry/syscalls/syscall_64.tbl (see ../patches/). On a kernel that
 * already defines it, <sys/syscall.h> would provide __NR_hello and this #ifndef
 * would skip — but on today's hosts it does not exist, so we spell it out.
 *
 * 463 is the first free 64-bit ("common") slot as of Linux 6.10. Numbers 512+
 * are reserved for the x32 ABI, so a common/64 syscall must stay below 512.
 * If you target a different kernel, set this to whatever free number your patch
 * used (`tail arch/x86/entry/syscalls/syscall_64.tbl` in your tree). It is NOT
 * a magic constant — it is just the array index into sys_call_table[]. */
#ifndef __NR_hello
#define __NR_hello 463
#endif

int main(void)
{
	/* Where the kernel will write the greeting. 256 bytes is comfortably
	 * larger than the payload, so no truncation. This lives on our stack, in
	 * userspace — the kernel copies INTO it via copy_to_user(). */
	char buf[256];

	/* Zero it so that if something goes wrong we do not print stack garbage.
	 * Defensive, cheap, and makes the demo deterministic. */
	memset(buf, 0, sizeof(buf));

	/* THE CALL. Arguments after the number map 1:1 onto the syscall's C
	 * signature hello(char __user *buf, size_t len):
	 *   arg1 = buf          -> ends up in %rdi
	 *   arg2 = sizeof(buf)  -> ends up in %rsi
	 * syscall() returns the kernel's %rax: our byte count on success, or -1
	 * with errno set on failure (it converts the kernel's -errno for us). */
	long ret = syscall(__NR_hello, buf, sizeof(buf));

	if (ret < 0) {
		/* errno was set from the kernel's negative return. The two you are
		 * most likely to see:
		 *   ENOSYS  -> the running kernel has no syscall #463 (unpatched).
		 *   EFAULT  -> the kernel rejected our buffer pointer.
		 *   EINVAL  -> we passed len == 0. */
		perror("syscall(__NR_hello)");
		fprintf(stderr,
			"hint: ENOSYS means this kernel wasn't built with the patch; "
			"boot the patched kernel in QEMU.\n");
		return 1;
	}

	/* Success: the kernel copied `ret` bytes into buf. Print exactly what we
	 * were told was written (fwrite, not printf("%s"), in case truncation left
	 * the buffer without a NUL). */
	printf("sys_hello returned %ld bytes:\n", ret);
	fwrite(buf, 1, (size_t)ret, stdout);

	return 0;
}
