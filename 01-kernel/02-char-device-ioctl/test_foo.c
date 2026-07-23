// SPDX-License-Identifier: GPL-2.0
/* ===========================================================================
 * test_foo.c — userspace exerciser for /dev/foo.
 * ===========================================================================
 *
 * This program is the "other half" of the ioctl contract in foo.h. It opens the
 * device and drives every code path in the module so you can watch the kernel
 * side respond (keep `dmesg -w` open in another terminal):
 *
 *   1. write() some bytes, then GET_LEVEL/GET_INFO to see the ring fill up.
 *   2. read() them back and confirm the FIFO ordering.
 *   3. SET_LOWAT + poll() to show readiness gating on the low-water mark.
 *   4. fork a reader that BLOCKS in read(), then have the parent write to it,
 *      proving the wait_event/wake_up path works across processes.
 *   5. lseek() forward to DISCARD bytes from the stream.
 *   6. mmap() the backing store and read bytes with zero copy_to_user calls.
 *
 * Every syscall's return value is checked — the error paths are half the point.
 *
 * Build:  cc -Wall -Wextra -O2 -o test_foo test_foo.c
 * Run  :  ./test_foo            (needs the foo.ko module loaded; see README)
 * =========================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/mman.h>
#include <sys/wait.h>

#include "foo.h"   /* the SHARED ioctl numbers — identical to what the driver sees */

#define DEV "/dev/foo"

/* die: print the failing call + errno string and abort. Keeps the demo terse
 * while still surfacing exactly which syscall failed and why. */
static void die(const char *what)
{
	fprintf(stderr, "%s: %s\n", what, strerror(errno));
	exit(1);
}

/* Pretty-print a FOO_IOC_GET_INFO snapshot. */
static void show_info(int fd, const char *when)
{
	struct foo_info info;

	if (ioctl(fd, FOO_IOC_GET_INFO, &info) < 0)
		die("ioctl GET_INFO");

	printf("  [%s] capacity=%u count=%u head=%u tail=%u\n",
	       when, info.capacity, info.count, info.head, info.tail);
}

int main(void)
{
	int fd, level;
	ssize_t n;

	/* O_RDWR: we both read and write the FIFO. No O_NONBLOCK here, so read()
	 * will BLOCK when the buffer is empty — exactly what we want to test. */
	fd = open(DEV, O_RDWR);
	if (fd < 0)
		die("open " DEV);

	puts("== 1. write then inspect ==");
	/* Start from a known-empty state regardless of earlier runs. */
	if (ioctl(fd, FOO_IOC_RESET) < 0)
		die("ioctl RESET");

	const char *msg = "hello, ring buffer";
	n = write(fd, msg, strlen(msg));
	if (n < 0)
		die("write");
	printf("  wrote %zd bytes\n", n);

	/* _IOR: the kernel copies the level back into `level`. */
	if (ioctl(fd, FOO_IOC_GET_LEVEL, &level) < 0)
		die("ioctl GET_LEVEL");
	printf("  GET_LEVEL reports %d bytes buffered\n", level);
	show_info(fd, "after write");

	puts("== 2. read it back (FIFO order) ==");
	char buf[64] = {0};
	n = read(fd, buf, sizeof(buf) - 1);
	if (n < 0)
		die("read");
	buf[n] = '\0';
	printf("  read %zd bytes: \"%s\"\n", n, buf);
	show_info(fd, "after read");

	puts("== 3. SET_LOWAT + poll() readiness gating ==");
	/* Ask poll() to consider us readable only once >= 8 bytes are buffered. */
	level = 8;
	if (ioctl(fd, FOO_IOC_SET_LOWAT, &level) < 0)
		die("ioctl SET_LOWAT");

	/* Write 4 bytes: below the 8-byte watermark, so poll() must NOT report
	 * POLLIN yet (but should report POLLOUT — space is available). */
	if (write(fd, "1234", 4) < 0)
		die("write 4");

	struct pollfd pfd = { .fd = fd, .events = POLLIN | POLLOUT };
	if (poll(&pfd, 1, 0) < 0)
		die("poll");
	printf("  4 bytes buffered, lowat=8 -> POLLIN=%d POLLOUT=%d (expect 0/1)\n",
	       !!(pfd.revents & POLLIN), !!(pfd.revents & POLLOUT));

	/* Cross the watermark: now poll() should report readable. */
	if (write(fd, "5678", 4) < 0)
		die("write 4 more");
	pfd.revents = 0;
	if (poll(&pfd, 1, 0) < 0)
		die("poll 2");
	printf("  8 bytes buffered, lowat=8 -> POLLIN=%d POLLOUT=%d (expect 1/1)\n",
	       !!(pfd.revents & POLLIN), !!(pfd.revents & POLLOUT));

	/* Restore a 1-byte watermark and drain, so the next tests start clean. */
	level = 1;
	if (ioctl(fd, FOO_IOC_SET_LOWAT, &level) < 0)
		die("ioctl SET_LOWAT reset");
	if (ioctl(fd, FOO_IOC_RESET) < 0)
		die("ioctl RESET 2");

	puts("== 4. blocking read across a fork (wait_event/wake_up) ==");
	pid_t pid = fork();
	if (pid < 0)
		die("fork");
	if (pid == 0) {
		/* CHILD: block in read() on the empty FIFO until the parent writes. */
		char cbuf[32] = {0};
		ssize_t cn = read(fd, cbuf, sizeof(cbuf) - 1);
		if (cn < 0)
			die("child read");
		cbuf[cn] = '\0';
		printf("  child woke from blocking read, got: \"%s\"\n", cbuf);
		close(fd);
		_exit(0);
	}
	/* PARENT: give the child a moment to enter read() and go to sleep on the
	 * wait queue, then feed it. usleep is a demo-grade synchronisation; the
	 * correctness does not depend on it (a late write just wakes it later). */
	usleep(100 * 1000);
	if (write(fd, "wake up!", 8) < 0)
		die("parent write");
	if (waitpid(pid, NULL, 0) < 0)
		die("waitpid");

	puts("== 5. lseek() forward discards buffered bytes ==");
	if (ioctl(fd, FOO_IOC_RESET) < 0)
		die("ioctl RESET 3");
	if (write(fd, "ABCDEFGHIJ", 10) < 0)   /* 10 bytes buffered */
		die("write 10");
	/* Skip 4 bytes forward from the current stream position: drops "ABCD". */
	off_t pos = lseek(fd, 4, SEEK_CUR);
	if (pos < 0)
		die("lseek");
	printf("  lseek(+4) -> stream position %ld\n", (long)pos);
	char rest[16] = {0};
	n = read(fd, rest, sizeof(rest) - 1);
	if (n < 0)
		die("read after seek");
	rest[n] = '\0';
	printf("  remaining bytes after skip: \"%s\" (expect \"EFGHIJ\")\n", rest);

	puts("== 6. mmap() zero-copy view of the backing store ==");
	if (ioctl(fd, FOO_IOC_RESET) < 0)
		die("ioctl RESET 4");
	if (write(fd, "MMAP-VISIBLE", 12) < 0)
		die("write mmap");

	long pgsz = sysconf(_SC_PAGESIZE);
	/* Map one page read-only; MAP_SHARED so we observe the kernel's live bytes. */
	unsigned char *p = mmap(NULL, pgsz, PROT_READ, MAP_SHARED, fd, 0);
	if (p == MAP_FAILED)
		die("mmap");

	/* The bytes we wrote landed at physical offset (tail & MASK) == 0 here,
	 * because RESET leaves head==tail and the first write starts at offset 0.
	 * We read them straight out of the shared page — no read() syscall. */
	struct foo_info info;
	if (ioctl(fd, FOO_IOC_GET_INFO, &info) < 0)
		die("ioctl GET_INFO mmap");
	unsigned int off = info.tail % info.capacity;
	printf("  mmap sees (offset %u): \"%.12s\"\n", off, (char *)(p + off));

	if (munmap(p, pgsz) < 0)
		die("munmap");

	close(fd);
	puts("== all checks done ==");
	return 0;
}
