/* ===========================================================================
 * lexer.c — turn a raw input line into a stream of tokens.
 * ===========================================================================
 *
 * The lexer is where QUOTING and '$' EXPANSION happen, because both are about
 * *character context* and that context is only known while scanning bytes:
 *
 *   'single quotes'   everything literal, no '$', no globbing
 *   "double quotes"   literal except  $VAR / ${VAR} / $? / $$  expand; \" \$ \\
 *   \x                backslash escapes one character to a literal
 *   $NAME ${NAME}     replaced by the environment value (empty if unset)
 *   ~ (at word start) replaced by $HOME
 *
 * Glob metacharacters (* ? [) are deliberately NOT expanded here — that needs
 * the filesystem and happens later in expand.c. The lexer only *records* whether
 * a word is eligible for globbing, via token.can_glob. Our rule (a documented
 * simplification of bash): a word globs iff it contains an UNQUOTED * ? or [ and
 * no part of it was quoted or escaped. So `*.c` globs, but `"*.c"`, `'*.c'`, and
 * `\*.c` are literal. bash globs the unquoted portions of a partly-quoted word;
 * doing that faithfully needs per-character quote tracking, which we skip on
 * purpose to keep the tokenizer legible.
 * ===========================================================================
 */
#define _GNU_SOURCE
#include <ctype.h>       /* isalnum                                            */
#include <stdio.h>       /* fprintf, snprintf                                  */
#include <stdlib.h>      /* getenv                                             */
#include <string.h>      /* strlen                                             */

#include "shell.h"

/* ---------------------------------------------------------------------------
 * A bounded "word builder" over a caller-supplied fixed buffer (token.text).
 * Every append checks the bound, so we can never overflow; if the word would
 * exceed TOK_MAX we set `err` and lex_next turns that into a clean -1 rather
 * than smashing the stack. This is the whole reason the lexer needs no malloc.
 * --------------------------------------------------------------------------- */
typedef struct {
    char *b;     /* buffer (points at token.text)                             */
    int   cap;   /* capacity (TOK_MAX)                                         */
    int   len;   /* bytes used so far                                         */
    int   err;   /* set once on overflow                                      */
} wb;

static void wb_putc(wb *w, char c)
{
    if (w->len < w->cap - 1)   /* leave room for the terminating NUL          */
        w->b[w->len++] = c;
    else
        w->err = 1;            /* overflow: remembered, reported by caller     */
}

static void wb_puts(wb *w, const char *s)
{
    while (*s)
        wb_putc(w, *s++);
}

/* ---------------------------------------------------------------------------
 * expand_dollar — handle a '$...' expansion starting at `p` (which points AT
 * the '$'). Appends the expanded text to `w` and returns the cursor just past
 * the consumed expression. Recognizes $NAME, ${NAME}, $?, and $$.
 *
 * Word-splitting note: a real shell re-splits the expanded value on $IFS and
 * may then glob it. We intentionally do NEITHER — the value is inserted as a
 * single literal chunk. This keeps expansion a pure string operation and is
 * documented as a teaching simplification.
 * --------------------------------------------------------------------------- */
static const char *expand_dollar(const char *p, wb *w)
{
    p++;   /* step over the '$' */

    if (*p == '{') {
        /* ${NAME} — braces let the name butt up against following text. */
        p++;
        char name[256];
        int  n = 0;
        while (*p && *p != '}' && n < (int)sizeof name - 1)
            name[n++] = *p++;
        name[n] = '\0';
        if (*p == '}')
            p++;                       /* consume the closing brace           */
        const char *val = (n > 0) ? getenv(name) : NULL;
        if (val)
            wb_puts(w, val);
        return p;
    }

    if (*p == '?') {
        /* $? — the exit status of the most recent foreground command. */
        char buf[16];
        snprintf(buf, sizeof buf, "%d", last_status);
        wb_puts(w, buf);
        return p + 1;
    }

    if (*p == '$') {
        /* $$ — the shell's own pid. */
        char buf[16];
        snprintf(buf, sizeof buf, "%d", (int)shell_pid);
        wb_puts(w, buf);
        return p + 1;
    }

    /* $NAME — a run of [A-Za-z0-9_]. A leading digit is unusual for env vars
     * but we accept the same character class either way for simplicity. */
    if (isalnum((unsigned char)*p) || *p == '_') {
        char name[256];
        int  n = 0;
        while ((isalnum((unsigned char)*p) || *p == '_') && n < (int)sizeof name - 1)
            name[n++] = *p++;
        name[n] = '\0';
        const char *val = getenv(name);
        if (val)
            wb_puts(w, val);
        return p;
    }

    /* A lone '$' (e.g. end of line or "$ ") is just a literal dollar sign. */
    wb_putc(w, '$');
    return p;
}

/* Characters that terminate an unquoted word (they begin operators or space). */
static int is_word_delim(char c)
{
    return c == '\0' || c == '\n' || c == ' ' || c == '\t' ||
           c == '|'  || c == '<'  || c == '>' || c == '&';
}

/* ---------------------------------------------------------------------------
 * lex_next — the public tokenizer. See shell.h for the contract.
 * --------------------------------------------------------------------------- */
int lex_next(const char **cursor, token *t)
{
    const char *p = *cursor;

    /* Skip inter-token whitespace. */
    while (*p == ' ' || *p == '\t')
        p++;

    t->can_glob = 0;

    /* End of line. */
    if (*p == '\0' || *p == '\n') {
        t->type   = T_EOF;
        *cursor   = p;
        return 0;
    }

    /* Operators. Order matters: check the two-character '>>' and '2>' before
     * the single-character forms, and before falling through to WORD. */
    if (*p == '|') { t->type = T_PIPE; *cursor = p + 1; return 0; }
    if (*p == '<') { t->type = T_LT;   *cursor = p + 1; return 0; }
    if (*p == '&') { t->type = T_AMP;  *cursor = p + 1; return 0; }
    if (*p == '>') {
        if (p[1] == '>') { t->type = T_GTGT; *cursor = p + 2; }
        else             { t->type = T_GT;   *cursor = p + 1; }
        return 0;
    }
    /* '2>' is only special at the START of a token (after whitespace). A '2'
     * anywhere else — e.g. inside "file2" — is an ordinary word character. */
    if (p[0] == '2' && p[1] == '>') { t->type = T_2GT; *cursor = p + 2; return 0; }

    /* Otherwise: a WORD. Accumulate characters, resolving quotes/escapes/'$'. */
    t->type = T_WORD;
    wb w = { t->text, TOK_MAX, 0, 0 };
    int quoted   = 0;   /* did any quoting/escaping happen in this word?        */
    int has_meta = 0;   /* an UNQUOTED glob metacharacter appeared?             */

    while (!is_word_delim(*p)) {
        if (*p == '\'') {
            /* Single quotes: copy verbatim until the closing quote. Nothing
             * inside is special — not '$', not '\\', not a glob metachar. */
            quoted = 1;
            p++;
            while (*p && *p != '\'')
                wb_putc(&w, *p++);
            if (*p != '\'') {
                fprintf(stderr, "shell: unterminated single quote\n");
                return -1;
            }
            p++;                       /* consume the closing '\''             */
        } else if (*p == '"') {
            /* Double quotes: '$' still expands and \" \$ \\ are escapes, but a
             * glob metacharacter inside is literal (quoted == 1 suppresses it). */
            quoted = 1;
            p++;
            while (*p && *p != '"') {
                if (*p == '\\' && (p[1] == '"' || p[1] == '\\' || p[1] == '$')) {
                    wb_putc(&w, p[1]);     /* escaped: emit the literal char    */
                    p += 2;
                } else if (*p == '$') {
                    p = expand_dollar(p, &w);
                } else {
                    wb_putc(&w, *p++);
                }
            }
            if (*p != '"') {
                fprintf(stderr, "shell: unterminated double quote\n");
                return -1;
            }
            p++;                       /* consume the closing '"'              */
        } else if (*p == '\\') {
            /* Backslash escapes the next character to a literal (glob-disabled).
             * A trailing backslash at end of line is treated as a literal '\\'. */
            quoted = 1;
            if (p[1]) { wb_putc(&w, p[1]); p += 2; }
            else      { wb_putc(&w, '\\'); p += 1; }
        } else if (*p == '$') {
            p = expand_dollar(p, &w);
        } else if (*p == '~' && w.len == 0 &&
                   (p[1] == '\0' || p[1] == '/' || p[1] == ' ' || p[1] == '\t')) {
            /* Tilde expansion: a '~' at the very start of an unquoted word,
             * followed by '/' or the word end, becomes $HOME. */
            const char *home = getenv("HOME");
            if (home) wb_puts(&w, home);
            else      wb_putc(&w, '~');
            p++;
        } else {
            /* An ordinary character. Track unquoted glob metacharacters so the
             * expander knows whether to attempt globbing on this word. */
            if (*p == '*' || *p == '?' || *p == '[')
                has_meta = 1;
            wb_putc(&w, *p++);
        }
    }

    if (w.err) {
        fprintf(stderr, "shell: word longer than %d bytes\n", TOK_MAX);
        return -1;
    }

    t->text[w.len] = '\0';
    t->can_glob    = has_meta && !quoted;   /* the documented globbing rule    */
    *cursor        = p;
    return 0;
}
