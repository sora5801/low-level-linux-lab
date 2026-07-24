/* ===========================================================================
 * util.c — die()/warn()/write_file()/run_cmd().
 * ===========================================================================
 */
#define _GNU_SOURCE          /* O_CLOEXEC on some libcs comes from _GNU_SOURCE  */
#include "util.h"

#include <errno.h>           /* errno                                          */
#include <fcntl.h>           /* open, O_WRONLY, O_CLOEXEC                       */
#include <stdio.h>           /* perror, fprintf                                */
#include <stdlib.h>          /* exit                                           */
#include <string.h>          /* strlen                                         */
#include <sys/wait.h>        /* waitpid, W* status macros                      */
#include <unistd.h>          /* write, close, fork, execvp, ssize_t            */

void die(const char *msg)
{
    /* perror() formats "<msg>: <strerror(errno)>" to stderr. errno is whatever
     * the failed syscall left behind — exactly the detail we want when a
     * container refuses to start. */
    perror(msg);
    exit(1);
}

void warn(const char *msg)
{
    perror(msg);
}

int write_file(const char *path, const char *data)
{
    /* open(2), mode O_WRONLY: these are existing kernel pseudo-files, so no
     * O_CREAT. O_CLOEXEC keeps the fd from leaking across the container's
     * execve. Returns a new fd or -1/errno. */
    int fd = open(path, O_WRONLY | O_CLOEXEC);
    if (fd == -1)
        return -1;                       /* caller inspects errno (EACCES, ...) */

    size_t remaining = strlen(data);
    const char *p = data;

    /* write(2) may legally write FEWER bytes than asked (a "short write") and
     * may be interrupted before writing anything (EINTR). For these tiny
     * proc/cgroup files a single write almost always suffices, but we write the
     * general, correct loop so the pattern is worth copying elsewhere. */
    while (remaining > 0) {
        ssize_t n = write(fd, p, remaining);
        if (n == -1) {
            if (errno == EINTR)
                continue;                /* signal arrived mid-call; just retry */
            close(fd);                   /* preserve the write errno for caller */
            return -1;
        }
        p         += n;
        remaining -= (size_t)n;
    }

    /* close(2) can itself fail (a late-flushed EIO). For a control file a
     * failing close means the value may not have taken, so we surface it. */
    if (close(fd) == -1)
        return -1;

    return 0;
}

int run_cmd(char *const argv[], int quiet)
{
    /* fork(2): duplicate this process. The child returns 0, the parent gets the
     * child pid. We use the classic fork+exec+wait rather than system(3) so no
     * shell is involved — no word-splitting, no $PATH surprises on the argv, and
     * we control fd inheritance. */
    pid_t pid = fork();
    if (pid == -1)
        return -1;

    if (pid == 0) {
        /* --- child --- */
        if (quiet) {
            /* Send stderr to /dev/null for the "-C" existence probes, whose
             * failure is expected and not interesting. We reopen fd 2 onto
             * /dev/null; if that fails we carry on (worst case: noisy). */
            int nul = open("/dev/null", O_WRONLY | O_CLOEXEC);
            if (nul != -1) {
                dup2(nul, 2);            /* fd 2 (stderr) now points at the sink */
                close(nul);
            }
        }
        /* execvp(3): search $PATH for argv[0] (so "ip"/"iptables" resolve) and
         * replace this child's image. On success it never returns; on failure we
         * exit 127, the conventional "command not found" status. */
        execvp(argv[0], argv);
        _exit(127);
    }

    /* --- parent: reap the child and translate its status --- */
    int status;
    while (waitpid(pid, &status, 0) == -1) {
        if (errno == EINTR)
            continue;                    /* interrupted wait: retry             */
        return -1;
    }
    if (WIFEXITED(status))
        return WEXITSTATUS(status);      /* the command's own exit code         */
    return -1;                           /* killed by a signal => treat as error */
}
