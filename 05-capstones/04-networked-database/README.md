# A networked database 🟥 🧩

**What it is.** A distributed, replicated key/value database — assembled from the
pieces this lab builds elsewhere — that you can actually run: a **single-binary
node** with a durable in-process KV store (write-ahead log + `fsync` + crash
recovery), served over TCP through a non-blocking **epoll** event loop speaking a
small line protocol, and replicated across a **3-node localhost cluster with
Raft** (leader election + log replication) so the cluster stays consistent
through crashes and network partitions. It ships with an **injected-fault
(partition) demo** that isolates the leader, watches Raft elect a new one, keeps
serving writes, and reconciles on heal.

This is a **teaching core (🧩)**, and it is honest about that: it runs the real
integration end to end at small scale — real syscalls, real consensus, real
durable logs — but each subsystem is the *smallest correct version*, and the
[Scope](#scope-what-runs-vs-what-a-real-system-needs) section says exactly where
the simplifications are. The full-scale version of each piece lives in a sibling
project, linked from the [Architecture](#architecture) table below.

> **Platform: Linux (or WSL2).** The node uses `epoll`, `timerfd`, `accept4`, and
> `fsync`, which are Linux-only; it will not build on Windows/macOS. The
> **assembly deliverable is host-portable** — `make asm` cross-targets Linux with
> clang and runs anywhere.

---

## Architecture

One process is one node. A single epoll thread (no locks) multiplexes clients,
peers, and two timers. Writes flow client → Raft (replicate) → storage engine
(apply). Two durable logs — the Raft replication log and the storage-engine WAL —
each do a distinct job (see [Two logs](#why-two-logs)).

```
                          ┌───────────────────────────────────────────────┐
   client (TCP,           │                 ONE  db  NODE                  │
   line protocol)         │                                               │
      PUT/GET/DEL  ─────►  │  ┌─────────────┐   epoll event loop (1 thread)│
      +OK / $val / -MOVED  │  │  server.c   │   ── client fds               │
                          │  │  reactor +  │   ── peer fds  (Raft RPC)      │
                          │  │  framing +  │   ── election timerfd          │
                          │  │  protocol   │   ── heartbeat timerfd         │
                          │  └──────┬──────┘                                │
                          │         │ propose(cmd)          apply(cmd)      │
                          │         ▼                          ▲           │
                          │  ┌─────────────┐            ┌──────┴───────┐    │
                          │  │   raft.c    │  commit    │   store.c    │    │
                          │  │ election +  │ ─────────► │  KV hashtable│    │
                          │  │ replication │            │  (state m/c) │    │
                          │  └──────┬──────┘            └──────┬───────┘    │
                          │         │ append+fsync            │ append+fsync│
                          │         ▼                          ▼           │
                          │   raft.log  raft.state        store.wal        │
                          │  (replication log)          (storage WAL)      │
                          └─────────┬─────────────────────────────────────┘
                                    │  AppendEntries / RequestVote
                                    │  (length-framed + CRC32, over TCP)
                    ┌───────────────┼───────────────┐
                    ▼               ▼               ▼
               ┌─────────┐     ┌─────────┐     ┌─────────┐
               │ node 2  │◄───►│ node 1  │◄───►│ node 3  │      majority = 2 of 3
               └─────────┘     └─────────┘     └─────────┘
```

**Each subsystem is a full project elsewhere in this lab.** The table maps what
this capstone distills to where the complete, test-harnessed version lives:

| Subsystem (here) | What this core does | Full project (sibling) |
|---|---|---|
| **Storage engine** — KV + WAL + recovery | Hash table + append-only WAL, CRC32 per record, `fsync`, replay-on-boot recovery (`src/store.c`, `src/wal.c`) | [`../../02-systems-tools/13-embedded-db`](../../02-systems-tools/13-embedded-db) — B-tree/LSM, page cache, real crash recovery |
| **Event-loop server** — epoll reactor | Level-triggered `epoll`, non-blocking `accept4`, per-connection framing, keep-alive (`src/server.c`) | [`../../03-networking/04-c10k-http-server`](../../03-networking/04-c10k-http-server) — edge-triggered, `sendfile`, `SO_REUSEPORT`, C10k |
| **Consensus** — Raft | Leader election, log replication, commit rule, log-matching (`src/raft.c`) | [`../../03-networking/14-raft-consensus`](../../03-networking/14-raft-consensus) — snapshots, membership changes, a fuzz harness |
| **Protocol** — request framing | A tiny RESP-like line protocol + `-MOVED` redirects | [`../../03-networking/07-redis-clone`](../../03-networking/07-redis-clone) — full RESP, pipelining, pub/sub |
| **Observability** — tracing | Structured stderr event trace (`DB_TRACE=1`) narrating elections/commits | [`../../01-kernel/06-kprobe-ftrace-tracer`](../../01-kernel/06-kprobe-ftrace-tracer) — kprobe/ftrace on the very syscalls below |

The last row is the key honesty point: userspace tracing (this core) tells you
*what the node decided*; the kernel tracer (sibling) attaches **kprobes/ftrace**
to the `fsync`/`sendto`/`epoll_wait` syscalls this node makes to tell you *what
the kernel did underneath* — the two halves of real observability.

### Why two logs?

Real replicated stores keep **two** durable logs, and so does this one — it is a
feature, not redundancy:

- **`raft.log` — the replication log.** Raft's safety proof requires each node to
  remember its log across crashes (Figure 2). This is the analogue of etcd's
  `wal/`. It answers "what has the cluster agreed to?"
- **`store.wal` — the storage-engine WAL.** Once an entry is *committed*, we apply
  it by durably writing it to the state machine's own log and mutating the hash
  table. Replaying `store.wal` on boot rebuilds the KV. It answers "what is the
  database's current durable state?"

A production system (etcd + bbolt, TiKV + RocksDB) separates them for exactly the
same reasons; it additionally **snapshots** so neither log grows forever. We never
snapshot — a documented omission below.

---

## What you'll learn

- **Write-ahead logging & crash recovery** — the ordering contract (log record →
  `fsync` → mutate memory), why doing it in that order survives power loss, and
  how a **per-record CRC32** lets recovery detect the *torn tail* and truncate to
  the last good record. (`src/wal.c`, `src/crc32.c`)
- **`fsync` semantics** — why `write()` alone is not durable (page cache) and what
  `fsync` actually blocks on; why databases batch writes into one `fsync`.
- **The epoll reactor** — one thread, no locks, everything-is-an-fd: client
  sockets, peer sockets, and **`timerfd`** timers all waited on together;
  edge- vs level-triggered readiness (`src/server.c`).
- **Length framing over TCP** — TCP is a byte stream with no message boundaries,
  so every peer message is `[u32 len][u32 crc][payload]`; the reader reassembles
  partial reads into whole messages. This is the subject of the annotated asm.
- **Raft** — randomized election timeouts (and why they must be ≫ the heartbeat),
  the RequestVote "up-to-date" rule that preserves Leader Completeness, the
  AppendEntries **log-matching** consistency check, and the current-term **commit
  rule** (`src/raft.c`).
- **Distributed-systems failure modes** — a partitioned leader that cannot commit,
  split votes, and log reconciliation on heal — demonstrated live by the
  partition demo.
- **Non-blocking `connect`** — dialing peers with `EINPROGRESS` + `EPOLLOUT`
  completion, and treating a dropped RPC as normal (Raft retries).

---

## Build & run

**Platform: Linux / WSL2.** Requires `clang` (or `gcc`) and, for the client
helper, `nc` (or a bash with `/dev/tcp`).

```bash
make                       # clang -Wall -Wextra -O2 ... src/*.c -o dbnode
```

### Single node (still exercises WAL + fsync + recovery + the full Raft path)

A one-node cluster elects itself instantly and commits with no network hop, so
this is the smallest thing that runs — and it runs the real code:

```bash
make run-single            # DB_TRACE=1 ./dbnode 1 ./data/n1 7001 8001
# in another terminal:
./scripts/client.sh 7001 PUT greeting "hello durable world"
./scripts/client.sh 7001 GET greeting          # -> $18 \r\n hello durable world
./scripts/client.sh 7001 STATUS
# kill it, restart it, GET again: the value survives (store.wal was replayed).
```

### 3-node cluster on localhost

```bash
make run-cluster           # starts n1/n2/n3, each in ./data/nN, logs to node.log
sleep 1
./scripts/client.sh 7001 STATUS                # find the leader (leader=<id>)
./scripts/client.sh 7001 PUT k1 v1             # a follower answers -MOVED host:port
tail -f data/n1/node.log                       # watch the election + replication trace
make stop-cluster
```

### The injected-fault (partition) demo — the payoff

```bash
make partition-demo        # scripts/partition-demo.sh
```

It: (1) forms a cluster and writes a value; (2) **partitions the leader** from the
other two using the in-process `ADMIN partition` switch; (3) waits for the
majority side to **elect a new leader**; (4) shows the isolated old leader
**cannot commit** while the new leader serves writes; (5) **heals** the partition
and shows the old leader step down and its log reconcile. Watch it narrate:

```bash
tail -n 40 data/n*/node.log
```

### Client protocol (one CRLF-terminated line per request)

| Request | Reply |
|---|---|
| `PING` | `+PONG` |
| `PUT <key> <value>` | `+OK` (after commit+apply) / `-MOVED host:port` / `-ERR ...` |
| `GET <key>` | `$<len>`⏎`<bytes>` (hit) · `$-1` (miss) · `-MOVED host:port` (on a follower) |
| `DEL <key>` | `+OK` / `-MOVED host:port` |
| `STATUS` | `+role=… term=… leader=… commit=… applied=… log=…` |
| `ADMIN partition <peerid> on\|off` | `+OK` — inject/heal a simulated partition |

Inspect the machinery while it runs:

```bash
strace -f -e trace=epoll_wait,epoll_ctl,accept4,fsync,write,connect ./dbnode 1 ./data/n1 7001 8001
ss -tan | grep -E ':(7001|8001)'      # client + peer sockets and their states
xxd data/n1/store.wal | head          # the framed, CRC'd WAL records on disk
```

---

## How it works

Read the source in this order.

- **`src/db.h`** — the map of the whole node: every type, on-disk/on-wire format,
  constant, and inter-module prototype, plus the architecture lecture in prose.
  Start here.
- **`src/crc32.c`** — CRC-32/IEEE, table-driven. Why every durable record is
  checksummed: to detect the torn tail after a crash and stop there.
- **`src/wal.c`** — the record format (`[reclen][crc][command body]`), the shared
  command (de)serializer used by *both* logs and the wire, and `wal_replay()` —
  the recovery algorithm that replays intact records and truncates a torn tail.
- **`src/store.c`** — the state machine: an FNV-1a separate-chaining hash table
  behind the WAL. `store_apply()` is the durable write path (log-ahead → `fsync`
  → mutate); `store_open()` rebuilds the table by replaying `store.wal`.
- **`src/raft.c`** — consensus. Persistent state (`currentTerm`, `votedFor`,
  `log[]`) with its own crash-safe log; the RequestVote/AppendEntries handlers;
  role transitions; and the commit rule. Pure logic — it never touches a socket
  or timer, only the `send()` / `reset_election()` callbacks server.c installs.
- **`src/server.c`** — the epoll reactor. Owns every fd, frames peer messages
  (length + CRC), runs the client line protocol, drives the election/heartbeat
  timerfds, dials peers, and matches an async commit back to the waiting client
  via a small pending-reply table. Also defines the `DB_TRACE` tracer.
- **`src/main.c`** — parse config, open the store and Raft, register peers, run.

### The write path, precisely

1. A client sends `PUT k v`. If this node is not the leader, `server.c` replies
   `-MOVED <leader-host:port>` (Redis-cluster style) and the client retries there.
2. On the leader, `raft_client_propose()` appends the command as a log entry,
   **`fsync`s `raft.log`**, and replicates via AppendEntries. The client is *not*
   answered yet — its `fd`+index are parked in the pending table.
3. When a **majority** of nodes have persisted the entry, the leader advances
   `commitIndex` and calls `apply_committed()`, which hands the command to
   `store_apply()` — appending to **`store.wal`**, `fsync`ing, and mutating the
   hash table.
4. `flush_pending()` sees the index is applied and finally replies `+OK`.

### Injecting a partition without root

`ADMIN partition <peerid> on` flips a per-peer `blocked` flag; `server_send` drops
outbound frames to a blocked peer and `peer_in_process` drops inbound frames from
one. The TCP connection stays up but the application silently drops — the effect
on Raft (no messages exchanged) is identical to a real partition. A production
test would use `iptables -j DROP` or `tc netem`; this needs no privileges and is
scriptable, which is why the demo uses it.

---

## Scope: what runs vs. what a real system needs

**This teaching core implements, and runs:**

- A durable KV: WAL + `fsync` + CRC-checked crash recovery (torn-tail truncation).
- A non-blocking epoll server, a line protocol, keep-alive, leader redirects.
- Raft leader election (randomized timeouts, split-vote resolution), log
  replication with the log-matching check and current-term commit rule, applied
  to the KV — forming a real 3-node cluster that stays consistent.
- A live partition/heal demo proving no acknowledged write is lost.

**It deliberately omits (each is additive, and called out in the code):**

- **Snapshotting / log compaction.** Both `raft.log` and `store.wal` grow forever;
  a real system snapshots the state machine and truncates the log prefix.
- **A real storage structure.** The index is a hash table (no range scans, no
  ordered iteration, no page cache); the B-tree/LSM lives in
  [`../../02-systems-tools/13-embedded-db`](../../02-systems-tools/13-embedded-db).
- **Linearizable reads.** `GET` is served on the leader from applied state without
  a read-index or leader lease, so a stale read is possible during a leadership
  change. Writes *are* linearizable (they go through the log).
- **Cluster membership changes** (Raft §6 joint consensus) — the cluster is fixed
  at startup.
- **Backpressure / partial-write buffering on peer links.** A peer send that would
  block is dropped (Raft retries) or resets the link; there is no outbound queue.
- **Edge-triggered epoll, TLS, auth, and per-connection idle timeouts.** The
  edge-triggered event-loop lesson is
  [`../../03-networking/04-c10k-http-server`](../../03-networking/04-c10k-http-server).
- **The kernel-side tracer.** Observability here is userspace stderr events; the
  kprobe/ftrace half is [`../../01-kernel/06-kprobe-ftrace-tracer`](../../01-kernel/06-kprobe-ftrace-tracer).

None of these change the integration lessons; they are the difference between a
core you can read in an afternoon and a system you run in production.

---

## Assembly notes

The annotated assembly is [`asm/demo.annotated.s`](asm/demo.annotated.s), built
from the node's most representative pure-logic routine: **turning a KV mutation
into a framed, checksummed WAL record** — `wal_frame_record` plus a table-free
`crc32_ieee` — extracted self-contained into [`asm/demo.c`](asm/demo.c) (its own
types, no system headers). What the annotation highlights:

- A **CRC is polynomial long division**: the inner loop's "shift out a bit; if it
  was 1, xor the reflected polynomial `0xEDB88320`" is rendered **branchlessly**
  by the optimizer with a negate-to-mask idiom (`and`/`neg`/`and`/`xor`), not a
  conditional jump — visible instruction-for-instruction.
- **`put_u32` writes little-endian one byte at a time for portability**, and the
  file shows the optimizer sometimes keeping the four byte-stores (the misaligned
  `klen`) and sometimes **fusing them into a single `movl`** (`vlen`, `reclen`) —
  legal only because x86 is little-endian with unaligned stores. The C never
  assumed the host's byte order; that is the whole point of spelling out the bytes.
- Record framing is **register-cheap** — length + checksum + byte copies, no heap,
  no stack locals — which is why every write can afford to be logged this way.

Regenerate the raw assembly on any host (clang cross-targets Linux):

```bash
make asm
```

The committed `.s` files are genuine `clang --target=x86_64-pc-linux-gnu -S`
output at `-O0` (naive), `-O1` (the annotated baseline), and `-O2` (the byte-copy
loops vectorize). `asm/demo.annotated.s` is hand-written from the `-O1` output.

---

## Going further

- **Stretch — snapshots + log compaction.** Add a state-machine snapshot
  (serialize the hash table, record `lastIncludedIndex/Term`), truncate the log
  prefix, and add the `InstallSnapshot` RPC so a far-behind or newly-added follower
  catches up from a snapshot instead of the whole log.
- **Stretch — linearizable reads.** Implement Raft's **read-index** (or a leader
  lease) so `GET` on the leader is guaranteed to see all prior committed writes.
- **Stretch — membership changes.** Single-server add/remove (Raft §6), then joint
  consensus, so the cluster can grow/shrink without downtime.
- **What production does.** Group commit (batch many writes per `fsync`); a real
  storage engine (B-tree/LSM) with a page cache; `io_uring` instead of epoll for
  fully-async disk+net ([`../../03-networking/05-io-uring-server`](../../03-networking/05-io-uring-server));
  pipelined RESP; TLS + auth; sharding/partitioning across many Raft groups
  (à la TiKV/CockroachDB); and end-to-end tracing tying the userspace events here
  to kernel `fsync`/`sendto` latency via the kprobe tracer.

---

## References

- **Raft** — Diego Ongaro & John Ousterhout, *"In Search of an Understandable
  Consensus Algorithm"* (2014); the extended version's **Figure 2** is the spec
  this file implements. The Raft website's visualization is the best intuition
  primer.
- **Write-ahead logging** — the ARIES paper (Mohan et al.); PostgreSQL's
  `src/backend/access/transam/xlog.c`; SQLite's WAL-mode docs.
- **CRC** — `man 3 crc32` (zlib); the CRC-32/IEEE polynomial `0xEDB88320`
  (reflected `0x04C11DB7`).
- `man 7 epoll`, `man 2 epoll_ctl`, `man 2 timerfd_create`, `man 2 accept4`,
  `man 2 fsync`, `man 2 connect` (the `EINPROGRESS` non-blocking dance).
- **Read the source of the real thing:** **etcd** (`raft/`, and `wal/` for the
  replication log), **RocksDB**/**bbolt** (the state store), **TiKV**
  (Raft-over-a-KV done for real), and **redis** (the event loop + RESP).
- Sibling projects that build each half at full size:
  [embedded-db](../../02-systems-tools/13-embedded-db),
  [c10k-http-server](../../03-networking/04-c10k-http-server),
  [raft-consensus](../../03-networking/14-raft-consensus),
  [kprobe-ftrace-tracer](../../01-kernel/06-kprobe-ftrace-tracer).
