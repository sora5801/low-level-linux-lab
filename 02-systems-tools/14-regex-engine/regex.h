/* ===========================================================================
 * regex.h — public interface of a linear-time (Thompson NFA) regex engine.
 * ===========================================================================
 *
 * WHY THIS ENGINE EXISTS (the one idea to take away)
 * --------------------------------------------------
 * A "backtracking" regex engine (PCRE, Perl, Python's `re`, JavaScript) matches
 * by trying one path through the pattern, and on failure *rewinding* the input
 * and trying the next path. For a pattern like  (a|a)*b  against  "aaaa...aaac"
 * the number of distinct paths is EXPONENTIAL in the input length, so the engine
 * can spend seconds — or effectively forever — on a 30-character string. This is
 * the "catastrophic backtracking" / ReDoS class of denial-of-service bugs.
 *
 * Ken Thompson's 1968 construction sidesteps this completely. Instead of walking
 * ONE path and backing up, we keep the SET of all NFA states the machine could
 * be in after reading each input byte, and advance the whole set one byte at a
 * time. There are at most `m` states (m = pattern size), the input is `n` bytes,
 * so the work is at most O(n * m): strictly linear in the input, no matter how
 * pathological the pattern. There is nothing to back up *because we never chose*.
 *
 * This header exposes that machine. The implementation is split so each stage is
 * legible on its own:
 *
 *     parser.c   pattern string  ->  AST            (recursive descent)
 *     nfa.c      AST             ->  Prog           (Thompson construction)
 *     sim.c      Prog + text     ->  match?         (bitset state-set simulation)
 *
 * `Prog` is deliberately a tiny *bytecode* for an NFA — the same design PCRE and
 * RE2 use internally (see Russ Cox, "Regular Expression Matching Can Be Simple
 * And Fast"). Four opcodes are enough to encode any regex built from
 * concatenation, alternation, and the `* + ?` quantifiers.
 *
 * PORTABILITY: pure ISO C plus two compiler builtins (__builtin_ctzll and
 * realloc/free from libc). Only <stddef.h> and <stdint.h> are pulled in here,
 * and both are *freestanding* headers — no libc runtime is required to compile
 * the matcher (sim.c), which is why its assembly can be generated with a bare
 * cross-compiler. See asm/README notes.
 * ===========================================================================
 */
#ifndef REGEX_H
#define REGEX_H

#include <stddef.h>   /* size_t                                   (freestanding) */
#include <stdint.h>   /* uint8_t / uint32_t / uint64_t / int32_t  (freestanding) */

/* ---------------------------------------------------------------------------
 * The NFA bytecode.
 *
 * Every regex compiles to a flat array of instructions ("states"). Execution is
 * a set of "threads", each just a program counter (an index into the array).
 * Only ONE opcode consumes an input byte; the other three are epsilon moves
 * (they change state without advancing the input), which is exactly how a
 * Thompson NFA is defined.
 *
 * Crucial layout invariant exploited by the simulator: a CONSUME instruction's
 * success edge is ALWAYS the physically next instruction (pc + 1). The compiler
 * emits fragments so this always holds, so we never store that edge — see nfa.c.
 * --------------------------------------------------------------------------- */
enum {
    OP_CONSUME = 0, /* match one input byte against the 256-bit set cls[pc];
                     *   on success the thread advances to state pc+1.
                     *   (CONSUME==0 on purpose: it is the hot case, and the
                     *   simulator's inner loop tests `op[s]==0` — a compare with
                     *   zero is one byte shorter and fuses on x86. See sim.c.) */
    OP_SPLIT,       /* epsilon FORK: the thread becomes two threads, at x[pc]
                     *   and y[pc]. This is how |, *, +, ? introduce choice —
                     *   and, critically, BOTH branches are explored at once, so
                     *   there is no backtracking. */
    OP_JMP,         /* epsilon GOTO x[pc] (unconditional; used to close loops). */
    OP_MATCH        /* accept: if a thread reaches here, the pattern matched. */
};

/* ---------------------------------------------------------------------------
 * Prog — a compiled program, stored Structure-of-Arrays (SoA) rather than
 * Array-of-Structs. Why SoA: the simulator's hot loop touches op[] and cls[]
 * for every active state on every input byte, but touches x[]/y[] only for the
 * rare SPLIT/JMP. Keeping op[] contiguous means a single cache line holds ~64
 * opcodes, so scanning the active set streams linearly through memory instead
 * of hopping over the wider fields it does not need. This is the same reason
 * ECS game engines and columnar databases use SoA.
 *
 * OWNERSHIP: re_compile() allocates every array below with a single owner. The
 * caller frees the whole thing with re_free(); nothing here is shared or
 * aliased, so the free is a straight sequence of free() calls.
 * --------------------------------------------------------------------------- */
typedef struct {
    int       nstate;   /* number of instructions == number of NFA states       */

    uint8_t  *op;       /* op[i]  : opcode of state i (one of OP_*)   [nstate]   */
    int32_t  *x;        /* x[i]   : SPLIT/JMP primary target          [nstate]   */
    int32_t  *y;        /* y[i]   : SPLIT secondary target            [nstate]   */
    int32_t  *cls;      /* cls[i] : class id for CONSUME, else -1     [nstate]   */

    int       ncls;     /* number of distinct 256-bit character sets            */
    uint64_t *sets;     /* ncls*4 words: a 256-bit membership set per class.
                         *   Byte b is in class k  <=>  bit b of sets[k*4 ..].
                         *   256 bits = 4 * 64-bit words. Literal 'a', '.', and
                         *   '[a-z]' all compile to one such set — uniform, and
                         *   it makes the match test a branch-free shift+AND. */

    int       start;    /* index of the start state (always 0)                  */
    int       match;    /* index of the OP_MATCH state (always nstate-1)        */
} Prog;

/* Reusable per-match scratch: two "thread sets" plus a work stack for the
 * epsilon-closure. Sized once for a given Prog and reused across many inputs, so
 * the hot path performs ZERO allocation — the matcher never calls malloc. This
 * matters for a server matching one pattern against millions of lines. */
typedef struct {
    int       nwords;   /* bitset width in 64-bit words = ceil(nstate/64)       */
    int       nstate;   /* mirror of Prog.nstate, for bounds/asserts            */
    uint64_t *cur;      /* the current active state set (bitset)                */
    uint64_t *seed;     /* raw successors of one step, BEFORE epsilon-closure   */
    uint64_t *next;     /* the closed successor set (becomes `cur` next round)   */
    int32_t  *stack;    /* explicit stack for iterative closure (no recursion)  */
} Scratch;

/* Parse-error report. On failure re_compile() fills this in (if non-NULL) with a
 * human message and the byte offset in the pattern where parsing gave up. */
typedef struct {
    int  ok;            /* 1 = success, 0 = error                               */
    int  offset;        /* byte offset into the pattern of the offending char   */
    char msg[64];       /* short, fixed-size, no allocation on the error path    */
} RegexError;

/* --- API ------------------------------------------------------------------- */

/* Compile a NUL-terminated pattern into a Prog. Returns NULL on a syntax error
 * (and fills *err if err != NULL). The returned Prog is owned by the caller. */
Prog *re_compile(const char *pattern, RegexError *err);

/* Release a Prog and all arrays it owns. Safe on NULL. */
void  re_free(Prog *p);

/* Allocate scratch sized for `p`; reuse it across many match calls. NULL on OOM. */
Scratch *scratch_new(const Prog *p);
void     scratch_free(Scratch *s);

/* ANCHORED match: does the ENTIRE text[0..n) match the pattern? (like ^pat$)
 * Returns 1 on match, 0 otherwise. O(n * m), allocation-free, never recurses. */
int re_fullmatch(const Prog *p, Scratch *s, const char *text, size_t n);

/* UNANCHORED search: does SOME substring of text[0..n) match? (like grep)
 * Returns 1 if a match exists anywhere, 0 otherwise. Same O(n * m) guarantee. */
int re_contains(const Prog *p, Scratch *s, const char *text, size_t n);

/* ---------------------------------------------------------------------------
 * nfa_step — the pure "state-set transition" inner loop, exposed for tests and
 * mirrored verbatim (with self-contained types) in asm/demo.c for the annotated
 * assembly. Given the current active set `cur`, it computes the RAW successor
 * set `next` for one input byte `c`: for every active CONSUME state whose 256-
 * bit set contains `c`, it marks that state's successor (pc+1). Epsilon-closure
 * is applied by the caller — keeping THIS function a branch-light bitset kernel
 * is precisely what makes it good assembly to study.
 *
 * Preconditions: `next` is pre-cleared to all zeros; nwords == ceil(nstate/64);
 * 0 <= c <= 255. It reads op/cls/sets from the Prog and writes only `next`.
 * --------------------------------------------------------------------------- */
void nfa_step(const uint8_t  *op,    /* op[i]  : opcode of state i               */
              const int32_t  *cls,   /* cls[i] : class id for CONSUME states     */
              const uint64_t *sets,  /* 256-bit membership sets, 4 words each    */
              const uint64_t *cur,   /* current active set (read)                */
              uint64_t       *next,  /* raw successor set (write; pre-cleared)   */
              int             nwords,/* bitset width in 64-bit words             */
              int             c);    /* input byte, 0..255                       */

#endif /* REGEX_H */
