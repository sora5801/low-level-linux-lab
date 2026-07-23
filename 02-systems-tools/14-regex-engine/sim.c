/* ===========================================================================
 * sim.c — the linear-time matcher: run a Prog over text with a BITSET of states.
 * ===========================================================================
 *
 * This file is the heart of the engine and the reason it cannot blow up.
 *
 * A backtracking matcher keeps ONE position in the NFA and, at every SPLIT,
 * commits to one branch and remembers the other on a stack to retry later. The
 * retries are what go exponential. WE DO SOMETHING DIFFERENT: we keep the SET of
 * all states the NFA could be in, and advance the entire set by one input byte
 * at a time. Reading a byte can only move each of the <= m states to a bounded
 * number of successors, so one step is O(m) and the whole match is O(n * m).
 * There is no stack of alternatives to explode because we never picked one.
 *
 * The set of states is a BITSET (one bit per instruction). Three primitives:
 *
 *   1. epsilon-closure (add_state): from a state, follow all input-free SPLIT/JMP
 *      edges and record every consuming/MATCH state reachable. A visited bit
 *      (the bitset itself) makes cycles — e.g. from  a*  — terminate.
 *
 *   2. transition (nfa_step): the pure bitset kernel. For each active CONSUME
 *      state whose 256-bit set contains the input byte, mark its successor
 *      (pc+1) in a fresh "seed" set. This is the loop reproduced in asm/demo.c.
 *
 *   3. step = transition then closure: seed = nfa_step(cur, c); next = closure(seed).
 *
 * DELIBERATELY DEPENDENCY-FREE: this translation unit includes only regex.h
 * (which pulls in the *freestanding* <stddef.h>/<stdint.h>) and calls NO libc
 * function — not even memset. All memory is caller-provided scratch. That is why
 * its assembly can be produced by a bare cross-compiler with no libc present,
 * and why it is one of the ".s of a real source file" deliverables.
 * ===========================================================================
 */
#include "regex.h"

/* ---- bitset primitives (branch-free, inlined) ----------------------------- */
/* A bitset is an array of 64-bit words; bit i lives in word i>>6 at position
 * i&63. We roll our own clear instead of memset so the file needs no libc. */
static inline void bs_clear(uint64_t *w, int nwords)
{
    for (int i = 0; i < nwords; i++) w[i] = 0;
}
static inline int bs_test(const uint64_t *w, int i)
{
    return (int)((w[i >> 6] >> (i & 63)) & 1u);
}
static inline void bs_set(uint64_t *w, int i)
{
    w[i >> 6] |= (uint64_t)1 << (i & 63);
}

/* ---------------------------------------------------------------------------
 * nfa_step — the pure state-set TRANSITION (no epsilon-closure). This is the
 * inner loop the whole project is built to showcase; asm/demo.c is a verbatim,
 * self-contained copy of exactly this logic so its assembly can be annotated.
 *
 * For every active state (a set bit in `cur`) that is an OP_CONSUME whose 256-
 * bit class set contains byte `c`, set the bit for its successor state (pc+1)
 * in `next`. `next` must be pre-cleared. Non-consuming states (SPLIT/JMP/MATCH)
 * are simply skipped here — epsilon edges are handled by the closure pass.
 *
 * The loop structure is "scan a bitset word, pop its lowest set bit, repeat":
 *   - `bits &= bits - 1`  clears the lowest set bit (the classic Kernighan trick;
 *     on x86 it is a single BLSR instruction — see the annotated asm).
 *   - `__builtin_ctzll(bits)` gives the index of that lowest set bit (TZCNT/BSF).
 * Together they iterate ONLY the set bits, so cost is proportional to the number
 * of ACTIVE states, not to nstate.
 * --------------------------------------------------------------------------- */
void nfa_step(const uint8_t  *op,
              const int32_t  *cls,
              const uint64_t *sets,
              const uint64_t *cur,
              uint64_t       *next,
              int             nwords,
              int             c)
{
    for (int wi = 0; wi < nwords; wi++) {
        uint64_t bits = cur[wi];             /* the active states in this word   */
        while (bits) {                       /* iterate only the SET bits        */
            int b     = __builtin_ctzll(bits);   /* lowest set bit's position    */
            bits     &= bits - 1;                /* clear it (BLSR)              */
            int state = (wi << 6) + b;           /* absolute state index / pc    */

            if (op[state] != OP_CONSUME)         /* only consuming states step   */
                continue;                        /*   on an input byte           */

            /* Membership test: is byte `c` in this state's 256-bit set?
             * The set is 4 words; word (c>>6), bit (c&63). One shift + AND. */
            int k = cls[state];
            uint64_t word = sets[(k << 2) + (c >> 6)];
            if ((word >> (c & 63)) & 1u) {
                int t = state + 1;               /* success edge == pc+1 (layout
                                                  *   invariant from nfa.c)      */
                next[t >> 6] |= (uint64_t)1 << (t & 63);
            }
        }
    }
}

/* ---------------------------------------------------------------------------
 * add_state — iterative epsilon-closure of one state into `set`.
 *
 * Follow every input-free edge (SPLIT forks to x and y; JMP goes to x) and mark
 * every state reached. We MARK ON PUSH: a state's bit is set the moment we first
 * see it, and it is pushed at most once, so (a) infinite loops from cyclic
 * epsilon paths like  a*  are impossible, and (b) the stack never needs more
 * than nstate slots (the exact size scratch_new allocates). No recursion — an
 * explicit stack keeps deep/rich patterns from smashing the C call stack.
 * --------------------------------------------------------------------------- */
static void add_state(const Prog *p, uint64_t *set, int32_t *stack, int start)
{
    if (bs_test(set, start))                 /* already in the set — nothing to do */
        return;
    int sp = 0;
    bs_set(set, start);
    stack[sp++] = start;

    while (sp) {
        int s = stack[--sp];
        switch (p->op[s]) {
        case OP_JMP: {
            int t = p->x[s];
            if (!bs_test(set, t)) { bs_set(set, t); stack[sp++] = t; }
            break;
        }
        case OP_SPLIT: {
            int t1 = p->x[s], t2 = p->y[s];
            if (!bs_test(set, t1)) { bs_set(set, t1); stack[sp++] = t1; }
            if (!bs_test(set, t2)) { bs_set(set, t2); stack[sp++] = t2; }
            break;
        }
        default:
            /* OP_CONSUME and OP_MATCH are terminal for the closure: they consume
             * a byte (or accept), so we do not chase edges out of them here. */
            break;
        }
    }
}

/* Close the raw successor bits in `seed` into `dst` (dst is cleared first). Each
 * seeded state is expanded through its epsilon edges. This is step 3's closure
 * half; it reads seed, writes dst, and shares the closure stack. */
static void closure_from_seed(const Prog *p, const uint64_t *seed,
                              uint64_t *dst, int32_t *stack, int nwords)
{
    bs_clear(dst, nwords);
    for (int wi = 0; wi < nwords; wi++) {
        uint64_t bits = seed[wi];
        while (bits) {
            int b = __builtin_ctzll(bits);
            bits &= bits - 1;
            add_state(p, dst, stack, (wi << 6) + b);
        }
    }
}

/* ---------------------------------------------------------------------------
 * re_fullmatch — ANCHORED: does the WHOLE text match? (semantics of ^pattern$)
 *
 * Seed the start state's closure, then for each input byte run one step. The
 * pattern matches iff, after consuming EVERY byte, the MATCH state is active —
 * because MATCH is reachable only at the end of the pattern, so its presence in
 * the final set means "the pattern consumed exactly this whole string".
 *
 * Early exit: if the active set ever becomes empty, no thread can survive, so we
 * can stop immediately and report no match. This is a real speedup, not just an
 * optimization — it is why "abc" against a megabyte of "x" is instant.
 * --------------------------------------------------------------------------- */
int re_fullmatch(const Prog *p, Scratch *s, const char *text, size_t n)
{
    uint64_t *cur = s->cur, *seed = s->seed, *next = s->next;
    int nwords = s->nwords;

    bs_clear(cur, nwords);
    add_state(p, cur, s->stack, p->start);   /* cur = closure({start})           */

    for (size_t i = 0; i < n; i++) {
        /* Is any thread still alive? If not, this string cannot match. */
        int alive = 0;
        for (int w = 0; w < nwords; w++) alive |= (cur[w] != 0);
        if (!alive) return 0;

        bs_clear(seed, nwords);
        nfa_step(p->op, p->cls, p->sets, cur, seed, nwords,
                 (unsigned char)text[i]);    /* raw transition                   */
        closure_from_seed(p, seed, next, s->stack, nwords); /* + epsilon closure */

        /* Swap cur <- next (pointer swap; no copy). */
        uint64_t *t = cur; cur = next; next = t;
    }

    /* Consumed all n bytes: accept iff MATCH is in the final active set. */
    return bs_test(cur, p->match);
}

/* ---------------------------------------------------------------------------
 * re_contains — UNANCHORED: does SOME substring match? (grep semantics)
 *
 * Same machine, one addition: at every input position we re-seed the start
 * state, which models "a match may begin here". Because add_state dedups against
 * the current set, re-seeding is idempotent and cheap, and the O(n * m) bound is
 * untouched. We accept as soon as MATCH appears in the active set — that means a
 * substring ending at the current position matched.
 *
 * Note we check for MATCH *after* seeding start at position i but *before*
 * consuming text[i]: that correctly reports zero-width and short matches, and it
 * checks the empty-string case at position 0 for patterns like  a*.
 * --------------------------------------------------------------------------- */
int re_contains(const Prog *p, Scratch *s, const char *text, size_t n)
{
    uint64_t *cur = s->cur, *seed = s->seed, *next = s->next;
    int nwords = s->nwords;

    bs_clear(cur, nwords);

    for (size_t i = 0; ; i++) {
        /* Unanchored: allow a fresh match to start at position i. */
        add_state(p, cur, s->stack, p->start);

        if (bs_test(cur, p->match))          /* a substring ending at i matched  */
            return 1;

        if (i == n)                          /* processed the sentinel end slot  */
            break;

        bs_clear(seed, nwords);
        nfa_step(p->op, p->cls, p->sets, cur, seed, nwords,
                 (unsigned char)text[i]);
        closure_from_seed(p, seed, next, s->stack, nwords);

        uint64_t *t = cur; cur = next; next = t;
    }
    return 0;
}
