/* ===========================================================================
 * chase_lev.h — a growable Chase-Lev work-stealing deque.
 * ===========================================================================
 *
 * ONE OWNER, MANY THIEVES. Each worker thread owns exactly one of these deques.
 *   - The OWNER calls cl_push / cl_take on the "bottom" end (LIFO). These run on
 *     the fast path with NO atomic read-modify-write in the common case — just
 *     atomic loads and stores — because only the owner ever writes `bottom`.
 *   - Any number of THIEVES call cl_steal on the "top" end (FIFO), racing each
 *     other and the owner with a single CAS on `top`.
 *
 * This is *the* data structure behind Cilk, Intel TBB, Rust's rayon, the Go
 * scheduler's per-P run queue, and Java's ForkJoinPool. The reference for the
 * exact memory orders used below is Lê, Pop, Cohen & Nardelli, "Correct and
 * Efficient Work-Stealing for Weak Memory Models" (PPoPP 2013), which fixed the
 * ordering bugs in the original 2005 paper for weak models (ARM/POWER).
 *
 * See asm/demo.annotated.s for the push/take/steal lowered to x86-64 — that is
 * where you can watch the seq_cst fence become `mfence`, the release fence
 * become nothing, and the CAS become `lock cmpxchgq`.
 * ===========================================================================
 */
#ifndef CHASE_LEV_H
#define CHASE_LEV_H

#include <stdatomic.h>   /* C11 atomics: atomic_load_explicit, memory_order_*   */
#include <stddef.h>      /* size_t                                              */
#include <stdint.h>      /* int64_t — the signed index type                     */

/* A cache line on x86-64 (and Apple/ARM server parts) is 64 bytes. Two atomics
 * that live in the same line ping-pong that line between cores' caches even when
 * the threads touch *different* variables — "false sharing". We pad `top` and
 * `bottom` onto separate lines below for exactly this reason. */
#define CL_CACHELINE 64

/* The deque stores one pointer-sized payload per slot. In this project it is a
 * `Task *` (see threadpool.h), but the deque itself is payload-agnostic. */
typedef void *cl_item;

/* Out-of-band results. We never enqueue these as real items, so they are
 * unambiguous. cl_take/cl_steal return CL_EMPTY when there was nothing for us;
 * cl_steal additionally returns CL_ABORT when it lost the CAS race (the caller
 * should try a different victim rather than spin on this one). */
#define CL_EMPTY  ((cl_item)0)
#define CL_ABORT  ((cl_item)-1)

/* ---------------------------------------------------------------------------
 * The backing store is a power-of-two circular buffer held in a SEPARATE heap
 * object so it can be swapped for a larger one on overflow without tearing a
 * concurrent thief's read. `mask == cap - 1` turns `index % cap` into a single
 * AND, which is why cap is always a power of two.
 *
 * `prev` chains superseded arrays so cl_destroy can free them. We CANNOT free an
 * old array at grow time: a thief that loaded the array pointer just before the
 * swap may still be reading from it. This is the classic safe-memory-reclamation
 * hazard; deferring all frees to destroy (when every worker has joined) is the
 * simplest correct answer for a fixed-lifetime pool. Production uses hazard
 * pointers or epoch-based reclamation instead — see the README.
 * --------------------------------------------------------------------------- */
typedef struct cl_array {
    size_t             cap;    /* number of slots (a power of two)               */
    size_t             mask;   /* cap - 1                                        */
    struct cl_array   *prev;   /* previous (smaller) array, kept until destroy   */
    _Atomic(cl_item)  *slot;   /* cap atomic slots; racy reads must be atomic    */
} cl_array;

/* ---------------------------------------------------------------------------
 * The deque. `top` and `bottom` are 64-bit SIGNED indices. Signedness matters:
 * cl_take speculatively decrements `bottom` to -1 on an empty deque, and the
 * emptiness test `top <= bottom` only works if that -1 stays negative (unsigned
 * would wrap to a huge value and misreport non-empty).
 *
 * Layout is hand-padded so the two hot indices never share a cache line:
 *   - `bottom` is written by the owner on every push/take (very frequently);
 *   - `top` is read by every thief and CAS'd on every successful steal.
 * If they shared a line, each owner push would invalidate the thieves' cached
 * copy of `top`, serializing cores that are logically independent. The `_pad`
 * arrays cost 128 bytes of memory to buy back that parallelism.
 * --------------------------------------------------------------------------- */
typedef struct cl_deque {
    _Alignas(CL_CACHELINE) _Atomic(int64_t) top;    /* steal end (thieves CAS)  */
    char _pad0[CL_CACHELINE - sizeof(int64_t)];

    _Alignas(CL_CACHELINE) _Atomic(int64_t) bottom; /* owner end (owner r/w)    */
    char _pad1[CL_CACHELINE - sizeof(int64_t)];

    _Alignas(CL_CACHELINE) _Atomic(cl_array *) array;/* current backing array   */
    char _pad2[CL_CACHELINE - sizeof(cl_array *)];
} cl_deque;

/* API. init returns 0 on success, -1 on allocation failure. push/take/steal are
 * described at their definitions in chase_lev.c. */
int      cl_init(cl_deque *d, size_t init_cap);
void     cl_destroy(cl_deque *d);
int      cl_push(cl_deque *d, cl_item x);   /* owner only; 0 ok, -1 on OOM grow  */
cl_item  cl_take(cl_deque *d);              /* owner only; LIFO; CL_EMPTY if none */
cl_item  cl_steal(cl_deque *d);             /* thief; FIFO; CL_EMPTY/CL_ABORT     */

/* A relaxed, racy size estimate (bottom - top). Used only as a scheduling hint
 * ("does this deque look non-empty?"), never for correctness, so relaxed loads
 * that may be momentarily stale are fine. */
int64_t  cl_size(cl_deque *d);

#endif /* CHASE_LEV_H */
