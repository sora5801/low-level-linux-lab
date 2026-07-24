/* ===========================================================================
 * heap.h — the bump + segregated-free-list allocator that backs GC objects.
 * ===========================================================================
 *
 * This is the capstone's stand-in for sibling 02-systems-tools/05-malloc, tuned
 * for a garbage-collected heap: allocation is a pointer bump in the common case,
 * and freed blocks (produced by the GC's sweep) go onto size-classed free lists
 * for O(1) reuse. There is no coalescing and no per-block boundary tag search —
 * the GC hands us back whole objects, so we never merge neighbours.
 *
 * ARENAS: memory comes from the kernel in big chunks via mmap(MAP_ANONYMOUS)
 * (see heap.c). We bump-allocate down an arena; when it is exhausted we mmap
 * another and chain it. Arenas are only unmapped at shutdown.
 *
 * SIZE CLASSES: every request is rounded up to a 16-byte multiple (so every
 * payload stays 16-aligned, matching the SysV ABI's max_align). Rounded sizes
 * from 16..HEAP_SMALL_MAX map to an exact free list (`freeLists[size/16]`), so a
 * reused block is always exactly the right size. Larger blocks share one
 * best-effort list reused only on an exact size match.
 *
 * GC TRIGGERING lives here too: every allocation bumps `bytesAllocated`, and when
 * it crosses `nextGC` (and the GC is enabled) we run a full collection BEFORE
 * carving the new block. After a collection `nextGC` is regrown to
 * bytesAllocated * GC_HEAP_GROW_FACTOR so collections amortize as the live set grows.
 */
#ifndef LUMEN_HEAP_H
#define LUMEN_HEAP_H

#include "common.h"

typedef struct Obj Obj;

#define HEAP_ALIGN        16u                 /* payload alignment == max_align */
#define HEAP_SMALL_MAX    1024u               /* rounded sizes <= this are exact-binned */
#define HEAP_NUM_SMALL    (HEAP_SMALL_MAX / HEAP_ALIGN + 1) /* freeLists index 0..64 */
#define GC_HEAP_GROW_FACTOR 2                 /* nextGC = liveBytes * this        */

/* A reclaimed block, reinterpreted as a free-list node. It overlays the object's
 * own storage, so the minimum object size must be >= sizeof(FreeBlock) — which it
 * is, since the smallest object (ObjString header) already exceeds 16 bytes. */
typedef struct FreeBlock {
    struct FreeBlock *next;
    size_t            size;   /* rounded block size (needed for the "big" list) */
} FreeBlock;

typedef struct Arena {
    struct Arena *next;       /* chain of all mmap'd arenas (freed at shutdown) */
    char         *base;       /* start of the usable region                    */
    char         *cursor;     /* bump pointer: next free byte                   */
    char         *end;        /* one past the arena                            */
    size_t        mapLength;  /* bytes passed to munmap at shutdown            */
} Arena;

typedef struct {
    Arena     *arenas;                    /* head of the arena chain            */
    FreeBlock *freeLists[HEAP_NUM_SMALL]; /* exact-fit lists, index = size/16   */
    FreeBlock *bigFree;                   /* > HEAP_SMALL_MAX, exact-match reuse */

    Obj       *objects;        /* intrusive list of EVERY live object (sweep walks it) */
    size_t     bytesAllocated; /* live bytes handed out (grows on alloc, shrinks on free) */
    size_t     nextGC;         /* collect when bytesAllocated crosses this      */
    bool       gcEnabled;      /* false during compile/startup, true during run */
} Heap;

/* The single global heap. One global (like the VM) keeps the allocator and the
 * collector trivially able to see the same object list and counters. */
extern Heap heap;

void   heapInit(void);        /* map the first arena, zero the bins            */
void   heapShutdown(void);    /* munmap every arena                            */

/* Round `n` up to the allocator's granularity (>= sizeof(FreeBlock)). */
size_t heapRoundUp(size_t n);

/* Allocate `rounded` bytes (MUST be a heapRoundUp() result). May trigger a GC.
 * Returns 16-byte-aligned storage; the caller (allocateObject) fills the header. */
void  *heapAlloc(size_t rounded);

/* Return a swept block of `rounded` bytes to its free list. */
void   heapFree(void *block, size_t rounded);

#endif /* LUMEN_HEAP_H */
