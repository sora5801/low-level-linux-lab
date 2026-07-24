/* ===========================================================================
 * demo.c — Raft's two safety-critical pure-logic routines, extracted for asm.
 * ===========================================================================
 *
 * WHY THIS FILE EXISTS
 * --------------------
 * The real consensus core (../raft.c, ../persist.c, ../net.c) pulls in <pthread.h>,
 * <fcntl.h>, <sys/stat.h>, <time.h> — a pile of POSIX headers — so it cannot be
 * lowered to standalone assembly on an arbitrary host. Per the lab convention we
 * lift out the single most instructive, purely-computational pieces of the
 * algorithm into this header-free file so clang emits clean SysV asm we can
 * annotate. These two functions are exactly the safety decisions Raft makes,
 * minus the threading and I/O:
 *
 *   log_up_to_date        — the ELECTION RESTRICTION (Raft §5.4.1): may a voter
 *                           grant its vote to this candidate? Compares the two
 *                           logs' (lastTerm, lastIndex) lexicographically.
 *   majority_match_index  — the COMMIT-INDEX computation (Raft §5.3/§5.4.2): the
 *                           highest log index that is stored on a MAJORITY of
 *                           nodes, found by sorting the matchIndex[] array.
 *
 * They mirror ../raft.c's log_up_to_date() and the majority test inside
 * advance_commit_index() (raft.c does the equivalent with a downward scan; here
 * we show the sorted-median form, which is the same result computed in one pass).
 *
 * There are NO system headers here: we declare our own fixed-width types. On the
 * LP64 model (Linux x86-64) `unsigned long` is 64 bits, matching Raft's 64-bit
 * terms and indices, and `unsigned int` is 32 bits.
 *
 * WHY THESE TWO ROUTINES ARE THE HEART OF SAFETY
 * ----------------------------------------------
 * Raft's whole correctness argument reduces to "any committed entry survives
 * every future leader change." That rests on two facts, and these functions ARE
 * those two facts:
 *
 *  (1) A candidate can only win if its log is at least as up-to-date as a
 *      majority — so the winner already holds every committed entry
 *      (log_up_to_date is the gate).
 *  (2) The leader only advances the commit point to an index a majority stores
 *      (majority_match_index computes that index). Because any two majorities
 *      overlap, the entry a leader commits is guaranteed to be seen by the next
 *      leader's electing majority — closing the loop with (1).
 * =========================================================================== */

/* Our own fixed-width types — no <stdint.h>. LP64: long is 64-bit. */
typedef unsigned long u64;   /* a Raft term or log index                        */
typedef unsigned int  u32;

/* ---------------------------------------------------------------------------
 * log_up_to_date — the election restriction predicate.
 *
 * Returns 1 iff a candidate whose last log entry is (cand_term, cand_index) is
 * "at least as up-to-date" as a voter whose last entry is (my_term, my_index).
 * The comparison is LEXICOGRAPHIC on (term, index):
 *
 *   - A strictly higher last TERM always wins: a log that has seen a newer term
 *     cannot be missing anything the older-term log has committed, because terms
 *     only advance when a new leader is elected by a majority.
 *   - On EQUAL last terms, the LONGER (or equal-length) log wins — hence `>=`.
 *     An equal log is still acceptable: two identical logs are equally current.
 *
 * A voter combines this with "have I already voted this term?" before granting.
 * Getting the `>` vs `>=` here wrong is a classic way to break Raft: `>` on the
 * index would refuse a vote to a perfectly up-to-date candidate and could starve
 * elections; `>` on the term would let a stale candidate win and lose committed
 * entries. The asm shows this compiles to two compares and a couple of setcc's,
 * with no branches at -O1 — safety in a dozen instructions.
 * --------------------------------------------------------------------------- */
int log_up_to_date(u64 cand_term, u64 cand_index, u64 my_term, u64 my_index)
{
    if (cand_term != my_term)
        return cand_term > my_term;     /* different terms: newer term wins       */
    return cand_index >= my_index;      /* same term: at least as long wins       */
}

/* ---------------------------------------------------------------------------
 * majority_match_index — the commit-index majority computation.
 *
 * The leader tracks matchIndex[i] = the highest log index it KNOWS is replicated
 * on node i (with its own slot = its last index). An index N is safe to commit
 * once a MAJORITY of nodes have matchIndex >= N. The largest such N is found by
 * sorting matchIndex ascending and reading the element at position (n - majority):
 * that element, and everything above it, are >= it, and there are exactly
 * `majority` of them — so `majority` nodes store an index at least this large.
 *
 *   n = 5, majority = 3  -> return sorted[5-3] = sorted[2]  (the median)
 *   n = 3, majority = 2  -> return sorted[3-2] = sorted[1]  (the median)
 *
 * The caller (a real leader) then commits this index ONLY if the entry there was
 * created in the leader's current term (the Figure-8 rule) — that extra check
 * needs the log and so lives in raft.c, not here. This routine is the pure
 * "where does the majority reach?" arithmetic.
 *
 * We insertion-sort a bounded local copy (no <stdlib.h> qsort, no heap): n is at
 * most a handful of nodes, and insertion sort on a tiny array is both the fastest
 * choice AND the most legible assembly — a nested loop with an inner shift. The
 * copy leaves the caller's matchIndex array untouched.
 * --------------------------------------------------------------------------- */
u64 majority_match_index(const u64 *match, int n)
{
    if (n <= 0) return 0;
    if (n > 16) n = 16;                 /* clamp to the local buffer's capacity   */

    u64 tmp[16];
    for (int i = 0; i < n; i++)
        tmp[i] = match[i];              /* work on a copy; caller's array is const*/

    /* Insertion sort ascending: grow a sorted prefix tmp[0..i-1], insert tmp[i]
     * by shifting larger elements right. O(n^2), but n is tiny. */
    for (int i = 1; i < n; i++) {
        u64 key = tmp[i];
        int j = i - 1;
        while (j >= 0 && tmp[j] > key) {
            tmp[j + 1] = tmp[j];        /* slide the larger element up            */
            j--;
        }
        tmp[j + 1] = key;               /* drop key into its sorted slot          */
    }

    int maj = n / 2 + 1;                /* strict majority: > half the nodes      */
    return tmp[n - maj];               /* highest index a majority has reached   */
}
