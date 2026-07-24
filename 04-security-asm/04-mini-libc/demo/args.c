/* ===========================================================================
 * demo/args.c — inspect what the kernel handed us: argv, envp, and the auxv.
 * ===========================================================================
 *
 * This program exists to make the initial-stack tour concrete. start.S read
 * argc/argv off [rsp]; __libc_start_main derived envp and located the auxv.
 * Here we print all three, and pull specific values out of the auxiliary
 * vector with getauxval() — including AT_RANDOM, the kernel-supplied entropy a
 * real libc turns into the stack canary. That connects "the auxv walk" to the
 * defensive lesson: canaries come from bytes the kernel put on this very stack.
 *
 * Run it with a couple of args:   ./args foo bar
 * ===========================================================================
 */
#include "minilibc.h"

int main(int argc, char **argv, char **envp)
{
	printf("== argv (%d entries) ==\n", argc);
	for (int i = 0; i < argc; i++)
		printf("  argv[%d] = %s\n", i, argv[i]);

	/* envp is a NULL-terminated array just past argv. Print the first few so
	 * the output stays short regardless of how big the environment is. */
	int envc = 0;
	while (envp[envc] != NULL)
		envc++;
	printf("== envp (%d entries, first 3 shown) ==\n", envc);
	for (int i = 0; i < envc && i < 3; i++)
		printf("  envp[%d] = %s\n", i, envp[i]);

	/* The auxiliary vector: facts the kernel passes without a syscall. */
	printf("== auxv ==\n");
	printf("  AT_PAGESZ = %lu (bytes per page)\n", getauxval(AT_PAGESZ));
	printf("  AT_UID    = %lu\n", getauxval(AT_UID));
	printf("  AT_EUID   = %lu\n", getauxval(AT_EUID));
	printf("  AT_SECURE = %lu (1 => setuid/setgid, libc hardens)\n",
	       getauxval(AT_SECURE));

	/* AT_RANDOM points at 16 random bytes the kernel generated for THIS exec.
	 * A real libc copies 8 of them into its stack-canary global. We just show
	 * the first 8 as a 64-bit value to make the entropy visible. */
	unsigned long at_random = getauxval(AT_RANDOM);
	if (at_random != 0) {
		unsigned long *seed = (unsigned long *)at_random;
		printf("  AT_RANDOM -> %p, first 8 bytes = 0x%lx\n",
		       (void *)at_random, seed[0]);
		printf("  (a real libc seeds the stack canary from these bytes)\n");
	}

	return 0;
}
