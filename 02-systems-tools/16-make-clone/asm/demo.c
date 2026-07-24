/* ===========================================================================
 * demo.c — a SELF-CONTAINED extraction of mmake's pure-logic core, written to
 * be turned into readable teaching assembly.
 * ===========================================================================
 *
 * It has NO #includes and declares its own types, so clang can compile it
 * straight to x86-64 Linux assembly on any host (no libc, no headers needed):
 *
 *   clang --target=x86_64-pc-linux-gnu -S -O0 ... asm/demo.c -o asm/demo.O0.s
 *   clang --target=x86_64-pc-linux-gnu -S -O1 ... asm/demo.c -o asm/demo.s
 *   clang --target=x86_64-pc-linux-gnu -S -O2 ... asm/demo.c -o asm/demo.O2.s
 *
 * It lifts, verbatim minus the headers, the two decisions that make(1) makes
 * that are interesting to the CPU rather than to the kernel:
 *
 *   1. mk_needs_rebuild — the STALENESS test: given a target's mtime and the
 *      mtimes/rebuild-flags of its prerequisites, decide whether to run the
 *      recipe. This is the function annotated instruction-by-instruction in
 *      demo.annotated.s. It is a LEAF (calls nothing) and — because it takes
 *      seven parameters — it is the perfect specimen for watching the SysV ABI
 *      spill the 7th argument onto the stack.  (Mirrors graph.c's compute().)
 *
 *   2. mk_toposort — order targets so every prerequisite is built before the
 *      target that needs it (Kahn's algorithm over a dense adjacency matrix).
 *      Returning a count < n signals a dependency CYCLE ("circular ... dropped").
 *      It is NOT a leaf (the compiler clears an array with memset), so it shows
 *      callee-saved registers and stack-alignment math.  (Mirrors graph.c.)
 *
 * With no syscalls and no libc, the emitted assembly is nothing but the SysV
 * AMD64 ABI, integer math, loads/stores, and branches — exactly what the
 * annotated file dissects.
 * ===========================================================================
 */

/* Our own fixed-width aliases, matching the LP64 model x86-64 Linux uses:
 * `long long` is 64 bits (holds an mtime-in-nanoseconds without truncation). */
typedef long long mk_time;

#define MAX_NODES 64      /* fixed cap so toposort can use stack scratch arrays */

/* ---------------------------------------------------------------------------
 * mk_needs_rebuild — the staleness decision, the heart of any make.
 *
 * Returns 1 (rebuild) if ANY of:
 *   - `always`         : the -B flag forces every target stale;
 *   - `is_phony`       : a .PHONY target has no file, so it always runs;
 *   - `target_missing` : the output file does not exist yet;
 *   - a prerequisite was itself rebuilt (prereq_rebuilt[i] != 0);
 *   - a prerequisite is strictly NEWER than the target (mtime compare).
 * Otherwise returns 0 (up to date).
 *
 * SysV AMD64 argument mapping (the star of the annotation):
 *   edi = always          esi = is_phony        edx = target_missing
 *   rcx = target_mtime     r8  = prereq_mtime[]   r9 = prereq_rebuilt[]
 *   [stack] = nprereq      -> the SEVENTH integer argument spills to memory,
 *                             because only six registers carry integer args.
 *   result in eax.
 * --------------------------------------------------------------------------- */
int mk_needs_rebuild(int always, int is_phony, int target_missing,
                     mk_time target_mtime,
                     const mk_time *prereq_mtime,
                     const int     *prereq_rebuilt,
                     int nprereq)
{
    /* The three cheap scalar reasons to rebuild, tested before touching memory.*/
    if (always || is_phony || target_missing)
        return 1;

    /* Otherwise scan the prerequisites: any newer, or any that will itself be
     * rebuilt, forces us stale. This loop is the mtime comparison core. */
    for (int i = 0; i < nprereq; i++) {
        if (prereq_rebuilt[i])                 /* a prereq is (will be) rebuilt   */
            return 1;
        if (prereq_mtime[i] > target_mtime)    /* a prereq is strictly newer      */
            return 1;
    }
    return 0;                                  /* nothing newer: up to date       */
}

/* ---------------------------------------------------------------------------
 * mk_toposort — Kahn's algorithm over a dense n*n adjacency matrix.
 *
 * edges[i*n + j] != 0 means "target i depends on prerequisite j", i.e. j must
 * be built before i. We repeatedly emit a node whose every prerequisite has
 * already been emitted (in-degree 0), then relax its dependents. Emitting the
 * lowest ready index each round makes the order deterministic. The return value
 * is the number of nodes ordered: < n means some nodes never became ready, the
 * signature of a dependency cycle.
 *
 * ABI: edi = n, rsi = edges, rdx = order; result in eax.
 * --------------------------------------------------------------------------- */
int mk_toposort(int n, const unsigned char *edges, int *order)
{
    unsigned char indeg[MAX_NODES];   /* count of unbuilt prerequisites per node */
    unsigned char emitted[MAX_NODES]; /* has this node been placed in `order`?   */

    int i, j;
    int out = 0;                       /* how many nodes we have ordered so far   */

    /* Phase 1: in-degree of each node = number of its prerequisites. */
    for (i = 0; i < n; i++) {
        int deg = 0;
        const unsigned char *row = edges + (long)i * (long)n;  /* &edges[i][0]    */
        for (j = 0; j < n; j++)
            deg += (row[j] != 0);      /* branchless-friendly: +1 per edge        */
        indeg[i]   = (unsigned char)deg;
        emitted[i] = 0;
    }

    /* Phase 2: repeatedly emit the lowest ready node, then relax its dependents.*/
    for (i = 0; i < n; i++) {          /* at most n emission rounds               */
        int pick = -1;
        for (j = 0; j < n; j++) {      /* find the lowest unemitted, ready node   */
            if (!emitted[j] && indeg[j] == 0) { pick = j; break; }
        }
        if (pick < 0)
            break;                     /* none ready but nodes remain: a CYCLE    */

        order[out++] = pick;           /* commit pick to the build order          */
        emitted[pick] = 1;             /* "remove" it from the graph              */

        for (j = 0; j < n; j++) {      /* every dependent of pick loses a prereq  */
            const unsigned char *row = edges + (long)j * (long)n;
            if (!emitted[j] && row[pick] != 0 && indeg[j] > 0)
                indeg[j]--;
        }
    }

    return out;                        /* out == n: full order; out < n: cycle    */
}
