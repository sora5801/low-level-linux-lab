/* ===========================================================================
 * lex.c — turn source text into a flat array of parsed statements.
 * ===========================================================================
 *
 * This is a hand-written, line-oriented parser for the AT&T subset masm
 * accepts. There is no separate token stream: each source line is chopped in
 * place into at most a few pieces (labels, a mnemonic, comma-separated
 * operands). Keeping it line-oriented mirrors how real assemblers historically
 * worked and keeps the code short enough to read top to bottom.
 *
 * Grammar (informal), one line:
 *
 *     [ label: ]*  [ .directive args | mnemonic op1 , op2 ]   [ # comment ]
 *
 * Operand shapes:
 *     %rax                 register
 *     $123 / $0x2a         immediate (decimal or hex, optional leading '-')
 *     disp(%base)          memory:  [base + disp]
 *     sym(%rip)            RIP-relative reference to a symbol (needs a reloc)
 *     name                 a bare label (jmp/call target)
 *
 * The parser MUTATES the source buffer (inserts NULs). The caller owns the
 * buffer and keeps it alive until parsing is done.
 * ===========================================================================
 */
#include "asm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ---------------------------------------------------------------------------
 * Register name -> encoding number. The ORDER here is the x86 hardware order
 * (rax=0, rcx=1, rdx=2, rbx=3, rsp=4, rbp=5, rsi=6, rdi=7, then r8..r15),
 * NOT alphabetical. That number is what goes into the ModR/M reg/rm fields and
 * (its low 3 bits) into an opcode; the 4th bit becomes a REX.R/B bit. This
 * table is the single place that mapping is defined.
 * --------------------------------------------------------------------------- */
static const char *R64[16] = {
    "rax","rcx","rdx","rbx","rsp","rbp","rsi","rdi",
    "r8","r9","r10","r11","r12","r13","r14","r15"
};

int reg_number(const char *name)
{
    for (int i = 0; i < 16; i++)
        if (strcmp(name, R64[i]) == 0) return i;
    return -1;                    /* not a 64-bit GPR we recognise            */
}

/* A source-scoped error helper: prints "file:line: msg" and bumps a counter. */
static int err(const char *file, int line, const char *msg, const char *detail)
{
    if (detail) fprintf(stderr, "%s:%d: error: %s '%s'\n", file, line, msg, detail);
    else        fprintf(stderr, "%s:%d: error: %s\n",      file, line, msg);
    return 1;
}

/* Is `c` a legal character inside a symbol/mnemonic name? GAS allows letters,
 * digits, and _ . $ — the '.' is what lets local labels look like `.L1`. */
static int is_name_char(int c) { return isalnum(c) || c=='_' || c=='.' || c=='$'; }

/* Trim ASCII whitespace from both ends, in place; returns the new start. */
static char *trim(char *s)
{
    while (*s && isspace((unsigned char)*s)) s++;
    char *e = s + strlen(s);
    while (e > s && isspace((unsigned char)e[-1])) *--e = '\0';
    return s;
}

/* Parse a signed integer literal. strtol with base 0 accepts decimal, 0x hex,
 * and 0 octal, with an optional sign — exactly the immediate forms we allow.
 * Returns 0 on success and stores into *out; nonzero if the text is not a
 * clean number (trailing garbage is rejected so typos don't silently parse). */
static int parse_int(const char *s, int64_t *out)
{
    char *end;
    if (*s == '\0') return 1;
    /* strtoll (not strtol) so a 64-bit .quad literal survives even when the
     * host's `long` is 32-bit (Windows). Base 0 => auto-detect 0x / 0 / dec. */
    *out = (int64_t)strtoll(s, &end, 0);
    return (*end != '\0');        /* leftover chars => not a valid literal    */
}

/* ---------------------------------------------------------------------------
 * Parse ONE operand token into `op`. Returns 0 on success.
 * --------------------------------------------------------------------------- */
static int parse_operand(char *tok, Operand *op, const char *file, int line)
{
    tok = trim(tok);
    memset(op, 0, sizeof *op);

    if (tok[0] == '%') {                        /* --- register ------------- */
        int r = reg_number(tok + 1);
        if (r < 0) return err(file, line, "unknown register", tok);
        op->kind = OP_REG;
        op->reg  = r;
        return 0;
    }

    if (tok[0] == '$') {                         /* --- immediate ------------ */
        int64_t v;
        if (parse_int(tok + 1, &v)) return err(file, line, "bad immediate", tok);
        op->kind = OP_IMM;
        op->imm  = v;
        return 0;
    }

    char *lp = strchr(tok, '(');
    if (lp) {                                    /* --- memory --------------- */
        char *rp = strchr(lp, ')');
        if (!rp) return err(file, line, "missing ')' in memory operand", tok);
        *lp = '\0';                              /* split displacement | base */
        *rp = '\0';
        char *disp = trim(tok);                  /* text before '('           */
        char *base = trim(lp + 1);               /* text inside parens        */

        if (base[0] != '%')
            return err(file, line, "expected %reg base in memory operand", base);
        op->kind = OP_MEM;
        if (strcmp(base + 1, "rip") == 0) {
            op->rip = 1;                         /* RIP-relative form         */
        } else {
            int r = reg_number(base + 1);
            if (r < 0) return err(file, line, "unknown base register", base);
            op->reg = r;
        }

        if (disp[0] == '\0') {                   /* (%base): no displacement  */
            op->imm = 0;
        } else if (disp[0]=='-' || disp[0]=='+' || isdigit((unsigned char)disp[0])) {
            int64_t v;
            if (parse_int(disp, &v)) return err(file, line, "bad displacement", disp);
            op->imm = v;                         /* numeric displacement      */
        } else {                                 /* symbolic displacement     */
            if (!op->rip)
                return err(file, line, "symbol displacement needs %rip base", disp);
            op->have_sym = 1;
            snprintf(op->sym, MAXNAME, "%s", disp);
        }
        return 0;
    }

    /* --- bare name: a jmp/call/lea target symbol --------------------------- */
    if (is_name_char((unsigned char)tok[0]) && !isdigit((unsigned char)tok[0])) {
        op->kind = OP_SYM;
        snprintf(op->sym, MAXNAME, "%s", tok);
        return 0;
    }
    return err(file, line, "cannot parse operand", tok);
}

/* Split `s` on top-level commas into up to `max` trimmed pieces. Our subset
 * never puts a comma inside parentheses, so a flat split is exact. */
static int split_operands(char *s, char **out, int max)
{
    int n = 0;
    s = trim(s);
    if (*s == '\0') return 0;
    char *tok = s;
    for (char *p = s; ; p++) {
        if (*p == ',' || *p == '\0') {
            int last = (*p == '\0');
            *p = '\0';
            if (n < max) out[n++] = tok;
            tok = p + 1;
            if (last) break;
        }
    }
    return n;
}

/* Grab the next statement slot, bounds-checked. */
static Stmt *new_stmt(Stmt *out, int max, int *count, int line, StmtKind k)
{
    if (*count >= max) return NULL;
    Stmt *s = &out[*count];
    memset(s, 0, sizeof *s);
    s->kind = k;
    s->line = line;
    (*count)++;
    return s;
}

/* ---------------------------------------------------------------------------
 * Parse one already-comment-stripped line into >=0 statements.
 * --------------------------------------------------------------------------- */
static int parse_line(char *line, int lineno, const char *file,
                      Stmt *out, int max, int *count)
{
    char *p = line;

    /* Peel off any leading `label:` prefixes. A line may carry several. */
    for (;;) {
        while (*p && isspace((unsigned char)*p)) p++;
        char *start = p;
        while (is_name_char((unsigned char)*p)) p++;
        char *nameend = p;
        char *q = p;
        while (*q && isspace((unsigned char)*q)) q++;   /* look past spaces  */
        if (nameend > start && *q == ':') {
            /* It's a label definition. */
            *nameend = '\0';
            Stmt *s = new_stmt(out, max, count, lineno, STK_LABEL);
            if (!s) return err(file, lineno, "too many statements", NULL);
            snprintf(s->label, MAXNAME, "%s", start);
            p = q + 1;                            /* continue after the ':'   */
            continue;
        }
        p = start;                                /* not a label; rewind      */
        break;
    }

    while (*p && isspace((unsigned char)*p)) p++;
    if (*p == '\0') return 0;                     /* label-only or blank line */

    /* First token is a directive (starts with '.') or a mnemonic. Read the
     * run of name characters, then NUL-terminate it and let `rest` point at
     * whatever follows (the operand text, possibly empty). */
    char *mstart = p;
    while (is_name_char((unsigned char)*p)) p++;
    char *rest = p;                               /* char after the mnemonic  */
    if (mstart == rest)
        return err(file, lineno, "expected an instruction or directive", NULL);
    if (*rest != '\0') { *rest = '\0'; rest++; }  /* terminate mnemonic, advance */

    char mnem[16];
    snprintf(mnem, sizeof mnem, "%s", mstart);

    if (mnem[0] == '.') {                          /* -------- directive ----- */
        Stmt *s = new_stmt(out, max, count, lineno, STK_DIR);
        if (!s) return err(file, lineno, "too many statements", NULL);
        snprintf(s->dir, sizeof s->dir, "%s", mnem);

        if (strcmp(mnem, ".text")==0 || strcmp(mnem, ".data")==0) {
            return 0;                              /* section switch, no args  */
        }
        if (strcmp(mnem, ".globl")==0 || strcmp(mnem, ".global")==0) {
            char *arg = trim(rest);
            if (*arg == '\0') return err(file, lineno, ".globl needs a name", NULL);
            snprintf(s->dsym, MAXNAME, "%s", arg);
            snprintf(s->dir, sizeof s->dir, ".globl");   /* normalise spelling */
            return 0;
        }
        if (strcmp(mnem, ".byte")==0 || strcmp(mnem, ".quad")==0) {
            char *parts[MAX_DARGS];
            int np = split_operands(rest, parts, MAX_DARGS);
            if (np == 0) return err(file, lineno, "directive needs operands", mnem);
            for (int i = 0; i < np; i++) {
                int64_t v;
                if (parse_int(trim(parts[i]), &v))
                    return err(file, lineno, "bad numeric operand", parts[i]);
                s->dargs[s->ndargs++] = v;
            }
            return 0;
        }
        return err(file, lineno, "unknown directive", mnem);
    }

    /* -------- instruction -------- */
    Stmt *s = new_stmt(out, max, count, lineno, STK_INSN);
    if (!s) return err(file, lineno, "too many statements", NULL);
    snprintf(s->mnem, sizeof s->mnem, "%s", mnem);

    char *parts[MAX_OPS + 2];
    int np = split_operands(rest, parts, MAX_OPS + 2);
    if (np > MAX_OPS) return err(file, lineno, "too many operands", mnem);
    for (int i = 0; i < np; i++)
        if (parse_operand(parts[i], &s->ops[i], file, lineno)) return 1;
    s->nops = np;
    return 0;
}

/* ---------------------------------------------------------------------------
 * Public entry: split the whole file into lines and parse each.
 * --------------------------------------------------------------------------- */
int parse_source(const char *srcname, char *src, Stmt *out, int max, int *count)
{
    *count = 0;
    int lineno = 0;
    int errors = 0;
    char *p = src;

    while (*p) {
        /* Isolate one physical line [p, eol). */
        char *eol = p;
        while (*eol && *eol != '\n') eol++;
        char saved = *eol;
        *eol = '\0';
        lineno++;

        /* Drop a trailing '\r' (files authored on Windows use CRLF). */
        size_t L = strlen(p);
        if (L && p[L-1] == '\r') p[L-1] = '\0';

        /* Strip comments: '#' to end of line, or C++-style '//'. No strings
         * exist in this subset, so we can cut on the first occurrence. */
        for (char *c = p; *c; c++) {
            if (*c == '#' || (*c == '/' && c[1] == '/')) { *c = '\0'; break; }
        }

        if (parse_line(p, lineno, srcname, out, max, count)) errors++;

        if (saved == '\0') break;
        p = eol + 1;
    }
    return errors;
}
