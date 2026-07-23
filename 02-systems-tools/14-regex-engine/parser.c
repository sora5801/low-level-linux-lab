/* ===========================================================================
 * parser.c — recursive-descent parser: pattern string  ->  AST.
 * ===========================================================================
 *
 * This is a hand-written recursive-descent parser, one function per grammar
 * rule (see ast.h for the grammar). Recursive descent is the right tool here:
 * the grammar is small, and mapping "one function per production" makes the code
 * a direct transcription of the grammar, which is the whole didactic point.
 *
 * Error handling without exceptions: the parser state carries a RegexError. The
 * first time a rule detects a problem it calls fail(), which records the message
 * and offset and returns NULL. Every caller checks for NULL and bubbles it up.
 * Because all nodes live in an arena (see ast.h), a mid-parse failure leaks
 * nothing — the caller frees the whole arena in one shot.
 * ===========================================================================
 */
#include "ast.h"

/* Parser state: a cursor over the pattern plus the arena and error sink. */
typedef struct {
    const char *p;      /* current read position (points at the next byte)      */
    const char *base;   /* start of the pattern, so we can compute offsets      */
    Arena      *arena;  /* where nodes come from                                */
    RegexError *err;    /* error report; err->ok stays 1 until fail() is called */
} P;

/* ---- arena bump-allocation ------------------------------------------------ */
/* Hand back the next node slot, or NULL if we somehow hit the (generous) cap.
 * A NULL return is treated as a parse failure by callers, so an exhausted arena
 * degrades safely instead of writing out of bounds. */
static Node *node_new(P *ps, int kind)
{
    if (ps->arena->n >= ps->arena->cap)
        return NULL;                         /* refuse to overflow the pool     */
    Node *nd = &ps->arena->pool[ps->arena->n++];
    nd->kind = kind;
    nd->a = nd->b = NULL;
    /* set[] is only meaningful for N_SET; callers that need it fill it in. */
    return nd;
}

/* Record a syntax error (only the FIRST one sticks) and return NULL so the
 * failure unwinds up the recursion. `at` points into the pattern. */
static Node *fail(P *ps, const char *at, const char *msg)
{
    if (ps->err && ps->err->ok) {            /* keep the earliest error         */
        ps->err->ok = 0;
        ps->err->offset = (int)(at - ps->base);
        /* Bounded copy, no libc str* dependency, always NUL-terminated. */
        int i = 0;
        while (msg[i] && i < (int)sizeof(ps->err->msg) - 1) {
            ps->err->msg[i] = msg[i];
            i++;
        }
        ps->err->msg[i] = '\0';
    }
    return NULL;
}

/* Peek at the current byte without consuming (0 == end of pattern). We read as
 * unsigned char so bytes >= 0x80 are not sign-extended into negative ints. */
static int peek(P *ps) { return (unsigned char)*ps->p; }

/* ---- 256-bit set helpers (a byte set lives in node->set[32]) --------------
 * A character class is a membership predicate over the 256 possible byte values,
 * so it fits in exactly 256 bits = 32 bytes. Storing it as a bitmap (rather than
 * a list of ranges) makes both "add a byte" and, later, "does byte c match?"
 * O(1) — and it is the exact shape the NFA compiler packs into its class sets.
 * Bit b lives in byte b>>3 of the array, at bit position b&7 within that byte. */
static void set_add(uint8_t set[32], int b)     { set[(b & 0xFF) >> 3] |= (uint8_t)(1u << (b & 7)); }
static void set_clear_all(uint8_t set[32])      { for (int i = 0; i < 32; i++) set[i] = 0; }
/* Fill an inclusive byte range [lo,hi] — the workhorse behind '.', [a-z], \d... */
static void set_add_range(uint8_t s[32], int lo, int hi) { for (int b = lo; b <= hi; b++) set_add(s, b); }
/* Complement over ALL 256 byte values — this is what [^...] and \D/\W/\S do.
 * Note it flips every bit, so a negated class can match high bytes (0x80..0xFF)
 * too; that matches grep/PCRE's byte-oriented behavior for a non-Unicode engine. */
static void set_negate(uint8_t set[32])         { for (int i = 0; i < 32; i++) set[i] = (uint8_t)~set[i]; }

/* Forward declarations (the productions are mutually recursive). */
static Node *parse_alt(P *ps);
static Node *parse_concat(P *ps);
static Node *parse_repeat(P *ps);
static Node *parse_atom(P *ps);

/* ---------------------------------------------------------------------------
 * Escape handling. After a backslash, translate the next byte into either a
 * single literal byte (\n, \t, \\, \., an escaped metacharacter, ...) or a
 * predefined character class (\d \w \s and their negations \D \W \S). We fill
 * `set` and return 1 on success, or call fail() and return 0.
 * --------------------------------------------------------------------------- */
static int parse_escape(P *ps, uint8_t set[32])
{
    const char *at = ps->p;                  /* points at the backslash         */
    ps->p++;                                 /* consume '\'                     */
    int e = peek(ps);
    if (e == 0)
        return (fail(ps, at, "trailing backslash"), 0);
    ps->p++;                                 /* consume the escaped char        */
    set_clear_all(set);

    switch (e) {
    /* C-style single-byte escapes: turn the two-character source sequence "\n"
     * into the single control byte it denotes, so patterns can name bytes that
     * are awkward to type literally. Each adds exactly one bit to the set. */
    case 'n': set_add(set, '\n'); return 1;
    case 't': set_add(set, '\t'); return 1;
    case 'r': set_add(set, '\r'); return 1;
    case 'f': set_add(set, '\f'); return 1;
    case 'v': set_add(set, '\v'); return 1;
    case '0': set_add(set, '\0'); return 1;

    /* Perl-ish class shorthands. \d = [0-9], \w = [A-Za-z0-9_], \s = whitespace.
     * The uppercase forms are the complement over all 256 byte values. */
    case 'd': set_add_range(set, '0', '9'); return 1;
    case 'D': set_add_range(set, '0', '9'); set_negate(set); return 1;
    case 'w': set_add_range(set, 'a', 'z'); set_add_range(set, 'A', 'Z');
              set_add_range(set, '0', '9'); set_add(set, '_'); return 1;
    case 'W': set_add_range(set, 'a', 'z'); set_add_range(set, 'A', 'Z');
              set_add_range(set, '0', '9'); set_add(set, '_'); set_negate(set); return 1;
    case 's': set_add(set, ' '); set_add(set, '\t'); set_add(set, '\n');
              set_add(set, '\r'); set_add(set, '\f'); set_add(set, '\v'); return 1;
    case 'S': set_add(set, ' '); set_add(set, '\t'); set_add(set, '\n');
              set_add(set, '\r'); set_add(set, '\f'); set_add(set, '\v');
              set_negate(set); return 1;

    /* Anything else is a literal of that byte: \. \* \\ \[ etc. This is the
     * escape hatch that lets you match a metacharacter literally. */
    default:  set_add(set, e); return 1;
    }
}

/* ---------------------------------------------------------------------------
 * Bracket class:  '[' '^'? ( item )* ']'  where item is a byte, an escape, or a
 * range  lo '-' hi. We enter with ps->p at '['. A leading '^' negates the set
 * over all 256 byte values. A ']' as the very first item is a literal ']'
 * (the traditional POSIX rule), so we handle position 0 specially.
 * --------------------------------------------------------------------------- */
static Node *parse_class(P *ps)
{
    const char *open = ps->p;                /* remember '[' for error offset    */
    ps->p++;                                 /* consume '['                     */

    Node *nd = node_new(ps, N_SET);
    if (!nd) return fail(ps, open, "out of AST nodes");
    set_clear_all(nd->set);

    int negate = 0;
    if (peek(ps) == '^') { negate = 1; ps->p++; }   /* [^...] complement        */

    int first = 1;                           /* to allow a leading literal ']'  */
    for (;;) {
        int c = peek(ps);
        if (c == 0)
            return fail(ps, open, "unterminated character class '['");
        if (c == ']' && !first) { ps->p++; break; }  /* end of class            */
        first = 0;

        /* Read one endpoint (either an escape or a raw byte). */
        int lo;
        if (c == '\\') {
            /* An escape inside a class may itself be a whole set (\d, \s...).
             * If so, we OR it in and it cannot participate in a range. */
            uint8_t esc[32];
            const char *before = ps->p;
            if (!parse_escape(ps, esc)) return NULL;
            /* Detect "single byte" escapes vs multi-byte class shorthands by
             * counting bits: a shorthand set has many bits; treat >1 as "not a
             * range endpoint" and just union it. */
            int bits = 0, only = -1;
            for (int b = 0; b < 256; b++)
                if (esc[b >> 3] & (1u << (b & 7))) { bits++; only = b; }
            if (bits != 1) {
                for (int i = 0; i < 32; i++) nd->set[i] |= esc[i];
                continue;                     /* shorthand consumed; next item   */
            }
            lo = only;
            (void)before;
        } else {
            lo = c; ps->p++;                  /* consume the literal byte        */
        }

        /* Is this a range "lo - hi"?  A '-' begins a range ONLY if it is not the
         * last item in the class: "[a-z]" is a range, but "[a-]" and "[a-\0]"
         * mean a literal '-'. We look one byte ahead to decide, which is why the
         * '-' is treated as an ordinary literal when it is trailing. */
        if (peek(ps) == '-' && ps->p[1] != ']' && ps->p[1] != '\0') {
            ps->p++;                          /* consume '-'                     */
            int hi;
            if (peek(ps) == '\\') {
                uint8_t esc[32];
                if (!parse_escape(ps, esc)) return NULL;
                hi = -1;
                for (int b = 0; b < 256; b++)
                    if (esc[b >> 3] & (1u << (b & 7))) { hi = b; break; }
            } else {
                hi = peek(ps); ps->p++;
            }
            if (hi < lo)
                return fail(ps, open, "character range is out of order (hi < lo)");
            set_add_range(nd->set, lo, hi);
        } else {
            set_add(nd->set, lo);             /* just a single byte              */
        }
    }

    if (negate) set_negate(nd->set);          /* [^...] flips the whole 256 bits */
    return nd;
}

/* ---------------------------------------------------------------------------
 * atom := '(' alternation ')' | class | '.' | escape | literal
 * The lowest-precedence-binding, indivisible unit a quantifier can attach to.
 * --------------------------------------------------------------------------- */
static Node *parse_atom(P *ps)
{
    int c = peek(ps);

    if (c == '(') {                          /* a parenthesised sub-expression  */
        const char *open = ps->p;
        ps->p++;                             /* consume '('                     */
        Node *inner = parse_alt(ps);         /* recurse into full grammar       */
        if (!inner) return NULL;
        if (peek(ps) != ')')
            return fail(ps, open, "unbalanced '(' — expected ')'");
        ps->p++;                             /* consume ')'                     */
        return inner;                        /* groups are purely structural    */
    }

    if (c == '[')                            /* bracket character class         */
        return parse_class(ps);

    if (c == '.') {                          /* '.' matches any byte but newline */
        ps->p++;
        Node *nd = node_new(ps, N_SET);
        if (!nd) return fail(ps, ps->p, "out of AST nodes");
        set_clear_all(nd->set);
        set_add_range(nd->set, 0, 255);      /* everything...                   */
        nd->set['\n' >> 3] &= (uint8_t)~(1u << ('\n' & 7)); /* ...except '\n'    */
        return nd;
    }

    if (c == '\\') {                         /* escape -> single byte or shorthand */
        Node *nd = node_new(ps, N_SET);
        if (!nd) return fail(ps, ps->p, "out of AST nodes");
        if (!parse_escape(ps, nd->set)) return NULL;
        return nd;
    }

    /* These can never START an atom. Seeing one here means the pattern is
     * malformed (e.g. a stray ')' or a quantifier with nothing to repeat). */
    if (c == 0)   return fail(ps, ps->p, "unexpected end of pattern");
    if (c == ')') return fail(ps, ps->p, "unmatched ')'");
    if (c == '|') return fail(ps, ps->p, "empty alternative before '|'");
    if (c == '*' || c == '+' || c == '?')
        return fail(ps, ps->p, "quantifier with nothing to repeat");

    /* Otherwise it is an ordinary literal byte. */
    ps->p++;
    Node *nd = node_new(ps, N_SET);
    if (!nd) return fail(ps, ps->p, "out of AST nodes");
    set_clear_all(nd->set);
    set_add(nd->set, c);
    return nd;
}

/* ---------------------------------------------------------------------------
 * repetition := atom ('*' | '+' | '?')*
 * Postfix quantifiers. We allow them to STACK (a** , a*? , a+? ...) because each
 * simply wraps the node built so far — the grammar is happy to nest them and it
 * costs nothing. (We do not implement lazy `*?` vs greedy `*` distinctly: this
 * engine reports match / no-match, and greediness only affects which capture a
 * backtracker would pick, not WHETHER a set-simulation matches.)
 * --------------------------------------------------------------------------- */
static Node *parse_repeat(P *ps)
{
    Node *nd = parse_atom(ps);
    if (!nd) return NULL;

    for (;;) {
        int c = peek(ps);
        int kind;
        if      (c == '*') kind = N_STAR;
        else if (c == '+') kind = N_PLUS;
        else if (c == '?') kind = N_QUEST;
        else break;                          /* no more quantifiers             */
        ps->p++;                             /* consume the quantifier          */
        Node *w = node_new(ps, kind);
        if (!w) return fail(ps, ps->p, "out of AST nodes");
        w->a = nd;                           /* wrap the current node           */
        nd = w;
    }
    return nd;
}

/* ---------------------------------------------------------------------------
 * concatenation := repetition*
 * A run of adjacent terms with no operator between them. Zero terms is the
 * empty string (N_EMPTY), which legitimately matches ""  (e.g. the pattern
 * "a|" has an empty right alternative). We fold the terms LEFT into a chain of
 * N_CONCAT nodes.
 * --------------------------------------------------------------------------- */
static Node *parse_concat(P *ps)
{
    /* A concatenation ends at '|', ')', or end-of-pattern. */
    int c = peek(ps);
    if (c == 0 || c == '|' || c == ')')
        return node_new(ps, N_EMPTY);        /* empty concatenation = epsilon   */

    Node *lhs = parse_repeat(ps);
    if (!lhs) return NULL;

    for (;;) {
        c = peek(ps);
        if (c == 0 || c == '|' || c == ')')
            break;                           /* end of this concatenation       */
        Node *rhs = parse_repeat(ps);
        if (!rhs) return NULL;
        Node *cat = node_new(ps, N_CONCAT);
        if (!cat) return fail(ps, ps->p, "out of AST nodes");
        cat->a = lhs; cat->b = rhs;
        lhs = cat;                           /* left-associative fold           */
    }
    return lhs;
}

/* ---------------------------------------------------------------------------
 * alternation := concatenation ('|' concatenation)*
 * The lowest-precedence rule, so it is the entry point. Folds LEFT into N_ALT.
 * --------------------------------------------------------------------------- */
static Node *parse_alt(P *ps)
{
    Node *lhs = parse_concat(ps);
    if (!lhs) return NULL;

    while (peek(ps) == '|') {
        ps->p++;                             /* consume '|'                     */
        Node *rhs = parse_concat(ps);
        if (!rhs) return NULL;
        Node *alt = node_new(ps, N_ALT);
        if (!alt) return fail(ps, ps->p, "out of AST nodes");
        alt->a = lhs; alt->b = rhs;
        lhs = alt;
    }
    return lhs;
}

/* Public entry: drive the top rule, then insist we consumed the whole pattern.
 * A leftover byte here is almost always an unbalanced ')'. */
Node *parse(const char *pat, Arena *arena, RegexError *err)
{
    P ps = { pat, pat, arena, err };
    if (err) { err->ok = 1; err->offset = 0; err->msg[0] = '\0'; }

    Node *root = parse_alt(&ps);
    if (!root) return NULL;

    if (peek(&ps) != 0)
        return fail(&ps, ps.p, "unexpected trailing character (unbalanced ')'?)");
    return root;
}
