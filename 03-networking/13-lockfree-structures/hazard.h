/* ===========================================================================
 * hazard.h — Hazard Pointers (Michael, 2004): safe memory reclamation for
 *            lock-free structures.
 * ===========================================================================
 *
 * THE PROBLEM HAZARD POINTERS SOLVE
 * --------------------------------
 * In a lock-free structure a thread can hold a pointer to a node that ANOTHER
 * thread simultaneously unlinks and wants to free(). If it frees, the first
 * thread dereferences freed memory — a use-after-free crash or silent
 * corruption. Tagged pointers (see the Treiber stack) stop a stale CAS from
 * *succeeding*, but they do NOT stop that stale *dereference*; they only work if
 * memory is never unmapped (type-stable pools). When you genuinely want to give
 * memory back to malloc, you need Safe Memory Reclamation. Hazard pointers are
 * the canonical lock-free SMR scheme.
 *
 * THE IDEA
 * --------
 * Before a thread dereferences a shared node, it publishes that node's address
 * in a per-thread, single-writer/multi-reader "hazard pointer" slot, then
 * re-checks that the node is still linked (so it didn't publish too late). To
 * free a node, a thread does NOT free immediately: it *retires* it onto a
 * private list, and periodically SCANS every thread's hazard slots. A retired
 * node is freed only once NO hazard slot points at it. Reads are wait-free-ish
 * (a store + a re-check); reclamation is batched and lock-free.
 *
 * THE TWO RACES, AND WHY THE ORDERING IS seq_cst ON THE HOT PATH
 * -------------------------------------------------------------
 * Correctness hinges on a StoreLoad relationship on BOTH sides:
 *   - Protector: STORE hazard=X, then LOAD the source to confirm X still linked.
 *   - Reclaimer: (unlink X, then) LOAD every hazard slot before freeing X.
 * If either side's store could be reordered after its load, a node could be
 * freed while a protector is about to use it. StoreLoad is the one ordering x86
 * does NOT give for free, so we use seq_cst on the hazard store, the validate
 * reload, and the scan loads. That is the simplest provably-correct choice;
 * production libraries (folly, Michael's original) shave it to release/acquire
 * plus explicit seq_cst fences — see the README references.
 * ===========================================================================
 */
#ifndef HAZARD_H
#define HAZARD_H

/* Sizing. A real domain would grow these dynamically; fixed caps keep the
 * teaching code allocation-free and easy to reason about. */
#define HP_MAX_THREADS      64                              /* registered threads   */
#define HP_PER_THREAD       2                               /* hazard slots/thread  */
#define HP_NUM_SLOTS        (HP_MAX_THREADS * HP_PER_THREAD)
/* Retire when a thread has this many deferred nodes. Chosen > HP_NUM_SLOTS so
 * every scan is guaranteed to reclaim at least (threshold - HP_NUM_SLOTS)
 * nodes — bounding memory to O(threads^2) and amortizing the scan cost. */
#define HP_RETIRE_THRESHOLD (HP_NUM_SLOTS * 2)

#include <stdatomic.h>

/* One deferred-free entry: the node and how to free it (so the domain can serve
 * more than one node type). */
typedef struct hp_retired {
    void  *ptr;
    void (*deleter)(void *);
} hp_retired;

/* The shared domain: one flat array of hazard slots (single-writer per slot,
 * read by every scanner) plus a high-water registration counter. */
typedef struct hp_domain {
    _Atomic(void *) slots[HP_NUM_SLOTS];
    _Atomic(int)    nthreads;   /* how many threads have registered (bounds scan) */
} hp_domain;

/* Per-thread handle. `base` is this thread's first slot index; its slots are
 * base .. base+HP_PER_THREAD-1, written ONLY by this thread. The retire list is
 * thread-private (no synchronization needed to append). */
typedef struct hp_thread {
    hp_domain *dom;
    int        base;
    hp_retired retired[HP_RETIRE_THRESHOLD];
    int        nretired;
} hp_thread;

/* Domain lifecycle. */
void hp_domain_init(hp_domain *d);

/* Register the calling thread. Returns 0, or -1 if HP_MAX_THREADS is exceeded. */
int  hp_thread_init(hp_thread *t, hp_domain *d);

/* Announce that the calling thread is about to dereference `p` via hazard slot
 * `which` (0..HP_PER_THREAD-1). seq_cst — see the header comment. */
void hp_set(hp_thread *t, int which, void *p);

/* Stop protecting slot `which`. */
void hp_clear(hp_thread *t, int which);

/* Clear all of this thread's hazard slots (call when done with an operation). */
void hp_clear_all(hp_thread *t);

/* Defer freeing `p` (freed later via `deleter` once no hazard slot names it).
 * Triggers a scan when the retire list hits HP_RETIRE_THRESHOLD. */
void hp_retire(hp_thread *t, void *p, void (*deleter)(void *));

/* Force a scan and free everything now reclaimable. Call at thread shutdown to
 * drain the retire list (by then no hazard slots point at these nodes). */
void hp_thread_flush(hp_thread *t);

#endif /* HAZARD_H */
