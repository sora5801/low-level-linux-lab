/* ===========================================================================
 * chase_lev.c — implementation of the growable Chase-Lev work-stealing deque.
 * ===========================================================================
 *
 * Read chase_lev.h first for the layout and the big picture. This file is the
 * memory-ordering heart of the whole project; every explicit order below is
 * load-bearing and is justified in a comment. The canonical asm for these three
 * routines (minus the resize) lives in asm/demo.annotated.s.
 * ===========================================================================
 */
#include "chase_lev.h"

#include <stdlib.h>   /* malloc, free                                          */

/* ---------------------------------------------------------------------------
 * array_new — allocate a backing array of `cap` slots (cap must be a power of
 * two). Returns NULL on OOM. The slots are left UNINITIALIZED: a slot is only
 * ever read after the owner has stored an item into it and advanced `bottom`
 * past it (push), so uninitialized slots are never observed as items.
 * --------------------------------------------------------------------------- */
static cl_array *array_new(size_t cap)
{
    cl_array *a = malloc(sizeof(*a));
    if (!a)
        return NULL;
    /* One malloc for the header, one for the slots. sizeof(_Atomic(cl_item)) ==
     * 8 here; the storage is plain memory that we only ever touch through the
     * atomic builtins, which is what makes racy thief reads well-defined. */
    a->slot = malloc(cap * sizeof(_Atomic(cl_item)));
    if (!a->slot) {
        free(a);
        return NULL;
    }
    a->cap  = cap;
    a->mask = cap - 1;   /* cap is a power of two, so mask is all-ones below cap */
    a->prev = NULL;
    return a;
}

/* ---------------------------------------------------------------------------
 * cl_init — set up an empty deque with an initial capacity (rounded is caller's
 * job; pass a power of two). top == bottom == 0 is the canonical empty state.
 * --------------------------------------------------------------------------- */
int cl_init(cl_deque *d, size_t init_cap)
{
    cl_array *a = array_new(init_cap);
    if (!a)
        return -1;
    /* No other thread can see `d` yet (the pool wires workers up before it
     * starts their threads), so plain relaxed initialization is fine here. */
    atomic_store_explicit(&d->top,    0, memory_order_relaxed);
    atomic_store_explicit(&d->bottom, 0, memory_order_relaxed);
    atomic_store_explicit(&d->array,  a, memory_order_relaxed);
    return 0;
}

/* ---------------------------------------------------------------------------
 * cl_destroy — free the current array and every superseded array on the `prev`
 * chain. MUST be called only after all worker threads that touched this deque
 * have joined: it does no synchronization because there is nothing left to race.
 * --------------------------------------------------------------------------- */
void cl_destroy(cl_deque *d)
{
    cl_array *a = atomic_load_explicit(&d->array, memory_order_relaxed);
    while (a) {
        cl_array *prev = a->prev;   /* walk the chain of grown-out arrays */
        free(a->slot);
        free(a);
        a = prev;
    }
    atomic_store_explicit(&d->array, NULL, memory_order_relaxed);
}

/* ---------------------------------------------------------------------------
 * grow — double the backing array. OWNER ONLY (called from cl_push). Copies the
 * live items [t, b) into a fresh array, links the old one on `prev` for deferred
 * freeing, and publishes the new array with a RELEASE store so a thief that
 * later ACQUIRE-loads `array` sees fully-copied slots. Returns 0 or -1 on OOM.
 * --------------------------------------------------------------------------- */
static int grow(cl_deque *d, int64_t b, int64_t t)
{
    cl_array *old = atomic_load_explicit(&d->array, memory_order_relaxed);
    cl_array *na  = array_new(old->cap * 2);
    if (!na)
        return -1;             /* leave the deque intact; caller reports OOM     */

    na->prev = old;            /* retain old array; freed in cl_destroy          */

    /* Copy the currently-live window [t, b). Both indices are the owner's to
     * read cheaply, and the thieves only ever *advance* t, so copying a possibly
     * slightly-stale-but-safe superset is fine — any item a thief steals in the
     * meantime is simply copied and then skipped (its top moved past it). */
    for (int64_t i = t; i < b; i++) {
        cl_item v = atomic_load_explicit(&old->slot[i & old->mask],
                                         memory_order_relaxed);
        atomic_store_explicit(&na->slot[i & na->mask], v,
                              memory_order_relaxed);
    }

    /* Publish. RELEASE so the slot copies above happen-before any thief's use of
     * the new array (that thief pairs with an ACQUIRE/consume load of `array`). */
    atomic_store_explicit(&d->array, na, memory_order_release);
    return 0;
}

/* ---------------------------------------------------------------------------
 * cl_push — OWNER ONLY. Append x at the bottom (LIFO end).
 *
 * Ordering (see asm/demo.annotated.s for the x86 lowering):
 *   1. RELAXED load of bottom       — the owner's own index; cheap.
 *   2. ACQUIRE load of top          — needed to compute the true size and decide
 *                                     whether to grow; acquire pairs with the
 *                                     thieves' release of top.
 *   3. RELAXED store of the item    — the slot is still private (bottom unmoved).
 *   4. RELEASE fence                — publishes the item store ahead of the
 *                                     bottom bump so a thief that reads the new
 *                                     bottom also sees a fully-written slot.
 *   5. RELAXED store of bottom+1    — makes the item takeable.
 * --------------------------------------------------------------------------- */
int cl_push(cl_deque *d, cl_item x)
{
    int64_t   b = atomic_load_explicit(&d->bottom, memory_order_relaxed);
    int64_t   t = atomic_load_explicit(&d->top,    memory_order_acquire);
    cl_array *a = atomic_load_explicit(&d->array,  memory_order_relaxed);

    /* Full? The array holds `cap` items; if the live count b - t would exceed
     * cap - 1 after this push, double the array first. (cap - 1 leaves the
     * classic one-slot gap that keeps the empty/full states distinguishable.) */
    if (b - t > (int64_t)(a->cap - 1)) {
        if (grow(d, b, t) != 0)
            return -1;                  /* OOM: report; item not enqueued        */
        a = atomic_load_explicit(&d->array, memory_order_relaxed);
    }

    /* Store the payload while the slot is still private (bottom not yet moved). */
    atomic_store_explicit(&a->slot[b & a->mask], x, memory_order_relaxed);

    /* Release fence: on x86 this emits no instruction (TSO already orders it),
     * but on ARM/POWER it is a real barrier that prevents the item store from
     * being reordered after the bottom store below. */
    atomic_thread_fence(memory_order_release);

    /* Publish: bottom now counts the new item. Relaxed is enough because the
     * release fence above already ordered the item store before this one. */
    atomic_store_explicit(&d->bottom, b + 1, memory_order_relaxed);
    return 0;
}

/* ---------------------------------------------------------------------------
 * cl_take — OWNER ONLY. Pop from the bottom (LIFO — the most cache-hot item).
 * Returns CL_EMPTY if the deque is empty.
 *
 * The subtlety is the single-remaining-element case, where a thief may be trying
 * to steal the very item we are popping. We claim it by decrementing bottom,
 * then a SEQ_CST fence orders that decrement before our read of top; if only one
 * item remained we resolve the tie with the SAME CAS the thief would use, so the
 * item goes to exactly one of us.
 * --------------------------------------------------------------------------- */
cl_item cl_take(cl_deque *d)
{
    int64_t   b = atomic_load_explicit(&d->bottom, memory_order_relaxed) - 1;
    cl_array *a = atomic_load_explicit(&d->array,  memory_order_relaxed);

    /* Speculatively claim the bottom slot. A thief reading bottom now sees one
     * fewer item, which is what lets the fence-plus-CAS below arbitrate safely. */
    atomic_store_explicit(&d->bottom, b, memory_order_relaxed);

    /* SEQ_CST fence: the linchpin. It prevents the bottom store above from being
     * reordered after the top load below (x86's only native reordering, and the
     * general case on weak models). It puts THIS decrement and a thief's CAS on
     * top into one global order, so both cannot win the last element. */
    atomic_thread_fence(memory_order_seq_cst);

    int64_t t = atomic_load_explicit(&d->top, memory_order_relaxed);
    cl_item x;

    if (t <= b) {
        /* Non-empty. Read the candidate item; only the owner writes slots and no
         * push is in flight, so a relaxed load is correct. */
        x = atomic_load_explicit(&a->slot[b & a->mask], memory_order_relaxed);

        if (t == b) {
            /* Exactly one item, and a thief may be racing for it. Contend with a
             * CAS on top: success == we won; failure == a thief already took it,
             * so hand back CL_EMPTY. Either way top ends at t+1. */
            int64_t expected = t;
            if (!atomic_compare_exchange_strong_explicit(
                    &d->top, &expected, t + 1,
                    memory_order_seq_cst,     /* success: ties into the fence     */
                    memory_order_relaxed)) {  /* failure: no ordering needed      */
                x = CL_EMPTY;                 /* lost the race                    */
            }
            /* Deque is now empty; restore bottom to the canonical empty state
             * (bottom == top == t+1). */
            atomic_store_explicit(&d->bottom, b + 1, memory_order_relaxed);
        }
        return x;
    }

    /* t > b: the deque was already empty. Undo the speculative decrement so
     * bottom == top again, and report empty. */
    atomic_store_explicit(&d->bottom, b + 1, memory_order_relaxed);
    return CL_EMPTY;
}

/* ---------------------------------------------------------------------------
 * cl_steal — ANY THIEF. Take from the top (FIFO — the oldest, coarsest item).
 * Returns CL_EMPTY if empty, CL_ABORT if we lost the CAS (try another victim).
 *
 * Ordering:
 *   1. ACQUIRE load of top          — pairs with other thieves'/owner's release
 *                                     of top; we see what has already been stolen.
 *   2. SEQ_CST fence                — thief side of cl_take's fence: orders the
 *                                     top load before the bottom load so we can't
 *                                     miss an owner's take that emptied the deque.
 *   3. ACQUIRE load of bottom       — pairs with push's release fence so the slot
 *                                     we read below is a fully-published item.
 *   4. CONSUME load of array        — the array pointer; consume (== acquire in
 *                                     practice) so the slots it points at are the
 *                                     copied-in ones after a grow.
 *   5. RELAXED load of the slot     — speculative; only ours if the CAS wins.
 *   6. SEQ_CST CAS on top           — the single atomic RMW; commits the steal.
 * --------------------------------------------------------------------------- */
cl_item cl_steal(cl_deque *d)
{
    int64_t t = atomic_load_explicit(&d->top, memory_order_acquire);

    /* Fence between the top and bottom reads (see the header note and cl_take). */
    atomic_thread_fence(memory_order_seq_cst);

    int64_t b = atomic_load_explicit(&d->bottom, memory_order_acquire);
    cl_item x = CL_EMPTY;

    if (t < b) {
        /* Looks non-empty. Consume-load the array so a post-grow thief follows
         * the pointer to the array whose slots were already copied in. */
        cl_array *a = atomic_load_explicit(&d->array, memory_order_consume);

        /* Speculatively read the top slot. It only becomes "ours" if the CAS
         * below succeeds; until then another thief may read the same slot. */
        x = atomic_load_explicit(&a->slot[t & a->mask], memory_order_relaxed);

        /* Commit: advance top from t to t+1. If anyone moved top first, `expected`
         * no longer matches and the CAS fails -> we abandon x and report ABORT.
         * This is the ONLY atomic RMW on the steal path (one `lock cmpxchgq`). */
        int64_t expected = t;
        if (!atomic_compare_exchange_strong_explicit(
                &d->top, &expected, t + 1,
                memory_order_seq_cst,      /* success: order against take's fence */
                memory_order_relaxed)) {   /* failure: just retry elsewhere       */
            return CL_ABORT;
        }
    }
    return x;   /* the stolen item, or CL_EMPTY if the deque was empty (t >= b) */
}

/* ---------------------------------------------------------------------------
 * cl_size — relaxed, racy size estimate. Scheduling hint only (never used to
 * decide correctness), so momentarily-stale relaxed loads are acceptable.
 * --------------------------------------------------------------------------- */
int64_t cl_size(cl_deque *d)
{
    int64_t b = atomic_load_explicit(&d->bottom, memory_order_relaxed);
    int64_t t = atomic_load_explicit(&d->top,    memory_order_relaxed);
    int64_t n = b - t;
    return n < 0 ? 0 : n;   /* clamp: a transient take() decrement can make b<t  */
}
