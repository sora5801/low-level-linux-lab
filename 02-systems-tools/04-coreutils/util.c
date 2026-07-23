/* ===========================================================================
 * util.c — diagnostics + the robust read/write loops every applet shares.
 * ===========================================================================
 *
 * These four functions are the "really" in "coreutils, but really." They are
 * short, but they encode the two facts that break naive implementations:
 *
 *   1. read(2)/write(2) may transfer FEWER bytes than requested (a "short"
 *      transfer) with NO error at all. You must loop.
 *   2. Either call may return -1 / EINTR because a signal was delivered while
 *      the process was blocked in the kernel. That is not a failure; you retry.
 *
 * Get these wrong and you get the classic bugs: truncated pipes, "cp" that
 * drops bytes when a window resize (SIGWINCH) lands mid-copy, "cat" that stops
 * early on a slow terminal.
 * =========================================================================== */
#include "coreutils.h"

#include <errno.h>    /* errno, EINTR                                          */
#include <stdarg.h>   /* va_list for the printf-style diagnostics              */
#include <stdio.h>    /* vfprintf, fprintf, fputs to stderr                    */
#include <stdlib.h>   /* exit                                                  */
#include <string.h>   /* strerror                                              */
#include <unistd.h>   /* read, write                                          */

/* Set by the multiplexer before any applet runs; see coreutils.h. */
const char *g_prog = "coreutils";

/* ---------------------------------------------------------------------------
 * Diagnostic helpers. All output goes to stderr (fd 2) so it never contaminates
 * a tool's real data stream on stdout — critical for cat/wc used in pipelines.
 * ------------------------------------------------------------------------- */

/* Shared body: print "g_prog: <formatted msg>", optionally "+: strerror(err)". */
static void vwarn_impl(int use_errno, int err, const char *fmt, va_list ap)
{
    fprintf(stderr, "%s: ", g_prog);
    vfprintf(stderr, fmt, ap);
    if (use_errno)
        fprintf(stderr, ": %s", strerror(err));
    fputc('\n', stderr);
}

void warn(const char *fmt, ...)
{
    int err = errno;                 /* snapshot NOW: vfprintf may clobber it   */
    va_list ap; va_start(ap, fmt);
    vwarn_impl(1, err, fmt, ap);
    va_end(ap);
}

void warnx(const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    vwarn_impl(0, 0, fmt, ap);
    va_end(ap);
}

void die(const char *fmt, ...)
{
    int err = errno;
    va_list ap; va_start(ap, fmt);
    vwarn_impl(1, err, fmt, ap);
    va_end(ap);
    exit(1);
}

void diex(const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    vwarn_impl(0, 0, fmt, ap);
    va_end(ap);
    exit(1);
}

/* ---------------------------------------------------------------------------
 * write_all — write(2) number 1. Kernel copies bytes from `buf` to `fd`.
 *   rax=1, rdi=fd, rsi=buf, rdx=count.  Returns bytes written (>=0) or -1.
 *
 * The kernel may return a value in [1, n): a "short write" (pipe buffer full,
 * disk quota edge, signal after partial progress). It is NOT an error, so we
 * advance our pointer and count and go again. EINTR (a signal arrived before
 * ANY byte moved) means "nothing happened, try again." We treat 0 defensively
 * as an error (a well-behaved blocking fd never returns 0 from write).
 * ------------------------------------------------------------------------- */
int write_all(int fd, const void *buf, size_t n)
{
    const char *p = (const char *)buf;   /* char* so pointer math is in bytes  */
    while (n > 0) {
        ssize_t w = write(fd, p, n);
        if (w < 0) {
            if (errno == EINTR)          /* interrupted before any progress    */
                continue;                /*   -> simply reissue the same write */
            return -1;                   /* EPIPE, ENOSPC, EIO, ... : real fail */
        }
        if (w == 0)                      /* shouldn't happen on a blocking fd;  */
            { errno = EIO; return -1; }  /*   guard against a 0-progress spin   */
        p += (size_t)w;                  /* advance past the bytes accepted     */
        n -= (size_t)w;                  /* and shrink the remaining count      */
    }
    return 0;
}

/* ---------------------------------------------------------------------------
 * read_retry — read(2) number 0. Kernel copies up to `n` bytes into `buf`.
 *   rax=0, rdi=fd, rsi=buf, rdx=count.  Returns bytes read, 0 at EOF, -1 err.
 *
 * We retry ONLY EINTR. We deliberately do NOT loop until the buffer is full:
 * a short read is normal (a pipe with 100 bytes ready, a terminal line) and
 * the caller must see EOF (0) to stop. Looping-to-full belongs in helpers that
 * read a fixed-size record, not in a streaming copy.
 * ------------------------------------------------------------------------- */
ssize_t read_retry(int fd, void *buf, size_t n)
{
    ssize_t r;
    do {
        r = read(fd, buf, n);
    } while (r < 0 && errno == EINTR);   /* signal mid-read: just try again     */
    return r;                            /* 0=EOF, >0=data, -1=real error       */
}

/* ---------------------------------------------------------------------------
 * copy_fd — the streaming copy loop shared by cat and cp's fallback path.
 *
 * Read a chunk, write ALL of it, repeat until EOF. Using a caller-supplied
 * buffer lets the caller size it to the source's st_blksize (one page-cache
 * unit per syscall = fewer syscalls). Returns 0 on clean EOF, -1 on the first
 * read or write error (errno preserved for the caller's diagnostic).
 * ------------------------------------------------------------------------- */
int copy_fd(int in, int out, char *buf, size_t bufsz)
{
    for (;;) {
        ssize_t r = read_retry(in, buf, bufsz);
        if (r < 0) return -1;            /* read error: errno already set       */
        if (r == 0) return 0;            /* EOF: the copy is complete           */
        if (write_all(out, buf, (size_t)r) < 0)
            return -1;                   /* write error (e.g. EPIPE downstream) */
    }
}
