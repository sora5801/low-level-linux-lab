/* ===========================================================================
 * threadpool.c — a work-stealing thread pool over per-worker Chase-Lev deques.
 * ===========================================================================
 *
 * Read threadpool.h for the shape of the thing. This file wires together four
 * low-level Linux mechanisms, each flagged where it appears:
 *
 *   [ATOMICS]     the Chase-Lev deques and the eventcount (chase_lev.c + below)
 *   [FUTEX]       parking idle workers without burning CPU (futex_wait/wake)
 *   [AFFINITY]    sched_setaffinity(2), pinning each worker to one CPU
 *   [FALSE-SHARE] cache-line padding of the per-worker and pool hot fields
 *
 * The scheduler is deliberately a *teaching core*: bounded random-victim
 * stealing, a mutex-guarded injector for external submits, and a
 * gate/eventcount for parking. It is correct and complete for those pieces; it
 * omits the production refinements (lock-free injector, sleeping-worker
 * heuristics, NUMA-aware victim choice) called out in the README.
 * ===========================================================================
 */
#define _GNU_SOURCE          /* cpu_set_t, sched_setaffinity, CPU_SET, syscall  */

#include "threadpool.h"
#include "chase_lev.h"

#include <pthread.h>         /* pthread_create/join, mutex                      */
#include <sched.h>           /* sched_setaffinity, cpu_set_t, CPU_ZERO/SET      */
#include <unistd.h>          /* syscall, sysconf                                */
#include <sys/syscall.h>     /* SYS_futex                                       */
#include <linux/futex.h>     /* FUTEX_WAIT, FUTEX_WAKE, FUTEX_PRIVATE_FLAG      */
#include <stdlib.h>          /* aligned_alloc, malloc, free                     */
#include <string.h>          /* memset                                          */
#include <stdint.h>          /* int64_t                                         */
#include <stdatomic.h>       /* C11 atomics                                     */
#include <limits.h>          /* INT_MAX                                         */
#include <time.h>            /* struct timespec (futex timeout arg type)        */
#include <stdio.h>           /* perror                                          */

/* Cache line size; see chase_lev.h for the false-sharing rationale. */
#define TP_CACHELINE      64
/* Initial per-worker deque capacity (a power of two; grows on demand). 256
 * pointer slots = 2 KiB, comfortably enough that most fork-join workloads never
 * hit the grow path. */
#define TP_INIT_DEQUE_CAP 256u

/* ---------------------------------------------------------------------------
 * A unit of work. The submitter mallocs it; the worker that runs it frees the
 * box (fn is responsible for arg). `next` lets the injector thread a FIFO list
 * through the tasks themselves without a second allocation.
 * --------------------------------------------------------------------------- */
typedef struct Task {
    tp_task_fn    fn;
    void         *arg;
    struct Task  *next;   /* injector FIFO linkage (unused once dequeued)        */
} Task;

/* ---------------------------------------------------------------------------
 * Per-worker state. [FALSE-SHARE] The whole struct is cache-line aligned and
 * tail-padded so two workers' hot fields never land in the same 64-byte line;
 * an owner writing its own `bottom`/`tasks_run` must not invalidate a *different*
 * worker's cached state. The cl_deque inside is itself internally padded so this
 * worker's own `top` and `bottom` sit on separate lines (owner vs. thieves).
 * --------------------------------------------------------------------------- */
typedef struct worker {
    cl_deque    deque;        /* this worker's deque (owner push/take)           */
    tp_pool    *pool;         /* back-pointer to the shared pool                 */
    pthread_t   thread;       /* the OS thread                                   */
    int         id;           /* index in pool->workers                          */
    int         cpu;          /* CPU this worker is pinned to                    */
    unsigned    rng;          /* per-worker xorshift RNG (no sharing => no lock) */
    long        tasks_run;    /* stat; SINGLE writer (this thread) => plain long */
    char        _pad[TP_CACHELINE];  /* separate us from the next worker's line  */
} __attribute__((aligned(TP_CACHELINE))) worker;

/* ---------------------------------------------------------------------------
 * The pool. The four cross-thread counters each get their own cache line: they
 * are hammered from many cores and must not false-share with each other. Two of
 * them (`gate`, `pending`) double as FUTEX WORDS, so they are 32-bit `int`
 * (the width futex(2) operates on) and are naturally aligned.
 * --------------------------------------------------------------------------- */
struct tp_pool {
    worker *workers;
    int     nworkers;

    /* [FUTEX] eventcount: bumped on every submit and on shutdown. Workers park
     * on it (FUTEX_WAIT) and are woken by it (FUTEX_WAKE). Monotonic. */
    _Alignas(TP_CACHELINE) _Atomic int gate;
    /* [FUTEX] outstanding-task counter. tp_wait parks on it; it is woken when a
     * task completion drives it to zero. */
    _Alignas(TP_CACHELINE) _Atomic int pending;
    /* Number of workers currently parked. A pure optimization: it lets a
     * producer skip the FUTEX_WAKE syscall when nobody is asleep. seq_cst so it
     * interlocks correctly with `gate` (see the parking protocol below). */
    _Alignas(TP_CACHELINE) _Atomic int nsleeping;
    /* Set once, at shutdown; workers observe it and exit. */
    _Alignas(TP_CACHELINE) _Atomic int shutdown;

    /* The injector: a mutex-guarded FIFO for tasks submitted by threads that do
     * NOT own a deque (e.g. main). A Chase-Lev push is owner-only, so an outside
     * thread cannot legally push onto any worker's deque; it lands here instead,
     * and idle workers drain it. External submits are rare in fork-join code
     * (the root task is external; every recursive submit runs on a worker and
     * takes the lock-free local path), so one mutex is not a bottleneck. */
    _Alignas(TP_CACHELINE) pthread_mutex_t inj_lock;
    Task           *inj_head;
    Task           *inj_tail;
    _Atomic long    inj_count;   /* racy-readable size hint (kept under the lock) */
};

/* [ATOMICS] Which worker is THIS thread, if any. Set at worker startup; NULL on
 * external threads. Lets tp_submit choose the lock-free local path when called
 * from inside a running task. */
static _Thread_local worker *g_self;

/* ======================= [FUTEX] raw syscall wrappers ====================== */

/* futex(2) is x86-64 syscall 202. Full form:
 *   futex(uaddr, futex_op, val, timeout, uaddr2, val3)
 * We only need the 2-operand WAIT/WAKE forms, and always OR in
 * FUTEX_PRIVATE_FLAG: the futex is shared only among threads of one process, so
 * the kernel can use a per-process (not global) hash bucket — faster and the
 * correct choice for a thread pool. */
static long futex_call(_Atomic int *addr, int op, int val,
                       const struct timespec *timeout)
{
    /* The kernel treats *addr as a raw u32; cast the _Atomic away for the ABI. */
    return syscall(SYS_futex, (int *)addr, op, val, timeout, NULL, 0);
}

/* FUTEX_WAIT performs, atomically against a concurrent FUTEX_WAKE on the same
 * address: "if *addr == expected, block; else return EAGAIN immediately." That
 * atomic compare-then-block is the entire reason futex avoids lost wakeups: a
 * producer that changes *addr before waking cannot slip a change in between our
 * compare and our sleep. EINTR (a signal) also just returns; the caller loops
 * and re-checks its real condition, so we ignore the specific errno here. */
static void futex_wait(_Atomic int *addr, int expected)
{
    futex_call(addr, FUTEX_WAIT | FUTEX_PRIVATE_FLAG, expected, NULL);
}

/* FUTEX_WAKE wakes up to `n` threads parked on `addr` (INT_MAX = all of them). */
static void futex_wake(_Atomic int *addr, int n)
{
    futex_call(addr, FUTEX_WAKE | FUTEX_PRIVATE_FLAG, n, NULL);
}

/* ========================= [AFFINITY] CPU pinning ========================== */

/* Pin the calling thread to a single CPU. [AFFINITY] sched_setaffinity(2) is
 * x86-64 syscall 203; pid 0 means "the calling thread". Restricting a worker to
 * one CPU keeps its deque's cache lines resident on that core and stops the
 * scheduler from migrating it (which would cold-miss the whole working set).
 * Failure is non-fatal — affinity is an optimization, not a correctness
 * requirement — so we log and run unpinned (common inside restrictive cgroup
 * cpusets, where sched_setaffinity returns EINVAL). */
static void pin_to_cpu(int cpu)
{
    cpu_set_t set;
    CPU_ZERO(&set);          /* clear the affinity mask                          */
    CPU_SET(cpu, &set);      /* allow exactly this one CPU                       */
    if (sched_setaffinity(0, sizeof(set), &set) != 0)
        perror("sched_setaffinity (continuing unpinned)");
}

/* ============================ tiny per-worker RNG ========================== */

/* xorshift32: a fast, dependency-free PRNG for choosing steal victims. State is
 * per-worker, so there is zero cross-thread contention on it. Seeded nonzero. */
static unsigned xorshift(unsigned *s)
{
    unsigned x = *s;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *s = x;
    return x;
}

/* =============================== injector ================================== */

/* Append a task to the shared FIFO. Called from tp_submit's external path (and
 * as the OOM fallback of the local path). */
static void injector_push(tp_pool *p, Task *t)
{
    pthread_mutex_lock(&p->inj_lock);
    t->next = NULL;
    if (p->inj_tail)
        p->inj_tail->next = t;      /* link after the current tail              */
    else
        p->inj_head = t;            /* was empty: this is also the head         */
    p->inj_tail = t;
    /* Kept under the lock, but declared atomic so injector_pop/has_any_work may
     * read it racily as a hint without taking the lock. */
    atomic_fetch_add_explicit(&p->inj_count, 1, memory_order_relaxed);
    pthread_mutex_unlock(&p->inj_lock);
}

/* Pop the oldest task, or NULL if empty. */
static Task *injector_pop(tp_pool *p)
{
    /* Cheap racy bail-out: if the count looks zero, skip the lock entirely. A
     * false "zero" only costs one missed pop, retried on the next loop. */
    if (atomic_load_explicit(&p->inj_count, memory_order_relaxed) == 0)
        return NULL;

    pthread_mutex_lock(&p->inj_lock);
    Task *t = p->inj_head;
    if (t) {
        p->inj_head = t->next;
        if (!p->inj_head)
            p->inj_tail = NULL;     /* list emptied: fix up the tail            */
        atomic_fetch_sub_explicit(&p->inj_count, 1, memory_order_relaxed);
    }
    pthread_mutex_unlock(&p->inj_lock);
    return t;
}

/* =========================== stealing & waking ============================= */

/* Try to steal one task from a random victim. Bounded attempts: randomization
 * spreads thieves out (so they don't all pound worker 0) and guarantees
 * termination even when every steal keeps ABORTing against live contention. */
static Task *try_steal(tp_pool *p, worker *self)
{
    int n = p->nworkers;
    if (n <= 1)
        return NULL;                /* nobody to steal from                     */

    int attempts = n * 2;           /* give roughly two passes over the victims */
    for (int k = 0; k < attempts; k++) {
        int v = (int)(xorshift(&self->rng) % (unsigned)n);
        if (v == self->id)
            continue;               /* don't steal from ourselves               */
        cl_item x = cl_steal(&p->workers[v].deque);
        if (x == CL_ABORT)
            continue;               /* lost the CAS; treat as a miss, keep going */
        if (x != CL_EMPTY)
            return (Task *)x;       /* got a real task                          */
    }
    return NULL;                    /* found nothing this round                 */
}

/* Wake any workers that might be parked. [FUTEX] The gate bump is UNCONDITIONAL
 * and comes first: it both (a) advances the eventcount so a worker still in its
 * pre-sleep window fails its FUTEX_WAIT compare and re-checks, and (b) is a
 * seq_cst RMW that publishes the just-enqueued task ahead of the wake. Only the
 * FUTEX_WAKE syscall itself is skipped when nsleeping shows nobody is asleep —
 * and the seq_cst interlock between `gate` and `nsleeping` guarantees that if a
 * worker is (or is becoming) parked, either it observes the new gate value or we
 * observe its nsleeping increment, so the wake is never wrongly skipped. */
static void pool_wake(tp_pool *p)
{
    atomic_fetch_add_explicit(&p->gate, 1, memory_order_seq_cst);
    if (atomic_load_explicit(&p->nsleeping, memory_order_seq_cst) > 0)
        futex_wake(&p->gate, INT_MAX);
}

/* A relaxed scan for "is there work anywhere?" Used only to decide whether to
 * park; correctness never depends on it (the gate eventcount does), so stale
 * relaxed reads are fine. */
static int has_any_work(tp_pool *p)
{
    if (atomic_load_explicit(&p->inj_count, memory_order_relaxed) > 0)
        return 1;
    for (int i = 0; i < p->nworkers; i++)
        if (cl_size(&p->workers[i].deque) > 0)
            return 1;
    return 0;
}

/* =============================== execution ================================= */

/* Run one task, free its box, and account for it. When the outstanding-task
 * count reaches zero, wake tp_wait. */
static void run_task(worker *w, Task *t)
{
    tp_pool *p = w->pool;

    t->fn(t->arg);              /* run it; fn owns and frees `arg` if heap        */
    free(t);                    /* the worker owns the Task box; free it here     */
    w->tasks_run++;             /* single-writer stat; no atomic needed           */

    /* [FUTEX] Decrement outstanding tasks. fetch_sub returns the PREVIOUS value,
     * so the thread that observes 1 -> 0 is the unique one that drained the pool
     * and must wake tp_wait. acq_rel: the acquire pairs with other completions'
     * releases (so we see all their effects), the release publishes ours. */
    if (atomic_fetch_sub_explicit(&p->pending, 1, memory_order_acq_rel) == 1)
        futex_wake(&p->pending, INT_MAX);
}

/* ---------------------------------------------------------------------------
 * worker_main — the per-thread loop: take, steal, drain injector, else park.
 * --------------------------------------------------------------------------- */
static void *worker_main(void *arg)
{
    worker  *w = (worker *)arg;
    tp_pool *p = w->pool;

    g_self = w;                 /* [ATOMICS] mark self for tp_submit's fast path  */
    pin_to_cpu(w->cpu);         /* [AFFINITY] stick to one core                   */

    for (;;) {
        /* 1. Own deque first (LIFO): the most recently pushed task is the most
         *    cache-hot, and taking it needs no atomic RMW in the common case. */
        Task *t = (Task *)cl_take(&w->deque);
        /* 2. Empty: try to steal one task (FIFO) from a random victim. */
        if (!t)
            t = try_steal(p, w);
        /* 3. Still nothing: drain a task submitted from outside the pool. */
        if (!t)
            t = injector_pop(p);

        if (t) {
            run_task(w, t);
            continue;           /* got work; go again immediately                */
        }

        /* 4. No work anywhere. If we're shutting down, exit the loop. */
        if (atomic_load_explicit(&p->shutdown, memory_order_acquire))
            break;

        /* 5. Park on the gate (eventcount protocol). ORDER MATTERS:
         *      a) announce we are about to sleep (nsleeping++), then
         *      b) snapshot the gate value, then
         *      c) re-scan for work / shutdown.
         *    Because (a) and (b) precede (c), any producer that adds work after
         *    our scan must have bumped the gate past our snapshot, so our
         *    FUTEX_WAIT compare fails and we loop instead of sleeping through it.
         *    All seq_cst so this interlocks with pool_wake's gate/nsleeping ops. */
        atomic_fetch_add_explicit(&p->nsleeping, 1, memory_order_seq_cst);
        int g = atomic_load_explicit(&p->gate, memory_order_seq_cst);

        if (has_any_work(p) ||
            atomic_load_explicit(&p->shutdown, memory_order_acquire)) {
            atomic_fetch_sub_explicit(&p->nsleeping, 1, memory_order_seq_cst);
            continue;           /* work/shutdown appeared; don't sleep           */
        }

        /* 6. Sleep until the gate changes (a submit or shutdown bumps it). If it
         *    already changed since our snapshot, FUTEX_WAIT returns EAGAIN at
         *    once. Either way we then re-check everything from the top. */
        futex_wait(&p->gate, g);
        atomic_fetch_sub_explicit(&p->nsleeping, 1, memory_order_seq_cst);
    }
    return NULL;
}

/* ================================= API ==================================== */

int tp_submit(tp_pool *p, tp_task_fn fn, void *arg)
{
    Task *t = malloc(sizeof(*t));      /* submitter allocates; worker frees      */
    if (!t)
        return -1;
    t->fn   = fn;
    t->arg  = arg;
    t->next = NULL;

    /* [ATOMICS] Count the task as outstanding BEFORE it becomes visible, so
     * tp_wait can never see pending briefly drop to zero mid-flight. Relaxed is
     * fine: the release that actually publishes the task is the deque's bottom
     * store / the injector unlock, and pool_wake's seq_cst gate bump fences it. */
    atomic_fetch_add_explicit(&p->pending, 1, memory_order_relaxed);

    worker *w = g_self;
    if (w && w->pool == p) {
        /* Fast path: we ARE a worker of this pool, so we may push onto our own
         * deque (owner-only push, lock-free). This is the common fork-join case:
         * a task spawning subtasks feeds its own LIFO end. */
        if (cl_push(&w->deque, t) != 0)
            injector_push(p, t);       /* grow OOM: don't lose it — use injector */
    } else {
        /* Slow path: an outside thread. It must not touch a worker deque, so the
         * task goes to the shared injector. */
        injector_push(p, t);
    }

    pool_wake(p);                      /* make a parked thief come get it        */
    return 0;
}

void tp_wait(tp_pool *p)
{
    /* [FUTEX] Block until pending hits zero. We only get woken when a completion
     * drives pending 1 -> 0, but intermediate decrements are handled too: if
     * pending changed from our snapshot `n` before we slept, FUTEX_WAIT returns
     * EAGAIN and we re-read. So we spin-then-sleep down to zero without ever
     * blocking on a value that has already passed. */
    for (;;) {
        int n = atomic_load_explicit(&p->pending, memory_order_acquire);
        if (n == 0)
            return;
        futex_wait(&p->pending, n);
    }
}

void tp_destroy(tp_pool *p)
{
    /* Signal shutdown, then wake EVERYONE unconditionally (no nsleeping check —
     * at teardown we must not miss a single parked worker). The gate bump also
     * makes any worker mid-parking-protocol fail its FUTEX_WAIT compare. */
    atomic_store_explicit(&p->shutdown, 1, memory_order_release);
    atomic_fetch_add_explicit(&p->gate, 1, memory_order_seq_cst);
    futex_wake(&p->gate, INT_MAX);

    for (int i = 0; i < p->nworkers; i++)
        pthread_join(p->workers[i].thread, NULL);

    /* Single-threaded from here: every worker has joined, so we can touch the
     * deques and injector without synchronization. Free any tasks that were
     * still queued (abandoned by an abrupt destroy). We free the Task boxes; a
     * task's `arg` is leaked because we cannot know how to free it — call
     * tp_wait() before tp_destroy() to drain gracefully and avoid this. */
    Task *t;
    while ((t = injector_pop(p)) != NULL)
        free(t);
    for (int i = 0; i < p->nworkers; i++) {
        Task *lt;
        while ((lt = (Task *)cl_take(&p->workers[i].deque)) != NULL)
            free(lt);
        cl_destroy(&p->workers[i].deque);
    }

    pthread_mutex_destroy(&p->inj_lock);
    free(p->workers);
    free(p);
}

int tp_nworkers(tp_pool *p)
{
    return p->nworkers;
}

long tp_worker_tasks(tp_pool *p, int i)
{
    if (i < 0 || i >= p->nworkers)
        return -1;
    return p->workers[i].tasks_run;
}

/* ------------------------------ construction ------------------------------ */

tp_pool *tp_create(int nworkers)
{
    long ncpu = sysconf(_SC_NPROCESSORS_ONLN);
    if (ncpu < 1)
        ncpu = 1;
    if (nworkers <= 0)
        nworkers = (int)ncpu;          /* default: one worker per online CPU     */

    /* [FALSE-SHARE] aligned_alloc, not malloc: the pool's _Alignas(64) counters
     * and the workers' aligned deques only actually land on cache-line
     * boundaries if the base allocation is 64-aligned. sizeof is already a
     * multiple of 64 (forced by the _Alignas members), satisfying aligned_alloc. */
    tp_pool *p = aligned_alloc(TP_CACHELINE, sizeof(*p));
    if (!p)
        return NULL;
    memset(p, 0, sizeof(*p));          /* aligned_alloc does not zero             */

    p->nworkers = nworkers;
    atomic_store_explicit(&p->gate,      0, memory_order_relaxed);
    atomic_store_explicit(&p->pending,   0, memory_order_relaxed);
    atomic_store_explicit(&p->nsleeping, 0, memory_order_relaxed);
    atomic_store_explicit(&p->shutdown,  0, memory_order_relaxed);
    atomic_store_explicit(&p->inj_count, 0, memory_order_relaxed);
    p->inj_head = p->inj_tail = NULL;

    if (pthread_mutex_init(&p->inj_lock, NULL) != 0) {
        free(p);
        return NULL;
    }

    p->workers = aligned_alloc(TP_CACHELINE, (size_t)nworkers * sizeof(worker));
    if (!p->workers) {
        pthread_mutex_destroy(&p->inj_lock);
        free(p);
        return NULL;
    }
    memset(p->workers, 0, (size_t)nworkers * sizeof(worker));

    /* Initialize EVERY deque before starting ANY thread: a worker that starts
     * early may immediately try to steal from a worker whose thread has not yet
     * spawned, and that victim's deque must already be a valid (empty) one. */
    for (int i = 0; i < nworkers; i++) {
        worker *w = &p->workers[i];
        w->pool      = p;
        w->id        = i;
        w->cpu       = (int)(i % ncpu);           /* round-robin over CPUs        */
        w->rng       = (unsigned)(i * 2654435761u) | 1u; /* nonzero seed          */
        w->tasks_run = 0;
        if (cl_init(&w->deque, TP_INIT_DEQUE_CAP) != 0) {
            for (int j = 0; j < i; j++)
                cl_destroy(&p->workers[j].deque);
            free(p->workers);
            pthread_mutex_destroy(&p->inj_lock);
            free(p);
            return NULL;
        }
    }

    /* Spawn the threads. On a partial failure, tear down cleanly: signal
     * shutdown, wake+join the ones that started, destroy all deques, free. */
    for (int i = 0; i < nworkers; i++) {
        if (pthread_create(&p->workers[i].thread, NULL,
                           worker_main, &p->workers[i]) != 0) {
            atomic_store_explicit(&p->shutdown, 1, memory_order_release);
            atomic_fetch_add_explicit(&p->gate, 1, memory_order_seq_cst);
            futex_wake(&p->gate, INT_MAX);
            for (int j = 0; j < i; j++)
                pthread_join(p->workers[j].thread, NULL);
            for (int j = 0; j < nworkers; j++)
                cl_destroy(&p->workers[j].deque);
            free(p->workers);
            pthread_mutex_destroy(&p->inj_lock);
            free(p);
            return NULL;
        }
    }

    return p;
}
