/* ===========================================================================
 * sched.h — cooperative green threads / coroutines (add-on demo).
 * ===========================================================================
 *
 * Stand-in for sibling 02-systems-tools/18-green-threads. coroDemo() spins up two
 * cooperative tasks that hand control back and forth with gtYield(), driven by a
 * round-robin scheduler — all in ONE OS thread, with zero kernel involvement per
 * switch. Each task runs on its own mmap'd stack guarded by a PROT_NONE page.
 *
 * We build the switch on POSIX <ucontext.h> (getcontext/makecontext/swapcontext)
 * for a correct, portable core; the sibling project shows the ~20-instruction
 * hand-written register save (`switch.S`) that replaces it. Linux/x86-64.
 */
#ifndef LUMEN_SCHED_H
#define LUMEN_SCHED_H

int coroDemo(void);   /* run the two-coroutine demo; 0 on success              */

#endif /* LUMEN_SCHED_H */
