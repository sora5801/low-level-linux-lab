/* ===========================================================================
 * filter.h — a tiny compiler from a tcpdump-like expression to classic BPF.
 * ===========================================================================
 *
 * "classic BPF" (cBPF) is the little accept/reject virtual machine the kernel
 * has run since 1992 to decide, per packet, whether userspace wants a copy.
 * You hand the kernel an array of `struct sock_filter` instructions via
 * setsockopt(SO_ATTACH_FILTER); for every captured frame the kernel runs that
 * program and keeps the packet only if the program returns a non-zero length.
 *
 * This header is the PUBLIC face of our miniature `tcpdump`-style compiler. It
 * turns a human string like "tcp port 80" or "host 1.2.3.4" into that byte
 * code. The gory codegen lives in filter.c; the *interpreter* that mirrors what
 * the kernel does is extracted into asm/demo.c for the assembly deliverable.
 * ===========================================================================
 */
#ifndef SNIFFER_FILTER_H
#define SNIFFER_FILTER_H

#include <linux/filter.h>   /* struct sock_filter, struct sock_fprog, BPF_* */
#include <stddef.h>

/* The kernel's classic-BPF programs are capped at BPF_MAXINSNS (4096). Our
 * generated programs are tiny (a dozen instructions at most), but we size the
 * output buffer generously and bound-check against it while emitting. */
#define FILTER_MAX_INSNS 64

/* How many bytes of each matching packet to hand up to userspace when the
 * filter ACCEPTS. cBPF signals "accept" by returning this length from the
 * program; returning 0 means "drop, don't copy". 262144 (256 KiB) is the value
 * tcpdump uses as its "capture the whole packet" sentinel. */
#define FILTER_SNAPLEN 262144u

/* Result of compiling an expression. `prog`/`len` are ready to drop into a
 * `struct sock_fprog` for SO_ATTACH_FILTER. When the expression is empty we set
 * len==0, which the caller reads as "no filter — accept everything". */
struct compiled_filter {
    struct sock_filter prog[FILTER_MAX_INSNS];
    unsigned short     len;      /* number of instructions actually emitted   */
};

/* Compile `expr` (may be NULL or "" for "match all") into `out`.
 * Returns 0 on success, -1 on a syntax error (message written to `errbuf`,
 * which must hold at least `errlen` bytes). No allocation happens — everything
 * lives in the caller-provided `out`, so there is nothing to free. */
int filter_compile(const char *expr, struct compiled_filter *out,
                   char *errbuf, size_t errlen);

/* Pretty-print the compiled program in the classic `tcpdump -d` style, one
 * instruction per line, to `fp`. Purely didactic: it lets a reader SEE the
 * accept/reject VM the string compiled down to. */
void filter_dump(const struct compiled_filter *f, void *fp /* FILE* */);

#endif /* SNIFFER_FILTER_H */
