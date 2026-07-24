/* ===========================================================================
 * main.c — a fork-join demo that exercises the work-stealing pool.
 * ===========================================================================
 *
 * The workload is a divide-and-conquer sum of the integers [0, N): a task either
 * sums a small leaf range directly, or splits its range in half and submits the
 * two halves as new tasks. This is the canonical shape that makes a work-stealing
 * deque shine, and it demonstrates BOTH submission paths:
 *
 *   - the ROOT task is submitted from main() — an external thread — so it lands
 *     in the injector;
 *   - every recursive split runs INSIDE a worker, so it pushes onto that worker's
 *     own deque (the lock-free local fast path).
 *
 * Because the owner takes LIFO (its freshly-pushed small halves, cache-hot) while
 * idle thieves steal FIFO (the oldest, largest un-split ranges), a single idle
 * worker steals a big chunk of the tree per steal and the load balances itself.
 * We print each worker's task count at the end so you can see that happen.
 *
 * Build & run (Linux/WSL):  make && ./demo [N] [workers]
 * ===========================================================================
 */
/* Expose POSIX clock_gettime / CLOCK_MONOTONIC even under a strict -std=c11,
 * where glibc otherwise hides them. (Harmless under -std=gnu11, which the
 * Makefile uses.) Must precede any system header. */
#define _POSIX_C_SOURCE 200809L

#include "threadpool.h"

#include <stdatomic.h>   /* the shared accumulator                              */
#include <stdio.h>       /* printf                                              */
#include <stdlib.h>      /* malloc, free, strtol, EXIT_*                        */
#include <time.h>        /* clock_gettime, CLOCK_MONOTONIC                      */

/* Leaf size: ranges this small or smaller are summed inline instead of split.
 * Too small and we drown in task-creation overhead; too large and there is not
 * enough parallelism to steal. A few thousand is a good middle ground. */
#define LEAF 4096

/* The one piece of shared mutable state: the running total. Every leaf adds its
 * partial sum here. relaxed is enough — we only require that all the adds have
 * landed by the time tp_wait() returns (which they have: each add happens-before
 * that task's pending-decrement, and tp_wait synchronizes with pending hitting
 * zero), and addition is commutative so ordering among the adds is irrelevant. */
static _Atomic long g_sum;

/* The argument each task carries. Allocated by whoever submits the task, freed by
 * the task itself — see the free() at the end of sum_task. `p` is threaded through
 * so a task can submit its children. */
typedef struct {
    tp_pool *p;
    long     lo;   /* inclusive */
    long     hi;   /* exclusive */
} Range;

/* Allocate + submit one child range. Kept tiny so the recursion reads cleanly.
 * On allocation failure we print and drop the child; the demo will then report a
 * wrong sum, which is the honest signal that something went wrong. */
static void submit_range(tp_pool *p, long lo, long hi);

/* The task body. */
static void sum_task(void *arg)
{
    Range *r = (Range *)arg;
    long lo = r->lo, hi = r->hi;

    if (hi - lo <= LEAF) {
        /* Leaf: sum this range directly and fold it into the global total. */
        long s = 0;
        for (long i = lo; i < hi; i++)
            s += i;
        atomic_fetch_add_explicit(&g_sum, s, memory_order_relaxed);
    } else {
        /* Internal node: split in half and hand both halves back to the pool.
         * These submit() calls run on a worker thread, so they push onto this
         * worker's own deque (the lock-free path). */
        long mid = lo + (hi - lo) / 2;
        submit_range(r->p, lo, mid);
        submit_range(r->p, mid, hi);
    }

    free(r);   /* we own our argument; release it now that we're done with it */
}

static void submit_range(tp_pool *p, long lo, long hi)
{
    Range *r = malloc(sizeof(*r));
    if (!r) {
        fprintf(stderr, "malloc(Range) failed; result will be short\n");
        return;
    }
    r->p = p;
    r->lo = lo;
    r->hi = hi;
    if (tp_submit(p, sum_task, r) != 0) {
        fprintf(stderr, "tp_submit failed; result will be short\n");
        free(r);
    }
}

/* Wall-clock seconds between two CLOCK_MONOTONIC samples. */
static double secs_between(struct timespec a, struct timespec b)
{
    return (double)(b.tv_sec - a.tv_sec) + (double)(b.tv_nsec - a.tv_nsec) / 1e9;
}

int main(int argc, char **argv)
{
    /* Defaults chosen so the sum (~5e15 for N=1e8) fits comfortably in int64. */
    long N       = (argc > 1) ? strtol(argv[1], NULL, 10) : 100000000L;
    int  workers = (argc > 2) ? (int)strtol(argv[2], NULL, 10) : 0; /* 0 = all CPUs */

    if (N < 0) {
        fprintf(stderr, "N must be >= 0\n");
        return EXIT_FAILURE;
    }

    tp_pool *p = tp_create(workers);
    if (!p) {
        fprintf(stderr, "tp_create failed\n");
        return EXIT_FAILURE;
    }
    atomic_store_explicit(&g_sum, 0, memory_order_relaxed);

    printf("summing [0, %ld) with %d worker(s), leaf=%d\n",
           N, tp_nworkers(p), LEAF);

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    /* Kick off the whole computation with the root task (external submit). */
    submit_range(p, 0, N);

    /* Block until the entire task tree has drained. */
    tp_wait(p);

    clock_gettime(CLOCK_MONOTONIC, &t1);

    long got      = atomic_load_explicit(&g_sum, memory_order_relaxed);
    /* Closed form: 0 + 1 + ... + (N-1) = N*(N-1)/2. Compute in a way that avoids
     * overflow for the demo's range by dividing the even operand first. */
    long expected = (N % 2 == 0) ? (N / 2) * (N - 1) : N * ((N - 1) / 2);

    printf("result   = %ld\n", got);
    printf("expected = %ld  -> %s\n", expected,
           (got == expected) ? "OK" : "MISMATCH");
    printf("elapsed  = %.3f s\n", secs_between(t0, t1));

    /* Show load balance: how many tasks each worker executed. With stealing the
     * counts should be within the same order of magnitude across workers even
     * though only worker(s) that grabbed the root did any pushing. */
    printf("tasks per worker:");
    for (int i = 0; i < tp_nworkers(p); i++)
        printf(" [%d]=%ld", i, tp_worker_tasks(p, i));
    printf("\n");

    tp_destroy(p);   /* graceful: we already tp_wait()'d, so nothing is abandoned */
    return (got == expected) ? EXIT_SUCCESS : EXIT_FAILURE;
}
