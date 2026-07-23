/* ===========================================================================
 * util.h — tiny error-handling and file-writing helpers shared by every module.
 * ===========================================================================
 * These exist because container setup is a long chain of syscalls where the
 * *only* correct response to most failures is "print why and abort" — a
 * half-constructed container is worse than none. Centralizing that keeps the
 * real logic uncluttered while still checking every return value.
 * ===========================================================================
 */
#ifndef MINIC_UTIL_H
#define MINIC_UTIL_H

/* Print "minic: <msg>: <strerror(errno)>" and exit(1). Use for unrecoverable
 * setup failures (a container that is only partly isolated must not run). */
void die(const char *msg);

/* Same message, but return instead of exiting. Use where the feature is
 * genuinely optional (e.g. rootless cgroup limits the kernel won't let us set). */
void warn(const char *msg);

/* Write the whole NUL-terminated string `data` to the file at `path`, handling
 * EINTR and short writes. Returns 0 on success, -1 (with errno set) on failure.
 *
 * This is THE primitive for the kernel's pseudo-filesystems: UID/GID maps under
 * /proc/<pid>/ and the controller files under /sys/fs/cgroup/ are all "open,
 * write the value, close". Those files reject O_APPEND and expect the value in
 * one logical write, which is why we open with plain O_WRONLY and write the
 * exact bytes with no trailing newline unless the caller included one. */
int  write_file(const char *path, const char *data);

#endif /* MINIC_UTIL_H */
