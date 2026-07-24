/* ===========================================================================
 * crt.c — the C half of program startup: __libc_start_main, environ, auxv.
 * ===========================================================================
 *
 * start.S (assembly) did the un-C-able part: it read argc/argv straight off the
 * raw entry stack and aligned rsp. It then called us with a clean SysV-ABI
 * signature. From here on everything is ordinary C. Our jobs, in order:
 *
 *   1. Derive envp   — it sits just past argv's NULL terminator.
 *   2. Publish environ — so getenv()-style code and the program can see it.
 *   3. Locate the auxv — the ELF auxiliary vector, just past envp's NULL.
 *   4. Call main(argc, argv, envp).
 *   5. Turn main's `return` into exit_group(status) — because returning from
 *      _start has nowhere to go.
 *
 * Compare musl's src/env/__libc_start_main.c, which additionally runs the ELF
 * init-array constructors, sets up TLS, and installs the stack-canary value
 * from AT_RANDOM. We do the argv/envp/auxv plumbing (the didactic core) and
 * document the rest as the honest gap.
 * ===========================================================================
 */
#include "minilibc.h"

/* Global startup state, declared extern in the header. */
char **environ = NULL;             /* the environment array (NULL-terminated) */

/* Private pointer to the auxiliary vector, walked by getauxval(). It is an
 * array of (a_type, a_val) pairs — we model it as consecutive unsigned longs:
 *   __auxv[0]=a_type, __auxv[1]=a_val, __auxv[2]=a_type, ... , a_type==AT_NULL. */
static unsigned long *__auxv = NULL;

/* main is supplied by the linked program; start.S already took its address. */
extern int main(int argc, char **argv, char **envp);

/* ---------------------------------------------------------------------------
 * __libc_start_main — called exactly once, from _start, and never returns.
 *
 * Arguments (marshaled by start.S into rdi/rsi/rdx):
 *   mainp : address of the program's main()
 *   argc  : the argument count read from [rsp]
 *   argv  : &argv[0], an array of argc `char*` then a NULL terminator
 * --------------------------------------------------------------------------- */
__attribute__((noreturn))
void __libc_start_main(int (*mainp)(int, char **, char **),
                       long argc, char **argv)
{
	/* envp begins immediately after argv's NULL terminator. argv has `argc`
	 * real entries at indices 0..argc-1, then argv[argc] == NULL, so envp is
	 * &argv[argc + 1]. This is the pointer arithmetic start.S could have done,
	 * but it is clearer (and checkable) in C. */
	char **envp = argv + argc + 1;
	environ = envp;

	/* Walk envp to its NULL terminator; the auxv starts one slot past it. The
	 * kernel lays out [argv NULL][envp...][NULL][auxv...][AT_NULL] contiguously,
	 * so this walk is how a libc *finds* the auxv without the kernel telling it
	 * where it is. */
	char **p = envp;
	while (*p != NULL)
		p++;                       /* stop on the NULL that ends envp          */
	__auxv = (unsigned long *)(p + 1); /* auxv sits right after that NULL      */

	/* Hand control to the program. main() runs with a fully-formed C
	 * environment now: environ set, heap ready (malloc self-initializes on
	 * first use), stdout writable via write(2). */
	int status = mainp((int)argc, argv, envp);

	/* main returned normally -> that is defined to mean exit(status). We must
	 * NOT `return`: _start's caller is the kernel and there is no return
	 * address to go back to. */
	exit(status);
}

/* ---------------------------------------------------------------------------
 * getauxval — look up one entry in the auxiliary vector by type.
 *
 * The auxv is how the kernel passes a grab-bag of facts to userspace without a
 * syscall: the page size (AT_PAGESZ), whether we are setuid (AT_SECURE), and —
 * the security payload — AT_RANDOM, a pointer to 16 fresh random bytes the
 * kernel generated for THIS exec. A real libc copies bytes from AT_RANDOM into
 * its stack-canary global; every function prologue then stores that value
 * below the return address, and the epilogue checks it before `ret`. An
 * attacker overflowing a stack buffer must overwrite the canary to reach the
 * return address, and cannot guess these kernel-random bytes — that is the
 * whole cost the canary imposes. Walking the auxv here is where that chain of
 * defense begins.
 * --------------------------------------------------------------------------- */
unsigned long getauxval(unsigned long type)
{
	if (__auxv == NULL)
		return 0;
	for (unsigned long *a = __auxv; a[0] != AT_NULL; a += 2) {
		if (a[0] == type)
			return a[1];           /* a[0]=type matched -> return its a_val    */
	}
	return 0;                      /* not present (0 is the documented miss)   */
}

/* ---------------------------------------------------------------------------
 * exit / _exit — terminate the process.
 *
 * A real exit() first runs atexit() handlers and flushes stdio buffers; we have
 * neither, so exit() and _exit() coincide here (our printf is unbuffered across
 * calls — each printf flushes itself). Both use exit_group so that, in a
 * multi-threaded process, the WHOLE process dies, not just the calling thread.
 * These never return, hence noreturn + __builtin_unreachable to let the
 * compiler drop any code after the syscall.
 * --------------------------------------------------------------------------- */
__attribute__((noreturn))
void _exit(int status)
{
	/* exit_group(2) #231 : rdi = status (low 8 bits become the shell's $?). */
	__asm__ volatile("syscall"
		: /* no outputs: it never returns */
		: "a"(SYS_exit_group), "D"(status)
		: "rcx", "r11", "memory");
	__builtin_unreachable();
}

__attribute__((noreturn))
void exit(int status)
{
	_exit(status);
}

/* ---------------------------------------------------------------------------
 * write_all — write a whole buffer, defeating short writes and EINTR.
 *
 * write(2) is allowed to consume only part of the buffer (a "short write",
 * common on pipes/sockets and when a signal lands mid-write). Naive code that
 * assumes one write() suffices silently drops output. We loop until every byte
 * is out, retrying transparently when the kernel returns EINTR (interrupted by
 * a signal before any byte moved). This is the primitive printf flushes through.
 * --------------------------------------------------------------------------- */
ssize_t write_all(int fd, const void *buf, size_t count)
{
	const char *p = (const char *)buf;
	size_t left = count;
	while (left > 0) {
		ssize_t w = write(fd, p, left);
		if (w < 0) {
			if (errno == EINTR)
				continue;          /* signal, no bytes lost -> just retry      */
			return -1;             /* a real error (EBADF, EPIPE, EFAULT...)   */
		}
		if (w == 0)
			break;                 /* defensive: no progress, avoid a spin     */
		p += (size_t)w;
		left -= (size_t)w;
	}
	return (ssize_t)(count - left);
}
