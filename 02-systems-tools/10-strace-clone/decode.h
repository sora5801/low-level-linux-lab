/* ===========================================================================
 * decode.h — turn a raw syscall argument word into human-readable text.
 * ===========================================================================
 *
 * The tracer hands us three things per argument: the tracee's pid (so we can
 * follow pointers into its address space), the argument's declared type tag
 * (from the syscall table), and the raw 64-bit value out of the register. Our
 * job is to render that into a byte of strace-style text: a quoted string, a
 * decoded flag set, a decimal fd, and so on.
 *
 * This unit is NOT self-contained (it calls ptrace to peek tracee memory), so
 * it is Linux-only and does not ship its own teaching assembly. The *pure*
 * logic it embodies — the flag-decode table walk — is extracted verbatim into
 * asm/demo.c, which is where the annotated assembly comes from.
 * ===========================================================================
 */
#ifndef DECODE_H
#define DECODE_H

#include <sys/types.h>   /* pid_t — the one system type we take at the boundary */

/* Read a NUL-terminated string from tracee `pid` at remote address `addr` into
 * `buf` (capacity `cap`, always NUL-terminated on success). Returns the number
 * of bytes placed before the terminator, or -1 if the very first word could not
 * be read (e.g. addr is unmapped -> ptrace sets EFAULT). Short reads at a page
 * boundary return what was gathered so far, mirroring how strace copes with a
 * string that straddles the edge of mapped memory. */
long read_tracee_str(pid_t pid, unsigned long addr, char *buf, long cap);

/* Render one argument of arg_type `type` (see enum arg_type) whose raw register
 * value is `val`, writing at most `cap` bytes (always NUL-terminated) into
 * `out`. May consult the tracee (via `pid`) for A_STR. Returns the number of
 * bytes written, so callers can advance an output cursor. */
int format_arg(pid_t pid, int type, unsigned long val, char *out, int cap);

/* Render a syscall RETURN value the way strace does: a decimal (or 0x hex for
 * address-returning calls when `hex` is set), and for the -1..-4095 "negative
 * errno" range, the symbolic "-1 ENOENT (No such file or directory)" form. */
int format_retval(long ret, int hex, char *out, int cap);

/* Map a signal number to its symbolic name ("SIGSEGV"); used by the tracer to
 * report signal-delivery stops and kills. Returns "SIG_<n>" for unknowns. */
const char *signal_name(int sig);

#endif /* DECODE_H */
