/* ===========================================================================
 * jobserver.c — the POSIX jobserver: a pipe whose readable BYTES are "job
 * tokens", used to bound how many recipe processes run at once.
 * ===========================================================================
 *
 * THE PROBLEM. With `-jN` we may run up to N recipes in parallel. But in a
 * RECURSIVE build ($(MAKE) inside a recipe) each sub-make would separately try
 * to run N jobs, and N sub-makes would launch N*N processes — a fork bomb. The
 * budget has to be GLOBAL across every make in the tree.
 *
 * THE TRICK (invented by GNU make). Represent the budget as physical objects:
 * N-1 one-byte TOKENS living inside a pipe, plus ONE token every make holds
 * IMPLICITLY (it is always allowed to run a single job itself). To start an
 * additional parallel job a make must first OWN a token: it reads one byte out
 * of the pipe. When that job finishes it writes the byte back. The pipe's kernel
 * buffer is the shared counter; read/write are the atomic decrement/increment.
 * A sub-make inherits the pipe's read/write fds through the environment variable
 * MAKEFLAGS (`--jobserver-auth=R,W`), so every make in the tree draws from the
 * same N tokens. No shared memory, no locks — just a pipe and the kernel.
 *
 * WHY THE IMPLICIT TOKEN MATTERS. It guarantees forward progress and prevents
 * deadlock: a make can ALWAYS run at least one job without reading the pipe, so
 * it never blocks holding nothing while waiting for a token only it could
 * return. We therefore make pipe reads NON-BLOCKING: "no token right now" simply
 * means "don't start another job yet; go wait for a running one to finish."
 *
 * SCOPE NOTE. In this teaching build the top-level make is the sole reader of
 * its own pipe (recipe children are plain shell commands that never touch it),
 * so the non-blocking read has no competitor and no race. Recursive sub-makes
 * (which would race on the shared pipe and, in real make, use blocking reads +
 * poll with careful signal handling) are a documented stretch — see the README.
 * ===========================================================================
 */
#include "mk.h"

#include <unistd.h>    /* pipe2, read, write, close                          */
#include <fcntl.h>     /* fcntl, F_GETFD, F_GETFL, F_SETFL, O_NONBLOCK       */
#include <errno.h>     /* errno, EAGAIN, EWOULDBLOCK, EINTR                  */
#include <stdlib.h>    /* getenv, setenv, atoi                               */
#include <stdio.h>     /* snprintf, sscanf                                   */
#include <string.h>    /* strstr                                             */

/* The literal byte we push into the pipe as a token. Its VALUE is irrelevant —
 * only the COUNT of bytes matters — but a visible character makes a pipe dump
 * during debugging readable. */
#define TOKEN_BYTE '+'

/* Set O_NONBLOCK on a file descriptor, preserving its other status flags.
 * Non-blocking is what turns "acquire a token" into a poll-free try-acquire:
 * read() returns immediately with EAGAIN when the pipe is empty. */
static void set_nonblocking(int fd)
{
    int fl = fcntl(fd, F_GETFL, 0);
    if (fl < 0) die("fcntl F_GETFL");
    if (fcntl(fd, F_SETFL, fl | O_NONBLOCK) < 0)
        die("fcntl F_SETFL O_NONBLOCK");
}

/* Is `fd` a currently-open, valid descriptor? fcntl(F_GETFD) is the cheapest
 * probe: it returns the close-on-exec flag for a live fd, or -1/EBADF if the fd
 * is closed. We use it to validate the fds handed to us by a parent make before
 * trusting them. */
static int fd_is_open(int fd)
{
    return fd >= 0 && fcntl(fd, F_GETFD) != -1;
}

/* ---------------------------------------------------------------------------
 * Try to attach to a jobserver we INHERITED from a parent make. The parent
 * advertises its pipe in MAKEFLAGS as "--jobserver-auth=R,W" (modern) or the
 * older "--jobserver-fds=R,W". If we find a valid pair, adopt it and return 1.
 * --------------------------------------------------------------------------- */
static int attach_inherited(jobserver *js)
{
    const char *mf = getenv("MAKEFLAGS");
    if (!mf) return 0;

    const char *p = strstr(mf, "--jobserver-auth=");
    if (p) p += sizeof("--jobserver-auth=") - 1;
    if (!p) {
        p = strstr(mf, "--jobserver-fds=");
        if (p) p += sizeof("--jobserver-fds=") - 1;
    }
    if (!p) return 0;

    int r = -1, w = -1;
    if (sscanf(p, "%d,%d", &r, &w) != 2) return 0;   /* not the R,W fd form      */
    if (!fd_is_open(r) || !fd_is_open(w)) return 0;   /* stale fds: ignore        */

    js->rfd     = r;
    js->wfd     = w;
    js->own_pipe = 0;                  /* the parent owns/closes the pipe, not us */
    set_nonblocking(js->rfd);          /* see the scope note about the caveat     */
    return 1;
}

/* ===========================================================================
 * jobserver_init — attach to an inherited jobserver, else create our own.
 * ===========================================================================
 */
void jobserver_init(jobserver *js, int jobs)
{
    if (jobs < 1) jobs = 1;
    js->jobs         = jobs;
    js->implicit_free = 1;             /* our own always-available slot is free   */
    js->pipe_held    = 0;
    js->rfd = js->wfd = -1;
    js->own_pipe     = 0;

    if (attach_inherited(js))          /* recursive make: share the parent's pool */
        return;

    if (jobs <= 1)                     /* serial build: the implicit token is all */
        return;                        /*   we need; no pipe at all.              */

    /* Create our own token pipe. We deliberately do NOT set O_CLOEXEC: the fds
     * must survive execve so recursive sub-makes can inherit them. (Ordinary
     * recipe children inherit them too but never touch them.) */
    int fds[2];
    if (pipe2(fds, 0) < 0)
        die("pipe2 jobserver");
    js->rfd      = fds[0];
    js->wfd      = fds[1];
    js->own_pipe = 1;

    /* Prime the pipe with jobs-1 tokens; the implicit token is the Nth. Each is
     * a single byte, so the pipe's buffer now literally COUNTS the free slots. */
    for (int i = 0; i < jobs - 1; i++) {
        char t = TOKEN_BYTE;
        ssize_t k;
        do { k = write(js->wfd, &t, 1); } while (k < 0 && errno == EINTR);
        if (k != 1) die("write jobserver token");
    }

    /* Our reads are non-blocking so "no token" returns immediately (see header).*/
    set_nonblocking(js->rfd);
}

/* ===========================================================================
 * jobserver_acquire — grab a slot without blocking.
 * ===========================================================================
 * Order: prefer the free IMPLICIT slot (costs no syscall), then try to pull a
 * PIPE token. TOKEN_NONE means every slot is busy right now — the caller should
 * stop launching and go reap a running child (which will return its token).
 */
token_kind jobserver_acquire(jobserver *js)
{
    if (js->implicit_free) {
        js->implicit_free = 0;         /* take our own slot                       */
        return TOKEN_IMPLICIT;
    }

    if (js->rfd < 0)                   /* serial build: no pipe, no more slots     */
        return TOKEN_NONE;

    for (;;) {
        char c;
        ssize_t k = read(js->rfd, &c, 1);   /* non-blocking: 1, 0(EOF), or -1     */
        if (k == 1) {
            js->pipe_held++;           /* we now owe one byte back on release      */
            return TOKEN_PIPE;
        }
        if (k == 0)                    /* write end fully closed: pool is gone     */
            return TOKEN_NONE;
        if (errno == EINTR)            /* a signal interrupted the read: retry     */
            continue;
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return TOKEN_NONE;         /* pipe empty: no free token this instant   */
        die("read jobserver token");   /* any other errno is a real failure        */
    }
}

/* ===========================================================================
 * jobserver_release — return a slot when a job finishes.
 * ===========================================================================
 * The implicit slot is just a flag flip. A pipe token must be written BACK so
 * some make (possibly a sibling sub-make) can acquire it next. Losing a token
 * here permanently shrinks the pool, so we insist the write succeeds.
 */
void jobserver_release(jobserver *js, token_kind t)
{
    if (t == TOKEN_IMPLICIT) {
        js->implicit_free = 1;
        return;
    }
    if (t == TOKEN_PIPE) {
        char c = TOKEN_BYTE;
        ssize_t k;
        do { k = write(js->wfd, &c, 1); } while (k < 0 && errno == EINTR);
        if (k != 1) die("write-back jobserver token");   /* must not lose a token */
        js->pipe_held--;
    }
    /* TOKEN_NONE: nothing was held; nothing to release. */
}

/* ===========================================================================
 * jobserver_export — advertise the pipe to child sub-makes via MAKEFLAGS.
 * ===========================================================================
 * A recursive `$(MAKE)` recipe inherits our environment; seeing
 * "--jobserver-auth=R,W" it calls attach_inherited() and shares our token pool.
 * We only publish when we actually have a pipe to share.
 */
void jobserver_export(jobserver *js)
{
    if (js->rfd < 0 || js->wfd < 0)
        return;                        /* serial build: nothing to advertise      */

    char buf[128];
    snprintf(buf, sizeof buf, "--jobserver-auth=%d,%d -j%d",
             js->rfd, js->wfd, js->jobs);
    if (setenv("MAKEFLAGS", buf, 1) < 0)
        die("setenv MAKEFLAGS");
}

/* ===========================================================================
 * jobserver_destroy — release OS resources we own.
 * ===========================================================================
 * Only close the pipe if WE created it; an inherited pipe belongs to the parent
 * make, which will close it when the whole tree is done.
 */
void jobserver_destroy(jobserver *js)
{
    if (js->own_pipe) {
        if (js->rfd >= 0) close(js->rfd);
        if (js->wfd >= 0) close(js->wfd);
    }
    js->rfd = js->wfd = -1;
    js->own_pipe = 0;
}
