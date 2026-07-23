# A sampling profiler 🟧

**What it is.** A working, self-contained CPU **sampling profiler**: it stops a
program many times a second, records where the program counter is and the chain
of return addresses above it (the call stack), aggregates identical stacks into
counts, and prints them in Brendan Gregg's **collapsed-stack** format — the exact
input a flame graph wants. It ships **two interchangeable sampling backends** so
you can see the concept from both ends:

- **perf backend** (the production shape): the kernel does the sampling and stack
  unwinding for us via `perf_event_open(2)`, and we read the samples out of a
  shared **mmap ring buffer**. This is how `perf record` works.
- **SIGPROF backend** (the from-scratch shape): a `setitimer(ITIMER_PROF)`
  interval timer raises `SIGPROF`, and inside the handler *we* read the
  interrupted registers from the `ucontext` and **walk the frame-pointer chain
  ourselves**. No privileges required.

Difficulty **🟧**. This is a genuine, end-to-end teaching core: run it and you get
a real flame graph of a real workload with real function names. See
[Going further](#going-further) for the honest list of what a production profiler
adds on top.

> **Platform: Linux (or WSL2) only.** The sources use `perf_event_open`, the perf
> ring buffer, `<ucontext.h>` register access, `setitimer`, and `dladdr`. The
> `make asm` target still works anywhere (clang cross-compiles to Linux).

## What you'll learn

- **`perf_event_open(2)`** end to end: filling `struct perf_event_attr` for
  frequency sampling (`freq=1`, `sample_freq`), choosing `PERF_TYPE_SOFTWARE` /
  `PERF_COUNT_SW_TASK_CLOCK`, requesting `PERF_SAMPLE_IP | TID | TIME |
  CALLCHAIN`, and why `exclude_kernel=1` is what lets an unprivileged user run it.
- **The mmap ring buffer head/tail protocol**: one control page + a power-of-two
  data ring; the kernel's `data_head` vs our `data_tail`; the **acquire/release
  memory barriers** that make the hand-off race-free, and exactly which race each
  one prevents.
- **Parsing `PERF_RECORD_SAMPLE`**: the fixed field order implied by
  `sample_type`, callchain **context markers**, and stitching a record that
  **wraps** across the end of the ring.
- **Frame-pointer stack unwinding**: why `[rbp]` / `[rbp+8]` form a linked list
  of frames, the guards that keep the walk from faulting, and why
  `-fno-omit-frame-pointer` is mandatory.
- **Signal-handler discipline**: `SA_SIGINFO` + `ucontext`, and the hard rule of
  **async-signal-safety** (no `malloc`/`printf`/`dladdr` in the handler).
- **Folding to a flame graph**: collapsed-stack format and `dladdr` symbolization
  (and its limits).

## Build & run (Linux / WSL2)

```bash
make                 # builds ./profiler with -fno-omit-frame-pointer -rdynamic

make run             # self-profile the built-in workload via perf; collapsed
                     # stacks go to stdout, a summary to stderr
make run-timer       # same, but via the SIGPROF backend (needs no privileges)

# Render a flame graph (flamegraph.pl is Brendan Gregg's script; not vendored):
./profiler > profile.folded
flamegraph.pl profile.folded > profile.svg
```

Useful options (`./profiler -h`):

```
-F hz       sampling frequency        (default 997)
-m pages    perf ring DATA pages,     (default 128 = 512 KiB; must be a power of
            power of two               two, and <= kernel.perf_event_mlock_kb when
                                       unprivileged)
-n rounds   built-in workload length  (default 150)
-t          use the SIGPROF backend instead of perf
-p pid      attach to an existing process (perf only)
-d ms       attach-mode duration      (default 1000)
```

Expected shape of the output (counts vary by machine speed):

```
main;workload_entry;wl_compute;wl_heavy_a;wl_hash 812
main;workload_entry;wl_compute;wl_heavy_b;wl_mul 486
...
-- 1300 samples, 6 unique stacks, 0 lost --
```

**If `make run` prints a `perf_event_open` permission error**, your machine has
`kernel.perf_event_paranoid` set high (3 on hardened Ubuntu). Either lower it
(`sudo sysctl kernel.perf_event_paranoid=2`) or just use `make run-timer`, which
never needs it. Attaching to another process (`-p`) needs paranoid `<= 0`.

## How it works

The program is a thin driver over two backends that both feed a shared
aggregator. Data flows: **backend → callchain → stackmap → collapsed stdout.**

- **`profiler.c`** — the driver. Parses flags, holds the built-in **workload** (a
  deliberate call tree: `wl_compute → wl_heavy_a → wl_hash` and `→ wl_heavy_b →
  wl_mul`, all `noinline` and externally linked so they unwind and symbolize
  cleanly), runs the chosen backend, then prints collapsed stacks to stdout and a
  summary to stderr.
- **`sampler.h`** — the seam. Defines `prof_stack_fn` (the "here is one callchain"
  callback) and `struct prof_config`, so both backends and the aggregator share
  one contract.
- **`perf_sampler.c`** — the perf backend. Calls `perf_event_open` (via raw
  `syscall`, since libc has no wrapper), `mmap`s the ring, `ioctl(ENABLE)`s the
  event, and drains records. **The drain loop is the centerpiece:** load-acquire
  `data_head`, walk records from `data_tail` to it (linearizing any that wrap),
  dispatch `PERF_RECORD_SAMPLE`/`PERF_RECORD_LOST`, then store-release the new
  `data_tail`. Self mode runs the workload then drains once; attach mode
  `poll()`s and drains until the target exits.
- **`sigprof_sampler.c`** — the timer backend. Arms `ITIMER_PROF`, and in the
  `SIGPROF` handler reads `%rip`/`%rbp`/`%rsp` from the `ucontext` and calls
  `fp_walk` (the frame-pointer chain walk) into a **preallocated** buffer —
  nothing else, because the handler must be async-signal-safe. After the run it
  replays the captured stacks (symbolizing off the signal path). It reads the
  main-thread stack bounds from **`/proc/self/maps`** so the walk can never read
  past the stack.
- **`stackmap.c`** — aggregation + output. A chained hash table keyed on the
  symbolized, `;`-joined stack string; FNV-1a buckets; `stackmap_print_folded`
  sorts by count and emits collapsed format.
- **`symbolize.c`** — `dladdr`-based address → function name, and an honest
  account of what that can't do (statics, foreign processes, the kernel).

### The ring buffer head/tail protocol, precisely

```
        base ─┐
              ▼
   ┌───────────────────────┐  page 0: struct perf_event_mmap_page
   │  control page         │          (data_head, data_tail, data_offset, …)
   ├───────────────────────┤  ◄─ base + data_offset  (normally +1 page)
   │                       │
   │   data ring (2^n      │  kernel APPENDS records here and bumps data_head
   │   pages, records,     │  we CONSUME records and bump data_tail
   │   wraps at the end)   │
   └───────────────────────┘
```

`data_head` (kernel-written) and `data_tail` (us) are **free-running 64-bit byte
counters** — never masked in the shared page, they just increase forever and wrap
at 2⁶⁴. The physical slot for any counter is `counter & (data_size - 1)` (the
buffer is a power of two, so modulo is a mask). Bytes available to read is
`data_head - data_tail`, computed unsigned so it stays correct across the wrap.
The two barriers:

- **load-acquire `data_head`** before reading records — pairs with the kernel's
  release when it publishes the head, guaranteeing the record bytes are visible.
  Without it, a load of the record could be reordered before the head read and
  observe half-written data.
- **store-release `data_tail`** after reading — guarantees our reads finish before
  the kernel sees the freed space, so it can't overwrite a record we're still
  parsing.

On x86 these compile to plain loads/stores, but the `__atomic_*` builtins still
emit the **compiler** barrier that actually matters in practice.

## Assembly notes

`asm/demo.c` extracts the profiler's two pure-logic hot loops — the ones with no
kernel dependency, so clang can compile them to assembly on any host:

1. **`fp_unwind`** — the frame-pointer stack walk. In `asm/demo.annotated.s` you
   can watch the `[rbp]`/`[rbp+8]` loads, the four guard compares of the `while`,
   and clang's neatest trick here: it fuses `if (saved <= fp) break; fp = saved;`
   into a single `cmp` + **`cmovaq`** + `ja` — a conditional move that advances the
   frame pointer only when the chain keeps climbing. It also **inlined `fp_unwind`
   into the self-test twice**, so the same loop appears three times; diffing them
   is a lesson in what inlining preserves.
2. **`rb_avail` / `rb_offset` / `rb_first_chunk`** — the ring index math. The asm
   shows the whole head/tail protocol reduced to primitives: backlog is one
   `sub` (`head - tail`); the slot index is one `and` (`counter & (size-1)`, with
   `size-1` computed by a `lea`); and the wrap split `min(need, size-off)` comes
   out **branchless** as a `cmov`.

A third routine, `fnv1a`, is the exact hash `stackmap.c` keys on, included so the
asm shows the tight xor/`imul` mixing loop. Compare the three optimization levels:

- [`asm/demo.O0.s`](asm/demo.O0.s) — naive, everything spilled to the stack.
- [`asm/demo.s`](asm/demo.s) — `-O1`, the annotated baseline.
- [`asm/demo.O2.s`](asm/demo.O2.s) — `-O2`, unrolling and folding on display.

> **Why `demo.c` and not the real sources?** `perf_sampler.c`,
> `sigprof_sampler.c`, and the rest all `#include` Linux/system headers, so clang
> cannot compile them to assembly on a non-Linux host. Per the repo convention we
> extract the header-free core into `asm/demo.c` and annotate that. The extracted
> loops are byte-for-byte what the real code runs (`rb_read` in perf_sampler.c and
> `fp_walk` in sigprof_sampler.c are the same arithmetic). Regenerate with
> `make asm`.

## Going further

The `Stretch` goal — and what a production profiler (perf, async-profiler,
pprof) does that this teaching core does not:

- **DWARF / `.eh_frame` unwinding.** Frame-pointer walking breaks the moment any
  frame in the stack was compiled with `-fomit-frame-pointer` (most of libc, most
  release builds). Production tools request `PERF_SAMPLE_STACK_USER` (a copy of
  the raw stack) or `PERF_SAMPLE_REGS_USER` and unwind using the `.eh_frame` CFI
  via **libunwind**/**libdw** — no frame pointer needed. That is the single
  biggest gap here.
- **Symbolizing foreign processes.** In `-p` attach mode our `dladdr` can't name
  another process's addresses. A real tool reads the target's `/proc/PID/maps`,
  finds the backing ELF for each address, and looks it up in that file's symbol
  table + DWARF (and handles ASLR by subtracting the mapping base).
- **Kernel and JIT symbols.** `/proc/kallsyms` for kernel frames (we set
  `exclude_kernel`), and `/tmp/perf-<pid>.map` for JITs like the JVM/V8.
- **Streaming and overflow.** We size the ring to avoid overflow in a short self
  run; `perf record` continuously drains via `poll` and writes `perf.data`. Our
  attach mode does poll-drain; watch the `lost` count if you push the rate up.

## References

- `man 2 perf_event_open` — the authoritative field-by-field reference; its
  "MMAP layout" and "Overflow handling" sections are exactly this protocol.
- Linux `tools/lib/perf/mmap.c` and `tools/perf/util/mmap.c` — the real drain
  loop, including `perf_mmap__read_head` and the `smp_rmb`/`smp_mb` barriers.
- Brendan Gregg, *"Flame Graphs"* and the `FlameGraph` repo (`stackcollapse-*.pl`,
  `flamegraph.pl`) — the collapsed-stack format and renderer.
- `man 2 setitimer`, `man 7 signal-safety` (the async-signal-safe function list),
  `man 3 dladdr`.
- The System V AMD64 ABI — the frame-pointer/prologue contract the unwind relies
  on (see `04-security-asm/01-nolibc-programs` for the ABI basics).
