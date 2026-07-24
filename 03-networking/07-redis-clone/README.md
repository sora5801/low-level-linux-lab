# A Redis clone 🟥

**What it is.** A working, single-threaded, epoll-driven key/value server that
speaks enough of the **RESP** protocol to talk to real `redis-cli`. It
implements the pieces that make Redis *Redis*: a non-blocking event loop, a hash
table with **incremental rehashing** (two tables live during a resize), key
expiration (lazy + active), **RDB** snapshots taken with `fork(2)` copy-on-write,
an **AOF** command log with a configurable `fsync` policy, and **pub/sub**. It is
a **teaching core**, not a drop-in Redis — see *Scope & honesty* below for the
exact line between what is real and what is omitted.

Difficulty: 🟥 (giant). Platform: **Linux / WSL2** (epoll, accept4, fork CoW).

---

## What you'll learn

- **The reactor pattern**: one thread, `epoll_wait`, non-blocking sockets, and a
  strict "never block the loop" discipline — which is *why* the data structures
  need no locks. (`server.c`)
- **Stream parsing under partial reads**: RESP inline and multibulk framing,
  resumable across `read()` boundaries because TCP delivers bytes, not messages.
  (`resp.c`)
- **A real hash table**: separate chaining, power-of-two masking, and
  **incremental rehashing** — the two-table dance that keeps a resize from
  stalling the server. (`dict.c`)
- **Expiration** done two ways: *lazy* (checked on access) and *active* (a
  sampled background sweep). (`db.c`)
- **`fork()` copy-on-write snapshots**: how a child serializes a frozen view of
  the heap while the parent keeps serving, and why you `_exit` in the child.
  (`persist.c`)
- **Durability trade-offs**: the AOF redo log and what `always` / `everysec` /
  `no` `fsync` policies actually cost. (`persist.c`)
- **Syscalls in anger**: `socket/bind/listen/accept4`, `epoll_create1/ctl/wait`,
  `read/write` with `EAGAIN`/`EINTR`/partial-I/O handling, `fork/waitpid`,
  `fsync`, `rename` for atomic file replacement.

---

## Build & run (Linux / WSL2)

```bash
make                     # builds ./redis-clone  (-Wall -Wextra, clang)
./redis-clone 6379       # listen on :6379

# from another terminal (a real client works):
redis-cli -p 6379 ping                 # PONG
redis-cli -p 6379 set foo bar          # OK
redis-cli -p 6379 get foo              # "bar"
redis-cli -p 6379 set s 1 ex 10        # value with a 10s TTL
redis-cli -p 6379 ttl s                # (integer) 10
redis-cli -p 6379 incr counter         # (integer) 1
redis-cli -p 6379 lpush mylist a b c   # (integer) 3
redis-cli -p 6379 lrange mylist 0 -1   # 1) "c" 2) "b" 3) "a"
redis-cli -p 6379 hset h f1 v1 f2 v2   # (integer) 2
redis-cli -p 6379 hgetall h            # f1 v1 f2 v2

# pub/sub (two terminals):
redis-cli -p 6379 subscribe news       # blocks, waiting for messages
redis-cli -p 6379 publish news hello   # (integer) 1  -> the subscriber prints it
```

Enable persistence:

```bash
./redis-clone 6379 --appendonly yes --appendfsync everysec   # AOF log
redis-cli -p 6379 bgsave                                      # fork() an RDB snapshot
```

Optional, dependency-free smoke test that drives the real event loop over a raw
socket (needs `bash`):

```bash
make test
```

`make asm` regenerates the teaching assembly on **any** host (clang cross-targets
Linux). `make clean` removes objects and the binary.

---

## How it works (a tour of the code)

The server is layered bottom-up; each file is self-contained and heavily
commented.

| File | Responsibility |
|------|----------------|
| `zmalloc.[ch]` | Fail-fast allocation wrappers; the one place OOM policy lives. |
| `sds.[ch]` | **Simple Dynamic Strings**: binary-safe, length-prefixed, header-behind-the-pointer strings. |
| `dict.[ch]` | The **incremental-rehashing hash table** — the centerpiece. |
| `object.c` | Value objects (`robj`: string/list/hash), the doubly linked list, and the `dictType` vtables that set per-table ownership. |
| `db.c` | The keyspace: `lookupKey*`, `setKey`, `dbDelete`, and **lazy + active expiration**. |
| `resp.c` | RESP parsing (inline + multibulk, resumable) and the `addReply*` reply builders; the strict integer parser `string2ll`. |
| `commands.c` | The command table and every handler (GET/SET/INCR, DEL/EXPIRE/TTL, LPUSH/LRANGE, HSET/HGET, …). |
| `pubsub.c` | SUBSCRIBE/UNSUBSCRIBE/PUBLISH and the channel↔subscriber maps. |
| `persist.c` | **RDB** snapshot (sync + `fork` CoW), **AOF** append/replay, and a CRC-64 integrity check. |
| `server.c` | `main`, the **epoll event loop**, networking, `serverCron`, and command dispatch. |

**The event loop** (`server.c`) is the spine. `epoll_wait` returns ready fds; the
listen fd (registered with a `NULL` cookie) accepts new clients, a readable
client is drained into `querybuf` and parsed, and a writable client resumes a
blocked reply flush. Every ~100 ms `serverCron` runs housekeeping: sample-expire
keys, push any in-flight rehash forward, reap a finished `BGSAVE` child, and
flush/`fsync` the AOF. Because command execution is single-threaded, nothing in
`dict`/`db`/the buffers needs a lock.

**Incremental rehashing** (`dict.c`) is the idea most worth studying. When the
load factor hits 1.0 the table allocates a second, larger bucket array (`ht[1]`)
and sets `rehashidx = 0` instead of moving everything at once. Each subsequent
operation migrates one bucket (`_dictRehashStep`), and `serverCron` nudges it
along when idle. During the migration, lookups and deletes check **both** tables
while inserts go only into `ht[1]` (so `ht[0]` can only shrink, guaranteeing the
resize terminates). When `ht[0]` empties, `ht[1]` is promoted and `rehashidx`
returns to `-1`. No single request ever pays the full O(n) resize cost.

**RDB via fork** (`persist.c`): `rdbSaveBackground` calls `fork()`. The child
inherits a **copy-on-write** image of the heap, so it serializes the dataset as
it existed at the fork instant while the parent keeps mutating; only pages the
parent writes get physically copied. The child `_exit`s (never `exit`) so it does
not flush the parent's stdio or run its atexit handlers. The snapshot is written
to a temp file, `fsync`'d, and `rename`'d into place so a crash can never leave a
half-written file at the real path.

---

## Assembly notes

`asm/demo.c` extracts the two purest-logic routines of the dict: the
**MurmurHash2-64A** hash (`dict_hash`) and the **incremental-rehash index step**
(`rehash_target_index`). It is freestanding (no headers, own types) so clang
emits clean teaching assembly. `asm/demo.annotated.s` is hand-written from the
committed `asm/demo.s` (`-O1`) with a comment on essentially every instruction.
Highlights the annotation calls out:

- The 64-bit Murmur multiplier is too wide for an immediate, so it is loaded once
  with `movabsq` and reused from `%rax` across every `imulq` — the hash is
  literally a chain of multiplies interleaved with xor-shifts.
- The hand-rolled little-endian byte assembly (`p[0] | p[1]<<8 | …`) was **fused
  by the optimizer into a single `movq` load**, because that byte order *is* the
  native x86-64 load. Endianness is why the fusion is correct.
- `switch (len & 7)` compiled with `-fno-jump-tables` becomes a **cmp/jcc binary
  search** that jumps into a fallthrough chain, one byte per case.
- `rehash_target_index` has **no branch**: the `?:` is a `cmovns`, and the
  power-of-two modulo is a single `andq`.

Compare `asm/demo.O0.s` (every value spilled to the stack, statement by
statement) with `asm/demo.s` (`-O1`) and `asm/demo.O2.s` (even more fused) to
watch the optimizer work. Regenerate all three with `make asm`.

---

## Scope & honesty (what this teaching core covers and omits)

**Implemented and real:** RESP2 inline + multibulk parsing (resumable);
epoll event loop with non-blocking I/O and back-pressure via `EPOLLOUT`;
incremental-rehash dict; string/list/hash types; GET, SET (with `EX`/`PX`), INCR,
DECR, INCRBY, DEL, EXISTS, EXPIRE, TTL, TYPE, DBSIZE, FLUSHDB, LPUSH, RPUSH,
LPOP, RPOP, LLEN, LRANGE, HSET, HGET, HDEL, HLEN, HGETALL, SUBSCRIBE,
UNSUBSCRIBE, PUBLISH, PING, ECHO, SAVE, BGSAVE, plus stubs (COMMAND, CONFIG) so
`redis-cli` connects cleanly; lazy + active expiration; RDB fork/CoW snapshots
with atomic rename and a CRC-64 footer; AOF append + replay with `always` /
`everysec` / `no` fsync.

**Deliberately omitted** (and where the honest gaps are):
- **RESP3, and inline quoting/escapes** — inline mode does a plain whitespace
  split (fine for hand typing; real clients use multibulk anyway).
- **Multiple databases, SELECT, clustering, replication.** One DB (db0).
- **No shared/refcounted objects or integer-object caching** — each key owns its
  value outright, which keeps ownership crisp at the cost of some memory.
- **The RDB file format is custom**, not byte-compatible with Redis (though it is
  real: length-prefixed, typed, CRC-checked). No AOF rewrite/compaction.
- **`expires` duplicates keys** instead of sharing the key object with the main
  dict (Redis shares them to save memory); we chose the safe, simple ownership.
- **Subscribe-mode command restrictions** are not enforced (Redis rejects most
  commands while a connection is subscribed); we allow them and note it here.
- No `maxmemory`/eviction, no `WAIT`, no keyspace notifications, no scripting.

None of the omissions change the lessons above; they trim breadth, not depth.

---

## Going further

- **Stretch:** add RESP3, a `SCAN` cursor over the two-table dict, and AOF
  rewrite (fork a child that emits a minimal command set rebuilding the current
  state — the same CoW trick as RDB).
- **What production does:** Redis uses **SipHash** (not Murmur) for DoS
  resistance, listpack/quicklist/intset/ziplist encodings that pack small
  collections into flat memory, an event library (`ae`) that abstracts
  epoll/kqueue/evport, an fsync-on-a-background-thread for AOF `everysec`, and
  incremental everything (rehash, expire, defrag) to hold tail latency down.

## References

- Redis source: `src/dict.c` (incremental rehashing), `src/networking.c` +
  `src/ae.c` (the event loop), `src/rdb.c`, `src/aof.c`, `src/t_string.c` /
  `t_list.c` / `t_hash.c`, `src/pubsub.c`, `src/expire.c`.
- Protocol spec: the RESP documentation on redis.io.
- `man 7 epoll`, `man 2 accept4`, `man 2 fork`, `man 2 fsync`, `man 2 rename`.
- MurmurHash2 by Austin Appleby (public domain); SysV AMD64 psABI for the asm.
