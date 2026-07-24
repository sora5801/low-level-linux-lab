/* ===========================================================================
 * util.c — the boring-but-load-bearing primitives: checked allocation, a
 * growable string buffer, and reading a whole file into memory.
 * ===========================================================================
 *
 * WHY a die-on-failure allocator? The rest of the code is about algorithms and
 * syscalls, and null-checking every malloc would bury that logic in noise. By
 * funneling allocation through wrappers that abort on OOM, callers get memory
 * that is *always* valid — without ever ignoring a failure. On a build tool,
 * "out of memory" is genuinely unrecoverable, so aborting is the honest choice.
 * ===========================================================================
 */
#include "mk.h"

#include <stdio.h>     /* fprintf, vfprintf, stderr                          */
#include <stdlib.h>    /* malloc, calloc, realloc, exit                      */
#include <string.h>    /* memcpy, strlen                                     */
#include <stdarg.h>    /* va_list for die()                                  */
#include <errno.h>     /* errno for die()                                    */
#include <unistd.h>    /* read, close                                        */
#include <fcntl.h>     /* open, O_RDONLY, O_CLOEXEC                           */
#include <sys/stat.h>  /* fstat, struct stat                                 */

/* ---------------------------------------------------------------------------
 * die — print a message (appending strerror(errno) when errno is set) and exit.
 *
 * We use exit code 2, which is make's convention for "make itself failed"
 * (as opposed to 1, "a recipe failed"). Keeping errno's text is what turns an
 * opaque abort into a debuggable one: "read_file: open kitchen: No such file".
 * --------------------------------------------------------------------------- */
void die(const char *fmt, ...)
{
    int saved = errno;                 /* capture before anything can clobber it */
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "mmake: ");
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    if (saved)                         /* only append if a syscall set errno    */
        fprintf(stderr, ": %s", strerror(saved));
    fputc('\n', stderr);
    exit(2);
}

/* Allocation wrappers. Each aborts on failure so callers can assume success.
 * Note the multiply-overflow guard in xcalloc: calloc already checks n*sz for
 * overflow internally, which is one reason we prefer it for arrays. */
void *xmalloc(size_t n)
{
    void *p = malloc(n ? n : 1);       /* malloc(0) is allowed to return NULL;  */
    if (!p) die("out of memory (malloc %zu)", n);  /* force a non-NULL result   */
    return p;
}

void *xcalloc(size_t n, size_t sz)
{
    void *p = calloc(n ? n : 1, sz ? sz : 1);
    if (!p) die("out of memory (calloc %zu*%zu)", n, sz);
    return p;
}

void *xrealloc(void *p, size_t n)
{
    void *q = realloc(p, n ? n : 1);
    if (!q) die("out of memory (realloc %zu)", n);
    return q;
}

char *xstrdup(const char *s)
{
    size_t n = strlen(s) + 1;          /* +1 for the NUL terminator             */
    char  *d = xmalloc(n);
    memcpy(d, s, n);
    return d;
}

/* Bounded strdup: copy at most n bytes, always NUL-terminate. Used constantly
 * by the parser to lift a slice of the makefile text into its own string. */
char *xstrndup(const char *s, size_t n)
{
    char *d = xmalloc(n + 1);
    memcpy(d, s, n);
    d[n] = '\0';
    return d;
}

/* ===========================================================================
 * strbuf — a growable, always-NUL-terminated byte buffer.
 * ===========================================================================
 * Growth is geometric (double, min 16), giving amortized O(1) appends: N single
 * appends cost O(N) copies total, not O(N^2). The buffer keeps a trailing NUL
 * at all times so `sb->data` is a valid C string mid-construction.
 */
void sb_init(strbuf *sb)
{
    sb->cap  = 16;
    sb->data = xmalloc(sb->cap);
    sb->data[0] = '\0';
    sb->len  = 0;
}

/* Ensure room for `extra` more bytes plus the NUL, growing geometrically. */
static void sb_reserve(strbuf *sb, size_t extra)
{
    size_t need = sb->len + extra + 1; /* +1 keeps space for the terminator     */
    if (need <= sb->cap) return;
    while (sb->cap < need) sb->cap *= 2;
    sb->data = xrealloc(sb->data, sb->cap);
}

void sb_addch(strbuf *sb, char c)
{
    sb_reserve(sb, 1);
    sb->data[sb->len++] = c;
    sb->data[sb->len]   = '\0';        /* re-terminate                          */
}

void sb_addbytes(strbuf *sb, const char *p, size_t n)
{
    sb_reserve(sb, n);
    memcpy(sb->data + sb->len, p, n);
    sb->len += n;
    sb->data[sb->len] = '\0';
}

void sb_addstr(strbuf *sb, const char *s)
{
    sb_addbytes(sb, s, strlen(s));
}

/* Hand the underlying buffer to the caller and reset the strbuf to empty. The
 * caller now OWNS the returned pointer and must free() it. */
char *sb_detach(strbuf *sb)
{
    char *d = sb->data;
    sb->data = NULL;
    sb->len = sb->cap = 0;
    return d;
}

void sb_free(strbuf *sb)
{
    free(sb->data);
    sb->data = NULL;
    sb->len = sb->cap = 0;
}

/* ===========================================================================
 * read_file — slurp a file into a fresh NUL-terminated buffer.
 * ===========================================================================
 *
 * Syscalls used:
 *   open(2)  : number 2. open(path, O_RDONLY|O_CLOEXEC). O_CLOEXEC sets the
 *              close-on-exec flag so this fd does NOT leak into the recipe
 *              children we later fork+exec — a classic fd-hygiene bug otherwise.
 *   fstat(2) : number 5. Gives st_size so we can size the buffer in one alloc
 *              for a regular file. (For pipes/char devices st_size is 0, so we
 *              also grow-on-demand rather than trusting it blindly.)
 *   read(2)  : number 0. Copies bytes into our buffer. read can return FEWER
 *              bytes than requested (a "short read") and can fail with EINTR if
 *              a signal interrupts it — BOTH are normal, not errors, so we loop.
 *   close(2) : number 3. Release the fd.
 * --------------------------------------------------------------------------- */
char *read_file(const char *path, size_t *out_len)
{
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0)
        die("cannot open %s", path);   /* errno set by open -> die appends it   */

    struct stat st;
    size_t cap = 0;
    if (fstat(fd, &st) == 0 && st.st_size > 0)
        cap = (size_t)st.st_size;      /* good initial guess for a regular file */
    if (cap == 0) cap = 4096;          /* pipes report size 0: start at a page  */

    char  *buf = xmalloc(cap + 1);     /* +1 reserves the terminating NUL       */
    size_t len = 0;

    for (;;) {
        if (len == cap) {              /* buffer full: double and keep reading  */
            cap *= 2;
            buf  = xrealloc(buf, cap + 1);
        }
        ssize_t r = read(fd, buf + len, cap - len);
        if (r < 0) {
            if (errno == EINTR) continue;  /* interrupted by a signal: retry    */
            die("read %s", path);
        }
        if (r == 0) break;             /* r == 0 is genuine end-of-file         */
        len += (size_t)r;              /* advance; loop handles the next chunk  */
    }

    if (close(fd) < 0)                 /* close can fail (e.g. EIO); report it   */
        die("close %s", path);

    buf[len] = '\0';                   /* make the bytes usable as a C string   */
    if (out_len) *out_len = len;
    return buf;                        /* caller owns and must free()           */
}
