/* ===========================================================================
 * bench.c — prove the SIMD versions are CORRECT, then measure the GAP to glibc.
 * ===========================================================================
 *
 * Two jobs:
 *   1. CORRECTNESS (the important half): differential-test every SIMD variant
 *      against the scalar reference AND against glibc's own strlen/memchr/memcpy
 *      across a spread of sizes and alignments, plus a batch of tricky UTF-8
 *      vectors. Any mismatch exits non-zero, so `make test` actually gates.
 *   2. THROUGHPUT: time each variant over a big buffer and print GB/s next to
 *      glibc, so you can SEE (and the README can EXPLAIN) why rep movsb / a
 *      vectorized strlen win.
 *
 * Build & run on Linux/WSL:  make test   (correctness)   make bench   (timing)
 * ===========================================================================
 */
#include "simd_primitives.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>     /* glibc strlen / memchr / memcpy — the baseline       */
#include <time.h>       /* clock_gettime for wall-clock timing                 */

/* ---- tiny timing helper: monotonic nanoseconds --------------------------- */
static double now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}

static int g_failures = 0;
#define CHECK(cond, msg) do {                                            \
        if (!(cond)) { printf("  FAIL: %s\n", (msg)); g_failures++; }    \
    } while (0)

/* ===========================================================================
 * CORRECTNESS
 * =========================================================================== */
static void test_strlen(void)
{
    printf("[strlen] correctness vs glibc + scalar\n");
    /* A big heap buffer so we can slide the string start across all 32 possible
     * alignments — the aligned-load/first-block path is where strlen bugs hide. */
    char *buf = malloc(4096 + 64);
    for (size_t len = 0; len <= 200; len++) {
        for (int off = 0; off < 32; off++) {
            char *s = buf + 32 + off;          /* vary start alignment          */
            memset(s, 'x', len);
            s[len] = '\0';                     /* NUL-terminate at `len`        */
            size_t want = strlen(s);           /* glibc == ground truth here    */
            CHECK(want == len, "glibc strlen sanity");
            CHECK(strlen_scalar(s)      == want, "strlen_scalar");
            CHECK(strlen_sse2_intrin(s) == want, "strlen_sse2_intrin");
            CHECK(strlen_avx2_intrin(s) == want, "strlen_avx2_intrin");
            CHECK(strlen_avx2_asm(s)    == want, "strlen_avx2_asm");
        }
    }
    free(buf);
}

static void test_memchr(void)
{
    printf("[memchr] correctness vs glibc + scalar\n");
    unsigned char *buf = malloc(1024);
    for (int i = 0; i < 1024; i++) buf[i] = (unsigned char)(i * 7 + 3);
    /* Search for bytes that are present, absent, at the very end, and with n=0. */
    int targets[] = {buf[0], buf[500], buf[1023], 0xEE, 0xAB};
    size_t lens[]  = {0, 1, 15, 16, 17, 31, 32, 33, 512, 1024};
    for (size_t ti = 0; ti < sizeof targets / sizeof *targets; ti++) {
        for (size_t li = 0; li < sizeof lens / sizeof *lens; li++) {
            int c = targets[ti]; size_t n = lens[li];
            void *want = memchr(buf, c, n);            /* glibc baseline        */
            CHECK(memchr_scalar(buf, c, n)      == want, "memchr_scalar");
            CHECK(memchr_sse2_intrin(buf, c, n) == want, "memchr_sse2_intrin");
            CHECK(memchr_sse2_asm(buf, c, n)    == want, "memchr_sse2_asm");
        }
    }
    free(buf);
}

static void test_memcpy(void)
{
    printf("[memcpy] correctness vs glibc + scalar\n");
    unsigned char *src = malloc(1024), *dst = malloc(1024), *ref = malloc(1024);
    for (int i = 0; i < 1024; i++) src[i] = (unsigned char)(i ^ 0x5A);
    size_t lens[] = {0, 1, 7, 8, 15, 16, 17, 31, 32, 63, 64, 255, 1024};
    for (size_t li = 0; li < sizeof lens / sizeof *lens; li++) {
        size_t n = lens[li];
        memset(ref, 0, 1024); memcpy(ref, src, n);            /* glibc baseline */

        memset(dst, 0, 1024); memcpy_scalar(dst, src, n);
        CHECK(memcmp(dst, ref, 1024) == 0, "memcpy_scalar");
        memset(dst, 0, 1024); memcpy_sse2_intrin(dst, src, n);
        CHECK(memcmp(dst, ref, 1024) == 0, "memcpy_sse2_intrin");
        memset(dst, 0, 1024); memcpy_erms_asm(dst, src, n);
        CHECK(memcmp(dst, ref, 1024) == 0, "memcpy_erms_asm");
        memset(dst, 0, 1024); memcpy_sse_asm(dst, src, n);
        CHECK(memcmp(dst, ref, 1024) == 0, "memcpy_sse_asm");
    }
    free(src); free(dst); free(ref);
}

static void test_utf8(void)
{
    printf("[utf8] correctness of scalar vs SIMD validator\n");
    /* {bytes, length, expected}. The invalid cases exercise every rejection
     * rule: overlong, surrogate, out-of-range, truncated, stray continuation. */
    struct { const unsigned char *b; size_t n; int ok; } t[] = {
        { (const unsigned char*)"",                 0, 1 },  /* empty is valid  */
        { (const unsigned char*)"hello ascii",     11, 1 },
        { (const unsigned char*)"\xC3\xA9",          2, 1 },  /* U+00E9  e-acute */
        { (const unsigned char*)"\xE2\x82\xAC",      3, 1 },  /* U+20AC  euro    */
        { (const unsigned char*)"\xF0\x9F\x98\x80",  4, 1 },  /* U+1F600 emoji   */
        { (const unsigned char*)"\xC0\x80",          2, 0 },  /* overlong NUL    */
        { (const unsigned char*)"\xED\xA0\x80",      3, 0 },  /* surrogate D800  */
        { (const unsigned char*)"\xF4\x90\x80\x80",  4, 0 },  /* > U+10FFFF      */
        { (const unsigned char*)"\xE2\x82",          2, 0 },  /* truncated 3-byte*/
        { (const unsigned char*)"\x80",              1, 0 },  /* stray cont.     */
        { (const unsigned char*)"\xC2",              1, 0 },  /* truncated 2-byte*/
    };
    for (size_t i = 0; i < sizeof t / sizeof *t; i++) {
        int sc = utf8_validate_scalar(t[i].b, t[i].n);
        int si = utf8_validate_simd  (t[i].b, t[i].n);
        CHECK(sc == t[i].ok, "utf8_validate_scalar verdict");
        CHECK(si == t[i].ok, "utf8_validate_simd verdict");
        CHECK(sc == si,      "scalar vs SIMD agree");
    }
    /* Also validate a long ASCII-heavy buffer with one multibyte char near the
     * end, to exercise the SIMD ASCII fast path AND its scalar fall-through. */
    size_t N = 1000;
    unsigned char *big = malloc(N + 4);
    memset(big, 'A', N); big[N] = 0xE2; big[N+1] = 0x82; big[N+2] = 0xAC;
    CHECK(utf8_validate_scalar(big, N + 3) == 1, "utf8 long ascii+euro scalar");
    CHECK(utf8_validate_simd  (big, N + 3) == 1, "utf8 long ascii+euro simd");
    free(big);
}

/* ===========================================================================
 * THROUGHPUT
 * =========================================================================== */
#define BENCH(label, expr, bytes)                                        \
    do {                                                                 \
        double t0 = now_ns();                                            \
        volatile size_t sink = 0; (void)sink;                           \
        for (int r = 0; r < iters; r++) sink += (size_t)(expr);         \
        double dt = now_ns() - t0;                                       \
        double gbps = ((double)(bytes) * iters) / dt;   /* bytes/ns=GB/s*/\
        printf("  %-22s %8.2f GB/s\n", label, gbps);                    \
    } while (0)

static void bench_all(void)
{
    const size_t N = 64 * 1024 * 1024;   /* 64 MiB: comfortably out of L2/L3    */
    const int iters = 20;
    char *a = malloc(N + 64), *b = malloc(N + 64);
    memset(a, 'q', N); a[N - 1] = '\0';  /* one long C string for strlen        */
    memset(b, 0, N);

    printf("\n[throughput] N=%zu MiB, %d iterations each\n", N >> 20, iters);

    printf(" strlen:\n");
    BENCH("glibc strlen",        strlen(a),               N - 1);
    BENCH("strlen_scalar",       strlen_scalar(a),        N - 1);
    BENCH("strlen_sse2_intrin",  strlen_sse2_intrin(a),   N - 1);
    BENCH("strlen_avx2_asm",     strlen_avx2_asm(a),      N - 1);

    /* memchr: search for a byte that only appears at the very end, so every
     * variant must scan the whole buffer (worst case — the fair comparison). */
    a[N - 1] = 'Z'; a[N] = '\0';
    printf(" memchr (miss until last byte):\n");
    BENCH("glibc memchr",        memchr(a, 'Z', N),       N);
    BENCH("memchr_scalar",       memchr_scalar(a, 'Z', N),N);
    BENCH("memchr_sse2_asm",     memchr_sse2_asm(a, 'Z', N), N);

    printf(" memcpy:\n");
    BENCH("glibc memcpy",        memcpy(b, a, N),         N);
    BENCH("memcpy_scalar",       memcpy_scalar(b, a, N),  N);
    BENCH("memcpy_sse_asm",      memcpy_sse_asm(b, a, N), N);
    BENCH("memcpy_erms_asm",     memcpy_erms_asm(b, a, N),N);

    printf(" utf8 validate (all-ASCII buffer):\n");
    BENCH("utf8_validate_scalar",utf8_validate_scalar((unsigned char*)a, N), N);
    BENCH("utf8_validate_simd",  utf8_validate_simd((unsigned char*)a, N),   N);

    free(a); free(b);
}

int main(int argc, char **argv)
{
    simd_init();
    cpu_features_t f = simd_detect_features();
    printf("CPU features: sse2=%d sse42=%d avx=%d avx2=%d\n",
           f.sse2, f.sse42, f.avx, f.avx2);

    int do_bench = (argc > 1 && strcmp(argv[1], "bench") == 0);

    test_strlen();
    test_memchr();
    test_memcpy();
    test_utf8();

    if (g_failures == 0) printf("ALL CORRECTNESS TESTS PASSED\n");
    else                 printf("%d CORRECTNESS FAILURES\n", g_failures);

    if (do_bench) bench_all();

    return g_failures == 0 ? 0 : 1;
}
