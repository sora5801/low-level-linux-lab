/* ===========================================================================
 * util.h — error-handling and small helpers shared by every module.
 * ===========================================================================
 * Container setup is a long chain of syscalls where the *only* correct response
 * to most failures is "print why and abort" — a half-constructed container is
 * more dangerous than none (it may be missing exactly the isolation you relied
 * on). Centralizing that keeps the real logic readable while still checking
 * every return value, per CONVENTIONS.md.
 * ===========================================================================
 */
#ifndef CENG_UTIL_H
#define CENG_UTIL_H

#include <stddef.h>   /* size_t */

/* Print "ceng: <msg>: <strerror(errno)>" and exit(1). Use for unrecoverable
 * setup failures — an incompletely isolated container must never run. */
void die(const char *msg);

/* Same message, but return instead of exiting. Use where the feature is
 * genuinely optional (rootless cgroup limits, best-effort network teardown). */
void warn(const char *msg);

/* Write the whole NUL-terminated string `data` to `path`, handling EINTR and
 * short writes. Returns 0 on success, -1 (errno set) on failure.
 *
 * This is THE primitive for the kernel's pseudo-filesystems: the UID/GID maps
 * under /proc/<pid>/ and the controller files under /sys/fs/cgroup/ are all
 * "open, write the value once, close". Those files reject O_APPEND and expect
 * the value in a single logical write, so we open plain O_WRONLY and write the
 * exact bytes with no added newline. */
int  write_file(const char *path, const char *data);

/* fork()+execvp() a host command, wait for it, and return its exit status (or
 * -1 if it could not be run). Used by network.c for the iproute2/iptables L3
 * plumbing — see the honest note there about why those steps are shelled out
 * rather than re-encoded over raw netlink. `argv` is NULL-terminated. When
 * `quiet` is nonzero the child's stderr is sent to /dev/null (for the
 * "does this rule already exist?" probes that are expected to fail). */
int  run_cmd(char *const argv[], int quiet);

#endif /* CENG_UTIL_H */
