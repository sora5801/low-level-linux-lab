/* ===========================================================================
 * sigprof_sampler.c — sample with a SIGPROF interval timer + our own unwind.
 * ===========================================================================
 *
 * This backend needs NO privileges and no perf subsystem. It is both the
 * fallback for locked-down machines and the most direct illustration of what
 * "sampling" and "stack unwinding" actually mean, because WE do both by hand:
 *
 *   * setitimer(ITIMER_PROF) charges down a per-process CPU-time budget and
 *     raises SIGPROF each time it hits zero. Because ITIMER_PROF counts CPU time
 *     (user + system), not wall-clock, samples land in proportion to where the
 *     program burns CPU — the same statistical property the perf backend gets.
 *
 *   * In the handler, the kernel hands us the FULL register state of the exact
 *     instruction it interrupted, via the ucontext. We read %rip (the leaf) and
 *     %rbp (the top frame pointer) and walk the saved-%rbp chain up the stack —
 *     the identical loop annotated in asm/demo.c (fp_unwind).
 *
 * ASYNC-SIGNAL-SAFETY (the rule that shapes this whole file)
 * ---------------------------------------------------------
 * A signal handler may interrupt the main thread at ANY instruction, including
 * in the middle of malloc or printf. So the handler may call ONLY async-signal-
 * safe functions. We therefore do the absolute minimum in it: read registers,
 * read stack memory, and append to a PREALLOCATED, fixed-size array. No malloc,
 * no printf, and crucially NO dladdr — symbolization happens later, back on the
 * main path, over the captured raw addresses.
 *
 * There is no data race between the handler and main *during* sampling: signal
 * delivery suspends the interrupted thread while the handler runs on that same
 * thread, so the two never touch g_store concurrently. The only cross-boundary
 * value is g_count, which the handler publishes and main reads only AFTER it has
 * stopped the timer (a syscall, i.e. a full barrier). g_count is `volatile` so
 * the compiler cannot cache it across the handler.
 * =========================================================================== */
#define _GNU_SOURCE
#include "sampler.h"

#include <signal.h>     /* sigaction, siginfo_t                                 */
#include <sys/time.h>   /* setitimer, ITIMER_PROF                               */
#include <ucontext.h>   /* ucontext_t, mcontext gregs, REG_RIP/RBP/RSP          */
#include <stdio.h>      /* fopen/fgets (main path only), sscanf                 */
#include <stdlib.h>     /* calloc, free                                         */
#include <string.h>     /* strstr, memset                                      */
#include <stdint.h>     /* uint64_t                                             */

/* ---------------------------------------------------------------------------
 * fp_walk — the frame-pointer stack walk. This is the real, in-signal-handler
 * twin of asm/demo.c's fp_unwind (kept in sync with it deliberately). It is
 * async-signal-safe: pure loads and comparisons, no calls.
 *
 * At a frame pointer `fp` (a saved %rbp) the ABI guarantees, for code built with
 * frame pointers:  [fp+0] = caller's saved %rbp,  [fp+8] = return address.
 * Guards, all mandatory when dereferencing untrusted stack memory:
 *   - fp within [lo, hi): stay inside the mapped stack, never fault.
 *   - fp 8-byte aligned:  a real saved %rbp always is.
 *   - fp strictly increasing: stacks grow DOWN, so walking up must climb; this
 *     both terminates the loop and defuses a corrupt cyclic chain.
 * ------------------------------------------------------------------------- */
static int fp_walk(uint64_t fp, uint64_t lo, uint64_t hi, uint64_t *out, int max)
{
    int n = 0;
    while (fp >= lo && fp < hi && (fp & 7u) == 0u && n < max) {
        uint64_t saved = *(const uint64_t *)(uintptr_t)fp;         /* [fp+0]      */
        uint64_t ret   = *(const uint64_t *)(uintptr_t)(fp + 8);   /* [fp+8]      */
        out[n++] = ret;
        if (saved <= fp)        /* not climbing -> outermost frame or corruption  */
            break;
        fp = saved;
    }
    return n;
}

/* One captured sample: a callchain and its length. Fixed size so the handler
 * never allocates. */
struct sp_sample {
    uint64_t ips[PROF_MAX_STACK];
    int      n;
};

/* Handler-visible state. File-scope because a SIGPROF handler takes no user
 * argument; this is the standard (and unavoidable) shape for signal-driven
 * capture. */
static struct sp_sample   *g_store;     /* preallocated ring of captures         */
static size_t              g_cap;       /* capacity in samples                   */
static volatile size_t     g_count;     /* filled so far (handler writes, main reads) */
static volatile unsigned long g_dropped;/* samples lost after g_store filled     */
static uint64_t            g_stack_hi;  /* top of the main-thread stack, or 0     */

/* THE handler. Runs on the interrupted thread with that thread's full register
 * file available in `uctx`. Keep it tiny and allocation-free. */
static void on_sigprof(int sig, siginfo_t *si, void *uctx)
{
    (void)sig; (void)si;
    const ucontext_t *uc = (const ucontext_t *)uctx;

    /* The exact machine state at the interrupted instruction. On x86-64 these
     * gregs indices name the architectural registers; %rip is the leaf PC and
     * %rbp is the innermost frame pointer to start the walk from. */
    uint64_t rip = (uint64_t)uc->uc_mcontext.gregs[REG_RIP];
    uint64_t rbp = (uint64_t)uc->uc_mcontext.gregs[REG_RBP];
    uint64_t rsp = (uint64_t)uc->uc_mcontext.gregs[REG_RSP];

    size_t idx = g_count;
    if (idx >= g_cap) {          /* capture buffer full: count the drop, move on  */
        g_dropped++;
        return;
    }
    struct sp_sample *s = &g_store[idx];

    int n = 0;
    s->ips[n++] = rip;           /* frame 0 is the sampled instruction itself     */

    /* Bound the walk: never read below the live stack pointer (that memory may be
     * unwritten / a guard page), and never above the known stack top. If we could
     * not learn the top from /proc/self/maps, use an 8 MiB window above rsp as a
     * safe-ish fallback (documented as best-effort). */
    uint64_t lo = rsp;
    uint64_t hi = g_stack_hi ? g_stack_hi : rsp + (8ULL << 20);
    n += fp_walk(rbp, lo, hi, s->ips + n, PROF_MAX_STACK - n);

    s->n = n;
    /* Publish AFTER the sample is fully written. Same-thread ordering makes this
     * a compiler barrier in practice; volatile guarantees the store is emitted. */
    g_count = idx + 1;
}

/* Read the main thread's stack region top from /proc/self/maps. Called on the
 * MAIN path before sampling starts (fopen/fgets are not async-signal-safe, so
 * they must not be in the handler). Returns the end address of the "[stack]"
 * mapping, or 0 if it cannot be determined. */
static uint64_t read_stack_top(void)
{
    FILE *f = fopen("/proc/self/maps", "r");
    if (!f)
        return 0;
    char line[512];
    uint64_t hi = 0;
    while (fgets(line, sizeof line, f)) {
        if (strstr(line, "[stack]")) {
            unsigned long start, end;
            /* maps lines look like:  7ffe...000-7ffe...000 rw-p ... [stack] */
            if (sscanf(line, "%lx-%lx", &start, &end) == 2)
                hi = (uint64_t)end;
            break;
        }
    }
    fclose(f);
    return hi;
}

int sigprof_sample_run(const struct prof_config *cfg,
                       void (*workload)(void *), void *wl_ctx,
                       prof_stack_fn on_stack, void *cb_ctx,
                       unsigned long *out_samples, unsigned long *out_lost)
{
    *out_samples = 0;
    *out_lost    = 0;

    g_stack_hi = read_stack_top();

    /* Size the capture buffer for ~60 s at the target rate, clamped so we never
     * reserve an absurd amount. Each sample is PROF_MAX_STACK*8 + a little, so
     * 32768 samples is ~17 MiB — plenty for a demo run; overflow is reported via
     * g_dropped rather than silently truncated. */
    size_t want = (size_t)(cfg->freq_hz > 0 ? cfg->freq_hz : 1000) * 60;
    if (want < 4096)  want = 4096;
    if (want > 32768) want = 32768;
    g_cap     = want;
    g_count   = 0;
    g_dropped = 0;
    g_store   = calloc(g_cap, sizeof *g_store);
    if (!g_store) {
        perror("calloc sample store");
        return -1;
    }

    /* Install the SIGPROF handler. SA_SIGINFO gives us the ucontext (the whole
     * point); SA_RESTART transparently restarts syscalls the signal interrupts
     * so the workload isn't perturbed by spurious EINTR. */
    struct sigaction sa, old;
    memset(&sa, 0, sizeof sa);
    sa.sa_sigaction = on_sigprof;
    sa.sa_flags     = SA_SIGINFO | SA_RESTART;
    sigemptyset(&sa.sa_mask);   /* SIGPROF is auto-blocked inside its own handler */
    if (sigaction(SIGPROF, &sa, &old) < 0) {
        perror("sigaction SIGPROF");
        free(g_store);
        g_store = NULL;
        return -1;
    }

    /* Arm the interval timer. Period = 1e6 / freq microseconds; ITIMER_PROF
     * decrements while the process consumes CPU time and fires SIGPROF at 0.
     * Setting both it_value (first fire) and it_interval (subsequent) makes it
     * repeat. */
    long usec = 1000000L / (cfg->freq_hz > 0 ? cfg->freq_hz : 1000);
    if (usec < 1) usec = 1;
    struct itimerval it;
    it.it_value.tv_sec     = usec / 1000000;
    it.it_value.tv_usec    = usec % 1000000;
    it.it_interval.tv_sec  = usec / 1000000;
    it.it_interval.tv_usec = usec % 1000000;

    int rc = 0;
    if (setitimer(ITIMER_PROF, &it, NULL) < 0) {
        perror("setitimer");
        rc = -1;
        goto restore;
    }

    /* Run the code under study. SIGPROF fires throughout, filling g_store. */
    if (workload)
        workload(wl_ctx);

    /* Disarm: a zero itimerval stops the timer so no more signals arrive. */
    {
        struct itimerval stop;
        memset(&stop, 0, sizeof stop);
        setitimer(ITIMER_PROF, &stop, NULL);
    }

restore:
    sigaction(SIGPROF, &old, NULL);     /* put the previous handler back          */

    /* Now, safely off the signal path, hand every captured stack to the
     * aggregator (which symbolizes with dladdr — fine here, forbidden in the
     * handler). */
    size_t got = g_count;
    for (size_t i = 0; i < got; i++)
        on_stack(cb_ctx, g_store[i].ips, g_store[i].n);

    *out_samples = (unsigned long)got;
    *out_lost    = g_dropped;

    free(g_store);
    g_store = NULL;
    return rc;
}
