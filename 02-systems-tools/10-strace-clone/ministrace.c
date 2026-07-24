/* ===========================================================================
 * ministrace.c — a strace clone via ptrace(PTRACE_SYSCALL).
 * ===========================================================================
 *
 * WHAT THIS PROGRAM DOES
 * ----------------------
 * It forks a child, has the child ask the kernel to trace it, execs the target
 * program, and then supervises it: the kernel stops the child TWICE per syscall
 * — once on ENTRY (arguments in registers, syscall not yet run) and once on EXIT
 * (return value in rax) — and hands control back to us at each stop. We read the
 * registers, decode them against a syscall table, and print a line like:
 *
 *     openat(AT_FDCWD, "/etc/ld.so.cache", O_RDONLY|O_CLOEXEC) = 3
 *
 * THE PTRACE MODEL (the whole point)
 * ----------------------------------
 * ptrace(2) is one syscall with a `request` selector; the ones we use:
 *   PTRACE_TRACEME   — the CHILD calls this once to say "let my parent trace
 *                      me." The next execve then stops the child so the tracer
 *                      can attach cleanly before the new program runs a thing.
 *   PTRACE_SETOPTIONS— set PTRACE_O_TRACESYSGOOD (so syscall-stops arrive as
 *                      signal SIGTRAP|0x80, distinguishable from a real SIGTRAP)
 *                      and PTRACE_O_EXITKILL (kill the child if the tracer dies,
 *                      so we never leak a stopped orphan).
 *   PTRACE_SYSCALL   — resume the child, but stop it again at the next syscall
 *                      entry OR exit. This is the engine of the trace loop.
 *   PTRACE_GETREGS   — copy the child's user-visible register file into us. On
 *                      x86-64: orig_rax = syscall number (preserved across the
 *                      call), rax = return value at exit, and the args are in
 *                      rdi, rsi, rdx, r10, r8, r9 (note r10, NOT rcx — the
 *                      `syscall` instruction clobbers rcx, so the kernel ABI
 *                      substitutes r10 for the 4th argument).
 *   PTRACE_GET_SYSCALL_INFO — the modern (Linux 5.3+) way to ask "is this stop
 *                      an entry or an exit?" without the tracer keeping its own
 *                      toggle. We use it when present and fall back to a toggle.
 *
 * Every ptrace/waitpid return is checked; the error paths (child died, ESRCH,
 * EINTR on wait) are part of what this project is meant to teach.
 *
 * Platform: Linux (or WSL2). It will not build on Windows/macOS — it needs
 * <sys/ptrace.h>, <sys/user.h>, and the Linux ptrace semantics.
 * ===========================================================================
 */
#include <sys/ptrace.h>    /* ptrace() and the PTRACE_* request constants     */
/* glibc's <sys/ptrace.h> declares the PTRACE_GET_SYSCALL_INFO *request* but not
 * the struct it fills; the kernel uapi header provides `struct
 * ptrace_syscall_info` and the PTRACE_SYSCALL_INFO_* op enum. On modern
 * toolchains the two headers are written to coexist. */
#include <linux/ptrace.h>  /* struct ptrace_syscall_info, PTRACE_SYSCALL_INFO_* */
#include <sys/types.h>     /* pid_t                                           */
#include <sys/wait.h>      /* waitpid(), W* status macros                     */
#include <sys/user.h>      /* struct user_regs_struct (the register layout)   */
#include <unistd.h>        /* fork(), execvp(), _exit()                       */
#include <signal.h>        /* SIGTRAP                                         */
#include <errno.h>         /* errno, ESRCH, EINTR                            */
#include <string.h>        /* memset                                         */
#include <stdio.h>         /* fprintf, snprintf                              */
#include <stdlib.h>        /* EXIT_FAILURE                                   */

#include "syscall_table.h"
#include "decode.h"

/* All trace output goes to STDERR, exactly like real strace. That keeps the
 * traced program's own stdout clean so `ministrace ./prog > out.txt` still
 * captures only prog's output, and the trace lands on the terminal. */
#define TRACE_OUT stderr

/* ---------------------------------------------------------------------------
 * tstate — everything the trace loop must remember between the two stops of a
 * single syscall. We BUFFER the entry line (name + decoded args) here instead
 * of printing it immediately, then complete it with " = <ret>" at the exit
 * stop. Buffering avoids the classic interleave bug where a write(2) the child
 * performs *between* our two stops would splat into the middle of our line.
 * --------------------------------------------------------------------------- */
struct tstate {
    char line[1024];   /* the syscall line being assembled                   */
    int  len;          /* bytes used in line[]                               */
    int  pending;      /* 1 => we printed an entry and await its exit ')'     */
    long nr;           /* the syscall number of the pending call             */
    int  expect_entry; /* toggle fallback: is the next stop an entry?         */
    int  info_ok;      /* 1 => PTRACE_GET_SYSCALL_INFO works on this kernel   */
};

/* The 6 syscall-argument registers, in ABI order, pulled out of a saved
 * register file. Written as a helper so the "r10 stands in for rcx" fact lives
 * in exactly one place — this is the same mapping asm/demo.c annotates. */
static void arg_regs(const struct user_regs_struct *r, unsigned long a[6])
{
    a[0] = r->rdi;   /* arg1 */
    a[1] = r->rsi;   /* arg2 */
    a[2] = r->rdx;   /* arg3 */
    a[3] = r->r10;   /* arg4 — r10, because `syscall` destroys rcx            */
    a[4] = r->r8;    /* arg5 */
    a[5] = r->r9;    /* arg6 */
}

/* Classify the current syscall-stop as entry (1) or exit (2).
 *
 * Preferred oracle: PTRACE_GET_SYSCALL_INFO fills a struct whose `.op` field is
 * ENTRY/EXIT/SECCOMP/NONE — authoritative, no bookkeeping. If the kernel is too
 * old (the request fails), we disable it for the rest of the run and fall back
 * to a strict entry/exit TOGGLE, which is correct because PTRACE_SYSCALL always
 * alternates entry, exit, entry, exit for a single-threaded tracee. */
static int classify(pid_t pid, struct tstate *st)
{
#ifdef PTRACE_GET_SYSCALL_INFO
    if (st->info_ok) {
        struct ptrace_syscall_info si;
        /* addr = sizeof(buffer), data = &buffer; returns bytes available > 0. */
        long r = ptrace(PTRACE_GET_SYSCALL_INFO, pid,
                        (void *)sizeof(si), &si);
        if (r > 0) {
            if (si.op == PTRACE_SYSCALL_INFO_ENTRY) { st->expect_entry = 0; return 1; }
            if (si.op == PTRACE_SYSCALL_INFO_EXIT)  { st->expect_entry = 1; return 2; }
            return 0;   /* SECCOMP/NONE: not an entry/exit we render here      */
        }
        st->info_ok = 0;   /* unsupported here — never ask again              */
    }
#else
    (void)pid;
#endif
    /* Toggle fallback. */
    if (st->expect_entry) { st->expect_entry = 0; return 1; }
    st->expect_entry = 1;
    return 2;
}

/* Does this syscall return a pointer/address rather than a count? Those want
 * hex formatting (mmap -> 0x7f..., brk -> 0x55...). Everything else is decimal
 * with negative-errno decoding. */
static int returns_pointer(long nr)
{
    return nr == 9    /* mmap    */
        || nr == 12   /* brk     */
        || nr == 25   /* mremap  */
        || nr == 30;  /* shmat   */
}

/* Assemble the "name(arg, arg, ...)" prefix for a syscall ENTRY into st->line
 * (no return value yet, no newline). Reads tracee memory for string args. */
static void build_entry(pid_t pid, struct tstate *st, const struct user_regs_struct *r)
{
    unsigned long a[6];
    arg_regs(r, a);
    long nr = (long)r->orig_rax;
    st->nr = nr;

    const struct syscall_ent *d = syscall_detail(nr);
    const char *nm = syscall_name(nr);
    int cap = (int)sizeof(st->line);
    int p;

    if (d) {
        /* Richly decoded: print the real name and each typed argument. */
        p = snprintf(st->line, cap, "%s(", d->name);
        for (int i = 0; i < d->argc && p < cap - 1; i++) {
            if (i) p += snprintf(st->line + p, cap - p, ", ");
            p += format_arg(pid, d->argt[i], a[i], st->line + p, cap - p);
        }
        p += snprintf(st->line + p, cap - p, ")");
    } else if (nm) {
        /* Name known but arg shape unknown: dump the six raw arg registers as
         * hex. Slots past the real arity hold leftover register contents — we
         * show them rather than silently guess an arity. */
        p = snprintf(st->line, cap, "%s(", nm);
        for (int i = 0; i < 6 && p < cap - 1; i++)
            p += snprintf(st->line + p, cap - p, "%s%#lx", i ? ", " : "", a[i]);
        p += snprintf(st->line + p, cap - p, ")");
    } else {
        /* Number entirely unknown (or bogus): name it by number. */
        p = snprintf(st->line, cap, "syscall_%ld(%#lx, %#lx, %#lx, ...)",
                     nr, a[0], a[1], a[2]);
    }
    st->len = p;
    st->pending = 1;
}

/* Complete the pending line at the EXIT stop and flush it. */
static void finish_exit(struct tstate *st, const struct user_regs_struct *r)
{
    long ret = (long)r->rax;
    char rbuf[64];
    format_retval(ret, returns_pointer(st->nr), rbuf, (int)sizeof(rbuf));
    fprintf(TRACE_OUT, "%s = %s\n", st->line, rbuf);
    st->pending = 0;
}

/* Handle one genuine syscall-stop (WSTOPSIG == SIGTRAP|0x80). */
static void on_syscall_stop(pid_t pid, struct tstate *st)
{
    struct user_regs_struct regs;
    /* PTRACE_GETREGS copies the whole user register file into `regs`. (The
     * portable-across-arches spelling is PTRACE_GETREGSET+NT_PRSTATUS; GETREGS
     * is x86-specific but far clearer for teaching.) */
    if (ptrace(PTRACE_GETREGS, pid, 0, &regs) < 0) {
        perror("PTRACE_GETREGS");
        return;
    }
    int op = classify(pid, st);
    if (op == 1)                 /* entry: capture args now (buffers valid)    */
        build_entry(pid, st, &regs);
    else if (op == 2 && st->pending)   /* exit: read rax, emit the line        */
        finish_exit(st, &regs);
}

/* ---------------------------------------------------------------------------
 * The supervision loop.
 * --------------------------------------------------------------------------- */
static int trace_loop(pid_t child)
{
    int status;
    struct tstate st;
    memset(&st, 0, sizeof(st));
    st.expect_entry = 1;   /* first post-exec syscall-stop is an entry         */
    st.info_ok = 1;        /* optimistically try PTRACE_GET_SYSCALL_INFO       */

    /* First wait: the child is stopped at the execve trap (because TRACEME made
     * execve stop it). waitpid can be interrupted by a signal to US -> retry. */
    while (waitpid(child, &status, 0) < 0) {
        if (errno == EINTR) continue;
        perror("waitpid");
        return 1;
    }
    if (WIFEXITED(status)) {          /* exec failed before it even stopped     */
        fprintf(TRACE_OUT, "+++ exited with %d +++\n", WEXITSTATUS(status));
        return WEXITSTATUS(status);
    }

    /* Now that the child is stopped, set our options. TRACESYSGOOD tags syscall
     * stops with bit 7 of the signal so we can tell them from a real SIGTRAP;
     * EXITKILL guarantees no orphaned, forever-stopped child if we crash. */
    if (ptrace(PTRACE_SETOPTIONS, child, 0,
               (void *)(PTRACE_O_TRACESYSGOOD | PTRACE_O_EXITKILL)) < 0) {
        perror("PTRACE_SETOPTIONS");   /* non-fatal: we can still trace         */
    }

    int restart_sig = 0;   /* signal to deliver to the child on the next resume */
    for (;;) {
        /* Resume the child, arranging to stop again at the next syscall
         * boundary. `restart_sig` re-injects a pending signal (0 = none). */
        if (ptrace(PTRACE_SYSCALL, child, 0, (void *)(long)restart_sig) < 0) {
            if (errno == ESRCH) break;   /* child already gone                 */
            perror("PTRACE_SYSCALL");
            break;
        }
        restart_sig = 0;

        while (waitpid(child, &status, 0) < 0) {
            if (errno == EINTR) continue;
            perror("waitpid");
            return 1;
        }

        if (WIFEXITED(status)) {
            if (st.pending)              /* e.g. exit_group never "returns"     */
                fprintf(TRACE_OUT, "%s = ?\n", st.line);
            fprintf(TRACE_OUT, "+++ exited with %d +++\n", WEXITSTATUS(status));
            return WEXITSTATUS(status);
        }
        if (WIFSIGNALED(status)) {
            fprintf(TRACE_OUT, "+++ killed by %s +++\n",
                    signal_name(WTERMSIG(status)));
            return 128 + WTERMSIG(status);
        }
        if (!WIFSTOPPED(status))
            continue;                    /* WIFCONTINUED etc. — nothing to do   */

        int sig = WSTOPSIG(status);
        if (sig == (SIGTRAP | 0x80)) {
            on_syscall_stop(child, &st); /* a syscall entry or exit             */
        } else {
            /* A signal-delivery-stop: the child was about to receive `sig`.
             * Report it and re-inject so the child's own handler/default action
             * still happens (dropping it would change program behaviour). */
            fprintf(TRACE_OUT, "--- %s ---\n", signal_name(sig));
            restart_sig = sig;
        }
    }
    return 0;
}

/* ---------------------------------------------------------------------------
 * main — fork, arrange tracing in the child, supervise in the parent.
 * --------------------------------------------------------------------------- */
int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s PROGRAM [ARGS...]\n", argv[0]);
        fprintf(stderr, "  e.g. %s /bin/echo hello\n", argv[0]);
        return EXIT_FAILURE;
    }

    pid_t child = fork();
    if (child < 0) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (child == 0) {
        /* ---- CHILD ----
         * Ask to be traced BEFORE exec. From here, the next execve will trap and
         * hand control to the parent. PTRACE_TRACEME cannot fail for a normal
         * process, but we check it anyway. */
        if (ptrace(PTRACE_TRACEME, 0, 0, 0) < 0) {
            perror("PTRACE_TRACEME");
            _exit(127);
        }
        /* Replace this process image with the target program. On success execvp
         * never returns; the kernel raises the trap that our parent waits for.
         * argv+1 is a NULL-terminated vector (argv[argc] == NULL by the C ABI). */
        execvp(argv[1], &argv[1]);
        /* Only reached if exec FAILED (bad path, no permission). */
        perror("execvp");
        _exit(127);
    }

    /* ---- PARENT (the tracer) ---- */
    return trace_loop(child);
}
