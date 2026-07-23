/* ===========================================================================
 * cat.c — concatenate files to stdout, correctly.
 * ===========================================================================
 *
 * `cat` looks trivial ("read, write, done"), which is exactly why it is a good
 * place to get the I/O contract right. The subtleties this file handles:
 *
 *   - Short reads/writes and EINTR — delegated to copy_fd()/write_all(), so a
 *     signal (SIGWINCH from a terminal resize, SIGCHLD, a profiler's SIGPROF)
 *     landing mid-copy never truncates the stream.
 *   - Buffer sizing from the SOURCE's st_blksize, so each read pulls one
 *     natural page-cache unit and we make the fewest syscalls.
 *   - "-" means stdin, and cat with no files reads stdin (POSIX behaviour).
 *   - A read error on one file is reported but we still process the rest, and
 *     the process exit status becomes non-zero (like GNU cat).
 *
 * Deliberately minimal on features (no -n/-A): the lesson here is the loop, not
 * option parsing. `wc`/`ls` below carry the option-parsing weight.
 * =========================================================================== */
#include "coreutils.h"

#include <errno.h>    /* errno                                                 */
#include <fcntl.h>    /* open, O_RDONLY                                        */
#include <stdlib.h>   /* malloc, free                                         */
#include <sys/stat.h> /* fstat, struct stat                                   */
#include <unistd.h>   /* read, write, close, STDIN/STDOUT_FILENO              */

/* A sane default when the source has no meaningful st_blksize (pipes, char
 * devices, or a filesystem reporting 0). 128 KiB amortises syscall overhead
 * without wasting much memory or blowing the CPU cache budget. */
#define CAT_BUFSZ (128 * 1024)

/* Copy one already-open fd to stdout, choosing a buffer sized to the source.
 * Returns 0 on success, -1 on error (errno set). */
static int cat_fd(int fd)
{
    /* Ask the source how big a "block" it likes. For a regular file this is the
     * filesystem's preferred I/O size (often 4 KiB or 1 MiB on network FS); for
     * a pipe fstat reports st_blksize too. We clamp into [BUFSZ floor, cap] so
     * a pathological value can't make us allocate gigabytes or thrash on tiny
     * reads. */
    struct stat st;
    size_t bufsz = CAT_BUFSZ;
    if (fstat(fd, &st) == 0 && st.st_blksize > 0) {
        size_t bs = (size_t)st.st_blksize;
        /* Round up to at least 64 KiB but never above 1 MiB. */
        if (bs < 64 * 1024)   bs = 64 * 1024;
        if (bs > 1024 * 1024) bs = 1024 * 1024;
        bufsz = bs;
    }

    /* Heap-allocate rather than a giant stack array: a 1 MiB VLA/array on the
     * stack risks blowing the default 8 MiB rlimit if cat is called re-entrantly
     * or from a small-stack thread. This block is owned here and freed below. */
    char *buf = malloc(bufsz);
    if (!buf) { errno = ENOMEM; return -1; }

    int rc = copy_fd(fd, STDOUT_FILENO, buf, bufsz);
    free(buf);                           /* return buffer to the allocator      */
    return rc;
}

int cat_main(int argc, char **argv)
{
    int status = 0;                      /* becomes 1 if any file fails          */

    /* No file operands: cat reads stdin. This is the `cat > file` / pipe case. */
    if (argc <= 1)
        return cat_fd(STDIN_FILENO) < 0 ? (warn("stdin"), 1) : 0;

    for (int i = 1; i < argc; i++) {
        int fd;
        if (argv[i][0] == '-' && argv[i][1] == '\0') {
            /* The conventional "-" operand = read standard input here. */
            fd = STDIN_FILENO;
        } else {
            /* open(2): rax=2, rdi=path, rsi=flags, rdx=mode(unused for RDONLY).
             * O_RDONLY only; the kernel returns the lowest free fd or -1 with
             * errno (ENOENT no such file, EACCES perms, EISDIR a directory). */
            fd = open(argv[i], O_RDONLY);
            if (fd < 0) { warn("%s", argv[i]); status = 1; continue; }
        }

        if (cat_fd(fd) < 0) { warn("%s", argv[i]); status = 1; }

        /* Only close what we opened; never close the shared STDIN_FILENO, or a
         * later "-" operand (and the shell) would see a closed fd 0. */
        if (fd != STDIN_FILENO)
            close(fd);
    }
    return status;
}
