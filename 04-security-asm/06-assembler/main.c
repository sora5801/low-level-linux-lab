/* ===========================================================================
 * main.c — the masm command-line front end.
 * ===========================================================================
 *
 *   masm [options] input.s
 *     -o FILE   write the object here (default: input basename + ".o")
 *     -v        print a symbol + relocation listing (great for learning)
 *     -d        print the assembled .text bytes as hex (for eyeballing/diff)
 *
 * The pipeline is deliberately linear and easy to trace:
 *     read file  ->  parse_source()  ->  assemble()  ->  write_elf()
 * with a hard stop the moment any stage reports an error, so we never emit an
 * object built from partially-parsed input.
 * ===========================================================================
 */
#include "asm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Slurp an entire file into a NUL-terminated, caller-freed buffer. Opened in
 * BINARY mode so a source file with CRLF line endings is read byte-for-byte;
 * the lexer strips the '\r' itself. */
static char *read_file(const char *path, size_t *out_len)
{
    FILE *fp = fopen(path, "rb");
    if (!fp) { fprintf(stderr, "masm: cannot open %s\n", path); return NULL; }
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (sz < 0) { fclose(fp); return NULL; }
    char *buf = (char *)malloc((size_t)sz + 1);
    if (!buf) { fclose(fp); fprintf(stderr, "masm: out of memory\n"); return NULL; }
    size_t got = fread(buf, 1, (size_t)sz, fp);
    buf[got] = '\0';
    fclose(fp);
    if (out_len) *out_len = got;
    return buf;
}

/* Derive "foo.o" from "foo.s" (or append ".o" if there is no extension). */
static void default_output(const char *in, char *out, size_t cap)
{
    snprintf(out, cap, "%s", in);
    char *dot = strrchr(out, '.');
    char *sl1 = strrchr(out, '/');
    char *sl2 = strrchr(out, '\\');
    char *sl  = sl1 > sl2 ? sl1 : sl2;            /* last path separator        */
    if (dot && dot > sl) strcpy(dot, ".o");       /* replace extension          */
    else                 strncat(out, ".o", cap - strlen(out) - 1);
}

/* A human-readable listing of what we assembled — the "aha" view for learners. */
static void print_listing(Assembler *A)
{
    fprintf(stderr, "-- sections --\n");
    fprintf(stderr, "  .text  %zu bytes\n", A->sec[SEC_TEXT].len);
    fprintf(stderr, "  .data  %zu bytes\n", A->sec[SEC_DATA].len);

    fprintf(stderr, "-- symbols --\n");
    for (int i = 0; i < A->nsyms; i++) {
        Symbol *s = &A->syms[i];
        const char *bind = s->is_global ? "GLOBAL" : "LOCAL ";
        if (!s->defined)
            fprintf(stderr, "  %-16s %s  *UND*\n", s->name, bind);
        else
            fprintf(stderr, "  %-16s %s  %s+0x%llx\n", s->name, bind,
                    s->section == SEC_TEXT ? ".text" : ".data",
                    (unsigned long long)s->value);
    }

    fprintf(stderr, "-- relocations (.text) --\n");
    for (int i = 0; i < A->nrelocs; i++) {
        Reloc *r = &A->relocs[i];
        const char *tn = r->type == R_X86_64_PC32  ? "PC32"  :
                         r->type == R_X86_64_PLT32 ? "PLT32" :
                         r->type == R_X86_64_64    ? "64"    : "?";
        fprintf(stderr, "  0x%04llx  %-6s %s  addend %lld\n",
                (unsigned long long)r->offset, tn, r->sym, (long long)r->addend);
    }
}

/* Dump .text as hex so you can compare it, byte for byte, against
 * `objdump -d` of a GNU-as object of the same source. */
static void dump_text(Assembler *A)
{
    for (size_t i = 0; i < A->sec[SEC_TEXT].len; i++) {
        printf("%02x", A->sec[SEC_TEXT].data[i]);
        putchar((i % 16 == 15) ? '\n' : ' ');
    }
    if (A->sec[SEC_TEXT].len % 16 != 0) putchar('\n');
}

int main(int argc, char **argv)
{
    const char *input = NULL;
    char        output[1024] = {0};
    int         verbose = 0, dump = 0;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-o") && i + 1 < argc) {
            snprintf(output, sizeof output, "%s", argv[++i]);
        } else if (!strcmp(argv[i], "-v")) {
            verbose = 1;
        } else if (!strcmp(argv[i], "-d")) {
            dump = 1;
        } else if (argv[i][0] == '-' && argv[i][1] != '\0') {
            fprintf(stderr, "masm: unknown option %s\n", argv[i]);
            return 2;
        } else {
            input = argv[i];
        }
    }
    if (!input) {
        fprintf(stderr, "usage: masm [-o out.o] [-v] [-d] input.s\n");
        return 2;
    }
    if (output[0] == '\0') default_output(input, output, sizeof output);

    /* --- read + parse --- */
    size_t len;
    char *src = read_file(input, &len);
    if (!src) return 1;

    /* Stmt is a fat struct; allocate the array on the heap rather than the
     * stack. One statement per label/instruction/directive. */
    Stmt *stmts = (Stmt *)malloc(sizeof(Stmt) * MAX_STMTS);
    if (!stmts) { fprintf(stderr, "masm: out of memory\n"); free(src); return 1; }

    int count = 0;
    int perr = parse_source(input, src, stmts, MAX_STMTS, &count);

    /* --- assemble (two passes) --- */
    Assembler A;
    memset(&A, 0, sizeof A);
    A.srcname = input;
    buf_init(&A.sec[SEC_TEXT]);
    buf_init(&A.sec[SEC_DATA]);

    int aerr = perr;
    if (!perr) aerr = assemble(&A, stmts, count);

    if (verbose) print_listing(&A);
    if (dump)    dump_text(&A);

    int rc = 0;
    if (aerr) {
        fprintf(stderr, "masm: %d error(s); no object written\n", A.errors ? A.errors : perr);
        rc = 1;
    } else {
        rc = write_elf(&A, output);
        if (rc == 0) fprintf(stderr, "masm: wrote %s (.text %zu, .data %zu, %d relocs)\n",
                             output, A.sec[SEC_TEXT].len, A.sec[SEC_DATA].len, A.nrelocs);
    }

    buf_free(&A.sec[SEC_TEXT]);
    buf_free(&A.sec[SEC_DATA]);
    free(stmts);
    free(src);
    return rc;
}
