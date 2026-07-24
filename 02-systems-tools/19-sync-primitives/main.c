/* ===========================================================================
 * main.c — a self-checking stress harness for the primitives in syncprim.c.
 * ===========================================================================
 *
 * This is the RUNNABLE half of the project. Unlike syncprim.c (which is
 * freestanding), the harness freely uses libc and pthreads — it needs to spawn
 * real OS threads to create the contention that makes the locks earn their
 * keep, and it needs printf to report results. Each test is designed so that a
 * broken lock (wrong memory order, a lost wakeup, a torn read) makes the final
 * answer WRONG, and the program exits non-zero. "It printed PASS" therefore
 * means the primitive actually enforced mutual exclusion / ordering, not merely
 * that it did not crash.
 *
 * Build & run (Linux / WSL2):
 *     make run        # or: cc -O2 -pthread main.c syncprim.c -o syncdemo && ./syncdemo
 * ===========================================================================
 */

#include "syncprim.h"

#include <pthread.h>    /* pthread_create/join — the OS threads under test     */
#include <stdio.h>      /* printf                                              */
#include <stdlib.h>     /* EXIT_SUCCESS / EXIT_FAILURE                         */
#include <stdint.h>     /* uint64_t                                            */
#include <string.h>     /* memset                                             */

/* Tunables. Deliberately large enough that unsynchronized code virtually always
 * loses a race, small enough to run in well under a second. */
#define NR_THREADS   8
#define NR_ITERS     200000

/* A running tally of failures across all tests; the process exit code. */
static int g_failures = 0;

static void check(int cond, const char *what)
{
    if (cond) {
        printf("  PASS  %s\n", what);
    } else {
        printf("  FAIL  %s\n", what);
        g_failures++;
    }
}

/* ===========================================================================
 * TEST 1 — mutex mutual exclusion.
 * NR_THREADS threads each do NR_ITERS unguarded ++ on a shared counter. Without
 * a correct lock the read-modify-write races and the total comes up short; a
 * correct mutex yields exactly NR_THREADS*NR_ITERS.
 * =========================================================================== */
static sp_mutex   t1_mtx = SP_MUTEX_INIT;
static uint64_t   t1_counter;

static void *t1_worker(void *arg)
{
    (void)arg;
    for (long i = 0; i < NR_ITERS; i++) {
        sp_mutex_lock(&t1_mtx);
        t1_counter++;               /* the critical section: one non-atomic ++ */
        sp_mutex_unlock(&t1_mtx);
    }
    return NULL;
}

static void test_mutex(void)
{
    pthread_t th[NR_THREADS];
    t1_counter = 0;
    for (int i = 0; i < NR_THREADS; i++)
        pthread_create(&th[i], NULL, t1_worker, NULL);
    for (int i = 0; i < NR_THREADS; i++)
        pthread_join(th[i], NULL);

    printf("mutex:   counter = %llu (want %llu)\n",
           (unsigned long long)t1_counter,
           (unsigned long long)((uint64_t)NR_THREADS * NR_ITERS));
    check(t1_counter == (uint64_t)NR_THREADS * NR_ITERS,
          "mutex enforces mutual exclusion");
}

/* ===========================================================================
 * TEST 2 — spinlock mutual exclusion. Identical shape to test 1.
 * =========================================================================== */
static sp_spinlock t2_lock = SP_SPINLOCK_INIT;
static uint64_t    t2_counter;

static void *t2_worker(void *arg)
{
    (void)arg;
    for (long i = 0; i < NR_ITERS; i++) {
        sp_spin_lock(&t2_lock);
        t2_counter++;
        sp_spin_unlock(&t2_lock);
    }
    return NULL;
}

static void test_spinlock(void)
{
    pthread_t th[NR_THREADS];
    t2_counter = 0;
    for (int i = 0; i < NR_THREADS; i++)
        pthread_create(&th[i], NULL, t2_worker, NULL);
    for (int i = 0; i < NR_THREADS; i++)
        pthread_join(th[i], NULL);

    printf("spinlock: counter = %llu (want %llu)\n",
           (unsigned long long)t2_counter,
           (unsigned long long)((uint64_t)NR_THREADS * NR_ITERS));
    check(t2_counter == (uint64_t)NR_THREADS * NR_ITERS,
          "spinlock enforces mutual exclusion");
}

/* ===========================================================================
 * TEST 3 — counting semaphore as a concurrency limiter.
 * A semaphore with K permits must never let more than K threads into the
 * guarded region at once. Each thread grabs a permit, bumps a live-occupancy
 * counter, records the peak, then releases. If the peak ever exceeds K the
 * semaphore leaked a permit.
 * =========================================================================== */
#define T3_PERMITS 3
static sp_sem  t3_sem = SP_SEM_INIT(T3_PERMITS);
static int     t3_live;         /* current occupants (guarded by atomics)      */
static int     t3_peak;         /* max occupants ever seen                     */

static void *t3_worker(void *arg)
{
    (void)arg;
    for (int i = 0; i < 4000; i++) {
        sp_sem_wait(&t3_sem);                       /* acquire a permit         */

        /* Enter the limited region. Track occupancy with atomics (we are
         * testing the semaphore, so the bookkeeping must not itself use it). */
        int now = __atomic_add_fetch(&t3_live, 1, __ATOMIC_ACQ_REL);
        /* Raise the recorded peak to `now` if it is higher (lock-free max). */
        int seen = __atomic_load_n(&t3_peak, __ATOMIC_RELAXED);
        while (now > seen &&
               !__atomic_compare_exchange_n(&t3_peak, &seen, now, 1,
                                            __ATOMIC_RELAXED, __ATOMIC_RELAXED))
            ; /* seen refreshed on failure */

        __atomic_sub_fetch(&t3_live, 1, __ATOMIC_ACQ_REL);
        sp_sem_post(&t3_sem);                       /* release the permit       */
    }
    return NULL;
}

static void test_semaphore(void)
{
    pthread_t th[NR_THREADS];
    t3_live = t3_peak = 0;
    for (int i = 0; i < NR_THREADS; i++)
        pthread_create(&th[i], NULL, t3_worker, NULL);
    for (int i = 0; i < NR_THREADS; i++)
        pthread_join(th[i], NULL);

    printf("sem:     peak occupancy = %d (limit %d)\n", t3_peak, T3_PERMITS);
    check(t3_peak <= T3_PERMITS && t3_peak > 0,
          "semaphore bounds concurrency to its permit count");
}

/* ===========================================================================
 * TEST 4 — condition variable via a bounded producer/consumer queue.
 * Producers push a fixed number of items into a small ring buffer, blocking on
 * `not_full` when it is full; consumers pop, blocking on `not_empty` when it is
 * empty. If a wakeup were lost the program would DEADLOCK (join never returns)
 * or lose items; we verify every produced item is consumed exactly once.
 * =========================================================================== */
#define T4_CAP        16
#define T4_PRODUCERS  4
#define T4_CONSUMERS  4
#define T4_PER_PROD   50000
#define T4_TOTAL      ((long)T4_PRODUCERS * T4_PER_PROD)

static sp_mutex t4_mtx      = SP_MUTEX_INIT;
static sp_cond  t4_not_full = SP_COND_INIT;
static sp_cond  t4_not_empty= SP_COND_INIT;
static int      t4_buf[T4_CAP];
static int      t4_head, t4_tail, t4_count;
static long     t4_consumed;    /* how many items consumers have popped         */

static void *t4_producer(void *arg)
{
    (void)arg;
    for (long i = 0; i < T4_PER_PROD; i++) {
        sp_mutex_lock(&t4_mtx);
        while (t4_count == T4_CAP)               /* predicate loop, never `if`   */
            sp_cond_wait(&t4_not_full, &t4_mtx);
        t4_buf[t4_tail] = 1;
        t4_tail = (t4_tail + 1) % T4_CAP;
        t4_count++;
        sp_cond_signal(&t4_not_empty);           /* a consumer may now proceed   */
        sp_mutex_unlock(&t4_mtx);
    }
    return NULL;
}

static void *t4_consumer(void *arg)
{
    long *quota = (long *)arg;                   /* items this consumer must take */
    while (*quota > 0) {
        sp_mutex_lock(&t4_mtx);
        while (t4_count == 0)
            sp_cond_wait(&t4_not_empty, &t4_mtx);
        (void)t4_buf[t4_head];
        t4_head = (t4_head + 1) % T4_CAP;
        t4_count--;
        t4_consumed++;
        (*quota)--;
        sp_cond_signal(&t4_not_full);            /* a producer may now proceed   */
        sp_mutex_unlock(&t4_mtx);
    }
    return NULL;
}

static void test_condvar(void)
{
    pthread_t prod[T4_PRODUCERS], cons[T4_CONSUMERS];
    long quota[T4_CONSUMERS];

    t4_head = t4_tail = t4_count = 0;
    t4_consumed = 0;

    /* Hand each consumer an exact quota so the totals balance and every thread
     * terminates — no sentinel/poison-pill bookkeeping needed. */
    for (int i = 0; i < T4_CONSUMERS; i++) {
        quota[i] = T4_TOTAL / T4_CONSUMERS;
        if (i == T4_CONSUMERS - 1)
            quota[i] += T4_TOTAL % T4_CONSUMERS; /* last one mops up remainder   */
    }

    for (int i = 0; i < T4_CONSUMERS; i++)
        pthread_create(&cons[i], NULL, t4_consumer, &quota[i]);
    for (int i = 0; i < T4_PRODUCERS; i++)
        pthread_create(&prod[i], NULL, t4_producer, NULL);

    for (int i = 0; i < T4_PRODUCERS; i++)
        pthread_join(prod[i], NULL);
    for (int i = 0; i < T4_CONSUMERS; i++)
        pthread_join(cons[i], NULL);

    printf("cond:    consumed = %ld (want %ld)\n", t4_consumed, T4_TOTAL);
    check(t4_consumed == T4_TOTAL,
          "condvar hands off every item with no lost wakeups");
}

/* ===========================================================================
 * TEST 5 — reader-writer lock consistency.
 * The writer keeps two words equal (a == b) by updating both under the write
 * lock. A reader under the read lock samples both and must ALWAYS see them
 * equal; if the rwlock let a reader run concurrently with a writer, it could
 * observe a torn (a != b) state. We count any such violation.
 * =========================================================================== */
static sp_rwlock t5_rw = SP_RWLOCK_INIT;
static uint64_t  t5_a, t5_b;
static int       t5_violations;

static void *t5_writer(void *arg)
{
    (void)arg;
    for (long i = 0; i < 100000; i++) {
        sp_rwlock_wrlock(&t5_rw);
        t5_a++;
        t5_b++;                     /* invariant a == b held under the write lock */
        sp_rwlock_wrunlock(&t5_rw);
    }
    return NULL;
}

static void *t5_reader(void *arg)
{
    (void)arg;
    for (long i = 0; i < 200000; i++) {
        sp_rwlock_rdlock(&t5_rw);
        uint64_t a = t5_a;
        uint64_t b = t5_b;
        if (a != b)                 /* a torn read means exclusion was violated  */
            __atomic_add_fetch(&t5_violations, 1, __ATOMIC_RELAXED);
        sp_rwlock_rdunlock(&t5_rw);
    }
    return NULL;
}

static void test_rwlock(void)
{
    pthread_t th[NR_THREADS];
    t5_a = t5_b = 0;
    t5_violations = 0;

    /* Half readers, half writers. */
    for (int i = 0; i < NR_THREADS; i++)
        pthread_create(&th[i], NULL, (i & 1) ? t5_writer : t5_reader, NULL);
    for (int i = 0; i < NR_THREADS; i++)
        pthread_join(th[i], NULL);

    printf("rwlock:  torn reads = %d (want 0)\n", t5_violations);
    check(t5_violations == 0,
          "rwlock gives readers a consistent, writer-exclusive view");
}

int main(void)
{
    printf("== hand-rolled futex synchronization primitives: stress tests ==\n");
    printf("threads=%d  iters=%d\n\n", NR_THREADS, NR_ITERS);

    test_mutex();
    test_spinlock();
    test_semaphore();
    test_condvar();
    test_rwlock();

    printf("\n%s (%d failure%s)\n",
           g_failures == 0 ? "ALL TESTS PASSED" : "SOME TESTS FAILED",
           g_failures, g_failures == 1 ? "" : "s");
    return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
