/* ===========================================================================
 * util.c — implementations of die()/warn()/write_file().
 * ===========================================================================
 */
#define _GNU_SOURCE          /* O_CLOEXEC on some libcs comes from _GNU_SOURCE  */
#include "util.h"

#include <errno.h>           /* errno                                          */
#include <fcntl.h>           /* open, O_WRONLY, O_CLOEXEC                       */
#include <stdio.h>           /* perror                                         */
#include <stdlib.h>          /* exit                                           */
#include <string.h>          /* strlen                                         */
#include <unistd.h>          /* write, close, ssize_t                          */

void die(const char *msg)
{
    /* perror() formats "<msg>: <strerror(errno)>" to stderr for us. errno is
     * whatever the failed syscall left behind, which is exactly the detail we
     * want when debugging a container that refused to start. */
    perror(msg);
    exit(1);
}

void warn(const char *msg)
{
    perror(msg);
}

int write_file(const char *path, const char *data)
{
    /* open(2): mode O_WRONLY. These are existing kernel pseudo-files, so we do
     * not pass O_CREAT. O_CLOEXEC prevents the fd leaking across the execve we
     * will later do into the container payload. Returns a new fd or -1/errno. */
    int fd = open(path, O_WRONLY | O_CLOEXEC);
    if (fd == -1)
        return -1;                       /* caller inspects errno (EACCES, ...) */

    size_t remaining = strlen(data);
    const char *p = data;

    /* write(2): copy `remaining` bytes from p to the fd. It may legally write
     * FEWER bytes than asked (a "short write"), and it may be interrupted by a
     * signal before writing anything (EINTR). Robust code loops on both. For
     * these small proc/cgroup files a single write almost always suffices, but
     * we write the general, correct loop so the pattern is worth copying. */
    while (remaining > 0) {
        ssize_t n = write(fd, p, remaining);
        if (n == -1) {
            if (errno == EINTR)
                continue;                /* signal arrived; just retry          */
            close(fd);                   /* preserve the write errno for caller */
            return -1;
        }
        p         += n;
        remaining -= (size_t)n;
    }

    /* close(2) can itself fail (e.g. EIO flushed late). For a control file a
     * failing close means the value may not have taken, so we surface it. */
    if (close(fd) == -1)
        return -1;

    return 0;
}
