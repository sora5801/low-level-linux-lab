/* ===========================================================================
 * vuln_rop.c — RUNG 3: Return-Oriented Programming (ROP), defeating NX properly.
 * ===========================================================================
 *
 *   >>> LEGAL / SAFETY: deliberately vulnerable; your own machine only.
 *   >>> Matching exploit: ../exploits/exploit_rung3_rop.py
 *   >>> Gadget finder:    ../exploits/ropgadget.py
 *
 * WHY THIS EXISTS: ret2libc (Rung 2) reuses ONE function (system). But if the
 * defender removes/hardens system, or you need to do something libc has no
 * single function for, you need finer-grained code reuse. ROP (Shacham, 2007)
 * is the general form: chain dozens of tiny "gadgets" — each a few instructions
 * ending in `ret` — so that each gadget does one small step and the `ret` jumps
 * to the next gadget whose address you planted on the stack. The stack becomes
 * a little PROGRAM; the `ret` instruction is its instruction-fetch. Because
 * every gadget is existing executable code, NX is irrelevant: we never execute
 * data, only code that was already there. NX cannot stop this — which is the
 * whole lesson, and why the *next* defenses (ASLR/PIE, canaries, CFI, CET
 * shadow stacks) had to be invented.
 *
 * OUR CHAIN builds a raw execve("/bin/sh", NULL, NULL) syscall by loading the
 * argument registers with `pop` gadgets and then hitting a `syscall` gadget:
 *      rax = 59 (SYS_execve),  rdi = "/bin/sh",  rsi = 0,  rdx = 0,  syscall.
 *
 * DETERMINISM NOTE (honest disclosure): a real ROP attack scavenges gadgets out
 * of whatever code the binary/libc happens to contain, and which gadgets exist
 * varies by build. To make this LAB reproduce identically for every reader, we
 * PLANT a known gadget set (the asm block below) and a "/bin/sh" string in the
 * binary. ropgadget.py still finds them by scanning bytes, exactly as it would
 * find scavenged gadgets — the planting only removes build-to-build luck, it
 * does not change the technique you are learning.
 *
 * BUILD (Makefile target `rung3`): NX on, canary off, no-PIE so the planted
 * gadgets and the "/bin/sh" string sit at fixed, script-computable addresses.
 * (The MAIN executable of a no-PIE program is not randomized even when ASLR is
 * enabled — only PIE images move — so we need no libc base and no leak here.)
 *   clang -g -O0 -fno-stack-protector -no-pie -fno-pie \
 *         vuln_rop.c -o vuln_rop
 *
 * LAB SETUP: ASLR off is recommended for the whole lab, though this rung does
 * not strictly need it (no-PIE .text is already fixed):
 *   echo 0 | sudo tee /proc/sys/kernel/randomize_va_space
 *
 * DEFENSE PREVIEW: PIE + ASLR randomize gadget addresses (need a leak); a stack
 * canary stops the overflow before it reaches the return address at all; Intel
 * CET shadow stack keeps an unwritable copy of each return address and faults
 * when a `ret` target does not match; CET-IBT / forward-edge CFI reject calls to
 * non-entry points. Each raises the bar; none is free (see README).
 * ===========================================================================
 */

#include <stdio.h>      /* puts, setvbuf                                       */
#include <unistd.h>     /* read(2)                                             */

/* ---------------------------------------------------------------------------
 * The gadget farm. A file-scope inline-asm block emits raw gadget bytes with
 * global labels so (a) the linker keeps them (global symbols survive without
 * --gc-sections) and (b) both nm and ropgadget.py can locate them. Each is the
 * minimal "do one thing, then ret" primitive ROP is built from. We take their
 * addresses in a volatile sink below so no toolchain is tempted to drop them.
 *
 * Encodings (what ropgadget.py matches on):
 *   pop %rdi ; ret  ==  5f c3       pop %rsi ; ret  ==  5e c3
 *   pop %rdx ; ret  ==  5a c3       pop %rax ; ret  ==  58 c3
 *   syscall  ; ret  ==  0f 05 c3
 * --------------------------------------------------------------------------- */
__asm__(
    ".text\n"
    ".globl gadget_pop_rdi\n" "gadget_pop_rdi:\n" "\tpop %rdi\n" "\tret\n"
    ".globl gadget_pop_rsi\n" "gadget_pop_rsi:\n" "\tpop %rsi\n" "\tret\n"
    ".globl gadget_pop_rdx\n" "gadget_pop_rdx:\n" "\tpop %rdx\n" "\tret\n"
    ".globl gadget_pop_rax\n" "gadget_pop_rax:\n" "\tpop %rax\n" "\tret\n"
    ".globl gadget_syscall\n" "gadget_syscall:\n" "\tsyscall\n" "\tret\n"
);

/* Declarations so C can take the gadgets' addresses (for the anti-DCE sink). */
extern char gadget_pop_rdi[], gadget_pop_rsi[], gadget_pop_rdx[],
            gadget_pop_rax[], gadget_syscall[];

/* The string execve() will run. Global -> fixed address under -no-pie; nm finds
 * it. (Static glibc also embeds "/bin/sh", but we plant our own for clarity.) */
char binsh_str[] = "/bin/sh";

/* ---------------------------------------------------------------------------
 * vuln — the same unbounded read() overflow. Only the payload differs: instead
 * of one address (Rung 1/2) it is a whole chain of gadget addresses + data.
 * --------------------------------------------------------------------------- */
static void vuln(void)
{
    char buf[64];
    ssize_t n = read(0, buf, 1024);     /* THE BUG: 1024 bytes into 64           */
    (void)n;
}

int main(void)
{
    /* Anti-dead-code sink: reference every gadget + the string through a
     * volatile pointer so the optimizer/linker cannot argue they are unused.
     * (Purely to keep the lab deterministic; it does nothing at runtime.) */
    static void *volatile keep[6];
    keep[0] = gadget_pop_rdi;  keep[1] = gadget_pop_rsi;
    keep[2] = gadget_pop_rdx;  keep[3] = gadget_pop_rax;
    keep[4] = gadget_syscall;  keep[5] = binsh_str;

    setvbuf(stdout, NULL, _IONBF, 0);
    puts("rung3: ROP  (NX on, static+no-pie). Chain gadgets -> execve(\"/bin/sh\").");
    vuln();
    puts("vuln() returned normally — no exploit fired.");
    return 0;
}
