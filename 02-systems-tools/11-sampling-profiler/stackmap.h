/* ===========================================================================
 * stackmap.h — aggregate raw callchains into counted, collapsed stacks.
 * ===========================================================================
 * The profiler fires thousands of samples; most land in the same few code paths.
 * A stackmap folds them: it symbolizes each callchain into a ';'-joined string
 * ("main;wl_compute;wl_hash") and counts identical strings. Dumping the table in
 * "stack<space>count" form yields exactly Brendan Gregg's "collapsed stack"
 * format, which flamegraph.pl turns into an SVG.
 * =========================================================================== */
#ifndef STACKMAP_H
#define STACKMAP_H

#include <stdint.h>     /* uint64_t */
#include <stdio.h>      /* FILE     */

struct stackmap;        /* opaque; the implementation owns the hash table       */

/* Create an empty map, or NULL on OOM. */
struct stackmap *stackmap_new(void);

/* Fold one callchain (innermost-first, `n` frames) into the map: symbolize each
 * frame, build the outermost-first collapsed key, and increment its count. This
 * is the `prof_stack_fn` both backends call. Safe to pass n==0 (ignored). */
void stackmap_add(void *sm, const uint64_t *ips, int n);

/* How many callchains have been folded in total (== samples attributed). */
unsigned long stackmap_total(const struct stackmap *sm);

/* Number of DISTINCT stacks recorded. */
unsigned long stackmap_unique(const struct stackmap *sm);

/* Write the collapsed-stack profile to `out`, one "stack count" line per unique
 * stack, sorted by count descending (ties broken by stack text) so the output is
 * deterministic and the hottest paths are on top. */
void stackmap_print_folded(struct stackmap *sm, FILE *out);

/* Release the table and every key it owns. */
void stackmap_free(struct stackmap *sm);

#endif /* STACKMAP_H */
