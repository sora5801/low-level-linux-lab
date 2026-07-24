/* ===========================================================================
 * treiber_stack.h — a lock-free LIFO stack (Treiber, 1986) with a 128-bit
 *                   tagged pointer to defeat ABA, and a type-stable free list
 *                   to make node reclamation safe.
 * ===========================================================================
 *
 * Treiber's stack is the "hello world" of lock-free programming: push and pop
 * are each a single compare-and-swap on the head pointer. Its subtlety — the
 * reason it is a teaching centerpiece — is the ABA problem and what you must do
 * about MEMORY RECLAMATION. This project attacks both with the classic pairing:
 *
 *   1. ABA  -> a version-counted ("tagged") head. head is not a bare pointer but
 *              {pointer, 64-bit counter}, updated together by a double-width CAS
 *              (x86 `lock cmpxchg16b`). Every successful update bumps the
 *              counter, so "the same pointer, later" is a DIFFERENT 128-bit word.
 *
 *   2. Reclamation -> a type-stable free list. Popped nodes are never handed
 *              back to malloc while the stack is live; they go on an internal
 *              lock-free free list and are recycled by future pushes. Because
 *              the memory is never unmapped, a thread that dereferences a stale
 *              head pointer reads valid (if outdated) node memory rather than
 *              faulting — and the tag then makes its CAS fail and retry. This is
 *              exactly why tagged pointers and node pooling go together.
 * ===========================================================================
 */
#ifndef TREIBER_STACK_H
#define TREIBER_STACK_H

#include "lockfree.h"

/* A stack node. `next` MUST be atomic: while one thread reads a node's ->next
 * during a pop, another thread that recycled the node may be writing ->next as
 * a free-list link. Making it _Atomic turns that overlap into a benign atomic
 * race (a clean old-or-new read, never a torn value) instead of undefined
 * behavior — see the long comment in treiber_stack.c. */
typedef struct ts_node {
    _Atomic(struct ts_node *) next;   /* next-lower node, or NULL at bottom   */
    lf_value                  value;  /* the payload                          */
} ts_node;

/* The tagged head: a pointer packed with a 64-bit ABA version counter. Sixteen
 * bytes, updated atomically by cmpxchg16b. A full 64-bit counter (versus the
 * 16-bit tag the packed-64 demo uses) is astronomically far from wrapping, so
 * ABA is effectively impossible, not merely improbable. */
typedef struct ts_tagged {
    ts_node  *ptr;   /* the node this head points at        */
    uintptr_t tag;   /* monotonically increasing version    */
} ts_tagged;

_Static_assert(sizeof(ts_tagged) == 16,
               "ts_tagged must be exactly 16 bytes for lock cmpxchg16b");

/* The stack. `head` and `free_list` are each on their own cache line: they are
 * independent contention hot spots (pushers hammer head, poppers touch both),
 * so we keep a write to one from invalidating the other's cached line. The
 * _Alignas(LF_CACHELINE) also guarantees the 16-byte alignment cmpxchg16b
 * requires. */
typedef struct treiber_stack {
    _Alignas(LF_CACHELINE) _Atomic(ts_tagged) head;       /* the live LIFO   */
    _Alignas(LF_CACHELINE) _Atomic(ts_tagged) free_list;  /* recycled nodes  */
} treiber_stack;

void ts_init(treiber_stack *s);
/* ts_push: returns 0 on success, -1 if a fresh node allocation failed (OOM). */
int  ts_push(treiber_stack *s, lf_value v);
/* ts_pop: writes the popped value through *out and returns 1; returns 0 if the
 * stack was empty. */
int  ts_pop(treiber_stack *s, lf_value *out);
/* ts_destroy: SINGLE-THREADED teardown. Frees every node in both lists. Must be
 * called with no concurrent operations in flight. */
void ts_destroy(treiber_stack *s);

#endif /* TREIBER_STACK_H */
