/* ===========================================================================
 * main.c — exercise the green-thread library: round-robin yielding, and join.
 * ===========================================================================
 *
 * This is the "does it actually work" driver. It runs two scenes:
 *
 *   Scene 1 — three worker tasks that count and gt_yield() between steps. The
 *             interleaved output ("A0 B0 C0 A1 B1 C1 ...") is the visible proof
 *             that a single OS thread is round-robining three independent
 *             stacks with no kernel involvement.
 *
 *   Scene 2 — a supervisor task that spawns two children and gt_join()s them,
 *             printing only after both have finished. This proves the blocking
 *             primitive and the wake-on-death path.
 *
 * Build & run (Linux / WSL):  make run
 * ===========================================================================
 */

#include "gt.h"
#include <stdio.h>

/* A worker counts to 3, yielding after each step so its peers interleave. The
 * `arg` carries a small integer id we cast back out. We also print a rough rsp
 * so you can SEE that each task runs on a different stack (the addresses differ
 * by roughly the mmap stride). */
static void worker(void *arg) {
    long id = (long)arg;
    for (int i = 0; i < 3; i++) {
        int marker;                                  /* a stack local: &marker ~ rsp */
        printf("  worker %ld: step %d   (stack ~ %p)\n",
               id, i, (void *)&marker);
        gt_yield();                                  /* cooperative hand-off        */
    }
    printf("  worker %ld: done\n", id);
    /* Returning here falls into gt_trampoline -> gt_coro_exit: the task dies. */
}

/* A child task for the join scene: does a little work, then returns (dies). */
static void child(void *arg) {
    long id = (long)arg;
    for (int i = 0; i < 2; i++) {
        printf("    child %ld working (%d)\n", id, i);
        gt_yield();
    }
    printf("    child %ld finished\n", id);
}

/* The supervisor spawns two children and waits for BOTH via gt_join before it
 * announces completion. Because join is cooperative, the supervisor yields to
 * the scheduler while the children run; it only resumes once each child dies. */
static void supervisor(void *arg) {
    (void)arg;
    printf("  supervisor: spawning two children\n");
    gt_task *a = gt_spawn(child, (void *)10, 0);
    gt_task *b = gt_spawn(child, (void *)20, 0);
    if (!a || !b) {
        fprintf(stderr, "  supervisor: spawn failed\n");
        return;
    }
    gt_join(a);                                      /* block until child 10 dies */
    gt_join(b);                                      /* block until child 20 dies */
    printf("  supervisor: both children joined; all done\n");
}

int main(void) {
    gt_init();

    printf("== Scene 1: three workers round-robin via yield ==\n");
    gt_spawn(worker, (void *)1, 0);
    gt_spawn(worker, (void *)2, 0);
    gt_spawn(worker, (void *)3, 0);
    gt_run();                                        /* drives them to completion */

    printf("\n== Scene 2: supervisor spawns and joins two children ==\n");
    gt_spawn(supervisor, NULL, 0);
    gt_run();

    printf("\nall scenes complete\n");
    return 0;
}
