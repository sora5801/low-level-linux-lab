/* ===========================================================================
 * main.c — CLI driver: parse a Brainfuck program, then interpret and/or JIT it,
 *          and compare the two for correctness and speed.
 * ===========================================================================
 *
 * Usage (see usage() below):
 *     bfjit FILE.bf            run FILE with the JIT (real stdin/stdout)
 *     bfjit --interp FILE.bf   run FILE with the interpreter instead
 *     bfjit --hello            run the built-in "Hello World!" via the JIT
 *     bfjit --bench            run the built-in benchmark on BOTH engines,
 *                              check they agree, and print the speedup
 *
 * The I/O layer is a tiny indirection so the SAME generated code and the SAME
 * interpreter can be pointed either at the real terminal or at an in-memory
 * capture buffer (used by --bench to compare the two engines byte-for-byte).
 * ===========================================================================
 */
#include "bf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>       /* clock_gettime, CLOCK_MONOTONIC */

/* ---------------------------------------------------------------------------
 * I/O back end. The bf_io callbacks have fixed signatures (no user-data arg,
 * because the JIT calls them from generated code with a plain `call`), so the
 * channel they act on is selected through file-scope state. This is the one
 * place we accept globals: the alternative (threading a context pointer into
 * generated code) would complicate the ABI for no teaching benefit here.
 * --------------------------------------------------------------------------- */

/* Output sink: if `cap_buf` is non-NULL we append to it (capture mode); else we
 * write to stdout. */
static unsigned char *cap_buf = NULL;
static size_t         cap_len = 0, cap_cap = 0;

/* Input source: a fixed byte string with a cursor (used so benchmarks are
 * reproducible and need no real stdin). If `in_data` is NULL we read stdin. */
static const unsigned char *in_data = NULL;
static size_t               in_pos  = 0, in_len = 0;

static void out_reset(void)     { free(cap_buf); cap_buf = NULL; cap_len = cap_cap = 0; }
static void out_to_stdout(void) { out_reset(); }         /* cap_buf==NULL => stdout */
static void out_to_capture(void){ out_reset(); cap_cap = 256; cap_buf = malloc(cap_cap); }

/* write_byte: the '.' back end. In capture mode, append (doubling the buffer as
 * needed); otherwise putchar to stdout. Only the low 8 bits are meaningful. */
static void io_write_byte(long v)
{
    unsigned char b = (unsigned char)v;
    if (cap_buf) {
        if (cap_len == cap_cap) {
            size_t ncap = cap_cap * 2;
            unsigned char *nb = realloc(cap_buf, ncap);
            if (!nb) { perror("realloc"); exit(1); }   /* capture must not lie */
            cap_buf = nb; cap_cap = ncap;
        }
        cap_buf[cap_len++] = b;
    } else {
        putchar((int)b);
    }
}

/* read_byte: the ',' back end. Returns the next byte, or 0 at EOF (this JIT's
 * documented end-of-input convention: the cell becomes 0). */
static long io_read_byte(void)
{
    if (in_data) {
        if (in_pos >= in_len) return 0;             /* EOF -> 0 */
        return (long)in_data[in_pos++];
    }
    int ch = getchar();
    return (ch == EOF) ? 0 : (long)(unsigned char)ch;
}

static const bf_io IO = { io_read_byte, io_write_byte };

/* ---------------------------------------------------------------------------
 * The Brainfuck tape (cell array). 30000 cells is the historical Brainfuck
 * minimum; we give a bit more headroom. It is calloc'd so every cell starts at
 * 0 as the language requires. Ownership: freed at the end of each run. Note the
 * data pointer is handed to native code, so this memory must outlive the JIT
 * call — it does, since we free only after the call returns.
 * --------------------------------------------------------------------------- */
#define TAPE_CELLS 65536

static double now_sec(void)
{
    /* CLOCK_MONOTONIC never jumps (no NTP/settimeofday effects), which is what
     * you want for measuring elapsed intervals. Resolution is nanoseconds. */
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

/* Read an entire file into a freshly malloc'd, NUL-terminated buffer. Returns
 * the byte length via *out_len, or -1 on error. */
static char *read_file(const char *path, size_t *out_len)
{
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); return NULL; }
    if (fseek(f, 0, SEEK_END) != 0) { perror("fseek"); fclose(f); return NULL; }
    long sz = ftell(f);
    if (sz < 0) { perror("ftell"); fclose(f); return NULL; }
    rewind(f);
    char *buf = malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t got = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    buf[got] = '\0';
    *out_len = got;
    return buf;
}

/* The canonical 106-byte "Hello World!" — its output is exactly the 14 bytes
 * "Hello World!\n", which we use as ground truth for the correctness check. */
static const char *HELLO =
    "++++++++[>++++[>++>+++>+++>+<<<<-]>+>+>->>+[<]<-]"
    ">>.>---.+++++++..+++.>>.<-.<.+++.------.--------.>>+.>++.";

/* Build a compute-heavy benchmark program: a triple-nested countdown. The
 * innermost body runs N*N*N times (default 200^3 = 8,000,000), which after
 * op-folding is ~40M interpreter dispatches — enough to make the JIT's win
 * obvious while finishing quickly. The program produces a small deterministic
 * output (the four counter cells, all wrapped back to 0), so comparing the two
 * engines' output still validates that both walked the tape identically.
 *
 * Structure (pointer starts at c0):
 *   c0=N [ > c1+=N [ > c2+=N [ >+ <- ] <- ] <- ]  >>>.<.<.<.  (print c3..c0)
 * Returns a malloc'd source string; caller frees. */
static char *make_bench(int N)
{
    /* Upper bound on length: 3 count-blocks of N '+' + structural chars. */
    size_t cap = (size_t)N * 3 + 64;
    char *s = malloc(cap);
    if (!s) return NULL;
    size_t k = 0;
    for (int i = 0; i < N; i++) s[k++] = '+';       /* c0 = N            */
    s[k++] = '[';   s[k++] = '>';
    for (int i = 0; i < N; i++) s[k++] = '+';       /* c1 += N           */
    s[k++] = '[';   s[k++] = '>';
    for (int i = 0; i < N; i++) s[k++] = '+';       /* c2 += N           */
    s[k++] = '[';   s[k++] = '>'; s[k++] = '+';     /* c3++              */
    s[k++] = '<';   s[k++] = '-'; s[k++] = ']';     /* c2--  (c2 loop)   */
    s[k++] = '<';   s[k++] = '-'; s[k++] = ']';     /* c1--  (c1 loop)   */
    s[k++] = '<';   s[k++] = '-'; s[k++] = ']';     /* c0--  (c0 loop)   */
    /* print c3, c2, c1, c0 (all 0 here) so both engines emit identical bytes */
    s[k++] = '>'; s[k++] = '>'; s[k++] = '>'; s[k++] = '.';
    s[k++] = '<'; s[k++] = '.'; s[k++] = '<'; s[k++] = '.'; s[k++] = '<'; s[k++] = '.';
    s[k] = '\0';
    return s;
}

/* Run a parsed program through the interpreter, capturing output. */
static double run_interp_capture(const bf_program *prog)
{
    unsigned char *tape = calloc(TAPE_CELLS, 1);
    if (!tape) { perror("calloc"); exit(1); }
    out_to_capture();
    double t0 = now_sec();
    bf_interp(prog, tape, &IO);
    double t1 = now_sec();
    free(tape);
    return t1 - t0;
}

/* Run a parsed program through the JIT, capturing output. Compilation time is
 * measured separately from execution time so the comparison is honest: a JIT
 * pays an up-front compile cost that an interpreter does not. */
static int run_jit_capture(const bf_program *prog, double *compile_s, double *exec_s)
{
    char err[128];
    jit_code jc;
    double c0 = now_sec();
    if (bf_jit_compile(prog, &jc, err, sizeof err) != 0) {
        fprintf(stderr, "jit: %s\n", err);
        return -1;
    }
    double c1 = now_sec();

    unsigned char *tape = calloc(TAPE_CELLS, 1);
    if (!tape) { perror("calloc"); bf_jit_free(&jc); exit(1); }
    out_to_capture();
    double e0 = now_sec();
    bf_jit_run(&jc, tape, &IO);
    double e1 = now_sec();

    free(tape);
    bf_jit_free(&jc);
    *compile_s = c1 - c0;
    *exec_s    = e1 - e0;
    return 0;
}

static void usage(const char *argv0)
{
    fprintf(stderr,
        "usage: %s [--jit|--interp] FILE.bf   run a program (JIT by default)\n"
        "       %s --hello                     built-in Hello World via the JIT\n"
        "       %s --bench                     benchmark JIT vs interpreter\n",
        argv0, argv0, argv0);
}

int main(int argc, char **argv)
{
    if (argc < 2) { usage(argv[0]); return 2; }

    /* ---- --bench: the headline comparison ------------------------------- */
    if (strcmp(argv[1], "--bench") == 0) {
        /* First, a correctness anchor with real output: Hello World on both
         * engines must equal the known string. */
        bf_program hp;
        char err[128];
        if (bf_parse(HELLO, strlen(HELLO), &hp, err, sizeof err) != 0) {
            fprintf(stderr, "parse hello: %s\n", err); return 1;
        }
        /* Ground truth: the canonical program prints exactly "Hello World!\n"
         * (13 bytes — 12 visible chars + newline). Derive the length from the
         * literal so the constant can never drift out of sync with the string. */
        static const char HW[] = "Hello World!\n";
        const size_t HWLEN = sizeof HW - 1;   /* 13; sizeof counts the NUL */
        run_interp_capture(&hp);
        int hello_ok_i = (cap_len == HWLEN && memcmp(cap_buf, HW, HWLEN) == 0);
        double cs, es;
        if (run_jit_capture(&hp, &cs, &es) != 0) return 1;
        int hello_ok_j = (cap_len == HWLEN && memcmp(cap_buf, HW, HWLEN) == 0);
        bf_free_program(&hp);
        printf("hello world: interp %s, jit %s\n",
               hello_ok_i ? "OK" : "WRONG", hello_ok_j ? "OK" : "WRONG");

        /* Now the heavy program. Parse once; both engines share the IR. */
        int N = 200;
        char *src = make_bench(N);
        if (!src) { fprintf(stderr, "oom building benchmark\n"); return 1; }
        bf_program bp;
        if (bf_parse(src, strlen(src), &bp, err, sizeof err) != 0) {
            fprintf(stderr, "parse bench: %s\n", err); free(src); return 1;
        }
        free(src);
        printf("benchmark: triple-nested countdown N=%d (~%.1fM inner iters), "
               "%zu ops after folding\n",
               N, (double)N*N*N / 1e6, bp.n);

        /* Interpreter run (capture output for comparison). */
        double it = run_interp_capture(&bp);
        unsigned char *iout = cap_buf; size_t ilen = cap_len;
        cap_buf = NULL; cap_len = cap_cap = 0;     /* detach so JIT run can reuse */

        /* JIT run. */
        double jc_s = 0, je_s = 0;
        if (run_jit_capture(&bp, &jc_s, &je_s) != 0) { free(iout); return 1; }
        unsigned char *jout = cap_buf; size_t jlen = cap_len;
        cap_buf = NULL; cap_len = cap_cap = 0;

        int same = (ilen == jlen) && (memcmp(iout, jout, ilen) == 0);
        free(iout); free(jout);
        bf_free_program(&bp);

        printf("output agreement: %s (%zu bytes each)\n",
               same ? "MATCH" : "DIFFER", ilen);
        printf("interpreter:  %8.4f s\n", it);
        printf("jit compile:  %8.4f s\n", jc_s);
        printf("jit execute:  %8.4f s\n", je_s);
        if (je_s > 0.0)
            printf("speedup (execute-only): %.1fx\n", it / je_s);
        return same && hello_ok_i && hello_ok_j ? 0 : 1;
    }

    /* ---- --hello: visible demo through the JIT -------------------------- */
    if (strcmp(argv[1], "--hello") == 0) {
        bf_program prog;
        char err[128];
        if (bf_parse(HELLO, strlen(HELLO), &prog, err, sizeof err) != 0) {
            fprintf(stderr, "parse: %s\n", err); return 1;
        }
        jit_code jc;
        if (bf_jit_compile(&prog, &jc, err, sizeof err) != 0) {
            fprintf(stderr, "jit: %s\n", err); bf_free_program(&prog); return 1;
        }
        unsigned char *tape = calloc(TAPE_CELLS, 1);
        if (!tape) { perror("calloc"); return 1; }
        out_to_stdout();
        bf_jit_run(&jc, tape, &IO);         /* prints "Hello World!\n" */
        free(tape);
        bf_jit_free(&jc);
        bf_free_program(&prog);
        return 0;
    }

    /* ---- default: run a file with the selected engine ------------------- */
    int use_interp = 0;
    const char *path = NULL;
    for (int i = 1; i < argc; i++) {
        if      (strcmp(argv[i], "--interp") == 0) use_interp = 1;
        else if (strcmp(argv[i], "--jit")    == 0) use_interp = 0;
        else                                       path = argv[i];
    }
    if (!path) { usage(argv[0]); return 2; }

    size_t src_len = 0;
    char *src = read_file(path, &src_len);
    if (!src) return 1;

    bf_program prog;
    char err[128];
    if (bf_parse(src, src_len, &prog, err, sizeof err) != 0) {
        fprintf(stderr, "%s: %s\n", path, err); free(src); return 1;
    }
    free(src);

    unsigned char *tape = calloc(TAPE_CELLS, 1);
    if (!tape) { perror("calloc"); bf_free_program(&prog); return 1; }
    out_to_stdout();          /* real terminal output; ',' reads real stdin  */

    if (use_interp) {
        bf_interp(&prog, tape, &IO);
    } else {
        jit_code jc;
        if (bf_jit_compile(&prog, &jc, err, sizeof err) != 0) {
            fprintf(stderr, "jit: %s\n", err);
            free(tape); bf_free_program(&prog); return 1;
        }
        bf_jit_run(&jc, tape, &IO);
        bf_jit_free(&jc);
    }

    free(tape);
    bf_free_program(&prog);
    return 0;
}
