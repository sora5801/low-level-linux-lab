/* ===========================================================================
 * order.c — dependency ordering + restart backoff. Pure logic, zero syscalls.
 * ===========================================================================
 *
 * This is the algorithmic heart of the supervisor, factored out of the syscall-
 * heavy event loop so that (a) it can be reasoned about and tested in isolation
 * and (b) it compiles straight to Linux assembly with no headers in the way.
 * See order.h for the contract and the WHY behind the header-free design.
 * ===========================================================================
 */
#include "order.h"

/* ---------------------------------------------------------------------------
 * sup_toposort — Kahn's algorithm over a dense adjacency matrix.
 *
 * We keep two fixed-size scratch arrays on the stack (no malloc — the supervisor
 * avoids the heap so that even under memory pressure PID 1 cannot fail an
 * allocation and wedge the whole system):
 *
 *   indeg[i]  : how many prerequisites of service i have NOT yet been emitted.
 *   emitted[i]: 1 once service i has been placed into `order`, else 0. This
 *               stands in for Kahn's "remove the node from the graph" step.
 *
 * Both are SUP_MAX_NODES wide; the caller guarantees n <= SUP_MAX_NODES, so the
 * indices below are always in range. That guarantee is the invariant that keeps
 * these raw array writes sound.
 * --------------------------------------------------------------------------- */
int sup_toposort(int n, const unsigned char *edges, int *order)
{
    unsigned char indeg[SUP_MAX_NODES];   /* unmet-prereq count per node        */
    unsigned char emitted[SUP_MAX_NODES]; /* has this node been output yet?     */

    int i, j;
    int out = 0;                          /* how many nodes we have emitted      */

    /* --- Phase 1: compute in-degrees ------------------------------------- *
     * indeg[i] = number of j such that edges[i*n + j] != 0, i.e. the count of
     * i's prerequisites. A node with indeg 0 has nothing to wait for and can be
     * started immediately. We also clear `emitted`. */
    for (i = 0; i < n; i++) {
        int deg = 0;
        const unsigned char *row = edges + (sup_u64)i * (sup_u64)n; /* &edges[i][0] */
        for (j = 0; j < n; j++)
            deg += (row[j] != 0);         /* branchless: add 1 when an edge exists */
        indeg[i] = (unsigned char)deg;
        emitted[i] = 0;
    }

    /* --- Phase 2: repeatedly emit a zero-in-degree node ------------------ *
     * Classic Kahn: scan for any not-yet-emitted node whose in-degree is 0,
     * emit it, then "delete" it by decrementing the in-degree of every node
     * that depended on it. We loop up to n times because each pass emits at
     * least one node UNLESS none is ready — and "none ready while nodes remain"
     * is exactly the signature of a cycle, which breaks us out early.
     *
     * Emitting the LOWEST ready index each pass makes the order deterministic
     * (stable across runs), which matters for a config file a human reads. */
    for (i = 0; i < n; i++) {             /* at most n emission rounds           */
        int pick = -1;

        for (j = 0; j < n; j++) {         /* find the lowest ready node          */
            if (!emitted[j] && indeg[j] == 0) {
                pick = j;
                break;
            }
        }

        if (pick < 0)
            break;                        /* nothing ready but nodes remain: CYCLE */

        order[out++] = pick;              /* commit pick to the start order      */
        emitted[pick] = 1;               /* remove it from the graph            */

        /* Relax every dependent of `pick`: any k with an edge k->pick loses one
         * unmet prerequisite. When k's in-degree hits 0 it becomes eligible on
         * a later pass. */
        for (j = 0; j < n; j++) {
            const unsigned char *row = edges + (sup_u64)j * (sup_u64)n;
            if (!emitted[j] && row[pick] != 0 && indeg[j] > 0)
                indeg[j]--;
        }
    }

    /* out == n  => full valid ordering.
     * out <  n  => a cycle left some nodes permanently blocked; the caller
     *              reports the config as unstartable. */
    return out;
}

/* ---------------------------------------------------------------------------
 * sup_backoff_delay_ms — delay = min(base_ms << restart_count, cap_ms).
 *
 * Two hazards this code guards against, both classic:
 *   1. Shift overflow / UB. `x << 64` (or more) is UNDEFINED BEHAVIOUR in C for
 *      a 64-bit x. We clamp the shift amount to 63 so the shift is always well
 *      defined. Beyond a handful of doublings we are past cap_ms anyway.
 *   2. Numeric overflow before the clamp. Doubling base_ms enough times will
 *      wrap a 64-bit value around to a small number, which would *shorten* the
 *      delay — the opposite of what we want. We detect "already >= cap" and
 *      "the shift lost bits" and saturate to cap_ms.
 * --------------------------------------------------------------------------- */
sup_u64 sup_backoff_delay_ms(unsigned restart_count, sup_u64 base_ms, sup_u64 cap_ms)
{
    sup_u64 delay = base_ms;
    unsigned shift = restart_count;

    /* Clamp the shift so `<<` is always defined (0..63 on a 64-bit type). */
    if (shift > 63u)
        shift = 63u;

    /* Double `delay` one bit at a time, saturating the moment we reach or exceed
     * the cap. Doing it in a loop (rather than one big shift) lets us stop early
     * and makes the overflow test trivial: if a doubling would push us past the
     * cap, we are done. This is the pure integer core the annotated asm dissects. */
    for (unsigned k = 0; k < shift; k++) {
        if (delay >= cap_ms)              /* already at the ceiling: stop        */
            return cap_ms;
        delay <<= 1;                      /* one doubling                        */
    }

    if (delay > cap_ms)                   /* final clamp after the last doubling */
        delay = cap_ms;
    return delay;
}
