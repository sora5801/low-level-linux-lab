/* ===========================================================================
 * ucontext_demo.c — the SAME idea, first shown with the libc ucontext API.
 * ===========================================================================
 *
 * Before hand-rolling a context switch in assembly (switch.S), it is worth
 * seeing that libc already ships one: the <ucontext.h> family. This file builds
 * a two-coroutine ping-pong with makecontext/swapcontext so you can compare the
 * "official" mechanism against ours. The lesson: our switch.S does exactly what
 * swapcontext does, minus the parts we don't need.
 *
 * THE FOUR CALLS
 * --------------
 *   getcontext(&uc)          snapshot the CURRENT machine context into `uc`
 *                            (registers + signal mask + a pointer to the stack).
 *   uc.uc_stack = {...}      point a context at a stack YOU provide.
 *   uc.uc_link  = &other     where to continue when this context's function
 *                            RETURNS (NULL => the program exits).
 *   makecontext(&uc, fn, 0)  rewrite `uc` so that swapcontext INTO it starts
 *                            executing fn() on uc's stack.
 *   swapcontext(&save, &to)  save the current context into `save` and resume
 *                            `to`. This is the actual switch — the analogue of
 *                            our gt_switch(from, to).
 *
 * WHY WE STILL HAND-ROLL IT (see switch.S and the README)
 * -------------------------------------------------------
 *   * swapcontext saves/restores the SIGNAL MASK, which costs a sigprocmask(2)
 *     syscall on every switch — pure overhead for a cooperative scheduler that
 *     never touches signals. Our switch is a dozen instructions, no syscall.
 *   * ucontext is deprecated by POSIX and absent on some libcs. Owning the
 *     switch means owning our portability.
 *   * You cannot SEE the ABI in a library call. switch.S is the teaching point.
 *
 * Build & run (Linux / WSL, glibc):  make ucontext && ./ucontext_demo
 * NOTE: <ucontext.h> is a Linux/glibc facility; this file does not build on
 * this Windows host — that is expected (the README says so).
 * ===========================================================================
 */

#define _XOPEN_SOURCE 700     /* expose the ucontext API from <ucontext.h>     */
#include <ucontext.h>
#include <stdio.h>
#include <stdlib.h>

/* Three contexts: main, and two "coroutines" that bounce control back and
 * forth. They are file-scope because makecontext's fn takes no closure — it can
 * only reach state through globals (a real limitation the assembly version
 * fixes by passing an argument in a register; see switch.S / gt_trampoline). */
static ucontext_t uc_main, uc_ping, uc_pong;

/* Each coroutine needs its own stack. 64 KiB is plenty here. We use a static
 * array rather than mmap to keep this illustration dependency-free; the real
 * library (gt.c) mmaps with a guard page. */
static char stack_ping[64 * 1024];
static char stack_pong[64 * 1024];

static int rounds = 3;

static void ping(void) {
    for (int i = 0; i < rounds; i++) {
        printf("ping %d\n", i);
        /* Save OUR context into uc_ping and resume uc_pong. When pong later
         * swaps back to us, execution continues right here, at the next loop. */
        swapcontext(&uc_ping, &uc_pong);
    }
    /* Returning falls through to uc_ping.uc_link (set to uc_main below). */
    printf("ping: done\n");
}

static void pong(void) {
    for (int i = 0; i < rounds; i++) {
        printf("pong %d\n", i);
        swapcontext(&uc_pong, &uc_ping);
    }
    printf("pong: done\n");
}

int main(void) {
    /* Build the ping context: snapshot, give it a stack, say where to go when
     * ping() returns (back to main), then graft ping() onto it. */
    if (getcontext(&uc_ping) == -1) { perror("getcontext"); return 1; }
    uc_ping.uc_stack.ss_sp   = stack_ping;
    uc_ping.uc_stack.ss_size = sizeof stack_ping;
    uc_ping.uc_link          = &uc_main;          /* return here when ping ends  */
    makecontext(&uc_ping, ping, 0);               /* 0 = ping() takes no args    */

    /* Build the pong context the same way; when pong() returns, hand off to
     * ping's context so ping can print its "done" and unwind to main. */
    if (getcontext(&uc_pong) == -1) { perror("getcontext"); return 1; }
    uc_pong.uc_stack.ss_sp   = stack_pong;
    uc_pong.uc_stack.ss_size = sizeof stack_pong;
    uc_pong.uc_link          = &uc_ping;
    makecontext(&uc_pong, pong, 0);

    printf("== ucontext ping-pong (%d rounds) ==\n", rounds);
    /* Kick it off: save main into uc_main, jump into ping. Control returns to
     * main only when the uc_link chain unwinds back to uc_main. */
    swapcontext(&uc_main, &uc_ping);

    printf("back in main; ucontext demo complete\n");
    return 0;
}
