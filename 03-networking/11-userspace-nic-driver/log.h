/* ===========================================================================
 * log.h — tiny logging + error-checking helpers.
 * ===========================================================================
 *
 * A userspace driver has almost no safety net: if we mis-program a register or
 * hand the NIC a bad physical address, the failure mode is a silent hang, a
 * DMA into the wrong page, or an MCE — not a friendly errno. So we are pedantic
 * about checking EVERY syscall and library-call return value and aborting loud
 * and early. These macros make that cheap enough that there is no excuse to
 * skip a check.
 * ========================================================================= */
#ifndef IXY_LOG_H
#define IXY_LOG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

/* Emit a timestamped line to stderr with file:line:function context. The
 * do/while(0) wrapper makes the macro a single statement so it composes with
 * `if (x) LOG(...); else ...` without dangling-else surprises. */
#define log_line(level, fmt, ...)                                             \
    do {                                                                      \
        fprintf(stderr, "[%s] %s:%d %s(): " fmt "\n", level,                  \
                __FILE__, __LINE__, __func__, ##__VA_ARGS__);                 \
    } while (0)

#define info(fmt, ...)  log_line("INFO",  fmt, ##__VA_ARGS__)
#define warn(fmt, ...)  log_line("WARN",  fmt, ##__VA_ARGS__)
#define debug(fmt, ...) log_line("DEBUG", fmt, ##__VA_ARGS__)

/* Fatal error: log and abort. There is no sensible way to "recover" from a
 * failed BAR mmap or a failed hugepage allocation in a teaching driver — the
 * device is half-configured and continuing would only corrupt state. We append
 * strerror(errno) because most of our failures are syscall failures. */
#define error(fmt, ...)                                                       \
    do {                                                                      \
        log_line("ERROR", fmt " (errno=%d: %s)", ##__VA_ARGS__,              \
                 errno, strerror(errno));                                     \
        abort();                                                              \
    } while (0)

/* check_err(expr, msg): the workhorse. Evaluate `expr` exactly once; if it
 * returns a negative/failure sentinel, abort with context. Used to wrap
 * open()/mmap()/read() etc. so the happy path stays a single readable line:
 *
 *     int fd = check_err(open(path, O_RDWR), "opening resource");
 *
 * We deliberately return the value on success so the macro is an expression. */
static inline long check_err_impl(long value, const char *op,
                                  const char *file, int line)
{
    /* mmap failure sentinel is (void*)-1 which is -1 as a long; open/read/etc.
     * use -1; so a single "< 0" test covers all of them. */
    if (value == -1) {
        fprintf(stderr, "[ERROR] %s:%d: %s failed (errno=%d: %s)\n",
                file, line, op, errno, strerror(errno));
        abort();
    }
    return value;
}
#define check_err(expr, op) check_err_impl((long)(expr), (op), __FILE__, __LINE__)

#endif /* IXY_LOG_H */
