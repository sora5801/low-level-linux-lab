/* ===========================================================================
 * profiler.c — the driver: pick a backend, run it, print collapsed stacks.
 * ===========================================================================
 *
 * This ties the pieces together:
 *   - a built-in WORKLOAD with a deliberate call tree, so a self-profile has a
 *     recognisable shape to fold into a flame graph;
 *   - COMMAND-LINE parsing to choose frequency, ring size, backend, and target;
 *   - the run itself, delegating to perf_sampler.c or sigprof_sampler.c;
 *   - OUTPUT: the collapsed-stack profile goes to stdout (pipe it to
 *     flamegraph.pl); human diagnostics go to stderr so the two never mix.
 *
 * WHY THE WORKLOAD FUNCTIONS HAVE EXTERNAL LINKAGE
 * ------------------------------------------------
 * Symbolization (symbolize.c) uses dladdr, which only sees symbols exported into
 * the dynamic symbol table. `static` functions are never exported, so we give
 * the workload functions external linkage; combined with -rdynamic in the
 * Makefile, that makes them resolve to their real names ("wl_hash", ...) instead
 * of the nearest wrong symbol. This is also why the whole program is built with
 * -fno-omit-frame-pointer: both the kernel's callchain unwinder AND our SIGPROF
 * fp_walk follow the %rbp chain, which only exists if frame pointers are kept.
 * =========================================================================== */
#define _GNU_SOURCE
#include "sampler.h"
#include "stackmap.h"

#include <stdio.h>      /* fprintf, printf                                     */
#include <stdlib.h>     /* atoi, strtoul, EXIT_*                               */
#include <string.h>     /* (none directly, kept for clarity)                   */
#include <unistd.h>     /* getopt, optarg, optind                             */

/* A sink the workload feeds so the optimizer can't prove the computation is dead
 * and delete it. `volatile` forces every store to actually happen. 64-bit
 * (`long long`) so the hash math below is genuinely 64-bit on every target. */
volatile unsigned long long g_sink;

/* --- The workload: a fixed call tree. Depth and branching are chosen so the
 * folded output shows two clearly different hot paths meeting at wl_compute.
 * All are noinline (real stack frames, real call edges) and externally linked
 * (dladdr-resolvable names). ------------------------------------------------ */

/* Leaf A: an FNV-1a-style mixing loop — pure integer xor/multiply. */
__attribute__((noinline)) void wl_hash(unsigned iters)
{
    unsigned long long h = 14695981039346656037ULL;
    for (unsigned i = 0; i < iters; i++) {
        h ^= i;
        h *= 1099511628211ULL;
    }
    g_sink += h;                 /* publish so the loop isn't optimized away     */
}

/* Leaf B: a different arithmetic loop — a multiply-add hash — so it shows up as
 * a distinct leaf next to wl_hash in the flame graph. */
__attribute__((noinline)) void wl_mul(unsigned iters)
{
    unsigned long long a = 1;
    for (unsigned i = 0; i < iters; i++)
        a = a * 2654435761ULL + i;
    g_sink += a;
}

/* Two mid-level frames, each hammering one leaf. These give the flame graph its
 * middle layer (wl_heavy_a over wl_hash, wl_heavy_b over wl_mul). */
__attribute__((noinline)) void wl_heavy_a(unsigned iters)
{
    for (int k = 0; k < 8; k++)
        wl_hash(iters);
}
__attribute__((noinline)) void wl_heavy_b(unsigned iters)
{
    for (int k = 0; k < 8; k++)
        wl_mul(iters);
}

/* The root of the workload tree: alternate between the two subtrees. */
__attribute__((noinline)) void wl_compute(unsigned rounds, unsigned iters)
{
    for (unsigned r = 0; r < rounds; r++) {
        wl_heavy_a(iters);
        wl_heavy_b(iters);
    }
}

/* Adapter matching the backends' `void (*)(void *)` workload type. */
struct wlargs { unsigned rounds, iters; };
void workload_entry(void *p)
{
    struct wlargs *w = p;
    wl_compute(w->rounds, w->iters);
    /* A trailing side effect keeps the wl_compute call OUT of tail position, so
     * the compiler cannot turn it into a `jmp` that would drop workload_entry
     * from every captured stack. */
    g_sink += w->rounds;
}

static void usage(const char *argv0)
{
    fprintf(stderr,
"usage: %s [-F hz] [-m pages] [-n rounds] [-t] [-p pid] [-d ms]\n"
"\n"
"  Samples a program and prints a COLLAPSED-STACK profile to stdout, one\n"
"  \"stack count\" line per unique call path (feed it to flamegraph.pl).\n"
"\n"
"  -F hz      sampling frequency in Hz            (default 997)\n"
"  -m pages   perf ring DATA pages, power of two  (default 128 = 512 KiB;\n"
"             bounded by /proc/sys/kernel/perf_event_mlock_kb when unprivileged)\n"
"  -n rounds  built-in workload length            (default 150)\n"
"  -t         use the SIGPROF timer backend (no privileges) instead of perf\n"
"  -p pid     attach to an existing process (perf only) instead of the\n"
"             built-in workload; symbols for a foreign process are not resolved\n"
"  -d ms      attach-mode duration in ms          (default 1000)\n"
"\n"
"  With no -p, it profiles its own built-in workload so you get a full,\n"
"  self-contained flame graph with real function names.\n",
        argv0);
}

/* A power-of-two check for the ring size (the ring math relies on it). */
static int is_pow2(int v) { return v > 0 && (v & (v - 1)) == 0; }

int main(int argc, char **argv)
{
    struct prof_config cfg = {
        .freq_hz     = 997,     /* a prime: avoids aliasing with periodic work   */
        .mmap_pages  = 128,     /* 512 KiB data ring — within the default mlock   */
        .target_pid  = 0,       /* 0 = self                                       */
        .duration_ms = 1000,
    };
    int use_sigprof = 0;
    unsigned rounds = 150;
    unsigned iters  = 200000;

    int opt;
    while ((opt = getopt(argc, argv, "F:m:n:p:d:th")) != -1) {
        switch (opt) {
        case 'F': cfg.freq_hz     = atoi(optarg); break;
        case 'm': cfg.mmap_pages  = atoi(optarg); break;
        case 'n': rounds          = (unsigned)strtoul(optarg, NULL, 10); break;
        case 'p': cfg.target_pid  = (pid_t)atoi(optarg); break;
        case 'd': cfg.duration_ms = atoi(optarg); break;
        case 't': use_sigprof     = 1; break;
        case 'h': usage(argv[0]); return 0;
        default:  usage(argv[0]); return 2;
        }
    }

    if (cfg.freq_hz <= 0) {
        fprintf(stderr, "frequency (-F) must be positive\n");
        return 2;
    }
    if (!is_pow2(cfg.mmap_pages)) {
        fprintf(stderr, "ring pages (-m) must be a positive power of two\n");
        return 2;
    }

    struct stackmap *sm = stackmap_new();
    if (!sm) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    unsigned long samples = 0, lost = 0;
    int rc;

    if (cfg.target_pid > 0) {
        /* ATTACH mode: only the perf backend can sample another process. */
        if (use_sigprof) {
            fprintf(stderr,
                "-t (SIGPROF) profiles THIS process only; it cannot attach to a pid.\n");
            stackmap_free(sm);
            return 2;
        }
        fprintf(stderr, "profiling pid %d for %d ms at %d Hz (perf ring buffer)...\n",
                (int)cfg.target_pid, cfg.duration_ms, cfg.freq_hz);
        rc = perf_sample_run(&cfg, NULL, NULL, stackmap_add, sm, &samples, &lost);
    } else {
        /* SELF mode: profile the built-in workload with the chosen backend. */
        struct wlargs wl = { rounds, iters };
        if (use_sigprof) {
            fprintf(stderr,
                "self-profiling built-in workload at %d Hz (SIGPROF timer)...\n",
                cfg.freq_hz);
            rc = sigprof_sample_run(&cfg, workload_entry, &wl,
                                    stackmap_add, sm, &samples, &lost);
        } else {
            fprintf(stderr,
                "self-profiling built-in workload at %d Hz (perf ring buffer)...\n",
                cfg.freq_hz);
            rc = perf_sample_run(&cfg, workload_entry, &wl,
                                 stackmap_add, sm, &samples, &lost);
        }
    }

    if (rc != 0) {
        fprintf(stderr, "sampling failed.\n");
        stackmap_free(sm);
        return 1;
    }

    /* The PROFILE itself -> stdout (machine-readable collapsed stacks). */
    stackmap_print_folded(sm, stdout);

    /* Human summary -> stderr, so `./profiler > out.folded` captures only data. */
    fprintf(stderr, "\n-- %lu samples, %lu unique stacks, %lu lost --\n",
            samples, stackmap_unique(sm), lost);
    if (samples == 0)
        fprintf(stderr,
            "(no samples captured: workload may be too short — raise -n — or perf\n"
            " may be restricted — try -t for the SIGPROF backend.)\n");
    if (lost > 0)
        fprintf(stderr,
            "(%lu samples were dropped: the ring filled — raise -m, or lower -n.)\n",
            lost);

    stackmap_free(sm);
    return 0;
}
