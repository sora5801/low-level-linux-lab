/* ===========================================================================
 * main.c — a tiny grep built on the linear-time engine, plus self-test & bench.
 * ===========================================================================
 *
 * Usage:
 *     regex [OPTIONS] PATTERN [FILE...]      # print lines that match PATTERN
 *     regex --selftest                       # run built-in correctness checks
 *     regex --bench                          # demonstrate O(n*m), no ReDoS
 *
 * Options (a deliberately small, grep-compatible subset):
 *     -c   print only a COUNT of matching lines
 *     -v   inVert: print lines that do NOT match
 *     -x   match the whole line (anchored), not just a substring
 *     -n   prefix each printed line with its 1-based line number
 *     -h   help
 *
 * With no FILE, input is read from stdin. This file is the only one that talks
 * to the outside world (stdio, argv, the clock), which is why the engine proper
 * (parser/nfa/sim) stays free of I/O and — for sim.c — free of libc entirely.
 * ===========================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "regex.h"

/* ---------------------------------------------------------------------------
 * read_line — a portable getline(): read one line into a growable buffer,
 * stripping a trailing "\n" or "\r\n". Returns the line length, or -1 at EOF
 * with no data. We grow the buffer by doubling so reading N bytes total costs
 * O(N) amortized. (POSIX getline() would do, but it is not ISO C, and this lab
 * targets Linux, WSL, and the msys2 clang on Windows equally.)
 * --------------------------------------------------------------------------- */
static long read_line(FILE *f, char **buf, size_t *cap)
{
    size_t len = 0;
    int ch;
    for (;;) {
        ch = fgetc(f);
        if (ch == EOF) {
            if (len == 0) return -1;         /* nothing read: real EOF          */
            break;                           /* last line without a newline     */
        }
        if (ch == '\n') break;               /* end of line                     */

        if (len + 1 >= *cap) {               /* +1 keeps room for the NUL        */
            size_t ncap = *cap ? *cap * 2 : 128;
            char *nb = realloc(*buf, ncap);
            if (!nb) { perror("realloc"); exit(2); }
            *buf = nb; *cap = ncap;
        }
        (*buf)[len++] = (char)ch;
    }
    /* Strip a trailing '\r' so CRLF files match the same as LF files. */
    if (len > 0 && (*buf)[len - 1] == '\r') len--;
    if (*cap == 0) {                         /* empty line, buffer never grown   */
        *buf = malloc(1); *cap = 1;
        if (!*buf) { perror("malloc"); exit(2); }
    }
    (*buf)[len] = '\0';
    return (long)len;
}

/* Match one line under the chosen mode, honoring -x (full line) and -v (invert). */
static int line_matches(const Prog *p, Scratch *sc, const char *line, size_t len,
                        int full, int invert)
{
    int m = full ? re_fullmatch(p, sc, line, len)
                 : re_contains(p, sc, line, len);
    return invert ? !m : m;
}

/* Run the grep loop over one open stream; accumulate/print results. */
static long grep_stream(FILE *f, const Prog *p, Scratch *sc,
                        int opt_count, int opt_invert, int opt_full,
                        int opt_lineno, const char *fname, int show_fname)
{
    char  *buf = NULL;
    size_t cap = 0;
    long   line = 0, matches = 0;

    for (;;) {
        long len = read_line(f, &buf, &cap);
        if (len < 0) break;
        line++;
        if (line_matches(p, sc, buf, (size_t)len, opt_full, opt_invert)) {
            matches++;
            if (!opt_count) {
                if (show_fname) printf("%s:", fname);
                if (opt_lineno) printf("%ld:", line);
                printf("%s\n", buf);
            }
        }
    }
    free(buf);
    return matches;
}

/* ---------------------------------------------------------------------------
 * Self-test: a table of (pattern, text, expected fullmatch, expected contains).
 * These are the cases most likely to catch a construction or simulation bug:
 * alternation precedence, nested stars, empty matches, negated classes, the
 * pathological patterns that murder backtrackers, etc. `make test` runs this.
 * --------------------------------------------------------------------------- */
static int selftest(void)
{
    struct { const char *pat, *txt; int full, cont; } T[] = {
        /* pattern            text            full contains */
        { "abc",              "abc",            1, 1 },
        { "abc",              "xabcx",          0, 1 },
        { "abc",              "ab",             0, 0 },
        { "",                 "",               1, 1 },   /* empty pattern      */
        { "",                 "x",              0, 1 },   /* empty matches ""   */
        { "a|b",              "a",              1, 1 },
        { "a|b",              "b",              1, 1 },
        { "a|b",              "c",              0, 0 },
        { "gray|grey",        "grey",           1, 1 },
        { "a*",               "",               1, 1 },
        { "a*",               "aaaa",           1, 1 },
        { "a*",               "aaab",           0, 1 },
        { "a+",               "",               0, 0 },
        { "a+",               "aaa",            1, 1 },
        { "a?b",              "b",              1, 1 },
        { "a?b",              "ab",             1, 1 },
        { "a?b",              "aab",            0, 1 },
        { "(ab)+",            "ababab",         1, 1 },
        { "(ab)+",            "aba",            0, 1 },
        { "(a|b)*c",          "abbabc",         1, 1 },
        { "(a|b)*c",          "abbab",          0, 0 },
        { "[a-z]+",           "hello",          1, 1 },
        { "[a-z]+",           "Hello",          0, 1 },   /* 'ello' contains    */
        { "[^0-9]+",          "abc",            1, 1 },
        { "[^0-9]+",          "12",             0, 0 },
        { "[A-Za-z_][A-Za-z0-9_]*", "foo_bar9", 1, 1 },   /* an identifier      */
        { "\\d+",             "2026",           1, 1 },
        { "\\d+",             "20a6",           0, 1 },
        { "\\w+",             "id_9",           1, 1 },
        { "a.c",              "abc",            1, 1 },
        { "a.c",              "a\nc",           0, 0 },   /* '.' excludes '\n'  */
        { "colou?r",          "color",          1, 1 },
        { "colou?r",          "colour",         1, 1 },
        { "(foo|bar|baz)+",   "barbazfoo",      1, 1 },
        /* The classic catastrophic-backtracking pattern. A backtracker takes
         * exponential time on the non-matching input; we take microseconds. */
        { "(a|a)*b",          "aaaaaaaaaaaaac", 0, 0 },
        { "(a+)+$",           "aaaaaaaaaaaa$",  1, 1 },   /* '$' is a LITERAL here */
        { "(a+)+$",           "aaaaaaaaaaaa!",  0, 0 },   /* no '$' -> no match    */
    };
    int n = (int)(sizeof T / sizeof T[0]);
    int fails = 0;

    for (int i = 0; i < n; i++) {
        RegexError err;
        Prog *p = re_compile(T[i].pat, &err);
        if (!p) {
            printf("FAIL compile \"%s\": %s (at %d)\n",
                   T[i].pat, err.msg, err.offset);
            fails++;
            continue;
        }
        Scratch *sc = scratch_new(p);
        if (!sc) { printf("FAIL scratch\n"); re_free(p); fails++; continue; }

        size_t len = strlen(T[i].txt);
        int gf = re_fullmatch(p, sc, T[i].txt, len);
        int gc = re_contains (p, sc, T[i].txt, len);

        if (gf != T[i].full || gc != T[i].cont) {
            printf("FAIL /%s/ on \"%s\": full=%d(want %d) contains=%d(want %d)\n",
                   T[i].pat, T[i].txt, gf, T[i].full, gc, T[i].cont);
            fails++;
        }
        scratch_free(sc);
        re_free(p);
    }

    /* A handful of patterns that MUST fail to compile (error paths matter). */
    struct { const char *pat; } BAD[] = { {"("}, {"a)"}, {"[a"}, {"*abc"}, {"\\"} };
    for (int i = 0; i < (int)(sizeof BAD / sizeof BAD[0]); i++) {
        RegexError err;
        Prog *p = re_compile(BAD[i].pat, &err);
        if (p) {
            printf("FAIL: expected \"%s\" to be a syntax error\n", BAD[i].pat);
            re_free(p);
            fails++;
        }
    }

    if (fails == 0) printf("selftest: ALL PASSED (%d match cases)\n", n);
    else            printf("selftest: %d FAILURE(S)\n", fails);
    return fails ? 1 : 0;
}

/* ---------------------------------------------------------------------------
 * Benchmark: prove two things empirically.
 *  (1) The engine is immune to catastrophic backtracking: the pattern
 *      (a+)+$-style nemesis runs in microseconds on inputs that would hang a
 *      backtracking engine for longer than the age of the universe.
 *  (2) Runtime scales LINEARLY with input length (double n -> ~double time).
 * --------------------------------------------------------------------------- */
static double time_match(const Prog *p, Scratch *sc, const char *s, size_t n)
{
    clock_t t0 = clock();
    volatile int r = re_contains(p, sc, s, n);   /* volatile: don't optimize out */
    clock_t t1 = clock();
    (void)r;
    return (double)(t1 - t0) * 1000.0 / CLOCKS_PER_SEC;   /* milliseconds        */
}

static int bench(void)
{
    /* The ReDoS classic: nested quantifiers over the same character. A PCRE/Perl
     * backtracker explores 2^n ways to split the a's; we track a state SET, so
     * the work is linear regardless. */
    const char *pat = "(a+)+b";
    RegexError err;
    Prog *p = re_compile(pat, &err);
    if (!p) { printf("bench compile failed: %s\n", err.msg); return 1; }
    Scratch *sc = scratch_new(p);
    if (!sc) { re_free(p); return 1; }

    printf("Pattern /%s/ vs. a string of N 'a's ending in 'c' (never matches):\n", pat);
    printf("  a backtracking engine is O(2^N) here; this engine is O(N*m).\n\n");
    printf("      N      time(ms)\n");
    for (int N = 1000; N <= 512000; N *= 2) {
        char *s = malloc((size_t)N + 2);
        if (!s) break;
        memset(s, 'a', (size_t)N);
        s[N] = 'c';                          /* forces a non-match at the very end */
        s[N + 1] = '\0';
        double ms = time_match(p, sc, s, (size_t)N + 1);
        printf("  %7d   %8.3f\n", N, ms);
        free(s);
    }
    printf("\n  (Notice time roughly doubles when N doubles: linear, not exponential.)\n");

    scratch_free(sc);
    re_free(p);
    return 0;
}

static void usage(const char *argv0)
{
    fprintf(stderr,
        "usage: %s [-c] [-v] [-x] [-n] PATTERN [FILE...]\n"
        "       %s --selftest      run built-in correctness checks\n"
        "       %s --bench         show linear-time / no-ReDoS behavior\n"
        "\n"
        "  -c  print a count of matching lines\n"
        "  -v  print non-matching lines (invert)\n"
        "  -x  require the whole line to match (anchored)\n"
        "  -n  prefix matches with their line number\n",
        argv0, argv0, argv0);
}

int main(int argc, char **argv)
{
    if (argc >= 2 && strcmp(argv[1], "--selftest") == 0) return selftest();
    if (argc >= 2 && strcmp(argv[1], "--bench") == 0)    return bench();

    int opt_count = 0, opt_invert = 0, opt_full = 0, opt_lineno = 0;
    int argi = 1;

    /* Parse leading single-letter flags (possibly bundled, e.g. -cn). */
    for (; argi < argc && argv[argi][0] == '-' && argv[argi][1] != '\0'; argi++) {
        if (strcmp(argv[argi], "--") == 0) { argi++; break; }
        for (const char *f = argv[argi] + 1; *f; f++) {
            switch (*f) {
            case 'c': opt_count  = 1; break;
            case 'v': opt_invert = 1; break;
            case 'x': opt_full   = 1; break;
            case 'n': opt_lineno = 1; break;
            case 'h': usage(argv[0]); return 0;
            default:
                fprintf(stderr, "%s: unknown option -%c\n", argv[0], *f);
                usage(argv[0]);
                return 2;
            }
        }
    }

    if (argi >= argc) { usage(argv[0]); return 2; }
    const char *pattern = argv[argi++];

    /* Compile ONCE; reuse the Prog and Scratch for every line and file. */
    RegexError err;
    Prog *p = re_compile(pattern, &err);
    if (!p) {
        fprintf(stderr, "%s: bad pattern: %s (at offset %d)\n",
                argv[0], err.msg, err.offset);
        return 2;
    }
    Scratch *sc = scratch_new(p);
    if (!sc) { fprintf(stderr, "%s: out of memory\n", argv[0]); re_free(p); return 2; }

    long total = 0;
    int  rc = 1;                             /* grep convention: 1 == no matches */
    int  nfiles = argc - argi;

    if (nfiles == 0) {
        total = grep_stream(stdin, p, sc, opt_count, opt_invert, opt_full,
                            opt_lineno, "(stdin)", 0);
    } else {
        for (; argi < argc; argi++) {
            FILE *f = strcmp(argv[argi], "-") == 0 ? stdin : fopen(argv[argi], "rb");
            if (!f) { perror(argv[argi]); rc = 2; continue; }
            long m = grep_stream(f, p, sc, opt_count, opt_invert, opt_full,
                                 opt_lineno, argv[argi], nfiles > 1);
            total += m;
            if (f != stdin) fclose(f);
        }
    }

    if (opt_count) printf("%ld\n", total);
    if (total > 0 && rc == 1) rc = 0;        /* 0 == found at least one match     */

    scratch_free(sc);
    re_free(p);
    return rc;
}
