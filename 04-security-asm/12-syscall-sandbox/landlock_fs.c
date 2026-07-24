/* ===========================================================================
 * landlock_fs.c — restrict the target's filesystem view with Landlock.
 * ===========================================================================
 *
 * Landlock is a Linux Security Module (since 5.13) that lets an UNPRIVILEGED
 * process sandbox itself: build a ruleset describing which filesystem
 * operations are permitted beneath which directories, then irreversibly apply
 * it to the current thread and its future children. Unlike seccomp, Landlock is
 * PATH-AWARE — it resolves the path the kernel is actually about to act on, so
 * there is no pointer-dereference problem and no TOCTOU window. That is exactly
 * the job seccomp cannot do (it only sees the pointer register) and that the
 * ptrace tracer can only do racily.
 *
 * Three syscalls, in order:
 *   1. landlock_create_ruleset(&attr, size, 0)      -> ruleset fd
 *        attr.handled_access_fs = the set of access rights this ruleset GOVERNS.
 *        Anything in this set is DENIED unless a rule re-permits it. Anything
 *        NOT in this set is out of scope (unaffected).
 *   2. landlock_add_rule(fd, LANDLOCK_RULE_PATH_BENEATH, &rule, 0)  (repeat)
 *        rule.parent_fd = an O_PATH fd of a directory; rule.allowed_access =
 *        the rights permitted beneath it.
 *   3. landlock_restrict_self(fd, 0)                 -> enforce, forever.
 *
 * COMPATIBILITY: newer kernels understand more access-right bits. If we ask to
 * "handle" a bit the running kernel does not know, create_ruleset fails with
 * EINVAL. The correct pattern (implemented below) is to query the supported ABI
 * version and mask our handled set down to what this kernel supports, so the
 * sandbox is as strict as possible AND still loads on older kernels.
 *
 * This file talks to the kernel through raw syscall numbers and hand-declared
 * structs (guarded by #ifndef) so it builds even where <linux/landlock.h> is
 * absent — and so you can see the ABI with nothing hidden.
 * ===========================================================================
 */
#define _GNU_SOURCE
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <fcntl.h>          /* open, O_PATH, O_CLOEXEC, O_DIRECTORY           */
#include <unistd.h>
#include <string.h>
#include <sys/syscall.h>

#include "sandbox.h"

/* --- Landlock ABI, declared locally so we do not depend on new headers ----- */

/* Syscall numbers are stable kernel ABI (x86-64). */
#ifndef __NR_landlock_create_ruleset
#define __NR_landlock_create_ruleset 444
#endif
#ifndef __NR_landlock_add_rule
#define __NR_landlock_add_rule       445
#endif
#ifndef __NR_landlock_restrict_self
#define __NR_landlock_restrict_self  446
#endif

/* Passed to create_ruleset(NULL, 0, LANDLOCK_CREATE_RULESET_VERSION) to learn
 * the highest ABI the kernel supports (returns a positive version number). */
#ifndef LANDLOCK_CREATE_RULESET_VERSION
#define LANDLOCK_CREATE_RULESET_VERSION (1U << 0)
#endif

/* The filesystem access-right bits. Each guards one class of operation. We list
 * them all so the "handled" set can deny broadly; the kernel masks the ones it
 * is too old to know about after we down-negotiate the ABI version. */
#ifndef LANDLOCK_ACCESS_FS_EXECUTE
#define LANDLOCK_ACCESS_FS_EXECUTE     (1ULL << 0)  /* execute a file          */
#define LANDLOCK_ACCESS_FS_WRITE_FILE  (1ULL << 1)  /* open a file for writing */
#define LANDLOCK_ACCESS_FS_READ_FILE   (1ULL << 2)  /* open a file for reading */
#define LANDLOCK_ACCESS_FS_READ_DIR    (1ULL << 3)  /* list a directory        */
#define LANDLOCK_ACCESS_FS_REMOVE_DIR  (1ULL << 4)  /* rmdir                    */
#define LANDLOCK_ACCESS_FS_REMOVE_FILE (1ULL << 5)  /* unlink                  */
#define LANDLOCK_ACCESS_FS_MAKE_CHAR   (1ULL << 6)  /* mknod char device       */
#define LANDLOCK_ACCESS_FS_MAKE_DIR    (1ULL << 7)  /* mkdir                    */
#define LANDLOCK_ACCESS_FS_MAKE_REG    (1ULL << 8)  /* create a regular file   */
#define LANDLOCK_ACCESS_FS_MAKE_SOCK   (1ULL << 9)  /* bind a unix socket      */
#define LANDLOCK_ACCESS_FS_MAKE_FIFO   (1ULL << 10) /* mkfifo                  */
#define LANDLOCK_ACCESS_FS_MAKE_BLOCK  (1ULL << 11) /* mknod block device      */
#define LANDLOCK_ACCESS_FS_MAKE_SYM    (1ULL << 12) /* symlink                 */
#endif
/* ABI 2 added REFER (rename/link across dirs); ABI 3 added TRUNCATE;
 * ABI 5 added IOCTL_DEV. Older kernels reject these bits, hence the masking. */
#ifndef LANDLOCK_ACCESS_FS_REFER
#define LANDLOCK_ACCESS_FS_REFER       (1ULL << 13)
#endif
#ifndef LANDLOCK_ACCESS_FS_TRUNCATE
#define LANDLOCK_ACCESS_FS_TRUNCATE    (1ULL << 14)
#endif
#ifndef LANDLOCK_ACCESS_FS_IOCTL_DEV
#define LANDLOCK_ACCESS_FS_IOCTL_DEV   (1ULL << 15)
#endif

#ifndef LANDLOCK_RULE_PATH_BENEATH
#define LANDLOCK_RULE_PATH_BENEATH 1
#endif

/* The two ABI structs. Both are part of the kernel/userspace contract; the
 * kernel checks the `size` we pass against the version it knows, so mismatched
 * struct sizes are detected rather than silently misread. */
struct ll_ruleset_attr {
    uint64_t handled_access_fs;   /* rights this ruleset governs (denies)      */
    uint64_t handled_access_net;  /* network rights (ABI 4+); 0 = don't handle */
};
struct ll_path_beneath_attr {
    uint64_t allowed_access;      /* rights permitted beneath parent_fd        */
    int32_t  parent_fd;           /* O_PATH fd of the directory                */
} __attribute__((packed));        /* packed: the kernel expects no tail padding*/

/* Thin syscall wrappers (no glibc wrappers exist for these on most systems). */
static long ll_create_ruleset(const struct ll_ruleset_attr *attr,
                              size_t size, uint32_t flags)
{
    return syscall(__NR_landlock_create_ruleset, attr, size, flags);
}
static long ll_add_rule(int ruleset_fd, int rule_type,
                        const void *rule_attr, uint32_t flags)
{
    return syscall(__NR_landlock_add_rule, ruleset_fd, rule_type, rule_attr, flags);
}
static long ll_restrict_self(int ruleset_fd, uint32_t flags)
{
    return syscall(__NR_landlock_restrict_self, ruleset_fd, flags);
}

/* The rights a *file* can carry (as opposed to directory-only rights like
 * MAKE_DIR). When we grant access "beneath" a directory, a plain file needs its
 * read/execute bits set on the rule too, or opening the file itself is denied. */
#define ACCESS_FILE (LANDLOCK_ACCESS_FS_EXECUTE  | \
                     LANDLOCK_ACCESS_FS_WRITE_FILE | \
                     LANDLOCK_ACCESS_FS_READ_FILE  | \
                     LANDLOCK_ACCESS_FS_TRUNCATE)

/* install_landlock — the public entry.
 *
 * We HANDLE (i.e. deny-by-default) every filesystem right the kernel knows, so
 * the target starts with zero filesystem authority, then grant READ + EXECUTE
 * beneath each path the caller allowlisted. Write/create/delete are never
 * re-granted here: the confined program can read the code/data it was pointed
 * at and nothing else, and cannot modify the filesystem at all.
 *
 * Returns 0 on success, -1 (errno) on failure. ENOSYS/EOPNOTSUPP mean the
 * kernel has no Landlock; the caller decides whether to proceed without it. */
int install_landlock(const char *const paths[], size_t n)
{
    /* Start from the full, newest-kernel handled set... */
    uint64_t handled =
        LANDLOCK_ACCESS_FS_EXECUTE     | LANDLOCK_ACCESS_FS_WRITE_FILE |
        LANDLOCK_ACCESS_FS_READ_FILE   | LANDLOCK_ACCESS_FS_READ_DIR   |
        LANDLOCK_ACCESS_FS_REMOVE_DIR  | LANDLOCK_ACCESS_FS_REMOVE_FILE|
        LANDLOCK_ACCESS_FS_MAKE_CHAR   | LANDLOCK_ACCESS_FS_MAKE_DIR   |
        LANDLOCK_ACCESS_FS_MAKE_REG    | LANDLOCK_ACCESS_FS_MAKE_SOCK  |
        LANDLOCK_ACCESS_FS_MAKE_FIFO   | LANDLOCK_ACCESS_FS_MAKE_BLOCK |
        LANDLOCK_ACCESS_FS_MAKE_SYM    | LANDLOCK_ACCESS_FS_REFER      |
        LANDLOCK_ACCESS_FS_TRUNCATE    | LANDLOCK_ACCESS_FS_IOCTL_DEV;

    /* ...then ask the kernel its ABI version and drop bits it cannot handle.
     * Without this a modern handled-set (e.g. with IOCTL_DEV) would be rejected
     * with EINVAL on a 5.13 kernel. This is the recommended forward/backward-
     * compatible negotiation. */
    long abi = ll_create_ruleset(NULL, 0, LANDLOCK_CREATE_RULESET_VERSION);
    if (abi < 0)
        return -1;                       /* no Landlock at all (ENOSYS/EINVAL) */
    if (abi < 5) handled &= ~LANDLOCK_ACCESS_FS_IOCTL_DEV;
    if (abi < 3) handled &= ~LANDLOCK_ACCESS_FS_TRUNCATE;
    if (abi < 2) handled &= ~LANDLOCK_ACCESS_FS_REFER;

    struct ll_ruleset_attr rattr = {
        .handled_access_fs  = handled,
        .handled_access_net = 0,        /* leave networking to seccomp/Landlock-net */
    };

    /* Create the ruleset. The returned fd is a handle we add rules to and then
     * "apply" to ourselves; it is close-on-exec because we do not want the
     * target to inherit an open handle to its own jailer. */
    int rs = (int)ll_create_ruleset(&rattr, sizeof(rattr), 0);
    if (rs < 0)
        return -1;

    /* Grant READ + EXECUTE beneath each allowlisted path. Directories also get
     * READ_DIR so the target may list them. We intentionally do NOT grant any
     * write/create/delete right — read-only exposure. */
    const uint64_t grant = LANDLOCK_ACCESS_FS_READ_FILE |
                           LANDLOCK_ACCESS_FS_READ_DIR  |
                           LANDLOCK_ACCESS_FS_EXECUTE;

    for (size_t i = 0; i < n; i++) {
        /* O_PATH opens the directory as a bare reference (no read/write of its
         * contents), which is all Landlock needs and the least authority we can
         * hold. O_CLOEXEC so the fd never leaks across the execve. */
        int pfd = open(paths[i], O_PATH | O_CLOEXEC);
        if (pfd < 0) {
            int e = errno;
            close(rs);
            errno = e;
            return -1;              /* a path we were told to allow is unusable */
        }
        struct ll_path_beneath_attr pb = {
            .allowed_access = grant & handled, /* never grant an unhandled bit  */
            .parent_fd      = pfd,
        };
        if (ll_add_rule(rs, LANDLOCK_RULE_PATH_BENEATH, &pb, 0) < 0) {
            int e = errno;
            close(pfd);
            close(rs);
            errno = e;
            return -1;
        }
        close(pfd);                 /* the rule captured what it needs; drop fd */
    }

    /* ENFORCE. From here on, this thread and every child are bound by the
     * ruleset. It cannot be relaxed or removed for the life of the process —
     * the same one-way property as seccomp, and the reason we install it right
     * before execve. Requires PR_SET_NO_NEW_PRIVS (set earlier by the caller). */
    if (ll_restrict_self(rs, 0) < 0) {
        int e = errno;
        close(rs);
        errno = e;
        return -1;
    }
    close(rs);
    return 0;
}
