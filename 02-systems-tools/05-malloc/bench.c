/* ===========================================================================
 * bench.c — a small, reproducible workload to compare our allocator to glibc.
 * ===========================================================================
 *
 * The harness only ever calls the standard names malloc/free/calloc/realloc, so
 * the SAME source runs against either allocator:
 *
 *   - link it alone           -> uses glibc's malloc           (./bench_glibc)
 *   - link it with mymalloc.c -> our malloc interposes         (./bench_mine)
 *   - LD_PRELOAD=./libmymalloc.so ./bench_glibc                -> ours, no relink
 *
 * We time three patterns that stress different parts of an allocator:
 *   1. churn  — random-size small allocs, freed in a shuffled order. Exercises the
 *               segregated free lists, splitting, and coalescing.
 *   2. lifo   — allocate N then free in reverse (stack-like). Cheapest pattern;
 *               shows the tcache/fast-path at its best.
 *   3. large  — allocations above the mmap threshold. Exercises mmap/munmap.
 *
 * Timing uses CLOCK_MONOTONIC (clock_gettime(2), immune to wall-clock jumps).
 * Randomness is a deterministic xorshift so runs are comparable and the PRNG
 * itself never allocates.
 * ===========================================================================
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#ifdef WITH_MYMALLOC
#include "mymalloc.h"   /* only for the end-of-run stats + heap check */
#endif

/* Deterministic, allocation-free PRNG (xorshift64*). */
static uint64_t g_rng = 0x9E3779B97F4A7C15ull;
static uint64_t xrand(void) {
    uint64_t x = g_rng;
    x ^= x >> 12; x ^= x << 25; x ^= x >> 27;
    g_rng = x;
    return x * 0x2545F4914F6CDD1Dull;
}

static double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

/* ---- workload 1: churn -----------------------------------------------------
 * Keep a live set of `slots` pointers. Each iteration: pick a random slot; if it
 * is empty allocate a random small size and touch the memory (so the pages are
 * really faulted in and a lazy allocator cannot cheat); if it is full, free it.
 * The touch also validates that neighbouring allocations do not overlap. */
static double bench_churn(size_t iters, size_t slots) {
    void **live = calloc(slots, sizeof(void *));
    if (!live) { perror("calloc"); exit(1); }

    double t0 = now_sec();
    for (size_t i = 0; i < iters; i++) {
        size_t s = xrand() % slots;
        if (live[s]) {
            free(live[s]);
            live[s] = NULL;
        } else {
            size_t n = 8 + (xrand() % 512);       /* 8..519 bytes */
            unsigned char *p = malloc(n);
            if (!p) { fprintf(stderr, "OOM in churn\n"); exit(1); }
            p[0] = (unsigned char)n;              /* touch first + last byte */
            p[n - 1] = (unsigned char)i;
            live[s] = p;
        }
    }
    double t1 = now_sec();

    for (size_t s = 0; s < slots; s++) free(live[s]);
    free(live);
    return t1 - t0;
}

/* ---- workload 2: lifo ------------------------------------------------------
 * Allocate `depth` blocks, then free them in reverse. Repeated `rounds` times.
 * This is the pattern the per-thread cache is built for. */
static double bench_lifo(size_t rounds, size_t depth) {
    void **stack = calloc(depth, sizeof(void *));
    if (!stack) { perror("calloc"); exit(1); }

    double t0 = now_sec();
    for (size_t r = 0; r < rounds; r++) {
        for (size_t i = 0; i < depth; i++) {
            unsigned char *p = malloc(64);
            if (!p) { fprintf(stderr, "OOM in lifo\n"); exit(1); }
            p[0] = (unsigned char)i;
            stack[i] = p;
        }
        for (size_t i = depth; i-- > 0; ) free(stack[i]);
    }
    double t1 = now_sec();

    free(stack);
    return t1 - t0;
}

/* ---- workload 3: large -----------------------------------------------------
 * Allocate/free blocks above the mmap threshold to exercise mmap/munmap. */
static double bench_large(size_t iters) {
    double t0 = now_sec();
    for (size_t i = 0; i < iters; i++) {
        size_t n = (256u * 1024u) + (xrand() % (256u * 1024u));   /* 256K..512K */
        unsigned char *p = malloc(n);
        if (!p) { fprintf(stderr, "OOM in large\n"); exit(1); }
        p[0] = 1; p[n - 1] = 2;                   /* fault first + last page */
        free(p);
    }
    double t1 = now_sec();
    return t1 - t0;
}

int main(int argc, char **argv) {
    /* Scale factor so you can dial the run time; default is a few hundred ms. */
    size_t scale = (argc > 1) ? (size_t)strtoul(argv[1], NULL, 10) : 1;
    if (scale == 0) scale = 1;

    size_t churn_iters = 2000000u * scale;
    size_t lifo_rounds = 20000u   * scale;
    size_t large_iters = 2000u    * scale;

    printf("mymalloc benchmark (scale=%zu)\n", scale);
    printf("  %-8s %12s %14s\n", "workload", "seconds", "ops/sec");

    double t;
    t = bench_churn(churn_iters, 4096);
    printf("  %-8s %12.4f %14.0f\n", "churn", t, (double)churn_iters / t);

    t = bench_lifo(lifo_rounds, 256);
    printf("  %-8s %12.4f %14.0f\n", "lifo", t, (double)(lifo_rounds * 256 * 2) / t);

    t = bench_large(large_iters);
    printf("  %-8s %12.4f %14.0f\n", "large", t, (double)large_iters / t);

#ifdef WITH_MYMALLOC
    struct my_stats st;
    my_heap_stats(&st);
    printf("\nmymalloc stats after run:\n");
    printf("  bytes_from_os = %zu\n", st.bytes_from_os);
    printf("  blocks_live   = %zu\n", st.blocks_live);
    printf("  free_blocks   = %zu\n", st.free_blocks);
    printf("  mmap_blocks   = %zu\n", st.mmap_blocks);
    printf("  heap_check    = %d (0 = consistent)\n", my_heap_check());
#endif
    return 0;
}
