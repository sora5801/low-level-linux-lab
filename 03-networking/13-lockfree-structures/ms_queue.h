/* ===========================================================================
 * ms_queue.h — the Michael & Scott lock-free FIFO queue (PODC 1996), the
 *              MPMC (multi-producer/multi-consumer) workhorse behind most
 *              real lock-free queues, with hazard-pointer reclamation.
 * ===========================================================================
 *
 * STRUCTURE
 * ---------
 * A singly-linked list with a permanent DUMMY node. `head` always points at the
 * dummy (whose successor, if any, is the real front); `tail` points at (or just
 * behind) the last node. The dummy is what makes the empty and non-empty cases
 * uniform and lets enqueue and dequeue operate on opposite ends without ever
 * contending on the same word in the common case.
 *
 * TWO INVARIANTS THE ALGORITHM MAINTAINS
 * --------------------------------------
 *   1. tail never points *past* the last node — at worst it "lags" one node
 *      behind, and any thread that notices helps swing it forward (a CAS). This
 *      cooperative helping is what keeps the structure lock-free: no single
 *      thread's stall can block others.
 *   2. head->next is the real front; dequeue advances head to head->next and
 *      returns that successor's value, retiring the OLD dummy.
 *
 * WHY HAZARD POINTERS HERE (and tagged pointers on the stack)?
 * -----------------------------------------------------------
 * Dequeue frees the old dummy node back to malloc. Another thread may still be
 * mid-dereference of that node. So we cannot recycle-forever like the stack;
 * we need real Safe Memory Reclamation, which is exactly hazard pointers. Every
 * dereference of a shared node is guarded by publishing it to a hazard slot
 * first (see hazard.h), and the freed node is only actually released once no
 * hazard slot names it.
 * ===========================================================================
 */
#ifndef MS_QUEUE_H
#define MS_QUEUE_H

#include "lockfree.h"
#include "hazard.h"

typedef struct msq_node {
    _Atomic(struct msq_node *) next;   /* successor; NULL at the tail          */
    lf_value                   value;  /* payload (undefined in the dummy node) */
} msq_node;

/* head and tail live on separate cache lines: producers hammer `tail`,
 * consumers hammer `head`, and we do not want those two hot words sharing a
 * line and ping-ponging between cores. */
typedef struct ms_queue {
    _Alignas(LF_CACHELINE) _Atomic(msq_node *) head;
    _Alignas(LF_CACHELINE) _Atomic(msq_node *) tail;
} ms_queue;

/* Returns 0 on success, -1 if the initial dummy allocation failed. */
int  msq_init(ms_queue *q);

/* Enqueue v. Uses hazard slots of `hp`. Returns 0, or -1 on node OOM. */
int  msq_enqueue(ms_queue *q, hp_thread *hp, lf_value v);

/* Dequeue into *out. Returns 1 on success, 0 if the queue was empty. */
int  msq_dequeue(ms_queue *q, hp_thread *hp, lf_value *out);

/* SINGLE-THREADED teardown: frees the dummy and any remaining nodes. */
void msq_destroy(ms_queue *q);

#endif /* MS_QUEUE_H */
