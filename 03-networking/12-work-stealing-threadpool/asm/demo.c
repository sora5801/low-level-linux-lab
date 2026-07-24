/* ===========================================================================
 * demo.c — the Chase-Lev work-stealing deque, extracted to pure logic.
 * ===========================================================================
 *
 * WHY THIS FILE EXISTS
 * --------------------
 * The real project (../threadpool.c, ../chase_lev.c) needs <stdlib.h> (malloc
 * for the growable array), <pthread.h>, and the raw futex/affinity syscalls, so
 * it is NOT a self-contained translation unit — clang cannot lower it to Linux
 * asm on this host without those headers. Per the lab convention we therefore
 * extract the single most instructive, purely-computational routine — the
 * Chase-Lev push / take / steal index dance and its `lock cmpxchg` — into this
 * header-free file so it can be compiled to didactic assembly.
 *
 * There are NO system headers here. We spell out our own integer types and use
 * the compiler's `__atomic_*` builtins (predefined; no include needed) instead
 * of <stdatomic.h>. Each builtin takes an explicit memory-order argument via the
 * compiler-provided __ATOMIC_* constants:
 *
 *     __ATOMIC_RELAXED = 0   no ordering, only atomicity
 *     __ATOMIC_CONSUME = 1   (treated as ACQUIRE by every real compiler)
 *     __ATOMIC_ACQUIRE = 2   later loads/stores can't hoist above this load
 *     __ATOMIC_RELEASE = 3   earlier loads/stores can't sink below this store
 *     __ATOMIC_ACQ_REL = 4   both, for read-modify-write
 *     __ATOMIC_SEQ_CST = 5   a single total order across ALL seq_cst ops
 *
 * THE ALGORITHM (Chase & Lev, 2005; memory orders per Lê/Pop/Cohen/Nardelli,
 * "Correct and Efficient Work-Stealing for Weak Memory Models", PPoPP 2013)
 * ------------------------------------------------------------------------
 * One deque per worker. Two indices into a circular buffer:
 *
 *     top     -->  the STEAL end.  Thieves take from here (FIFO), racing each
 *                  other and the owner with a CAS. Only ever increases.
 *     bottom  -->  the OWNER end.  The owning worker pushes and pops here (LIFO),
 *                  with NO atomic RMW on the fast path — just loads and stores.
 *
 *     indices:   0    1    2    3    4    5   ...
 *              [ .    A    B    C    .    .  ]
 *                     ^top            ^bottom
 *                     (thieves)       (owner pushes/pops)
 *
 * The number of items is `bottom - top`. Owner and thieves only contend on the
 * ONE boundary case — a single remaining element that both a `take` and a
 * `steal` want — and that tie is broken by a CAS on `top`. Everything else is
 * uncontended, which is the entire performance argument for the structure.
 *
 * Indices are SIGNED (long). `take` speculatively decrements `bottom` to -1 on
 * an empty deque; signed comparison `t <= b` then correctly reports "empty"
 * (0 <= -1 is false). With unsigned indices that underflow would read as a huge
 * positive value and corrupt the emptiness test.
 * ===========================================================================
 */

/* Our own types — no <stdint.h>. On the LP64 model Linux uses for x86-64, a
 * `long` is 64 bits, wide enough for a monotonically increasing index that will
 * not realistically wrap, and for a pointer-sized payload. */
typedef long  i64;            /* signed 64-bit: the top/bottom indices          */
typedef void *item;           /* deque payload: one pointer-sized word per slot */

/* Sentinels returned by take/steal. We never store these as real items, so they
 * are unambiguous out-of-band signals:
 *   EMPTY  — the deque had no item for us.
 *   ABORT  — (steal only) we lost the CAS race for the last item; caller retries
 *            a different victim rather than spinning on this one. */
#define EMPTY  ((item)0)
#define ABORT  ((item)-1)

/* The backing store: a power-of-two circular buffer. `mask == cap - 1` turns the
 * modulo `index % cap` into a single AND (`index & mask`), which is why cap must
 * be a power of two. In the full project this array can be atomically swapped
 * for a larger one on overflow; here it is fixed so the file stays header-free
 * (no malloc) and the asm shows the pure index logic. */
typedef struct {
    i64    cap;               /* number of slots (a power of two)                */
    i64    mask;              /* cap - 1                                         */
    item  *slot;             /* cap contiguous pointer-sized slots              */
} array;

/* The deque itself is just two indices and a pointer to the current array.
 * In the real code `top` and `bottom` sit on SEPARATE cache lines so the owner's
 * frequent `bottom` writes do not invalidate the line thieves cache `top` in
 * (false sharing). That padding is irrelevant to the logic, so it is omitted
 * here to keep the generated assembly focused on the algorithm. */
typedef struct {
    i64    top;               /* steal end  — CAS'd up by thieves                */
    i64    bottom;            /* owner end  — plain load/store by the owner      */
    array *arr;               /* current backing array                          */
} deque;

/* ---------------------------------------------------------------------------
 * dq_push — OWNER ONLY. Append `x` at the bottom (the LIFO end).
 *
 * The subtle part is the ordering, not the arithmetic:
 *   1. We write the item into the slot with a RELAXED store — at this instant no
 *      thief can legally see the slot yet, because `bottom` has not advanced.
 *   2. A RELEASE fence then publishes that store: any thief that later reads the
 *      new `bottom` and, through it, this slot, is guaranteed (by pairing with
 *      the thief's ACQUIRE load of bottom) to observe the fully-written item and
 *      not a stale/garbage word. Without this fence a weak memory model (ARM,
 *      POWER) could reorder the slot write after the bottom write and hand a
 *      thief uninitialized memory.
 *   3. The RELAXED store of `bottom+1` makes the item visible. It is the owner's
 *      private index for writing, so it needs no stronger order of its own.
 * (Capacity/overflow handling is omitted in this extraction — see chase_lev.c.)
 * --------------------------------------------------------------------------- */
void dq_push(deque *d, item x)
{
    /* Both indices are the owner's to read cheaply here: bottom is owner-private,
     * and we take an ACQUIRE snapshot of top only to compute the size in the full
     * version. RELAXED/ACQUIRE respectively; neither needs seq_cst. */
    i64    b = __atomic_load_n(&d->bottom, __ATOMIC_RELAXED);
    array *a = d->arr;                       /* no resize here => plain load     */

    /* Store the payload FIRST, while the slot is still private (bottom unmoved). */
    __atomic_store_n(&a->slot[b & a->mask], x, __ATOMIC_RELAXED);

    /* Publish it: the release fence orders the slot write before the bottom bump
     * so a stealing thread that sees the new bottom also sees the item. */
    __atomic_thread_fence(__ATOMIC_RELEASE);

    /* Advance bottom. Now `bottom - top` counts one more; the item is takeable. */
    __atomic_store_n(&d->bottom, b + 1, __ATOMIC_RELAXED);
}

/* ---------------------------------------------------------------------------
 * dq_take — OWNER ONLY. Pop from the bottom (LIFO — the most-recently pushed,
 * hence the most cache-hot, item). Returns EMPTY if the deque is empty.
 *
 * This is the delicate half of Chase-Lev. The owner optimistically claims a slot
 * by DECREMENTING bottom before it knows whether a thief is simultaneously
 * claiming that same last slot from the top. A full (seq_cst) fence between the
 * bottom write and the top read is what forces a single global order between
 * THIS decrement and a thief's CAS, so exactly one of them wins the last item.
 * --------------------------------------------------------------------------- */
item dq_take(deque *d)
{
    /* Speculatively claim the bottom slot by lowering bottom by one. */
    i64    b = __atomic_load_n(&d->bottom, __ATOMIC_RELAXED) - 1;
    array *a = d->arr;
    __atomic_store_n(&d->bottom, b, __ATOMIC_RELAXED);

    /* The linchpin. This seq_cst fence prevents the CPU from reordering the
     * bottom store (above) after the top load (below). Combined with the seq_cst
     * CAS in steal(), it yields a total order in which the owner and a thief
     * cannot both believe they hold the last element. */
    __atomic_thread_fence(__ATOMIC_SEQ_CST);

    i64  t = __atomic_load_n(&d->top, __ATOMIC_RELAXED);
    item x;

    if (t <= b) {
        /* At least one element remains for us. Read it (still safe: only the
         * owner writes slots, and only via push which is not running now). */
        x = __atomic_load_n(&a->slot[b & a->mask], __ATOMIC_RELAXED);

        if (t == b) {
            /* Exactly ONE element, and a thief may be racing us for it. Try to
             * bump top ourselves: if the CAS succeeds we won it; if it fails a
             * thief already took it, so we return EMPTY. Either way `top` ends
             * up at t+1. The seq_cst success order ties into the fence above. */
            if (!__atomic_compare_exchange_n(&d->top, &t, t + 1,
                                             /*weak=*/0,
                                             __ATOMIC_SEQ_CST,
                                             __ATOMIC_RELAXED)) {
                x = EMPTY;                   /* lost the race — thief got it      */
            }
            /* Deque is now empty; reset bottom to the canonical empty state so
             * bottom == top == t+1. */
            __atomic_store_n(&d->bottom, b + 1, __ATOMIC_RELAXED);
        }
        return x;
    }

    /* t > b: the deque was already empty. Undo our speculative decrement so
     * bottom is restored to top (the canonical empty state) and report EMPTY. */
    __atomic_store_n(&d->bottom, b + 1, __ATOMIC_RELAXED);
    return EMPTY;
}

/* ---------------------------------------------------------------------------
 * dq_steal — ANY THIEF. Take from the top (FIFO — the OLDEST item). Returns
 * EMPTY if the deque is empty, ABORT if we lost the CAS to another thief or the
 * owner (the caller should try a different victim, not retry blindly).
 *
 * Why steal FIFO while the owner pops LIFO? In divide-and-conquer workloads the
 * oldest item at `top` is the coarsest, least-decomposed chunk — stealing it
 * hands the thief a big slice of work per (expensive) steal, while the owner
 * keeps churning the fine, cache-hot items at `bottom`.
 * --------------------------------------------------------------------------- */
item dq_steal(deque *d)
{
    /* ACQUIRE-load top first: pairs with the owner's/other thieves' release of
     * top so we see a consistent view of what has already been stolen. */
    i64 t = __atomic_load_n(&d->top, __ATOMIC_ACQUIRE);

    /* seq_cst fence between reading top and reading bottom — the thief-side
     * partner of the fence in take(). It guarantees that if the owner's take()
     * has moved bottom below top, we observe it here and back off. */
    __atomic_thread_fence(__ATOMIC_SEQ_CST);

    i64  b = __atomic_load_n(&d->bottom, __ATOMIC_ACQUIRE);
    item x = EMPTY;

    if (t < b) {
        /* Non-empty as far as we can tell. Read the top slot BEFORE the CAS —
         * the read is speculative and only becomes "ours" if the CAS below wins.
         * The ACQUIRE on bottom (paired with push's release fence) ensures this
         * slot's contents are the fully-published item, not a torn write. */
        array *a = d->arr;
        x = __atomic_load_n(&a->slot[t & a->mask], __ATOMIC_RELAXED);

        /* Commit by advancing top from t to t+1 with a CAS. If another thief (or
         * the owner in take's single-element path) moved top first, `t` no longer
         * matches and the CAS fails: we abandon the speculatively-read item and
         * report ABORT. This is the ONLY atomic RMW on the steal fast path — it
         * lowers to a single `lock cmpxchg` (see demo.annotated.s). */
        if (!__atomic_compare_exchange_n(&d->top, &t, t + 1,
                                         /*weak=*/0,
                                         __ATOMIC_SEQ_CST,
                                         __ATOMIC_RELAXED)) {
            return ABORT;                    /* lost the race — try another victim*/
        }
    }
    return x;                                /* real item, or EMPTY if t >= b     */
}
