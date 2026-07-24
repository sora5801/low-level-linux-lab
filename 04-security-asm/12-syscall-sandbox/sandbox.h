/* ===========================================================================
 * sandbox.h — shared declarations for the three-layer syscall sandbox.
 * ===========================================================================
 *
 * This project confines a *target* program three complementary ways, each
 * closing a hole the others cannot:
 *
 *   (1) seccomp-bpf ALLOWLIST  — a hand-built classic-BPF program that runs in
 *       the kernel on every syscall entry and returns ALLOW / ERRNO / TRACE /
 *       KILL based ONLY on the syscall number (and a couple of scalar arg
 *       registers). It is fast and unbypassable, but it CANNOT dereference
 *       pointers (see the README) so it cannot make path-based decisions.
 *
 *   (2) ptrace interposer       — a supervising parent that the SECCOMP_RET_TRACE
 *       action wakes up on selected syscalls. It CAN read the tracee's memory
 *       (so it can inspect a path argument) but that read is inherently racy
 *       (TOCTOU) on a multithreaded tracee. Teaches why arg inspection in a
 *       tracer is fragile.
 *
 *   (3) Landlock                — a kernel LSM that enforces filesystem access
 *       by path, in-kernel, with no race. It is the robust answer to the exact
 *       job seccomp cannot do and ptrace does unsafely.
 *
 * Install order is fixed and load-bearing (main() does this):
 *       no_new_privs -> Landlock ruleset -> seccomp filter -> execve(target)
 * A seccomp filter and a Landlock ruleset both survive execve by design; that
 * inheritance is the whole point of "confine, then hand control to the target."
 *
 * Everything here is for YOUR OWN machines and programs you compiled yourself.
 * The "escape attempts" run against a sandbox you built, on a box you own. See
 * README.md "On your own box only."
 * ===========================================================================
 */
#ifndef SANDBOX_H
#define SANDBOX_H

#include <stddef.h>     /* size_t                                             */
#include <sys/types.h>  /* pid_t                                              */

/* ---------------------------------------------------------------------------
 * How the seccomp filter should treat a syscall that is NOT plain-allowed.
 * These map directly onto the SECCOMP_RET_* actions the kernel understands; we
 * expose them as an enum so the policy table below reads like intent, not like
 * magic 32-bit constants.
 * --------------------------------------------------------------------------- */
enum policy_action {
    ACT_ALLOW,   /* SECCOMP_RET_ALLOW  — run the syscall unmodified.           */
    ACT_ERRNO,   /* SECCOMP_RET_ERRNO  — do NOT run it; return -errno instead.
                  *   The tracee survives and just sees a failed call. Good for
                  *   syscalls you want to *neutralize* without killing (e.g.
                  *   ptrace: hand back -EPERM so anti-debug code copes).       */
    ACT_TRACE,   /* SECCOMP_RET_TRACE  — stop and defer to the ptrace tracer.
                  *   If NO tracer is attached the kernel fails the call with
                  *   ENOSYS (a safe default), so TRACE never silently allows.  */
    ACT_KILL     /* SECCOMP_RET_KILL_PROCESS — terminate the whole process with
                  *   SIGSYS. This is the default for everything not listed:
                  *   an ALLOWLIST denies by default (see README "why allowlist
                  *   beats denylist").                                         */
};

/* One row of the human-readable policy. seccomp_filter.c compiles this table
 * into a flat BPF program. `errno_val` is only consulted for ACT_ERRNO rows. */
struct policy_rule {
    long                nr;         /* x86-64 syscall number (e.g. __NR_write) */
    enum policy_action  action;     /* what to do when the target calls it     */
    int                 errno_val;  /* for ACT_ERRNO: the errno to fake         */
    const char         *name;       /* for diagnostics / the supervisor log    */
};

/* The default sandbox policy, defined once in seccomp_filter.c and shared with
 * the supervisor (so its log can name syscalls) and main. NULL-`name`-terminated
 * is avoided; we pair it with an explicit count. */
extern const struct policy_rule SANDBOX_POLICY[];
extern const size_t             SANDBOX_POLICY_LEN;

/* ---------------------------------------------------------------------------
 * Public entry points, one per confinement layer. Each returns 0 on success or
 * -1 on failure with errno set, so the caller can fail closed (refuse to exec
 * the target if any layer could not be installed — a sandbox that silently
 * half-installs is worse than none).
 * --------------------------------------------------------------------------- */

/* Turn on PR_SET_NO_NEW_PRIVS. REQUIRED before an unprivileged process may load
 * a seccomp filter or a Landlock ruleset: it promises the kernel that no future
 * execve can gain privileges (setuid, file caps), which is what makes it safe to
 * let an unprivileged task restrict itself and its children. */
int enable_no_new_privs(void);

/* Build and install the classic-BPF allowlist from SANDBOX_POLICY.
 * `use_trace` chooses whether ACT_TRACE rows emit SECCOMP_RET_TRACE (when a
 * ptrace supervisor is present) or are downgraded to plain ALLOW (when running
 * without a tracer, so the target is not hit with ENOSYS on those calls). */
int install_seccomp_filter(int use_trace);

/* Create a Landlock ruleset that permits read+execute beneath each path in
 * `paths[0..n)` and denies the rest of the filesystem, then restrict_self.
 * A count of 0 means "install an empty ruleset" = deny ALL filesystem access
 * for the handled access rights. Returns -1 (errno) if the kernel lacks
 * Landlock; the caller decides whether that is fatal. */
int install_landlock(const char *const paths[], size_t n);

/* The ptrace supervisor loop. Runs in the PARENT after fork; `child` is the
 * tracee pid. It handles PTRACE_EVENT_SECCOMP stops (raised by ACT_TRACE),
 * inspects the syscall's arguments (including reading a path out of tracee
 * memory to show both the technique and its TOCTOU hazard), decides allow vs
 * deny, and reaps the child. Returns the child's exit code (or 128+signal). */
int supervise(pid_t child);

/* Map an x86-64 syscall number to a short name for logs; returns "syscall_N"
 * for anything not in the policy table. Never returns NULL. */
const char *syscall_name(long nr);

#endif /* SANDBOX_H */
