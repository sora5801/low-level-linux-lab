/* ===========================================================================
 * crackme.c — a small serial validator with two "light" anti-debug tricks.
 * ===========================================================================
 *
 * THIS IS *YOUR* BINARY. Compile it, then reverse it: read its disassembly
 * (objdump/Ghidra), trace it (gdb), defeat its anti-debugging, and write a
 * keygen. Everything here is legal because you own every byte — see README.md.
 *
 * WHAT IT DOES
 *   crackme <username> <serial>
 *   -> exit 0 and print "Correct!" iff serial == transform(username)
 *
 * The transform lives in serial.h (shared with keygen.c so they can never
 * disagree). The interesting part of *this* file is the two anti-debug checks
 * wrapped around the validation, included specifically so you can learn how
 * they work and how they are bypassed:
 *
 *   1. ptrace(PTRACE_TRACEME) self-attach  — the most common Linux anti-debug.
 *   2. rdtsc timing gate                    — measure wall-clock cycles across a
 *                                             tiny loop; a single-stepping
 *                                             debugger blows the budget.
 *
 * Both are DELIBERATELY weak. The lesson (see README "Defense") is that
 * client-side anti-tamper only raises the cost of analysis a little; it is
 * never a security boundary. We ship the bypasses too.
 *
 * Build (Linux/WSL) — see Makefile for the mitigation flags and why:
 *   clang -O1 -no-pie -fno-stack-protector crackme.c -o crackme
 * ===========================================================================
 */

#define _GNU_SOURCE          /* expose ptrace() and friends from glibc headers  */
#include <stdio.h>           /* printf/fprintf                                  */
#include <string.h>          /* strlen                                          */
#include <stdint.h>          /* uint64_t/uint32_t                               */
#include <sys/ptrace.h>      /* ptrace(), PTRACE_TRACEME                         */

#include "serial.h"          /* key_from_name / format_serial / ct_equal        */

/* --------------------------------------------------------------------------
 * rdtsc — read the CPU's 64-bit Time-Stamp Counter (cycles since reset).
 *
 * `rdtsc` writes the low 32 bits into EAX and the high 32 into EDX (it does NOT
 * touch RAX/RDX's upper halves in a useful way — you must recombine EDX:EAX).
 * We tell the compiler EAX/EDX are outputs ("=a","=d") and stitch them into a
 * 64-bit value. It is not serializing (the CPU may reorder around it), which is
 * one reason this timing check is only "light" — but it is exactly the sequence
 * you will meet in real anti-debug and malware, so learn to recognize it:
 *
 *      rdtsc                 # EDX:EAX <- timestamp
 *      shl $32, %rdx ; or %rdx, %rax     (or the compiler's equivalent)
 *
 * WHY A DEBUGGER TRIPS IT: if you single-step (`si`) or sit at a breakpoint
 * between the two rdtsc reads, thousands to millions of cycles elapse instead
 * of a few hundred, so `t1 - t0` explodes past the threshold. WHY IT IS WEAK:
 * an ordinary context switch or interrupt also inflates the delta (false
 * positives), and a reverser can just run full-speed to the compare, or NOP the
 * branch out. See docs/writeup.md.
 * -------------------------------------------------------------------------- */
static inline uint64_t rdtsc(void)
{
    uint32_t lo, hi;
    __asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

/* Generous cycle budget for the timing gate. On a multi-GHz CPU the guarded
 * loop below is a few hundred cycles when run natively; we allow ~5,000,000 so
 * that ordinary scheduling jitter does not cause false positives, while a human
 * single-stepping still trips it easily. Tune-by-machine fragility is exactly
 * why timing checks are unreliable — that unreliability is the teaching point. */
#define TIMING_BUDGET_CYCLES  5000000ULL

/* Exit codes, distinct so you can tell *which* gate fired from `echo $?`:
 *   0  correct serial            2  detected under a debugger (ptrace)
 *   1  wrong serial / usage      3  timing gate tripped                         */
enum { EXIT_OK = 0, EXIT_WRONG = 1, EXIT_DEBUGGER = 2, EXIT_SLOW = 3 };

/* --------------------------------------------------------------------------
 * anti_debug_ptrace — the classic self-ptrace check.
 *
 * A process on Linux can have AT MOST ONE tracer. PTRACE_TRACEME asks "let my
 * parent trace me." If NO debugger is attached, the request succeeds (returns
 * 0) and simply designates the shell as a nominal tracer — harmless, the
 * program runs normally. If gdb (or strace, or another ptrace-based tool) is
 * ALREADY attached, it owns the single trace slot, so TRACEME fails with -1 and
 * errno==EPERM. That failure is the tell: "someone is already tracing me."
 *
 * Returns 1 if a debugger was detected, 0 otherwise.
 *
 * NOTE ON HOOKABILITY: we deliberately call libc's `ptrace()` (a PLT-resolved
 * dynamic symbol) rather than issuing the raw `syscall`, so that the LD_PRELOAD
 * bypass in libfakeptrace.c can intercept it. A raw-syscall version would
 * DEFEAT LD_PRELOAD — a real anti-debug hardening step — and would then have to
 * be beaten by patching or a seccomp/ptrace-based syscall filter. The writeup
 * covers both directions.
 * -------------------------------------------------------------------------- */
static int anti_debug_ptrace(void)
{
    /* ptrace(PTRACE_TRACEME, 0, 0, 0): args are (request, pid, addr, data);
     * for TRACEME the last three are ignored. glibc returns -1 on error. */
    long r = ptrace(PTRACE_TRACEME, 0, (void *)0, (void *)0);
    return (r == -1);      /* -1 => already traced => debugger present          */
}

/* --------------------------------------------------------------------------
 * anti_debug_timing — the rdtsc gate. Time a throwaway loop and compare the
 * elapsed cycles against a fixed budget. `volatile` on the accumulator stops
 * the optimizer from deleting the loop (dead-store elimination) — without it
 * the whole thing folds to a constant and there is nothing to time.
 *
 * Returns 1 if we appear to be single-stepped/slowed, 0 otherwise.
 * -------------------------------------------------------------------------- */
static int anti_debug_timing(void)
{
    uint64_t t0 = rdtsc();
    volatile uint64_t sink = 0;
    for (int i = 0; i < 1000; ++i)
        sink += (uint64_t)i;          /* trivial, cheap, un-eliminable work     */
    uint64_t dt = rdtsc() - t0;       /* cycles that actually elapsed           */
    (void)sink;
    return (dt > TIMING_BUDGET_CYCLES);
}

static void usage(const char *argv0)
{
    fprintf(stderr,
        "usage: %s <username> <serial>\n"
        "  serial format: GGGG-GGGG-GGGG-GGGG (uppercase hex)\n"
        "  hint: build the keygen and run  ./keygen <username>\n",
        argv0);
}

int main(int argc, char **argv)
{
    /* --- Gate 1: refuse to run under a debugger ---------------------------- */
    if (anti_debug_ptrace()) {
        fprintf(stderr, "nice try — I'm being traced. Bailing.\n");
        return EXIT_DEBUGGER;
    }

    /* --- Gate 2: refuse if we appear to be single-stepped ------------------ */
    if (anti_debug_timing()) {
        fprintf(stderr, "too slow — am I being single-stepped?\n");
        return EXIT_SLOW;
    }

    /* --- The actual license check ----------------------------------------- */
    if (argc != 3) {
        usage(argv[0]);
        return EXIT_WRONG;
    }

    const char *name   = argv[1];
    const char *serial = argv[2];

    /* Recompute the expected serial from the username. This is the value a
     * keygen must reproduce; recovering *this call* and its transform is the
     * heart of the reverse-engineering exercise. */
    uint64_t key = key_from_name(name);
    char expected[20];
    format_serial(key, expected);

    /* Length gate first (cheap, and it bounds the constant-time compare), then
     * the constant-time comparison itself. We compare exactly 19 bytes — the
     * fixed serial length — so the compare's timing never depends on the input.
     * (strlen of the *user-supplied* serial is not itself constant-time, but it
     * reveals only the length the user already typed, not the secret.) */
    if (strlen(serial) == 19 && ct_equal(serial, expected, 19)) {
        printf("Correct! Serial valid for user '%s'.\n", name);
        return EXIT_OK;
    }

    printf("Wrong serial for user '%s'.\n", name);
    return EXIT_WRONG;
}
