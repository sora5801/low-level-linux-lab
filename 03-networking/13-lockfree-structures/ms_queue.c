/* ===========================================================================
 * ms_queue.c — Michael & Scott queue implementation with hazard pointers.
 * ===========================================================================
 */
#include "ms_queue.h"
#include <stdlib.h>   /* malloc, free */

/* The deleter handed to hp_retire: our nodes are plain malloc'd blocks. */
static void free_node(void *p) { free(p); }

/* ---------------------------------------------------------------------------
 * protect — safely publish a shared pointer into hazard slot `which` before we
 * dereference it. The load/announce/re-check dance closes the StoreLoad race
 * described in hazard.h: we might announce the pointer only *after* a reclaimer
 * snapshotted the slots and decided to free it, so we re-read the source and
 * loop until our announcement provably preceded the source still naming it.
 *
 * After this returns `p`, `p` cannot be freed out from under us until we clear
 * the slot, because any reclaimer will see our hazard pointer during its scan.
 * --------------------------------------------------------------------------- */
static msq_node *protect(hp_thread *hp, int which, _Atomic(msq_node *) *src)
{
    /* ACQUIRE: if we end up using *p's fields, we must observe the writes that
     * published it (paired with the release CAS that linked it in). */
    msq_node *p = atomic_load_explicit(src, memory_order_acquire);
    for (;;) {
        hp_set(hp, which, p);            /* seq_cst announce (StoreLoad half 1) */
        /* seq_cst re-read: ordered after the announce in the total order S, so
         * either a concurrent reclaimer sees our hazard, or we see its unlink
         * here and retry with the fresh value. */
        msq_node *p2 = atomic_load_explicit(src, memory_order_seq_cst);
        if (p2 == p)
            return p;                    /* announcement is now valid */
        p = p2;                          /* pointer moved: re-announce */
    }
}

int msq_init(ms_queue *q)
{
    /* The permanent dummy: head and tail both start on it. Its ->value is never
     * read; its ->next is the real front once something is enqueued. */
    msq_node *dummy = (msq_node *)malloc(sizeof *dummy);
    if (dummy == NULL)
        return -1;
    atomic_init(&dummy->next, (msq_node *)NULL);
    atomic_init(&q->head, dummy);
    atomic_init(&q->tail, dummy);
    return 0;
}

int msq_enqueue(ms_queue *q, hp_thread *hp, lf_value v)
{
    msq_node *n = (msq_node *)malloc(sizeof *n);
    if (n == NULL)
        return -1;
    n->value = v;
    /* RELAXED: n is thread-private until the link CAS publishes it; that CAS is
     * a release, which will order this store before n becomes visible. */
    atomic_store_explicit(&n->next, (msq_node *)NULL, memory_order_relaxed);

    for (;;) {
        /* Protect the tail node: a concurrent dequeue could otherwise free the
         * node we are about to inspect if the queue drains to it. */
        msq_node *tail = protect(hp, 0, &q->tail);

        /* ACQUIRE: observe a link another enqueuer may have just published. */
        msq_node *next = atomic_load_explicit(&tail->next, memory_order_acquire);

        /* Consistency: did `tail` stay the queue's tail while we looked? If not,
         * our `next` view is stale — restart. (ACQUIRE to pair with whoever
         * swung the tail.) */
        if (tail != atomic_load_explicit(&q->tail, memory_order_acquire))
            continue;

        if (next != NULL) {
            /* tail is LAGGING one node behind. Invariant #1 says we must help:
             * swing q->tail forward, then retry. RELEASE so the tail update
             * publishes coherently; failure RELAXED (someone else helped). */
            atomic_compare_exchange_strong_explicit(
                &q->tail, &tail, next,
                memory_order_release, memory_order_relaxed);
            continue;
        }

        /* tail->next is NULL: try to link our node at the very end.
         * RELEASE on success publishes n's value/next to a future dequeuer that
         * ACQUIRE-loads this same next pointer. */
        msq_node *expected = NULL;
        if (atomic_compare_exchange_strong_explicit(
                &tail->next, &expected, n,
                memory_order_release, memory_order_relaxed)) {
            /* Linked. Try to swing tail to n. This MAY fail (another thread
             * helped, or a dequeuer moved it) — that is fine; the invariant only
             * requires tail to eventually catch up, and the next operation will
             * finish the job. Best-effort, so we ignore the result. */
            atomic_compare_exchange_strong_explicit(
                &q->tail, &tail, n,
                memory_order_release, memory_order_relaxed);
            hp_clear(hp, 0);
            return 0;
        }
        /* Lost the link race to another enqueuer — loop and retry. */
    }
}

int msq_dequeue(ms_queue *q, hp_thread *hp, lf_value *out)
{
    for (;;) {
        /* Protect head (the current dummy) — it is exactly the node we may free
         * at the end, and other consumers may free it too. */
        msq_node *head = protect(hp, 0, &q->head);

        /* Snapshot tail to detect the "tail lagging" case below. ACQUIRE keeps
         * this consistent with enqueue's release swings. */
        msq_node *tail = atomic_load_explicit(&q->tail, memory_order_acquire);

        /* Protect head->next (the real front): we will read its value and it
         * becomes the new dummy, so it must not be freed under us either. */
        msq_node *next = protect(hp, 1, &head->next);

        /* Consistency: if head moved while we were protecting `next`, our view
         * is stale — restart. */
        if (head != atomic_load_explicit(&q->head, memory_order_acquire))
            continue;

        if (next == NULL) {
            /* Only the dummy is present -> queue is empty. */
            hp_clear_all(hp);
            *out = 0;
            return 0;
        }

        if (head == tail) {
            /* head == tail but there IS a next: tail is lagging behind the last
             * enqueue. Help advance it before we can dequeue, then retry. */
            atomic_compare_exchange_strong_explicit(
                &q->tail, &tail, next,
                memory_order_release, memory_order_relaxed);
            continue;
        }

        /* Read the value BEFORE unlinking. `next` is hazard-protected, so even
         * though it is about to become the new dummy, it cannot be freed while
         * we hold it. (Its ->value stops being meaningful once it is the dummy,
         * so we must capture it now.) */
        lf_value v = next->value;

        /* Try to swing head from the old dummy to `next`. RELEASE so consumers
         * that later observe the new head also observe our bookkeeping. */
        if (atomic_compare_exchange_strong_explicit(
                &q->head, &head, next,
                memory_order_release, memory_order_relaxed)) {
            *out = v;
            hp_clear_all(hp);
            /* The old dummy `head` is now unlinked and unreachable via the
             * queue. DO NOT free() it directly: another thread may still hold it
             * in a hazard slot. Retire it — hazard.c frees it once no slot names
             * it. This is the entire point of hazard pointers. */
            hp_retire(hp, head, free_node);
            return 1;
        }
        /* Lost the head race to another consumer — loop and retry. */
    }
}

void msq_destroy(ms_queue *q)
{
    /* SINGLE-THREADED teardown. Walk head..end freeing every still-linked node
     * (including the final dummy). Nodes that were already retired live in the
     * per-thread retire lists and are freed by hp_thread_flush at each worker's
     * shutdown, so there is no double free. */
    msq_node *n = atomic_load_explicit(&q->head, memory_order_relaxed);
    while (n != NULL) {
        msq_node *nx = atomic_load_explicit(&n->next, memory_order_relaxed);
        free(n);
        n = nx;
    }
}
