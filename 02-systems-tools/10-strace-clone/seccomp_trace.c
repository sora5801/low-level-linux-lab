/* ===========================================================================
 * seccomp_trace.c — the SAME trace, driven by seccomp-bpf instead of
 *                   PTRACE_SYSCALL. The interesting half of this project.
 * ===========================================================================
 *
 * WHY A SECOND WAY?
 * ----------------
 * ministrace.c uses PTRACE_SYSCALL: the kernel stops the tracee on EVERY
 * syscall, entry and exit, and wakes the tracer each time. That is two context
 * switches per syscall, for every syscall, whether you care about it or not.
 *
 * Here we push the decision INTO the kernel. We install a classic-BPF program
 * (seccomp filter) that inspects each syscall and returns SECCOMP_RET_TRACE for
 * the ones we want; only those generate a PTRACE_EVENT_SECCOMP stop that wakes
 * us. A filter that returns SECCOMP_RET_ALLOW for the rest lets them run with no
 * tracer round-trip at all. This is exactly the mechanism real sandboxes use to
 * make syscall interception cheap — you pay the context switch only for the
 * syscalls of interest, and the in-kernel BPF does the filtering at native
 * speed. (Here we trace *all* syscalls to mirror strace, but the machinery to
 * filter is right there — see the filter program below.)
 *
 * THE PIECES
 * ----------
 *   PR_SET_NO_NEW_PRIVS — a prctl the tracee MUST set before installing a
 *       seccomp filter (unless it is privileged). It promises "exec can never
 *       grant me more privilege," which closes a setuid-based bypass and is the
 *       kernel's precondition for unprivileged seccomp.
 *   seccomp(SECCOMP_SET_MODE_FILTER, ...) — load the BPF program. From then on
 *       the kernel runs it on every syscall this process (and its children)
 *       makes.
 *   SECCOMP_RET_TRACE — the filter's verdict that means "trap to the ptrace
 *       tracer if it asked for seccomp events, else just run." The low 16 bits
 *       ("data") are delivered to the tracer via PTRACE_GET_SYSCALL_INFO or
 *       PTRACE_GETEVENTMSG, a cheap side-channel we could use to tag syscalls.
 *   PTRACE_O_TRACESECCOMP — the tracer option that turns those verdicts into
 *       PTRACE_EVENT_SECCOMP stops we can catch with waitpid.
 *
 * A seccomp stop fires at syscall ENTRY (before the call runs). To also print
 * the return value we single-step to the syscall-EXIT stop with one
 * PTRACE_SYSCALL, read rax, then PTRACE_CONT back to full speed until the next
 * filtered syscall.
 *
 * Platform: Linux (or WSL2). Needs the Linux seccomp/BPF headers.
 * ===========================================================================
 */
#include <sys/ptrace.h>    /* ptrace(), PTRACE_* requests + event constants   */
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/user.h>      /* struct user_regs_struct                         */
#include <sys/prctl.h>     /* prctl(), PR_SET_NO_NEW_PRIVS                     */
#include <sys/syscall.h>   /* SYS_seccomp — no portable glibc wrapper         */
#include <linux/seccomp.h> /* SECCOMP_SET_MODE_FILTER, SECCOMP_RET_*, seccomp_data */
#include <linux/filter.h>  /* struct sock_filter / sock_fprog, BPF_* macros   */
#include <linux/audit.h>   /* AUDIT_ARCH_X86_64                               */
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <stddef.h>        /* offsetof                                        */
#include <stdio.h>
#include <stdlib.h>

#include "syscall_table.h"
#include "decode.h"

#define TRACE_OUT stderr

/* Older kernels/headers call the "kill the whole process" verdict differently;
 * fall back gracefully so this still compiles broadly. */
#ifndef SECCOMP_RET_KILL_PROCESS
#define SECCOMP_RET_KILL_PROCESS SECCOMP_RET_KILL
#endif

/* ===========================================================================
 * The seccomp filter — a tiny classic-BPF program.
 * ===========================================================================
 *
 * BPF here is the OLD "classic" cBPF: a fixed-length array of {code,jt,jf,k}
 * instructions run by an in-kernel interpreter/JIT over a `struct seccomp_data`
 * (nr, arch, instruction_pointer, args[6]). Each instruction is built with the
 * BPF_STMT (no branch) / BPF_JUMP (conditional) macros.
 *
 * Program logic, top to bottom:
 *   1. Load the 32-bit `arch` field. We MUST check it: syscall NUMBERS differ
 *      per ABI, so a filter that keys on numbers is only safe once it has
 *      confirmed the arch it was written for. A process can enter x86-64 or the
 *      x32/compat ABI; we only understand x86-64 here.
 *   2. If arch != AUDIT_ARCH_X86_64, kill the process. (Defense in depth: this
 *      is the standard guard that stops a compat-ABI end-run around a filter.)
 *   3. Otherwise, return SECCOMP_RET_TRACE for the syscall so our ptrace tracer
 *      is notified. To FILTER instead of trace-all, you would compare
 *      seccomp_data.nr here and RET_TRACE only for chosen numbers, RET_ALLOW
 *      for the rest — that is the one edit that turns this into a real sandbox.
 * =========================================================================== */
static struct sock_filter filter_prog[] = {
    /* A = seccomp_data.arch  (BPF_LD word, absolute offset into seccomp_data) */
    BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(struct seccomp_data, arch)),
    /* if (A == AUDIT_ARCH_X86_64) skip the next (kill) insn, else fall through */
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, AUDIT_ARCH_X86_64, 1, 0),
    /* wrong ABI -> refuse to run at all                                       */
    BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL_PROCESS),
    /* right ABI -> hand every syscall to the tracer                           */
    BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_TRACE),
};

/* Install the filter in the CURRENT process. Returns 0 on success, -1 on error
 * (errno set). Must run in the tracee after PR_SET_NO_NEW_PRIVS and before exec. */
static int install_seccomp_filter(void)
{
    struct sock_fprog prog = {
        .len    = (unsigned short)(sizeof(filter_prog) / sizeof(filter_prog[0])),
        .filter = filter_prog,
    };

    /* NO_NEW_PRIVS: the kernel refuses an unprivileged seccomp filter without
     * it. Args after the option are unused for this prctl; pass zeros. */
    if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) < 0)
        return -1;

    /* seccomp(2) has no guaranteed glibc wrapper, so we make the raw syscall.
     * SECCOMP_SET_MODE_FILTER installs `prog`; flags 0 (we do not need the
     * SYNC/LOG/NOTIF extensions). */
    if (syscall(SYS_seccomp, SECCOMP_SET_MODE_FILTER, 0, &prog) < 0)
        return -1;

    return 0;
}

/* ===========================================================================
 * Register decoding — the same ABI mapping ministrace uses. Duplicated (rather
 * than shared through a header) so each tracer reads as a complete, standalone
 * teaching program. See ministrace.c for the fuller commentary.
 * =========================================================================== */
static void arg_regs(const struct user_regs_struct *r, unsigned long a[6])
{
    a[0] = r->rdi; a[1] = r->rsi; a[2] = r->rdx;
    a[3] = r->r10;  /* arg4 is r10, not rcx (the `syscall` insn clobbers rcx) */
    a[4] = r->r8;  a[5] = r->r9;
}

static char g_line[1024];   /* buffered entry line (see ministrace for why)   */
static long g_nr;           /* pending syscall number                        */

static void build_entry(pid_t pid, const struct user_regs_struct *r)
{
    unsigned long a[6];
    arg_regs(r, a);
    g_nr = (long)r->orig_rax;

    const struct syscall_ent *d = syscall_detail(g_nr);
    const char *nm = syscall_name(g_nr);
    int cap = (int)sizeof(g_line), p;

    if (d) {
        p = snprintf(g_line, cap, "%s(", d->name);
        for (int i = 0; i < d->argc && p < cap - 1; i++) {
            if (i) p += snprintf(g_line + p, cap - p, ", ");
            p += format_arg(pid, d->argt[i], a[i], g_line + p, cap - p);
        }
        snprintf(g_line + p, cap - p, ")");
    } else if (nm) {
        p = snprintf(g_line, cap, "%s(", nm);
        for (int i = 0; i < 6 && p < cap - 1; i++)
            p += snprintf(g_line + p, cap - p, "%s%#lx", i ? ", " : "", a[i]);
        snprintf(g_line + p, cap - p, ")");
    } else {
        snprintf(g_line, cap, "syscall_%ld(%#lx, %#lx, %#lx, ...)",
                 g_nr, a[0], a[1], a[2]);
    }
}

static int returns_pointer(long nr)
{
    return nr == 9 || nr == 12 || nr == 25 || nr == 30;   /* mmap/brk/mremap/shmat */
}

static void finish_exit(const struct user_regs_struct *r)
{
    char rbuf[64];
    format_retval((long)r->rax, returns_pointer(g_nr), rbuf, (int)sizeof(rbuf));
    fprintf(TRACE_OUT, "%s = %s\n", g_line, rbuf);
}

/* ===========================================================================
 * Feature probe: does THIS kernel deliver PTRACE_EVENT_SECCOMP?
 * ===========================================================================
 *
 * A filter can return SECCOMP_RET_TRACE only usefully if the kernel actually
 * traps to the ptrace tracer. Filtering (RET_ALLOW/RET_ERRNO/RET_KILL) and the
 * ptrace *notification* are separable: some kernels — notably several WSL2 /
 * "microsoft-standard" builds — compile in CONFIG_SECCOMP_FILTER but do NOT
 * wire up the PTRACE_EVENT_SECCOMP path. On such a kernel a RET_TRACE syscall
 * silently returns -ENOSYS to the tracee instead of stopping it, which would
 * break (and usually crash) the traced program with a baffling SIGSEGV.
 *
 * Rather than let that happen, we probe up front with a throwaway child that
 * installs a filter tracing only getpid(2) — everything else is ALLOWed, so the
 * probe child can never be broken by an unexpected ENOSYS — and check whether
 * its getpid produces a real PTRACE_EVENT_SECCOMP. Returns 1 if the feature
 * works, 0 if not. This probe is itself a compact, honest demonstration of the
 * whole seccomp+ptrace handshake.
 * =========================================================================== */
static int seccomp_trace_available(void)
{
    static struct sock_filter probe[] = {
        BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(struct seccomp_data, arch)),
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, AUDIT_ARCH_X86_64, 1, 0),
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL_PROCESS),
        BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(struct seccomp_data, nr)),
        /* if nr == getpid(39): fall through to RET_TRACE, else skip to ALLOW */
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, 39, 0, 1),
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_TRACE),
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
    };

    pid_t k = fork();
    if (k < 0)
        return 0;                       /* cannot tell -> report unavailable    */
    if (k == 0) {
        struct sock_fprog p = {
            .len = (unsigned short)(sizeof(probe) / sizeof(probe[0])),
            .filter = probe,
        };
        /* TRACEME, then stop ourselves so the parent can enable TRACESECCOMP
         * BEFORE we install the filter and call getpid — otherwise the probed
         * call would race the option and give a false negative. */
        ptrace(PTRACE_TRACEME, 0, 0, 0);
        raise(SIGSTOP);
        prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
        syscall(SYS_seccomp, SECCOMP_SET_MODE_FILTER, 0, &p);
        (void)getpid();                 /* the RET_TRACE call we are testing     */
        _exit(0);
    }

    int status, supported = 0;
    if (waitpid(k, &status, 0) < 0)     /* the SIGSTOP-stop                      */
        return 0;
    ptrace(PTRACE_SETOPTIONS, k, 0,
           (void *)(PTRACE_O_TRACESECCOMP | PTRACE_O_EXITKILL));
    for (;;) {
        if (ptrace(PTRACE_CONT, k, 0, 0) < 0)   /* resume (suppress the STOP)    */
            break;
        if (waitpid(k, &status, 0) < 0)
            break;
        if (WIFEXITED(status) || WIFSIGNALED(status))
            break;                      /* child ran to completion, no event     */
        if (WIFSTOPPED(status)) {
            unsigned ev = (unsigned)status >> 8;
            if (ev == (unsigned)(SIGTRAP | (PTRACE_EVENT_SECCOMP << 8))) {
                supported = 1;          /* got the notification -> it works       */
                break;
            }
            /* any other stop: loop and continue past it */
        }
    }
    kill(k, SIGKILL);                   /* tear the probe child down             */
    waitpid(k, &status, 0);            /* and reap it (no zombie)               */
    return supported;
}

/* ===========================================================================
 * The supervision loop, seccomp-event driven.
 * =========================================================================== */
static int trace_loop(pid_t child)
{
    int status;

    /* Wait for the initial execve trap (from PTRACE_TRACEME). */
    while (waitpid(child, &status, 0) < 0) {
        if (errno == EINTR) continue;
        perror("waitpid");
        return 1;
    }
    if (WIFEXITED(status)) {
        fprintf(TRACE_OUT, "+++ exited with %d +++\n", WEXITSTATUS(status));
        return WEXITSTATUS(status);
    }

    /* Ask for seccomp events (and tag syscall-stops with bit 7 via
     * TRACESYSGOOD so we can distinguish the exit-stop we single-step to). */
    if (ptrace(PTRACE_SETOPTIONS, child, 0,
               (void *)(PTRACE_O_TRACESECCOMP | PTRACE_O_TRACESYSGOOD |
                        PTRACE_O_EXITKILL)) < 0) {
        perror("PTRACE_SETOPTIONS");
    }

    int restart_sig = 0;
    for (;;) {
        /* Run at FULL SPEED until the filter fires (or a signal/exit). Unlike
         * ministrace we do NOT stop on every syscall — the kernel BPF decides. */
        if (ptrace(PTRACE_CONT, child, 0, (void *)(long)restart_sig) < 0) {
            if (errno == ESRCH) break;
            perror("PTRACE_CONT");
            break;
        }
        restart_sig = 0;

        while (waitpid(child, &status, 0) < 0) {
            if (errno == EINTR) continue;
            perror("waitpid");
            return 1;
        }

        if (WIFEXITED(status)) {
            fprintf(TRACE_OUT, "+++ exited with %d +++\n", WEXITSTATUS(status));
            return WEXITSTATUS(status);
        }
        if (WIFSIGNALED(status)) {
            fprintf(TRACE_OUT, "+++ killed by %s +++\n",
                    signal_name(WTERMSIG(status)));
            return 128 + WTERMSIG(status);
        }
        if (!WIFSTOPPED(status))
            continue;

        /* Is this a PTRACE_EVENT_SECCOMP stop? Ptrace encodes an event in the
         * high byte of the wait status: status>>8 == (SIGTRAP | event<<8). */
        unsigned event = (unsigned)status >> 8;
        if (event == (unsigned)(SIGTRAP | (PTRACE_EVENT_SECCOMP << 8))) {
            struct user_regs_struct regs;

            /* --- syscall ENTRY (the filter just matched) --- */
            if (ptrace(PTRACE_GETREGS, child, 0, &regs) < 0) {
                perror("PTRACE_GETREGS");
                continue;
            }
            build_entry(child, &regs);

            /* Single-step to the syscall-EXIT stop so we can read the result.
             * PTRACE_SYSCALL here resumes and stops once more at exit. */
            if (ptrace(PTRACE_SYSCALL, child, 0, 0) < 0) {
                if (errno == ESRCH) break;
                perror("PTRACE_SYSCALL");
                break;
            }
            while (waitpid(child, &status, 0) < 0) {
                if (errno == EINTR) continue;
                perror("waitpid");
                return 1;
            }
            if (WIFEXITED(status)) {
                /* e.g. exit_group: the entry had no matching exit. */
                fprintf(TRACE_OUT, "%s = ?\n", g_line);
                fprintf(TRACE_OUT, "+++ exited with %d +++\n", WEXITSTATUS(status));
                return WEXITSTATUS(status);
            }
            if (WIFSIGNALED(status)) {
                fprintf(TRACE_OUT, "+++ killed by %s +++\n",
                        signal_name(WTERMSIG(status)));
                return 128 + WTERMSIG(status);
            }
            /* --- syscall EXIT: read rax and print the completed line. --- */
            if (ptrace(PTRACE_GETREGS, child, 0, &regs) == 0)
                finish_exit(&regs);
            /* Fall through to the top -> PTRACE_CONT back to full speed. */
        } else {
            /* A plain signal-delivery stop: report and re-inject. */
            int sig = WSTOPSIG(status);
            if (sig != SIGTRAP) {   /* SIGTRAP without an event = our own step */
                fprintf(TRACE_OUT, "--- %s ---\n", signal_name(sig));
                restart_sig = sig;
            }
        }
    }
    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s PROGRAM [ARGS...]\n", argv[0]);
        fprintf(stderr, "  e.g. %s /bin/echo hello\n", argv[0]);
        return EXIT_FAILURE;
    }

    /* Fail loudly on a kernel that won't deliver seccomp events, rather than
     * letting the tracee crash with a mystifying -ENOSYS/SIGSEGV. */
    if (!seccomp_trace_available()) {
        fprintf(stderr,
            "%s: this kernel accepts a SECCOMP_RET_TRACE filter but does NOT\n"
            "deliver PTRACE_EVENT_SECCOMP to the tracer (verified by probe).\n"
            "That path is required for seccomp-driven tracing and is missing on\n"
            "some WSL2 / microsoft-standard kernels. Use ./ministrace (the\n"
            "PTRACE_SYSCALL tracer), which needs no such kernel support, or run\n"
            "this on a stock Linux kernel.\n",
            argv[0]);
        return EXIT_FAILURE;
    }

    pid_t child = fork();
    if (child < 0) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (child == 0) {
        /* ---- CHILD ----
         * Order matters: TRACEME, then NO_NEW_PRIVS + filter, then exec. The
         * filter is installed BEFORE exec so it governs the target program (and
         * the exec itself proceeds because the tracer has not yet enabled
         * TRACESECCOMP). */
        if (ptrace(PTRACE_TRACEME, 0, 0, 0) < 0) {
            perror("PTRACE_TRACEME");
            _exit(127);
        }
        if (install_seccomp_filter() < 0) {
            perror("install_seccomp_filter");
            _exit(127);
        }
        execvp(argv[1], &argv[1]);
        perror("execvp");
        _exit(127);
    }

    return trace_loop(child);
}
