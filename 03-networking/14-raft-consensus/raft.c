/* ===========================================================================
 * raft.c — the Raft consensus core: election, replication, safety, snapshots.
 * ===========================================================================
 *
 * This is where the algorithm lives. Each node runs one thread (node_main) that
 * repeatedly: drains its inbox, reacts to timers, applies newly-committed log
 * entries to the state machine, and flushes outgoing RPCs. The three sub-problems
 * of Raft map to three clusters of functions here:
 *
 *   LEADER ELECTION  — become_candidate / handle_request_vote{,_reply} / tick.
 *       Randomized election timeouts (RAFT_ELECTION_MIN..MAX) make one follower
 *       time out first, bump the term, and solicit votes. A node grants at most
 *       one vote per term, and only to a candidate whose log is at least as
 *       up-to-date as its own (the ELECTION RESTRICTION — see log_up_to_date).
 *
 *   LOG REPLICATION  — raft_submit / send_append_entries / handle_append_entries
 *       {,_reply} / advance_commit_index. The leader appends a client command,
 *       replicates it with the (prevLogIndex, prevLogTerm) consistency anchor,
 *       and advances commitIndex once a majority stores it — but ONLY for entries
 *       from the leader's current term (the Figure-8 safety rule).
 *
 *   SAFETY & COMPACTION — the two rules above, plus persist-before-reply
 *       (persist.c) and snapshotting (maybe_snapshot / handle_install_snapshot)
 *       to bound the log.
 *
 * LOCKING: every field of a node is guarded by node->mu. A thread holds at most
 * one node's mu at a time; RPCs to send are appended to an out-queue while the
 * lock is held and flushed to the network AFTER unlocking (flush_outgoing), so a
 * node never calls into another node while holding its own lock. That is the
 * whole deadlock-avoidance story (see net.c for the mirror image).
 * =========================================================================== */

#include "raft.h"

#include <stdlib.h>   /* malloc, free, realloc                                   */
#include <string.h>   /* memcpy, memset, strncpy                                 */
#include <stdio.h>    /* snprintf                                                */
#include <time.h>     /* clock_gettime, nanosleep                               */
#include <sys/stat.h> /* mkdir                                                   */
#include <errno.h>    /* errno, EEXIST                                          */

/* ===========================================================================
 * Small time / RNG utilities.
 * =========================================================================== */

/* Monotonic clock in nanoseconds. CLOCK_MONOTONIC never jumps backward (unlike
 * CLOCK_REALTIME, which NTP or an admin can step), so it is the correct base for
 * timeouts: an election must fire after a real elapsed duration, not be confused
 * by a wall-clock adjustment. The node condvar is initialized with this same
 * clock so cond_timedwait deadlines are comparable to these values. */
static uint64_t now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static uint32_t xorshift32(uint32_t *s)
{
    uint32_t x = *s;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return *s = x;
}

/* Majority of the cluster: strictly more than half. For n=5 -> 3, n=3 -> 2. A
 * decision agreed by a majority is safe because any two majorities of the same
 * cluster must overlap in at least one node, so no two conflicting decisions can
 * both win. This single fact underpins both elections and commits. */
static int majority(int n) { return n / 2 + 1; }

/* Arm the election timer to fire after a RANDOM delay in [MIN, MAX] ms. The
 * randomness is the anti-livelock mechanism: with identical timeouts every node
 * would become candidate simultaneously, split the vote every round, and never
 * elect a leader. A spread means one node almost always wakes first and wins. */
static void reset_election_timer(struct raft_node *n)
{
    uint32_t span = RAFT_ELECTION_MAX_MS - RAFT_ELECTION_MIN_MS;
    uint32_t jitter = xorshift32(&n->rng) % (span + 1);
    uint64_t ms = RAFT_ELECTION_MIN_MS + jitter;
    n->election_deadline = now_ns() + ms * 1000000ull;
}

/* ===========================================================================
 * Log accessors — the ONLY code that knows about the snapshot offset. entries[0]
 * has index snap_last_index+1; everything at or below snap_last_index lives in
 * the snapshot, not the array. Concentrating the arithmetic here keeps every
 * caller from re-deriving (and mis-deriving) index<->slot math.
 * =========================================================================== */

/* Index of the last entry the node holds (snapshot base if the live log is
 * empty). This is "how far my log reaches." */
static uint64_t log_last_index(const struct raft_node *n)
{
    return n->log.snap_last_index + n->log.count;
}

/* Term of the entry at `idx`. Index 0 is the sentinel (term 0, "before the
 * log"). idx == snap_last_index returns the snapshot's term. An idx below the
 * snapshot base is compacted history (known committed/consistent) — we return
 * the snapshot term as a best-effort; normal call sites never ask below the
 * base. idx beyond the last entry returns 0 (no such entry). */
static uint64_t log_term_at(const struct raft_node *n, uint64_t idx)
{
    if (idx == 0) return 0;
    if (idx < n->log.snap_last_index) return n->log.snap_last_term;
    if (idx == n->log.snap_last_index) return n->log.snap_last_term;
    uint64_t last = log_last_index(n);
    if (idx > last) return 0;
    return n->log.entries[idx - n->log.snap_last_index - 1].term;
}

/* Pointer to the live entry at `idx`, or NULL if it is not in the array (either
 * compacted away or beyond the end). */
static struct log_entry *log_get(struct raft_node *n, uint64_t idx)
{
    if (idx <= n->log.snap_last_index) return NULL;
    if (idx > log_last_index(n)) return NULL;
    return &n->log.entries[idx - n->log.snap_last_index - 1];
}

/* Ensure the array can hold at least `need` entries; grow geometrically to keep
 * amortized append O(1). Returns 0 / -1 on OOM. */
static int log_reserve(struct raft_node *n, size_t need)
{
    if (n->log.cap >= need) return 0;
    size_t ncap = n->log.cap ? n->log.cap * 2 : 8;
    if (ncap < need) ncap = need;
    struct log_entry *ne = (struct log_entry *)realloc(n->log.entries,
                                                       ncap * sizeof(struct log_entry));
    if (!ne) return -1;
    n->log.entries = ne;
    n->log.cap = ncap;
    return 0;
}

/* Append one entry to the live log, stamping its index. Ownership of the bytes is
 * a plain value copy — the source (a client command or an AppendEntries entry) is
 * independent afterward. */
static int log_append(struct raft_node *n, uint64_t term, const char *cmd, uint16_t cmd_len)
{
    if (log_reserve(n, n->log.count + 1) != 0) return -1;
    struct log_entry *e = &n->log.entries[n->log.count];
    memset(e, 0, sizeof(*e));
    e->term = term;
    e->index = log_last_index(n) + 1;
    e->cmd_len = cmd_len < RAFT_CMD_MAX ? cmd_len : RAFT_CMD_MAX;
    memcpy(e->cmd, cmd, e->cmd_len);
    n->log.count++;
    return 0;
}

/* Delete every entry with index >= `idx` (a divergent suffix). Only the count is
 * moved; the freed capacity is reused by the next append. Never truncates below
 * the snapshot base. */
static void log_truncate_from(struct raft_node *n, uint64_t idx)
{
    if (idx <= n->log.snap_last_index + 1) { n->log.count = 0; return; }
    uint64_t keep = idx - n->log.snap_last_index - 1;   /* number of entries kept */
    if (keep < n->log.count) n->log.count = (size_t)keep;
}

/* Drop the prefix of the live log at or below `base`, folding it into the
 * snapshot. Called by maybe_snapshot / handle_install_snapshot after the state
 * machine already reflects `base`. */
static void log_compact(struct raft_node *n, uint64_t base, uint64_t base_term)
{
    if (base <= n->log.snap_last_index) return;         /* nothing new to compact */
    uint64_t drop = base - n->log.snap_last_index;      /* entries to remove       */
    if (drop >= n->log.count) {
        n->log.count = 0;                               /* compacted the whole log */
    } else {
        memmove(n->log.entries, n->log.entries + drop,
                (n->log.count - drop) * sizeof(struct log_entry));
        n->log.count -= (size_t)drop;
    }
    n->log.snap_last_index = base;
    n->log.snap_last_term = base_term;
}

/* ===========================================================================
 * THE ELECTION RESTRICTION predicate (Raft §5.4.1). Extracted verbatim into
 * asm/demo.c for the annotated assembly, because it is the linchpin of safety.
 *
 * A voter grants its vote only if the candidate's log is AT LEAST AS UP-TO-DATE
 * as the voter's own, where "up-to-date" compares (lastTerm, lastIndex)
 * lexicographically: a higher last term wins outright; on equal last terms the
 * longer log wins (>=, so an equal log still qualifies). This guarantees the
 * winner of any election already stores every committed entry — a committed
 * entry sits on a majority, any majority overlaps the election's majority in
 * some node, and that node would refuse to vote for a candidate missing the
 * entry. Hence "Leader Completeness," hence State Machine Safety.
 * =========================================================================== */
static bool log_up_to_date(uint64_t cand_last_term, uint64_t cand_last_index,
                           uint64_t my_last_term,   uint64_t my_last_index)
{
    if (cand_last_term != my_last_term)
        return cand_last_term > my_last_term;   /* strictly newer term is better  */
    return cand_last_index >= my_last_index;    /* same term: at least as long    */
}

/* ===========================================================================
 * Outgoing message queue. Handlers/timers append to the node's out-queue while
 * holding mu; the loop flushes it after unlocking (see the locking note up top).
 * =========================================================================== */
static void enqueue_out(struct raft_node *n, struct message *m)
{
    if (!m) return;                             /* msg_alloc OOM: treat as a drop */
    m->next = NULL;
    if (n->out_tail) n->out_tail->next = m;
    else             n->out_head = m;
    n->out_tail = m;
}

/* ===========================================================================
 * Role transitions.
 * =========================================================================== */

/* observe_term — the universal rule applied to EVERY inbound message: if it
 * carries a term greater than ours, we are stale; adopt the term, forget any
 * vote (votedFor is per-term), and revert to follower. Returns true if this
 * changed persistent state (term/votedFor) so the caller knows a persist is
 * owed before it replies. */
static bool observe_term(struct raft_node *n, uint64_t term)
{
    if (term > n->current_term) {
        n->current_term = term;
        n->voted_for = -1;
        n->role = ROLE_FOLLOWER;
        return true;
    }
    return false;
}

/* Forward decls for the replication helpers the transitions use. */
static void replicate_to_peer(struct raft_node *n, int peer);
static void replicate_to_all(struct raft_node *n);

/* become_leader — reinitialize leader volatile state after winning an election.
 * nextIndex is optimistic (assume the follower is caught up: last+1) and backs
 * off on rejection; matchIndex is pessimistic (0, proven only by acks). We then
 * send immediate heartbeats to assert authority and stop other nodes' election
 * timers. NOTE: we do NOT append a no-op entry here; that means entries from a
 * PRIOR term are only committed once a current-term client write is committed
 * (advance_commit_index enforces this). Production Raft appends a no-op to commit
 * the tail immediately — see the README. */
static void become_leader(struct raft_node *n)
{
    n->role = ROLE_LEADER;
    uint64_t last = log_last_index(n);
    for (int i = 0; i < n->n_nodes; i++) {
        n->next_index[i]  = last + 1;
        n->match_index[i] = 0;
    }
    n->match_index[n->id] = last;               /* the leader trivially has it all */
    n->heartbeat_deadline = now_ns();           /* due now: the flush sends them   */
    replicate_to_all(n);                        /* initial heartbeats               */
    n->heartbeat_deadline = now_ns() + RAFT_HEARTBEAT_MS * 1000000ull;
}

/* become_candidate — start a new election: bump term, vote for self, PERSIST
 * (currentTerm+votedFor must be durable before we solicit votes, or a crash
 * could let us vote again this term), then RequestVote every peer. */
static void become_candidate(struct raft_node *n)
{
    n->current_term++;
    n->role = ROLE_CANDIDATE;
    n->voted_for = n->id;
    n->votes_granted = 1;                        /* our own vote                    */
    memset(n->vote_from, 0, sizeof(n->vote_from));
    n->vote_from[n->id] = true;
    reset_election_timer(n);                     /* time the election itself        */

    /* Durability barrier: the vote for self is now on stable storage BEFORE any
     * RequestVote leaves this node. */
    persist_save_state(n);

    uint64_t last_idx = log_last_index(n);
    uint64_t last_term = log_term_at(n, last_idx);
    for (int i = 0; i < n->n_nodes; i++) {
        if (i == n->id) continue;
        struct message *m = msg_alloc(n->id, i, MSG_REQUEST_VOTE);
        if (!m) continue;
        m->u.rv.term = n->current_term;
        m->u.rv.candidate_id = n->id;
        m->u.rv.last_log_index = last_idx;
        m->u.rv.last_log_term = last_term;
        enqueue_out(n, m);
    }
}

/* ===========================================================================
 * Replication: build AppendEntries (or InstallSnapshot) for one peer.
 * =========================================================================== */

/* Serialize the current state machine into the node's snapshot cache. Called
 * whenever the snapshot base changes so InstallSnapshot can ship exact bytes. */
static void refresh_snapshot_cache(struct raft_node *n)
{
    n->snap_len = (uint32_t)kv_serialize(&n->kv, n->snap_data, RAFT_SNAP_MAX);
}

/* Ship the whole snapshot to a follower whose needed entries were compacted. */
static void send_install_snapshot(struct raft_node *n, int peer)
{
    struct message *m = msg_alloc(n->id, peer, MSG_INSTALL_SNAPSHOT);
    if (!m) return;
    m->u.is.term = n->current_term;
    m->u.is.leader_id = n->id;
    m->u.is.last_included_index = n->log.snap_last_index;
    m->u.is.last_included_term = n->log.snap_last_term;
    uint32_t len = n->snap_len < RAFT_SNAP_MAX ? n->snap_len : RAFT_SNAP_MAX;
    m->u.is.data_len = len;
    memcpy(m->u.is.data, n->snap_data, len);
    enqueue_out(n, m);
}

/* Build one AppendEntries for `peer` starting at next_index[peer]. If the peer
 * needs entries the leader has already compacted (next_index <= snap base), send
 * a snapshot instead. A pure heartbeat is just this with zero entries. */
static void replicate_to_peer(struct raft_node *n, int peer)
{
    if (peer == n->id) return;
    uint64_t nexti = n->next_index[peer];
    if (nexti <= n->log.snap_last_index) {       /* needed entries are gone         */
        send_install_snapshot(n, peer);
        return;
    }
    uint64_t prev_index = nexti - 1;
    uint64_t prev_term  = log_term_at(n, prev_index);

    struct message *m = msg_alloc(n->id, peer, MSG_APPEND_ENTRIES);
    if (!m) return;
    m->u.ae.term = n->current_term;
    m->u.ae.leader_id = n->id;
    m->u.ae.prev_log_index = prev_index;
    m->u.ae.prev_log_term = prev_term;
    m->u.ae.leader_commit = n->commit_index;

    int cnt = 0;
    uint64_t last = log_last_index(n);
    for (uint64_t idx = nexti; idx <= last && cnt < RAFT_MAX_BATCH; idx++) {
        struct log_entry *e = log_get(n, idx);
        if (!e) break;
        m->u.ae.entries[cnt++] = *e;             /* value copy into the batch       */
    }
    m->u.ae.n_entries = cnt;
    enqueue_out(n, m);
}

static void replicate_to_all(struct raft_node *n)
{
    for (int i = 0; i < n->n_nodes; i++)
        if (i != n->id) replicate_to_peer(n, i);
}

/* ===========================================================================
 * Commit advancement (leader). Find the highest index N replicated on a majority
 * AND created in the CURRENT term, and mark everything up to N committed.
 *
 * The current-term restriction is the Figure-8 safety rule: an entry stored on a
 * majority is NOT necessarily safe to commit if it comes from an older term,
 * because a different future leader could still overwrite that index. It becomes
 * safe only once the leader commits an entry FROM ITS OWN TERM on a majority; by
 * the Log Matching Property that entry drags all earlier entries with it. So we
 * skip any candidate N whose term != current_term.
 * =========================================================================== */
static void advance_commit_index(struct raft_node *n)
{
    uint64_t last = log_last_index(n);
    for (uint64_t N = last; N > n->commit_index; N--) {
        if (log_term_at(n, N) != n->current_term)
            continue;                            /* skip old-term entries (§5.4.2)  */
        int count = 1;                           /* the leader stores N itself      */
        for (int i = 0; i < n->n_nodes; i++)
            if (i != n->id && n->match_index[i] >= N)
                count++;
        if (count >= majority(n->n_nodes)) {
            n->commit_index = N;                 /* highest safe index -> commit it */
            break;                               /* everything below is committed too*/
        }
    }
}

/* Apply committed-but-unapplied entries to the state machine, in index order.
 * This is the only place the KV store is mutated by log entries, and it happens
 * identically on every node — that is why the replicas converge. Entries at or
 * below the snapshot base are already reflected in kv, so we skip straight past
 * them. */
static void apply_committed(struct raft_node *n)
{
    if (n->last_applied < n->log.snap_last_index)
        n->last_applied = n->log.snap_last_index; /* snapshot already applied       */
    while (n->last_applied < n->commit_index) {
        uint64_t idx = n->last_applied + 1;
        struct log_entry *e = log_get(n, idx);
        if (!e) { n->last_applied = idx; continue; }  /* compacted: already applied */
        kv_apply(&n->kv, e->cmd, e->cmd_len);
        n->last_applied = idx;
    }
}

/* Take a snapshot once the live log grows past the threshold, compacting away
 * everything already applied. Persist ordering is snapshot-then-state (see
 * persist.c): the state file records the shorter log and MUST NOT become durable
 * before the snapshot that justifies the truncation. */
static void maybe_snapshot(struct raft_node *n)
{
    if (n->log.count <= RAFT_SNAPSHOT_THRESHOLD) return;
    if (n->last_applied <= n->log.snap_last_index) return;   /* nothing new applied */

    uint64_t base = n->last_applied;
    uint64_t base_term = log_term_at(n, base);
    refresh_snapshot_cache(n);                    /* bytes == kv at `base`           */
    log_compact(n, base, base_term);              /* drop the applied prefix         */

    persist_save_snapshot(n, n->snap_data, n->snap_len);  /* FIRST: the data         */
    persist_save_state(n);                                /* THEN: the shorter log   */
}

/* ===========================================================================
 * RPC handlers. Each takes the node (locked) and the inbound message; it may
 * mutate state and append replies/outgoing RPCs to the out-queue. A handler
 * that changed persistent state calls persist_save_state BEFORE enqueuing the
 * reply, so the fsync completes before the reply is flushed to the network.
 * =========================================================================== */

static void handle_request_vote(struct raft_node *n, struct message *m)
{
    struct msg_request_vote *rv = &m->u.rv;
    bool dirty = observe_term(n, rv->term);

    struct message *rep = msg_alloc(n->id, m->from, MSG_REQUEST_VOTE_REPLY);
    if (!rep) { if (dirty) persist_save_state(n); return; }
    rep->u.rvr.term = n->current_term;
    rep->u.rvr.vote_granted = false;

    /* A stale candidate (older term) is refused outright; the term in our reply
     * teaches it that it has fallen behind and must step down. */
    if (rv->term < n->current_term) {
        if (dirty) persist_save_state(n);
        enqueue_out(n, rep);
        return;
    }

    /* Grant iff we have not already voted for someone else this term AND the
     * candidate's log is at least as up-to-date as ours (the election
     * restriction). */
    bool can_vote = (n->voted_for == -1 || n->voted_for == rv->candidate_id);
    uint64_t my_last_idx = log_last_index(n);
    uint64_t my_last_term = log_term_at(n, my_last_idx);
    bool up_to_date = log_up_to_date(rv->last_log_term, rv->last_log_index,
                                     my_last_term, my_last_idx);
    if (can_vote && up_to_date) {
        n->voted_for = rv->candidate_id;
        dirty = true;
        rep->u.rvr.vote_granted = true;
        reset_election_timer(n);                 /* we backed a leader; be patient  */
    }

    /* PERSIST votedFor before the grant leaves the node: a crash after replying
     * "granted" but before the vote is durable could let us grant a second,
     * conflicting vote in this term on restart. */
    if (dirty) persist_save_state(n);
    enqueue_out(n, rep);
}

static void handle_request_vote_reply(struct raft_node *n, struct message *m)
{
    struct msg_request_vote_reply *rvr = &m->u.rvr;
    if (observe_term(n, rvr->term)) {            /* peer is ahead: we lost, step down*/
        persist_save_state(n);
        return;
    }
    /* Ignore replies that are not for our current campaign. */
    if (n->role != ROLE_CANDIDATE || rvr->term != n->current_term)
        return;
    if (!rvr->vote_granted) return;
    if (m->from < 0 || m->from >= n->n_nodes) return;
    if (n->vote_from[m->from]) return;           /* dedupe duplicate replies        */
    n->vote_from[m->from] = true;
    n->votes_granted++;
    if (n->votes_granted >= majority(n->n_nodes))
        become_leader(n);                        /* majority -> lead immediately     */
}

static void handle_append_entries(struct raft_node *n, struct message *m)
{
    struct msg_append_entries *ae = &m->u.ae;
    bool dirty = observe_term(n, ae->term);

    struct message *rep = msg_alloc(n->id, m->from, MSG_APPEND_ENTRIES_REPLY);
    if (!rep) { if (dirty) persist_save_state(n); return; }
    rep->u.aer.term = n->current_term;
    rep->u.aer.success = false;
    rep->u.aer.match_index = 0;
    rep->u.aer.conflict_index = 0;

    /* Reject a stale leader; our higher term in the reply steps it down. */
    if (ae->term < n->current_term) {
        if (dirty) persist_save_state(n);
        enqueue_out(n, rep);
        return;
    }

    /* A legitimate leader for our term exists: any candidacy ends, and hearing
     * from it resets our election timer so we do not start a needless election. */
    n->role = ROLE_FOLLOWER;
    reset_election_timer(n);

    /* --- Log Matching consistency check on prevLogIndex/prevLogTerm. --- */
    uint64_t prev = ae->prev_log_index;
    if (prev > n->log.snap_last_index) {         /* prev is within our live log     */
        if (prev > log_last_index(n)) {
            /* We are missing entries up to prev: ask the leader to back up to the
             * first index we lack (fast-backup hint), avoiding one-at-a-time. */
            rep->u.aer.conflict_index = log_last_index(n) + 1;
            if (dirty) persist_save_state(n);
            enqueue_out(n, rep);
            return;
        }
        uint64_t t = log_term_at(n, prev);
        if (t != ae->prev_log_term) {
            /* Terms disagree at prev: our log diverged in term `t`. Hint the
             * leader to back up to the first index of that term so it can retry
             * from before the divergence. We do NOT truncate here; the append
             * loop truncates once prev finally matches. */
            uint64_t ci = prev;
            while (ci > n->log.snap_last_index + 1 && log_term_at(n, ci - 1) == t)
                ci--;
            rep->u.aer.conflict_index = ci;
            if (dirty) persist_save_state(n);
            enqueue_out(n, rep);
            return;
        }
    }
    /* prev <= snap_last_index: the prefix is covered by our snapshot (committed,
     * known-consistent) — the check passes and the loop skips those entries. */

    /* --- Append the new entries, reconciling any overlap. --- */
    for (int k = 0; k < ae->n_entries; k++) {
        struct log_entry *e = &ae->entries[k];
        if (e->index <= n->log.snap_last_index)
            continue;                            /* already in our snapshot          */
        if (e->index <= log_last_index(n)) {
            if (log_term_at(n, e->index) != e->term) {
                /* Conflict: same index, different term. Delete this entry and the
                 * whole divergent suffix, then take the leader's version. */
                log_truncate_from(n, e->index);
                log_append(n, e->term, e->cmd, e->cmd_len);
                dirty = true;
            }
            /* else identical entry already present: idempotent, skip. */
        } else {
            log_append(n, e->term, e->cmd, e->cmd_len);
            dirty = true;
        }
    }

    /* --- Advance our commitIndex toward the leader's, clamped to what we hold.
     * We can only mark committed what we actually store, hence the min. */
    if (ae->leader_commit > n->commit_index) {
        uint64_t last = log_last_index(n);
        n->commit_index = ae->leader_commit < last ? ae->leader_commit : last;
    }

    rep->u.aer.success = true;
    rep->u.aer.match_index = prev + (uint64_t)ae->n_entries;  /* last index we now hold */

    /* Persist any log/term change before acking: the leader may count this ack
     * toward a commit, so the entries must be durable here. */
    if (dirty) persist_save_state(n);
    enqueue_out(n, rep);
}

static void handle_append_entries_reply(struct raft_node *n, struct message *m)
{
    struct msg_append_entries_reply *aer = &m->u.aer;
    if (observe_term(n, aer->term)) {            /* follower is ahead: step down     */
        persist_save_state(n);
        return;
    }
    if (n->role != ROLE_LEADER || aer->term != n->current_term)
        return;                                  /* stale reply, ignore              */
    int i = m->from;
    if (i < 0 || i >= n->n_nodes) return;

    if (aer->success) {
        if (aer->match_index > n->match_index[i])
            n->match_index[i] = aer->match_index;
        n->next_index[i] = n->match_index[i] + 1;
        advance_commit_index(n);                 /* this ack may complete a majority */
        /* If the follower still trails, keep the pipeline moving immediately
         * rather than waiting for the next heartbeat. */
        if (n->next_index[i] <= log_last_index(n))
            replicate_to_peer(n, i);
    } else {
        /* Rejected: back nextIndex up to the follower's hint and retry now. The
         * hint collapses what would be O(log length) round trips into ~O(#terms).*/
        uint64_t ci = aer->conflict_index;
        if (ci < 1) ci = 1;
        n->next_index[i] = ci;
        replicate_to_peer(n, i);                 /* immediate retry (or snapshot)    */
    }
}

static void handle_install_snapshot(struct raft_node *n, struct message *m)
{
    struct msg_install_snapshot *is = &m->u.is;
    bool dirty = observe_term(n, is->term);

    struct message *rep = msg_alloc(n->id, m->from, MSG_INSTALL_SNAPSHOT_REPLY);
    if (rep) rep->u.isr.term = n->current_term;

    if (is->term < n->current_term) {            /* stale leader                     */
        if (dirty) persist_save_state(n);
        if (rep) enqueue_out(n, rep);
        return;
    }
    n->role = ROLE_FOLLOWER;
    reset_election_timer(n);

    /* Already have this snapshot (or newer): nothing to install. */
    if (is->last_included_index <= n->log.snap_last_index) {
        if (dirty) persist_save_state(n);
        if (rep) enqueue_out(n, rep);
        return;
    }

    /* Install: the snapshot is the authoritative state up to last_included_index.
     * Rebuild the state machine from its bytes, cache them for onward shipping,
     * discard the now-covered live log, and advance the snapshot base. We discard
     * the ENTIRE live log for simplicity (a strict superset-safe choice); the
     * leader will resend anything beyond the base. */
    kv_deserialize(&n->kv, is->data, is->data_len);
    if (is->data_len <= RAFT_SNAP_MAX) {
        memcpy(n->snap_data, is->data, is->data_len);
        n->snap_len = is->data_len;
    }
    n->log.count = 0;
    n->log.snap_last_index = is->last_included_index;
    n->log.snap_last_term = is->last_included_term;
    if (n->commit_index < is->last_included_index)
        n->commit_index = is->last_included_index;
    n->last_applied = is->last_included_index;   /* snapshot is fully applied        */
    dirty = true;

    /* Snapshot bytes durable first, then the state that references them. */
    persist_save_snapshot(n, n->snap_data, n->snap_len);
    persist_save_state(n);
    if (rep) enqueue_out(n, rep);
}

static void handle_install_snapshot_reply(struct raft_node *n, struct message *m)
{
    struct msg_install_snapshot_reply *isr = &m->u.isr;
    if (observe_term(n, isr->term)) { persist_save_state(n); return; }
    if (n->role != ROLE_LEADER || isr->term != n->current_term) return;
    int i = m->from;
    if (i < 0 || i >= n->n_nodes) return;
    /* The follower is now caught up to our snapshot base; advance its indices
     * (monotonically) and continue normal replication from there. */
    if (n->log.snap_last_index > n->match_index[i])
        n->match_index[i] = n->log.snap_last_index;
    n->next_index[i] = n->match_index[i] + 1;
    replicate_to_peer(n, i);
}

/* Dispatch one inbound message to its handler. */
static void raft_handle(struct raft_node *n, struct message *m)
{
    switch (m->type) {
    case MSG_REQUEST_VOTE:            handle_request_vote(n, m); break;
    case MSG_REQUEST_VOTE_REPLY:      handle_request_vote_reply(n, m); break;
    case MSG_APPEND_ENTRIES:          handle_append_entries(n, m); break;
    case MSG_APPEND_ENTRIES_REPLY:    handle_append_entries_reply(n, m); break;
    case MSG_INSTALL_SNAPSHOT:        handle_install_snapshot(n, m); break;
    case MSG_INSTALL_SNAPSHOT_REPLY:  handle_install_snapshot_reply(n, m); break;
    }
}

/* ===========================================================================
 * Timers. Called once per loop iteration with the current time.
 * =========================================================================== */
static void tick(struct raft_node *n, uint64_t now)
{
    if (n->role == ROLE_LEADER) {
        if (now >= n->heartbeat_deadline) {
            replicate_to_all(n);                 /* heartbeats keep leadership alive */
            n->heartbeat_deadline = now + RAFT_HEARTBEAT_MS * 1000000ull;
        }
    } else {
        /* Follower or candidate: no leader contact within the timeout -> begin a
         * new election. A candidate that timed out simply starts a fresh one with
         * a higher term (split votes resolve because the next timeout is random). */
        if (now >= n->election_deadline)
            become_candidate(n);
    }
}

/* ===========================================================================
 * The node thread. See the file header for the lock/flush protocol that keeps
 * cross-node messaging deadlock-free.
 * =========================================================================== */

/* Pop the head of the inbox (caller holds mu). Returns NULL if empty. */
static struct message *inbox_pop(struct raft_node *n)
{
    struct message *m = n->inbox_head;
    if (!m) return NULL;
    n->inbox_head = m->next;
    if (!n->inbox_head) n->inbox_tail = NULL;
    m->next = NULL;
    return m;
}

/* Flush the out-queue to the network WITHOUT holding any node lock — this is the
 * step that makes the whole system deadlock-free (a node never holds its own mu
 * while net_send briefly takes a destination's mu). net_send takes ownership of
 * each message. */
static void flush_outgoing(struct raft_node *n, struct message *head)
{
    while (head) {
        struct message *next = head->next;
        head->next = NULL;
        net_send(n->cluster, head);              /* ownership transferred            */
        head = next;
    }
}

/* Convert an absolute CLOCK_MONOTONIC nanosecond deadline to a timespec for
 * pthread_cond_timedwait. The condvar was created with CLOCK_MONOTONIC, so the
 * two are comparable (mismatching clocks here is a classic bug that makes waits
 * fire early or late whenever the wall clock is adjusted). */
static void ns_to_timespec(uint64_t ns, struct timespec *ts)
{
    ts->tv_sec  = (time_t)(ns / 1000000000ull);
    ts->tv_nsec = (long)(ns % 1000000000ull);
}

static void *node_main(void *arg)
{
    struct raft_node *n = (struct raft_node *)arg;
    pthread_mutex_lock(&n->mu);
    while (n->running) {
        if (n->stopped) {
            /* Simulated crash: drain and discard any straggler messages, do no
             * work, and just wait to be restarted or shut down. */
            struct message *m;
            while ((m = inbox_pop(n)) != NULL) free(m);
        } else {
            /* 1. Drain the inbox, handling each message (may enqueue outgoing). */
            struct message *m;
            while ((m = inbox_pop(n)) != NULL) {
                raft_handle(n, m);
                free(m);                         /* receiver owns and frees it       */
            }
            /* 2. Timers, then apply committed entries, then maybe compact. */
            tick(n, now_ns());
            apply_committed(n);
            maybe_snapshot(n);
        }

        /* 3. Detach outgoing and the next wake deadline while still locked. */
        struct message *out = n->out_head;
        n->out_head = n->out_tail = NULL;
        uint64_t deadline = n->stopped ? now_ns() + 50000000ull  /* recheck in 50ms */
                          : (n->role == ROLE_LEADER ? n->heartbeat_deadline
                                                    : n->election_deadline);
        pthread_mutex_unlock(&n->mu);

        /* 4. Send outside the lock (deadlock-free). */
        flush_outgoing(n, out);

        /* 5. Sleep until the deadline or until a message wakes us. */
        pthread_mutex_lock(&n->mu);
        if (n->running && n->inbox_head == NULL) {
            struct timespec ts;
            ns_to_timespec(deadline, &ts);
            pthread_cond_timedwait(&n->inbox_cv, &n->mu, &ts);
            /* Spurious wakeups and early returns are fine: we loop and re-evaluate
             * timers/inbox from scratch, so no correctness rides on the wait's
             * exact duration. */
        }
    }
    pthread_mutex_unlock(&n->mu);
    return NULL;
}

/* ===========================================================================
 * Public API — cluster lifecycle, client submit, observation, fault injection.
 * =========================================================================== */

/* Best-effort directory creation (ignore "already exists"). */
static int ensure_dir(const char *path)
{
    if (mkdir(path, 0755) == 0) return 0;
    if (errno == EEXIST) return 0;
    return -1;
}

struct cluster *cluster_create(int n, const char *state_dir)
{
    if (n < 1 || n > RAFT_MAX_NODES) return NULL;
    struct cluster *c = (struct cluster *)calloc(1, sizeof(struct cluster));
    if (!c) return NULL;
    c->n = n;
    c->drop_permille = 0;
    c->net_rng = 0x9e3779b9u;                    /* nonzero seed for xorshift        */
    pthread_mutex_init(&c->net_mu, NULL);
    for (int a = 0; a < n; a++)
        for (int b = 0; b < n; b++)
            c->reachable[a][b] = true;           /* full mesh initially              */

    ensure_dir(state_dir);

    /* A condattr that pins condvars to CLOCK_MONOTONIC (see ns_to_timespec). */
    pthread_condattr_t cattr;
    pthread_condattr_init(&cattr);
    pthread_condattr_setclock(&cattr, CLOCK_MONOTONIC);

    for (int i = 0; i < n; i++) {
        struct raft_node *nd = (struct raft_node *)calloc(1, sizeof(struct raft_node));
        if (!nd) { /* partial cleanup */ c->nodes[i] = NULL; continue; }
        nd->id = i;
        nd->n_nodes = n;
        nd->cluster = c;
        nd->current_term = 0;
        nd->voted_for = -1;
        nd->role = ROLE_FOLLOWER;
        nd->commit_index = 0;
        nd->last_applied = 0;
        nd->rng = 0x100 + (uint32_t)i * 2654435761u; /* distinct per-node jitter seed*/
        if (nd->rng == 0) nd->rng = 1;
        kv_reset(&nd->kv);
        snprintf(nd->dir, sizeof(nd->dir), "%s/node%d", state_dir, i);
        ensure_dir(nd->dir);

        /* Load any state a previous run fsync'd. Fresh dirs leave the zero
         * defaults set above. */
        persist_load(nd);
        nd->commit_index = nd->log.snap_last_index;  /* re-learned from the leader   */
        nd->last_applied = nd->log.snap_last_index;
        refresh_snapshot_cache(nd);

        pthread_mutex_init(&nd->mu, NULL);
        pthread_cond_init(&nd->inbox_cv, &cattr);
        reset_election_timer(nd);
        c->nodes[i] = nd;
    }
    pthread_condattr_destroy(&cattr);
    return c;
}

void cluster_start(struct cluster *c)
{
    for (int i = 0; i < c->n; i++) {
        struct raft_node *nd = c->nodes[i];
        if (!nd) continue;
        nd->running = true;
        nd->stopped = false;
        pthread_create(&nd->thread, NULL, node_main, nd);
    }
}

void cluster_stop(struct cluster *c)
{
    /* Ask every thread to exit, then join. Signaling under the lock guarantees a
     * thread parked in cond_timedwait sees running=false on wake. */
    for (int i = 0; i < c->n; i++) {
        struct raft_node *nd = c->nodes[i];
        if (!nd) continue;
        pthread_mutex_lock(&nd->mu);
        nd->running = false;
        pthread_cond_signal(&nd->inbox_cv);
        pthread_mutex_unlock(&nd->mu);
    }
    for (int i = 0; i < c->n; i++) {
        struct raft_node *nd = c->nodes[i];
        if (!nd) continue;
        pthread_join(nd->thread, NULL);
        /* Free any messages left in the inbox/out-queue, then the node. */
        struct message *m;
        while ((m = inbox_pop(nd)) != NULL) free(m);
        m = nd->out_head;
        while (m) { struct message *nx = m->next; free(m); m = nx; }
        free(nd->log.entries);
        pthread_mutex_destroy(&nd->mu);
        pthread_cond_destroy(&nd->inbox_cv);
        free(nd);
        c->nodes[i] = NULL;
    }
    pthread_mutex_destroy(&c->net_mu);
    free(c);
}

/* raft_submit — offer a command to node `id`. If it is the leader, append it
 * (persisting before it counts toward any commit) and return its log index;
 * otherwise return -1 so the caller can retry on the real leader. */
long raft_submit(struct cluster *c, int id, const char *cmd)
{
    if (id < 0 || id >= c->n || !c->nodes[id]) return -1;
    struct raft_node *n = c->nodes[id];
    long result = -1;
    pthread_mutex_lock(&n->mu);
    if (!n->stopped && n->role == ROLE_LEADER) {
        uint16_t len = 0;
        while (len < RAFT_CMD_MAX - 1 && cmd[len]) len++;
        if (log_append(n, n->current_term, cmd, len) == 0) {
            uint64_t idx = log_last_index(n);
            n->match_index[n->id] = idx;         /* the leader stores it now         */
            /* Durability: the new entry is on stable storage before the leader
             * ever counts it toward a majority. */
            persist_save_state(n);
            n->heartbeat_deadline = now_ns();    /* replicate on the next tick       */
            pthread_cond_signal(&n->inbox_cv);   /* wake the loop to do it promptly  */
            result = (long)idx;
        }
    }
    pthread_mutex_unlock(&n->mu);
    return result;
}

/* Wait until some node reports commit_index >= index (which only happens once
 * the entry is truly committed), or the timeout elapses. Polls with short sleeps
 * — a teaching-harness convenience, not part of the algorithm. */
bool raft_wait_commit(struct cluster *c, long index, int timeout_ms)
{
    if (index < 0) return true;
    uint64_t deadline = now_ns() + (uint64_t)timeout_ms * 1000000ull;
    for (;;) {
        for (int i = 0; i < c->n; i++) {
            struct raft_node *nd = c->nodes[i];
            if (!nd) continue;
            pthread_mutex_lock(&nd->mu);
            bool done = (!nd->stopped && nd->commit_index >= (uint64_t)index);
            pthread_mutex_unlock(&nd->mu);
            if (done) return true;
        }
        if (now_ns() >= deadline) return false;
        struct timespec ts = { 0, 5 * 1000000L };   /* 5 ms poll interval           */
        nanosleep(&ts, NULL);
    }
}

int raft_find_leader(struct cluster *c)
{
    /* Return the leader of the highest term seen (there can be at most one per
     * term; a stale leader from an older term will step down shortly). */
    int best = -1; uint64_t best_term = 0;
    for (int i = 0; i < c->n; i++) {
        struct raft_node *nd = c->nodes[i];
        if (!nd) continue;
        pthread_mutex_lock(&nd->mu);
        if (!nd->stopped && nd->role == ROLE_LEADER && nd->current_term >= best_term) {
            best_term = nd->current_term; best = i;
        }
        pthread_mutex_unlock(&nd->mu);
    }
    return best;
}

uint64_t raft_current_term(struct cluster *c, int id)
{
    if (id < 0 || id >= c->n || !c->nodes[id]) return 0;
    struct raft_node *n = c->nodes[id];
    pthread_mutex_lock(&n->mu);
    uint64_t t = n->current_term;
    pthread_mutex_unlock(&n->mu);
    return t;
}

uint64_t raft_commit_index(struct cluster *c, int id)
{
    if (id < 0 || id >= c->n || !c->nodes[id]) return 0;
    struct raft_node *n = c->nodes[id];
    pthread_mutex_lock(&n->mu);
    uint64_t ci = n->commit_index;
    pthread_mutex_unlock(&n->mu);
    return ci;
}

uint64_t raft_snapshot_index(struct cluster *c, int id)
{
    if (id < 0 || id >= c->n || !c->nodes[id]) return 0;
    struct raft_node *n = c->nodes[id];
    pthread_mutex_lock(&n->mu);
    uint64_t s = n->log.snap_last_index;
    pthread_mutex_unlock(&n->mu);
    return s;
}

uint64_t raft_last_log_index(struct cluster *c, int id)
{
    if (id < 0 || id >= c->n || !c->nodes[id]) return 0;
    struct raft_node *n = c->nodes[id];
    pthread_mutex_lock(&n->mu);
    uint64_t li = n->log.snap_last_index + n->log.count;
    pthread_mutex_unlock(&n->mu);
    return li;
}

bool raft_kv_get(struct cluster *c, int id, const char *key, char *out, size_t outlen)
{
    if (id < 0 || id >= c->n || !c->nodes[id]) return false;
    struct raft_node *n = c->nodes[id];
    pthread_mutex_lock(&n->mu);
    bool ok = kv_get(&n->kv, key, out, outlen);
    pthread_mutex_unlock(&n->mu);
    return ok;
}

/* raft_crash_node — simulate a power loss: the node stops processing and its
 * inbox is dropped. In-memory state is intentionally left as-is; a restart is
 * what proves recovery works, by rebuilding purely from disk. */
void raft_crash_node(struct cluster *c, int id)
{
    if (id < 0 || id >= c->n || !c->nodes[id]) return;
    struct raft_node *n = c->nodes[id];
    pthread_mutex_lock(&n->mu);
    n->stopped = true;
    pthread_cond_signal(&n->inbox_cv);
    pthread_mutex_unlock(&n->mu);
}

/* raft_restart_node — reboot: THROW AWAY volatile in-memory state and rebuild
 * everything from what was fsync'd. This is the recovery path that persist.c's
 * ordering guarantees is correct: currentTerm/votedFor/log come back exactly as
 * they were durable, so the node cannot violate any invariant it upheld before
 * the crash. */
void raft_restart_node(struct cluster *c, int id)
{
    if (id < 0 || id >= c->n || !c->nodes[id]) return;
    struct raft_node *n = c->nodes[id];
    pthread_mutex_lock(&n->mu);
    /* Reset all volatile state to a fresh follower. */
    n->role = ROLE_FOLLOWER;
    n->current_term = 0;
    n->voted_for = -1;
    n->votes_granted = 0;
    memset(n->vote_from, 0, sizeof(n->vote_from));
    memset(n->next_index, 0, sizeof(n->next_index));
    memset(n->match_index, 0, sizeof(n->match_index));
    free(n->log.entries);
    n->log.entries = NULL; n->log.count = 0; n->log.cap = 0;
    n->log.snap_last_index = 0; n->log.snap_last_term = 0;
    kv_reset(&n->kv);

    /* Rebuild persistent state and the snapshot from disk. */
    persist_load(n);
    n->commit_index = n->log.snap_last_index;   /* volatile: re-learned from leader */
    n->last_applied = n->log.snap_last_index;
    refresh_snapshot_cache(n);

    reset_election_timer(n);
    n->stopped = false;                          /* back in service                  */
    pthread_cond_signal(&n->inbox_cv);
    pthread_mutex_unlock(&n->mu);
}
