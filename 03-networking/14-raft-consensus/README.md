# Raft consensus 🟥

**What it is.** A working teaching-core implementation of the **Raft consensus
algorithm** — the protocol that keeps a replicated state machine consistent while
machines crash, restart, and the network partitions. It implements the three
pillars of Raft end to end: **leader election** with randomized election timeouts
and the `RequestVote` RPC; **log replication** via `AppendEntries` with the log
matching check, `nextIndex`/`matchIndex`, and majority commit; and **safety**
(the election restriction and the current-term commit rule) that make committed
entries survive any leader change. On top of that it adds **crash-safe
persistence** with real `fsync` ordering, **snapshotting / log compaction** with
`InstallSnapshot`, a small **replicated key/value state machine**, and a **test
harness that injects network partitions and crashes** and checks that the cluster
does the right thing.

Nodes are **threads** (one per node) that talk over an **in-process simulated
network** whose partition matrix makes "split the cluster" a one-line call. That
is a deliberate choice: it isolates the *algorithm* from socket plumbing and
makes partition tests deterministic — the same way the Raft authors tested their
reference implementation. The transport is the only thing between this and a real
TCP cluster (see [Going further](#going-further)).

**Teaching-core honesty.** This faithfully implements the parts of Raft that
carry the *ideas*. It intentionally simplifies the parts that are engineering, not
insight: the persistent log is rewritten whole on each change (not an append-only
WAL), snapshots are sent in one bounded message (not chunked), the state machine
and commands are tiny fixed-size strings, and it does **not** implement cluster
**membership changes** (joint consensus) or **linearizable client sessions**
(dedup of retried commands, read leases). Exactly what is in and out is listed in
[Going further](#going-further).

## What you'll learn

- **Consensus from first principles.** Why a *majority* is the magic number (any
  two majorities of the same set overlap), and how that single fact powers both
  elections and commits.
- **Leader election** — terms as a logical clock, the follower/candidate/leader
  state machine, and why **randomized** election timeouts are what stop a cluster
  from livelocking on split votes.
- **The election restriction** (`log_up_to_date`) — the "is the candidate at
  least as up-to-date as me?" test on `(lastTerm, lastIndex)` that guarantees a
  new leader already holds every committed entry.
- **Log replication & the Log Matching Property** — how one `(prevLogIndex,
  prevLogTerm)` check certifies an entire log prefix, how conflicts are detected
  and repaired, and the fast-backup `conflictIndex` hint.
- **The commit-safety rule (Raft Figure 8)** — why a leader may only advance the
  commit index over an entry **from its own term**, even if an older entry sits on
  a majority.
- **Crash-safe persistence & `fsync` ordering** — the `write → fsync(file) →
  rename → fsync(dir)` durable-replace dance, *why each fsync closes a specific
  crash window*, and the write-ahead rule "snapshot durable **before** the state
  that references it." Plus a **CRC-32** integrity check on every file.
- **Snapshotting / log compaction** — bounding an unbounded log, and shipping a
  snapshot to a follower whose entries were already compacted (`InstallSnapshot`).
- **Concurrency done carefully** — one mutex + condvar per node, a strict
  "hold at most one node lock; send after unlocking" discipline that makes the
  cluster provably deadlock-free, and a `CLOCK_MONOTONIC` condvar so timeouts are
  immune to wall-clock jumps.

Syscalls / APIs exercised: `open`, `write`, `read`, **`fsync`** (on files *and* a
directory), `rename`, `mkdir`, `clock_gettime(CLOCK_MONOTONIC)`, `nanosleep`,
`pthread_create/join`, `pthread_mutex_*`, `pthread_cond_timedwait` +
`pthread_condattr_setclock`, `opendir`/`readdir`.

## Build & run (Linux / WSL2)

The node threads use pthreads and the persistence layer fsyncs both files and the
containing directory, so this needs **Linux or WSL2**. **No root or capabilities
are required.**

```bash
make                 # builds ./raft with -Wall -Wextra -pthread
make run             # runs the harness against a fresh 5-node cluster
```

`make run` prints a narrated pass/fail transcript of five scenarios:

```
=== 1. Leader election ===
    [PASS] a leader was elected from a cold start
=== 2. Log replication ===
    [PASS] all replicas converged on color=blue
=== 3. Partition safety (isolate the leader) ===
    [PASS] majority elected a new leader
    [PASS] isolated node could NOT commit its write
=== 3b. Heal partition (logs reconcile, stale write overwritten) ===
    [PASS] old leader's uncommitted 'red' was discarded
=== 4. Crash recovery (kill a follower, commit, restart it) ===
    [PASS] restarted node caught up to the committed log
=== 5. Snapshot & log compaction ===
    [PASS] leader compacted its log into a snapshot
    [PASS] node recovered from snapshot and reconverged
```

Because Raft is asynchronous with randomized timeouts, the harness *waits* for
conditions with generous bounds rather than assuming instant results; a missed
bound on a heavily loaded machine is a scheduling artifact, not a Raft bug. The
per-node on-disk state lives under `./raftdata/node<i>/` (`state`, `snapshot`);
`make clean` removes it. You can `xxd raftdata/node0/state` to see the
little-endian on-disk format and the trailing CRC.

## How it works

Five C files plus one header:

- **`raft.h`** — the whole data model in one place: `struct log_entry` and the
  snapshot-aware `struct raft_log`; the five RPC message shapes
  (`RequestVote`/`AppendEntries`/`InstallSnapshot` + replies) inside a tagged
  `struct message`; the per-node state split into *persistent* (`currentTerm`,
  `votedFor`, `log`) vs *volatile* (`commitIndex`, `nextIndex[]`, `matchIndex[]`);
  and the `struct cluster` holding the `reachable[][]` partition matrix. Read this
  first — every `.c` file just moves nodes between the states named here.

- **`raft.c`** — the algorithm. Grouped by sub-problem:
  - *Election*: `become_candidate` (bump term, self-vote, **persist**, then
    `RequestVote` all peers), `handle_request_vote` (the `votedFor` + up-to-date
    gate, **persist before replying**), `handle_request_vote_reply` (tally votes,
    `become_leader` on a majority), and `tick` (randomized election timeout).
  - *Replication*: `raft_submit` (leader appends + persists a client command),
    `replicate_to_peer` (build `AppendEntries`, or `InstallSnapshot` if the peer
    needs compacted entries), `handle_append_entries` (term check → log-matching
    on `prevLog*` → conflict-truncate-and-append → advance commit → **persist
    before ack**), `handle_append_entries_reply` (update `matchIndex`/`nextIndex`,
    fast-backup on rejection), and `advance_commit_index` (the majority + current-
    term commit rule).
  - *State machine & compaction*: `apply_committed` (feed committed entries to the
    KV store, in order, on every node), `maybe_snapshot` (compact the log once it
    grows), and the `InstallSnapshot` handlers.
  - *Plumbing*: `node_main` (the per-node loop: drain inbox → timers → apply →
    snapshot → flush outgoing **after unlocking**), plus the public API
    (`cluster_*`, `raft_submit`, `raft_crash_node`/`raft_restart_node`).
  - The single most important comment block is the **locking discipline** at the
    top: a thread holds at most one node lock and only sends after unlocking, so
    the cluster cannot deadlock.

- **`persist.c`** — durable state. `persist_save_state` / `persist_save_snapshot`
  encode everything **little-endian** with a trailing **CRC-32**, then hand it to
  `durable_write`, which performs the crash-safe replace: write temp →
  `fsync(temp)` → `rename` → `fsync(dir)`. Every `fsync`'s comment names the exact
  crash window it closes. `persist_load` verifies the CRC and reconstructs a node
  purely from disk — the path a restart takes.

- **`net.c`** — the simulated network. `net_send` consults `reachable[from][to]`
  (and an optional random-loss knob) and either enqueues the message to the
  destination's inbox or drops it; `net_partition`/`net_heal` flip the matrix.
  This is where you'd marshal onto a TCP socket instead.

- **`kv.c`** — the replicated state machine: a bounded key/value store applying
  `SET`/`DEL` commands, with `kv_serialize`/`kv_deserialize` for snapshots.
  Deliberately a *pure function of its command sequence* (no clocks, no
  randomness), which is what lets every replica converge.

- **`main.c`** — the harness described under *Build & run*, including a small
  `rm_rf` (opendir/readdir/unlink/rmdir) so each run starts clean.

The data flow of one committed write: `raft_submit` appends to the leader's log
and fsyncs it → `AppendEntries` replicates it → each follower fsyncs and acks →
the leader sees a majority `matchIndex` and advances `commitIndex` (if the entry
is from its term) → every node's `apply_committed` feeds it to the KV store.

## Assembly notes

`asm/demo.c` is a **self-contained** extraction (no system headers, own types) of
Raft's two safety-critical *pure-logic* routines, because `raft.c` pulls in
pthreads/POSIX headers and cannot be lowered to standalone asm. It compiles to
genuine SysV assembly with the exact commands in the brief (see the `asm` target):

- **`log_up_to_date`** — the election restriction. `asm/demo.annotated.s`
  (from `-O1`) shows clang compile the lexicographic `(term, index)` compare into
  a **fully branchless** sequence: it computes *both* candidate answers
  (`setae` for `index >=`, `seta` for `term >`) and selects between them with a
  single `cmove` on term-equality — no conditional jump to mispredict.
- **`majority_match_index`** — the commit-index computation (sort `matchIndex[]`,
  return the median). The annotation highlights a **non-leaf** function: because
  it calls `memcpy`, it cannot use the red zone, builds a real 128-byte frame for
  its `tmp[16]`, and keeps its live values (`n`, the clamped count) in
  **callee-saved** `rbx`/`r14` across the call. It also shows clang computing
  `n - (n/2 + 1)` with a `shr`/`not`/`add` instead of a divide-and-subtract, and
  an insertion sort as a clean nested loop.
- **`asm/demo.O0.s`** is the naive mapping (every variable spilled to the stack),
  easiest to read one C statement at a time; **`asm/demo.O2.s`** is the optimizer
  off the leash. Regenerate all three with `make asm` (clang cross-targets Linux
  from any host); `asm/demo.annotated.s` is hand-written from `demo.s`.

## Going further

The list's **`Stretch:`** goal and what production Raft (etcd, TiKV, LogCabin,
Hashicorp Raft) adds beyond this teaching core:

- **Real TCP/UDP transport.** Swap `net.c` for a socket layer: length-prefix each
  `struct message`, run a reader thread per connection that unmarshals into the
  inbox, and reconnect on failure. The algorithm in `raft.c` does not change.
- **Cluster membership changes.** Add/remove servers safely via **joint
  consensus** (`C_old,new`) or the single-server-at-a-time method — the one major
  Raft feature omitted here.
- **Linearizable client semantics.** Client session IDs + sequence numbers to make
  retried commands idempotent, a leader **no-op entry on election** to commit the
  tail immediately, and **read leases / ReadIndex** so reads don't have to go
  through the log.
- **An append-only log + incremental fsync.** Replace the whole-file rewrite in
  `persist.c` with a segmented WAL (append records, fsync a batch, recover by
  replay), which is how production keeps per-write cost O(1) instead of O(log).
- **Chunked snapshots & snapshot streaming**, plus **pre-vote** and
  **leadership transfer** to avoid disruptive elections, and **batching /
  pipelining** many client commands per `AppendEntries`.

## References

- Ongaro & Ousterhout, ***In Search of an Understandable Consensus Algorithm*
  (Raft)**, USENIX ATC 2014 — the paper; §5 (election/replication/safety) and §7
  (snapshotting) map directly onto this code. Diego Ongaro's PhD thesis is the
  extended version with membership changes.
- **`man 2 fsync`**, **`man 2 rename`**, **`man 2 open`** — the durability
  primitives; see also LWN's "*Ensuring data reaches disk*" for the
  fsync-the-directory subtlety.
- **etcd `raft`** (`go.etcd.io/raft`) and **LogCabin** (Ongaro's reference C++
  implementation) — production Raft to read once the ideas here click; **TiKV**'s
  `raft-rs` is the Rust equivalent.
- **MIT 6.824** labs 2A–2D — the canonical exercise of building exactly this
  (election, replication, persistence, snapshots) against a simulated network.
- `man 7 pthreads`, `man 3 pthread_cond_timedwait`,
  `man 3 pthread_condattr_setclock` — the concurrency primitives used here.
