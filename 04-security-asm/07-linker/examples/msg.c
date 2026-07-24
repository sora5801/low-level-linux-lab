/* ===========================================================================
 * examples/msg.c — one half of the two-object demo we link with minild.
 * ===========================================================================
 *
 * This is deliberately nolibc (freestanding): it defines a couple of GLOBAL
 * symbols that the OTHER object (start.c) will reference. When the two .o files
 * are linked, minild must:
 *   - resolve start.c's undefined `print_hello`/`sys_exit` to the definitions
 *     here (cross-object symbol resolution),
 *   - relocate print_hello's reference to the local `hello` string in .rodata,
 *   - relocate `minild_hook` (a function pointer in writable .data) to the
 *     final address of print_hello — an R_X86_64_64 absolute relocation.
 *
 * Compile (from the project dir), forcing the Linux ABI and no PIC so the
 * relocations stay in the simple 32S/PC32/PLT32/64 family the linker handles:
 *   clang --target=x86_64-pc-linux-gnu -ffreestanding -fno-pic \
 *         -fno-stack-protector -c examples/msg.c -o msg.o
 * ===========================================================================
 */

/* Linux x86-64 syscall numbers (stable kernel ABI; no headers, we are nolibc). */
#define SYS_write        1
#define SYS_exit_group 231

typedef unsigned long usize;

/* Issue a 3-argument syscall. Identical contract to 01-nolibc-programs:
 * rax=number; args in rdi,rsi,rdx; rcx/r11 clobbered by the `syscall` insn. */
static inline long syscall3(long n, long a1, long a2, long a3) {
    long ret;
    __asm__ volatile ("syscall"
        : "=a"(ret)
        : "a"(n), "D"(a1), "S"(a2), "d"(a3)
        : "rcx", "r11", "memory");
    return ret;
}

/* The message. `static const` => it lands in .rodata. print_hello's reference
 * to it becomes a relocation against the .rodata SECTION symbol + an addend,
 * which minild resolves once .rodata has a final address. */
static const char hello[] = "Hello from a minild-linked program!\n";

/* print_hello — GLOBAL (start.c calls it across the object boundary). */
void print_hello(void) {
    /* write(1, hello, len). sizeof includes the NUL, so subtract 1. */
    syscall3(SYS_write, 1, (long)hello, (long)(sizeof(hello) - 1));
}

/* sys_exit — GLOBAL. exit_group(code); never returns. Marked noreturn so the
 * caller in start.c can be noreturn too. */
__attribute__((noreturn))
void sys_exit(int code) {
    syscall3(SYS_exit_group, code, 0, 0);
    __builtin_unreachable();
}

/* minild_hook — a GLOBAL function pointer, initialised to print_hello's
 * address. Because its initialiser is an address that only the LINKER knows,
 * the compiler emits an R_X86_64_64 relocation into a writable data section
 * (.data / .data.rel.ro): "store the final 8-byte address of print_hello here."
 * start.c reads this pointer and calls through it, exercising both a 64-bit
 * absolute relocation (here) and a cross-object data reference (there). */
void (*minild_hook)(void) = print_hello;
