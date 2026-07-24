/* ===========================================================================
 * lockfree.h — shared vocabulary for the lock-free structures in this project.
 * ===========================================================================
 *
 * There is intentionally very little here: the whole point of the project is
 * that a correct lock-free structure needs almost no machinery beyond C11
 * atomics and the right memory orders. This header pins down the few things all
 * three structures share: the payload type, the cache-line size (for padding
 * away false sharing), and a CPU "relax" hint for spin/backoff.
 * ===========================================================================
 */
#ifndef LOCKFREE_H
#define LOCKFREE_H

#include <stdatomic.h>  /* C11 atomics: atomic_*, memory_order_*             */
#include <stdint.h>     /* uintptr_t                                         */
#include <stddef.h>     /* size_t                                            */

/* The payload we store in the stack and queue. A machine word: big enough to
 * hold either a small integer or a pointer, which is all a teaching container
 * needs. (A production container would be generic over element size.) */
typedef uintptr_t lf_value;

/* Size of an x86-64 / arm64 cache line. Two atomics that different cores hammer
 * must not share a line, or every write to one bounces the other's cached copy
 * between cores ("false sharing") — a silent 10x slowdown. We pad hot fields to
 * this granularity. */
#define LF_CACHELINE 64

/* lf_pause — hint to the CPU that we are in a spin-wait retry.
 *
 * On x86 the `pause` instruction (a) throttles the speculative pipeline so we
 * don't burn power and issue slots hammering a CAS that keeps failing, and
 * (b) mitigates the memory-order-violation pipeline flush a tight CAS loop
 * would otherwise suffer. On arm64 `yield` is the analogue. Neither changes
 * correctness — a lock-free algorithm is correct without any backoff — but both
 * dramatically improve throughput under contention.
 *
 * It also carries a "memory" clobber so the compiler does not hoist loads out
 * of the spin loop, which would turn our re-read of a shared word into a
 * dead spin on a stale register. */
static inline void lf_pause(void)
{
#if defined(__x86_64__) || defined(__i386__)
    __asm__ __volatile__("pause" ::: "memory");
#elif defined(__aarch64__)
    __asm__ __volatile__("yield" ::: "memory");
#else
    /* Portable fallback: a compiler barrier is enough to keep the loop honest. */
    atomic_signal_fence(memory_order_seq_cst);
#endif
}

#endif /* LOCKFREE_H */
