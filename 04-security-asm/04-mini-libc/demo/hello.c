/* ===========================================================================
 * demo/hello.c — a normal-looking C program, but linked against mini-libc.
 * ===========================================================================
 *
 * Nothing here hints that there is no glibc underneath: it has a `main`, calls
 * printf/malloc/strcpy/free, and returns a status. That is the whole point —
 * mini-libc supplies _start, the CRT that calls main, the heap, and stdio, so
 * ordinary C "just works" with `-nostdlib`. Build it with the Makefile, then
 * `strace ./hello one two` and watch: brk (malloc), write (printf), exit_group.
 * ===========================================================================
 */
#include "minilibc.h"

int main(int argc, char **argv, char **envp)
{
	/* printf goes through our varargs + itoa + buffered write path. */
	printf("Hello from mini-libc!\n");

	/* argv came off the initial stack in start.S and was threaded through
	 * __libc_start_main into main — exactly as a real libc does it. */
	printf("argc = %d\n", argc);
	for (int i = 0; i < argc; i++)
		printf("  argv[%d] = %s\n", i, argv[i]);

	/* Count the environment (envp is NULL-terminated) to prove we found it. */
	int envc = 0;
	while (envp[envc] != NULL)
		envc++;
	printf("inherited %d environment variables\n", envc);

	/* Exercise the heap: malloc -> strcpy -> print -> free. `m` is a 16-aligned
	 * block from our first-fit free list, grown out of brk. %p shows its
	 * address (well above the code, in the heap region). */
	char *m = malloc(32);
	if (m == NULL) {
		dprintf(2, "malloc failed\n");
		return 1;
	}
	strcpy(m, "heap-allocated string");
	printf("malloc(32) -> %p : \"%s\"\n", (void *)m, m);
	free(m);

	/* Show every printf conversion specifier we implement, in one line. */
	printf("formats: d=%d u=%u x=%x X=%X c=%c p=%p ld=%ld\n",
	       -42, 42u, 0xdead, 0xbeef, '!', (void *)main, (long)-1);

	/* Returning from main becomes exit_group((return)&0xff) inside our CRT.
	 * Status 0 -> the shell sees $?=0. */
	return 0;
}
