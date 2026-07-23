/* ===========================================================================
 * hello.c — a complete Linux program with ZERO libc.
 * ===========================================================================
 *
 * Normally `#include <stdio.h>` + `printf("hi\n")` drags in the C runtime: the
 * `_start` entry point, `__libc_start_main`, TLS setup, the stdio buffering
 * machinery, and a `write(2)` wrapper. Here we throw ALL of that away and talk
 * to the kernel directly. The goal is to *feel the ABI*: there is nothing
 * between this C and the `syscall` instruction.
 *
 * Build (Linux):
 *     clang -nostdlib -static -no-pie -fno-stack-protector -O2 hello.c -o hello
 *     # -nostdlib : no CRT objects (crt1.o/crti.o) and no libc
 *     # -static   : no dynamic loader involved at all
 *     # -no-pie   : fixed load address, so no startup relocation is needed
 *     ./hello ; echo "exit=$?"      ->  Hello from a program with no libc!\nexit=7
 *
 * There is no `main()` here. The kernel, after `execve`, does not know or care
 * about `main`; it transfers control to the ELF entry point, which the linker
 * fills in from the symbol named `_start`. WE provide `_start`. That is the
 * real front door of every Linux process.
 *
 * The System V AMD64 ABI, which every function below obeys:
 *   - function args, in order:  rdi, rsi, rdx, rcx, r8, r9
 *   - a `syscall`:              number in rax; args in rdi, rsi, rdx, r10, r8, r9
 *                               return value in rax; rcx and r11 are DESTROYED
 *                               by the `syscall` instruction itself (it stashes
 *                               the return RIP in rcx and RFLAGS in r11).
 *   - return value:            rax
 * ===========================================================================
 */

/* Linux x86-64 syscall numbers we need. These live in <asm/unistd_64.h> on a
 * real system, but we are nolibc, so we spell them out. They are part of the
 * kernel's stable ABI: number 1 has meant write(2) for the life of x86-64. */
#define SYS_write        1
#define SYS_exit_group 231   /* exit the whole thread group, i.e. the process */

/* A `long` is 64 bits in the LP64 data model Linux uses on x86-64, so it holds
 * a pointer, a size, or a syscall return without truncation. We use it for all
 * kernel-facing values. */
typedef unsigned long usize;

/* ---------------------------------------------------------------------------
 * syscall3 — the heart of everything. Issue a 3-argument system call.
 *
 * We hand the compiler an inline-asm template and, crucially, a description of
 * which C values must live in which registers when the `syscall` executes:
 *
 *   "=a"(ret)  : after the instruction, read the result OUT of rax into `ret`.
 *   "a"(n)     : before it, put the syscall number `n` INTO rax.
 *   "D"(a1)    : put arg1 into rdi   (D is the ABI letter for rdi)
 *   "S"(a2)    : put arg2 into rsi   (S = rsi)
 *   "d"(a3)    : put arg3 into rdx   (d = rdx)
 *
 * The clobber list "rcx","r11","memory" tells the compiler the truth about the
 * `syscall` instruction: it always trashes rcx and r11, and (because it can
 * read/write our buffers) it may touch memory, so the compiler must not cache
 * memory across it. Forgetting "memory" here is a classic, maddening bug.
 * --------------------------------------------------------------------------- */
static inline long syscall3(long n, long a1, long a2, long a3)
{
    long ret;
    __asm__ volatile (
        "syscall"
        : "=a"(ret)                         /* out: rax -> ret               */
        : "a"(n), "D"(a1), "S"(a2), "d"(a3) /* in : rax,rdi,rsi,rdx          */
        : "rcx", "r11", "memory"            /* clobbered by the instruction  */
    );
    return ret;
}

/* strlen without <string.h>: walk bytes until the NUL terminator. The kernel's
 * write(2) needs a byte count, and our string is a compile-time literal, so we
 * measure it ourselves. This is O(n) and deliberately simple; project 02
 * (SIMD primitives) shows how glibc does 16 bytes at a time. */
static usize kstrlen(const char *s)
{
    const char *p = s;          /* p walks forward from the start            */
    while (*p) p++;             /* stop at the NUL byte (value 0)            */
    return (usize)(p - s);      /* pointer difference = number of bytes      */
}

/* write(2): kernel copies `len` bytes from `buf` to file descriptor `fd`.
 * fd 1 is stdout by convention (the shell wires it to your terminal). Returns
 * the number of bytes written, or a negative errno on failure. */
static long sys_write(long fd, const void *buf, usize len)
{
    return syscall3(SYS_write, fd, (long)buf, (long)len);
}

/* exit_group(2): terminate every thread in the process with `code` as the
 * status the shell sees in `$?`. It never returns — the process is gone — so we
 * mark it noreturn and, to satisfy the compiler, spin unreachably afterward. */
__attribute__((noreturn))
static void sys_exit(int code)
{
    /* exit_group takes ONE argument (the status), so a1=code, a2=a3=0. */
    syscall3(SYS_exit_group, code, 0, 0);
    __builtin_unreachable();    /* tell the optimizer control never gets here */
}

/* ---------------------------------------------------------------------------
 * _start — the process entry point. Not `main`: there is no CRT to call main.
 *
 * At the instant the kernel jumps here after execve:
 *   - rsp points at `argc`, with argv[]/envp[]/auxv[] laid out just above it;
 *   - rsp is 16-byte aligned (important: any `call` we make needs rsp%16==0);
 *   - no registers hold anything useful except rsp; there is no return address,
 *     because nobody "called" us — returning from _start would jump into
 *     garbage, which is why we must exit via a syscall instead.
 *
 * We ignore argc/argv here (project keeps it minimal) and just print + exit. */
__attribute__((noreturn))
void _start(void)
{
    static const char msg[] = "Hello from a program with no libc!\n";

    /* Print the message to stdout (fd 1). We pass the literal's length rather
     * than a hardcoded number so editing the string can't desync the count. */
    sys_write(1, msg, kstrlen(msg));

    /* Exit with status 7 so you can verify control reached here: `echo $?`. */
    sys_exit(7);
}
