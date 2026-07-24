/* ===========================================================================
 * raft.c — the Raft consensus module (Ongaro & Ousterhout, 2014).
 * ===========================================================================
 *
 * This is the replication brain. It implements the core of Raft's Figure 2:
 * persistent state (currentTerm, votedFor, log[]), the two RPCs (RequestVote and
 * AppendEntries), randomized leader election, log replication with the log-
 * matching consistency check, and the commit rule. Once an entry commits, we
 * apply it to the storage engine (store.c). See ../../03-networking/14-raft-consensus
 * for the standalone, test-harnessed version this distills.
 *
 * WHAT RAFT BUYS YOU
 * -----------------
 * A single durable KV (store.c) survives *crashes* but not the *loss of the
 * machine*. Raft keeps an identical replicated log on a majority of nodes, so
 * the cluster keeps serving and never loses an acknowledged write as long as a
 * majority survives. The three guarantees it maintains:
 *   - Election Safety: at most one leader per term.
 *   - Log Matching: if two logs share an entry at some index+term, all prior
 *     entries are identical (enforced by the AppendEntries consistency check).
 *   - Leader Completeness: a committed entry is present in every future leader's
 *     log (enforced by the "up-to-date" restriction on granting votes).
 *
 * THIS FILE IS PURE LOGIC. It never touches a socket or a timer directly: it
 * calls r->send() to transmit a payload and r->reset_election() to re-arm the
 * election timeout. server.c owns those fds. That separation is why the whole
 * algorithm here reads as a state machine, not as I/O plumbing.
 *
 * THE WIRE MESSAGES (payloads; server.c wraps each in a length+CRC frame)
 * ----------------------------------------------------------------------
 *   common prefix:  [u8 type][u64 term][u32 from]
 *   REQUEST_VOTE      + [u64 lastLogIndex][u64 lastLogTerm]
 *   REQUEST_VOTE_RSP  + [u8 voteGranted]
 *   APPEND_ENTRIES    + [u64 prevLogIndex][u64 prevLogTerm][u64 leaderCommit]
 *                       [u32 nEntries] then nEntries×{ [u64 term][u64 index]
 *                                                       [u32 blen][body] }
 *   APPEND_ENTRIES_RSP+ [u8 success][u64 matchIndex]
 * ===========================================================================
 */
#include "db.h"

#include <unistd.h>    /* read, write, ftruncate, lseek, fsync, close          */
#include <fcntl.h>     /* open                                                 */
#include <string.h>    /* memcpy, memset                                       */
#include <stdlib.h>    /* malloc, realloc, free, random                        */
#include <errno.h>
#include <stdio.h>     /* snprintf                                             */

/* Message type tags (first payload byte). */
enum {
    MSG_REQUEST_VOTE = 1,
    MSG_REQUEST_VOTE_RSP,
    MSG_APPEND_ENTRIES,
    MSG_APPEND_ENTRIES_RSP
};

/* Cap the entries shipped in one AppendEntries so a single RPC stays under the
 * frame ceiling; a lagging follower catches up over several heartbeats. */
#define AE_MAX_BATCH 64

/* ---- small log helpers ---------------------------------------------------- */

/* Highest index in our log (0 when empty). log is 1-indexed; log[0] is unused. */
static uint64_t last_index(const struct raft *r) { return r->log_len; }

/* Term of the entry at `idx` (0 for idx 0 or out of range). */
static uint64_t term_at(const struct raft *r, uint64_t idx)
{
    if (idx == 0 || idx > r->log_len) return 0;
    return r->log[idx].term;
}

/* Total nodes and the majority threshold. Majority = floor(N/2)+1: the smallest
 * set that must intersect any other majority, which is why two leaders can never
 * both be elected in the same term (their vote sets would have to overlap). */
static int cluster_size(const struct raft *r) { return 1 + r->npeers; }
static int majority(const struct raft *r)     { return cluster_size(r) / 2 + 1; }

/* Grow the log array to hold at least `need` entries (indices 1..need). */
static int log_reserve(struct raft *r, uint64_t need)
{
    if (need + 1 <= r->log_cap) return 0;      /* +1 for the unused slot 0       */
    uint64_t cap = r->log_cap ? r->log_cap : 8;
    while (cap < need + 1) cap *= 2;
    struct raft_entry *n = realloc(r->log, cap * sizeof *n);
    if (!n) return -1;
    r->log = n;
    r->log_cap = cap;
    return 0;
}

/* ---- persistence ---------------------------------------------------------- */
/*
 * Two files under the data dir:
 *   raft.state — [u64 currentTerm][i32 votedFor][u32 crc], rewritten (and
 *                fsync'd) whenever term or vote changes. votedFor == -1 means
 *                "no vote this term". A CRC guards a torn rewrite.
 *   raft.log   — append-only records: [u32 reclen][u32 crc][u64 term][u64 index]
 *                [command body]. reclen = 16 + blen; CRC covers everything after
 *                the 8-byte header. Same framing discipline as store.wal.
 *
 * Note the honest simplification: on a conflicting AppendEntries we rewrite the
 * whole raft.log (log_persist_all) rather than surgically truncating on disk.
 * Correct, and truncation is rare; a production log truncates in place.
 */

static int persist_state(struct raft *r)
{
    uint8_t buf[16];
    put_u64(buf, r->current_term);
    put_u32(buf + 8, (uint32_t)r->voted_for);          /* -1 stored as 0xFFFFFFFF */
    put_u32(buf + 12, crc32_ieee(buf, 12));            /* guard the 12-byte body  */
    /* pwrite at offset 0 overwrites the fixed-size record without an lseek race. */
    if (pwrite(r->state_fd, buf, 16, 0) != 16) return -1;
    if (fsync(r->state_fd) < 0) return -1;             /* durable before we act   */
    return 0;
}

static void load_state(struct raft *r)
{
    uint8_t buf[16];
    r->current_term = 0;
    r->voted_for = -1;                                  /* defaults for a fresh node */
    if (pread(r->state_fd, buf, 16, 0) == 16 &&
        crc32_ieee(buf, 12) == get_u32(buf + 12)) {     /* only trust an intact rec */
        r->current_term = get_u64(buf);
        r->voted_for = (int)get_u32(buf + 8);
    }
}

/* Append one entry's record to raft.log and fsync it durable. */
static int log_persist_append(struct raft *r, const struct raft_entry *e)
{
    size_t reclen = 16 + e->blen;                       /* term(8)+index(8)+body   */
    size_t total  = 8 + reclen;
    uint8_t *rec = malloc(total);
    if (!rec) return -1;
    put_u32(rec, (uint32_t)reclen);
    put_u64(rec + 8, e->term);
    put_u64(rec + 16, e->index);
    if (e->blen) memcpy(rec + 24, e->body, e->blen);
    put_u32(rec + 4, crc32_ieee(rec + 8, reclen));      /* CRC over the payload    */

    int rc = 0;
    size_t off = 0;
    while (off < total) {
        ssize_t w = write(r->raftlog_fd, rec + off, total - off);
        if (w < 0) { if (errno == EINTR) continue; rc = -1; break; }
        off += (size_t)w;
    }
    if (rc == 0 && fsync(r->raftlog_fd) < 0) rc = -1;
    free(rec);
    return rc;
}

/* Rewrite raft.log from scratch to match the in-memory log[1..log_len]. Used
 * after we delete a conflicting suffix (a follower reconciling with a new
 * leader). O(total log bytes) but only on the rare conflict path. */
static int log_persist_all(struct raft *r)
{
    if (ftruncate(r->raftlog_fd, 0) < 0) return -1;
    if (lseek(r->raftlog_fd, 0, SEEK_SET) < 0) return -1;
    for (uint64_t i = 1; i <= r->log_len; i++)
        if (log_persist_append(r, &r->log[i]) < 0) return -1;
    return 0;
}

/* Replay raft.log on boot to rebuild log[]. Mirrors wal_replay's torn-tail
 * discipline: stop at the first short/CRC-bad record and truncate to it. */
static int log_replay(struct raft *r)
{
    if (lseek(r->raftlog_fd, 0, SEEK_SET) < 0) return -1;
    off_t good = 0;
    for (;;) {
        uint8_t hdr[8];
        ssize_t got = 0; size_t need = 8; uint8_t *p = hdr;
        while (got < (ssize_t)need) {                    /* read the 8-byte header */
            ssize_t k = read(r->raftlog_fd, p + got, need - got);
            if (k < 0) { if (errno == EINTR) continue; return -1; }
            if (k == 0) break;
            got += k;
        }
        if (got == 0) break;                             /* clean EOF              */
        if (got < 8) break;                              /* torn header            */
        uint32_t reclen = get_u32(hdr), crc = get_u32(hdr + 4);
        if (reclen < 16 || reclen > 16 + 1 + 4 + DB_MAX_KEY + 4 + DB_MAX_VAL)
            break;                                       /* insane length ⇒ torn   */
        uint8_t *pl = malloc(reclen);
        if (!pl) return -1;
        got = 0; p = pl;
        while (got < (ssize_t)reclen) {
            ssize_t k = read(r->raftlog_fd, p + got, reclen - got);
            if (k < 0) { if (errno == EINTR) continue; free(pl); return -1; }
            if (k == 0) break;
            got += k;
        }
        if ((uint32_t)got < reclen) { free(pl); break; } /* torn body              */
        if (crc32_ieee(pl, reclen) != crc) { free(pl); break; } /* bad CRC ⇒ torn  */

        uint64_t term  = get_u64(pl);
        uint64_t index = get_u64(pl + 8);
        uint32_t blen  = reclen - 16;
        if (index != r->log_len + 1) { free(pl); break;} /* gap ⇒ corrupt          */
        if (log_reserve(r, index) < 0) { free(pl); return -1; }
        struct raft_entry *e = &r->log[index];
        e->term = term; e->index = index; e->blen = blen;
        e->body = malloc(blen ? blen : 1);
        if (!e->body) { free(pl); return -1; }
        memcpy(e->body, pl + 16, blen);
        r->log_len = index;
        free(pl);
        good += 8 + reclen;
    }
    if (ftruncate(r->raftlog_fd, good) < 0) return -1;
    if (lseek(r->raftlog_fd, good, SEEK_SET) < 0) return -1;
    return 0;
}

/* ---- applying committed entries to the state machine ---------------------- */

/* Advance last_applied up to commit_index, handing each entry to store.c. This
 * is the bridge from "replicated log" to "queryable database". */
static void apply_committed(struct raft *r)
{
    while (r->last_applied < r->commit_index) {
        uint64_t i = ++r->last_applied;
        struct command c;
        /* The entry body is exactly a command body — decode and apply it. */
        if (cmd_decode_body(r->log[i].body, r->log[i].blen, &c) == 0) {
            (void)store_apply(r->store, &c, true);
            TR("apply", "node=%d idx=%llu op=%u", r->id,
               (unsigned long long)i, c.op);
        }
    }
}

/* ---- role transitions ----------------------------------------------------- */

static void become_follower(struct raft *r, uint64_t term)
{
    bool changed = (term != r->current_term) || (r->voted_for != -1 && term > r->current_term);
    if (term > r->current_term) {
        r->current_term = term;
        r->voted_for = -1;                     /* a new term resets our vote     */
        changed = true;
    }
    r->role = ROLE_FOLLOWER;
    if (changed) persist_state(r);             /* term/vote change must be durable */
}

static void become_leader(struct raft *r)
{
    r->role = ROLE_LEADER;
    r->leader_id = r->id;
    /* Reinitialize per-peer replication indices (Raft Figure 2): optimistically
     * assume each peer matches our whole log, and re-discover the true match
     * point via AppendEntries failures. */
    for (int i = 0; i < r->npeers; i++) {
        r->peers[i].next_index = last_index(r) + 1;
        r->peers[i].match_index = 0;
    }
    TR("raft", "node=%d BECAME LEADER term=%llu lastIndex=%llu",
       r->id, (unsigned long long)r->current_term, (unsigned long long)last_index(r));
    /* Send an immediate round of heartbeats to assert leadership and stop other
     * nodes' election timers. */
    raft_on_heartbeat(r);
}

/* ---- sending helpers ------------------------------------------------------ */

/* Build the common payload prefix [type][term][from] into buf; return offset. */
static size_t put_prefix(uint8_t *buf, uint8_t type, uint64_t term, int from)
{
    buf[0] = type;
    put_u64(buf + 1, term);
    put_u32(buf + 9, (uint32_t)from);
    return 13;
}

/* Broadcast RequestVote to all peers (skips blocked links inside r->send). */
static void send_request_vote(struct raft *r)
{
    uint8_t buf[13 + 16];
    size_t n = put_prefix(buf, MSG_REQUEST_VOTE, r->current_term, r->id);
    put_u64(buf + n, last_index(r)); n += 8;   /* our lastLogIndex               */
    put_u64(buf + n, term_at(r, last_index(r))); n += 8; /* our lastLogTerm       */
    for (int i = 0; i < r->npeers; i++)
        r->send(r->net, r->peers[i].id, buf, n);
}

/* Send an AppendEntries to one peer, shipping entries starting at next_index.
 * A heartbeat is just an AppendEntries carrying zero entries. */
static void send_append_entries(struct raft *r, struct raft_peer *p)
{
    static uint8_t buf[DB_PEER_FRAME_MAX];     /* single-threaded ⇒ one scratch  */
    uint64_t prev = p->next_index - 1;         /* index right before what we send */
    size_t n = put_prefix(buf, MSG_APPEND_ENTRIES, r->current_term, r->id);
    put_u64(buf + n, prev); n += 8;                    /* prevLogIndex           */
    put_u64(buf + n, term_at(r, prev)); n += 8;        /* prevLogTerm            */
    put_u64(buf + n, r->commit_index); n += 8;         /* leaderCommit           */

    /* Fill up to AE_MAX_BATCH entries from next_index..last_index. */
    uint32_t cnt = 0;
    size_t cnt_off = n; n += 4;                         /* reserve the count slot */
    for (uint64_t idx = p->next_index;
         idx <= last_index(r) && cnt < AE_MAX_BATCH; idx++) {
        struct raft_entry *e = &r->log[idx];
        if (n + 20 + e->blen > sizeof buf) break;       /* frame ceiling          */
        put_u64(buf + n, e->term); n += 8;
        put_u64(buf + n, e->index); n += 8;
        put_u32(buf + n, e->blen); n += 4;
        memcpy(buf + n, e->body, e->blen); n += e->blen;
        cnt++;
    }
    put_u32(buf + cnt_off, cnt);                        /* backfill the real count */
    r->send(r->net, p->id, buf, n);
}

/* ---- incoming message handlers ------------------------------------------- */

static void on_request_vote(struct raft *r, const uint8_t *pl, size_t len)
{
    if (len < 13 + 16) return;                          /* malformed             */
    uint64_t term       = get_u64(pl + 1);
    int      cand       = (int)get_u32(pl + 9);
    uint64_t cand_lasti = get_u64(pl + 13);
    uint64_t cand_lastt = get_u64(pl + 21);

    /* Rule 1: a message from a higher term makes us step down first. */
    if (term > r->current_term) become_follower(r, term);

    bool granted = false;
    if (term >= r->current_term &&                      /* not from a stale term  */
        (r->voted_for == -1 || r->voted_for == cand)) { /* haven't voted / same   */
        /* "At least as up-to-date" (Raft §5.4.1): compare (lastTerm,lastIndex)
         * lexicographically. This is what preserves Leader Completeness — a node
         * missing a committed entry can never collect a majority. */
        uint64_t my_lastt = term_at(r, last_index(r));
        bool up_to_date = (cand_lastt > my_lastt) ||
                          (cand_lastt == my_lastt && cand_lasti >= last_index(r));
        if (up_to_date) {
            granted = true;
            r->voted_for = cand;
            persist_state(r);                            /* remember the vote      */
            r->reset_election(r->net);                   /* we saw a live candidate */
        }
    }

    uint8_t rsp[13 + 1];
    size_t n = put_prefix(rsp, MSG_REQUEST_VOTE_RSP, r->current_term, r->id);
    rsp[n++] = granted ? 1 : 0;
    r->send(r->net, cand, rsp, n);
    TR("vote", "node=%d %s vote to %d term=%llu", r->id,
       granted ? "GRANTED" : "denied", cand, (unsigned long long)r->current_term);
}

static void on_request_vote_rsp(struct raft *r, const uint8_t *pl, size_t len)
{
    if (len < 13 + 1) return;
    uint64_t term = get_u64(pl + 1);
    int from      = (int)get_u32(pl + 9);
    bool granted  = pl[13] != 0;

    if (term > r->current_term) { become_follower(r, term); return; }
    /* Ignore replies that aren't for our current election. */
    if (r->role != ROLE_CANDIDATE || term != r->current_term) return;

    if (granted) {
        /* Count each peer at most once per election. */
        for (int i = 0; i < r->npeers; i++) {
            if (r->peers[i].id == from && !r->peers[i].vote_granted) {
                r->peers[i].vote_granted = true;
                r->votes_granted++;
                break;
            }
        }
        if (r->votes_granted >= majority(r))            /* majority ⇒ we win      */
            become_leader(r);
    }
}

static void on_append_entries(struct raft *r, const uint8_t *pl, size_t len)
{
    if (len < 13 + 28) return;                          /* prefix + 3×u64 + count */
    uint64_t term     = get_u64(pl + 1);
    int      leader   = (int)get_u32(pl + 9);
    uint64_t prev_i   = get_u64(pl + 13);
    uint64_t prev_t   = get_u64(pl + 21);
    uint64_t l_commit = get_u64(pl + 29);
    uint32_t nent     = get_u32(pl + 37);
    const uint8_t *e  = pl + 41;
    const uint8_t *end = pl + len;

    /* A stale-term leader is rejected outright (Raft §5.1). */
    bool success = false;
    uint64_t match = 0;
    if (term < r->current_term) goto reply;

    /* Valid current-or-newer leader: adopt its term, become follower, and — the
     * important bit — RESET the election timer so we don't challenge it. */
    if (term > r->current_term) become_follower(r, term);
    r->role = ROLE_FOLLOWER;
    r->leader_id = leader;
    r->reset_election(r->net);

    /* Log-matching consistency check: we must already have prev_i@prev_t, else we
     * reject and the leader will retry with an earlier prevLogIndex, walking back
     * until our logs agree. */
    if (prev_i > last_index(r)) goto reply;             /* we're missing entries  */
    if (prev_i > 0 && term_at(r, prev_i) != prev_t) goto reply; /* term conflict  */

    /* Append/overwrite the shipped entries starting at prev_i+1. */
    for (uint32_t k = 0; k < nent; k++) {
        if (e + 20 > end) goto reply;                   /* truncated frame        */
        uint64_t et = get_u64(e);
        uint64_t ei = get_u64(e + 8);
        uint32_t bl = get_u32(e + 16);
        const uint8_t *body = e + 20;
        if (body + bl > end) goto reply;
        e = body + bl;

        if (ei <= last_index(r)) {
            /* We already have something at this index. If the term differs it is
             * a conflict: delete this entry and everything after it (§5.3), then
             * rewrite the on-disk log to match. */
            if (term_at(r, ei) != et) {
                for (uint64_t d = ei; d <= r->log_len; d++) free(r->log[d].body);
                r->log_len = ei - 1;
                if (log_persist_all(r) < 0) return;      /* durability first       */
            } else {
                continue;                                /* identical ⇒ skip       */
            }
        }
        /* Append the new entry (in memory + durably). */
        if (log_reserve(r, ei) < 0) return;
        struct raft_entry *ne = &r->log[ei];
        ne->term = et; ne->index = ei; ne->blen = bl;
        ne->body = malloc(bl ? bl : 1);
        if (!ne->body) return;
        memcpy(ne->body, body, bl);
        r->log_len = ei;
        if (log_persist_append(r, ne) < 0) return;
    }

    /* Advance our commit index: everything the leader has committed AND that we
     * now hold is safe to apply. */
    if (l_commit > r->commit_index) {
        uint64_t newc = l_commit < last_index(r) ? l_commit : last_index(r);
        r->commit_index = newc;
        apply_committed(r);
    }
    success = true;
    match = prev_i + nent;                               /* highest index we now hold */

reply: {
    uint8_t rsp[13 + 1 + 8];
    size_t n = put_prefix(rsp, MSG_APPEND_ENTRIES_RSP, r->current_term, r->id);
    rsp[n++] = success ? 1 : 0;
    put_u64(rsp + n, match); n += 8;
    r->send(r->net, leader, rsp, n);
}
}

static void on_append_entries_rsp(struct raft *r, const uint8_t *pl, size_t len)
{
    if (len < 13 + 1 + 8) return;
    uint64_t term  = get_u64(pl + 1);
    int      from  = (int)get_u32(pl + 9);
    bool     ok    = pl[13] != 0;
    uint64_t match = get_u64(pl + 14);

    if (term > r->current_term) { become_follower(r, term); return; }
    if (r->role != ROLE_LEADER || term != r->current_term) return;

    struct raft_peer *p = NULL;
    for (int i = 0; i < r->npeers; i++)
        if (r->peers[i].id == from) { p = &r->peers[i]; break; }
    if (!p) return;

    if (ok) {
        /* Advance this peer's replication progress. */
        if (match + 1 > p->next_index) p->next_index = match + 1;
        if (match > p->match_index)    p->match_index = match;

        /* Commit rule (Raft §5.3/§5.4.2): find the highest N replicated on a
         * MAJORITY whose entry is from the CURRENT term, and commit up to it. We
         * only commit current-term entries directly; earlier-term entries commit
         * transitively once a current-term entry above them commits. */
        for (uint64_t N = last_index(r); N > r->commit_index; N--) {
            if (term_at(r, N) != r->current_term) continue;
            int cnt = 1;                                 /* the leader has it too  */
            for (int i = 0; i < r->npeers; i++)
                if (r->peers[i].match_index >= N) cnt++;
            if (cnt >= majority(r)) {
                r->commit_index = N;
                apply_committed(r);
                break;
            }
        }
    } else {
        /* Follower rejected our prevLogIndex: back off and retry with an earlier
         * point. Decrement (bounded at 1) and immediately re-send. */
        if (p->next_index > 1) p->next_index--;
        send_append_entries(r, p);
    }
}

/* ---- public entry points -------------------------------------------------- */

void raft_on_message(struct raft *r, int from, const uint8_t *payload, size_t len)
{
    (void)from;                                         /* `from` is inside the payload */
    if (len < 1) return;
    switch (payload[0]) {
    case MSG_REQUEST_VOTE:        on_request_vote(r, payload, len);       break;
    case MSG_REQUEST_VOTE_RSP:    on_request_vote_rsp(r, payload, len);   break;
    case MSG_APPEND_ENTRIES:      on_append_entries(r, payload, len);     break;
    case MSG_APPEND_ENTRIES_RSP:  on_append_entries_rsp(r, payload, len); break;
    default: break;                                     /* unknown ⇒ ignore       */
    }
}

/* Election timeout fired: unless we are already leader, start a new election in
 * a higher term (Raft §5.2). */
void raft_on_election_timeout(struct raft *r)
{
    if (r->role == ROLE_LEADER) return;                 /* leaders never time out */

    r->current_term++;                                  /* bump the term          */
    r->role = ROLE_CANDIDATE;
    r->voted_for = r->id;                               /* vote for self          */
    persist_state(r);
    r->votes_granted = 1;                               /* self counts            */
    for (int i = 0; i < r->npeers; i++) r->peers[i].vote_granted = false;
    r->reset_election(r->net);                          /* fresh random timeout   */
    TR("raft", "node=%d start election term=%llu", r->id,
       (unsigned long long)r->current_term);

    if (majority(r) == 1) {                             /* single-node cluster    */
        become_leader(r);                               /* win instantly          */
        return;
    }
    send_request_vote(r);
}

/* Heartbeat timer fired: if leader, send AppendEntries (possibly empty) to every
 * peer. This both replicates new entries and refreshes followers' timers. */
void raft_on_heartbeat(struct raft *r)
{
    if (r->role != ROLE_LEADER) return;
    for (int i = 0; i < r->npeers; i++)
        send_append_entries(r, &r->peers[i]);
}

/* Client proposal: append `c` as a new log entry (leader only), persist it, and
 * kick replication. Returns the new index, 0 if not leader, -1 on I/O error. */
int64_t raft_client_propose(struct raft *r, const struct command *c)
{
    if (r->role != ROLE_LEADER) return 0;               /* caller should redirect */

    uint64_t idx = last_index(r) + 1;
    if (log_reserve(r, idx) < 0) return -1;
    struct raft_entry *e = &r->log[idx];
    e->term = r->current_term;
    e->index = idx;
    e->blen = (uint32_t)cmd_body_size(c);
    e->body = malloc(e->blen);
    if (!e->body) return -1;
    cmd_encode_body(e->body, c);
    r->log_len = idx;

    if (log_persist_append(r, e) < 0) return -1;        /* durable on the leader  */
    TR("raft", "node=%d propose idx=%llu term=%llu", r->id,
       (unsigned long long)idx, (unsigned long long)r->current_term);

    /* Replicate right away instead of waiting for the next heartbeat, so latency
     * is one round trip, not up to one heartbeat interval. */
    raft_on_heartbeat(r);

    /* Single-node: no peers, so a majority is just us — commit immediately. */
    if (majority(r) == 1) {
        r->commit_index = idx;
        apply_committed(r);
    }
    return (int64_t)idx;
}

bool raft_is_applied(struct raft *r, uint64_t index)
{
    return index <= r->last_applied;
}

/* ---- lifecycle ------------------------------------------------------------ */

void raft_add_peer(struct raft *r, int id, const char *host, int peer_port,
                   int client_port)
{
    if (r->npeers >= DB_MAX_PEERS) return;
    struct raft_peer *p = &r->peers[r->npeers++];
    memset(p, 0, sizeof *p);
    p->id = id;
    snprintf(p->host, sizeof p->host, "%s", host);
    p->peer_port = peer_port;
    p->client_port = client_port;
    p->out_fd = -1;                                     /* dialed lazily by server */
}

int raft_open(struct raft *r, int id, const char *data_dir, struct store *store)
{
    memset(r, 0, sizeof *r);
    r->id = id;
    r->role = ROLE_FOLLOWER;
    r->voted_for = -1;
    r->leader_id = -1;
    r->store = store;
    /* r->send / r->reset_election / r->net are installed by server_run(). */
    snprintf(r->dir, sizeof r->dir, "%s", data_dir);

    char path[600];
    snprintf(path, sizeof path, "%s/raft.state", data_dir);
    r->state_fd = open(path, O_RDWR | O_CREAT | O_CLOEXEC, 0644);
    if (r->state_fd < 0) return -1;
    snprintf(path, sizeof path, "%s/raft.log", data_dir);
    r->raftlog_fd = open(path, O_RDWR | O_CREAT | O_CLOEXEC, 0644);
    if (r->raftlog_fd < 0) { close(r->state_fd); return -1; }

    load_state(r);                                      /* term + votedFor        */
    if (log_replay(r) < 0) { raft_close(r); return -1; } /* rebuild log[]         */
    TR("raft", "node=%d opened term=%llu lastIndex=%llu", r->id,
       (unsigned long long)r->current_term, (unsigned long long)last_index(r));
    return 0;
}

void raft_close(struct raft *r)
{
    for (uint64_t i = 1; i <= r->log_len; i++) free(r->log[i].body);
    free(r->log);
    r->log = NULL; r->log_len = r->log_cap = 0;
    if (r->state_fd >= 0)   close(r->state_fd);
    if (r->raftlog_fd >= 0) close(r->raftlog_fd);
    r->state_fd = r->raftlog_fd = -1;
}
