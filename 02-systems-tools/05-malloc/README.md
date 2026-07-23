# Your own malloc 🟥

**What it is.** A genuine `malloc`/`free`/`realloc`/`calloc` for Linux/x86-64,
built the way real allocators are: it gets memory from the kernel with
**brk/sbrk** (the main heap) and **mmap** (large blocks), hands out **16-byte
aligned** payloads, uses **boundary-tag headers/footers** so it can coalesce
adjacent free blocks in **O(1)**, sorts free blocks into **segregated free lists**
by size class, catches heap corruption with a **header magic + footer redundancy
check**, and adds a **tcmalloc-style per-thread cache** for the lock-free small
path. Built as a shared object it drops in under **`LD_PRELOAD`** and replaces
glibc's allocator for any dynamically-linked program; a benchmark harness compares
the two.

It is a **teaching core**, and honestly scoped. It really works — it passes a
400k-operation randomized stress test, an 8-thread concurrency test, and drives
real programs (`ls`, `bash`) under `LD_PRELOAD` — but it deliberately omits the
machinery that makes production allocators fast and hard: multiple arenas, page
run management, decay-based reclaim (`MADV_FREE`), and security hardening. The
["Going further"](#going-further) section spells out the gap.

## What you'll learn

- **brk/sbrk(2)** — moving the program break to grow one contiguous heap, and why
  successive `sbrk`s return adjacent memory (so coalescing is pure pointer math).
- **mmap/munmap(2)** — serving big allocations from their own mapping so `free`
  can hand the pages straight back to the kernel, and *why* there is a threshold.
- **Boundary tags** — the header/footer trick that makes merging a freed block with
  either physical neighbour O(1) with no search (CS:APP's `malloclab` design).
- **Segregated free lists** — size classes indexed by a **count-leading-zeros bit
  trick** (`63 - clz(x) == floor(log2 x)`), giving O(1) best-bin lookup.
- **Alignment math** — why a 16-byte header keeps every payload 16-aligned, and the
  `(n + a-1) & ~(a-1)` rounding (whose mask `~(a-1)` is just `-a`).
- **Fragmentation** — internal vs external, and the two levers (coalescing +
  splitting) that fight them.
- **Per-thread caches** — how thread-local free lists remove the lock from the hot
  path, and how a `pthread_key` destructor flushes them on thread exit.
- **LD_PRELOAD** — how exporting strong `malloc`/`free` symbols lets you replace an
  allocator process-wide with no recompile.

## Build & run (Linux / WSL)

This is **Linux-only** — it calls `sbrk` and `mmap`. On Windows use WSL. (The
teaching *assembly* in `asm/` regenerates on any host; see below.)

```bash
make                 # builds libmymalloc.so + bench_glibc + bench_mine
make test            # correctness smoke test + LD_PRELOAD a real program
make bench           # glibc vs ours (linked) vs ours (LD_PRELOAD), side by side
```

Use it on **any** program without recompiling it:

```bash
LD_PRELOAD=./libmymalloc.so ls
LD_PRELOAD=./libmymalloc.so bash -c 'echo hi'
```

Inspect what the benchmark does to the heap (it ends by walking every block and
printing `heap_check = 0`, i.e. all canaries and boundary tags agree):

```bash
./bench_mine 1
```

**Measured** (single thread, WSL2, gcc -O2; ops/sec, higher is better):

| workload | glibc | ours (linked) | notes |
|----------|------:|--------------:|-------|
| churn (random small) | ~52M | ~15M | segregated lists + split/coalesce |
| lifo  (stack-like)   | ~140M | ~40M | our tcache fast path vs glibc's |
| large (>128 KiB)     | very fast | ~160k | **we mmap+munmap every call**; glibc caches |

We are ~3x slower single-threaded, and much slower on the `large` pattern because
we return pages to the kernel immediately (honest, but `mmap`/`munmap` are pricey)
while glibc recycles them. That is the expected price of a simple, transparent
design; the point here is to *see* how an allocator works, not to beat ptmalloc.

## How it works

**`mymalloc.c`** — the allocator (heavily commented; read it top-to-bottom).

- *Block layout.* Every block is `[16-byte header][payload][8-byte footer]`. The
  header packs the total size (a multiple of 16, so the low 4 bits are flag bits:
  `ALLOC`, `MMAP`) plus a `magic` canary. The footer mirrors the size+flags so any
  block can find its physical predecessor at `(header - 8)` — that is the boundary
  tag. A free block reuses the first 16 bytes of its payload as `next`/`prev`
  free-list links, so segregation costs no extra space.
- *Size classes.* `bin_index()` maps a size to one of 16 geometric bins via
  `bsr` (bit-scan-reverse). `find_fit()` does first-fit starting at the exact bin
  and walking up.
- *Splitting & coalescing.* `place_and_split()` carves the tail off an over-large
  block when the leftover is itself usable (`>= MIN_BLOCK`). `heap_free_locked()`
  merges with a free predecessor and/or successor in O(1) using the tags.
- *Growth.* `extend_heap()` calls `sbrk` (aligning the first block to 16), grows in
  256 KiB chunks to amortise the syscall, and coalesces the new region onto the old
  top. `mmap_alloc()`/`mmap_free()` handle the `>= 128 KiB` path.
- *Corruption detection.* `check_header()` validates the magic on every `free`/
  `realloc`; `my_heap_check()` walks the whole heap verifying header==footer. The
  reporter uses raw `write(2)`, never `printf`, to avoid re-entering the allocator.
- *Per-thread cache.* `__thread` bins park recently-freed small blocks; `my_malloc`/
  `my_free` hit them with **no lock**. A `pthread_key` destructor flushes a dying
  thread's cache back to the global heap (verified: the 8-thread test ends with
  `blocks_live = 0`).
- *Interposers.* The bottom of the file exports `malloc`/`free`/`calloc`/`realloc`/
  `aligned_alloc`/`posix_memalign`/`malloc_usable_size` as strong symbols — that is
  what `LD_PRELOAD` overrides.

**`mymalloc.h`** — the thin public interface (both the `my_*` API and the standard
names) plus the `my_stats`/`my_heap_check` diagnostics.

**`bench.c`** — an allocator-agnostic harness (it only calls the standard names, so
the same source runs against glibc or us). Three patterns — `churn`, `lifo`,
`large` — timed with `clock_gettime(CLOCK_MONOTONIC)`, driven by an allocation-free
xorshift PRNG so runs are reproducible.

**`asm/demo.c`** — a self-contained extraction of the pure-logic core (no headers)
for the assembly deliverable.

## Assembly notes

The real allocator can't be compiled to assembly standalone (it needs the Linux
headers for `sbrk`/`mmap`/`pthread`), so `asm/demo.c` lifts out the part that is
100% register-and-pointer math — which is also the most instructive to read:
alignment rounding, the size-class bit trick, block splitting, and boundary-tag
coalescing. It is compiled with the repo's exact flags:

```bash
make asm    # regenerates asm/demo.{O0.s, s, O2.s} (needs clang; cross-targets Linux)
```

[`asm/demo.annotated.s`](asm/demo.annotated.s) is the hand-written, per-instruction
walkthrough of the `-O1` output. The highlights it calls out:

- **`align_up`** — the mask `~(a-1)` is emitted as a single `neg` (`-a == ~(a-1)`).
- **`bin_index`** — `__builtin_clzll` lowers to one `bsr`; and the optimizer *proves*
  the `if (idx < 0)` branch is dead (because the size is clamped to `>= 48`) and
  **deletes it** — a thing you can only see in the asm.
- **`split_block`** / **`coalesce_prev`** — coalescing is pure pointer arithmetic:
  forward via our own size, backward via the neighbour's footer at `(b - 8)`.

Compare [`asm/demo.O0.s`](asm/demo.O0.s) (naive, every value spilled to the stack)
with [`asm/demo.O2.s`](asm/demo.O2.s) (same logic, laid out for the branch predictor).

## Going further

The **`Stretch:`** goal for this project is the **per-thread cache**, which is
implemented here (`§11` of `mymalloc.c`) — thread-local magazines for small size
classes that serve the fast path with no global lock, flushed on thread exit. What
a *production* allocator adds beyond this core:

- **Multiple arenas** (glibc/ptmalloc) or **per-thread heaps** (mimalloc) so the
  central lock is rarely contended, not just bypassed for small sizes.
- **Page-run / slab management** (tcmalloc, jemalloc): carve pages into runs of one
  size class, so headers shrink toward zero and metadata is off to the side.
- **Returning memory to the OS** lazily with `madvise(MADV_FREE/MADV_DONTNEED)`
  and decay timers, instead of our immediate `munmap`.
- **Security hardening**: pointer-mangling of free-list links, guarded/randomized
  bins, `tcache` double-free detection (glibc's `key` field) — our single `magic`
  is only a first line of defense.
- **Footer elimination**: production allocators drop footers on *allocated* blocks
  by keeping a "previous is free" bit in the next header, halving our 24-byte
  overhead. We keep footers everywhere for clarity.

## References

- **CS:APP, ch. 9.9** — the boundary-tag / segregated-list allocator this is modeled
  on; the `malloclab` writeup is the canonical exercise.
- **`man 2 brk` / `man 2 sbrk` / `man 2 mmap` / `man 2 munmap`** — the syscalls.
- **glibc `malloc/malloc.c`** (ptmalloc2) — read `_int_malloc`, `_int_free`, and the
  `tcache` code to see the production version of every idea here.
- **`jemalloc`**, **`tcmalloc`**, **`mimalloc`** — the modern designs (arenas, slabs,
  radix trees, decay) named in "Going further".
- Doug Lea, *A Memory Allocator* (dlmalloc notes) — the classic writeup of boundary
  tags, bins, and the mmap threshold.
