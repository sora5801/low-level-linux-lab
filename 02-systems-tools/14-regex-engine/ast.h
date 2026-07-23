/* ===========================================================================
 * ast.h — the abstract syntax tree the parser produces and the compiler eats.
 * ===========================================================================
 *
 * The grammar this AST encodes (standard regex precedence, lowest to highest):
 *
 *     alternation  := concatenation ('|' concatenation)*      -- N_ALT
 *     concatenation:= repetition*                             -- N_CONCAT / N_EMPTY
 *     repetition   := atom ('*' | '+' | '?')*                 -- N_STAR/PLUS/QUEST
 *     atom         := '(' alternation ')' | class | '.' | literal
 *
 * Every "atom that consumes a byte" — a literal, '.', an escape like \d, or a
 * bracket class [a-z] — collapses to the SAME node kind, N_SET, carrying a
 * 256-bit membership bitmap. Unifying them means the NFA compiler has exactly
 * one consuming construct to handle, and the simulator has exactly one match
 * test. That is a deliberate simplification that costs nothing and removes a
 * whole category of special cases.
 * ===========================================================================
 */
#ifndef AST_H
#define AST_H

#include "regex.h"   /* RegexError, and the uint8_t/int types via <stdint.h>    */

enum {
    N_SET,     /* consumes one byte; `set` is its 256-bit membership bitmap      */
    N_EMPTY,   /* the empty string (epsilon); matches without consuming input    */
    N_CONCAT,  /* a followed by b        (children a, b)                         */
    N_ALT,     /* a OR b                 (children a, b)                         */
    N_STAR,    /* a repeated 0+ times    (child  a)                              */
    N_PLUS,    /* a repeated 1+ times    (child  a)                              */
    N_QUEST    /* a optional (0 or 1)    (child  a)                              */
};

/* One AST node. Binary ops use a and b; unary ops use a; N_SET uses `set`. */
typedef struct Node {
    int          kind;    /* one of the N_* tags above                          */
    struct Node *a, *b;   /* children (interpretation depends on kind)          */
    uint8_t      set[32]; /* 256-bit byte set, valid only when kind == N_SET    */
} Node;

/* ---------------------------------------------------------------------------
 * Arena allocator for AST nodes.
 *
 * WHY AN ARENA: an AST is a throwaway intermediate — we build it, compile it to
 * bytecode, and drop the whole thing. Individually malloc/free-ing dozens of
 * tiny nodes (and carefully freeing them on every parse-error path) is fiddly
 * and leak-prone. Instead we bump-allocate all nodes from ONE contiguous block
 * whose size is bounded by the pattern length, and reclaim EVERYTHING with a
 * single free(). The block never grows, so node pointers never move — which is
 * exactly why we can store real `Node*` child pointers instead of indices.
 *
 * The bound: parsing produces O(len) nodes (each source character contributes a
 * constant number). We reserve 6*len + 16 slots, which is comfortably above the
 * true maximum, and the allocator refuses to exceed it (returns NULL) rather
 * than risk an overflow — a NULL node propagates out as a parse failure.
 * --------------------------------------------------------------------------- */
typedef struct {
    Node *pool;   /* one contiguous array of nodes; stable addresses            */
    int   n;      /* next free slot (bump pointer)                              */
    int   cap;    /* total slots; n never exceeds this                          */
} Arena;

/* Parse `pat` (NUL-terminated) into an AST allocated from `arena`. On success
 * returns the root node and sets err->ok = 1. On a syntax error returns NULL and
 * fills *err (message + offset). The caller owns arena->pool and frees it. */
Node *parse(const char *pat, Arena *arena, RegexError *err);

#endif /* AST_H */
