/* ===========================================================================
 * parse.c — Brainfuck source text -> optimized op[] IR.
 * ===========================================================================
 *
 * This is the shared front end for both back ends. It does three jobs:
 *
 *   1. LEXING: skip every byte that is not one of the eight commands (Brainfuck
 *      treats all other characters as comments), so whitespace and prose cost
 *      nothing at run time.
 *
 *   2. PEEPHOLE FOLDING (a first, source-level optimizer pass):
 *        - runs of '+'/'-' collapse to one OP_ADD with a net byte delta;
 *        - runs of '>'/'<' collapse to one OP_MOVE with a net signed delta;
 *        - runs of '.' collapse to one OP_OUT with a repeat count;
 *        - the idiom "[-]" / "[+]" (decrement/increment a cell until it hits 0)
 *          collapses to a single OP_CLEAR (set the cell to 0). This is the most
 *          famous Brainfuck peephole; it turns an O(cell value) loop into one
 *          store, and both back ends benefit.
 *      Folding shrinks the IR dramatically and is *why* the JIT's generated
 *      code is compact — fewer, wider instructions instead of one per source
 *      character.
 *
 *   3. BRACKET MATCHING: pair every '[' with its ']' and store, in each JZ/JNZ,
 *      the IR index to jump to. An explicit stack (not recursion) does this in
 *      one pass and lets us report unmatched brackets precisely.
 * ===========================================================================
 */
#include "bf.h"

#include <stdlib.h>   /* malloc, realloc, free            */
#include <string.h>   /* memcpy for the error string      */
#include <stdio.h>    /* snprintf for error messages      */

/* Grow-on-demand vector of ops. We do not know the final op count up front
 * (folding makes it smaller than the source, but by an unknown amount), so we
 * start modest and double. Doubling gives amortized O(1) appends: across the
 * whole parse the total copying is < 2x the final size. */
typedef struct {
    op    *v;
    size_t n;    /* used     */
    size_t cap;  /* allocated */
} opvec;

static int opvec_push(opvec *ov, op x)
{
    if (ov->n == ov->cap) {
        size_t ncap = ov->cap ? ov->cap * 2 : 64;
        /* realloc may move the block; the old pointer is dead afterward. We
         * assign only on success so a failed grow leaves the vector intact and
         * the caller can free what it already has. */
        op *nv = realloc(ov->v, ncap * sizeof *nv);
        if (!nv) return -1;               /* OOM: report up, do not leak */
        ov->v = nv;
        ov->cap = ncap;
    }
    ov->v[ov->n++] = x;
    return 0;
}

/* Bracket-matching stack of IR indices (the position of each open '['). Its
 * maximum depth is the source's maximum bracket nesting, which cannot exceed
 * the source length, so we size it to src_len and never reallocate. */
int bf_parse(const char *src, size_t src_len, bf_program *out,
             char *err, size_t errlen)
{
    opvec ov = {0};
    size_t *stack = NULL;
    size_t sp = 0;          /* stack pointer / current nesting depth */
    int rc = -1;

    /* Worst case one bracket per byte; +1 avoids a zero-size malloc. */
    stack = malloc((src_len + 1) * sizeof *stack);
    if (!stack) { if (err) snprintf(err, errlen, "out of memory"); goto done; }

    for (size_t i = 0; i < src_len; ) {
        char ch = src[i];
        switch (ch) {

        /* ---- fold a run of cell arithmetic ('+'/'-') into one OP_ADD ------ */
        case '+':
        case '-': {
            int delta = 0;
            /* Walk while the next byte is '+' or '-', accumulating the net
             * change. We store it masked to 8 bits because cells are bytes:
             * e.g. "+++" over "-" nets +2, and "-" alone nets 255 (== -1 mod
             * 256). The interpreter and JIT both apply it as a byte add. */
            while (i < src_len && (src[i] == '+' || src[i] == '-')) {
                delta += (src[i] == '+') ? 1 : -1;
                i++;
            }
            delta &= 0xff;                       /* fold into 0..255            */
            if (delta != 0) {                    /* net-zero run is a true no-op */
                if (opvec_push(&ov, (op){OP_ADD, delta})) goto oom;
            }
            break;
        }

        /* ---- fold a run of pointer moves ('>'/'<') into one OP_MOVE -------- */
        case '>':
        case '<': {
            int delta = 0;
            while (i < src_len && (src[i] == '>' || src[i] == '<')) {
                delta += (src[i] == '>') ? 1 : -1;
                i++;
            }
            if (delta != 0) {
                if (opvec_push(&ov, (op){OP_MOVE, delta})) goto oom;
            }
            break;
        }

        /* ---- fold a run of output ('.') into one OP_OUT with a count ------- */
        case '.': {
            int count = 0;
            while (i < src_len && src[i] == '.') { count++; i++; }
            if (opvec_push(&ov, (op){OP_OUT, count})) goto oom;
            break;
        }

        case ',':
            if (opvec_push(&ov, (op){OP_IN, 1})) goto oom;
            i++;
            break;

        /* ---- '[' : peephole "[-]"/"[+]" -> OP_CLEAR, else emit OP_JZ ------- */
        case '[': {
            /* Look ahead for the exact 3-byte idiom "[-]" or "[+]". If found,
             * the whole loop is equivalent to "set cell = 0", so we emit a
             * single OP_CLEAR and skip all three characters. This is safe
             * because the loop body is exactly one decrement/increment: it runs
             * (cell) times and terminates with cell==0 regardless of start. */
            if (i + 2 < src_len && (src[i+1] == '-' || src[i+1] == '+')
                                 &&  src[i+2] == ']') {
                if (opvec_push(&ov, (op){OP_CLEAR, 0})) goto oom;
                i += 3;
                break;
            }
            /* Otherwise a real loop: push this JZ's index so the matching ']'
             * can back-fill our jump target, and vice versa. arg is filled in
             * when we see the ']'. */
            stack[sp++] = ov.n;
            if (opvec_push(&ov, (op){OP_JZ, 0})) goto oom;
            i++;
            break;
        }

        case ']': {
            if (sp == 0) {                       /* ']' with no open '[' */
                if (err) snprintf(err, errlen,
                    "unmatched ']' at byte %zu", i);
                goto done;
            }
            size_t open = stack[--sp];           /* index of the matching '[' */
            if (opvec_push(&ov, (op){OP_JNZ, 0})) goto oom;
            /* Wire the pair together. Interpreter semantics:
             *   JZ  jumps to the op AFTER the ']'  (index of JNZ + 1),
             *   JNZ jumps to the op AFTER the '['  (index of JZ  + 1).
             * The +1 avoids re-testing the cell we just compared. */
            size_t close = ov.n - 1;             /* index of the JNZ we just pushed */
            ov.v[open].arg  = (int)(close + 1);
            ov.v[close].arg = (int)(open + 1);
            i++;
            break;
        }

        default:
            /* Any other byte is a comment in Brainfuck: skip it, no op emitted. */
            i++;
            break;
        }
    }

    if (sp != 0) {                                /* leftover '[' with no ']' */
        if (err) snprintf(err, errlen,
            "unmatched '[' (%zu still open at end of input)", sp);
        goto done;
    }

    /* Terminate the IR with an explicit HALT so both back ends have a single,
     * unambiguous stopping op rather than relying on the array length. */
    if (opvec_push(&ov, (op){OP_HALT, 0})) goto oom;

    out->ops = ov.v;
    out->n   = ov.n;
    ov.v = NULL;         /* ownership transferred to the caller */
    rc = 0;
    goto done;

oom:
    if (err) snprintf(err, errlen, "out of memory building IR");
done:
    free(stack);
    free(ov.v);          /* NULL after a successful hand-off, so this is a no-op then */
    return rc;
}

void bf_free_program(bf_program *p)
{
    if (!p) return;
    free(p->ops);
    p->ops = NULL;
    p->n = 0;
}
