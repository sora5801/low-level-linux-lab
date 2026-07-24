/* ===========================================================================
 * vuln_fmt.c — RUNG 4: format-string arbitrary write via %n.
 * ===========================================================================
 *
 *   >>> LEGAL / SAFETY: deliberately vulnerable; your own machine only.
 *   >>> Matching exploit: ../exploits/exploit_rung4_fmt.py
 *
 * A DIFFERENT PRIMITIVE. Rungs 1-3 all began with a buffer overflow. This bug
 * is not an overflow at all — the buffer is used safely. The flaw is passing
 * ATTACKER-CONTROLLED DATA as the FORMAT STRING of printf:
 *
 *      printf(line);           // BUG: should be printf("%s", line);
 *
 * printf walks the format string and, for each %-directive, fetches an argument
 * from the next register/stack slot (per the SysV varargs ABI: rsi, rdx, rcx,
 * r8, r9, then the stack). But the caller passed NO variadic arguments — so
 * printf reads whatever happens to be in those slots, which for the stack slots
 * is... the attacker's own buffer (`line` lives on the stack). Two consequences:
 *
 *   READ  primitive: %p / %x / %s leak stack, then arbitrary memory (%s follows
 *         a pointer the attacker placed in `line`).
 *   WRITE primitive: %n writes the NUMBER OF BYTES PRINTED SO FAR to an int* the
 *         attacker placed in `line`. Control the byte count (with width, e.g.
 *         %100c) and the pointer, and %n becomes a WRITE-WHAT-WHERE. That is the
 *         crown jewel: arbitrary memory write with no overflow, no shellcode.
 *
 * Our goal: flip the global `target` from 0 to 0xDEADBEEF using %n, which trips
 * the check at the end of vuln() and calls win() -> a shell. The exploit writes
 * the 4-byte value as two 16-bit `%hn` stores (writing ~3.7 billion bytes for a
 * single %n is not practical), aiming at &target and &target+2.
 *
 * BUILD (Makefile target `rung4`): no-PIE so &target is a fixed, nm-resolvable
 * address, and we intentionally do NOT enable the format-string mitigations:
 *   clang -g -O0 -no-pie -fno-pie -Wno-format-security \
 *         -U_FORTIFY_SOURCE vuln_fmt.c -o vuln_fmt
 *   (-Wno-format-security silences the warning that IS the primary defense;
 *    -U_FORTIFY_SOURCE removes glibc's runtime %n-in-writable-format guard.)
 *
 * LAB SETUP: ASLR off keeps stack contents / offsets stable across runs:
 *   echo 0 | sudo tee /proc/sys/kernel/randomize_va_space
 *
 * DEFENSE PREVIEW (README has the full table): compiling with
 * -Werror=format-security refuses to build `printf(line)` at all; glibc's
 * _FORTIFY_SOURCE=2 aborts at runtime when a %n appears in a writable-memory
 * format string; Full RELRO makes the GOT read-only (killing the classic
 * GOT-overwrite variant of this write); ASLR/PIE hide the address to aim at.
 * ===========================================================================
 */

#include <stdio.h>      /* printf, fgets, puts, setvbuf                        */
#include <stdlib.h>     /* system                                             */

/* The attacker's objective. `volatile` so the compiler cannot cache it in a
 * register and must re-read memory for the comparison — i.e. our %n write to
 * its address is actually observed. Global -> fixed address under -no-pie. */
volatile unsigned int target = 0;

/* The payoff. Reached only if the format-string write sets target correctly. */
static void win(void)
{
    puts("[win] target == 0xDEADBEEF via %n — spawning a shell.");
    system("/bin/sh");                  /* the reward for a correct write        */
}

/* ---------------------------------------------------------------------------
 * vuln — reads one line and (mis)uses it as a printf format string.
 * --------------------------------------------------------------------------- */
static void vuln(void)
{
    char line[256];                     /* input buffer; used SAFELY (bounded)   */

    /* fgets bounds the read to sizeof(line) — no overflow here. The vulnerability
     * is entirely in how `line` is USED two lines down, not in how it is filled. */
    if (!fgets(line, sizeof line, stdin))
        return;

    /* THE BUG: `line` is the format string. Every %-directive the attacker put
     * in `line` now drives printf to read (and, with %n, WRITE) memory. */
    printf(line);                       /* should be printf("%s", line);         */

    /* Did the %n write land the magic value? If so, we "unlocked" the shell.
     * In a real target the equivalent is overwriting a GOT entry or a saved
     * return address rather than a convenient global — same write primitive. */
    if (target == 0xDEADBEEFu)
        win();
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);

    /* Leak &target for convenience so the exploit can run end-to-end without a
     * separate step. It is also resolvable statically with `nm vuln_fmt | grep
     * target` because the binary is no-PIE; the exploit shows both paths. */
    printf("rung4: format-string %%n write. target @ %p (want 0xDEADBEEF)\n",
           (void *)&target);

    vuln();

    puts("done (target was not 0xDEADBEEF).");
    return 0;
}
