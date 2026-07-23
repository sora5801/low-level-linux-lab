/* ===========================================================================
 * perf_sampler.c — sample with perf_event_open + a mmap ring buffer.
 * ===========================================================================
 *
 * This is the production-shaped backend: we ask the KERNEL to sample the program
 * and to walk its stack for us, and we read the results out of a shared memory
 * ring. perf(1) itself works exactly this way.
 *
 * THE FOUR MOVES
 * --------------
 *   1. perf_event_open(2) creates a sampling event and returns a file descriptor.
 *   2. mmap(2) on that fd maps a control page + a power-of-two data ring shared
 *      with the kernel.
 *   3. The kernel, on each sample, appends a PERF_RECORD_SAMPLE to the ring and
 *      advances a head pointer.
 *   4. We read records between our tail and the kernel's head, then publish a new
 *      tail so the kernel can reuse the space.
 *
 * There is no glibc wrapper for perf_event_open, so we invoke it through
 * syscall(2) ourselves — a good reminder that a "syscall" is just a number plus
 * arguments in registers (see 04-security-asm/01-nolibc-programs for the raw
 * mechanics).
 * =========================================================================== */
#define _GNU_SOURCE
#include "sampler.h"

#include <linux/perf_event.h>   /* struct perf_event_attr, PERF_* constants     */
#include <sys/syscall.h>        /* SYS_perf_event_open                          */
#include <sys/ioctl.h>          /* ioctl, PERF_EVENT_IOC_*                       */
#include <sys/mman.h>           /* mmap, munmap, PROT_*, MAP_*                   */
#include <unistd.h>             /* syscall, close, sysconf, read                */
#include <poll.h>               /* poll, struct pollfd                          */
#include <string.h>             /* memset, memcpy                               */
#include <errno.h>              /* errno, EINTR                                 */
#include <stdio.h>              /* fprintf, perror                              */
#include <stdlib.h>             /* malloc, free                                 */
#include <stdint.h>             /* uint64_t etc.                                */
#include <time.h>               /* clock_gettime — attach-mode deadline         */

/* Some libcs predate this constant in <sys/syscall.h>. It is ABI-stable at 298
 * on x86-64; spell it out as a fallback so the file always builds. */
#ifndef SYS_perf_event_open
# define SYS_perf_event_open 298
#endif

/* ---------------------------------------------------------------------------
 * perf_event_open — the raw syscall.
 *
 * syscall number in rax; args in rdi..r9. The kernel validates `attr` (its
 * `size` field lets it version the struct), sets up a perf_event bound to
 * (pid, cpu), and returns a NEW file descriptor. Arguments:
 *   attr      : the event description (type, sample_type, frequency, ...)
 *   pid       : 0 = the calling process; >0 = that process; -1 = all (needs CPU)
 *   cpu       : -1 = follow the task across all CPUs; >=0 = that CPU only
 *   group_fd  : -1 = this event leads its own group
 *   flags     : PERF_FLAG_FD_CLOEXEC so the fd doesn't leak across exec
 * Returns the fd, or -1 with errno (EACCES/EPERM when perf_event_paranoid is too
 * high; that is the case the SIGPROF backend exists to cover).
 * ------------------------------------------------------------------------- */
static long perf_event_open(struct perf_event_attr *attr, pid_t pid, int cpu,
                            int group_fd, unsigned long flags)
{
    return syscall(SYS_perf_event_open, attr, pid, cpu, group_fd, flags);
}

/* ===========================================================================
 * The mmap ring buffer.
 * ===========================================================================
 * Layout of the region mmap'd from the perf fd:
 *
 *     +----------------------+  <- base
 *     | perf_event_mmap_page |  page 0: the CONTROL page (metadata: data_head,
 *     |     (metadata)       |          data_tail, data_offset, data_size, ...)
 *     +----------------------+  <- base + data_offset  (normally +1 page)
 *     |                      |
 *     |    DATA ring buffer  |  the next 2^n pages: a byte ring of records
 *     |   (records, wraps)   |
 *     |                      |
 *     +----------------------+
 *
 * data_head : written by the KERNEL. Free-running count of total bytes ever
 *             produced. To find where the newest byte is, mask: head % data_size.
 * data_tail : written by US. Free-running count of total bytes ever consumed.
 * The number of unread bytes is (data_head - data_tail), unsigned, wrap-safe.
 *
 * THE MEMORY BARRIERS (the race this code prevents)
 * -------------------------------------------------
 * The kernel does, conceptually: write record bytes; then STORE-RELEASE data_head.
 * We must do the mirror: LOAD-ACQUIRE data_head; then read the record bytes.
 * The acquire load pairs with the kernel's release store so that once we observe
 * the new head, we are guaranteed to also observe the record bytes it points to.
 * Without it, a CPU could reorder our byte reads BEFORE the head read and we'd
 * parse garbage the kernel hadn't finished writing.
 *
 * Symmetrically, before we advance data_tail we must be done reading, so we
 * STORE-RELEASE data_tail. That pairs with the kernel's read of data_tail: it
 * won't overwrite bytes below the tail we publish, so it can't clobber a record
 * we're still parsing. On x86 the acquire is a plain load and the release a plain
 * store (the hardware is that strong), but __atomic_* still emits the necessary
 * COMPILER barrier so the optimizer can't reorder across them — which is the part
 * that actually bites you in practice.
 * =========================================================================== */

/* rb_read — copy `len` bytes starting at ring counter `tail` into `dst`,
 * transparently stitching across the wrap. THIS is the real-code twin of
 * asm/demo.c's rb_offset + rb_first_chunk. `mask` == data_size-1 (power of two).*/
static void rb_read(const uint8_t *data, uint64_t data_size, uint64_t mask,
                    uint64_t tail, void *dst, size_t len)
{
    uint64_t off    = tail & mask;                   /* rb_offset: counter->slot */
    uint64_t to_end = data_size - off;               /* bytes until the wrap     */
    size_t   first  = (len <= to_end) ? len : (size_t)to_end;  /* rb_first_chunk */

    memcpy(dst, data + off, first);                  /* the pre-wrap contiguous   */
    if (first < len)                                 /* record straddled the end  */
        memcpy((uint8_t *)dst + first, data, len - first);     /* the wrapped tail*/
}

/* We sample with this fixed sample_type; the record body layout below is a
 * direct consequence of it and of the FIXED field order the kernel documents in
 * <linux/perf_event.h> (the order fields appear is NOT the order you OR the bits
 * — it is the enum order). */
#define SAMPLE_TYPE (PERF_SAMPLE_IP | PERF_SAMPLE_TID | \
                     PERF_SAMPLE_TIME | PERF_SAMPLE_CALLCHAIN)

/* parse_sample — decode one PERF_RECORD_SAMPLE (already linearized into `rec`)
 * and deliver its callchain. Body layout for SAMPLE_TYPE, after the 8-byte
 * perf_event_header:
 *     u64 ip;                       // PERF_SAMPLE_IP    (the leaf PC)
 *     u32 pid; u32 tid;             // PERF_SAMPLE_TID
 *     u64 time;                     // PERF_SAMPLE_TIME
 *     u64 nr; u64 ips[nr];          // PERF_SAMPLE_CALLCHAIN
 * The callchain ips[] is innermost-first and contains CONTEXT MARKERS
 * (PERF_CONTEXT_USER, _KERNEL, ...) as sentinel values >= PERF_CONTEXT_MAX; we
 * skip those and keep only real addresses. */
static void parse_sample(const uint8_t *rec, uint32_t size,
                         prof_stack_fn on_stack, void *cb_ctx)
{
    const uint8_t *p   = rec + sizeof(struct perf_event_header);
    const uint8_t *end = rec + size;

    uint64_t ip = 0;
    if (p + 8 > end) return;  memcpy(&ip, p, 8); p += 8;   /* IP                  */
    if (p + 8 > end) return;  p += 8;                       /* TID (pid+tid): skip */
    if (p + 8 > end) return;  p += 8;                       /* TIME: skip          */

    uint64_t nr = 0;
    if (p + 8 > end) return;  memcpy(&nr, p, 8); p += 8;    /* CALLCHAIN count     */

    uint64_t ips[PROF_MAX_STACK];
    int n = 0;
    for (uint64_t i = 0; i < nr && p + 8 <= end; i++) {
        uint64_t v;
        memcpy(&v, p, 8); p += 8;
        if (v >= PERF_CONTEXT_MAX)              /* a context marker, not an addr  */
            continue;
        if (n < PROF_MAX_STACK)
            ips[n++] = v;
    }

    /* If the kernel could not unwind (e.g. the target omitted frame pointers),
     * the callchain may be empty. Fall back to the single sampled IP so the
     * sample is still attributed to a leaf rather than discarded. */
    if (n == 0 && ip)
        ips[n++] = ip;

    if (n > 0)
        on_stack(cb_ctx, ips, n);
}

/* drain — consume every complete record between tail and the kernel's head,
 * dispatch samples, tally lost records, then publish the new tail. Returns 0
 * normally, or -1 if it hit a corrupt record it could not step over. */
static int drain(struct perf_event_mmap_page *meta, const uint8_t *data,
                 uint64_t data_size, uint8_t *scratch, size_t scratch_len,
                 prof_stack_fn on_stack, void *cb_ctx,
                 unsigned long *samples, unsigned long *lost)
{
    const uint64_t mask = data_size - 1;

    /* LOAD-ACQUIRE the head: see the barrier discussion above. Everything we read
     * from the ring afterwards is guaranteed visible. */
    uint64_t head = __atomic_load_n(&meta->data_head, __ATOMIC_ACQUIRE);
    /* Our own tail; only this thread writes it, so a plain read is fine. */
    uint64_t tail = meta->data_tail;

    while (tail != head) {
        /* Read just the 8-byte header first to learn the record's type & size.
         * The header itself can straddle the wrap, so we go through rb_read. */
        struct perf_event_header hdr;
        rb_read(data, data_size, mask, tail, &hdr, sizeof hdr);

        /* Defensive: a record must be at least a header and fit our scratch. A
         * violation means the ring state is inconsistent (should never happen);
         * bail rather than loop forever or overflow scratch. */
        if (hdr.size < sizeof hdr || hdr.size > scratch_len) {
            __atomic_store_n(&meta->data_tail, head, __ATOMIC_RELEASE); /* resync */
            return -1;
        }

        /* Linearize the whole record into scratch (handles the wrap once), then
         * parse from the contiguous copy — far simpler than parsing in place. */
        rb_read(data, data_size, mask, tail, scratch, hdr.size);
        const struct perf_event_header *h = (const struct perf_event_header *)scratch;

        switch (h->type) {
        case PERF_RECORD_SAMPLE:
            parse_sample(scratch, hdr.size, on_stack, cb_ctx);
            (*samples)++;
            break;
        case PERF_RECORD_LOST: {
            /* struct: header; u64 id; u64 lost; — the kernel dropped `lost`
             * samples because we didn't drain fast enough. Count them honestly. */
            uint64_t lost_n = 0;
            memcpy(&lost_n, scratch + sizeof(struct perf_event_header) + 8, 8);
            *lost += lost_n;
            break;
        }
        default:
            /* PERF_RECORD_THROTTLE/UNTHROTTLE/COMM/etc. — not needed for a basic
             * profile; skip by size. */
            break;
        }

        tail += hdr.size;   /* records are 8-byte aligned; size includes padding */
    }

    /* STORE-RELEASE the new tail: we are done reading, so the kernel may now
     * reuse everything below `tail`. */
    __atomic_store_n(&meta->data_tail, tail, __ATOMIC_RELEASE);
    return 0;
}

/* now_ms — a monotonic millisecond clock for the attach-mode deadline. */
static long long now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

int perf_sample_run(const struct prof_config *cfg,
                    void (*workload)(void *), void *wl_ctx,
                    prof_stack_fn on_stack, void *cb_ctx,
                    unsigned long *out_samples, unsigned long *out_lost)
{
    *out_samples = 0;
    *out_lost    = 0;

    /* --- 1. Describe the event. --------------------------------------------
     * PERF_TYPE_SOFTWARE / PERF_COUNT_SW_TASK_CLOCK is a per-task software clock
     * that advances only while the task is ON-CPU. Sampling it at a fixed
     * frequency therefore places samples in proportion to where the task spends
     * CPU time — precisely what a CPU profiler wants — and, being a software
     * event, it needs no PMU and works in restricted environments (VMs, most
     * containers) where hardware "cycles" is unavailable. */
    struct perf_event_attr attr;
    memset(&attr, 0, sizeof attr);
    attr.type           = PERF_TYPE_SOFTWARE;
    attr.size           = sizeof attr;              /* struct version for the kernel */
    attr.config         = PERF_COUNT_SW_TASK_CLOCK;
    attr.sample_freq    = (unsigned long)cfg->freq_hz;
    attr.freq           = 1;                        /* sample_freq is a FREQUENCY   */
    attr.sample_type    = SAMPLE_TYPE;
    attr.disabled       = 1;                        /* start stopped; we ENABLE below*/
    attr.exclude_kernel = 1;                        /* user-space stacks only:      */
    attr.exclude_hv     = 1;                        /*   required for unprivileged  */
                                                    /*   use, and all we can        */
                                                    /*   symbolize anyway.          */
    attr.wakeup_events  = 1;                        /* wake a poll() every sample   */

    /* pid: 0 (self) in workload mode, else the attach target. cpu -1 = follow
     * the task across CPUs. */
    pid_t pid = workload ? 0 : cfg->target_pid;
    long fd = perf_event_open(&attr, pid, -1, -1, PERF_FLAG_FD_CLOEXEC);
    if (fd < 0) {
        fprintf(stderr,
            "perf_event_open: %s\n"
            "  (need /proc/sys/kernel/perf_event_paranoid <= 2 for self-profiling,\n"
            "   or <= 0 to attach to another pid; try the -t SIGPROF backend, which\n"
            "   needs no privileges.)\n",
            strerror(errno));
        return -1;
    }

    /* --- 2. Map the ring. ---------------------------------------------------
     * mmap length = (1 control page + 2^n data pages) * page_size. MAP_SHARED is
     * mandatory: the kernel writes into this mapping, so our view must be shared,
     * not a private copy. PROT_WRITE is needed because WE write data_tail. */
    long page = sysconf(_SC_PAGESIZE);
    size_t mmap_len = (size_t)(1 + cfg->mmap_pages) * (size_t)page;
    void *base = mmap(NULL, mmap_len, PROT_READ | PROT_WRITE, MAP_SHARED, (int)fd, 0);
    if (base == MAP_FAILED) {
        perror("mmap perf ring");
        close((int)fd);
        return -1;
    }

    struct perf_event_mmap_page *meta = base;
    /* data_offset/data_size were added in Linux 3.4; every kernel that supports
     * this program has them, but guard anyway: fall back to "data starts after
     * one page and spans the rest". */
    uint64_t data_off  = meta->data_offset ? meta->data_offset : (uint64_t)page;
    uint64_t data_size = meta->data_size   ? meta->data_size
                                           : (uint64_t)cfg->mmap_pages * (uint64_t)page;
    const uint8_t *data = (const uint8_t *)base + data_off;

    /* Scratch to linearize a wrapped record into. A single record cannot exceed
     * the data area; 64 KiB is comfortably larger than any callchain sample. */
    size_t scratch_len = 64 * 1024;
    uint8_t *scratch = malloc(scratch_len);
    if (!scratch) {
        perror("malloc scratch");
        munmap(base, mmap_len);
        close((int)fd);
        return -1;
    }

    /* --- 3. Run and collect. ------------------------------------------------ */
    int rc = 0;
    if (ioctl((int)fd, PERF_EVENT_IOC_RESET, 0) < 0 ||
        ioctl((int)fd, PERF_EVENT_IOC_ENABLE, 0) < 0) {
        perror("ioctl ENABLE");
        rc = -1;
        goto out;
    }

    if (workload) {
        /* SELF mode: run the code we want to profile, then stop and drain once.
         * The ring is sized (default 64 data pages = 256 KiB) to hold a short
         * run's samples; if it overflows, PERF_RECORD_LOST tells us how many we
         * missed and the summary reports it — no silent lies. */
        workload(wl_ctx);
        if (ioctl((int)fd, PERF_EVENT_IOC_DISABLE, 0) < 0)
            perror("ioctl DISABLE");            /* non-fatal: we can still drain  */
        rc = drain(meta, data, data_size, scratch, scratch_len,
                   on_stack, cb_ctx, out_samples, out_lost);
    } else {
        /* ATTACH mode: poll the fd and drain whenever the kernel wakes us
         * (wakeup_events=1) or POLLHUP tells us the target died. Stop at the
         * deadline if one was given. */
        struct pollfd pfd = { .fd = (int)fd, .events = POLLIN, .revents = 0 };
        long long deadline = cfg->duration_ms > 0 ? now_ms() + cfg->duration_ms : -1;

        for (;;) {
            int timeout = -1;
            if (deadline >= 0) {
                long long rem = deadline - now_ms();
                if (rem <= 0) break;
                timeout = (int)rem;
            }
            int pr = poll(&pfd, 1, timeout);
            if (pr < 0) {
                if (errno == EINTR) continue;   /* a signal woke poll; retry      */
                perror("poll");
                rc = -1;
                break;
            }
            /* Drain on every wakeup (and on timeout, to catch the tail). */
            if (drain(meta, data, data_size, scratch, scratch_len,
                      on_stack, cb_ctx, out_samples, out_lost) < 0) {
                rc = -1;
                break;
            }
            if (pfd.revents & POLLHUP)           /* target exited                  */
                break;
        }
        ioctl((int)fd, PERF_EVENT_IOC_DISABLE, 0);
        /* Final sweep for anything produced between the last drain and exit. */
        drain(meta, data, data_size, scratch, scratch_len,
              on_stack, cb_ctx, out_samples, out_lost);
    }

out:
    free(scratch);
    munmap(base, mmap_len);      /* unmap the shared ring                        */
    close((int)fd);              /* closing the fd tears the event down          */
    return rc;
}
