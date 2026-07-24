# Embedded database / KV store 🟥

**What it is.** A persistent, crash-safe key/value store in ~1000 lines of C: an
on-disk **B+-tree** of fixed 4 KiB pages, made durable by a **write-ahead log**
that is `fdatasync`-ed *before* the data pages it protects, with **crash recovery**
that replays the log on open. Every page carries a **CRC32** so a torn write is
detected, not trusted. All I/O is **positioned** (`pread`/`pwrite`). This is a
genuinely working teaching *core* — it stores, retrieves, deletes, scans in key
order, splits nodes as it grows, and survives `kill -9` at any instant — with the
harder production concerns (page reclamation, node merging, MVCC, batched
checkpoints, overflow pages) deliberately left out and called out below.

The one idea to take away: **the log hits stable storage before the home block it
protects.** That single ordering rule, enforced with two `fdatasync`s per write,
is the whole reason a power cut can't corrupt this file.

## What you'll learn

- **Durability syscalls and their ordering:** `pwrite(2)` (offset-carrying,
  race-free, O_DIRECT-friendly), and **`fdatasync(2)` vs `fsync(2)`** — why the
  WAL barrier uses `fdatasync` (flush the data + the file size, skip the
  timestamp write) and why its *placement* between the log write and the data
  write is the correctness point.
- **Write-ahead logging & recovery:** frame format, a per-generation salt +
  per-frame CRC to reject torn/stale frames, commit markers, and idempotent redo
  replay. The crash-window analysis (what happens if we die at each step) is
  written out in `wal.c`.
- **B+-tree on a slotted page:** the page header, a sorted **slot array** with a
  separate unsorted cell area, **binary search** (lower-bound for leaves,
  upper-bound for internal descent), node **splitting** with separator promotion,
  and leaf sibling links for O(1)-per-step range scans.
- **On-disk format discipline:** hand-written **little-endian** codecs (never
  `*(u32*)p`), explicit byte offsets, and a page CRC — the things that make a
  file format portable and corruption-evident.
- **O_DIRECT alignment** (the `align_up` trick) and where `mmap` would fit — as
  notes; the code stays on `pread`/`pwrite`.

## Build & run (Linux / WSL; also macOS)

```bash
make            # builds ./db  (clang -Wall -Wextra -O2)
make run        # 2000-insert self-check: inserts, point-gets, full scan, page count
make test       # put/get/del/scan smoke test + the durability story
```

Use it directly:

```bash
./db kv.db put user:1 alice
./db kv.db put user:2 bob
./db kv.db get user:1            # -> alice
./db kv.db del user:2
./db kv.db scan                  # every pair, in key order
```

**See crash recovery for real.** There is a built-in crash-injection point: set
`KVDB_CRASH_AFTER_WAL=1` and a `put` dies the instant *after* its commit is
durable in the log but *before* any page reaches its home block — the most
dangerous moment:

```bash
./db kv.db put a baseline
KVDB_CRASH_AFTER_WAL=1 ./db kv.db put b survives   # process exits 99, data file stale
ls -l kv.db-wal                                    # > 32 bytes: an uncheckpointed commit
./db kv.db get b                                   # -> survives   (recovered on reopen!)
```

On the reopen, `db_open` runs `wal_recover`, replays the committed frame into the
data file, and resets the log. You always observe *either* the complete write
*or* none of it — never a half-written page. (A plain `kill -9` at any moment
gives the same guarantee; the env var just makes the window reproducible.) The
`kv.db-wal` sidecar is the log.

> Assembly generation (`make asm`) works on any host — clang cross-targets Linux.

## How it works

A write flows top-to-bottom; each file is one layer.

- **`db.h`** — the on-disk contract: 4 KiB page geometry, the 16-byte **node
  header** (crc, type, nslots, cell_top, rightmost) with every byte offset named,
  the **meta page** (page 0) fields, and the little-endian `rd16/rd32/wr16/wr32`
  codecs that are the *only* path between memory and disk.
- **`crc32.c`** — table-driven reflected CRC32 (poly `0xEDB88320`, the
  zlib/gzip/PNG variant), exposed in streaming form so the WAL can checksum a
  frame header and page image in one pass.
- **`pager.c`** — turns the file into numbered pages: robust `pread_full`/
  `pwrite_full` loops (handle `EINTR` and partial I/O), per-page CRC verify on
  read / stamp on write, a resident page cache, page allocation (grow-only), and
  the per-transaction **dirty set**. Buffers are 4 KiB-aligned (O_DIRECT-ready).
- **`wal.c`** — the durability engine. `wal_commit` performs the five-step
  ordering (log → **`fdatasync(WAL)`** → checkpoint to home blocks →
  `fdatasync(data)` → reset log); `wal_recover` replays committed frames on open.
  The header comment is the durability proof.
- **`btree.c`** — the B+-tree over slotted pages: `key_cmp`, the in-page binary
  searches, and insert/delete written as **decode → modify → re-emit (fit or
  split)**, which makes the split logic legible and obviously correct. Leaves are
  chained for scans.
- **`db.c`** — open/recover/close and the auto-committing `db_put`/`db_del`
  (mutate cache, then `wal_commit`; roll back the in-memory changes on error) plus
  read-only `db_get`/`db_scan`.
- **`main.c`** — the `put/get/del/scan/demo` CLI.

**Page layout (slotted page), the shape every node shares:**

```
+--------------------+ 0     node header: crc(4) type(1) nslots(2) cell_top(2) rightmost(4)
| slot[0] slot[1] .. | 16    sorted u16 cell offsets  (binary search runs here)
| ...   (grows ->)   |
+--------------------+ <- slots_end
|     free space     |
+--------------------+ <- cell_top   (cells grow DOWN from the page end)
| cell   cell   cell | 4096  variable-length records, unsorted; only slots sorted
+--------------------+
   leaf cell    : u16 key_len, u32 val_len, key, val
   internal cell: u32 child,  u16 key_len,  key
```

**What this core covers, honestly:** insert/overwrite/get/delete/ordered-scan;
leaf and internal node splits with separator promotion and a growing root; full
WAL durability and crash recovery; page CRCs. **What it omits** (each a pointer
for going further): no page reclamation / freelist (the file is grow-only); no
node **merge/rebalance on delete** (the tree stays correct but can get sparse);
no **overflow pages** (a single key+value must fit one page — enforced); no MVCC
or concurrency (one handle, single-threaded); checkpoint runs **every commit**
rather than batching many commits behind one `fsync`; `mmap` and `O_DIRECT` are
discussed and the buffers are aligned for them, but the live path uses
`pread`/`pwrite`.

## Assembly notes

`asm/demo.c` extracts the store's pure-logic core (no syscalls, self-contained)
and `asm/demo.annotated.s` walks the clang **-O1** output instruction by
instruction. Highlights:

- **`le16`/`le32`** — the hand-rolled little-endian decoders compile to a *single*
  `movzwl`/`movl`. On a little-endian CPU the portable byte-assembly costs
  nothing; you keep portability and give up no speed.
- **`align_up`** — the O_DIRECT/mmap alignment mask `~(a-1)` becomes one `neg`
  (`-a == ~(a-1)`).
- **`slot_lower_bound`** — the star: clang **inlines** `key_cmp` into the search
  and makes the `lo = mid+1 else hi = mid` update **branchless** with `cmov`
  (`lo` in `eax`, `hi` in `esi`). You can see the slot arithmetic as
  `movzwl 16(%rdi,%r9)` (slot → offset), then `(%rdi,%r10)` (→ key_len), then
  `6(%r10,%rbx)` (→ key bytes).
- **`crc32_byte`** — the inner `if` is a `cmov`: compute both the plain shift and
  the shift-XOR-polynomial, select on the shifted-out bit. That is exactly the
  math the 256-entry table in `crc32.c` precomputes.

Regenerate with `make asm`. Compare `asm/demo.O0.s` (naive, everything spilled)
with `asm/demo.s` (-O1, annotated) and `asm/demo.O2.s` (-O2).

## Going further (the `Stretch:` from the list)

- **LSM-tree instead of a B-tree.** Buffer writes in an in-memory sorted table,
  flush it to an immutable sorted run (SSTable), and **compact** overlapping runs
  in the background. Writes become sequential appends (great for SSDs) at the
  cost of read/space amplification.
- **Bloom filters per SSTable** so a point lookup can skip runs that certainly
  lack the key — one bit-array probe instead of a binary search per level.
- **Batched WAL + background checkpoint** with a WAL-index, so many transactions
  share one `fsync` and readers can find a page that still lives only in the log.
- **MVCC / snapshot isolation:** keep versioned pages and a read snapshot so
  readers never block writers (what real SQLite-WAL and Postgres do).
- **O_DIRECT + async I/O** (`io_uring`) to bypass the page cache with our already
  aligned buffers; **overflow pages** for large values; delete **merge/borrow**
  to keep the tree balanced.

The canonical reads: SQLite's `btree.c` and its WAL design doc; the LMDB source
(a compact copy-on-write B+-tree with `mmap`); the LevelDB/RocksDB internals for
the LSM side.

## References

- `man 2 pwrite`, `man 2 fsync`, `man 2 fdatasync`, `man 2 open` (`O_DIRECT`),
  `man 2 ftruncate`.
- SQLite: "Write-Ahead Logging" (sqlite.org/wal.html) and the file-format doc.
- LMDB (Howard Chu): a memory-mapped, copy-on-write B+-tree worth reading whole.
- "Modern B-Tree Techniques" (Graefe) for splits, prefixes, and slotted pages.
