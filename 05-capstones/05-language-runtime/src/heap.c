/* ===========================================================================
 * heap.c — bump allocation from mmap'd arenas + segregated free-list reuse.
 * ===========================================================================
 *
 * Platform: Linux/x86-64 (uses mmap/munmap). The allocator is deliberately
 * simple — no boundary tags, no coalescing — because a GC-backed heap frees whole
 * objects, never fragments of them, so neighbours never need merging. The
 * interesting parts are (1) getting raw pages from the kernel and bumping through
 * them, and (2) recycling swept blocks by exact size class in O(1).
 */
#define _GNU_SOURCE            /* expose MAP_ANONYMOUS on glibc                  */
#include <string.h>
#include <unistd.h>            /* sysconf(_SC_PAGESIZE)                          */
#include <sys/mman.h>         /* mmap, munmap, PROT_*, MAP_*                    */

#include "heap.h"
#include "gc.h"               /* collectGarbage()                               */

#ifndef MAP_ANONYMOUS
#  ifdef MAP_ANON
#    define MAP_ANONYMOUS MAP_ANON
#  endif
#endif

/* The one global heap (declared extern in heap.h). */
Heap heap;

#define ARENA_BYTES (1u << 20)          /* 1 MiB of usable space per arena       */
#define GC_FIRST    (1u << 20)          /* first collection after ~1 MiB live    */

/* Round to the system page size (mmap grants whole pages anyway). */
static size_t pageRound(size_t n)
{
    long pg = sysconf(_SC_PAGESIZE);
    size_t p = (pg > 0) ? (size_t)pg : 4096u;
    return (n + p - 1) & ~(p - 1);
}

/* Reserve a new arena from the kernel and push it onto the arena chain, making
 * it the current (head) arena we bump from.
 *
 * mmap(2): addr=NULL (kernel picks), length=len, prot=PROT_READ|PROT_WRITE (we
 * write object bytes here; this heap is data, never executable — contrast the
 * JIT's PROT_EXEC page), flags=MAP_PRIVATE|MAP_ANONYMOUS (not file-backed; zero-
 * filled on first touch), fd=-1, offset=0. Returns MAP_FAILED on error. The
 * Arena bookkeeping struct itself is tiny and lives in the libc heap, so it never
 * competes with object storage. */
static Arena *mapArena(size_t need)
{
    size_t len = pageRound(need > ARENA_BYTES ? need : ARENA_BYTES);

    void *mem = mmap(NULL, len, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) {
        fprintf(stderr, "lumen: fatal: mmap of %zu bytes failed\n", len);
        exit(1);
    }
    Arena *a = (Arena *)malloc(sizeof(Arena));
    if (a == NULL) { fprintf(stderr, "lumen: fatal: OOM (arena)\n"); exit(1); }

    a->base = a->cursor = (char *)mem;
    a->end       = (char *)mem + len;
    a->mapLength = len;
    a->next      = heap.arenas;
    heap.arenas  = a;
    return a;
}

void heapInit(void)
{
    heap.arenas = NULL;
    memset(heap.freeLists, 0, sizeof(heap.freeLists));
    heap.bigFree        = NULL;
    heap.objects        = NULL;
    heap.bytesAllocated = 0;
    heap.nextGC         = GC_FIRST;
    heap.gcEnabled      = false;      /* stays off until the VM starts running   */
    mapArena(ARENA_BYTES);            /* one arena ready to go                    */
}

void heapShutdown(void)
{
    /* Unmap every arena. After this, all object/free-list pointers dangle, so we
     * also clear the heads — callers must have already run freeAllObjects() to
     * release the libc-owned bits (chunk arrays) hanging off functions. */
    for (Arena *a = heap.arenas; a != NULL; ) {
        Arena *next = a->next;
        munmap(a->base, a->mapLength);
        free(a);
        a = next;
    }
    heap.arenas  = NULL;
    heap.bigFree = NULL;
    heap.objects = NULL;
    memset(heap.freeLists, 0, sizeof(heap.freeLists));
}

size_t heapRoundUp(size_t n)
{
    /* Never smaller than a FreeBlock node (a swept block is reinterpreted as one)
     * and always a 16-byte multiple so every payload stays SysV-max-aligned. */
    if (n < sizeof(FreeBlock)) n = sizeof(FreeBlock);
    return (n + (HEAP_ALIGN - 1)) & ~((size_t)HEAP_ALIGN - 1);
}

/* Carve `rounded` bytes off the current arena, mapping a fresh one if the head
 * arena can't fit the request. (Leftover tail space in a retired arena is simply
 * abandoned — a deliberate simplicity/utilization trade, noted in the README.) */
static void *bumpAlloc(size_t rounded)
{
    Arena *a = heap.arenas;
    if (a == NULL || (size_t)(a->end - a->cursor) < rounded)
        a = mapArena(rounded);
    void *p = a->cursor;
    a->cursor += rounded;
    return p;
}

void *heapAlloc(size_t rounded)
{
    /* --- GC trigger, BEFORE we carve. ------------------------------------
     * Running the collector here (not after) means the not-yet-allocated block
     * can't be mistaken for garbage, and every *live* input to the current op is
     * already a root (on the value stack / in a frame). Under -DDEBUG_STRESS_GC
     * we collect on every single allocation — the strongest root-set test. */
#ifdef DEBUG_STRESS_GC
    if (heap.gcEnabled) collectGarbage();
#endif
    if (heap.gcEnabled && heap.bytesAllocated + rounded > heap.nextGC)
        collectGarbage();

    heap.bytesAllocated += rounded;

    /* --- 1. Try to reuse a swept block of exactly this size. -------------- */
    if (rounded <= HEAP_SMALL_MAX) {
        size_t cls = rounded / HEAP_ALIGN;          /* exact size class         */
        FreeBlock *fb = heap.freeLists[cls];
        if (fb != NULL) {
            heap.freeLists[cls] = fb->next;
            return fb;                              /* recycled, O(1)           */
        }
    } else {
        /* Large blocks share one list; reuse only on an exact size match so the
         * recorded object size stays truthful. */
        for (FreeBlock **pp = &heap.bigFree; *pp != NULL; pp = &(*pp)->next) {
            if ((*pp)->size == rounded) {
                FreeBlock *fb = *pp;
                *pp = fb->next;
                return fb;
            }
        }
    }

    /* --- 2. Nothing to recycle: bump. ------------------------------------ */
    return bumpAlloc(rounded);
}

void heapFree(void *block, size_t rounded)
{
    heap.bytesAllocated -= rounded;
    FreeBlock *fb = (FreeBlock *)block;             /* overlay a node on the block*/
    fb->size = rounded;
    if (rounded <= HEAP_SMALL_MAX) {
        size_t cls = rounded / HEAP_ALIGN;
        fb->next = heap.freeLists[cls];
        heap.freeLists[cls] = fb;
    } else {
        fb->next = heap.bigFree;
        heap.bigFree = fb;
    }
}
