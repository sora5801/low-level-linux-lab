/* ===========================================================================
 * rt.c — the instrumentation RUNTIME that gets linked into every target.
 * ===========================================================================
 *
 * This is the target-side half of the fuzzer. It is the analogue of AFL's
 * `afl-compiler-rt.o`: a tiny library the instrumented program links against.
 * It does three jobs:
 *
 *   (1) Provide the callbacks the compiler's `-fsanitize-coverage=trace-pc-guard`
 *       instrumentation calls at every control-flow edge, and turn those calls
 *       into updates of the shared coverage bitmap. THIS is the "AFL heart":
 *       hash(prev_loc ^ cur_loc) -> index the map -> bump the hit count.
 *
 *   (2) Attach the shared-memory coverage bitmap the fuzzer created, so those
 *       updates are visible to the fuzzer in another process for free.
 *
 *   (3) Run the fork server: park after init, and fork a fresh child per test
 *       case on the fuzzer's command.
 *
 * CRUCIAL BUILD RULE: compile THIS file *without* `-fsanitize-coverage`.
 * If the runtime instrumented itself, every write()/read()/fork() in the fork
 * server loop would scribble into the coverage map and every input would look
 * like it found "new" coverage. The Makefile builds rt.c plain and parser.c
 * instrumented — read the Makefile to see the split.
 * ===========================================================================
 */

#include <stdint.h>     /* uint8_t, uint32_t — fixed-width, ABI-stable        */
#include <stdlib.h>     /* getenv, atoi                                       */
#include <unistd.h>     /* read, write, fork, _exit, close                    */
#include <string.h>     /* memset                                             */
#include <sys/shm.h>    /* shmat — attach the SysV shared-memory segment      */
#include <sys/wait.h>   /* waitpid, status macros                             */
#include <sys/types.h>  /* pid_t                                              */

#include "forkserver.h"

/* ---------------------------------------------------------------------------
 * The coverage bitmap pointer.
 *
 * Until the fuzzer attaches us to real shared memory, this points at a private
 * fallback buffer (`__afl_area_initial`) so the target can still run *standalone*
 * — e.g. `./parser crash_input` to reproduce a saved crash outside the fuzzer.
 * When run under the fuzzer, __afl_start_forkserver()'s init repoints it at the
 * shared segment. AFL uses this exact "initial area then swap to shm" pattern.
 * --------------------------------------------------------------------------- */
static uint8_t  __afl_area_initial[MAP_SIZE];
static uint8_t *__afl_area_ptr = __afl_area_initial;

/* prev_loc: the second half of the AFL edge hash. It holds the *previous* basic
 * block's id (already shifted right by one — see below). It is deliberately a
 * plain global, NOT thread-local: this teaching core assumes a single-threaded
 * target, exactly as classic AFL does. A multithreaded target would race on
 * this and smear its coverage; AFL++ offers a thread-local mode for that. */
static uint32_t __afl_prev_loc = 0;

/* ---------------------------------------------------------------------------
 * __sanitizer_cov_trace_pc_guard_init(start, stop)
 *
 * The compiler emits, per translation unit, an array of 32-bit "guard" slots —
 * one per instrumented edge — and a call to THIS function at program startup
 * with [start, stop) bracketing that array. Our job: hand every guard a unique
 * small integer id. That id becomes the block's `cur_loc` in the edge hash.
 *
 * Classic AFL picks a *random* compile-time constant per block; trace-pc-guard
 * instead lets the runtime number them, so we just count 1,2,3,.... Sequential
 * ids are fine here — the XOR-with-prev_loc + the >>1 shift (below) still spread
 * edges across the map. We start at 1 and never hand out 0, so a guard that is
 * still 0 means "compiler never assigned it" (belt-and-suspenders).
 * --------------------------------------------------------------------------- */
void __sanitizer_cov_trace_pc_guard_init(uint32_t *start, uint32_t *stop)
{
    static uint32_t next_id = 1;    /* persists across multiple init calls    */

    /* The loader may call this once per linked object. `start == stop` is an
     * empty range (nothing to do); a nonzero *start means we already numbered
     * this array on a previous call, so bail to stay idempotent. */
    if (start == stop || *start) return;

    for (uint32_t *g = start; g < stop; g++) {
        /* Assign this edge its id. We keep ids inside the map so the later
         * mask is a formality, not a truncation that silently aliases blocks. */
        *g = next_id++;
        if (next_id >= MAP_SIZE) next_id = 1;  /* wrap; 0 stays reserved      */
    }
}

/* ---------------------------------------------------------------------------
 * __sanitizer_cov_trace_pc_guard(guard)  —  THE AFL HEART.
 *
 * The compiler inserts a call to this at the top of every instrumented basic
 * block, passing &guard for THIS block. On entry *guard is the block's id
 * (assigned above). We convert the pair (previous block, this block) — i.e. one
 * CONTROL-FLOW EDGE — into a single bucket in the 64 KiB map and bump it.
 *
 * Why hash an EDGE and not just the block? Block coverage ("did we reach line
 * L?") is far weaker than edge coverage ("did we take branch A->B?"). Loops,
 * error paths, and state machines are all about *which transitions* fire, and
 * two inputs can hit the same set of blocks via different edges. AFL's headline
 * insight was that cheap edge coverage finds dramatically more bugs than block
 * coverage for the same cost. This function is that insight in five lines.
 *
 * The exact recurrence (identical in spirit to AFL's afl-as.h):
 *     idx        = (cur_loc ^ prev_loc) & (MAP_SIZE-1)
 *     map[idx]  += 1                       (saturating, see note)
 *     prev_loc   = cur_loc >> 1
 *
 * Why `prev_loc = cur_loc >> 1` and not just `cur_loc`?  Two reasons, both
 * essential:
 *   - DIRECTIONALITY: without the shift, edge A->B and edge B->A hash to the
 *     same bucket (XOR is commutative: A^B == B^A). Shifting one operand breaks
 *     that symmetry so A->B and B->A land in different buckets. Direction of a
 *     branch is exactly the signal a fuzzer needs.
 *   - SELF-LOOPS: a tight loop is the edge A->A. Plain A^A == 0 would jam every
 *     self-loop into bucket 0 and make them invisible. With the shift the bucket
 *     is A ^ (A>>1) != 0, so self-loops register.
 * --------------------------------------------------------------------------- */
void __sanitizer_cov_trace_pc_guard(uint32_t *guard)
{
    uint32_t cur = *guard;                       /* this block's id = cur_loc */

    /* Fold the edge (prev, cur) into a map index. The AND is the whole reason
     * MAP_SIZE is a power of two: it is one cheap instruction instead of a div. */
    uint32_t idx = (cur ^ __afl_prev_loc) & (MAP_SIZE - 1);

    /* Bump the hit counter for this edge. We saturate at 255 rather than let it
     * wrap 255->0 like classic AFL does. AFL's wrap is a known wart: an edge hit
     * exactly 256 times reads back as 0 ("never covered") and can hide a path.
     * Saturating is what AFL++ does and keeps has_new_bits() honest. The raw
     * per-run counts are later bucketed into hit-count *classes* on the fuzzer
     * side (see asm/demo.c count_class_lookup8). */
    if (__afl_area_ptr[idx] < 255) __afl_area_ptr[idx]++;

    /* Remember this block (shifted) as prev_loc for the NEXT edge. */
    __afl_prev_loc = cur >> 1;
}

/* ---------------------------------------------------------------------------
 * attach_shared_bitmap — swap our fallback buffer for the fuzzer's shm segment.
 *
 * Reads the decimal SysV shm id the fuzzer stashed in the environment, attaches
 * it read/write into our address space with shmat(2), and repoints the coverage
 * pointer at it. From now on every trace_pc_guard write lands in memory the
 * fuzzer can read directly. If the env var is absent we are NOT under the fuzzer
 * (someone ran the target by hand) — leave the fallback buffer in place.
 * --------------------------------------------------------------------------- */
static void attach_shared_bitmap(void)
{
    char *id_str = getenv(SHM_ENV_VAR);
    if (!id_str) return;                 /* standalone run: keep private area  */

    int shm_id = atoi(id_str);

    /* shmat(2): attach SysV segment `shm_id` at a kernel-chosen address (NULL),
     * flags 0 = read/write. Returns the attach address, or (void*)-1 on error.
     * The mapping is inherited across fork(), so every child we spawn sees the
     * same physical pages without re-attaching. */
    void *area = shmat(shm_id, NULL, 0);
    if (area == (void *)-1) _exit(1);    /* misconfigured; fail loud, not silent*/

    __afl_area_ptr = (uint8_t *)area;
}

/* ---------------------------------------------------------------------------
 * __afl_start_forkserver — become the fork server (or run once, standalone).
 *
 * Called once from the target's main() AFTER all expensive one-time init but
 * BEFORE it reads the fuzz input. That placement is the whole game: everything
 * above this line runs exactly once; everything below (the input read + parse)
 * runs fresh in each forked child.
 *
 * THE CLASSIC AFL FORK-SERVER PROTOCOL (must match fuzzer.c byte for byte):
 *   1. Server -> fuzzer:  write 4-byte HELLO on ST_FD ("I'm alive").
 *   2. loop forever:
 *        a. fuzzer -> server: read 4 bytes on CTL_FD (the "go" command; its
 *           value is ignored here — its ARRIVAL is the signal to run one case).
 *        b. fork().
 *        c. child:  close the control fds, reset prev_loc, RETURN so main()
 *                   continues into "read input; parse it; exit".
 *        d. parent: write the child's pid (4 bytes) on ST_FD, waitpid() for it,
 *                   then write the child's raw wait `status` (4 bytes) on ST_FD.
 *                   The fuzzer decodes that status to tell crash from clean exit.
 * --------------------------------------------------------------------------- */
void __afl_start_forkserver(void)
{
    /* Do the shm attach here, lazily, right before we park. (It could also live
     * in the guard_init constructor; doing it here keeps all the fuzzer-coupling
     * in one function that's easy to read.) */
    attach_shared_bitmap();

    uint32_t hello = FORKSRV_HELLO;

    /* Step 1: announce we are ready. If ST_FD isn't a real pipe (we were run by
     * hand, not by the fuzzer), this write fails with EBADF — which we take as
     * "not under a fuzzer" and simply return, letting main() run one time
     * normally. This is what makes `./parser somefile` work for crash repro. */
    if (write(FORKSRV_ST_FD, &hello, 4) != 4) return;

    while (1) {
        uint32_t cmd;

        /* Step 2a: block until the fuzzer says "run one". A 0-byte read means
         * the fuzzer closed the pipe (it exited) — so we exit too. */
        if (read(FORKSRV_CTL_FD, &cmd, 4) != 4) _exit(0);

        /* Step 2b: fork. The child is a copy-on-write snapshot of us taken at
         * this exact, fully-initialised point — cheap to create, identical
         * every time, which is what makes runs reproducible and fast. */
        pid_t child = fork();
        if (child < 0) _exit(1);         /* out of pids/memory; give up loudly */

        if (child == 0) {
            /* Step 2c: CHILD. It must not speak the fork protocol, so drop the
             * server fds. Reset prev_loc so this run's edge hashing starts from
             * a known state (the very first edge hashes against 0). Returning
             * hands control back to main(), which reads the input and parses. */
            close(FORKSRV_CTL_FD);
            close(FORKSRV_ST_FD);
            __afl_prev_loc = 0;
            return;
        }

        /* Step 2d: PARENT (the server). Tell the fuzzer which pid it can kill if
         * the run hangs, then reap the child and forward its exact status. We
         * never touch the coverage map here — this code is uninstrumented — so
         * the map the fuzzer reads reflects ONLY the child's execution. */
        if (write(FORKSRV_ST_FD, &child, 4) != 4) _exit(1);

        int status;
        /* waitpid(2): block until `child` changes state. status encodes whether
         * it exited normally (WIFEXITED) or died from a signal (WIFSIGNALED) —
         * a SIGSEGV/SIGABRT here is exactly the crash the fuzzer is hunting. */
        if (waitpid(child, &status, 0) < 0) _exit(1);

        if (write(FORKSRV_ST_FD, &status, 4) != 4) _exit(1);
    }
}
