/* ===========================================================================
 * supervisor.c — the ptrace interposer (layer 2).
 * ===========================================================================
 *
 * seccomp (layer 1) can say "openat is interesting" but not "openat of THIS
 * path is bad", because the filter only sees the pointer register, never the
 * bytes it points at. The SECCOMP_RET_TRACE action bridges that gap: it stops
 * the tracee at syscall entry and wakes a ptrace SUPERVISOR (this file, running
 * in the parent). The supervisor CAN read the tracee's memory, so it can pull
 * the path string out and apply a policy seccomp cannot express.
 *
 * The mechanism:
 *   - The child did ptrace(PTRACE_TRACEME) and installed a filter whose openat
 *     rule is SECCOMP_RET_TRACE | data. When the child calls openat, the kernel
 *     stops it and reports a PTRACE_EVENT_SECCOMP to us.
 *   - We wake up in waitpid with status encoding that event. We read the
 *     tracee's registers (orig_rax = nr, rdi/rsi/... = args), read the path out
 *     of the tracee's address space, decide, and either let the syscall run
 *     (PTRACE_CONT) or CANCEL it by rewriting the syscall number to -1 and
 *     setting a return of -EACCES.
 *
 * THE LESSON — TOCTOU (time-of-check to time-of-use):
 *   We read the path at the seccomp stop, BEFORE the syscall executes. Between
 *   our read (the "check") and the kernel copying the path in during the real
 *   syscall (the "use"), a SECOND THREAD in the tracee can overwrite the buffer.
 *   We validate "/home/me/ok.txt"; the racing thread flips it to "/etc/shadow";
 *   the kernel opens the latter. This is not a bug we can fully fix in a tracer:
 *   the check and the use are on different sides of a context switch and the
 *   memory is writable by the tracee the whole time. It is THE classic reason
 *   userspace-tracer sandboxes leak, and the reason path policy belongs in
 *   Landlock (layer 3), which checks inside the kernel at the moment of use,
 *   after the pointer has been resolved, with no window.
 *
 *   We even demonstrate a partial mitigation and its limits below.
 * ===========================================================================
 */
#define _GNU_SOURCE
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>        /* SIGTRAP, SIGSYS                                */
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>       /* struct user_regs_struct                        */
#include <sys/uio.h>        /* process_vm_readv, struct iovec                 */
#include <sys/syscall.h>

#include "sandbox.h"

/* PTRACE_EVENT_SECCOMP is reported in the high byte of the wait status:
 *   status >> 8 == (SIGTRAP | (PTRACE_EVENT_SECCOMP << 8))
 * These may be missing on very old headers, so provide the values. */
#ifndef PTRACE_EVENT_SECCOMP
#define PTRACE_EVENT_SECCOMP 7
#endif
#ifndef PTRACE_O_TRACESECCOMP
#define PTRACE_O_TRACESECCOMP 0x00000080
#endif
#ifndef PTRACE_O_EXITKILL
#define PTRACE_O_EXITKILL     0x00100000
#endif

/* An allowlist of directory prefixes the supervisor accepts for openat. Kept
 * separate from Landlock on purpose: this is the *racy* userspace check, so we
 * can contrast it with the kernel's race-free one. */
static const char *ALLOWED_PREFIXES[] = {
    "/usr/", "/lib/", "/etc/hostname", "/tmp/sandbox-ok/",
};
static const size_t ALLOWED_PREFIX_N =
    sizeof(ALLOWED_PREFIXES) / sizeof(ALLOWED_PREFIXES[0]);

/* Read up to `cap-1` bytes of a NUL-terminated string out of the tracee's
 * address space at address `remote`. Uses process_vm_readv, one syscall that
 * copies across address spaces (far cheaper than word-at-a-time PTRACE_PEEKDATA).
 *
 * TOCTOU NOTE: the bytes we get are a *snapshot*. Nothing stops the tracee (or
 * a sibling thread of it) from changing them the instant after we return. Do
 * not treat the result as the value the kernel will ultimately use. */
static ssize_t read_tracee_string(pid_t pid, unsigned long remote,
                                  char *buf, size_t cap)
{
    size_t got = 0;
    while (got < cap - 1) {
        /* Copy in chunks; stop when we find a NUL or hit a short read (e.g. an
         * unmapped page — the string may legitimately end at a page boundary). */
        size_t want = cap - 1 - got;
        struct iovec local  = { .iov_base = buf + got, .iov_len = want };
        struct iovec remio  = { .iov_base = (void *)(remote + got), .iov_len = want };
        ssize_t r = process_vm_readv(pid, &local, 1, &remio, 1, 0);
        if (r <= 0) {
            if (got == 0)
                return -1;          /* could not read even the first byte      */
            break;
        }
        for (ssize_t i = 0; i < r; i++) {
            if (buf[got + i] == '\0') { buf[got + i] = '\0'; return (ssize_t)(got + i); }
        }
        got += (size_t)r;
    }
    buf[got] = '\0';
    return (ssize_t)got;
}

/* Decide whether a resolved path is acceptable to the userspace policy. This is
 * a prefix match — deliberately simple, and deliberately UNSAFE against symlink
 * and "/../" tricks as well as the TOCTOU race, to make the point that a tracer
 * reimplementing path canonicalization is a losing game vs. Landlock. */
static int path_allowed(const char *path)
{
    /* Reject an obvious traversal attempt so the demo's message is honest, but
     * note this does NOT make prefix-matching safe (a racing thread bypasses
     * it, and there are canonicalization cases we do not cover). */
    if (strstr(path, "/../") || strncmp(path, "../", 3) == 0)
        return 0;
    for (size_t i = 0; i < ALLOWED_PREFIX_N; i++) {
        size_t plen = strlen(ALLOWED_PREFIXES[i]);
        if (strncmp(path, ALLOWED_PREFIXES[i], plen) == 0)
            return 1;
    }
    return 0;
}

/* Cancel the syscall the tracee is stopped at, making it return `ret` (a
 * negative errno) without ever executing. The trick: set orig_rax to -1, which
 * is not a valid syscall number, so the kernel skips the call, then set rax to
 * our desired return value. This is the standard "deny in a tracer" move. */
static void deny_syscall(pid_t pid, struct user_regs_struct *regs, long ret)
{
    regs->orig_rax = (unsigned long long)-1; /* invalidate the syscall number  */
    regs->rax      = (unsigned long long)ret;/* the value the tracee will see   */
    ptrace(PTRACE_SETREGS, pid, 0, regs);
}

/* supervise — the parent's trace loop. Returns the child's exit code, or
 * 128+signal if it died from a signal (shell convention). */
int supervise(pid_t child)
{
    int status;

    /* First stop: the child raised via TRACEME + execve. Wait for it. */
    if (waitpid(child, &status, 0) < 0) {
        perror("supervisor: initial waitpid");
        return 127;
    }

    /* Configure tracing:
     *  - TRACESECCOMP : deliver PTRACE_EVENT_SECCOMP on SECCOMP_RET_TRACE.
     *  - EXITKILL     : if the supervisor dies, the kernel kills the tracee too,
     *                   so a crashed jailer never leaves an unconfined child. */
    if (ptrace(PTRACE_SETOPTIONS, child, 0,
               (void *)(PTRACE_O_TRACESECCOMP | PTRACE_O_EXITKILL)) < 0) {
        perror("supervisor: PTRACE_SETOPTIONS");
        return 127;
    }

    /* Release the tracee; it runs until the next event of interest. */
    if (ptrace(PTRACE_CONT, child, 0, 0) < 0) {
        perror("supervisor: initial PTRACE_CONT");
        return 127;
    }

    for (;;) {
        if (waitpid(child, &status, 0) < 0) {
            if (errno == EINTR) continue;
            perror("supervisor: waitpid");
            return 127;
        }

        if (WIFEXITED(status)) {
            /* Normal termination: propagate the exit code. */
            return WEXITSTATUS(status);
        }
        if (WIFSIGNALED(status)) {
            /* Killed by a signal — e.g. SIGSYS from a seccomp KILL on a denied
             * syscall the tracer never even saw (KILL happens in-kernel). */
            int sig = WTERMSIG(status);
            fprintf(stderr, "[supervisor] target terminated by signal %d%s\n",
                    sig, sig == 31 ? " (SIGSYS: seccomp KILL)" : "");
            return 128 + sig;
        }
        if (!WIFSTOPPED(status)) {
            continue;                 /* PTRACE_EVENT_STOP etc.; nothing to do  */
        }

        /* Is this a seccomp event (our ACT_TRACE openat)? The event id lives in
         * the top byte of the status word. */
        int event = status >> 8;
        if (event == (SIGTRAP | (PTRACE_EVENT_SECCOMP << 8))) {
            struct user_regs_struct regs;
            if (ptrace(PTRACE_GETREGS, child, 0, &regs) < 0) {
                perror("supervisor: PTRACE_GETREGS");
                ptrace(PTRACE_CONT, child, 0, 0);
                continue;
            }

            /* At syscall ENTRY the kernel stashes the syscall number in
             * orig_rax (rax is reused for the return value), and the arguments
             * are in the syscall ABI registers: rdi, rsi, rdx, r10, r8, r9. */
            long nr = (long)regs.orig_rax;

            if (nr == SYS_openat) {
                /* openat(int dirfd, const char *pathname, int flags, ...)
                 *   dirfd  = rdi, pathname = rsi, flags = rdx.
                 * We inspect rsi, the path POINTER seccomp could not follow. */
                unsigned long path_ptr = regs.rsi;
                char path[512];
                ssize_t plen = read_tracee_string(child, path_ptr,
                                                  path, sizeof(path));
                if (plen < 0) {
                    /* Unreadable path (bad pointer): deny with EFAULT. */
                    fprintf(stderr, "[supervisor] openat: unreadable path ptr "
                                    "0x%lx -> deny EFAULT\n", path_ptr);
                    deny_syscall(child, &regs, -EFAULT);
                } else if (path_allowed(path)) {
                    fprintf(stderr, "[supervisor] openat(\"%s\") -> allow "
                                    "(but note: TOCTOU-racy, see README)\n", path);
                    /* Let the real syscall proceed unchanged. */
                } else {
                    fprintf(stderr, "[supervisor] openat(\"%s\") -> DENY EACCES\n",
                            path);
                    deny_syscall(child, &regs, -EACCES);
                }
            } else {
                /* Any other TRACE'd syscall would land here; log and allow. */
                fprintf(stderr, "[supervisor] trace event for %s -> allow\n",
                        syscall_name(nr));
            }

            /* Resume until the next event. */
            if (ptrace(PTRACE_CONT, child, 0, 0) < 0) {
                perror("supervisor: PTRACE_CONT (post-event)");
                return 127;
            }
            continue;
        }

        /* Some other stop (a real signal delivered to the tracee). Forward the
         * signal so the tracee sees it as it normally would, and continue. */
        int sig = WSTOPSIG(status);
        /* Do not re-inject SIGTRAP (our own control signal). */
        ptrace(PTRACE_CONT, child, 0, (void *)(long)(sig == SIGTRAP ? 0 : sig));
    }
}
