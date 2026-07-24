/* ===========================================================================
 * asm/demo.c — THE AFL HEART, extracted for the assembly walkthrough.
 * ===========================================================================
 *
 * This file is a SELF-CONTAINED slice of the fuzzer's coverage machinery, with
 * NO system headers and its OWN integer types, so it compiles to clean, legible
 * assembly on any host (clang cross-targets Linux). It is the routine the
 * project brief calls "the AFL heart": hash the edge (prev_loc ^ cur_loc), index
 * the 64 KiB bitmap, and bucket the hit count.
 *
 * Three pure functions, each a few instructions of asm you can read end to end:
 *
 *   cov_update()      the per-edge update the instrumented target runs millions
 *                     of times a second. This is the hot path of ALL of AFL.
 *   classify_count()  fold a raw 0..255 hit count into a hit-count CLASS bit,
 *                     so "hit 19 times" and "hit 20 times" are not treated as
 *                     different behaviours but "hit twice" vs "hit 8 times" are.
 *   has_new_bits()    the feedback decision: did this run touch a class bit no
 *                     previous run did? That single bit of information is what
 *                     makes a fuzzer "coverage-guided" instead of random.
 *
 * These mirror rt.c (target side) and fuzzer.c (driver side); here they stand
 * alone so the generated asm is about the ALGORITHM, not I/O or syscalls.
 *
 * Generate the committed assembly (exactly as CONVENTIONS.md prescribes):
 *   clang --target=x86_64-pc-linux-gnu -S -O0 -fno-asynchronous-unwind-tables \
 *         -fno-jump-tables -fno-omit-frame-pointer demo.c -o demo.O0.s
 *   clang --target=x86_64-pc-linux-gnu -S -O1 -fno-asynchronous-unwind-tables \
 *         -fno-jump-tables -fno-omit-frame-pointer demo.c -o demo.s
 *   clang --target=x86_64-pc-linux-gnu -S -O2 -fno-asynchronous-unwind-tables \
 *         -fno-jump-tables demo.c -o demo.O2.s
 * Then read demo.annotated.s for the line-by-line explanation.
 * ===========================================================================
 */

/* ---- our own fixed-width types (no <stdint.h>) --------------------------- */
typedef unsigned char      u8;    /* one coverage-map byte / hit counter       */
typedef unsigned int       u32;   /* an edge id / map index (32-bit)           */
typedef unsigned long      usize; /* a byte count / loop bound (64-bit on LP64)*/

/* The map is a power of two so "mod MAP_SIZE" becomes one AND with the mask.
 * That AND is on the hottest path in the whole fuzzer — it runs on every edge —
 * so shaving it from a division to a single-cycle AND is a real win. */
#define MAP_SIZE_POW2 16
#define MAP_SIZE      (1u << MAP_SIZE_POW2)   /* 65536                          */
#define MAP_MASK      (MAP_SIZE - 1u)         /* 0xFFFF                         */

/* ---------------------------------------------------------------------------
 * cov_update — the per-edge coverage update. Returns the NEW prev_loc.
 *
 * Called once at the top of every instrumented basic block. `cur_loc` is this
 * block's compile-assigned id; `prev_loc` is the previous block's id already
 * shifted right by one (see the return). Together (prev, cur) name a control-
 * flow EDGE. We fold that edge into a single 16-bit bucket and bump its counter.
 *
 *   idx      = (cur_loc ^ prev_loc) & MAP_MASK      // edge -> bucket
 *   map[idx] = saturating_inc(map[idx])             // count the hit
 *   return   cur_loc >> 1                            // becomes next prev_loc
 *
 * The `>> 1` on the way out is AFL's key trick and earns its keep twice:
 *   - it breaks XOR's symmetry so edge A->B and edge B->A get DIFFERENT buckets
 *     (without it, A^B == B^A would merge the two directions of a branch);
 *   - it keeps self-loops A->A visible: A ^ (A>>1) != 0, whereas A^A == 0 would
 *     dump every tight loop into bucket 0.
 *
 * We saturate at 255 rather than let the byte wrap 255->0 (classic AFL's known
 * wart, where an edge hit exactly 256 times reads back as "never hit").
 * --------------------------------------------------------------------------- */
u32 cov_update(u8 *map, u32 prev_loc, u32 cur_loc)
{
    u32 idx = (cur_loc ^ prev_loc) & MAP_MASK;   /* fold edge into 0..65535    */

    if (map[idx] != 255u)                        /* saturating increment       */
        map[idx] = (u8)(map[idx] + 1u);

    return cur_loc >> 1;                          /* next iteration's prev_loc  */
}

/* ---------------------------------------------------------------------------
 * classify_count — map a raw hit count (0..255) to a single hit-count CLASS bit.
 *
 * Raw counts are too jittery to compare run-to-run: a loop that spun 19 vs 20
 * times is not "new behaviour", but 1 vs 2 vs 8 iterations often IS (it can mean
 * a different branch fired). AFL buckets counts into power-of-two ranges and
 * represents each range as one bit, so downstream code compares CLASSES, not
 * exact counts. The one-hot encoding lets has_new_bits() OR/AND them cheaply.
 *
 * Buckets:  0 -> 0   1 -> 1   2 -> 2   3 -> 4   4..7 -> 8   8..15 -> 16
 *          16..31 -> 32   32..127 -> 64   128..255 -> 128
 * --------------------------------------------------------------------------- */
u8 classify_count(u8 count)
{
    if (count == 0)   return 0;
    if (count == 1)   return 1;
    if (count == 2)   return 2;
    if (count == 3)   return 4;
    if (count <= 7)   return 8;
    if (count <= 15)  return 16;
    if (count <= 31)  return 32;
    if (count <= 127) return 64;
    return 128;
}

/* ---------------------------------------------------------------------------
 * has_new_bits — the coverage-feedback decision, over a whole map.
 *
 * `virgin` starts all-0xFF (every class bit of every edge still unseen). For
 * each edge we classify this run's raw count, then ask: does this run set a
 * class bit that virgin still marks unseen? If so it reached genuinely NEW
 * behaviour: we clear that bit in virgin (now "seen") and remember to return 1.
 * The caller keeps any input for which this returns 1 — that IS the corpus
 * growth rule that steers the whole search.
 *
 * Note we keep scanning after the first hit so virgin fully absorbs everything
 * new THIS run discovered (a run can open several edges at once).
 * --------------------------------------------------------------------------- */
int has_new_bits(u8 *virgin, const u8 *trace_raw, usize map_size)
{
    int is_new = 0;

    for (usize i = 0; i < map_size; i++) {
        u8 cls = classify_count(trace_raw[i]);   /* raw count -> class bit      */
        if (cls == 0) continue;                  /* edge not taken this run     */

        /* Bit set in `cls` AND still virgin (1) in `virgin[i]` == first sighting
         * of this class for this edge. */
        if (virgin[i] & cls) {
            virgin[i] = (u8)(virgin[i] & ~cls);  /* mark it seen                */
            is_new = 1;
        }
    }
    return is_new;
}

/* ---------------------------------------------------------------------------
 * demo_selftest — a tiny, allocation-free driver so the file is a complete,
 * runnable translation unit and the generated asm shows the routines in use.
 * It simulates a 4-edge "trace" (blocks 10 -> 20 -> 20 -> 30, i.e. a self-loop
 * on block 20), then checks how much new coverage a fresh virgin map reports.
 * Returns a small integer checksum; no I/O, no headers, verifiable by eye.
 * --------------------------------------------------------------------------- */
int demo_selftest(void)
{
    /* A stand-in coverage map and virgin map, sized down to keep the asm short
     * while exercising the exact same code paths as the 64 KiB real thing. */
    static u8 map[MAP_SIZE];
    static u8 virgin[MAP_SIZE];

    usize i;
    for (i = 0; i < MAP_SIZE; i++) { map[i] = 0; virgin[i] = 0xFFu; }

    /* Walk the edges 10->20->20->30 through cov_update, threading prev_loc. */
    u32 prev = 0;
    prev = cov_update(map, prev, 10u);   /* edge (0,10)                        */
    prev = cov_update(map, prev, 20u);   /* edge (10>>1, 20)                   */
    prev = cov_update(map, prev, 20u);   /* self-loop edge on 20               */
    prev = cov_update(map, prev, 30u);   /* edge (20>>1, 30)                   */

    /* Ask the feedback function how many distinct new class bits this trace
     * opened. First call should report "new"; a second identical call should
     * report "nothing new" because virgin already absorbed them. */
    int first  = has_new_bits(virgin, map, MAP_SIZE);
    int second = has_new_bits(virgin, map, MAP_SIZE);

    /* Expected: first == 1 (new coverage), second == 0 (saturated). Encode both
     * plus the final prev_loc into a checksum the reader can predict. */
    return (first << 1) | second | (int)(prev << 2);
}
