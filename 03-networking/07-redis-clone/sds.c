/* ===========================================================================
 * sds.c — Simple Dynamic Strings implementation (see sds.h for the layout).
 * =========================================================================== */
#include "sds.h"
#include "zmalloc.h"

#include <string.h>   /* memcpy, strlen, memcmp */
#include <stdio.h>    /* vsnprintf              */
#include <stdarg.h>   /* va_list                */

/* Allocate a header + `initlen` data bytes + 1 trailing NUL, and return a
 * pointer to the DATA (not the header). If `init` is non-NULL its bytes are
 * copied in; either way a '\0' is written just past the data so the result is
 * also a valid C string when it contains no embedded NULs. */
sds sdsnewlen(const void *init, size_t initlen)
{
    /* One allocation holds header + data + NUL. `hdr->buf` is the data start. */
    struct sdshdr *hdr = zmalloc(sizeof(struct sdshdr) + initlen + 1);
    hdr->len   = initlen;
    hdr->alloc = initlen;
    if (init && initlen)
        memcpy(hdr->buf, init, initlen);   /* binary-safe copy (may contain NUL) */
    hdr->buf[initlen] = '\0';              /* sentinel for C-string interop      */
    return hdr->buf;                       /* the sds pointer aims at the data   */
}

sds sdsnew(const char *s)
{
    /* strlen is fine here: callers pass real C strings (command names, etc.). */
    return sdsnewlen(s, s ? strlen(s) : 0);
}

sds sdsempty(void)
{
    return sdsnewlen("", 0);
}

sds sdsdup(const sds s)
{
    return sdsnewlen(s, sdslen(s));        /* copy exactly the logical length    */
}

void sdsfree(sds s)
{
    if (s == NULL) return;                 /* mirror free(NULL): a safe no-op    */
    zfree(SDS_HDR(s));                     /* free from the HEADER, not the data */
}

/* Ensure at least `addlen` more bytes fit after the current length, growing the
 * allocation if needed. Returns the (possibly moved) sds. Growth uses a simple
 * doubling policy for small strings and exact fit for large ones — Redis uses a
 * 1 MB threshold; we keep the idea, smaller code. */
static sds sdsMakeRoomFor(sds s, size_t addlen)
{
    struct sdshdr *hdr = SDS_HDR(s);
    size_t len   = hdr->len;
    size_t avail = hdr->alloc - len;       /* free bytes already in the buffer   */
    if (avail >= addlen) return s;         /* fast path: it already fits         */

    size_t newlen = len + addlen;
    if (newlen < 1024 * 1024) newlen *= 2; /* small: over-allocate to amortize   */
    else                      newlen += 1024 * 1024; /* large: grow by 1 MB      */

    /* realloc from the header; on move, `hdr` and the returned data pointer both
     * change, so we recompute buf. */
    hdr = zrealloc(hdr, sizeof(struct sdshdr) + newlen + 1);
    hdr->alloc = newlen;
    return hdr->buf;
}

sds sdscatlen(sds s, const void *t, size_t len)
{
    size_t curlen = sdslen(s);
    s = sdsMakeRoomFor(s, len);            /* may reallocate/move                */
    memcpy(s + curlen, t, len);            /* append after the current data      */
    struct sdshdr *hdr = SDS_HDR(s);
    hdr->len = curlen + len;               /* update the logical length          */
    s[hdr->len] = '\0';                    /* re-terminate                       */
    return s;
}

sds sdscat(sds s, const char *t)
{
    return sdscatlen(s, t, strlen(t));
}

sds sdscatprintf(sds s, const char *fmt, ...)
{
    /* Format into a stack buffer first (fast common case). We size it generously
     * and fall back to a heap buffer if the formatted output would overflow. */
    char staticbuf[1024];
    char *buf = staticbuf;
    size_t buflen = sizeof(staticbuf);

    for (;;) {
        va_list ap;
        va_start(ap, fmt);
        int n = vsnprintf(buf, buflen, fmt, ap);  /* returns needed length       */
        va_end(ap);
        if (n < 0) { if (buf != staticbuf) zfree(buf); return s; } /* enc error   */
        if ((size_t)n < buflen) {                 /* it fit: append and return    */
            s = sdscatlen(s, buf, (size_t)n);
            if (buf != staticbuf) zfree(buf);
            return s;
        }
        /* Did not fit: allocate exactly enough (+1 for NUL) and retry once. */
        buflen = (size_t)n + 1;
        if (buf != staticbuf) zfree(buf);
        buf = zmalloc(buflen);
    }
}

int sdscmp(const sds s1, const sds s2)
{
    size_t l1 = sdslen(s1), l2 = sdslen(s2);
    size_t minlen = l1 < l2 ? l1 : l2;
    int cmp = memcmp(s1, s2, minlen);      /* compare the shared prefix (binary)  */
    if (cmp != 0) return cmp;
    /* Prefixes equal: the shorter string sorts first. Return the length delta. */
    if (l1 == l2) return 0;
    return l1 < l2 ? -1 : 1;
}
