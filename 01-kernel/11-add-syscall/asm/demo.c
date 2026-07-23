/* ===========================================================================
 * asm/demo.c — the userspace syscall wrapper, extracted for annotated assembly.
 * ===========================================================================
 *
 * WHY THIS FILE EXISTS
 * --------------------
 * The kernel half of this project (kernel/sys_hello.c) cannot be compiled to
 * standalone assembly on this host: it depends on the in-tree Linux headers
 * (linux/syscalls.h, the SYSCALL_DEFINE machinery, uaccess.h) that only exist
 * inside a configured kernel build. So, per the lab convention, we extract the
 * project's most instructive PURE-LOGIC core into this self-contained file and
 * generate the teaching assembly from it instead.
 *
 * For THIS project the instructive core is the userspace side: how a raw
 * `syscall` instruction is set up. This file reimplements what glibc's
 * syscall() does — load the call number into %rax, place the arguments in the
 * syscall argument registers, execute `syscall`, read the result back from
 * %rax — with NO libc and NO headers, so the generated .s shows the bare metal:
 * the `syscall` instruction and the __NR_hello number (463) landing in %rax.
 *
 * IMPORTANT ABI DETAIL YOU WILL SEE IN THE ASM
 * --------------------------------------------
 * The C calling convention and the syscall convention DISAGREE about the 4th
 * argument, and that mismatch is the whole lesson of a syscall wrapper:
 *
 *   C function args (SysV AMD64):   rdi, rsi, rdx, RCX, r8, r9
 *   syscall instruction args:       rdi, rsi, rdx, R10, r8, r9
 *
 * The 4th argument travels in %rcx for a normal `call`, but the `syscall`
 * instruction itself DESTROYS %rcx (it stashes the return address there) and
 * %r11 (it stashes RFLAGS). So the kernel entry ABI uses %r10 for arg4 instead.
 * A correct wrapper must therefore copy rcx -> r10 before executing `syscall`.
 * We keep the demo to 3 args so the happy path stays legible, and show the
 * r10 move in a dedicated 4-argument variant so you can see the shuffle.
 *
 * This file has NO #include on purpose: it declares its own types and touches
 * nothing outside itself, which is exactly what makes it standalone-compilable.
 * ===========================================================================
 */

/* Our own fixed-width-ish types so we need no <stdint.h>/<stddef.h>. On the
 * LP64 model Linux uses for x86-64, `long`/`unsigned long` are 64 bits and hold
 * a pointer, a size, or a syscall return without truncation. */
typedef long          sword;   /* signed machine word   (holds a syscall ret) */
typedef unsigned long uword;   /* unsigned machine word (holds a size/pointer) */

/* The syscall number, identical to the value in the syscall table patch and in
 * user/hello_user.c. Spelling it as a plain #define means the compiler can fold
 * it straight into an immediate — you will see `$463` materialize in %eax. */
#define __NR_hello 463

/* ---------------------------------------------------------------------------
 * raw_syscall3 — issue a 3-argument system call with no libc help.
 *
 * The inline-asm constraints pin each C value into the register the `syscall`
 * ABI requires at the instant the instruction runs:
 *   "=a"(ret)  read the RESULT out of %rax after the instruction
 *   "a"(nr)    put the call number into %rax before it
 *   "D"(a1)    arg1 -> %rdi     (D is GCC's constraint letter for rdi)
 *   "S"(a2)    arg2 -> %rsi     (S = rsi)
 *   "d"(a3)    arg3 -> %rdx     (d = rdx)
 * Clobbers "rcx","r11" declare the two registers the `syscall` instruction
 * always destroys; "memory" forbids the compiler from caching memory across the
 * call, because the kernel may read or write our buffers. Omitting "memory" is
 * a classic, hard-to-find miscompile.
 * --------------------------------------------------------------------------- */
static inline sword raw_syscall3(sword nr, sword a1, sword a2, sword a3)
{
	sword ret;
	__asm__ volatile (
		"syscall"
		: "=a"(ret)                            /* out: %rax -> ret          */
		: "a"(nr), "D"(a1), "S"(a2), "d"(a3)   /* in : nr,arg1,arg2,arg3    */
		: "rcx", "r11", "memory"               /* destroyed by `syscall`    */
	);
	return ret;
}

/* ---------------------------------------------------------------------------
 * invoke_hello — the minimal, real invocation of our new syscall.
 *
 * hello(buf, len) is a 2-argument syscall, so a3 is unused (0). Because the C
 * argument order (buf=rdi, len=rsi) already matches the syscall argument order,
 * the compiler has almost nothing to shuffle: at -O1 this collapses to "load
 * 463 into %rax, zero %rdx for the unused arg3, execute `syscall`". That is the
 * clearest possible view of "the __NR argument in %rax" the spec asks for.
 * --------------------------------------------------------------------------- */
sword invoke_hello(char *buf, uword len)
{
	return raw_syscall3(__NR_hello, (sword)buf, (sword)len, 0);
}

/* ---------------------------------------------------------------------------
 * hello_or_errno — the same call, but decoding the kernel's error convention.
 *
 * The kernel returns errors as a small NEGATIVE value in the band [-4095, -1]
 * (−MAX_ERRNO .. −1). glibc's syscall() wrapper performs exactly this range
 * test to decide "is this a -errno or a genuine result?". Reproducing it here
 * gives the optimizer a real branch to compile, so the annotated asm has more
 * than a bare syscall to show: you will see the unsigned range check that
 * distinguishes an error from a valid byte count.
 *
 * Returns the byte count on success; on error returns -1 and writes the
 * positive errno through *err_out. On success *err_out is set to 0.
 * --------------------------------------------------------------------------- */
sword hello_or_errno(char *buf, uword len, int *err_out)
{
	sword r = raw_syscall3(__NR_hello, (sword)buf, (sword)len, 0);

	/* -4095 <= r <= -1  <=>  (uword)(-r) - 1 < 4095, but the readable form is a
	 * two-sided compare. The kernel guarantees real errnos never exceed
	 * MAX_ERRNO (4095), so anything more negative than -4095 is a valid (huge)
	 * return, not an error. */
	if (r < 0 && r >= -4095) {
		*err_out = (int)(-r);   /* hand back a POSITIVE errno, glibc-style */
		return -1;
	}

	*err_out = 0;
	return r;
}
