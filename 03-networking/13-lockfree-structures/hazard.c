/* ===========================================================================
 * hazard.c — hazard-pointer implementation.
 * ===========================================================================
 */
#include "hazard.h"

void hp_domain_init(hp_domain *d)
{
    for (int i = 0; i < HP_NUM_SLOTS; i++)
        atomic_init(&d->slots[i], (void *)0);
    atomic_init(&d->nthreads, 0);
}

int hp_thread_init(hp_thread *t, hp_domain *d)
{
    /* Claim a unique thread index. fetch_add is a single atomic RMW, so two
     * threads racing to register get distinct indices. RELAXED is fine: the
     * only thing that matters is uniqueness of the returned value, and
     * fetch_add's atomicity guarantees that regardless of ordering; the slots
     * themselves were zeroed in hp_domain_init before any thread started. */
    int idx = atomic_fetch_add_explicit(&d->nthreads, 1, memory_order_relaxed);
    if (idx >= HP_MAX_THREADS)
        return -1;                       /* domain full */
    t->dom      = d;
    t->base     = idx * HP_PER_THREAD;
    t->nretired = 0;
    return 0;
}

void hp_set(hp_thread *t, int which, void *p)
{
    /* seq_cst STORE. This is the "announce" half of the protector's StoreLoad:
     * it must not be reordered after the caller's subsequent validate-load of
     * the source pointer, or a scanner could miss our hazard and free the node
     * we are about to touch. seq_cst puts this store into the single total
     * order S that the scan's seq_cst loads also observe. */
    atomic_store_explicit(&t->dom->slots[t->base + which], p, memory_order_seq_cst);
}

void hp_clear(hp_thread *t, int which)
{
    /* RELEASE is enough to un-announce: we are only making "no longer hazarded"
     * visible; there is no later load in this thread that must not pass it. A
     * scanner that still sees the old value simply keeps the node one extra
     * round — safe, just slightly delayed. */
    atomic_store_explicit(&t->dom->slots[t->base + which], (void *)0,
                          memory_order_release);
}

void hp_clear_all(hp_thread *t)
{
    for (int i = 0; i < HP_PER_THREAD; i++)
        hp_clear(t, i);
}

/* hp_scan — reclaim every retired node not currently protected by any hazard
 * slot. O(retired * live_slots); production uses a hash/sorted set, but linear
 * is clearest for teaching. */
static void hp_scan(hp_thread *t)
{
    hp_domain *d = t->dom;

    /* A StoreLoad fence before we read the hazard slots. Pairs with the fact
     * that the caller already unlinked the retired nodes (via a release CAS in
     * the queue): this fence keeps those unlinks from being reordered after the
     * slot reads, and puts the reads into the total order S so we observe any
     * protector's seq_cst announce that preceded its validate. */
    atomic_thread_fence(memory_order_seq_cst);

    /* 1. Snapshot every currently-announced hazard pointer. We only need to
     *    scan slots belonging to threads that have registered. */
    void *hazard[HP_NUM_SLOTS];
    int   hn     = 0;
    int   active = atomic_load_explicit(&d->nthreads, memory_order_acquire);
    if (active > HP_MAX_THREADS) active = HP_MAX_THREADS;
    int   nslots = active * HP_PER_THREAD;
    for (int i = 0; i < nslots; i++) {
        /* seq_cst load: the reclaimer half of the StoreLoad race (see hp_set).*/
        void *p = atomic_load_explicit(&d->slots[i], memory_order_seq_cst);
        if (p != (void *)0)
            hazard[hn++] = p;
    }

    /* 2. Walk the retire list. Keep any node that appears in the snapshot;
     *    free the rest. We compact survivors down in place. */
    int keep = 0;
    for (int i = 0; i < t->nretired; i++) {
        void *node        = t->retired[i].ptr;
        int   is_hazarded = 0;
        for (int j = 0; j < hn; j++) {
            if (hazard[j] == node) { is_hazarded = 1; break; }
        }
        if (is_hazarded) {
            t->retired[keep++] = t->retired[i];   /* still in use: defer again */
        } else {
            t->retired[i].deleter(node);           /* provably unreachable: free */
        }
    }
    t->nretired = keep;
}

void hp_retire(hp_thread *t, void *p, void (*deleter)(void *))
{
    /* Append to the thread-private retire list — no synchronization needed, we
     * are the only writer. */
    t->retired[t->nretired].ptr     = p;
    t->retired[t->nretired].deleter = deleter;
    t->nretired++;

    /* Amortized batch reclamation: only scan when the buffer fills. Because the
     * threshold exceeds the total hazard-slot count, each scan frees at least a
     * constant fraction, so retired memory stays bounded. */
    if (t->nretired >= HP_RETIRE_THRESHOLD)
        hp_scan(t);
}

void hp_thread_flush(hp_thread *t)
{
    /* Drain as much as possible. At a clean shutdown no hazard slots point at
     * these nodes, so this frees them all; mid-run it frees whatever is
     * currently unprotected. */
    hp_scan(t);
}
