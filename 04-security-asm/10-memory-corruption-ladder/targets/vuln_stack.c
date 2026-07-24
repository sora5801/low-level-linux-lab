/* ===========================================================================
 * vuln_stack.c — RUNG 1: the classic stack smash (Aleph One, 1996).
 * ===========================================================================
 *
 *   >>> LEGAL / SAFETY: This program is DELIBERATELY broken. Compile and run it
 *   >>> ONLY on your own machine, only for learning. It reads bytes from stdin
 *   >>> straight over the end of a stack buffer — that is the whole point. The
 *   >>> matching exploit is ../exploits/exploit_rung1_stack.py.
 *
 * THE BUG: vuln() has a 64-byte stack buffer and read(2)s up to 1024 bytes into
 * it. Anything past byte 64 corrupts the frame; past the saved rbp it overwrites
 * the SAVED RETURN ADDRESS. We compile with an EXECUTABLE stack (`-z execstack`)
 * and NX effectively off, so the attacker's plan is the original one from
 * "Smashing the Stack for Fun and Profit": inject machine code (shellcode) into
 * the buffer and set the return address to point back into the buffer. When
 * vuln() returns, the CPU executes the attacker's bytes.
 *
 * BUILD (see Makefile target `rung1`):
 *   clang -g -O0 -fno-stack-protector -z execstack -no-pie -fno-pie \
 *         vuln_stack.c -o vuln_stack
 *     -fno-stack-protector : no canary between buf and the return address, so the
 *                            overwrite is not detected. (Canary = Rung-1 defense.)
 *     -z execstack         : mark the stack executable (removes NX for the stack),
 *                            so injected shellcode can run. (NX = Rung-1 defense.)
 *     -no-pie -fno-pie     : fixed load address; combined with ASLR off, the
 *                            stack lives at a predictable address. (ASLR/PIE = def.)
 *
 * LAB SETUP (ASLR off, so the leaked/observed stack address is stable):
 *   echo 0 | sudo tee /proc/sys/kernel/randomize_va_space
 *
 * DEFENSE PREVIEW (full discussion in README): a stack canary would catch the
 * overwrite before the `ret`; NX/DEP would fault when the CPU tried to execute
 * stack bytes; ASLR would hide the stack address the exploit jumps to.
 * ===========================================================================
 */

#include <stdio.h>      /* puts, printf, setvbuf                               */
#include <unistd.h>     /* read(2)                                             */

/* ---------------------------------------------------------------------------
 * vuln — copies unbounded stdin into a fixed 64-byte stack buffer.
 *
 * We deliberately LEAK the address of `buf`. In a real target you would not get
 * this for free; you would obtain a stack address from a separate info-leak bug,
 * or (as in this lab) read it from gdb with ASLR disabled. Printing it here just
 * lets the exploit script run end-to-end without gdb, so you can focus on the
 * overflow mechanics rather than on address discovery.
 * --------------------------------------------------------------------------- */
static void vuln(void)
{
    char buf[64];                       /* THE target buffer: 64 bytes, on stack */

    /* The stand-in info leak. %p prints the pointer; this is buf[0]'s address,
     * i.e. where the injected shellcode will sit and where we make `ret` land. */
    printf("[leak] buf @ %p\n", (void *)buf);

    /* THE BUG. read(2) is binary-safe (NUL bytes and address bytes pass through
     * unharmed, unlike strcpy/gets), so the attacker can send raw shellcode plus
     * a raw 8-byte return address. We ask for up to 1024 bytes into 64 — every
     * byte past index 63 corrupts the frame; past the saved rbp (see demo.c for
     * the exact 88-byte offset) it overwrites the saved return address.
     *
     * read() returns the byte count or -1/errno on error; we ignore it because
     * for the lab the interesting path is "attacker sent a full payload". */
    ssize_t n = read(0, buf, 1024);
    (void)n;

    /* If we get here with a benign (short) input, vuln() returns normally. With
     * the exploit payload, the `ret` below jumps into buf instead. */
}

int main(void)
{
    /* Unbuffered stdout so the "[leak]" line reaches the exploit immediately,
     * before we block in read(). Buffered stdout could withhold it until exit. */
    setvbuf(stdout, NULL, _IONBF, 0);

    puts("rung1: classic stack overflow  (NX off via -z execstack, ASLR off).");
    puts("       send: <shellcode> + <pad to offset> + <addr of buf>");

    vuln();                             /* the frame that gets smashed           */

    /* Reached only for benign input. The exploit never lets control return here;
     * it diverts the `ret` inside vuln() into the injected shellcode instead. */
    puts("vuln() returned normally — no exploit fired.");
    return 0;
}
