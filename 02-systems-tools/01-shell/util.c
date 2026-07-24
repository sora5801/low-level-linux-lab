/* ===========================================================================
 * util.c — allocation wrappers and the growable string vector.
 * ===========================================================================
 *
 * A shell allocates constantly (every argv, every word, every job). To keep the
 * call sites readable we wrap malloc/realloc/strdup so they abort on failure
 * instead of forcing a NULL check after every call. That is a legitimate policy
 * for a short-lived interactive process: if the kernel cannot give us a few
 * bytes for the next command, there is nothing sensible to do but die loudly.
 * A long-running daemon would handle OOM gracefully instead — see the README.
 * ===========================================================================
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "shell.h"

/* xmalloc — malloc that never returns NULL. On failure we cannot continue, so
 * we print and _exit. Ownership: the caller owns the returned block. */
void *xmalloc(size_t n)
{
    void *p = malloc(n);
    if (!p) {
        /* write() would be more signal-safe, but we are not in a handler here. */
        fprintf(stderr, "shell: out of memory (malloc %zu)\n", n);
        exit(1);
    }
    return p;
}

/* xrealloc — realloc that never returns NULL. Note the classic realloc pitfall
 * we avoid: we assign into a temporary first, so a failed realloc does not leak
 * the original block by overwriting the only pointer to it with NULL. */
void *xrealloc(void *p, size_t n)
{
    void *q = realloc(p, n);
    if (!q) {
        fprintf(stderr, "shell: out of memory (realloc %zu)\n", n);
        exit(1);
    }
    return q;
}

/* xstrdup — duplicate a C string into a fresh owned allocation. */
char *xstrdup(const char *s)
{
    size_t n = strlen(s) + 1;          /* include the NUL terminator          */
    char  *d = xmalloc(n);
    memcpy(d, s, n);
    return d;
}

/* ------------------------------------------------------------- strvec ------ */
/* A minimal std::vector<char*>. It grows geometrically (x2) so appending N
 * items costs O(N) amortized, not O(N^2). It OWNS the char* pushed into it. */

void strvec_init(strvec *v)
{
    v->items = NULL;
    v->len   = 0;
    v->cap   = 0;
}

void strvec_push(strvec *v, char *s)
{
    if (v->len == v->cap) {
        /* Double the capacity (start at 8). Geometric growth keeps the total
         * number of reallocs logarithmic in the final length. */
        int newcap = v->cap ? v->cap * 2 : 8;
        v->items = xrealloc(v->items, (size_t)newcap * sizeof *v->items);
        v->cap   = newcap;
    }
    v->items[v->len++] = s;   /* ownership of `s` transfers to the vector      */
}

void strvec_free(strvec *v)
{
    for (int i = 0; i < v->len; i++)
        free(v->items[i]);     /* free each owned string ...                   */
    free(v->items);            /* ... then the backing array                   */
    v->items = NULL;
    v->len = v->cap = 0;
}
