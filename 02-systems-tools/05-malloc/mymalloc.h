/* ===========================================================================
 * mymalloc.h — public interface of a teaching malloc(3) replacement.
 * ===========================================================================
 *
 * This header declares BOTH the internal `my_*` API (handy for unit tests and
 * for the benchmark harness when it is statically linked against us) and the
 * standard `malloc`/`free`/... names, which mymalloc.c also exports so the whole
 * thing works under `LD_PRELOAD`.
 *
 * The allocator itself (design, on-disk block layout, size classes, the mmap
 * threshold, the per-thread cache) is documented at the top of mymalloc.c —
 * read that file for the "why". This header is deliberately thin.
 *
 * Platform: Linux / WSL only. We call brk/sbrk(2) and mmap/munmap(2), which are
 * Linux (POSIX-ish) syscalls; the code will not build on native Windows. The
 * teaching *assembly* (asm/) is host-portable because clang cross-targets Linux.
 * ===========================================================================
 */
#ifndef MYMALLOC_H
#define MYMALLOC_H

#include <stddef.h>   /* size_t */

#ifdef __cplusplus
extern "C" {
#endif

/* ---- The core allocator API ------------------------------------------------
 * These match the C standard-library contracts exactly, so a program cannot
 * tell whether it is talking to us or to glibc. That equivalence is the whole
 * point of an LD_PRELOAD-able allocator. */

void  *my_malloc(size_t nbytes);                 /* uninitialised block, 16-aligned */
void   my_free(void *ptr);                        /* release a my_malloc block       */
void  *my_calloc(size_t nmemb, size_t size);      /* zeroed, overflow-checked         */
void  *my_realloc(void *ptr, size_t nbytes);      /* grow/shrink, copy if it must move*/

/* Alignment helpers. Our natural alignment is 16 bytes, which already satisfies
 * _Alignof(max_align_t) on x86-64, so the common case is a straight malloc. */
int    my_posix_memalign(void **out, size_t alignment, size_t nbytes);
void  *my_aligned_alloc(size_t alignment, size_t nbytes);

/* Capacity actually available in the block backing `ptr` (>= what was asked
 * for, because of rounding). Mirrors glibc malloc_usable_size(3). */
size_t my_malloc_usable_size(void *ptr);

/* ---- Diagnostics -----------------------------------------------------------
 * my_heap_check() walks every block on the brk heap and validates the header
 * magic and the header<->footer boundary-tag agreement. It returns 0 when the
 * heap is consistent and non-zero on the first anomaly (having written a short
 * diagnostic to stderr with write(2), never printf, to avoid re-entering the
 * allocator). This is the "heap-corruption detection" deliverable made callable. */
int    my_heap_check(void);

struct my_stats {
    size_t bytes_from_os;   /* total bytes obtained from sbrk + mmap                 */
    size_t bytes_payload;   /* payload bytes currently marked allocated             */
    size_t blocks_live;     /* allocated blocks (brk + mmap), incl. tcache-parked    */
    size_t mmap_blocks;     /* live mmap-backed allocations                         */
    size_t free_blocks;     /* blocks currently sitting on the segregated free lists*/
};
void   my_heap_stats(struct my_stats *out);

#ifdef __cplusplus
}
#endif

#endif /* MYMALLOC_H */
