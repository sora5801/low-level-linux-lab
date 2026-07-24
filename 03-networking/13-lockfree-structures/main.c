/* ===========================================================================
 * main.c — a multithreaded stress harness that proves the three structures
 *          keep their invariants under real MPMC contention.
 * ===========================================================================
 *
 * Each test uses a CONSERVATION invariant, which is the gold standard for
 * checking a concurrent container without a reference lock:
 *
 *   - Stack / Queue: T threads each insert OPS distinct values, then (after a
 *     barrier) each removes OPS values. Because #inserts == #removes, the
 *     container must end EMPTY, and the sum of everything removed must equal the
 *     sum of everything inserted (1+2+...+N). A lost node lowers the sum; a
 *     duplicated node (double-free / ABA corruption) raises it or crashes. Any
 *     remove that returns "empty" in the drain phase is itself a lost element.
 *
 *   - Map: T threads insert disjoint key ranges; afterwards every key must read
 *     back with its exact value, and tombstoned keys must miss.
 *
 * Platform: Linux (pthreads, C11 atomics). Build: see the Makefile. This file
 * is NOT run during asm generation; it is written to be correct on Linux.
 * ===========================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <pthread.h>

#include "treiber_stack.h"
#include "ms_queue.h"
#include "hashmap.h"
#include "hazard.h"

/* Smallest power of two >= n (for sizing the map's slot array). */
static size_t next_pow2(size_t n)
{
    size_t p = 1;
    while (p < n) p <<= 1;
    return p;
}

/* N*(N+1)/2 as a 64-bit value: the expected sum of keys 1..N. Values are
 * bounded so this does not overflow for the demo's default sizes. */
static uint64_t triangular(uint64_t n) { return n * (n + 1) / 2; }

/* =========================== Treiber stack test =========================== */

typedef struct {
    int                tid;
    long               ops;
    treiber_stack     *stack;
    _Atomic(uint64_t) *gsum;    /* global sum of popped values */
    _Atomic(int)      *gerr;    /* set nonzero on any invariant violation */
    pthread_barrier_t *barrier;
} stack_arg;

static void *stack_worker(void *p)
{
    stack_arg *a = (stack_arg *)p;

    /* Phase 1: push OPS distinct, nonzero values unique to this thread. */
    for (long i = 0; i < a->ops; i++) {
        lf_value v = (lf_value)((uint64_t)a->tid * (uint64_t)a->ops + (uint64_t)i + 1);
        if (ts_push(a->stack, v) != 0)
            atomic_store_explicit(a->gerr, 1, memory_order_relaxed);   /* OOM */
    }

    /* Barrier: make sure ALL pushes are done before ANY drain begins, so the
     * "drain must never see empty" invariant holds exactly. */
    pthread_barrier_wait(a->barrier);

    /* Phase 2: pop exactly OPS values. With #push == #pop globally, every pop
     * here must succeed; an empty return means an element was lost. */
    uint64_t local = 0;
    for (long i = 0; i < a->ops; i++) {
        lf_value v;
        if (!ts_pop(a->stack, &v)) {
            atomic_store_explicit(a->gerr, 1, memory_order_relaxed);
            break;
        }
        local += (uint64_t)v;
    }
    atomic_fetch_add_explicit(a->gsum, local, memory_order_relaxed);
    return NULL;
}

static int test_stack(int threads, long ops)
{
    treiber_stack st;
    ts_init(&st);

    _Atomic(uint64_t) gsum;  atomic_init(&gsum, 0);
    _Atomic(int)      gerr;  atomic_init(&gerr, 0);
    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier, NULL, (unsigned)threads);

    pthread_t *tids = malloc(sizeof *tids * threads);
    stack_arg *args = malloc(sizeof *args * threads);
    for (int i = 0; i < threads; i++) {
        args[i] = (stack_arg){ i, ops, &st, &gsum, &gerr, &barrier };
        pthread_create(&tids[i], NULL, stack_worker, &args[i]);
    }
    for (int i = 0; i < threads; i++)
        pthread_join(tids[i], NULL);

    uint64_t got      = atomic_load(&gsum);
    uint64_t expected = triangular((uint64_t)threads * (uint64_t)ops);
    int      err      = atomic_load(&gerr);

    /* The stack must also be empty now. */
    lf_value leftover;
    int not_empty = ts_pop(&st, &leftover);

    int ok = (!err && got == expected && !not_empty);
    printf("[stack]  threads=%d ops=%ld  sum=%" PRIu64 " expected=%" PRIu64
           "  empty=%s  -> %s\n",
           threads, ops, got, expected, not_empty ? "NO" : "yes",
           ok ? "PASS" : "FAIL");

    pthread_barrier_destroy(&barrier);
    free(tids); free(args);
    ts_destroy(&st);
    return ok ? 0 : 1;
}

/* ============================ MS queue test ============================== */

typedef struct {
    int                tid;
    long               ops;
    ms_queue          *queue;
    hp_thread         *hp;      /* main-owned handle (see note in test_queue) */
    hp_domain         *dom;
    _Atomic(uint64_t) *gsum;
    _Atomic(int)      *gerr;
    pthread_barrier_t *barrier;
} queue_arg;

static void *queue_worker(void *p)
{
    queue_arg *a = (queue_arg *)p;

    /* Register this thread with the hazard domain. The handle STORAGE is owned
     * by main (a->hp points into an array that outlives this thread) so that any
     * nodes still deferred in its retire list at exit are not lost — main
     * reclaims them after join. Registration itself is lock-free (one fetch_add
     * for the slot index). */
    if (hp_thread_init(a->hp, a->dom) != 0) {
        atomic_store_explicit(a->gerr, 1, memory_order_relaxed);
        pthread_barrier_wait(a->barrier);   /* still hit the barrier to avoid deadlock */
        return NULL;
    }

    for (long i = 0; i < a->ops; i++) {
        lf_value v = (lf_value)((uint64_t)a->tid * (uint64_t)a->ops + (uint64_t)i + 1);
        if (msq_enqueue(a->queue, a->hp, v) != 0)
            atomic_store_explicit(a->gerr, 1, memory_order_relaxed);
    }

    pthread_barrier_wait(a->barrier);

    uint64_t local = 0;
    for (long i = 0; i < a->ops; i++) {
        lf_value v;
        if (!msq_dequeue(a->queue, a->hp, &v)) {
            atomic_store_explicit(a->gerr, 1, memory_order_relaxed);
            break;
        }
        local += (uint64_t)v;
    }
    atomic_fetch_add_explicit(a->gsum, local, memory_order_relaxed);
    /* NOTE: we do NOT flush here. A node this thread retired may still be
     * hazarded by another thread that is not finished; flushing now would leave
     * those in our list and lose them when we exit. main flushes every handle
     * once ALL threads have joined and no hazard slots remain — see below. */
    return NULL;
}

static int test_queue(int threads, long ops)
{
    ms_queue q;
    if (msq_init(&q) != 0) { fprintf(stderr, "queue init OOM\n"); return 1; }

    hp_domain dom;
    hp_domain_init(&dom);

    _Atomic(uint64_t) gsum;  atomic_init(&gsum, 0);
    _Atomic(int)      gerr;  atomic_init(&gerr, 0);
    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier, NULL, (unsigned)threads);

    pthread_t *tids = malloc(sizeof *tids * threads);
    queue_arg *args = malloc(sizeof *args * threads);
    /* Hazard handles are owned HERE, not on the workers' stacks, so their retire
     * lists survive thread exit and we can reclaim any stragglers after join. */
    hp_thread *hps = malloc(sizeof *hps * threads);
    for (int i = 0; i < threads; i++) {
        args[i] = (queue_arg){ i, ops, &q, &hps[i], &dom, &gsum, &gerr, &barrier };
        pthread_create(&tids[i], NULL, queue_worker, &args[i]);
    }
    for (int i = 0; i < threads; i++)
        pthread_join(tids[i], NULL);

    uint64_t got      = atomic_load(&gsum);
    uint64_t expected = triangular((uint64_t)threads * (uint64_t)ops);
    int      err      = atomic_load(&gerr);

    /* Single-threaded now: probe once for emptiness (reusing a registered
     * handle), then flush EVERY handle. With no worker running, no hazard slot
     * names any node, so every deferred node is reclaimed here — no leak. */
    lf_value leftover;
    int not_empty = msq_dequeue(&q, &hps[0], &leftover);
    for (int i = 0; i < threads; i++)
        hp_thread_flush(&hps[i]);

    int ok = (!err && got == expected && !not_empty);
    printf("[queue]  threads=%d ops=%ld  sum=%" PRIu64 " expected=%" PRIu64
           "  empty=%s  -> %s\n",
           threads, ops, got, expected, not_empty ? "NO" : "yes",
           ok ? "PASS" : "FAIL");

    pthread_barrier_destroy(&barrier);
    free(tids); free(args); free(hps);
    msq_destroy(&q);
    return ok ? 0 : 1;
}

/* ============================= Hash map test ============================= */

typedef struct {
    int           tid;
    long          ops;
    hashmap      *map;
    _Atomic(int) *gerr;
} map_arg;

static void *map_worker(void *p)
{
    map_arg *a = (map_arg *)p;
    for (long i = 0; i < a->ops; i++) {
        uintptr_t key = (uintptr_t)((uint64_t)a->tid * (uint64_t)a->ops + (uint64_t)i + 1);
        uintptr_t val = key;             /* value == key: easy to verify later */
        if (hm_put(a->map, key, val) != 0)
            atomic_store_explicit(a->gerr, 1, memory_order_relaxed);   /* full */
    }
    return NULL;
}

static int test_map(int threads, long ops)
{
    uint64_t n = (uint64_t)threads * (uint64_t)ops;

    hashmap m;
    /* Keep the load factor <= ~0.5 for healthy linear-probe performance. */
    if (hm_init(&m, next_pow2((size_t)n * 2)) != 0) {
        fprintf(stderr, "map init failed\n");
        return 1;
    }

    _Atomic(int) gerr; atomic_init(&gerr, 0);

    pthread_t *tids = malloc(sizeof *tids * threads);
    map_arg   *args = malloc(sizeof *args * threads);
    for (int i = 0; i < threads; i++) {
        args[i] = (map_arg){ i, ops, &m, &gerr };
        pthread_create(&tids[i], NULL, map_worker, &args[i]);
    }
    for (int i = 0; i < threads; i++)
        pthread_join(tids[i], NULL);

    /* Verify: every inserted key reads back with the correct value. */
    long missing = 0, wrong = 0;
    for (uint64_t k = 1; k <= n; k++) {
        uintptr_t v;
        if (!hm_get(&m, (uintptr_t)k, &v))       missing++;
        else if (v != (uintptr_t)k)              wrong++;
    }

    /* Exercise deletion: tombstone every even key, then re-check. */
    for (uint64_t k = 2; k <= n; k += 2)
        hm_remove(&m, (uintptr_t)k);
    long bad_delete = 0;
    for (uint64_t k = 1; k <= n; k++) {
        uintptr_t v;
        int present = hm_get(&m, (uintptr_t)k, &v);
        int should  = (k % 2 == 1);              /* odds survive, evens removed */
        if (present != should) bad_delete++;
    }

    int err = atomic_load(&gerr);
    int ok  = (!err && missing == 0 && wrong == 0 && bad_delete == 0);
    printf("[map]    threads=%d ops=%ld  keys=%" PRIu64
           "  missing=%ld wrong=%ld delete_errs=%ld  -> %s\n",
           threads, ops, n, missing, wrong, bad_delete, ok ? "PASS" : "FAIL");

    free(tids); free(args);
    hm_destroy(&m);
    return ok ? 0 : 1;
}

int main(int argc, char **argv)
{
    /* Defaults chosen to finish in well under a second while still generating
     * heavy contention. Override: ./stress <threads> <ops-per-thread>. */
    int  threads = 4;
    long ops     = 200000;
    if (argc > 1) threads = atoi(argv[1]);
    if (argc > 2) ops     = atol(argv[2]);
    if (threads < 1) threads = 1;
    if (threads > HP_MAX_THREADS) threads = HP_MAX_THREADS;   /* hazard-slot cap */
    if (ops < 1) ops = 1;

    printf("lock-free stress: %d threads x %ld ops each\n", threads, ops);

    int fails = 0;
    fails += test_stack(threads, ops);
    fails += test_queue(threads, ops);
    fails += test_map(threads, ops);

    printf("\n%s\n", fails == 0 ? "ALL TESTS PASSED" : "SOME TESTS FAILED");
    return fails == 0 ? 0 : 1;
}
