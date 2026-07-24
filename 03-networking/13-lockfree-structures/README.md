# Lock-free data structures 🟥

**What it is.** Three concurrent containers built with **C11 atomics and
explicit memory ordering** — a Michael & Scott MPMC queue, a Treiber stack, and
a lock-free open-addressing hash map — plus the two canonical answers to the
**ABA problem**: a **version-tagged pointer** updated by double-width CAS
(`lock cmpxchg16b`) for the stack, and **hazard pointers** for the queue. Every
atomic carries an explicit `memory_order` and a comment naming the exact race
that order prevents. This is a 🟥 giant, so it ships a **teaching core**: the
structures are complete and correct, but the map deliberately omits resize
(see [Scope](#scope-what-this-covers-and-what-it-omits)).

## What you'll learn

- The **C11 memory model** in anger: `relaxed` / `acquire` / `release` /
  `acq_rel` / `seq_cst`, and *why each one and not a weaker one* at every site.
- **Compare-and-swap loops** as optimistic concurrency, and how they lower to a
  single `lock cmpxchg` (8-byte) or `lock cmpxchg16b` (16-byte) instruction.
- The **ABA problem** — what it is, and the two production fixes:
  - **Tagged pointers**: a version counter packed next to the pointer so a
    recycled address is a *different* word. Needs a **double-width CAS**.
  - **Hazard pointers**: per-thread published references + deferred free, the
    standard lock-free **Safe Memory Reclamation** scheme.
- Why ABA and **memory reclamation** are *different* problems, and why a correct
  Treiber stack pairs tagging with a **type-stable free list**.
- The **`lock` prefix**, cache-line **false sharing**, and the `pause` spin hint.

## Build & run (Linux / WSL)

```bash
make run                 # build ./stress and run the concurrency tests
./stress 8 500000        # 8 threads, 500k ops each (override the defaults)
make tsan                # rebuild under ThreadSanitizer and stress it — the
                         #   real validation of the memory ordering (see below)
```

Requirements: a C11 compiler with `_Atomic` (clang or gcc), `pthreads`, and an
x86-64 CPU with **CX16** (every CPU since ~2006) for the stack's 128-bit CAS.
The build passes `-mcx16` so `lock cmpxchg16b` is emitted inline; if your
toolchain instead emits a `__atomic_compare_exchange_16` call, add
`LDLIBS=-latomic`. No root or special capabilities are needed — this project is
pure userspace CPU/memory work.

Expected output (values depend on thread/op counts):

```
lock-free stress: 8 threads x 500000 ops each
[stack]  threads=8 ops=500000  sum=... expected=...  empty=yes  -> PASS
[queue]  threads=8 ops=500000  sum=... expected=...  empty=yes  -> PASS
[map]    threads=8 ops=500000  keys=4000000  missing=0 wrong=0 delete_errs=0  -> PASS
ALL TESTS PASSED
```

## How it works

The test harness uses a **conservation invariant**: T threads each insert `OPS`
distinct values, then (past a barrier) remove `OPS` each. Since inserts == removes
the container must end **empty** and the **sum removed must equal the sum
inserted**. A lost node changes the sum; a duplicated node (double-free / ABA
corruption) changes it or crashes; a spurious "empty" during the drain is itself
a lost element. It is a cheap, brutal correctness oracle.

File by file:

- **`lockfree.h`** — the shared vocabulary: the `lf_value` payload word, the
  cache-line constant used to pad hot atomics apart (false-sharing avoidance),
  and `lf_pause()` (the x86 `pause` / arm64 `yield` spin hint).

- **`treiber_stack.{h,c}`** — a LIFO stack whose `push`/`pop` are each one CAS
  on a **128-bit tagged head** `{pointer, 64-bit counter}`. The counter bumps on
  every update, so ABA is defeated: a recycled pointer arrives with a stale
  counter and its CAS fails. Reclamation is handled by an internal
  **type-stable free list** — popped nodes are recycled, never returned to
  `malloc` while the stack lives, so a stale dereference reads valid (if
  outdated) memory instead of faulting. This is *why* tagging and node pooling
  go together. Ordering: `push` publishes with **release**, `pop` observes with
  **acquire**; the long comments justify every one.

- **`hazard.{h,c}`** — **hazard pointers**. A thread publishes a node into a
  single-writer slot before dereferencing it, then re-validates; a reclaimer
  *retires* (defers) frees and only releases a node once no slot names it. The
  hot path is `seq_cst` because correctness needs a **StoreLoad** fence on both
  sides (announce-then-check vs. unlink-then-scan) — the one ordering x86 does
  not give for free. Batched scanning keeps it amortized-cheap and bounded.

- **`ms_queue.{h,c}`** — the **Michael & Scott** FIFO: a linked list with a
  permanent **dummy** node, `head` and `tail` on separate cache lines. Producers
  CAS a node onto `tail->next` then swing `tail`; a lagging `tail` is
  **cooperatively helped** forward by whoever notices (this "helping" is what
  makes it lock-free). Consumers advance `head` past the dummy and **retire** the
  old dummy through hazard pointers. Every shared dereference is hazard-guarded.

- **`hashmap.{h,c}`** — a fixed-capacity **open-addressing** map (linear
  probing). Insertion **claims an empty slot with one CAS** on the key; updates
  and lookups are lock-free loads/stores. The subtlety is the **publication
  window** (key set, value not yet stored): readers wait out `HM_BUSY` with
  `acquire`, the one bounded (single-writer, never a lock) wait in the map.

- **`main.c`** — the pthreads stress harness described above.

### The memory-ordering cheat-sheet (the heart of the project)

| Site | Order | Race it prevents |
|------|-------|------------------|
| Stack `push` CAS success | `release` | a popper seeing the new head but stale `node->next`/`value` |
| Stack `pop` head load / CAS | `acquire` | dereferencing `top->next` before the pusher's writes are visible |
| Stack tag bump | (in the word) | **ABA**: recycled pointer + old counter ≠ current word ⇒ CAS fails |
| Queue link CAS (`tail->next`) | `release` | a dequeuer reading `next->value` before it was written |
| Queue `head`/`tail`/`next` loads | `acquire` | using a node before its publication is visible |
| Hazard announce + validate | `seq_cst` | **StoreLoad**: reclaimer freeing a node between announce and use |
| Hazard scan slot loads | `seq_cst` | missing a just-announced hazard and freeing a live node |
| Map key claim CAS | `acq_rel` | two inserters both thinking they own a slot |
| Map value store / load | `release`/`acquire` | reader seeing a claimed key but an unpublished value |
| Registration / counters | `relaxed` | (nothing — pure atomicity, not used to order other data) |

## Assembly notes

`asm/demo.c` is a **self-contained** extraction of the project's most
instructive routine: the Treiber `push`/`pop` **CAS retry loops**. To make the
committed assembly show a real inline `lock cmpxchg` *without* needing `-mcx16`,
the demo uses a **64-bit packed tagged pointer** — `[tag:16 | pointer:48]` in one
word (x86-64 user addresses are canonical and fit in 48 bits), so the atomic is
a natively-lock-free 8-byte word and compiles to `lock cmpxchgq` at every `-O`.
(The real library uses the wider 128-bit `cmpxchg16b`; the demo trades counter
width for a flag-free build.)

- [`asm/demo.annotated.s`](asm/demo.annotated.s) — the hand-annotated `-O1`
  output: every instruction commented, plus the SysV AMD64 ABI header. It shows
  the two things worth seeing: the **`lock cmpxchgq`** at the center of each
  loop, and how the optimizer **precomputes the loop-invariant tag arithmetic**
  and folds `pack(ptr, tag+1)` into three `and`/`or`/`add` instructions using a
  carry trick (all explained inline).
- [`asm/demo.O0.s`](asm/demo.O0.s) — the naive mapping, every value spilled;
  easiest to trace statement by statement.
- [`asm/demo.s`](asm/demo.s) — `-O1`, the annotated baseline.
- [`asm/demo.O2.s`](asm/demo.O2.s) — `-O2`, for comparison.

Regenerate with `make asm`. Key takeaway from the asm: **on x86 (TSO) the
release/acquire orders are invisible** — plain `mov`s do the work — but they are
*not* free: they constrain what the **compiler** may reorder, and on
weakly-ordered ISAs (arm64) they become real barriers (`ldar`/`stlr`). Never
drop a memory order just because the x86 asm "looks the same."

## Scope (what this covers and what it omits)

Honest accounting, per the lab's 🟥 rules:

- **Complete & correct:** the Treiber stack (tagged pointer + type-stable
  reclamation), the Michael & Scott queue (with hazard-pointer SMR), and the
  hash map's concurrent probe/insert/update/delete with correct publication.
- **Omitted, and why:**
  - **Map resize.** A growable lock-free map (Cliff Click's) is a state machine
    over "copy-in-progress" slots — a project in itself. Ours is fixed-capacity;
    `hm_put` returns `-1` when full. Deleted slots are tombstoned, not
    reclaimed, so a table churned full of tombstones needs a rehash we don't do.
  - **Hazard-pointer dynamic growth.** The domain has a fixed thread/slot cap
    (`HP_MAX_THREADS`); production grows the slot list lazily.
  - **The relaxed hazard-pointer fast path.** We use `seq_cst` on the hot path
    for a *provably* correct, easy-to-read version; folly/Michael shave it to
    `release`/`acquire` + explicit `seq_cst` fences.

## Going further (the `Stretch:` from the list)

- **Model-check it.** Port each core loop to a **herd7** litmus test or run the
  whole thing under **CDSChecker**, which exhaustively explores the C11
  execution graph and will find a wrong memory order that stress testing misses
  by luck. `make tsan` is the pragmatic middle ground: **ThreadSanitizer**
  builds the happens-before graph at runtime and flags any unsynchronized
  access — run it with more threads than you have cores.
- **Elimination backoff** on the Treiber stack: pair a stalled `push` with a
  waiting `pop` so they cancel out, turning contention into throughput.
- **Read the real thing.** `folly::MPMCQueue` and `folly/synchronization/
  HazptrDomain`; `liburcu` (userspace RCU, the other big SMR family); Cliff
  Click's *NonBlockingHashMap*; Maged Michael's 2004 hazard-pointer paper and
  the 1996 Michael & Scott queue paper (both short and worth reading in full).

## References

- Maged M. Michael & Michael L. Scott, *Simple, Fast, and Practical Non-Blocking
  and Blocking Concurrent Queue Algorithms* (PODC 1996).
- Maged M. Michael, *Hazard Pointers: Safe Memory Reclamation for Lock-Free
  Objects* (IEEE TPDS 2004).
- R. Kent Treiber, *Systems Programming: Coping with Parallelism* (IBM, 1986).
- C11 standard §7.17 `<stdatomic.h>`; the Intel SDM entries for `CMPXCHG`,
  `CMPXCHG16B`, and the `LOCK` prefix.
- Herlihy & Shavit, *The Art of Multiprocessor Programming*, ch. 9–11.
