/* ===========================================================================
 * vuln_ret2libc.c — RUNG 2: return-to-libc, defeating NX.
 * ===========================================================================
 *
 *   >>> LEGAL / SAFETY: deliberately vulnerable; your own machine only.
 *   >>> Matching exploit: ../exploits/exploit_rung2_ret2libc.py
 *
 * WHAT CHANGED SINCE RUNG 1: we no longer mark the stack executable. NX/DEP is
 * ON (the default). Injected shellcode on the stack now faults the instant the
 * CPU tries to execute it — so Rung 1's trick is dead. NX was the defense that
 * ended Rung 1; this rung is how attackers answered it.
 *
 * THE IDEA (Solar Designer, 1997): don't inject NEW code — REUSE code that is
 * already executable. libc is mapped executable and contains system(). So we
 * overwrite the return address with the address of system(), and arrange the
 * stack so that when system() runs, its first argument (rdi, per the SysV ABI)
 * points at the string "/bin/sh" (also already sitting inside libc). Result:
 * system("/bin/sh") — a shell — using only code the process already trusted.
 *
 * To put "/bin/sh" into rdi we need one tiny building block: a `pop rdi; ret`
 * gadget (pop the next stack word into rdi, then return to the next word). The
 * overwritten stack therefore reads, top to bottom:
 *
 *      [ saved ret ] -> pop_rdi_gadget   # loads rdi from the next word...
 *      [   +8     ] -> &"/bin/sh"        # ...this word -> rdi = "/bin/sh"
 *      [  +16     ] -> ret               # alignment pad (keep rsp 16-aligned
 *                                        #   so movaps inside system won't #GP)
 *      [  +24     ] -> &system           # ret into system(); rdi already set
 *
 * BUILD (Makefile target `rung2`): NX stays on, canary off, dynamic libc.
 *   clang -g -O0 -fno-stack-protector -no-pie -fno-pie \
 *         vuln_ret2libc.c -o vuln_ret2libc
 *   (No -z execstack, so the stack is non-executable — that is the point.)
 *
 * LAB SETUP: ASLR off, so libc loads at a fixed base each run:
 *   echo 0 | sudo tee /proc/sys/kernel/randomize_va_space
 *
 * DEFENSE PREVIEW: ASLR is the mitigation here — randomizing libc's base means
 * the attacker cannot hardcode &system / &"/bin/sh" and must first LEAK a libc
 * address. Full RELRO + BIND_NOW hardens the GOT (a related target); CFI /
 * CET-IBT would reject a `ret`/`call` landing in the middle of system's ABI.
 * ===========================================================================
 */

#include <stdio.h>      /* puts, setvbuf                                       */
#include <unistd.h>     /* read(2)                                             */

/* ---------------------------------------------------------------------------
 * vuln — same shape as Rung 1: unbounded read() into a fixed stack buffer.
 * The overflow mechanism is identical; only the PAYLOAD differs (a chain of
 * addresses instead of shellcode), because the stack is no longer executable.
 * --------------------------------------------------------------------------- */
static void vuln(void)
{
    char buf[64];                       /* fixed buffer; overflow reaches ret    */

    /* No address leak this time: the exploit resolves &system and &"/bin/sh"
     * from the target's OWN libc mapping (/proc/<pid>/maps with ASLR off). That
     * is closer to real ret2libc work, where you compute libc addresses from a
     * known base rather than being handed a stack pointer. */
    ssize_t n = read(0, buf, 1024);     /* THE BUG: 1024 bytes into 64           */
    (void)n;
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    puts("rung2: ret2libc  (NX on, ASLR off). Overwrite ret -> system(\"/bin/sh\").");
    vuln();
    puts("vuln() returned normally — no exploit fired.");
    return 0;
}
