/* ===========================================================================
 * coreutils.h — shared declarations for the busybox-style multiplexer.
 * ===========================================================================
 *
 * This project ships ONE binary that behaves as six different tools (cat, cp,
 * ls, wc, xargs, tailf). That is the "busybox" model: a single `.text` segment
 * with an argv[0]-based (or first-argument-based) dispatch table. Each applet is
 * a plain `int <tool>_main(int argc, char **argv)` — exactly the shape of a real
 * `main`, so the code reads like six independent programs that happen to link
 * together.
 *
 * Everything here is Linux-specific by design (getdents64, inotify, copy_file_
 * range, SEEK_HOLE). See the README for why and how to build under Linux/WSL.
 * =========================================================================== */
#ifndef COREUTILS_H
#define COREUTILS_H

#include <stddef.h>   /* size_t                                                */
#include <sys/types.h>/* ssize_t, off_t                                        */

/* ---------------------------------------------------------------------------
 * Program identity for diagnostics.
 *
 * `g_prog` is set once by the multiplexer to the applet name ("cat", "cp", ...)
 * so every error line reads like the real tools:  "cp: cannot stat 'x': ...".
 * It is process-global and write-once (set before any applet runs), so there is
 * no data race: we are single-threaded until an applet forks, and a forked
 * child inherits the value by copy.
 * ------------------------------------------------------------------------- */
extern const char *g_prog;

/* Diagnostics. `warn`/`die` append ": <strerror(errno)>" like err(3); the *x
 * variants do not touch errno. `die`/`diex` never return (they exit(1)). We
 * roll our own instead of <err.h> so the message prefix is always `g_prog`. */
void warn(const char *fmt, ...)  __attribute__((format(printf, 1, 2)));
void warnx(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void die(const char *fmt, ...)
        __attribute__((format(printf, 1, 2), noreturn));
void diex(const char *fmt, ...)
        __attribute__((format(printf, 1, 2), noreturn));

/* ---------------------------------------------------------------------------
 * Robust I/O primitives — the whole point of "coreutils, but really".
 *
 * The kernel is permitted to move FEWER bytes than you asked for on every
 * read(2)/write(2): a signal can interrupt the call (EINTR), a pipe can fill,
 * a socket can drain slowly. Naive tools that assume `write(fd,buf,n)==n`
 * silently corrupt data. These helpers encode the correct loops once.
 * ------------------------------------------------------------------------- */

/* write_all: write EXACTLY n bytes (or fail). Retries on EINTR, advances past
 * short writes. Returns 0 on success, -1 with errno set on real error. */
int write_all(int fd, const void *buf, size_t n);

/* read_retry: a single read(2) that only retries the EINTR case. Returns the
 * byte count (0 = EOF, >0 = partial or full), or -1 with errno on error. The
 * caller loops on this; we do NOT loop-to-full here because EOF must surface. */
ssize_t read_retry(int fd, void *buf, size_t n);

/* copy_fd: stream all bytes from `in` to `out` using read_retry + write_all,
 * with a caller-supplied scratch buffer. Returns 0 on clean EOF, -1 on error.
 * This is cat's core and cp's read/write fallback. */
int copy_fd(int in, int out, char *buf, size_t bufsz);

/* ---------------------------------------------------------------------------
 * The applets. Each is a self-contained tool `main`.
 * ------------------------------------------------------------------------- */
int cat_main(int argc, char **argv);
int cp_main(int argc, char **argv);
int ls_main(int argc, char **argv);
int wc_main(int argc, char **argv);
int xargs_main(int argc, char **argv);
int tailf_main(int argc, char **argv);

#endif /* COREUTILS_H */
