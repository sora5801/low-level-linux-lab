/* ===========================================================================
 * memory.h — the ONE allocator every part of the VM funnels through, plus the
 *            public entry points of the mark-sweep garbage collector.
 * ===========================================================================
 *
 * Routing every heap operation through a single `reallocate()` gives the GC two
 * things it cannot work without:
 *   1. an exact running total of bytes allocated (so it knows when to collect);
 *   2. a single choke point at which to *decide* to collect — we check the
 *      threshold on every growth, and under DEBUG_STRESS_GC we collect on every
 *      allocation to shake out missing-root bugs deterministically.
 * ===========================================================================
 */
#ifndef CLOXI_MEMORY_H
#define CLOXI_MEMORY_H

#include "common.h"
#include "object.h"

/* Allocate a zero-based array of `count` T's. Wraps reallocate so the byte
 * accounting stays centralized. Used for chars[], code[], Value[] blocks. */
#define ALLOCATE(type, count) \
    ((type *)reallocate(NULL, 0, sizeof(type) * (size_t)(count)))

/* Free a single object of type `type` (oldSize known, newSize 0). */
#define FREE(type, pointer) \
    reallocate(pointer, sizeof(type), 0)

/* Growth policy: start at 8, then double. Doubling makes N appends amortize to
 * O(N) total copying (the geometric-series argument), so writeChunk /
 * writeValueArray are amortized O(1). */
#define GROW_CAPACITY(capacity) \
    ((capacity) < 8 ? 8 : (capacity) * 2)

/* Resize an array from oldCount to newCount elements, preserving contents. */
#define GROW_ARRAY(type, pointer, oldCount, newCount) \
    ((type *)reallocate(pointer, sizeof(type) * (size_t)(oldCount), \
                                 sizeof(type) * (size_t)(newCount)))

#define FREE_ARRAY(type, pointer, oldCount) \
    reallocate(pointer, sizeof(type) * (size_t)(oldCount), 0)

/* ---------------------------------------------------------------------------
 * reallocate — grow/shrink/allocate/free, all in one function.
 *   oldSize  newSize   effect
 *   0        n         allocate n bytes            (malloc)
 *   n        0         free                        (free)
 *   n        m>n       grow                        (realloc bigger)
 *   n        m<n       shrink                      (realloc smaller)
 * Returns the (possibly moved) block, or aborts the process on OOM — a teaching
 * VM treats out-of-memory as fatal rather than threading an error path through
 * every allocation site. The byte delta (newSize - oldSize) is added to the
 * VM's running total, and crossing the GC threshold triggers a collection.
 * --------------------------------------------------------------------------- */
void *reallocate(void *pointer, size_t oldSize, size_t newSize);

/* Mark a single object/value reachable — the roots-marking phase calls these.
 * markObject tolerates NULL and already-marked objects (the latter prevents
 * cycles from looping forever). */
void markObject(Obj *object);
void markValue(Value value);

/* Run one full mark-sweep cycle: mark roots, trace, sweep, then resize the
 * next-collection threshold based on how much survived. */
void collectGarbage(void);

/* Walk the intrusive object list and free every object — called once at VM
 * shutdown so a leak checker sees a clean heap. */
void freeObjects(void);

#endif /* CLOXI_MEMORY_H */
