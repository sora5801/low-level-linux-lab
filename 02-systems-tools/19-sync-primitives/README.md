# Your own synchronization primitives 🟧

**What it is.** A from-scratch implementation of the five locks you normally get
from a threading library — a **spinlock**, a **mutex**, a **condition variable**,
a **counting semaphore**, and a **reader-writer lock** — built on nothing but
C11/GCC atomics and the Linux **`futex(2)`** syscall. The star of the show is the
mutex's **fast-path / slow-path split**: an uncontended lock is a single atomic
`compare-exchange` in userspace (no syscall at all), and the kernel is asked to
park or wake a thread only when there is real contention. Every memory-ordering
choice (acquire on lock, release on unlock) and the futex value protocol (the
**three-state mutex** that makes lost wakeups impossible) is explained in the
comments and below. Difficulty: 🟧.

The core (`syncprim.c`) is written **freestanding** — it issues the futex
syscall with a raw `syscall` instruction and uses compiler atomic builtins — so
it pulls in **no system headers** and cross-compiles to Linux assembly on any
host. That is why the committed `asm/` files are *real project code*, not a toy.

## What you'll learn

- **`futex(2)`** (Linux syscall 202): `FUTEX_WAIT` (atomically "compare the word,
  and sleep only if it still matches") and `FUTEX_WAKE`, plus `FUTEX_PRIVATE_FLAG`
  for process-local locks. Why "compare-then-sleep in the kernel" is the exact
  guarantee that closes the lost-wakeup race a userspace check cannot.
- **The fast/slow split**: how a lock avoids the kernel entirely on the common
  path, and pays a syscall only under contention.
- **The three-state mutex** (`0` unlocked / `1` locked / `2` locked-with-waiters)
  and *why two states are not enough* — the third state is how `unlock` knows
  whether a `FUTEX_WAKE` is owed.
- **Memory ordering**: `__ATOMIC_ACQUIRE` on acquire, `__ATOMIC_RELEASE` on
  release, `__ATOMIC_RELAXED` where re-validated — and the specific race each one
  prevents. On x86 these are (mostly) free; the annotations note what they cost
  on a weakly-ordered CPU.
- **Spin-wait discipline**: test-and-**test**-and-set to keep a cache line
  Shared, and the `PAUSE` instruction as a spin hint.
- The **`lock cmpxchg` / `xchg` / `lock dec`** instructions the atomics lower to,
  read straight from the generated assembly.

## Build & run (Linux / WSL2)

The build and run targets need `futex(2)` and pthreads, so they are **Linux/WSL2
only**. (The `asm` target works anywhere — clang cross-targets Linux.)

```bash
make run        # build ./syncdemo and run the self-checking stress tests
# or:  cc -Wall -Wextra -O2 -pthread syncprim.c main.c -o syncdemo && ./syncdemo
```

Expected output (8 threads hammering each primitive):

```
mutex:   counter = 1600000 (want 1600000)
  PASS  mutex enforces mutual exclusion
spinlock: counter = 1600000 (want 1600000)
  PASS  spinlock enforces mutual exclusion
sem:     peak occupancy = 3 (limit 3)
  PASS  semaphore bounds concurrency to its permit count
cond:    consumed = 200000 (want 200000)
  PASS  condvar hands off every item with no lost wakeups
rwlock:  torn reads = 0 (want 0)
  PASS  rwlock gives readers a consistent, writer-exclusive view

ALL TESTS PASSED (0 failures)
```

Each test is designed so a *broken* lock (wrong ordering, lost wakeup, torn
read) makes the final number wrong and the process exit non-zero — `make test`
is therefore a genuine correctness gate, not just a smoke test. Watch the syscall
traffic with `strace -f -e trace=futex ./syncdemo`: you will see almost none on
the uncontended paths and a burst of `FUTEX_WAIT`/`FUTEX_WAKE` under contention.

## How it works (file by file)

- **`syncprim.h`** — the public API and the five state structs. Every lock's
  entire state is one (or, for the rwlock, one packed) 32-bit word, because that
  is exactly what a futex operates on. Heavily commented: read it first.

- **`syncprim.c`** — the implementations, freestanding:
  - `sp_futex()` wraps the raw syscall in inline asm (number in `rax`; args in
    `rdi, rsi, rdx, r10, r8, r9`; `rcx`/`r11` clobbered). A raw syscall returns
    `-errno`, not libc's `-1`; the blocking loops absorb `EAGAIN`/`EINTR`.
  - **spinlock** — test-and-test-and-set with `PAUSE`; never sleeps, never
    syscalls. Acquire = `xchg` (ACQUIRE), release = store 0 (RELEASE).
  - **mutex** — Drepper's three-state design. `lock` tries `CAS 0→1` (fast path),
    then falls back to `xchg`-to-2 + `FUTEX_WAIT` loop. `unlock` does a RELEASE
    `fetch_sub`; only if the word was `2` does it store 0 and `FUTEX_WAKE(1)`.
  - **cond** — a sequence counter. `wait` samples `seq` *while holding the
    mutex*, unlocks, then `FUTEX_WAIT`s on that sample; `signal`/`broadcast` bump
    `seq` (RELEASE) and wake. Sampling under the mutex is what prevents the lost
    wakeup.
  - **sem** — permits in one word; `wait` CASes the count down (or `FUTEX_WAIT`s
    on 0), `post` `fetch_add`s and `FUTEX_WAKE`s one.
  - **rwlock** — reader-preferring, one packed word (high bit = writer,
    low bits = reader count). Readers CAS the count up when the writer bit is
    clear; the last reader out (`1→0`) wakes a waiting writer; a writer CASes
    `0→WRITER`.

- **`main.c`** — the runnable harness (uses pthreads/stdio). Five contention
  tests: two counters (mutex, spinlock), a concurrency-limit check (semaphore), a
  bounded producer/consumer queue (condvar), and a torn-read detector (rwlock).

### The memory-ordering cheat-sheet (and the race each choice prevents)

| Operation | Order | Race it prevents |
|---|---|---|
| lock acquire (CAS/xchg success) | ACQUIRE | critical-section reads hoisted *before* we hold the lock; not seeing the previous holder's writes |
| unlock (store / `fetch_sub`) | RELEASE | our critical-section writes sinking *past* the unlock, so the next holder sees stale data |
| failed CAS / spin load / top-of-loop reload | RELAXED | (nothing — the value is re-validated by the next CAS or by `FUTEX_WAIT`) |
| cond `wait` sampling `seq` | ACQUIRE | pairs with the signaler's RELEASE bump so the predicate write is visible |

### The three-state mutex, precisely

Two states can't work: `unlock` would never know if a sleeper exists. Always
waking costs a syscall on every unlock; never waking deadlocks a sleeper. State
`2` records "a waiter may be parked", so `unlock` wakes **iff** the word was `2`.
A waiter always leaves `2` behind before sleeping (`xchg`-to-2), so the flag
stays armed until the lock is truly free. Worst case is one *extra* harmless
wake — never a *missed* one.

## Assembly notes

`asm/demo.annotated.s` is a hand-written, line-by-line annotation of the `-O1`
output for `asm/demo.c` (the mutex fast path + the `PAUSE` spinlock, extracted so
it compiles with no headers). It shows the payoff of the whole project in a
handful of instructions:

- `__atomic_compare_exchange_n(0→1)` becomes a single **`lock cmpxchgl`** — *that
  one instruction is the mutex acquire*. The `ZF` flag carries success; `eax`
  carries the value it saw, which clang reuses to avoid a reload.
- the spinlock lowers to **`xchgl`** (implicitly locked) to acquire, and a
  read-only spin — `movl (%rdi); testl; jne` — with **`pause`** between reads.
- `unlock` is **`lock decl`** whose `ZF` decides whether a wake is owed.
- clang splits the spinlock so the winning fast path **skips the stack frame
  entirely** — optimizers optimize for the uncontended case.

Because `syncprim.c` is itself freestanding, its **real** code is compiled too:
`asm/syncprim.s` (and `.O0.s`/`.O2.s`) contain the actual `sp_mutex_lock`,
`sp_spin_lock`, etc. — grep them for `lock cmpxchgl`, `xchgl`, `lock decl`,
`pause`, and `syscall` to see the entire design in machine code. Regenerate every
`.s` with `make asm` (the hand-annotated file is never overwritten). Compare
`demo.O0.s` (naive, everything spilled) with `demo.s` (-O1) and `demo.O2.s`.

## Going further (the `Stretch:` goal)

- **Beat the thundering herd.** `cond_broadcast` here wakes every waiter, and
  they all stampede the mutex. glibc uses **`FUTEX_CMP_REQUEUE`** to *move*
  all-but-one sleeper from the condvar's queue directly onto the mutex's queue,
  so exactly one runs and the rest re-park without ever waking. Implement it.
- **Writer preference / fairness.** The rwlock is reader-preferring and can starve
  writers. Add a "writer waiting" bit that blocks *new* readers, or build a
  phase-fair rwlock (readers and writers alternate in phases).
- **Timeouts & robustness.** Thread a `struct timespec` through `sp_futex` for
  `mutex_timedlock`/`sem_timedwait` (note `FUTEX_WAIT` uses a *relative* timeout,
  `FUTEX_WAIT_BITSET` an *absolute* one). Add `FUTEX_OWNER_DIED`/PI-futex handling
  for robust and priority-inheriting mutexes.
- **False sharing.** Pad hot locks to a 64-byte cache line (`_Alignas(64)`) so two
  independent locks don't ping-pong one line between cores; measure the
  difference under contention.
- **What production does.** Read glibc's `nptl/lowlevellock.h` and
  `pthread_mutex_lock.c`, and musl's `src/thread/` — the same three-state mutex
  and futex protocol, hardened for PI, robustness, and process-shared use.

## References

- Ulrich Drepper, *Futexes Are Tricky* — the canonical three-state mutex and the
  condvar/requeue design this project follows.
- `man 2 futex`, `man 7 futex`; the kernel's `Documentation/*` and
  `kernel/futex/`.
- Hans Boehm / the C11 & C++11 memory model; the GCC `__atomic` builtins manual.
- glibc `nptl/`, musl `src/thread/` — the real implementations.
