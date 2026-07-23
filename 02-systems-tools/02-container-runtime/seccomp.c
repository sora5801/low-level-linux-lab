/* ===========================================================================
 * seccomp.c — a classic-BPF syscall allowlist for the container payload.
 * ===========================================================================
 *
 * seccomp mode 2 ("filter") attaches a tiny BPF program to a thread. On EVERY
 * syscall the kernel runs that program, handing it a read-only `struct
 * seccomp_data` { int nr; __u32 arch; __u64 instruction_pointer; __u64 args[6] }
 * and using the program's 32-bit return value as a verdict:
 *
 *     SECCOMP_RET_ALLOW          — let the syscall run
 *     SECCOMP_RET_ERRNO | e      — block it; the syscall returns -e to userspace
 *     SECCOMP_RET_KILL_PROCESS   — kill the whole process immediately
 *
 * The filter is CLASSIC BPF (the packet-filter VM), not eBPF: a fixed-length
 * array of `struct sock_filter { u16 code; u8 jt; u8 jf; u32 k; }` with a single
 * accumulator, forward-only jumps, and no loops. That last property is why the
 * kernel can accept untrusted filters — they provably terminate.
 *
 * OUR POLICY. Default-DENY with EPERM (not KILL): unknown syscalls fail with
 * -EPERM so a curious shell keeps running and you can *see* which calls are
 * blocked, rather than the process vanishing. A production sandbox flips the
 * default to KILL_PROCESS; changing DEFAULT_ACTION below is all it takes.
 *
 * ARCHITECTURE PINNING. The very first thing we check is `arch`. syscall NUMBERS
 * differ per ABI (x86-64 vs the i386/x32 compat entry points), so a filter that
 * only checks `nr` can be bypassed by making the same-numbered call under a
 * different arch. If arch != x86-64 we KILL outright.
 * ===========================================================================
 */
#define _GNU_SOURCE
#include "container.h"
#include "util.h"

#include <errno.h>
#include <linux/audit.h>     /* AUDIT_ARCH_X86_64                              */
#include <linux/filter.h>    /* struct sock_filter, sock_fprog, BPF_* macros   */
#include <linux/seccomp.h>   /* SECCOMP_RET_*, SECCOMP_SET_MODE_FILTER         */
#include <stddef.h>          /* offsetof                                       */
#include <sys/prctl.h>       /* prctl, PR_SET_NO_NEW_PRIVS, PR_SET_SECCOMP     */
#include <sys/syscall.h>     /* __NR_* syscall numbers, SYS_seccomp            */
#include <unistd.h>          /* syscall()                                      */

/* Older headers may not define these; supply the canonical values so the filter
 * still builds. These constants are part of the kernel's stable UAPI. */
#ifndef SECCOMP_RET_KILL_PROCESS
#define SECCOMP_RET_KILL_PROCESS 0x80000000U
#endif
#ifndef SECCOMP_RET_ERRNO
#define SECCOMP_RET_ERRNO        0x00050000U
#endif
#ifndef SECCOMP_RET_ALLOW
#define SECCOMP_RET_ALLOW        0x7fff0000U
#endif
#ifndef SECCOMP_RET_DATA
#define SECCOMP_RET_DATA         0x0000ffffU
#endif

/* Flip this to SECCOMP_RET_KILL_PROCESS for a hard sandbox. */
#define DEFAULT_ACTION (SECCOMP_RET_ERRNO | (EPERM & SECCOMP_RET_DATA))

/* --- BPF program-building helpers ----------------------------------------- *
 * VALIDATE_ARCH: load seccomp_data.arch into the accumulator; if it equals
 * x86-64, skip the next (kill) instruction; otherwise fall through and die.
 *
 * ALLOW(nr): "if accumulator == nr, return ALLOW". The JEQ's jt/jf are RELATIVE
 * instruction counts: jt=0 means "fall through to the very next instruction"
 * (the RET ALLOW), jf=1 means "skip that RET" and continue scanning. */
#define VALIDATE_ARCH                                                          \
    BPF_STMT(BPF_LD  | BPF_W   | BPF_ABS, offsetof(struct seccomp_data, arch)),\
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, AUDIT_ARCH_X86_64, 1, 0),             \
    BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL_PROCESS)

#define LOAD_NR                                                               \
    BPF_STMT(BPF_LD  | BPF_W   | BPF_ABS, offsetof(struct seccomp_data, nr))

#define ALLOW(nr)                                                             \
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, (nr), 0, 1),                          \
    BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW)

int install_seccomp(void)
{
    /* NO_NEW_PRIVS is a hard precondition for an unprivileged seccomp filter:
     * it guarantees that no execve after this point can grant new privileges
     * (e.g. via a set-uid binary), which is what lets the kernel trust a filter
     * installed by a non-root process. It is also valuable on its own. */
    if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) == -1)
        return -1;

    /* The allowlist. Grouped by purpose; each ALLOW is two BPF instructions.
     * This set is deliberately broad enough to run /bin/sh + busybox coreutils
     * comfortably, and no broader. Anything absent returns EPERM.
     *
     * Newer syscalls are guarded with #ifdef so the filter compiles on older
     * kernel headers that predate them (an undefined __NR_* would be a build
     * error inside the array). */
    struct sock_filter filter[] = {
        VALIDATE_ARCH,          /* kill anything not native x86-64             */
        LOAD_NR,                /* A = seccomp_data.nr for all comparisons     */

        /* --- process lifecycle: exec, fork, wait, exit, signals --- */
        ALLOW(__NR_execve),
        ALLOW(__NR_exit),
        ALLOW(__NR_exit_group),
        ALLOW(__NR_wait4),
        ALLOW(__NR_clone),      /* the shell forks children this way           */
        ALLOW(__NR_fork),
        ALLOW(__NR_vfork),
        ALLOW(__NR_rt_sigaction),
        ALLOW(__NR_rt_sigprocmask),
        ALLOW(__NR_rt_sigreturn),
        ALLOW(__NR_kill),
        ALLOW(__NR_tgkill),

        /* --- file I/O --- */
        ALLOW(__NR_read),
        ALLOW(__NR_write),
        ALLOW(__NR_open),
        ALLOW(__NR_openat),
        ALLOW(__NR_close),
        ALLOW(__NR_lseek),
        ALLOW(__NR_pread64),
        ALLOW(__NR_pwrite64),
        ALLOW(__NR_dup),
        ALLOW(__NR_dup2),
        ALLOW(__NR_fcntl),
        ALLOW(__NR_ioctl),      /* terminals need TIOC* ioctls to be usable    */
        ALLOW(__NR_pipe),
        ALLOW(__NR_poll),
        ALLOW(__NR_select),
        ALLOW(__NR_getdents64),
        ALLOW(__NR_getcwd),
        ALLOW(__NR_chdir),
        ALLOW(__NR_fchdir),
        ALLOW(__NR_readlink),
        ALLOW(__NR_umask),

        /* --- stat family --- */
        ALLOW(__NR_stat),
        ALLOW(__NR_fstat),
        ALLOW(__NR_lstat),
        ALLOW(__NR_newfstatat),
        ALLOW(__NR_access),

        /* --- memory management --- */
        ALLOW(__NR_mmap),
        ALLOW(__NR_munmap),
        ALLOW(__NR_mprotect),
        ALLOW(__NR_brk),

        /* --- identity / process info --- */
        ALLOW(__NR_getpid),
        ALLOW(__NR_getppid),
        ALLOW(__NR_getuid),
        ALLOW(__NR_geteuid),
        ALLOW(__NR_getgid),
        ALLOW(__NR_getegid),
        ALLOW(__NR_getpgrp),
        ALLOW(__NR_getpgid),
        ALLOW(__NR_setpgid),
        ALLOW(__NR_getsid),
        ALLOW(__NR_setsid),
        ALLOW(__NR_uname),
        ALLOW(__NR_arch_prctl), /* glibc sets the TLS base with this on startup */
        ALLOW(__NR_set_tid_address),
        ALLOW(__NR_set_robust_list),
        ALLOW(__NR_futex),      /* pthread/locking fast path                   */

        /* --- time & scheduling --- */
        ALLOW(__NR_nanosleep),
        ALLOW(__NR_clock_gettime),
        ALLOW(__NR_clock_nanosleep),
        ALLOW(__NR_gettimeofday),
        ALLOW(__NR_sched_yield),
        ALLOW(__NR_getrlimit),
        ALLOW(__NR_prlimit64),

        /* --- newer syscalls; guarded for older headers --- */
#ifdef __NR_pipe2
        ALLOW(__NR_pipe2),
#endif
#ifdef __NR_dup3
        ALLOW(__NR_dup3),
#endif
#ifdef __NR_ppoll
        ALLOW(__NR_ppoll),
#endif
#ifdef __NR_faccessat
        ALLOW(__NR_faccessat),
#endif
#ifdef __NR_faccessat2
        ALLOW(__NR_faccessat2),
#endif
#ifdef __NR_readlinkat
        ALLOW(__NR_readlinkat),
#endif
#ifdef __NR_getrandom
        ALLOW(__NR_getrandom),  /* glibc/arc4random pulls entropy here         */
#endif
#ifdef __NR_statx
        ALLOW(__NR_statx),
#endif
#ifdef __NR_rseq
        ALLOW(__NR_rseq),       /* modern glibc registers restartable seqs     */
#endif
#ifdef __NR_clone3
        ALLOW(__NR_clone3),     /* newer glibc/thread libs prefer clone3       */
#endif

        /* Default verdict: everything not allowed above. */
        BPF_STMT(BPF_RET | BPF_K, DEFAULT_ACTION),
    };

    /* sock_fprog describes the program to the kernel: instruction count + ptr.
     * len is a u16, so a filter may not exceed 65535 instructions — we are far
     * under that. */
    struct sock_fprog prog = {
        .len    = (unsigned short)(sizeof filter / sizeof filter[0]),
        .filter = filter,
    };

    /* Install it. Prefer the seccomp(2) syscall, which (unlike the older prctl
     * interface) accepts flags such as SECCOMP_FILTER_FLAG_TSYNC. We pass no
     * flags here since the payload is single-threaded at this point. If the
     * running kernel predates seccomp(2) (< 3.17) we fall back to prctl. Once
     * set, a filter can never be removed — only narrowed by stacking more. */
#ifdef SYS_seccomp
    if (syscall(SYS_seccomp, SECCOMP_SET_MODE_FILTER, 0, &prog) == 0)
        return 0;
    /* ENOSYS => kernel too old for the syscall; any other errno is a real
     * failure worth reporting to the caller. */
    if (errno != ENOSYS)
        return -1;
#endif
    if (prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog, 0, 0) == -1)
        return -1;

    return 0;
}
