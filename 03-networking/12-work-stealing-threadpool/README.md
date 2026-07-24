# Work-stealing thread pool 🟧

**What it is.** A thread pool where every worker owns a **Chase-Lev
work-stealing deque**. The owner pushes and pops at one end (LIFO, cache-hot);
idle workers *steal* from the other end (FIFO) of a random victim. Idle workers
**park on a futex** instead of spinning, so an idle pool burns no CPU. Per-worker
state is **cache-line padded** to kill false sharing, and each worker is **pinned
to a CPU** with `sched_setaffinity(2)`. The demo is a divide-and-conquer parallel
sum — the workload work-stealing was invented for.

This is a teaching **core**: the lock-free per-worker deque, the futex parking
protocol, the false-sharing layout, and the affinity are all real and complete.
The scheduler around them is deliberately simple (bounded random-victim stealing,
a mutex-guarded injector for external submits); the production refinements are
listed under *Going further*.

## What you'll learn

- **The Chase-Lev deque** and, more importantly, the **memory ordering** on its
  `top`/`bottom` indices: why `push` needs only a release *fence*, why `take` and
  `steal` need a full `seq_cst` fence, and why the last-element tie is broken by a
  single **CAS** (`atomic_compare_exchange`).
- **C11 atomics** with explicit orders (`relaxed`/`acquire`/`release`/`seq_cst`)
  and `atomic_thread_fence` — and how each lowers to x86-64 (see the asm).
- **futex(2)** as the primitive under every blocking synchronizer: `FUTEX_WAIT`/
  `FUTEX_WAKE`, `FUTEX_PRIVATE_FLAG`, and the **eventcount** pattern that parks
  idle workers without losing wakeups.
- **False-sharing avoidance**: why putting `top` and `bottom` (and each worker's
  state) on separate 64-byte cache lines matters, and how `_Alignas`/padding does
  it.
- **CPU affinity** with `sched_setaffinity(2)` (syscall 203) and why pinning keeps
  a worker's deque resident on one core.

## Build & run

**Platform: Linux (or WSL2).** The pool uses the raw `futex(2)` syscall,
`sched_setaffinity(2)`, and pthreads; it does **not** build on macOS or Windows.
No root is required — a process may always set the affinity of its own threads,
and a process-private futex needs no privilege. (Inside a restrictive container
cpuset, `sched_setaffinity` may return `EINVAL`; the pool logs it and runs
unpinned, which is only a performance loss.)

```bash
make                     # builds ./demo  (clang -Wall -Wextra -O2 -std=gnu11 -pthread)
./demo                   # sum [0, 1e8) with one worker per online CPU
./demo 400000000 8       # sum [0, 4e8) with 8 workers
make run                 # == ./demo
make demo-big            # a larger, more visibly-parallel run
```

Prove the memory model is right, not just plausible, with ThreadSanitizer:

```bash
make tsan                # builds ./demo.tsan with -fsanitize=thread
./demo.tsan 2000000 4    # must report ZERO data races
```

Regenerate the committed teaching assembly (works on any host — clang
cross-targets Linux):

```bash
make asm                 # writes asm/demo.{O0.s,s,O2.s}
```

## How it works

### `chase_lev.h` / `chase_lev.c` — the deque

One deque per worker: two 64-bit **signed** indices, `top` (steal end) and
`bottom` (owner end), into a power-of-two circular buffer. Signedness is
load-bearing: `take` speculatively decrements `bottom` to `-1` on an empty deque,
and the emptiness test `top <= bottom` only works if that `-1` stays negative.
The buffer is a separate heap object so it can be **grown** (doubled) without
tearing a concurrent thief's read; superseded arrays are chained on `prev` and
freed at `cl_destroy` (freeing them at grow time would race a thief still reading
the old array — the classic safe-reclamation hazard).

The whole point is the ordering. Here is the table the code implements (per Lê,
Pop, Cohen & Nardelli, PPoPP 2013):

| op | on `bottom` | on `top` | fence | on the slot |
|----|-------------|----------|-------|-------------|
| `push`  (owner) | relaxed load, relaxed store `+1` | acquire load | **release** before the bottom store | relaxed store |
| `take`  (owner) | relaxed store `-1`, later relaxed restore | relaxed load; **CAS(seq_cst)** iff last element | **seq_cst** after the bottom store | relaxed load |
| `steal` (thief) | acquire load | acquire load; **CAS(seq_cst)** to commit | **seq_cst** between top and bottom loads | relaxed load (speculative) |

- **`push`**: write the item while the slot is still private (bottom unmoved),
  then a **release** fence publishes it *before* the `bottom` bump, so a thief who
  reads the new bottom is guaranteed to see a fully-written slot.
- **`take`**: claim the bottom slot by decrementing, then a **seq_cst** fence
  orders that store before the `top` read. If exactly one item remains, resolve
  the race with a thief using the *same* CAS the thief would use — so exactly one
  of them gets it.
- **`steal`**: acquire-load `top`, **seq_cst** fence, acquire-load `bottom`; if
  non-empty, speculatively read the top slot and commit with a **CAS** on `top`.
  A failed CAS returns `CL_ABORT` (lost the race → try another victim).

Only the contended last-element case does any atomic read-modify-write; the owner
fast paths are plain loads and stores. That asymmetry is the entire performance
argument for Chase-Lev.

### `threadpool.h` / `threadpool.c` — the pool

- **Workers.** Each `worker` is `_Alignas(64)` and tail-padded so two workers'
  hot fields never share a cache line **[false-sharing]**. Its `cl_deque` is
  itself internally padded so this worker's `top` (hammered by thieves) and
  `bottom` (written by the owner on every push/take) sit on different lines.
- **The loop.** `take` own deque → `steal` from a random victim (bounded attempts,
  xorshift-chosen to spread thieves out) → drain the **injector** → else **park**.
- **The injector.** A Chase-Lev push is *owner-only*, so a non-worker thread (like
  `main`) may not push onto any deque. External submits go to a small
  mutex-guarded FIFO that idle workers drain. In fork-join code this is used only
  for the root task; every recursive submit runs on a worker and takes the
  lock-free local path.
- **Parking with a futex eventcount [futex].** A monotonically increasing `gate`
  word is bumped by every submit and by shutdown. A worker about to sleep
  announces itself (`nsleeping++`), snapshots `gate`, re-checks for work, then
  `FUTEX_WAIT(&gate, snapshot)`. Because the snapshot is taken *before* the final
  re-check, any task made visible afterward must have bumped `gate` past the
  snapshot, so the `FUTEX_WAIT` compare fails and the worker loops instead of
  sleeping through the work — no lost wakeups. `nsleeping` (seq_cst, interlocked
  with `gate`) lets a producer skip the `FUTEX_WAKE` *syscall* when nobody is
  parked.
- **`tp_wait`.** Blocks the caller on a second futex word, `pending` (the
  outstanding-task count). The completion that drives `pending` from 1 to 0 wakes
  it. Intermediate decrements don't lose the wakeup: a changed value just makes
  `FUTEX_WAIT` return `EAGAIN` and the waiter re-reads.
- **Clean shutdown.** `tp_destroy` sets `shutdown`, bumps `gate`, and
  `FUTEX_WAKE(INT_MAX)` unconditionally, then joins every worker. Call `tp_wait`
  first for a graceful drain; tasks still queued at an abrupt destroy are freed
  (their `arg` is leaked — documented in the code).
- **Affinity [affinity].** Each worker calls `sched_setaffinity(0, …)` at startup
  to pin itself to `id % nCPU`.

### `main.c` — the demo

Divide-and-conquer sum of `[0, N)`: a task either sums a small leaf range or
splits in half and submits both halves. The root is an external submit (injector);
every split runs on a worker (local push). Because the owner takes its fresh small
halves LIFO while a thief steals the oldest, largest un-split range FIFO, one idle
worker grabs a big slice of the tree per steal and the load self-balances. The
program prints the result, verifies it against `N(N-1)/2`, times it, and reports
each worker's task count so you can see the balance.

## Assembly notes

`asm/demo.c` is the deque's `push`/`take`/`steal` extracted with no system headers
(own types, `__atomic_*` builtins) so it lowers to Linux asm on any host. The full
project files need `<pthread.h>`/`<linux/futex.h>` and are not standalone, so this
extraction is the didactic subject; `asm/demo.annotated.s` annotates the `-O1`
output instruction by instruction. What to look for:

- **The x86-TSO lesson.** The **release** fence in `push` compiles to *nothing* (a
  `#MEMBARRIER` marker) because x86 never reorders a store after a later store —
  release is free. The **seq_cst** fences in `take`/`steal` compile to a real
  **`mfence`**, because the one reordering x86 *does* allow is StoreLoad, which is
  exactly the bottom-store-then-top-load ordering Chase-Lev depends on. Same C,
  wildly different cost.
- **`lock cmpxchgq`.** The CAS on `top` in both `steal` and `take`'s last-element
  path is a single locked compare-exchange — the only atomic RMW on the hot path.
- **Branchless selects.** clang picks "got the item" vs. "lost the race"
  (`EMPTY`/`ABORT`) with `cmov` off the CAS's zero flag, no branch.

Compare `asm/demo.O0.s` (everything spilled to the stack, easiest to trace) and
`asm/demo.O2.s` (same logic, frame pointer dropped).

## Going further

The `Stretch:` direction and what production systems add:

- **Lock-free injector.** Replace the mutex FIFO with a Michael-Scott queue or a
  bounded MPMC ring so external submits never take a lock. (Go's global runq, Rust
  rayon's injector.)
- **Safe memory reclamation.** This core defers all grown-array frees to
  `destroy`. Production uses **hazard pointers** or **epoch-based reclamation** to
  free them promptly while thieves may still hold stale pointers.
- **Smarter parking.** Adaptive spin-then-park, waking exactly one worker per task
  (this core wakes all — simple, but a thundering herd), and a "spinning worker"
  count so the last spinner doesn't sleep with work pending.
- **NUMA-aware stealing.** Prefer victims on the same socket; fall back across
  sockets only when local stealing fails.
- **Continuation stealing (Cilk).** Steal the *continuation* of a spawn rather
  than the child, bounding stack depth to the serial depth — the deep end of this
  topic.

## References

- D. Chase, Y. Lev, *Dynamic Circular Work-Stealing Deque*, SPAA 2005 — the
  original growable deque.
- N. M. Lê, A. Pop, A. Cohen, F. Zappa Nardelli, *Correct and Efficient
  Work-Stealing for Weak Memory Models*, PPoPP 2013 — the C11 memory orders this
  code uses (and the bug in the 2005 fences it fixes).
- R. D. Blumofe, C. E. Leiserson, *Scheduling Multithreaded Computations by Work
  Stealing*, FOCS 1994 — the theory (Cilk).
- `man 2 futex`, `man 7 futex`, `man 2 sched_setaffinity`, `man 3 pthread_create`.
- Read the real thing: Go runtime `runtime/proc.go` (per-P run queues + global
  runq), Rust `crossbeam-deque` (a Chase-Lev implementation), Intel TBB
  `task_stealing`, Java `java.util.concurrent.ForkJoinPool`.
