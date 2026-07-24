/* ===========================================================================
 * zmalloc.h — checked allocation wrappers (named after Redis's own zmalloc).
 * ===========================================================================
 *
 * WHY A WRAPPER AT ALL
 * --------------------
 * Every other module (sds, dict, objects) allocates constantly. If each call
 * site had to test malloc() for NULL and unwind, the code would drown in error
 * handling that never fires in practice (Linux over-commits; malloc for a few
 * bytes effectively never returns NULL until the machine is already dying).
 *
 * So we centralize the policy in ONE place: on allocation failure we print a
 * diagnostic and abort(). That is a deliberate teaching simplification. The
 * REAL Redis zmalloc additionally (a) keeps a running total of bytes handed out
 * (used_memory, for INFO/maxmemory), stashing each block's size in a header word
 * just before the returned pointer, and (b) calls a *configurable* out-of-memory
 * handler instead of abort(). We keep the fail-fast contract but drop the
 * accounting so the reader can see the data structures, not the bookkeeping.
 *
 * OWNERSHIP CONTRACT (applies everywhere in this project):
 *   - Whoever calls z*alloc owns the block and must eventually zfree() it.
 *   - Functions that "take ownership" of a pointer say so in their comment and
 *     become responsible for freeing it (e.g. dictAdd takes ownership of key).
 * =========================================================================== */
#ifndef ZMALLOC_H
#define ZMALLOC_H

#include <stddef.h>   /* size_t */

/* All four never return NULL: on failure they abort the process. That is why
 * callers throughout this codebase do NOT null-check them — the contract is
 * "you get memory or the program dies," which keeps the logic readable. */
void *zmalloc(size_t size);            /* like malloc; contents uninitialized   */
void *zcalloc(size_t size);            /* like calloc(1,size); zero-initialized  */
void *zrealloc(void *ptr, size_t size);/* like realloc; grows/moves a block      */
void  zfree(void *ptr);                /* like free; zfree(NULL) is a safe no-op  */
char *zstrdup(const char *s);          /* duplicate a C string (owned copy)       */

#endif /* ZMALLOC_H */
