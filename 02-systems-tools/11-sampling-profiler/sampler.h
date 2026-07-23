/* ===========================================================================
 * sampler.h — the contract shared by the two sampling backends and the driver.
 * ===========================================================================
 *
 * A sampling profiler has three separable jobs:
 *
 *   (1) SAMPLE   — periodically stop time and record where the program's PC is,
 *                  plus the chain of return addresses above it (the call stack).
 *   (2) AGGREGATE— collapse thousands of raw stacks into "this exact stack was
 *                  seen N times" (stackmap.c).
 *   (3) PRESENT  — fold the aggregate into the collapsed-stack text a flame graph
 *                  eats (stackmap.c again).
 *
 * This header defines the seam between (1) and (2): a backend produces callchains
 * and hands each one to a `prof_stack_fn` callback, which is stackmap_add(). Two
 * backends implement (1) two different ways:
 *
 *   - perf_sampler.c  : the kernel does the sampling and stack-walking for us via
 *                       perf_event_open + a mmap ring buffer. The production path.
 *   - sigprof_sampler.c: we do it ourselves with a SIGPROF interval timer and a
 *                        hand-rolled frame-pointer walk. No privileges required.
 *
 * Keeping the callback identical means the entire aggregation/output half of the
 * program is written once and shared. That is the point of this file.
 * =========================================================================== */
#ifndef SAMPLER_H
#define SAMPLER_H

#include <stdint.h>     /* uint64_t                                            */
#include <sys/types.h>  /* pid_t                                               */

/* Maximum call-stack depth we keep per sample. The kernel's own cap is the
 * sysctl kernel.perf_event_max_stack (default 127); 64 is more than enough for
 * teaching workloads and keeps the SIGPROF backend's preallocated sample store
 * to a sane size. Both backends clamp to this. */
#define PROF_MAX_STACK 64

/* A backend calls this once per captured sample. `ips` is the callchain with the
 * INNERMOST frame first: ips[0] is the sampled instruction pointer (the leaf),
 * ips[1] its caller, and so on outward. `n` is how many are valid (1..PROF_MAX_
 * STACK). The callee (stackmap_add) must not retain the pointer past the call —
 * the backend owns that memory. */
typedef void (*prof_stack_fn)(void *ctx, const uint64_t *ips, int n);

/* Everything the driver tells a backend about how to sample. */
struct prof_config {
    int   freq_hz;      /* target samples per second (e.g. 997 — a prime, to    */
                        /*   avoid beating against periodic workloads)          */
    int   mmap_pages;   /* perf ring: number of DATA pages, MUST be a power of   */
                        /*   two (the buffer is 1 control page + this many)      */
    pid_t target_pid;   /* 0 = profile ourselves; >0 = attach to this pid        */
    int   duration_ms;  /* attach mode only: sample this long (<=0 = until the   */
                        /*   target exits). Ignored in self mode.                */
};

/* ---------------------------------------------------------------------------
 * perf_event_open backend.
 *
 * If `workload` is non-NULL we run in SELF mode: open the event on ourselves,
 * enable it, call workload(wl_ctx), disable, and drain the ring. If `workload`
 * is NULL we run in ATTACH mode: cfg->target_pid must be set, and we poll-drain
 * the ring until the target exits or cfg->duration_ms elapses.
 *
 * Each decoded PERF_RECORD_SAMPLE is passed to on_stack(cb_ctx, ...). On return
 * *out_samples holds the number of samples delivered and *out_lost the number
 * the kernel dropped because the ring was full (PERF_RECORD_LOST). Returns 0 on
 * success, -1 on failure (a diagnostic is printed and errno is set).
 * ------------------------------------------------------------------------- */
int perf_sample_run(const struct prof_config *cfg,
                    void (*workload)(void *), void *wl_ctx,
                    prof_stack_fn on_stack, void *cb_ctx,
                    unsigned long *out_samples, unsigned long *out_lost);

/* ---------------------------------------------------------------------------
 * SIGPROF interval-timer backend (SELF mode only — it samples the thread that
 * runs the workload). Needs no special privileges, which makes it the reliable
 * fallback when perf_event_open is locked down (perf_event_paranoid >= 3).
 * Same callback and out-parameter contract as above; *out_lost counts samples
 * dropped because our fixed capture buffer filled.
 * ------------------------------------------------------------------------- */
int sigprof_sample_run(const struct prof_config *cfg,
                       void (*workload)(void *), void *wl_ctx,
                       prof_stack_fn on_stack, void *cb_ctx,
                       unsigned long *out_samples, unsigned long *out_lost);

#endif /* SAMPLER_H */
