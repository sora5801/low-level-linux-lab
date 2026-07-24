/* ===========================================================================
 * raft.h — the whole Raft data model in one place.
 * ===========================================================================
 *
 * This header defines every type the consensus core touches: the persistent
 * per-node state (term / votedFor / log), the volatile leader bookkeeping
 * (nextIndex / matchIndex / commitIndex), the five RPC message shapes, and the
 * node/cluster objects. Read it top-to-bottom once and the .c files stop being
 * mysterious — every function there just moves a node between the states named
 * here in response to the messages named here.
 *
 * WHY RAFT AT ALL.  A single machine is a single point of failure. To keep a
 * service available while machines crash, you replicate its state across N
 * machines — but then the replicas must AGREE on the order of operations, even
 * as machines crash, restart, and the network drops or reorders packets. That
 * agreement problem is "consensus." Raft is a consensus algorithm designed to
 * be *understandable*: it decomposes the problem into (1) leader election,
 * (2) log replication, and (3) safety, and it maintains a strong leader so all
 * changes flow one direction (leader -> followers). This lab implements that
 * decomposition as a readable teaching core.
 *
 * THE ONE INVARIANT THAT MAKES IT WORK (State Machine Safety): if any node has
 * applied the log entry at index i to its state machine, then no other node
 * will ever apply a *different* entry at index i. Everything below — the term
 * numbers, the election restriction, the log-matching check, the commit rule,
 * the fsync ordering — exists to preserve exactly that invariant. Comments
 * throughout point back to it.
 *
 * Platform: Linux / WSL2. Uses pthreads (one thread per node), CLOCK_MONOTONIC
 * timers, and fsync-ordered durable writes (open/write/fsync/rename). Nodes
 * talk over an in-process simulated network (see net.c) whose partition matrix
 * makes "inject a network split" a one-line call — that is deliberately how the
 * Raft authors tested it, and it keeps the algorithm un-obscured by socket code.
 * =========================================================================== */
#ifndef RAFT_H
#define RAFT_H

#include <stdint.h>   /* uint64_t/uint32_t — terms and indices are 64-bit       */
#include <stdbool.h>  /* bool for the many yes/no flags                          */
#include <stddef.h>   /* size_t                                                  */
#include <pthread.h>  /* one thread + one mutex + one condvar per node           */

/* ---- Cluster / message sizing (bounded for a teaching core) --------------- */
#define RAFT_MAX_NODES     7    /* cluster is fixed-size; 3 or 5 is typical      */
#define RAFT_CMD_MAX      48    /* bytes in one state-machine command string     */
#define RAFT_MAX_BATCH    16    /* max log entries carried in one AppendEntries   */
#define RAFT_SNAP_MAX   4096    /* max serialized snapshot bytes (bounds InstallSnapshot) */

/* ---- Timers (milliseconds). Randomized election timeout is THE trick that
 * makes leader election terminate: if every node waited the same time, they
 * would all become candidates together, split the vote, and livelock. Each node
 * instead waits a random time in [MIN, MAX], so one node almost always times out
 * first, becomes candidate, and wins before the others wake. HEARTBEAT must be
 * comfortably smaller than ELECTION_MIN so a healthy leader's heartbeats keep
 * resetting followers' election timers before they fire. ------------------- */
#define RAFT_ELECTION_MIN_MS  250
#define RAFT_ELECTION_MAX_MS  500
#define RAFT_HEARTBEAT_MS      60

/* Trigger a snapshot once the in-memory log grows past this many entries. Real
 * systems use byte thresholds; a count keeps the teaching harness legible. */
#define RAFT_SNAPSHOT_THRESHOLD 12

/* ---- Roles. A node is always in exactly one of these three states. --------
 * Follower  : passive; redirects clients to the leader, resets its election
 *             timer whenever it hears from a current leader or grants a vote.
 * Candidate : ran out of patience, bumped the term, and is soliciting votes.
 * Leader    : won a majority; the sole node that accepts client writes and
 *             pushes them to followers via AppendEntries. */
enum raft_role { ROLE_FOLLOWER, ROLE_CANDIDATE, ROLE_LEADER };

/* ---------------------------------------------------------------------------
 * One log entry — the atom of replication.
 *
 * `term` is the leader's term when the entry was CREATED. It is the fingerprint
 * that the Log Matching Property relies on: if two logs contain an entry with
 * the same index AND the same term, Raft guarantees the entries are identical
 * and, by induction, so is everything before them. That is why AppendEntries
 * carries (prevLogIndex, prevLogTerm): one term+index match certifies the whole
 * prefix, so followers can detect and repair divergence with a single check.
 *
 * `index` is the entry's 1-based position in the replicated log. Index 0 is the
 * sentinel "before the log begins" (term 0). We store the index explicitly in
 * every entry rather than inferring it from the array slot because log
 * compaction (snapshotting) drops a prefix of the array, so slot 0 does NOT map
 * to index 1 after the first snapshot. See struct raft_log's accessors.
 * --------------------------------------------------------------------------- */
struct log_entry {
    uint64_t term;              /* term in which the leader created this entry   */
    uint64_t index;            /* 1-based log position (0 = the sentinel below) */
    uint16_t cmd_len;          /* bytes of `cmd` that are meaningful             */
    char     cmd[RAFT_CMD_MAX];/* opaque state-machine command, e.g. "SET x 42" */
};

/* ---------------------------------------------------------------------------
 * The log, with its snapshot base.
 *
 * `entries[0]` corresponds to index `snap_last_index + 1`. Everything at or
 * below `snap_last_index` has been compacted into the snapshot (the state
 * machine already reflects it), so it no longer lives in the array. The four
 * accessor helpers in raft.c (log_last_index / log_term_at / log_get /
 * log_get_from) are the ONLY code that does this index<->slot arithmetic, so
 * the snapshot offset is handled in exactly one place.
 * --------------------------------------------------------------------------- */
struct raft_log {
    struct log_entry *entries;  /* dynamic array of live (un-compacted) entries  */
    size_t   count;             /* number of live entries                        */
    size_t   cap;               /* allocated capacity of `entries`               */
    uint64_t snap_last_index;   /* lastIncludedIndex: highest index in snapshot  */
    uint64_t snap_last_term;    /* lastIncludedTerm: its term (for prev-log check)*/
};

/* ===========================================================================
 * RPC MESSAGES. Raft has three RPCs; each is a request + a reply, so five wire
 * shapes below (ClientRequest is delivered in-process, not as a network msg).
 * Every request and reply carries `term`: the term is a logical clock, and the
 * universal rule "if a message's term > mine, step down to follower and adopt
 * it; if it is < mine, reject the message as stale" is what lets a node that was
 * partitioned away safely rejoin without corrupting anything.
 * =========================================================================== */

enum msg_type {
    MSG_REQUEST_VOTE,           /* candidate -> peers: "vote for me"             */
    MSG_REQUEST_VOTE_REPLY,     /* peer -> candidate: granted / denied           */
    MSG_APPEND_ENTRIES,         /* leader -> followers: replicate + heartbeat    */
    MSG_APPEND_ENTRIES_REPLY,   /* follower -> leader: success / conflict hint   */
    MSG_INSTALL_SNAPSHOT,       /* leader -> lagging follower: ship the snapshot */
    MSG_INSTALL_SNAPSHOT_REPLY  /* follower -> leader: installed up to index     */
};

/* RequestVote (§5.2, §5.4.1). A candidate asks a peer for its vote. The peer
 * grants at most one vote per term (persisted!), and ONLY if the candidate's
 * log is at least as up-to-date as its own — the "election restriction" that
 * guarantees the winner already holds every committed entry. */
struct msg_request_vote {
    uint64_t term;              /* candidate's term                              */
    int      candidate_id;      /* who is asking (so the voter can record it)    */
    uint64_t last_log_index;    /* index of candidate's last log entry ...       */
    uint64_t last_log_term;    /* ... and its term — the up-to-date comparison  */
};
struct msg_request_vote_reply {
    uint64_t term;              /* voter's term (may teach the candidate it lost)*/
    bool     vote_granted;      /* true iff the vote was granted                 */
};

/* AppendEntries (§5.3). Doubles as the heartbeat (n_entries == 0). Carries the
 * (prevLogIndex, prevLogTerm) consistency anchor plus up to RAFT_MAX_BATCH new
 * entries, and the leader's commitIndex so followers can advance their own. */
struct msg_append_entries {
    uint64_t term;              /* leader's term                                 */
    int      leader_id;         /* so followers can redirect clients to it       */
    uint64_t prev_log_index;    /* index immediately BEFORE the new entries      */
    uint64_t prev_log_term;    /* term of that entry — the log-matching anchor  */
    uint64_t leader_commit;     /* leader's commitIndex (followers clamp to this)*/
    int      n_entries;         /* 0 = pure heartbeat                            */
    struct log_entry entries[RAFT_MAX_BATCH];
};
struct msg_append_entries_reply {
    uint64_t term;              /* follower's term                               */
    bool     success;           /* true iff the prev-log check passed            */
    uint64_t match_index;       /* on success: highest index now known matching  */
    uint64_t conflict_index;    /* on failure: fast-backup hint for nextIndex    */
};

/* InstallSnapshot (§7). Used when a follower is so far behind that the entries
 * it needs have already been compacted out of the leader's log. Rather than an
 * impossible AppendEntries, the leader ships the whole snapshot. Real Raft
 * chunks this; we send it in one bounded message for clarity. */
struct msg_install_snapshot {
    uint64_t term;              /* leader's term                                 */
    int      leader_id;
    uint64_t last_included_index;/* snapshot replaces the log up to here          */
    uint64_t last_included_term;
    uint32_t data_len;          /* bytes of serialized state-machine snapshot    */
    char     data[RAFT_SNAP_MAX];
};
struct msg_install_snapshot_reply {
    uint64_t term;
};

/* ---------------------------------------------------------------------------
 * A message envelope. Heap-allocated by the sender; OWNERSHIP transfers to the
 * receiver's inbox on a successful net_send(); the receiver frees it after
 * handling. The `next` link threads the message onto exactly one intrusive
 * singly-linked list at a time — either a node's inbox or a node's pending
 * out-queue, never both. Reusing one link is safe because a message is produced,
 * queued out, sent, queued in, handled, and freed, strictly in that order.
 * --------------------------------------------------------------------------- */
struct message {
    int  from;                  /* source node id (for reply routing / logging)  */
    int  to;                    /* destination node id                           */
    enum msg_type type;
    union {                     /* only the member named by `type` is valid      */
        struct msg_request_vote            rv;
        struct msg_request_vote_reply      rvr;
        struct msg_append_entries          ae;
        struct msg_append_entries_reply    aer;
        struct msg_install_snapshot        is;
        struct msg_install_snapshot_reply  isr;
    } u;
    struct message *next;       /* intrusive queue link (see ownership note)     */
};

/* ---- The replicated state machine: a tiny key/value store (kv.c). ---------
 * Raft replicates an opaque *command log*; the state machine gives those
 * commands meaning. Ours understands "SET k v" and "DEL k". Because every node
 * applies the identical committed command sequence, every node's kv_store ends
 * up identical — that is the entire point of consensus, made concrete. */
#define KV_CAPACITY 64
#define KV_KEYLEN   24
#define KV_VALLEN   24
struct kv_pair { bool used; char key[KV_KEYLEN]; char val[KV_VALLEN]; };
struct kv_store { struct kv_pair pairs[KV_CAPACITY]; };

struct cluster; /* forward decl; defined below after the node */

/* ===========================================================================
 * A single Raft node. The mutex `mu` guards EVERY field below it (state + inbox
 * + out-queue). The locking discipline that keeps the cluster deadlock-free:
 * a thread holds at most ONE node's `mu` at a time, and never calls into the
 * network (which briefly locks a *destination* node's mu) while holding its own.
 * The node loop therefore (1) locks mu, processes messages and timers, appends
 * any outgoing messages to `out_head`, (2) UNLOCKS, (3) flushes the out-queue
 * via the network. That ordering is why two nodes messaging each other can never
 * deadlock waiting on each other's mu.
 * =========================================================================== */
struct raft_node {
    int id;                     /* 0 .. n_nodes-1                                */
    int n_nodes;                /* cluster size (majority = n_nodes/2 + 1)       */
    struct cluster *cluster;    /* back-pointer for network + peer access        */

    /* ---- PERSISTENT state: survives a crash; fsync'd BEFORE any RPC reply
     * that depends on it. If this were lost on restart, a node could vote twice
     * in a term or forget a committed entry, breaking State Machine Safety. --*/
    uint64_t current_term;      /* latest term this node has seen (monotonic)    */
    int      voted_for;         /* candidate voted for this term, or -1 for none */
    struct raft_log log;        /* the replicated command log                    */

    /* ---- VOLATILE state (all roles). Rebuilt from the snapshot on restart;
     * commit/apply progress is re-learned from the leader, never persisted. ---*/
    enum raft_role role;
    uint64_t commit_index;      /* highest index known committed (safe to apply) */
    uint64_t last_applied;      /* highest index handed to the state machine     */

    /* ---- VOLATILE leader state, reinitialized on every election win. --------
     * next_index[i]  : the next log index the leader will try to send peer i.
     *                  Optimistically initialized to last_index+1, backed up on
     *                  rejection until it finds where peer i's log agrees.
     * match_index[i] : the highest index KNOWN to be replicated on peer i.
     *                  Conservatively 0 until proven; drives commit advancement.*/
    uint64_t next_index[RAFT_MAX_NODES];
    uint64_t match_index[RAFT_MAX_NODES];

    /* ---- Election bookkeeping (candidate). ---------------------------------*/
    int      votes_granted;                 /* votes tallied this election       */
    bool     vote_from[RAFT_MAX_NODES];     /* dedupe: count each peer once      */

    /* ---- Timers, in CLOCK_MONOTONIC nanoseconds. ---------------------------*/
    uint64_t election_deadline; /* if now >= this and not leader -> new election */
    uint64_t heartbeat_deadline;/* if now >= this and leader -> send AppendEntries*/
    uint32_t rng;               /* per-node xorshift state for timeout jitter    */

    struct kv_store kv;         /* the replicated state machine itself           */

    /* Cached serialized snapshot == the state machine AT snap_last_index. The
     * live `kv` advances past that base as later entries are applied, so we can
     * no longer re-serialize it to ship an InstallSnapshot. We therefore snapshot
     * the bytes at the moment of compaction (and after loading from disk) and
     * keep them here to send to a lagging follower. */
    char     snap_data[RAFT_SNAP_MAX];
    uint32_t snap_len;

    char dir[256];              /* on-disk directory for this node's state/snapshot*/

    /* ---- Threading / mailbox (all guarded by `mu`). ------------------------*/
    pthread_t       thread;
    pthread_mutex_t mu;         /* the node's single big lock (see class comment)*/
    pthread_cond_t  inbox_cv;   /* signaled on message arrival or shutdown        */
    struct message *inbox_head, *inbox_tail;  /* FIFO of incoming messages       */
    struct message *out_head,  *out_tail;     /* messages to flush after unlock  */
    bool running;               /* false -> the node thread should exit          */
    bool stopped;               /* true  -> simulated crash: ignore all input    */
};

/* ===========================================================================
 * The cluster: the nodes plus the simulated network. `reachable[a][b]` is the
 * partition matrix — true iff a message from a may be delivered to b. Injecting
 * a partition is just flipping a rectangle of this matrix to false; healing is
 * setting it all true. Because reachability is checked at SEND time under
 * `net_mu`, a heal takes effect on the very next message with no per-node state
 * to reconcile. `net_mu` is a leaf lock: it is always taken alone and released
 * before any node's `mu`, so it can never be part of a lock cycle.
 * =========================================================================== */
struct cluster {
    int n;
    struct raft_node *nodes[RAFT_MAX_NODES];
    pthread_mutex_t net_mu;                              /* guards the fields below */
    bool     reachable[RAFT_MAX_NODES][RAFT_MAX_NODES];  /* partition matrix        */
    unsigned drop_permille;                              /* random loss, parts/1000 */
    uint32_t net_rng;                                    /* PRNG for loss decisions */
};

/* ===========================================================================
 * PUBLIC API — everything the test harness (main.c) calls. All of these are
 * thread-safe: they take the relevant lock internally.
 * =========================================================================== */

/* Lifecycle. cluster_create allocates N nodes, each with its own persistence
 * directory `<state_dir>/node<i>`, loading any state left by a prior run. */
struct cluster *cluster_create(int n, const char *state_dir);
void            cluster_start(struct cluster *c);   /* spawn the node threads    */
void            cluster_stop(struct cluster *c);    /* join threads, free memory */

/* Client interface. Submit a command to node `id`; returns the log index it was
 * appended at, or -1 if that node is not the current leader (the harness then
 * retries on the real leader). raft_wait_commit blocks until `index` is
 * committed cluster-wide or the timeout elapses. */
long raft_submit(struct cluster *c, int id, const char *cmd);
bool raft_wait_commit(struct cluster *c, long index, int timeout_ms);

/* Observation helpers (all snapshot the field under the node's lock). */
int      raft_find_leader(struct cluster *c);       /* a node that believes it   */
                                                    /* is leader, or -1          */
uint64_t raft_current_term(struct cluster *c, int id);
uint64_t raft_commit_index(struct cluster *c, int id);
uint64_t raft_snapshot_index(struct cluster *c, int id); /* lastIncludedIndex   */
uint64_t raft_last_log_index(struct cluster *c, int id);  /* end of the log      */
bool     raft_kv_get(struct cluster *c, int id, const char *key, char *out, size_t outlen);

/* Fault injection — the whole reason this is a *test* harness. */
void net_partition(struct cluster *c, const int *group, int gsize); /* isolate group */
void net_heal(struct cluster *c);                                   /* full mesh     */
void net_set_loss(struct cluster *c, unsigned permille);            /* random drops  */
void raft_crash_node(struct cluster *c, int id);   /* simulate power loss         */
void raft_restart_node(struct cluster *c, int id); /* reboot: reload ONLY from disk*/

/* ===========================================================================
 * INTERNAL cross-file helpers (not for the harness, but shared between the .c
 * files). Grouped here so every translation unit sees one consistent set of
 * prototypes.
 * =========================================================================== */

/* kv.c — the state machine. */
void   kv_reset(struct kv_store *kv);
bool   kv_get(struct kv_store *kv, const char *key, char *out, size_t outlen);
void   kv_apply(struct kv_store *kv, const char *cmd, uint16_t cmd_len);
size_t kv_serialize(const struct kv_store *kv, char *buf, size_t cap);
void   kv_deserialize(struct kv_store *kv, const char *buf, size_t len);

/* persist.c — durable state with fsync ordering. All return 0 on success, -1 on
 * an I/O error (with errno set). persist_load repopulates term/votedFor/log and
 * the state machine from disk, or leaves a fresh node's fields at their zero
 * defaults if no prior state exists. */
uint32_t crc32_ieee(const void *data, size_t len);
int      persist_save_state(struct raft_node *n);
int      persist_save_snapshot(struct raft_node *n, const char *data, uint32_t len);
int      persist_load(struct raft_node *n);

/* net.c — the simulated network. msg_alloc heap-allocates a zeroed envelope the
 * caller fills and then hands to net_send, which takes ownership: on delivery it
 * is enqueued to the destination inbox, and on a drop (partition or random loss)
 * it is freed. Either way the caller must NOT touch it afterwards. */
struct message *msg_alloc(int from, int to, enum msg_type type);
void            net_send(struct cluster *c, struct message *m);

#endif /* RAFT_H */
