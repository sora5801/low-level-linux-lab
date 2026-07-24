/* ===========================================================================
 * parse.c — turn makefile TEXT into variables and rules, and expand $() refs.
 * ===========================================================================
 *
 * The makefile grammar we accept (a teaching subset of GNU make):
 *
 *   # comment                                 -> ignored (to end of line)
 *   NAME = value        (recursive/lazy)      -> variable, expanded at use
 *   NAME := value       (simple/immediate)    -> variable, expanded now
 *   NAME ?= value       (conditional)         -> set only if NAME is unset
 *   NAME += value       (append)              -> append to NAME
 *   target... : prereq... -> a rule; following TAB-indented lines are its recipe
 *   .PHONY: name...                           -> mark names as always-stale
 *
 * Two make features that make parsing subtle and that we handle here:
 *   1. LINE CONTINUATION: a backslash at end of line joins with the next line.
 *   2. TABS ARE SIGNIFICANT: a line beginning with a TAB is a recipe line for
 *      the rule currently being defined — never a variable or another rule.
 *
 * Deliberately OMITTED (documented in the README): pattern rules (%.o: %.c),
 * built-in implicit rules, functions like $(wildcard)/$(patsubst), conditionals
 * (ifeq/ifdef), include, and target-specific variables. This is a core, not a
 * clone of every corner of GNU make.
 * ===========================================================================
 */
#include "mk.h"

#include <string.h>   /* strchr, strcmp, memmove, memset, strlen             */
#include <ctype.h>    /* isspace                                             */
#include <stdlib.h>   /* free                                                */

/* ===========================================================================
 * VARIABLE TABLE
 * ===========================================================================
 */

/* Linear lookup. A real make uses a hash table; for the handful of variables in
 * a teaching makefile, O(n) scanning is simpler and plenty fast. */
variable *mk_var_find(mk *m, const char *name)
{
    for (size_t i = 0; i < m->nvar; i++)
        if (strcmp(m->vars[i].name, name) == 0)
            return &m->vars[i];
    return NULL;
}

/* Grow the variable array geometrically when full. */
static variable *var_alloc(mk *m)
{
    if (m->nvar == m->varcap) {
        m->varcap = m->varcap ? m->varcap * 2 : 8;
        m->vars   = xrealloc(m->vars, m->varcap * sizeof *m->vars);
    }
    return &m->vars[m->nvar++];
}

/* Set NAME to VALUE with the given flavor. For SIMPLE (`:=`) the caller has
 * already expanded VALUE. Re-setting an existing variable overwrites it, freeing
 * the old value string so we do not leak. */
void mk_var_set(mk *m, const char *name, const char *value, var_flavor fl)
{
    variable *v = mk_var_find(m, name);
    if (v) {
        free(v->value);                /* release the previous binding          */
        v->value  = xstrdup(value);
        v->flavor = fl;
        return;
    }
    v = var_alloc(m);
    v->name   = xstrdup(name);
    v->value  = xstrdup(value);
    v->flavor = fl;
}

/* ===========================================================================
 * VARIABLE EXPANSION:  "$(CC) -c $<"  ->  "clang -c foo.c"
 * ===========================================================================
 * We support:
 *   $$              -> a literal '$'
 *   $(NAME) ${NAME} -> the value of variable NAME (recursively expanded)
 *   $X              -> single-character variable NAME (rare but legal)
 *   $@ $< $^        -> automatic variables, from the recipe's autoctx
 *
 * Recursion: expanding a RECURSIVE variable may itself contain $() references,
 * so mk_expand calls itself on the variable's value. `depth` guards against a
 * self-referential definition like `A = $(A)` blowing the stack.
 */
#define EXPAND_MAX_DEPTH 100

static void expand_into(mk *m, const char *text, const autoctx *ctx,
                        strbuf *out, int depth);

/* Expand a single variable reference whose NAME is [name, name+len) into `out`.
 * Handles automatic variables first (they are not in the variable table), then
 * ordinary variables (recursively re-expanding recursive-flavored values). */
static void expand_var(mk *m, const char *name, size_t len, const autoctx *ctx,
                       strbuf *out, int depth)
{
    /* Automatic variables: valid only when we have a recipe context. */
    if (len == 1 && ctx) {
        const char *val = NULL;
        if      (name[0] == '@') val = ctx->target;
        else if (name[0] == '<') val = ctx->first;
        else if (name[0] == '^') val = ctx->all;
        if (val) { sb_addstr(out, val); return; }
        /* $@/$</$^ with no value (e.g. no prereqs) expand to empty. */
        if (name[0] == '@' || name[0] == '<' || name[0] == '^') return;
    }

    char     *key = xstrndup(name, len);
    variable *v   = mk_var_find(m, key);
    free(key);

    if (!v) return;                    /* undefined variable -> empty, like make */

    if (v->flavor == VAR_SIMPLE) {
        sb_addstr(out, v->value);      /* already fully expanded at definition   */
    } else {
        /* Recursive flavor: the stored text may contain more $() to expand. */
        expand_into(m, v->value, ctx, out, depth + 1);
    }
}

/* The expansion workhorse: scan `text`, copying literal bytes and resolving
 * each '$' construct into `out`. */
static void expand_into(mk *m, const char *text, const autoctx *ctx,
                        strbuf *out, int depth)
{
    if (depth > EXPAND_MAX_DEPTH)
        die("variable expansion too deep (recursive definition?)");

    for (const char *p = text; *p; ) {
        if (*p != '$') {               /* ordinary byte: copy verbatim          */
            sb_addch(out, *p++);
            continue;
        }
        p++;                           /* consume the '$'                       */
        if (*p == '\0') { sb_addch(out, '$'); break; } /* trailing '$'          */

        if (*p == '$') {               /* "$$" is an escaped literal '$'        */
            sb_addch(out, '$');
            p++;
        } else if (*p == '(' || *p == '{') {
            /* $(NAME) or ${NAME}: name runs to the first matching close. We do
             * NOT support nested parens or make functions here. */
            char open  = *p;
            char close = (open == '(') ? ')' : '}';
            p++;                       /* step past the opener                  */
            const char *start = p;
            while (*p && *p != close) p++;
            if (*p != close)
                die("unterminated variable reference: missing '%c'", close);
            expand_var(m, start, (size_t)(p - start), ctx, out, depth);
            p++;                       /* step past the closer                  */
        } else {
            /* $X : the single next character is the variable name. */
            expand_var(m, p, 1, ctx, out, depth);
            p++;
        }
    }
}

/* Public entry point: return a freshly-allocated expansion of `text`. */
char *mk_expand(mk *m, const char *text, const autoctx *ctx)
{
    strbuf sb;
    sb_init(&sb);
    expand_into(m, text, ctx, &sb, 0);
    return sb_detach(&sb);             /* caller owns the result                */
}

/* ===========================================================================
 * SMALL STRING HELPERS
 * ===========================================================================
 */

/* Append a heap string to a growable char* array. */
static void push_str(char ***arr, size_t *n, size_t *cap, char *s)
{
    if (*n == *cap) {
        *cap = *cap ? *cap * 2 : 4;
        *arr = xrealloc(*arr, *cap * sizeof **arr);
    }
    (*arr)[(*n)++] = s;
}

/* Split `s` on runs of whitespace, pushing each xstrdup'd word into arr. Used
 * for both target lists and prerequisite lists. */
static void split_words(const char *s, char ***arr, size_t *n, size_t *cap)
{
    while (*s) {
        while (*s && isspace((unsigned char)*s)) s++;     /* skip separators    */
        if (!*s) break;
        const char *start = s;
        while (*s && !isspace((unsigned char)*s)) s++;    /* the word           */
        push_str(arr, n, cap, xstrndup(start, (size_t)(s - start)));
    }
}

/* Trim leading and trailing ASCII whitespace IN PLACE, returning the new start.
 * Writes a NUL to chop the trailing run. */
static char *trim(char *s)
{
    while (*s && isspace((unsigned char)*s)) s++;
    if (!*s) return s;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) *end-- = '\0';
    return s;
}

/* Strip a '#' comment: NUL out from the first '#'. Only called on non-recipe
 * lines — inside recipes '#' is meaningful to the shell and left intact. */
static void strip_comment(char *s)
{
    for (char *p = s; *p; p++)
        if (*p == '#') { *p = '\0'; return; }
}

/* ===========================================================================
 * ASSIGNMENT DETECTION
 * ===========================================================================
 * Tell `CFLAGS := -O2` (assignment) apart from `foo: bar` (rule). We scan left
 * to right; the first of '=' / ':=' / '?=' / '+=' wins, but a bare ':' seen
 * first means we hit a rule's target separator, so it is NOT an assignment.
 * Returns a pointer to the operator (and fills the out-params), or NULL.
 */
static char *detect_assignment(char *line, var_flavor *flavor, int *conditional,
                               int *append, char **value)
{
    for (char *p = line; *p; p++) {
        if (*p == ':') {
            if (p[1] == '=') {         /* ':=' simple assignment                */
                *flavor = VAR_SIMPLE; *conditional = 0; *append = 0;
                *value = p + 2;
                return p;
            }
            return NULL;               /* a bare ':' first -> this is a rule    */
        }
        if (*p == '=') {               /* plain '=' recursive assignment        */
            *flavor = VAR_RECURSIVE; *conditional = 0; *append = 0;
            *value = p + 1;
            return p;
        }
        if (*p == '?' && p[1] == '=') { /* '?=' conditional                     */
            *flavor = VAR_RECURSIVE; *conditional = 1; *append = 0;
            *value = p + 2;
            return p;
        }
        if (*p == '+' && p[1] == '=') { /* '+=' append                         */
            *flavor = VAR_RECURSIVE; *conditional = 0; *append = 1;
            *value = p + 2;
            return p;
        }
    }
    return NULL;                       /* no operator -> not an assignment       */
}

/* Apply "NAME <op> VALUE" with the right semantics for :=, =, ?=, +=. */
static void do_assignment(mk *m, char *line, char *op, var_flavor flavor,
                          int conditional, int append, char *value)
{
    *op = '\0';                        /* terminate the name at the operator     */
    char *name = trim(line);
    value = trim(value);

    if (conditional && mk_var_find(m, name))
        return;                        /* '?=': already set, leave it alone      */

    if (append) {
        variable *v = mk_var_find(m, name);
        if (v) {
            /* '+=' concatenates onto the existing value with a separating
             * space, preserving the existing flavor. If the target is simple-
             * flavored, expand the new text now so the stored value stays fully
             * expanded; otherwise keep it raw for deferred expansion. */
            strbuf sb;
            sb_init(&sb);
            sb_addstr(&sb, v->value);
            sb_addch(&sb, ' ');
            if (v->flavor == VAR_SIMPLE) {
                char *ex = mk_expand(m, value, NULL);
                sb_addstr(&sb, ex);
                free(ex);
            } else {
                sb_addstr(&sb, value);
            }
            free(v->value);
            v->value = sb_detach(&sb);
            return;
        }
        flavor = VAR_RECURSIVE;        /* first definition: behave like '='      */
    }

    if (flavor == VAR_SIMPLE) {
        char *ex = mk_expand(m, value, NULL);   /* ':=' expands ONCE, now       */
        mk_var_set(m, name, ex, VAR_SIMPLE);
        free(ex);
    } else {
        mk_var_set(m, name, value, VAR_RECURSIVE); /* '=' stores raw, expands later */
    }
}

/* ===========================================================================
 * RULE CONSTRUCTION
 * ===========================================================================
 */
static rule *rule_alloc(mk *m)
{
    if (m->nrule == m->rulecap) {
        m->rulecap = m->rulecap ? m->rulecap * 2 : 8;
        m->rules   = xrealloc(m->rules, m->rulecap * sizeof *m->rules);
    }
    rule *r = &m->rules[m->nrule++];
    memset(r, 0, sizeof *r);
    return r;
}

/* Append one recipe command line to a rule. We realloc to exact size each time
 * (recipes are short — a handful of lines — so O(n^2) growth is irrelevant, and
 * it keeps the rule struct free of a capacity field). */
static void rule_add_recipe(rule *r, char *cmd)
{
    r->recipe = xrealloc(r->recipe, (r->nrecipe + 1) * sizeof *r->recipe);
    r->recipe[r->nrecipe++] = cmd;
}

/* Parse a single assembled logical line. `*cur` is the INDEX (into m->rules) of
 * the rule currently collecting recipe lines, or -1 if none. We use an index,
 * not a pointer, because rule_alloc may realloc m->rules and invalidate any
 * held pointer — the classic dangling-pointer-after-growth bug. */
static void process_line(mk *m, long *cur, char *line, int is_recipe)
{
    if (is_recipe) {
        /* Strip exactly ONE leading tab; the rest is the raw command, kept
         * unexpanded so $(VAR)/$@/$< expand at execution time against the node.*/
        char *cmd = line;
        if (*cmd == '\t') cmd++;

        int only_ws = 1;
        for (char *q = cmd; *q; q++)
            if (!isspace((unsigned char)*q)) { only_ws = 0; break; }
        if (only_ws) return;           /* blank recipe line: ignore             */

        if (*cur < 0)
            die("recipe commences before first target: \"%s\"", cmd);
        rule_add_recipe(&m->rules[*cur], xstrdup(cmd));
        return;
    }

    /* Non-recipe line: comments and surrounding whitespace are not significant. */
    strip_comment(line);
    char *t = trim(line);
    if (*t == '\0') {                  /* blank line: closes any open rule       */
        *cur = -1;
        return;
    }

    /* Assignment? */
    var_flavor fl; int cond, app; char *val;
    char *op = detect_assignment(t, &fl, &cond, &app, &val);
    if (op) {
        do_assignment(m, t, op, fl, cond, app, val);
        *cur = -1;                     /* an assignment is not a recipe context  */
        return;
    }

    /* Otherwise it must be a rule: "targets : prereqs". */
    char *colon = strchr(t, ':');
    if (!colon) {
        /* Not an assignment, not a rule. Likely an unsupported directive
         * (include/ifeq/...). Be honest and loud rather than silently wrong. */
        die("unsupported or malformed line: \"%s\"", t);
    }
    *colon = '\0';
    char *tstr = t;                    /* target list (left of ':')             */
    char *pstr = colon + 1;
    if (*pstr == ':') pstr++;          /* tolerate double-colon rules '::' loosely */

    rule *r = rule_alloc(m);
    /* rule_alloc may have grown m->rules; refer to the new rule by its index. */
    long idx = (long)(m->nrule - 1);

    size_t tc = 0, pc = 0;             /* throwaway capacities                   */
    split_words(tstr, &r->targets, &r->ntarget, &tc);
    split_words(pstr, &r->prereqs, &r->nprereq, &pc);

    if (r->ntarget == 0)
        die("rule has no target before ':'");

    *cur = idx;                        /* recipe lines now attach to this rule   */
}

/* ===========================================================================
 * mk_parse — the top-level line loop.
 * ===========================================================================
 * We assemble each LOGICAL line (gluing backslash-continued physical lines)
 * into a strbuf, then dispatch it. Recipe-ness is decided from the FIRST
 * physical line's leading byte, before any trimming, because a leading TAB is
 * the sole thing that distinguishes a recipe line from an ordinary one.
 */
void mk_parse(mk *m, const char *text)
{
    const char *p   = text;
    long        cur = -1;              /* index of the rule collecting recipes   */

    while (*p) {
        int    is_recipe = (*p == '\t');
        strbuf line;
        sb_init(&line);

        for (;;) {
            /* Span this physical line up to '\n' or end of text. */
            const char *nl = p;
            while (*nl && *nl != '\n') nl++;

            /* Count trailing backslashes: an ODD count is a real continuation,
             * an EVEN count is escaped backslashes (e.g. a literal "\\"). */
            const char *b = nl;
            size_t back = 0;
            while (b > p && b[-1] == '\\') { back++; b--; }

            if (*nl == '\n' && (back & 1)) {
                /* Continuation: append this segment minus the trailing '\',
                 * then a single space standing in for the folded newline. */
                sb_addbytes(&line, p, (size_t)(nl - p) - 1);
                sb_addch(&line, ' ');
                p = nl + 1;            /* keep gluing from the next line         */
                continue;
            }

            /* Terminal segment: append it and advance past the newline. */
            sb_addbytes(&line, p, (size_t)(nl - p));
            p = (*nl == '\n') ? nl + 1 : nl;
            break;
        }

        process_line(m, &cur, line.data, is_recipe);
        sb_free(&line);
    }
}
