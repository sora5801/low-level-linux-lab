/* ===========================================================================
 * db.h — the one shared header for the "networked database" teaching core.
 * ===========================================================================
 *
 * WHY ONE HEADER
 * --------------
 * A distributed KV database is naturally many modules (storage, log, consensus,
 * networking, event loop). To keep the *cross-references* obvious in a teaching
 * codebase, every type, constant, and inter-module prototype lives here, and
 * each .c file includes exactly this. Read this file first: it is the map of the
 * whole node. The prose comments below are the architecture lecture; the .c
 * files are the implementation of each lecture point.
 *
 * THE NODE AT A GLANCE
 * --------------------
 * One process = one database node. A single epoll event loop (server.c) drives
 * everything — there are no threads, so there are no locks, which is the whole
 * point of the reactor pattern. The loop multiplexes:
 *
 *   - a CLIENT listen socket  (line protocol: PUT/GET/DEL/ADMIN)
 *   - a PEER   listen socket  (binary Raft RPC, length-framed + CRC32)
 *   - N outgoing PEER sockets (we DIAL every peer; sends go out here)
 *   - accepted incoming peer sockets (receives come in here)
 *   - two timerfds            (Raft election timeout + leader heartbeat)
 *
 * DATA FLOW OF A WRITE (the money path)
 * -------------------------------------
 *   client ──PUT k v──▶ leader.server
 *                         │  (followers reply "-ERR notleader host:port")
 *                         ▼
 *                       raft_client_propose()  ── append to REPLICATION log
 *                         │                        (raft.log, CRC+framed, fsync)
 *                         ▼  AppendEntries RPC to followers ───────────────┐
 *                       (majority persist the entry) ◀──── AE replies ─────┘
 *                         ▼  commitIndex advances
 *                       raft_apply_committed()
 *                         ▼
 *                       store_apply()  ── append to STATE-MACHINE log
 *                         │                (store.wal, CRC+framed, fsync)
 *                         ▼
 *                       hash table mutated ; client gets "+OK"
 *
 * WHY TWO LOGS? (an honesty note that is itself the lesson)
 * --------------------------------------------------------
 * Real replicated stores keep *two* durable logs, and so do we:
 *   1. raft.log   — the REPLICATION log. Raft's correctness requires each node
 *                   to remember its log across crashes (§5.3, Figure 2). This is
 *                   the analogue of etcd's `wal/`.
 *   2. store.wal  — the STATE-MACHINE (storage-engine) write-ahead log. Applying
 *                   a committed command durably updates the KV; on reboot we
 *                   replay it to rebuild the in-memory index. This is the
 *                   embedded-db lesson (../../02-systems-tools/13-embedded-db).
 * A production system would put a B-tree/LSM behind store.wal and snapshot the
 * Raft log; we use a hash table and never compact. Those omissions are called
 * out in the README's Scope section — see it for exactly what is and isn't here.
 *
 * SINGLE-NODE MODE still exercises the full stack: with zero peers a node wins
 * an election of one instantly, every entry commits with no network round trip,
 * and you get a durable KV (raft.log + store.wal + fsync + recovery) in one
 * binary. That is the smallest thing that runs, and it runs the real code paths.
 * ===========================================================================
 */
#ifndef DB_H
#define DB_H

/* _GNU_SOURCE must precede the first system header: it exposes accept4(),
 * O_CLOEXEC on some libcs, and the timerfd/eventfd families we rely on. */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdint.h>   /* fixed-width ints — on-disk/on-wire formats must be exact */
#include <stddef.h>   /* size_t                                                    */
#include <stdbool.h>  /* bool                                                      */

/* ---------------------------------------------------------------------------
 * Compile-time limits. Bounded on purpose: a teaching core rejects oversized
 * input rather than growing unbounded buffers (that is a separate lesson, and
 * an unbounded buffer here would just be an untested slow path).
 * ------------------------------------------------------------------------- */
#define DB_MAX_PEERS        7      /* other nodes; cluster size <= 8              */
#define DB_MAX_KEY          256    /* bytes; keys longer than this are rejected   */
#define DB_MAX_VAL          4096   /* bytes; values longer than this are rejected */
#define DB_HASH_BUCKETS     4096   /* KV index buckets (power of two; & to mask)  */

/* Client protocol line buffer. One request must fit; oversize => error + close. */
#define DB_CLIENT_BUF       (DB_MAX_KEY + DB_MAX_VAL + 64)

/* Peer RPC frame ceiling. An AppendEntries with a batch of entries must fit;
 * anything larger is treated as a protocol error and the peer link is reset. */
#define DB_PEER_FRAME_MAX   (1u << 20)   /* 1 MiB — generous for teaching batches */

/* Raft timing. The cardinal rule (Raft §5.6): election timeout must be an order
 * of magnitude larger than the heartbeat interval and the network round trip, or
 * leaders get spuriously deposed. Election timeouts are RANDOMIZED per node in
 * [MIN,MAX) so that split votes resolve quickly instead of livelocking. */
#define DB_HEARTBEAT_MS     100    /* leader emits AppendEntries this often       */
#define DB_ELECTION_MIN_MS  400    /* >= ~4x heartbeat; randomized up to MAX      */
#define DB_ELECTION_MAX_MS  800

/* ---------------------------------------------------------------------------
 * The command that flows through both logs and over the wire.
 *
 * A "command" is a single mutation of the KV state machine. Its serialized form
 * is IDENTICAL whether it lives in a Raft log entry, a store.wal record, or an
 * AppendEntries payload — one encoder to learn, reused everywhere. The framing
 * is the subject of asm/demo.c. Wire layout of a command's body:
 *
 *     [u8  op] [u32 klen] [key bytes] [u32 vlen] [val bytes]
 *
 * op = OP_DEL has vlen == 0 and no value bytes. All multi-byte integers are
 * little-endian on the wire (see put_u32/get_u32) — we pick one endianness and
 * commit to it so a log written on one machine reads back the same on another.
 * ------------------------------------------------------------------------- */
enum cmd_op {
    OP_PUT = 1,   /* set key = val                                              */
    OP_DEL = 2    /* delete key (val ignored)                                   */
};

/* A decoded command, pointing INTO a caller-owned buffer (zero-copy slices).
 * The pointers are borrowed: valid only as long as the backing buffer lives. */
struct command {
    uint8_t     op;
    const char *key; uint32_t klen;
    const char *val; uint32_t vlen;
};

/* ===========================================================================
 * crc32.c — record integrity.
 * ===========================================================================
 * Every durable record and every peer frame carries a CRC32 (IEEE 802.3, the
 * same polynomial gzip/zlib use). It is NOT cryptographic; its job is to catch a
 * torn write (a partial fsync after a crash) or a corrupted frame so we STOP at
 * the last good record instead of feeding garbage into the state machine. This
 * routine is the heart of asm/demo.c. */
uint32_t crc32_ieee(const void *data, size_t len);

/* ===========================================================================
 * Little-endian integer (de)serialization — the "length framing" primitives.
 * ===========================================================================
 * Defined inline here because they are tiny, hot, and used by every module.
 * We never memcpy a struct to disk/wire (padding + host endianness make that
 * non-portable); we spell out each byte instead. */
static inline void put_u32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v);          /* low byte first == little-endian            */
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}
static inline uint32_t get_u32(const uint8_t *p) {
    return (uint32_t)p[0]        | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16)| ((uint32_t)p[3] << 24);
}
static inline void put_u64(uint8_t *p, uint64_t v) {
    put_u32(p, (uint32_t)v);              /* low 32                             */
    put_u32(p + 4, (uint32_t)(v >> 32));  /* high 32                            */
}
static inline uint64_t get_u64(const uint8_t *p) {
    return (uint64_t)get_u32(p) | ((uint64_t)get_u32(p + 4) << 32);
}

/* ===========================================================================
 * store.c / wal.c — the storage engine (KV hash table + state-machine WAL).
 * ===========================================================================
 * This is the "embedded database" behind Raft. It knows nothing about
 * consensus: it is a durable map<string,string>. Raft calls store_apply() once
 * an entry is committed; recovery replays store.wal on boot. */

/* One key/value pair, chained in a hash bucket. Keys and values are heap copies
 * owned by the entry (freed on delete / at shutdown). */
struct kv_entry {
    struct kv_entry *next;   /* separate chaining: collisions form a list       */
    uint32_t         hash;   /* cached full hash, so lookups skip most strcmps  */
    uint32_t         klen, vlen;
    char            *key;    /* owned                                           */
    char            *val;    /* owned                                           */
};

/* The storage engine handle. One per node. */
struct store {
    struct kv_entry *buckets[DB_HASH_BUCKETS]; /* the index (separate chaining) */
    size_t           count;                     /* live keys (for stats)        */
    int              wal_fd;                     /* append-only store.wal fd     */
    char             wal_path[512];
};

/* Lifecycle + operations. store_recover() opens store.wal, replays every intact
 * record into the hash table (stopping at the first bad CRC == torn tail), then
 * leaves the fd positioned for appends. Returns 0 on success, -1 on fatal I/O. */
int  store_open(struct store *s, const char *data_dir);
void store_close(struct store *s);

/* store_apply: durably apply a committed command. Appends a CRC+framed record to
 * store.wal, fsync's it (unless `sync` is false — used during bulk replay), then
 * mutates the in-memory table. This is the ONLY path that writes state.wal. */
int  store_apply(struct store *s, const struct command *c, bool sync);

/* Read path — borrows into the live table; copy out before the next mutation.
 * Returns 1 and sets *val/*vlen if present, 0 if absent. */
int  store_get(struct store *s, const char *key, uint32_t klen,
               const char **val, uint32_t *vlen);

/* Command body (de)serialization — the shared unit of both logs and the wire.
 * The body is [u8 op][u32 klen][key][u32 vlen][val]. cmd_body_size returns its
 * length; cmd_encode_body writes it (out must hold cmd_body_size bytes) and
 * returns bytes written; cmd_decode_body parses `len` bytes into `c` (whose key/
 * val borrow into `in`), returning 0 on success or -1 if the bytes are
 * malformed/short. These live in wal.c and are reused verbatim by raft.c. */
size_t cmd_body_size(const struct command *c);
size_t cmd_encode_body(uint8_t *out, const struct command *c);
int    cmd_decode_body(const uint8_t *in, size_t len, struct command *c);

/* wal.c internals reused by store.c (record encode + the replay driver). Split
 * into wal.c so the WAL — the star of the storage half — reads as its own unit.
 * wal_encode_record serializes [reclen][crc][command] into `out` (which must
 * hold at least wal_record_size(c) bytes) and returns the total byte count. */
size_t wal_record_size(const struct command *c);
size_t wal_encode_record(uint8_t *out, const struct command *c);
/* wal_replay: read every record from `fd`, decode, and invoke apply(ctx, &cmd)
 * for each intact one. Stops (cleanly) at EOF or the first corrupt/torn record,
 * truncating the file to the last good offset so future appends are contiguous.
 * Returns the number of records replayed, or -1 on fatal error. */
long wal_replay(int fd, void (*apply)(void *ctx, const struct command *c),
                void *ctx);

/* ===========================================================================
 * raft.c — leader election + log replication (Raft, Ongaro & Ousterhout 2014).
 * ===========================================================================
 * We implement the core of Figure 2: persistent state (currentTerm, votedFor,
 * log[]), the RequestVote and AppendEntries RPCs, randomized election timeouts,
 * and the commit rule. Snapshots/§7 membership changes are omitted (see README
 * Scope). Reads: served on the leader from applied state (not linearizable
 * without a read-index/lease — documented omission). */

enum raft_role { ROLE_FOLLOWER = 0, ROLE_CANDIDATE, ROLE_LEADER };

/* An in-memory Raft log entry. index is 1-based (index 0 is the "before the log"
 * sentinel). `body`/`blen` hold the serialized command (the same bytes as a
 * store.wal record's command body) so replication is just shipping these bytes. */
struct raft_entry {
    uint64_t term;    /* term in which the leader created the entry            */
    uint64_t index;   /* position in the log (1-based)                         */
    uint8_t *body;    /* owned: serialized command [op][klen][key][vlen][val]  */
    uint32_t blen;
};

/* Per-peer replication bookkeeping (leader-only volatile state, Raft Figure 2).
 * `blocked` is our injected-partition switch (see server.c ADMIN command): when
 * true we drop everything to/from this peer, simulating a network partition
 * without needing root/iptables. */
struct raft_peer {
    int      id;                 /* peer node id                               */
    char     host[64];           /* dial target for the outgoing socket        */
    int      client_port;        /* (kept for redirect hints, unused by raft)  */
    int      peer_port;          /* Raft RPC port we dial                      */
    int      out_fd;             /* our outgoing socket to this peer (or -1)   */
    bool     out_connecting;     /* nonblocking connect() in progress          */
    uint64_t next_index;         /* next log index to send (leader)            */
    uint64_t match_index;        /* highest index known replicated (leader)    */
    bool     vote_granted;       /* did this peer vote for us this election?   */
    bool     blocked;            /* injected partition: drop all traffic       */
    /* out_fd is our write-only socket to this peer, dialed by server.c. Incoming
     * frames arrive on a SEPARATE accepted socket that server.c tracks in its own
     * connection table (a frame is self-identifying via its `from` field), so no
     * receive buffer lives here — the peer struct is Raft-logic + the send fd. */
};

/* The Raft consensus module state — one per node. */
struct raft {
    int              id;                     /* this node's id                 */
    enum raft_role   role;
    /* --- persistent state (survives crashes; fsync'd on change) ------------ */
    uint64_t         current_term;           /* latest term seen               */
    int              voted_for;              /* candidate voted for this term, or -1 */
    struct raft_entry *log;                  /* dynamic array, log[0] unused   */
    uint64_t         log_len;                /* number of entries (== last idx)*/
    uint64_t         log_cap;
    int              raftlog_fd;             /* append-only raft.log fd        */
    int              state_fd;               /* raft.state (term+votedFor)     */
    char             dir[512];
    /* --- volatile state --------------------------------------------------- */
    uint64_t         commit_index;           /* highest known-committed index  */
    uint64_t         last_applied;           /* highest applied to the store   */
    int              leader_id;              /* current leader (for redirects) */
    int              votes_granted;          /* tally during an election       */
    /* --- cluster ---------------------------------------------------------- */
    struct raft_peer peers[DB_MAX_PEERS];
    int              npeers;
    /* --- wiring ----------------------------------------------------------- */
    struct store    *store;                  /* the state machine we apply to  */
    /* Callbacks into server.c (same process; server owns the fds and timers):
     *  - send: transmit `len` payload bytes to peer id. server.c adds the length
     *    frame + CRC and does the socket write; returns <0 if the link is down or
     *    blocked (a partition). Raft treats a failed send as a dropped message —
     *    exactly the fault it is built to survive — and retries via heartbeats.
     *  - reset_election: re-arm the election timerfd with a FRESH RANDOM timeout.
     *    Called whenever we must not time out yet: after granting a vote, after a
     *    valid AppendEntries from the current leader, and when starting our own
     *    election. Randomization is what breaks split-vote livelock (Raft §5.2). */
    int            (*send)(void *net, int peer_id, const uint8_t *buf, size_t len);
    void           (*reset_election)(void *net);
    void            *net;                    /* opaque server context           */
};

/* Raft lifecycle. raft_open loads persistent state (replays raft.log, reads
 * raft.state) and starts as a follower. The transport callbacks (send,
 * reset_election, net) are installed by server_run() just before the event loop,
 * because server.c owns the sockets and timers they touch — keeping raft.c free
 * of any I/O dependency. */
int  raft_open(struct raft *r, int id, const char *data_dir, struct store *store);
void raft_close(struct raft *r);

/* Register a peer node before entering the event loop. Cluster size (and thus
 * the election/commit majority) is 1 + the number of peers added. */
void raft_add_peer(struct raft *r, int id, const char *host, int peer_port,
                   int client_port);

/* Timer-driven ticks, called by the event loop when the timerfds fire. */
void raft_on_election_timeout(struct raft *r); /* become candidate, start vote */
void raft_on_heartbeat(struct raft *r);        /* leader: broadcast AppendEntries*/

/* Feed a fully-deframed RPC payload (one message) from peer `from` into Raft.
 * server.c handles the length framing + CRC; raft.c handles the semantics. */
void raft_on_message(struct raft *r, int from, const uint8_t *payload, size_t len);

/* Client proposal (leader only). Encodes `c` into a log entry, appends+persists,
 * and kicks replication. Returns the assigned log index (>0) the caller can wait
 * on, 0 if this node is not the leader (caller should redirect to leader_id),
 * or -1 on a durability error. */
int64_t raft_client_propose(struct raft *r, const struct command *c);

/* True once `index` has been applied to the store (used by server.c to release a
 * pending client reply). */
bool raft_is_applied(struct raft *r, uint64_t index);

/* ===========================================================================
 * server.c — the epoll reactor, client line protocol, peer transport, timers.
 * ===========================================================================
 * server.c owns all file descriptors and the length framing of peer messages;
 * raft.c and store.c are pure logic it drives. This is where the C10k epoll
 * lesson (../../03-networking/04-c10k-http-server) meets consensus. */

int  server_run(struct raft *r, int client_port, int peer_port);

/* ===========================================================================
 * trace.h-style lightweight tracing (defined in server.c).
 * ===========================================================================
 * Structured, single-line, timestamped events to stderr, gated by the DB_TRACE
 * env var, so a running cluster narrates its own elections and commits. This is
 * the userspace half of "observability"; the kernel half (attaching kprobes to
 * these very syscalls) is the sibling project ../../01-kernel/06-kprobe-ftrace-tracer.
 * TR(category, "fmt", ...) is a macro; it compiles to nothing when tracing off. */
void trace_emit(const char *cat, const char *fmt, ...);
extern int g_trace_enabled;
#define TR(cat, ...) do { if (g_trace_enabled) trace_emit((cat), __VA_ARGS__); } while (0)

/* Small shared helpers (util in server.c): die on fatal error with errno text. */
void die(const char *msg);

#endif /* DB_H */
