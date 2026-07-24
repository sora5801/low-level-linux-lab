/* ===========================================================================
 * sds.h — Simple Dynamic Strings: binary-safe, length-prefixed C strings.
 * ===========================================================================
 *
 * Redis keys and values are BINARY SAFE: a key may contain embedded NUL bytes,
 * newlines, anything. A plain `char *` cannot represent that (strlen stops at
 * the first 0). The classic Redis answer is `sds`, and we reimplement its core.
 *
 * THE MEMORY TRICK (this is the whole idea):
 * An sds is a `char *` that points at the CHARACTER DATA, but a fixed-size
 * header lives in the SAME allocation, immediately BEFORE that pointer:
 *
 *      +-----------+-----------+----------------------------+---+
 *      |  len      |  alloc    |  buf[0..len-1]  (the data) |\0 |
 *      +-----------+-----------+----------------------------+---+
 *      ^ struct sdshdr         ^
 *      |                       |
 *      malloc() returns here   the sds pointer WE hand out points here
 *
 * Because the returned pointer aims at `buf`, an sds can be passed to any
 * function expecting a `char *` (printf, write, memcmp) AND we still keep the
 * exact length one word behind it. We also keep a trailing '\0' so the data is
 * additionally usable as a C string when it happens to contain no NULs.
 *
 *   sdslen(s)  ==  ((struct sdshdr *)(s - sizeof(struct sdshdr)))->len
 *
 * Real Redis has FIVE header variants (sdshdr5/8/16/32/64) chosen by string
 * length to save header bytes on small strings, plus an embedded type byte.
 * We use ONE 16-byte header (two size_t fields) for clarity. That is the honest
 * simplification: same concept, no per-length header specialization.
 * =========================================================================== */
#ifndef SDS_H
#define SDS_H

#include <stddef.h>   /* size_t */

/* An sds IS a char* — that is the point. The typedef documents intent at call
 * sites: a parameter typed `sds` is a length-prefixed string, not a raw C str. */
typedef char *sds;

/* The header that sits just behind every sds pointer. `buf` is a C99 flexible
 * array member: it contributes 0 to sizeof(struct sdshdr) (== 16 on LP64) but
 * names the address where the character data begins. */
struct sdshdr {
    size_t len;    /* bytes currently used (the logical string length)          */
    size_t alloc;  /* bytes available in buf (NOT counting the trailing '\0')    */
    char   buf[];  /* the character data; the sds pointer == &buf[0]            */
};

/* Recover the header from an sds pointer by stepping back one header's width.
 * sizeof(struct sdshdr) is exactly the offset of buf here (no tail padding,
 * since both fields are size_t), so this lands precisely on the header. */
#define SDS_HDR(s) ((struct sdshdr *)((s) - sizeof(struct sdshdr)))

/* O(1) length: the reason sds exists. No scanning for a NUL. */
static inline size_t sdslen(const sds s) { return SDS_HDR(s)->len; }

/* Constructors (each returns a freshly-owned sds the caller must sdsfree):     */
sds sdsnewlen(const void *init, size_t initlen); /* copy initlen bytes (binary) */
sds sdsnew(const char *s);                       /* from a C string (uses strlen)*/
sds sdsempty(void);                              /* zero-length, ready to append */
sds sdsdup(const sds s);                         /* deep copy of an existing sds */
void sdsfree(sds s);                             /* free (sdsfree(NULL) is safe) */

/* Mutators. Like realloc, these may MOVE the allocation, so they RETURN the
 * (possibly new) sds and the caller must overwrite its variable with the value:
 *      s = sdscatlen(s, data, n);    // never ignore the return value           */
sds sdscatlen(sds s, const void *t, size_t len); /* append len raw bytes         */
sds sdscat(sds s, const char *t);                /* append a C string            */
sds sdscatprintf(sds s, const char *fmt, ...)    /* printf-style append          */
    __attribute__((format(printf, 2, 3)));       /*   let -Wformat check callers */

/* Compare two sds by content (binary, length-aware): <0, 0, >0 like memcmp. */
int sdscmp(const sds s1, const sds s2);

#endif /* SDS_H */
