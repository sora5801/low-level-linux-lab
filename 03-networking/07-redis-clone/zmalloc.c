/* ===========================================================================
 * zmalloc.c — the fail-fast allocator (see zmalloc.h for the rationale).
 * =========================================================================== */
#include "zmalloc.h"

#include <stdlib.h>   /* malloc, calloc, realloc, free, abort */
#include <stdio.h>    /* fprintf                              */
#include <string.h>   /* memcpy, strlen                       */

/* Central out-of-memory sink. We route every failure here so there is exactly
 * ONE place that decides what "no memory" means for this program. Marked with
 * the noreturn attribute so the optimizer knows control never comes back — that
 * lets callers omit a `return` after invoking it and keeps the hot path lean. */
static void zmalloc_oom(size_t size)
{
    /* write directly to stderr; we cannot rely on anything that might allocate. */
    fprintf(stderr, "zmalloc: out of memory trying to allocate %zu bytes\n", size);
    fflush(stderr);
    abort();   /* raise SIGABRT: produces a core dump for post-mortem debugging */
}

void *zmalloc(size_t size)
{
    /* malloc(0) is implementation-defined (may return NULL or a unique ptr).
     * We never intentionally ask for 0 bytes, so treat a NULL only as failure
     * when size>0; but to be safe we bump a 0 request to 1 byte so the returned
     * pointer is always freeable and distinct. */
    void *p = malloc(size ? size : 1);
    if (p == NULL) zmalloc_oom(size);
    return p;
}

void *zcalloc(size_t size)
{
    /* calloc zero-fills, which the kernel can satisfy cheaply for fresh pages
     * (they come from the page cache already zeroed). We use it for hash-table
     * bucket arrays so every bucket head starts as a NULL pointer. */
    void *p = calloc(1, size ? size : 1);
    if (p == NULL) zmalloc_oom(size);
    return p;
}

void *zrealloc(void *ptr, size_t size)
{
    /* realloc(NULL, n) == malloc(n), and realloc(p, 0) is again implementation
     * -defined; we never call the latter. On growth realloc may MOVE the block,
     * so every caller must use the returned pointer and drop the old one. */
    void *p = realloc(ptr, size ? size : 1);
    if (p == NULL) zmalloc_oom(size);
    return p;
}

void zfree(void *ptr)
{
    /* free(NULL) is defined as a no-op by the C standard, so callers can zfree
     * an already-cleared pointer without guarding it. */
    free(ptr);
}

char *zstrdup(const char *s)
{
    size_t len = strlen(s) + 1;      /* include the trailing NUL byte           */
    char  *p   = zmalloc(len);
    memcpy(p, s, len);               /* copy bytes AND the NUL in one shot      */
    return p;                        /* caller owns the copy -> must zfree it   */
}
