/* ===========================================================================
 * treiber_stack.c — implementation. Every atomic here carries an explicit
 *                   memory order and a note on the exact race it prevents.
 * ===========================================================================
 */
#include "treiber_stack.h"
#include <stdlib.h>   /* malloc, free */

/* ---------------------------------------------------------------------------
 * tagged_push / tagged_pop — the shared engine.
 *
 * Both the live stack (`head`) and the recycled-node pool (`free_list`) are
 * Treiber stacks, so we write the CAS loop ONCE and use it for both. This is
 * also what makes reclamation safe: popping the live stack and pushing to the
 * free list are the same primitive, and a node only ever lives in one of the
 * two lists at a time (a CAS transfers exclusive ownership atomically).
 * --------------------------------------------------------------------------- */

/* tagged_push — splice `n` onto the top of the tagged list at *h. */
static void tagged_push(_Atomic(ts_tagged) *h, ts_node *n)
{
    /* RELAXED load: push never dereferences the old top, it only needs its
     * value to build the new node's ->next. The CAS re-validates it anyway, so
     * no acquire is required here. */
    ts_tagged old = atomic_load_explicit(h, memory_order_relaxed);
    ts_tagged neu;
    do {
        /* Point n at the current top. RELAXED store: n is still private to this
         * thread (nobody can reach it until the CAS below publishes it), so
         * there is nothing to synchronize with yet. The release on the CAS is
         * what will make this write visible to future poppers. */
        atomic_store_explicit(&n->next, old.ptr, memory_order_relaxed);
        neu.ptr = n;
        neu.tag = old.tag + 1;   /* bump the version: defeats ABA on this word */
    } while (!atomic_compare_exchange_weak_explicit(
                 h, &old, neu,
                 /* success = RELEASE: publish n. Everything written into n
                  * (its ->next just now, and ->value by the caller) becomes
                  * visible-before the new head to any thread that later
                  * ACQUIRE-loads head. Without release, a popper could observe
                  * the new head pointer but stale/garbage node fields. */
                 memory_order_release,
                 /* failure = RELAXED: a failed CAS synchronizes nothing; it
                  * merely refreshed `old` with the current head so we can
                  * retry. `weak` may also fail spuriously, which is fine — we
                  * are in a loop. */
                 memory_order_relaxed));
}

/* tagged_pop — detach and return the top node of the tagged list at *h, or
 * NULL if it is empty. */
static ts_node *tagged_pop(_Atomic(ts_tagged) *h)
{
    /* ACQUIRE load: we are about to dereference the top (read top->next), so we
     * must observe the pusher's fully-initialized node. This acquire
     * synchronizes-with the RELEASE in tagged_push. */
    ts_tagged old = atomic_load_explicit(h, memory_order_acquire);
    ts_tagged neu;
    do {
        if (old.ptr == NULL)
            return NULL;                 /* empty list */

        /* Read the current top's ->next to become the new head.
         *
         * RELAXED is sufficient *for the load itself* because `old.ptr` was
         * obtained by an ACQUIRE (either the initial load, or the acquire
         * failure-order of the CAS below), which already established
         * happens-before with the push that built this node — so its ->next is
         * visible.
         *
         * RACE NOTE: another thread may be concurrently recycling this very
         * node (it won the head CAS, and is now writing ->next as a free-list
         * link in tagged_push). That overlap is a *benign atomic race*: because
         * ->next is _Atomic, we read a clean old-or-new pointer, never a torn
         * word. If we read the "wrong" (recycled) value, our CAS below fails
         * anyway — the head's tag changed — and we discard `neu` and retry.
         * This is the crux of why type-stable memory + a tagged pointer is a
         * correct reclamation strategy: the stale read is harmless, not a
         * use-after-free, because the node's storage is never unmapped. */
        neu.ptr = atomic_load_explicit(&old.ptr->next, memory_order_relaxed);
        neu.tag = old.tag + 1;           /* bump version (defeats ABA) */
    } while (!atomic_compare_exchange_weak_explicit(
                 h, &old, neu,
                 /* success = ACQUIRE: after we win, the caller reads
                  * old.ptr->value; acquire guarantees that read sees the
                  * published node. (A release here would be pointless: pop
                  * publishes nothing.) */
                 memory_order_acquire,
                 /* failure = ACQUIRE: the CAS wrote a fresh head into `old`,
                  * and we will dereference old.ptr->next on the next spin, so
                  * that new pointer must be acquired too. */
                 memory_order_acquire));
    return old.ptr;
}

/* ---------------------------------------------------------------------------
 * Public API — thin, readable wrappers over the two-list engine.
 * --------------------------------------------------------------------------- */

void ts_init(treiber_stack *s)
{
    /* atomic_init is the ONLY correct way to initialize an atomic object; a
     * plain assignment is not guaranteed atomic and may race with nothing here
     * but sets a bad example. Both lists start empty, tag 0. */
    ts_tagged empty = { NULL, 0 };
    atomic_init(&s->head, empty);
    atomic_init(&s->free_list, empty);
}

int ts_push(treiber_stack *s, lf_value v)
{
    /* Try to recycle a node from the free list first. This is what keeps the
     * allocator out of the fast path AND keeps memory type-stable: nodes cycle
     * between `head` and `free_list`, never back to malloc, for the life of the
     * stack. Only when the pool is empty do we allocate. */
    ts_node *n = tagged_pop(&s->free_list);
    if (n == NULL) {
        n = (ts_node *)malloc(sizeof *n);
        if (n == NULL)
            return -1;                   /* honest OOM: report, never crash */
    }

    /* Write the payload BEFORE publishing the node. This plain store is ordered
     * before the release-CAS inside tagged_push, so poppers that acquire the
     * head see the correct value. */
    n->value = v;
    tagged_push(&s->head, n);
    return 0;
}

int ts_pop(treiber_stack *s, lf_value *out)
{
    ts_node *n = tagged_pop(&s->head);
    if (n == NULL)
        return 0;                        /* empty */

    /* Safe to read the payload: we own `n` exclusively now — the CAS in
     * tagged_pop removed it from the live stack, and it is not yet on the free
     * list, so no other thread can touch its ->value. */
    *out = n->value;

    /* Recycle instead of free(). This is the reclamation strategy: the node
     * stays mapped, so any concurrent stale dereference of it (see the RACE
     * NOTE in tagged_pop) reads valid memory. */
    tagged_push(&s->free_list, n);
    return 1;
}

void ts_destroy(treiber_stack *s)
{
    /* Teardown is single-threaded by contract, so we can drain both lists with
     * the same primitive and actually free() the storage now that no thread can
     * race us. */
    ts_node *n;
    while ((n = tagged_pop(&s->head)) != NULL)
        free(n);
    while ((n = tagged_pop(&s->free_list)) != NULL)
        free(n);
}
