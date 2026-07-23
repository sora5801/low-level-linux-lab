/* ===========================================================================
 * nfa.c — Thompson construction: AST  ->  NFA bytecode (Prog), plus lifecycle.
 * ===========================================================================
 *
 * Ken Thompson's construction (CACM, 1968) turns a regex into a nondeterministic
 * finite automaton by gluing together tiny "fragments", one per grammar node.
 * The trick that makes it linear-time to SIMULATE later is that every fragment
 * has exactly one entry and a well-defined exit, and choice is expressed with
 * epsilon (input-free) SPLIT edges rather than by trying alternatives in turn.
 *
 * We compile to a flat instruction array (a bytecode) rather than a graph of
 * malloc'd state structs. This is the RE2/PCRE-internal representation and it
 * has two virtues: (1) a program counter is just an int index, so the "set of
 * active states" the simulator tracks is a set of ints — a bitset; (2) the
 * layout enforces the invariant the simulator relies on: a CONSUME's success
 * edge is the physically next instruction, so we never store it.
 *
 * THE LAYOUT INVARIANT, stated precisely: after compiling any node, the
 * instructions it emitted occupy a contiguous range [start, e->n), and control
 * "falls out the bottom" — i.e. the last instruction's natural successor is
 * e->n, which is exactly where the NEXT node will be emitted. Every fragment
 * below is written to preserve that, which is why concatenation is literally
 * "compile a; compile b" with nothing in between.
 * ===========================================================================
 */
#include <stdlib.h>   /* malloc, realloc, free                                   */
#include "ast.h"

/* ---------------------------------------------------------------------------
 * Emitter: growable Structure-of-Arrays under construction. We append
 * instructions and back-patch branch targets by index. Each array grows with
 * the classic doubling strategy so N appends cost O(N) amortized, not O(N^2).
 * --------------------------------------------------------------------------- */
typedef struct {
    uint8_t  *op;     /* opcode per instruction                                 */
    int32_t  *x, *y;  /* branch targets (SPLIT uses both; JMP uses x)           */
    int32_t  *cls;    /* class id for CONSUME, -1 otherwise                     */
    int       n, cap; /* count and capacity of the above four arrays            */

    uint64_t *sets;   /* 256-bit membership sets, 4 words each                  */
    int       ncls;   /* number of sets emitted so far                         */
    int       setcap; /* capacity of `sets` in WORDS                            */

    int       oom;    /* sticky out-of-memory flag; checked once at the end     */
} Emit;

/* Ensure the instruction arrays can hold one more entry; double on demand. */
static void ensure_inst(Emit *e)
{
    if (e->n < e->cap) return;
    int ncap = e->cap ? e->cap * 2 : 16;
    /* realloc each column. On failure we set `oom` and stop touching memory;
     * callers keep going but produce a program we will discard. Keeping the old
     * pointers on failure avoids leaking them (we free them in the error path). */
    uint8_t *op  = realloc(e->op,  (size_t)ncap * sizeof *op);
    int32_t *x   = realloc(e->x,   (size_t)ncap * sizeof *x);
    int32_t *y   = realloc(e->y,   (size_t)ncap * sizeof *y);
    int32_t *cls = realloc(e->cls, (size_t)ncap * sizeof *cls);
    if (!op || !x || !y || !cls) { e->oom = 1;
        /* Adopt whatever succeeded so nothing is leaked or double-freed. */
        if (op)  e->op  = op;
        if (x)   e->x   = x;
        if (y)   e->y   = y;
        if (cls) e->cls = cls;
        return;
    }
    e->op = op; e->x = x; e->y = y; e->cls = cls; e->cap = ncap;
}

/* Emit one instruction; return its program counter (index). Targets that are
 * not yet known are passed as -1 and back-patched by the caller. */
static int emit(Emit *e, int op, int x, int y, int cls)
{
    ensure_inst(e);
    if (e->oom) return 0;                 /* harmless dummy pc; oom is sticky    */
    int pc = e->n++;
    e->op[pc]  = (uint8_t)op;
    e->x[pc]   = x;
    e->y[pc]   = y;
    e->cls[pc] = cls;
    return pc;
}

/* Emit a CONSUME instruction, allocating a fresh 256-bit class set from the
 * node's byte bitmap. Each CONSUME gets its own set slot (no dedup): simplest,
 * and the memory is tiny — 32 bytes per consuming state. */
static int emit_consume(Emit *e, const uint8_t bytes[32])
{
    /* Grow the sets pool (measured in 64-bit words; 4 words == one 256-bit set). */
    if ((e->ncls + 1) * 4 > e->setcap) {
        int ncap = e->setcap ? e->setcap * 2 : 64;
        uint64_t *s = realloc(e->sets, (size_t)ncap * sizeof *s);
        if (!s) { e->oom = 1; return 0; }
        e->sets = s; e->setcap = ncap;
    }
    int k = e->ncls++;
    uint64_t *w = &e->sets[k * 4];

    /* Pack the 32-byte bitmap into 4 little-endian 64-bit words so the matcher
     * can test membership of byte c with:  (w[c>>6] >> (c&63)) & 1.
     * We assemble the words byte-by-byte to stay endianness-independent. */
    for (int word = 0; word < 4; word++) {
        uint64_t v = 0;
        for (int t = 0; t < 8; t++)
            v |= (uint64_t)bytes[word * 8 + t] << (t * 8);
        w[word] = v;
    }
    return emit(e, OP_CONSUME, -1, -1, k);
}

/* ---------------------------------------------------------------------------
 * compile(node) — emit the fragment for `node`, preserving the fall-out-the-
 * bottom invariant. Each case is a direct transcription of the classic Thompson
 * fragment diagrams; the ASCII sketches show the epsilon (SPLIT/JMP) wiring.
 * --------------------------------------------------------------------------- */
static void compile(Emit *e, const Node *nd)
{
    if (e->oom || !nd) return;

    switch (nd->kind) {

    case N_EMPTY:
        /* Epsilon: emit nothing. Control simply falls through to the next
         * fragment, which is exactly what "matches the empty string" means. */
        return;

    case N_SET:
        /* One byte consumed against nd->set. Success edge is implicit (pc+1). */
        emit_consume(e, nd->set);
        return;

    case N_CONCAT:
        /* a·b :  [ a ][ b ].  Because `a` falls out the bottom straight into the
         * first instruction of `b`, concatenation needs NO glue at all. */
        compile(e, nd->a);
        compile(e, nd->b);
        return;

    case N_ALT: {
        /* a|b :
         *        split ---> A: [ a ] ---> jmp --.
         *          \                            |
         *           `------> B: [ b ] ----------+--> (fall through here)
         */
        int p = emit(e, OP_SPLIT, -1, -1, -1);
        e->x[p] = e->n;                 /* preferred branch: enter A (a)         */
        compile(e, nd->a);
        int j = emit(e, OP_JMP, -1, -1, -1);
        e->y[p] = e->n;                 /* other branch: enter B (b)             */
        compile(e, nd->b);
        e->x[j] = e->n;                 /* both branches converge here           */
        return;
    }

    case N_STAR: {
        /* a* (greedy):
         *    L: split --(x)--> [ a ] --> jmp L
         *         \--(y)--> (exit, fall through)
         * The visited-set epsilon-closure in sim.c makes the split->...->jmp->
         * split cycle terminate even when `a` can match empty. */
        int p = emit(e, OP_SPLIT, -1, -1, -1);
        e->x[p] = e->n;                 /* x = enter the body (greedy: loop 1st) */
        compile(e, nd->a);
        emit(e, OP_JMP, p, -1, -1);     /* loop back to the split                */
        e->y[p] = e->n;                 /* y = exit the loop                     */
        return;
    }

    case N_PLUS: {
        /* a+ (greedy):
         *    B: [ a ]
         *       split --(x)--> B          (loop back for another a)
         *         \--(y)--> (exit)
         * `a` runs at least once before the split, giving "one or more". */
        int body = e->n;
        compile(e, nd->a);
        int p = emit(e, OP_SPLIT, body, -1, -1);
        e->y[p] = e->n;                 /* exit after the loop                   */
        return;
    }

    case N_QUEST: {
        /* a? (greedy):
         *    split --(x)--> [ a ] --> (exit)
         *      \--(y)--> (exit)         (skip a entirely)
         */
        int p = emit(e, OP_SPLIT, -1, -1, -1);
        e->x[p] = e->n;                 /* x = take the optional a               */
        compile(e, nd->a);
        e->y[p] = e->n;                 /* y = skip it                           */
        return;
    }

    default:
        return;                         /* unreachable: parser emits no others   */
    }
}

/* ---------------------------------------------------------------------------
 * re_compile — parse, then Thompson-compile, then package into a Prog.
 * --------------------------------------------------------------------------- */
Prog *re_compile(const char *pattern, RegexError *err)
{
    RegexError local;                   /* used if the caller passed err==NULL   */
    if (!err) err = &local;
    err->ok = 1; err->offset = 0; err->msg[0] = '\0';

    /* --- 1. Parse into an AST from a bounded arena (see ast.h for the bound). */
    size_t len = 0; while (pattern[len]) len++;
    Arena arena;
    arena.cap  = (int)(6 * len + 16);
    arena.n    = 0;
    arena.pool = malloc((size_t)arena.cap * sizeof(Node));
    if (!arena.pool) {
        err->ok = 0; err->offset = 0;
        /* Fixed message; no allocation on the error path. */
        const char *m = "out of memory"; int i = 0;
        while (m[i] && i < 63) { err->msg[i] = m[i]; i++; } err->msg[i] = 0;
        return NULL;
    }

    Node *root = parse(pattern, &arena, err);
    if (!root) { free(arena.pool); return NULL; }   /* err already filled        */

    /* --- 2. Thompson-compile the AST, then append the accepting MATCH state. */
    Emit e;
    e.op = NULL; e.x = NULL; e.y = NULL; e.cls = NULL; e.n = 0; e.cap = 0;
    e.sets = NULL; e.ncls = 0; e.setcap = 0; e.oom = 0;

    compile(&e, root);
    int match_pc = emit(&e, OP_MATCH, -1, -1, -1);

    free(arena.pool);                   /* AST no longer needed — one free()     */

    if (e.oom) {                        /* any allocation failed while emitting  */
        free(e.op); free(e.x); free(e.y); free(e.cls); free(e.sets);
        err->ok = 0; err->offset = 0;
        const char *m = "out of memory"; int i = 0;
        while (m[i] && i < 63) { err->msg[i] = m[i]; i++; } err->msg[i] = 0;
        return NULL;
    }

    /* --- 3. Move the emitter's arrays into a Prog (transfer of ownership). */
    Prog *p = malloc(sizeof *p);
    if (!p) {
        free(e.op); free(e.x); free(e.y); free(e.cls); free(e.sets);
        err->ok = 0; const char *m = "out of memory"; int i = 0;
        while (m[i] && i < 63) { err->msg[i] = m[i]; i++; } err->msg[i] = 0;
        return NULL;
    }
    p->nstate = e.n;
    p->op = e.op; p->x = e.x; p->y = e.y; p->cls = e.cls;
    p->ncls = e.ncls; p->sets = e.sets;
    p->start = 0;
    p->match = match_pc;
    return p;
}

/* Free a Prog and every array it owns. Ordering does not matter; all distinct. */
void re_free(Prog *p)
{
    if (!p) return;
    free(p->op); free(p->x); free(p->y); free(p->cls); free(p->sets);
    free(p);
}

/* ---------------------------------------------------------------------------
 * scratch_new / scratch_free — the reusable per-match working set.
 *
 * The matcher must never allocate in its hot loop, so we allocate ALL of its
 * memory here, once, sized to the program. Layout:
 *   - three bitsets (cur, seed, next), each ceil(nstate/64) words wide;
 *   - one closure stack of `nstate` ints (see sim.c: the mark-on-push closure
 *     pushes each state at most once, so nstate slots is the exact bound).
 * --------------------------------------------------------------------------- */
Scratch *scratch_new(const Prog *p)
{
    if (!p) return NULL;
    int nwords = (p->nstate + 63) / 64;      /* bits -> 64-bit words, rounded up */
    if (nwords < 1) nwords = 1;              /* at least one word (nstate>=1)    */

    Scratch *s = malloc(sizeof *s);
    if (!s) return NULL;
    s->nwords = nwords;
    s->nstate = p->nstate;
    s->cur   = malloc((size_t)nwords * sizeof(uint64_t));
    s->seed  = malloc((size_t)nwords * sizeof(uint64_t));
    s->next  = malloc((size_t)nwords * sizeof(uint64_t));
    s->stack = malloc((size_t)(p->nstate + 1) * sizeof(int32_t));
    if (!s->cur || !s->seed || !s->next || !s->stack) {
        scratch_free(s);                     /* frees whatever succeeded         */
        return NULL;
    }
    return s;
}

void scratch_free(Scratch *s)
{
    if (!s) return;
    free(s->cur); free(s->seed); free(s->next); free(s->stack);
    free(s);
}
