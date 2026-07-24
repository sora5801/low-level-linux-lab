/* ===========================================================================
 * sandbox.c — the launcher: install all three layers, then exec the target.
 * ===========================================================================
 *
 * Usage:
 *   ./sandbox [--trace] [--allow DIR]... -- PROGRAM [ARGS...]
 *
 *   --trace      run the ptrace supervisor so ACT_TRACE syscalls (openat) are
 *                inspected in userspace. Without it, ACT_TRACE degrades to ALLOW
 *                and Landlock alone guards the filesystem.
 *   --allow DIR  add DIR to the Landlock read/exec allowlist (repeatable).
 *                With no --allow, Landlock denies ALL filesystem access.
 *   --           end of options; everything after is the confined program.
 *
 * THE INSTALL ORDER (in the child, just before execve) is the crux:
 *
 *     1. PR_SET_NO_NEW_PRIVS   — makes unprivileged (2) and (3) legal, and stops
 *                                the target regaining privilege via setuid exec.
 *     2. Landlock ruleset      — filesystem allowlist (path-aware, race-free).
 *     3. seccomp filter        — syscall allowlist (fast, but pointer-blind).
 *     4. execve(target)        — both restrictions are INHERITED across exec by
 *                                design; the target can never shed them.
 *
 * (2) before (3) matters: installing Landlock itself needs syscalls
 * [landlock_create_ruleset/add_rule/restrict_self, open]. If seccomp went first
 * with those off the allowlist, we could not install Landlock. So we lock the
 * filesystem, THEN slam the syscall door behind us.
 *
 * For --trace, the child also does ptrace(PTRACE_TRACEME) FIRST so the parent
 * becomes its tracer; the parent then runs supervise().
 *
 * Legal note: run this only against programs you built, on a machine you own.
 * ===========================================================================
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>

#include "sandbox.h"

static void usage(const char *argv0)
{
    fprintf(stderr,
        "usage: %s [--trace] [--allow DIR]... -- PROGRAM [ARGS...]\n"
        "  --trace      inspect openat() paths via a ptrace supervisor\n"
        "  --allow DIR  permit read/exec beneath DIR (Landlock); repeatable\n"
        "  --           end options; the rest is the confined program\n",
        argv0);
}

int main(int argc, char **argv)
{
    int   use_trace = 0;
    const char *allow_paths[64];
    size_t n_allow = 0;
    int   i = 1;

    /* --- parse our own options up to "--" --------------------------------- */
    for (; i < argc; i++) {
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        else if (strcmp(argv[i], "--trace") == 0) {
            use_trace = 1;
        } else if (strcmp(argv[i], "--allow") == 0) {
            if (i + 1 >= argc) { usage(argv[0]); return 2; }
            if (n_allow >= sizeof(allow_paths) / sizeof(allow_paths[0])) {
                fprintf(stderr, "too many --allow paths\n"); return 2;
            }
            allow_paths[n_allow++] = argv[++i];
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage(argv[0]); return 0;
        } else {
            fprintf(stderr, "unknown option: %s\n", argv[i]);
            usage(argv[0]); return 2;
        }
    }
    if (i >= argc) { usage(argv[0]); return 2; }   /* no program to run        */
    char **child_argv = &argv[i];

    fprintf(stderr, "[sandbox] launching '%s' | trace=%s | landlock paths=%zu\n",
            child_argv[0], use_trace ? "on" : "off", n_allow);

    /* --- fork: parent supervises (or waits), child confines itself + execs - */
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }

    if (pid == 0) {
        /* =============== CHILD: build the jail around ourselves ============ */

        /* In trace mode, become traceable by our parent BEFORE installing the
         * filter, so the parent is already the tracer when the first
         * SECCOMP_RET_TRACE fires. */
        if (use_trace) {
            if (ptrace(PTRACE_TRACEME, 0, 0, 0) < 0) {
                perror("child: PTRACE_TRACEME");
                _exit(127);
            }
        }

        /* (1) No new privileges. Fail closed if this does not stick — every
         * later step depends on it. */
        if (enable_no_new_privs() < 0) {
            perror("child: PR_SET_NO_NEW_PRIVS");
            _exit(127);
        }

        /* (2) Landlock filesystem allowlist. If the kernel has no Landlock we
         * warn and continue (seccomp still applies); a stricter deployment
         * would treat this as fatal. This is a policy choice — we make it
         * visible rather than silent. */
        if (install_landlock(allow_paths, n_allow) < 0) {
            if (errno == ENOSYS || errno == EOPNOTSUPP)
                fprintf(stderr, "[sandbox] warning: kernel lacks Landlock; "
                                "filesystem NOT confined (seccomp still on)\n");
            else {
                perror("child: install_landlock");
                _exit(127);
            }
        }

        /* (3) seccomp syscall allowlist. This is the point of no return: after
         * it, only the policy's syscalls work; execve is on the allowlist so
         * the next step is legal. */
        if (install_seccomp_filter(use_trace) < 0) {
            perror("child: install_seccomp_filter");
            _exit(127);
        }

        /* (4) Hand control to the target. It inherits both restrictions. If
         * execve returns, it failed (e.g. target not found, or blocked). */
        execvp(child_argv[0], child_argv);
        perror("child: execvp");
        _exit(127);
    }

    /* =================== PARENT: supervise or just wait =================== */
    if (use_trace) {
        int code = supervise(pid);
        fprintf(stderr, "[sandbox] target exited with code %d\n", code);
        return code;
    } else {
        int status;
        if (waitpid(pid, &status, 0) < 0) { perror("waitpid"); return 1; }
        if (WIFEXITED(status)) {
            int code = WEXITSTATUS(status);
            fprintf(stderr, "[sandbox] target exited with code %d\n", code);
            return code;
        }
        if (WIFSIGNALED(status)) {
            int sig = WTERMSIG(status);
            /* SIGSYS (31) is what a seccomp KILL_PROCESS delivers. Naming it
             * makes the allowlist's default-deny visible in the demo output. */
            fprintf(stderr, "[sandbox] target killed by signal %d%s\n",
                    sig, sig == 31 ? " (SIGSYS: blocked by seccomp allowlist)" : "");
            return 128 + sig;
        }
        return 1;
    }
}
