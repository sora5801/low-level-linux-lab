/* ===========================================================================
 * seccomp_filter.c — build and install a hand-written classic-BPF ALLOWLIST.
 * ===========================================================================
 *
 * seccomp ("secure computing") mode 2 attaches a classic-BPF program to a task.
 * The kernel runs that program at the *entry* of every syscall, BEFORE the
 * syscall executes, passing it a read-only `struct seccomp_data`:
 *
 *     struct seccomp_data {
 *         int   nr;                    // offset 0  : the syscall number
 *         __u32 arch;                  // offset 4  : AUDIT_ARCH_* of the caller
 *         __u64 instruction_pointer;   // offset 8  : userspace RIP at the call
 *         __u64 args[6];               // offset 16 : the 6 syscall arg REGISTERS
 *     };
 *
 * CRUCIAL LIMITATION (the reason this whole project needs three layers):
 *   args[] are the raw REGISTER values. If arg0 is a `const char *path`, the
 *   filter sees the POINTER, an 8-byte number — it CANNOT follow it into user
 *   memory. Classic BPF has no way to dereference a userspace pointer (and it
 *   would be a TOCTOU trap if it could — see README). So seccomp can gate
 *   "may you call openat at all?" but never "may you open THIS path?". Path
 *   decisions belong to Landlock (layer 3) or, racily, to the ptrace tracer.
 *
 * The program returns a 32-bit action. The high bits pick the behaviour and,
 * for ERRNO/TRACE, the low 16 bits carry data (an errno or a tracer message).
 * When several filters are stacked, the kernel runs all of them and takes the
 * most restrictive result (lowest numeric value wins).
 *
 * WHY ALLOWLIST, NOT DENYLIST: we enumerate the handful of syscalls the target
 * legitimately needs and KILL everything else by default. A denylist ("block
 * these bad ones") fails open: the ~400 syscalls you forgot, plus every syscall
 * added in a future kernel, sail straight through. An allowlist fails closed.
 * ===========================================================================
 */
#define _GNU_SOURCE
#include <stddef.h>          /* offsetof                                      */
#include <stdint.h>
#include <string.h>          /* memcpy                                        */
#include <errno.h>
#include <unistd.h>
#include <sys/prctl.h>       /* prctl, PR_SET_NO_NEW_PRIVS, PR_SET_SECCOMP    */
#include <sys/syscall.h>     /* SYS_seccomp, __NR_*                           */
#include <linux/audit.h>     /* AUDIT_ARCH_X86_64                             */
#include <linux/filter.h>    /* struct sock_filter, struct sock_fprog, BPF_*  */
#include <linux/seccomp.h>   /* SECCOMP_RET_*, SECCOMP_SET_MODE_FILTER, ...   */

#include "sandbox.h"

/* ---------------------------------------------------------------------------
 * Some constants are missing on older <linux/*> headers. Define fall-backs so
 * this file compiles against any reasonably modern kernel headers, and so the
 * reader sees the raw ABI values rather than opaque macro names.
 * --------------------------------------------------------------------------- */
#ifndef SECCOMP_RET_KILL_PROCESS
#define SECCOMP_RET_KILL_PROCESS 0x80000000U /* kill the whole process (SIGSYS)*/
#endif
#ifndef SECCOMP_RET_TRACE
#define SECCOMP_RET_TRACE        0x7ff00000U /* defer to the ptrace tracer     */
#endif
#ifndef SECCOMP_RET_ERRNO
#define SECCOMP_RET_ERRNO        0x00050000U /* fail the call with errno in low16*/
#endif
#ifndef SECCOMP_RET_ALLOW
#define SECCOMP_RET_ALLOW        0x7fff0000U /* run the syscall unmodified      */
#endif
#ifndef SECCOMP_RET_DATA
#define SECCOMP_RET_DATA         0x0000ffffU /* low 16 bits = errno / trace tag */
#endif

/* On x86-64 the x32 ABI reuses the syscall table with bit 30 (0x40000000) set.
 * A filter that only checks the low bits could be fooled into thinking an x32
 * `openat` is a different, allowed syscall. We reject anything with this bit. */
#ifndef X32_SYSCALL_BIT
#define X32_SYSCALL_BIT 0x40000000U
#endif

/* ---------------------------------------------------------------------------
 * THE POLICY (the human-readable allowlist).
 *
 * This is deliberately tiny because our target (target.c) is a static,
 * no-libc binary that makes exactly the syscalls listed here — so you can read
 * the whole trust surface at a glance. A real allowlist for a glibc program is
 * derived empirically (run under `SECCOMP_RET_LOG` or strace, collect the set).
 *
 * It exercises ALL FOUR actions on purpose, so the demo shows each:
 *   ALLOW  — normal operation (write, exit, ...).
 *   ERRNO  — ptrace() is neutralized to -EPERM: the target's anti-debug probe
 *            fails gracefully instead of dying.
 *   TRACE  — openat() defers to the supervisor, which reads the path (the only
 *            layer that CAN) and applies extra policy / logging.
 *   KILL   — anything else (e.g. socket()) terminates the process. Default-deny.
 * --------------------------------------------------------------------------- */
const struct policy_rule SANDBOX_POLICY[] = {
    /* --- plain-allowed: the target's legitimate syscalls -------------------- */
    { SYS_write,       ACT_ALLOW, 0,      "write"       },
    { SYS_read,        ACT_ALLOW, 0,      "read"        },
    { SYS_close,       ACT_ALLOW, 0,      "close"       },
    { SYS_getpid,      ACT_ALLOW, 0,      "getpid"      },
    { SYS_exit,        ACT_ALLOW, 0,      "exit"        },
    { SYS_exit_group,  ACT_ALLOW, 0,      "exit_group"  },
    /* execve must be allowed: the sandbox installs the filter and THEN execs
     * the target, so the very first filtered syscall is this execve itself. */
    { SYS_execve,      ACT_ALLOW, 0,      "execve"      },

    /* --- neutralized: run, but always fail with a chosen errno ------------- */
    /* ptrace: a program that tries to PTRACE_TRACEME to detect/evade a debugger
     * gets -EPERM and keeps running, rather than being killed. Shows ACT_ERRNO. */
    { SYS_ptrace,      ACT_ERRNO, EPERM,  "ptrace"      },

    /* --- deferred to the tracer: the path-based decisions seccomp can't make */
    { SYS_openat,      ACT_TRACE, 0,      "openat"      },

    /* Everything not listed above hits the trailing default: ACT_KILL. */
};
const size_t SANDBOX_POLICY_LEN = sizeof(SANDBOX_POLICY) / sizeof(SANDBOX_POLICY[0]);

/* Look up a syscall number in the policy for logging; falls back to a numeric
 * name. O(n) over a table of <20 rows — clarity over speed here. */
const char *syscall_name(long nr)
{
    static char buf[32];
    for (size_t i = 0; i < SANDBOX_POLICY_LEN; i++)
        if (SANDBOX_POLICY[i].nr == nr)
            return SANDBOX_POLICY[i].name;
    /* Not in the policy => it is destined for KILL; give the number so the log
     * of "target killed attempting syscall_41" is actionable. */
    int n = (int)nr, len = 0; char tmp[16];
    buf[len++] = 's'; buf[len++] = 'y'; buf[len++] = 's'; buf[len++] = 'c';
    buf[len++] = 'a'; buf[len++] = 'l'; buf[len++] = 'l'; buf[len++] = '_';
    if (n == 0) { buf[len++] = '0'; }
    else {
        int t = 0; if (n < 0) { buf[len++] = '-'; n = -n; }
        while (n) { tmp[t++] = (char)('0' + n % 10); n /= 10; }
        while (t) buf[len++] = tmp[--t];
    }
    buf[len] = '\0';
    return buf;
}

/* Map our intent enum onto the concrete 32-bit SECCOMP_RET_* action word. */
static uint32_t action_word(const struct policy_rule *r, int use_trace)
{
    switch (r->action) {
    case ACT_ALLOW:
        return SECCOMP_RET_ALLOW;
    case ACT_ERRNO:
        /* Low 16 bits carry the errno the caller will see (as a negative
         * return in the usual Linux convention). */
        return SECCOMP_RET_ERRNO | ((uint32_t)r->errno_val & SECCOMP_RET_DATA);
    case ACT_TRACE:
        /* With a tracer attached, hand off. Without one (use_trace==0) a bare
         * SECCOMP_RET_TRACE would make the kernel fail the call with ENOSYS,
         * which would break the target's legitimate openat; so downgrade to
         * ALLOW and let Landlock (layer 3) do the filesystem enforcement. */
        return use_trace ? SECCOMP_RET_TRACE : SECCOMP_RET_ALLOW;
    case ACT_KILL:
    default:
        return SECCOMP_RET_KILL_PROCESS;
    }
}

/* ---------------------------------------------------------------------------
 * build_program — emit the flat classic-BPF instruction array.
 *
 * Layout of the program we generate (an "accumulator machine": one 32-bit
 * register A, loads are relative to the start of seccomp_data):
 *
 *     [0]  A = seccomp_data.arch                (BPF_LD | W | ABS, off=4)
 *     [1]  if A != AUDIT_ARCH_X86_64: goto KILL (BPF_JEQ, jt=1 -> skip kill)
 *     [2]  KILL                                 (guards against a foreign ABI
 *                                                smuggling in different numbers)
 *     [3]  A = seccomp_data.nr                  (BPF_LD | W | ABS, off=0)
 *     [4]  if A >= X32_SYSCALL_BIT: goto KILL   (reject the x32 alias space)
 *     [5]  KILL
 *     then, per policy row, two instructions:
 *          if A == nr: RET <action>             (BPF_JEQ nr, jt=0 fall to RET,
 *                                                jf=1 skip RET to next compare)
 *          RET <action>
 *     finally:
 *          RET KILL                             (default-deny catch-all)
 *
 * Using jt=0/jf=1 per comparison keeps every jump a *short, local* hop, so the
 * array is correct no matter how many rows the policy has — no fragile
 * long-distance offsets to recompute by hand.
 *
 * Returns the number of instructions written into `out` (capacity `cap`), or 0
 * if it would overflow.
 * --------------------------------------------------------------------------- */
static size_t build_program(struct sock_filter *out, size_t cap, int use_trace)
{
    size_t n = 0;

    /* Tiny helper macros scoped to this function. They mirror the kernel's own
     * BPF_STMT/BPF_JUMP but bounds-check against `cap` as they append. */
    #define EMIT(code_, k_) do {                         \
        if (n >= cap) return 0;                          \
        out[n].code = (unsigned short)(code_);           \
        out[n].jt = 0; out[n].jf = 0; out[n].k = (k_);   \
        n++;                                             \
    } while (0)
    #define EMIT_J(code_, k_, jt_, jf_) do {             \
        if (n >= cap) return 0;                          \
        out[n].code = (unsigned short)(code_);           \
        out[n].jt = (jt_); out[n].jf = (jf_); out[n].k = (k_); \
        n++;                                             \
    } while (0)

    /* [arch guard] Load the caller's arch tag and require x86-64. A 32-bit or
     * x32 caller uses a DIFFERENT syscall-number table, so numbers we allow
     * could mean something dangerous there. Fail closed on any mismatch. */
    EMIT(BPF_LD | BPF_W | BPF_ABS, (uint32_t)offsetof(struct seccomp_data, arch));
    EMIT_J(BPF_JMP | BPF_JEQ | BPF_K, AUDIT_ARCH_X86_64, /*jt: match ->*/ 1, /*jf ->*/ 0);
    EMIT(BPF_RET | BPF_K, SECCOMP_RET_KILL_PROCESS);

    /* [load nr] Everything below decides on the syscall number in A. */
    EMIT(BPF_LD | BPF_W | BPF_ABS, (uint32_t)offsetof(struct seccomp_data, nr));

    /* [x32 guard] Reject the x32 ABI alias range (nr with bit 30 set). */
    EMIT_J(BPF_JMP | BPF_JGE | BPF_K, X32_SYSCALL_BIT, /*jt: >= -> KILL*/ 0, /*jf*/ 1);
    EMIT(BPF_RET | BPF_K, SECCOMP_RET_KILL_PROCESS);

    /* [policy rows] if (nr == rule.nr) return rule.action; */
    for (size_t i = 0; i < SANDBOX_POLICY_LEN; i++) {
        uint32_t act = action_word(&SANDBOX_POLICY[i], use_trace);
        /* jt=0: on match, FALL THROUGH to the RET on the next line.
         * jf=1: on mismatch, SKIP that RET and try the next comparison. */
        EMIT_J(BPF_JMP | BPF_JEQ | BPF_K, (uint32_t)SANDBOX_POLICY[i].nr, 0, 1);
        EMIT(BPF_RET | BPF_K, act);
    }

    /* [default] Nothing matched: this is the allowlist's fail-closed floor. */
    EMIT(BPF_RET | BPF_K, SECCOMP_RET_KILL_PROCESS);

    #undef EMIT
    #undef EMIT_J
    return n;
}

/* enable_no_new_privs — set the bit that makes unprivileged seccomp/Landlock
 * legal. After this, no execve in this process or its children can acquire new
 * privileges (setuid bits and file capabilities are ignored). The kernel needs
 * that guarantee before it will let a non-root task install a filter, otherwise
 * a filter could be used to trick a setuid helper. It is a one-way latch. */
int enable_no_new_privs(void)
{
    /* prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0); the trailing zeros are reserved. */
    return prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
}

/* seccomp(2) has no glibc wrapper on some systems; call it directly. Args:
 *   operation = SECCOMP_SET_MODE_FILTER, flags, args = &sock_fprog.
 * We prefer seccomp() over prctl(PR_SET_SECCOMP, ...) because it accepts flags
 * (e.g. TSYNC, SPEC_ALLOW, LOG). We fall back to prctl if seccomp is ENOSYS. */
static int seccomp_set_filter(struct sock_fprog *prog)
{
    long rc = syscall(SYS_seccomp, SECCOMP_SET_MODE_FILTER, 0, prog);
    if (rc == 0)
        return 0;
    if (errno == ENOSYS) {
        /* Very old kernel without the seccomp() syscall: the prctl door still
         * works for the basic (flag-less) filter install. */
        return prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, prog, 0, 0);
    }
    return -1;
}

/* install_seccomp_filter — the public entry: build the program and load it.
 * MUST be called after enable_no_new_privs() (unless the caller is privileged).
 * After a successful return, the filter governs this process and, across the
 * upcoming execve, the target — permanently; a filter can never be removed. */
int install_seccomp_filter(int use_trace)
{
    /* Static storage: the kernel copies the program at install time, but the
     * sock_fprog must point at a valid array for the duration of the call.
     * Sized generously (2 fixed guards worth + 2 per rule + slack). */
    static struct sock_filter prog[8 + 2 * 64];
    size_t n = build_program(prog, sizeof(prog) / sizeof(prog[0]), use_trace);
    if (n == 0) {
        errno = ENOMEM;             /* program did not fit — refuse to install */
        return -1;
    }

    /* sock_fprog carries the length (in instructions) and a pointer to them.
     * `len` is an unsigned short: BPF programs are capped at 4096 instructions,
     * far above our ~20. */
    struct sock_fprog fprog = {
        .len    = (unsigned short)n,
        .filter = prog,
    };
    return seccomp_set_filter(&fprog);
}
