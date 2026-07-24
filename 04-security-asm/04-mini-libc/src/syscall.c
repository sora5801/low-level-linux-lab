/* ===========================================================================
 * syscall.c — the thin skin between C and the kernel.
 * ===========================================================================
 *
 * Every function a program calls (printf, malloc, open...) eventually bottoms
 * out in ONE machine instruction: `syscall`. This file is where C values are
 * loaded into the exact registers the kernel reads, `syscall` is executed, and
 * the kernel's return is translated into the (-1, errno) contract POSIX code
 * expects. Read musl's arch/x86_64/syscall_arch.h alongside this — it is the
 * same idea, production-hardened.
 *
 * THE LINUX x86-64 SYSCALL ABI (differs from the *function* call ABI!):
 *   - syscall NUMBER : rax
 *   - args, in order : rdi, rsi, rdx, r10, r8, r9      <-- note r10, NOT rcx
 *   - return value   : rax   (a negative value in [-4095,-1] means -errno)
 *   - CLOBBERED      : rcx and r11 are destroyed by the `syscall` instruction
 *                      itself — it saves the return RIP into rcx and RFLAGS
 *                      into r11. Every wrapper must declare that clobber.
 * The function ABI puts the 4th arg in rcx; the syscall ABI puts it in r10.
 * That single divergence is why we cannot just `syscall` a C function's args.
 * ===========================================================================
 */
#include "minilibc.h"

/* The one true definition of errno (declared extern in the header). A real
 * libc makes this __thread-local so concurrent syscalls in different threads
 * do not stomp each other; we keep a single global for teaching clarity. */
int errno = 0;

/* ---------------------------------------------------------------------------
 * The raw syscall templates, one per arity. `volatile` forbids the optimizer
 * from moving, duplicating, or deleting the instruction; "memory" tells it the
 * kernel may read/write any buffer, so it must not cache memory across the
 * call. r10/r8/r9 have no GCC constraint letter, so we bind them via named
 * register variables — the standard musl technique.
 * --------------------------------------------------------------------------- */
static inline long syscall1(long n, long a1)
{
	long ret;
	__asm__ volatile("syscall"
		: "=a"(ret)                       /* out: rax -> ret                 */
		: "a"(n), "D"(a1)                 /* in : rax=n, rdi=a1              */
		: "rcx", "r11", "memory");        /* clobbered by `syscall`          */
	return ret;
}

static inline long syscall3(long n, long a1, long a2, long a3)
{
	long ret;
	__asm__ volatile("syscall"
		: "=a"(ret)
		: "a"(n), "D"(a1), "S"(a2), "d"(a3)   /* rax,rdi,rsi,rdx             */
		: "rcx", "r11", "memory");
	return ret;
}

static inline long syscall6(long n, long a1, long a2, long a3,
                            long a4, long a5, long a6)
{
	long ret;
	/* Pin these three into the kernel's arg registers. `register ... asm("rX")`
	 * is the only portable way to force r10/r8/r9, which have no "=r"-style
	 * single-letter constraint. */
	register long r10 __asm__("r10") = a4;
	register long r8  __asm__("r8")  = a5;
	register long r9  __asm__("r9")  = a6;
	__asm__ volatile("syscall"
		: "=a"(ret)
		: "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8), "r"(r9)
		: "rcx", "r11", "memory");
	return ret;
}

/* ---------------------------------------------------------------------------
 * __syscall_ret — the (-1, errno) translation, applied to a raw kernel return.
 *
 * The kernel never returns a valid result in [-4095, -1]; that whole band is
 * reserved for negated errno codes. Treating `r` as UNSIGNED, the errors are
 * exactly the values strictly greater than -4096 (i.e. 0xFFFF...F001 .. FFFF).
 * On error we stash +errno and return -1; otherwise the value passes through.
 * --------------------------------------------------------------------------- */
static long __syscall_ret(unsigned long r)
{
	if (r > (unsigned long)-4096) {   /* r in [-4095, -1] -> an error         */
		errno = (int)(-(long)r);      /* -(-errno) = +errno                   */
		return -1;
	}
	return (long)r;
}

/* ---------------------------------------------------------------------------
 * The POSIX-named wrappers. Each is: build the syscall, then run the return
 * through __syscall_ret so callers see the familiar -1/errno behavior.
 * --------------------------------------------------------------------------- */

/* read(2)  #0 : rdi=fd, rsi=buf, rdx=count. Returns bytes read (0 = EOF),
 * or -1/errno (EINTR if a signal arrived, EBADF for a bad fd, EFAULT...). */
ssize_t read(int fd, void *buf, size_t count)
{
	return __syscall_ret(syscall3(SYS_read, fd, (long)buf, (long)count));
}

/* write(2) #1 : rdi=fd, rsi=buf, rdx=count. Returns bytes written — possibly
 * FEWER than requested (a short write); callers must loop (see write_all). */
ssize_t write(int fd, const void *buf, size_t count)
{
	return __syscall_ret(syscall3(SYS_write, fd, (long)buf, (long)count));
}

/* open(2)  #2 : rdi=path, rsi=flags (O_*), rdx=mode (permission bits, used
 * only when O_CREAT is set). Returns the new lowest-free fd, or -1/errno
 * (ENOENT if the path is missing, EMFILE if the fd table is full...). */
int open(const char *path, int flags, int mode)
{
	return (int)__syscall_ret(syscall3(SYS_open, (long)path, flags, mode));
}

/* close(2) #3 : rdi=fd. Releases the descriptor. -1/errno (EBADF) on failure. */
int close(int fd)
{
	return (int)__syscall_ret(syscall1(SYS_close, fd));
}

/* mmap(2)  #9 : rdi=addr, rsi=len, rdx=prot, r10=flags, r8=fd, r9=offset.
 * Six args -> syscall6, and note flags land in r10 (not rcx). Returns the
 * mapping address, or MAP_FAILED (not NULL) with errno set. mmap has its own
 * pointer-shaped error convention, so we check the band by hand rather than
 * reusing __syscall_ret's int result. */
void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off)
{
	unsigned long r = (unsigned long)syscall6(SYS_mmap, (long)addr, (long)len,
	                                           prot, flags, fd, off);
	if (r > (unsigned long)-4096) {   /* same [-4095,-1] error band as above  */
		errno = (int)(-(long)r);
		return MAP_FAILED;
	}
	return (void *)r;
}

/* munmap(2) #11 : rdi=addr, rsi=len. Unmaps the region; -1/errno (EINVAL if
 * addr/len are not page-aligned or name no mapping). */
int munmap(void *addr, size_t len)
{
	return (int)__syscall_ret(syscall3(SYS_munmap, (long)addr, (long)len, 0));
}

/* ---------------------------------------------------------------------------
 * brk / sbrk — grow the data segment. This is how our malloc gets memory.
 *
 * brk(2) #12 : rdi = requested new break address. The kernel's raw semantics
 * are unusual: it returns the *resulting* break — the new one on success, or
 * the UNCHANGED old one on failure. There is no errno; you detect failure by
 * "the break I got back is less than the one I asked for". brk(0) is the idiom
 * to query the current break without moving it.
 * --------------------------------------------------------------------------- */
void *sys_brk(void *addr)
{
	/* No __syscall_ret here: brk does not use the negative-errno convention. */
	return (void *)syscall1(SYS_brk, (long)addr);
}

/* sbrk(increment) — the friendly delta interface malloc actually calls. Moves
 * the break by `increment` bytes and returns the PREVIOUS break (so the caller
 * gets the base of the freshly added region). Returns (void*)-1 + ENOMEM on
 * failure. We cache the break in a static so repeated sbrk() calls are cheap
 * and consistent. */
void *sbrk(intptr_t increment)
{
	static unsigned long cur;            /* last known break; 0 = not yet queried */
	if (cur == 0)
		cur = (unsigned long)sys_brk((void *)0);   /* one-time initialization  */

	unsigned long old = cur;
	unsigned long want = old + (unsigned long)increment;
	unsigned long got = (unsigned long)sys_brk((void *)want);

	if (got < want) {                    /* kernel refused to grow the heap     */
		errno = ENOMEM;
		return (void *)-1;
	}
	cur = got;                           /* commit the new break                */
	return (void *)old;                  /* base of the region we just claimed  */
}
