/* ===========================================================================
 * gc.h — public interface of a conservative mark-and-sweep garbage collector.
 * ===========================================================================
 *
 * This is a Boehm-style CONSERVATIVE collector: you allocate with gc_malloc()
 * and simply never free. The collector reclaims an object once it can no longer
 * find any word — in your registers, on your stack, in your globals, or inside
 * another reachable object — that *looks like* a pointer to it. "Looks like" is
 * the whole idea: we do not know your types, so we treat every aligned machine
 * word that falls inside the heap as a possible pointer. That is why it is
 * called *conservative* — it never reclaims something that might still be in
 * use, at the price of occasionally keeping true garbage alive (a stack word
 * that merely holds an integer numerically equal to a heap address). See the
 * README for the full argument and for the register/stack-scanning trick.
 *
 * The design, on-heap layout, the reserve/commit page trick, the object table,
 * the mark bitmap, and the exact root-scanning procedure are all documented at
 * the top of gc.c — read that file for the "why". This header is deliberately
 * thin.
 *
 * Platform: Linux / WSL only. We call mmap/mprotect(2), setjmp(3) (to spill the
 * callee-saved registers), and read /proc/self/stat to find the stack bottom.
 * It will not build on native Windows. The teaching *assembly* under asm/ is
 * host-portable because clang cross-targets Linux to emit it.
 * ===========================================================================
 */
#ifndef GC_H
#define GC_H

#include <stddef.h>   /* size_t */

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Lifecycle -------------------------------------------------------------
 * gc_init() MUST be called once, as early in main() as possible — before you
 * create any roots you expect to survive a collection. It captures the stack
 * bottom (the highest stack address, since the stack grows downward on x86-64)
 * and reserves the heap's virtual address range. Calling it late means frames
 * above the capture point are invisible to the scanner and their pointers would
 * be missed — a use-after-free the collector itself introduced. */
void   gc_init(void);

/* ---- Allocation ------------------------------------------------------------
 * gc_malloc(n): a zeroed, 16-byte-aligned block of at least n bytes whose
 * interior WILL be scanned for pointers during collection (use it for anything
 * that may contain pointers: structs, nodes, arrays of pointers).
 *
 * gc_malloc_atomic(n): same, but the block is marked "atomic" = pointer-free,
 * so the collector never scans its interior. Use it for leaf data — strings,
 * pixel buffers, bignum limbs. This is both faster (nothing to scan) and more
 * precise (random bytes in a string can never be mistaken for a pointer and so
 * can never cause false retention). This mirrors Boehm's GC_malloc_atomic.
 *
 * Both may trigger a collection internally when allocation pressure crosses a
 * threshold, so DO keep a live pointer to anything you still need across a call
 * to gc_malloc — a value that exists only as, say, a freshly-cast integer is
 * not a root. Both return NULL only when the fixed virtual reserve is exhausted
 * (a genuine OOM); the returned memory is zero-filled. */
void  *gc_malloc(size_t nbytes);
void  *gc_malloc_atomic(size_t nbytes);

/* ---- Collection ------------------------------------------------------------
 * Force a full stop-the-world mark-and-sweep now and return the number of bytes
 * reclaimed. You rarely need to call this — allocation drives collection — but
 * it is the interesting entry point to watch, and the demo calls it explicitly
 * to show objects dying on cue. */
size_t gc_collect(void);

/* ---- Diagnostics -----------------------------------------------------------
 * A snapshot of collector state, filled by gc_get_stats(). All byte counts are
 * payload bytes (they exclude the collector's own bitmap/table overhead). */
struct gc_stats {
    size_t heap_reserved;    /* virtual bytes reserved up front (PROT_NONE)      */
    size_t heap_committed;   /* bytes actually backed by pages (mprotect'd)      */
    size_t bytes_live;       /* payload of objects that survived the last GC     */
    size_t objects_live;     /* number of live objects                           */
    size_t bytes_free;       /* payload sitting on the reuse free list           */
    size_t total_allocated;  /* cumulative bytes ever handed out by gc_malloc    */
    size_t collections;      /* number of mark-and-sweep cycles run              */
    size_t last_reclaimed;   /* bytes freed by the most recent collection        */
};
void   gc_get_stats(struct gc_stats *out);

/* Print a one-line human-readable summary of the stats to stderr (raw write(2),
 * never printf, so it is safe to call even from inside allocation paths). */
void   gc_dump(const char *label);

#ifdef __cplusplus
}
#endif

#endif /* GC_H */
