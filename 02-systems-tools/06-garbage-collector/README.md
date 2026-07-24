# A garbage collector 🟥

**What it is.** A working **conservative mark-and-sweep** garbage collector for
Linux/x86-64, built the way Boehm's `libgc` is: you call `gc_malloc()` and simply
**never free**. When memory runs low, the collector finds every object still
reachable from the program's roots — the **CPU registers**, the whole **call
stack**, and the global **data/bss** — and reclaims the rest. It is *conservative*
because it does not know your types: it treats **any aligned machine word that
points into the heap as a live pointer**. That policy can never free something
still in use (the safety we want), at the price of occasionally keeping true
garbage alive. The heap is one contiguous `mmap` reservation committed on demand,
objects are tracked in an object table, marks live in a bit-per-granule bitmap,
and swept blocks go on a reuse free list so a program with a bounded live set uses
bounded memory. It is also `LD_PRELOAD`-able (`make preload`) so it can service a
whole program's `malloc`/`free`.

It is a **teaching core**, honestly scoped. It really works: it spills registers
with `setjmp`, scans the exact live stack range, scans data/bss, marks with an
explicit (non-recursive) mark stack, sweeps, reuses memory, collects
automatically under allocation pressure, and the bundled demo shows objects dying
on cue, a register-only root surviving, interior pointers keeping an object alive,
atomic blocks staying unscanned, and memory bounded under churn. What it
deliberately is **not**: precise, moving/compacting, generational, incremental, or
thread-aware. The [Going further](#going-further) section is explicit about the
gap and the stretch goal.

## What you'll learn

- **Conservative root scanning** — why you can collect garbage in an *uncooperative*
  language like C, with no type information, by treating pointer-shaped words as
  pointers; and the exact meaning of "conservative" (never free a live object;
  sometimes keep a dead one).
- **The register-spill trick** — using `setjmp(3)` to force the callee-saved
  registers (`rbx`, `r12`–`r15`, …) into a `jmp_buf` on the stack so a pointer that
  lives *only* in a register becomes a scannable root.
- **The stack-scanning trick** — reading `%rsp` for the current top, finding the
  stack **bottom** from `/proc/self/stat` field 28 (`startstack`), and treating
  every word in `[sp, bottom)` as a candidate.
- **`mmap`/`mprotect` page tricks** — reserving a large contiguous virtual window
  with `PROT_NONE` (no RAM cost) and **committing** it a chunk at a time with
  `mprotect`, so the heap's address range is fixed but its physical footprint grows
  on demand. A contiguous heap makes "is this a heap pointer?" a single compare.
- **A mark bitmap** — one bit per 16-byte granule, set with the classic
  `word[g>>6] |= 1<<(g&63)` (which clang lowers to `bts`), kept separate from the
  objects so marking never dirties payload cache lines.
- **Interior pointers** — a binary search of the sorted object table that keeps an
  object alive even when only a pointer *into the middle* of it exists.
- **Atomic (pointer-free) allocation** — `gc_malloc_atomic` for leaf data, which is
  faster (nothing to scan) and more precise (its bytes can never be mistaken for a
  pointer).
- **Automatic collection** — a Boehm-style trigger that collects once the heap has
  grown by about as much as is currently live, keeping GC cost proportional to
  live data.

## Build & run (Linux / WSL)

This is **Linux-only** — it calls `mmap`/`mprotect`, uses glibc's `setjmp`, and
reads `/proc/self/stat`. On Windows use WSL. (The teaching *assembly* under `asm/`
regenerates on any host; see below.)

```bash
make            # builds ./gctest (the demo, linked against the collector)
make run        # build and run the demo
make test       # run the demo and check it completes
make preload    # builds libgc.so, the LD_PRELOAD interposer
```

Expected demo output (addresses/exact counts vary, the shape does not):

```
[1] reachability (live root keeps it; dropped root frees it)
  [reach] built 1000-node list, kept root -> live objects = 1000
  [reach] walked survivors: count=1000 sum=499500 (expect 1000, 499500)
  [reach] dropped root + collected: live objects 1000 -> 0, reclaimed ... bytes
[2] register/stack-only root
  [stack] object rooted only in a local survived; value=0xBEEF
[3] interior pointer keeps the whole object alive
  [interior] kept &arr[50]; arr[50]=1050 arr[0-via-mid]=1000 arr[99]=1099
[4] atomic (pointer-free) blocks are not scanned
  [atomic] address hidden in an atomic block is NOT a root: live objects N -> N-1 ...
[5] bounded memory under allocation churn
  [churn] allocated 30000000+ bytes total; committed only <a few MiB> bytes; K collections
```

Drive an arbitrary program's allocator through the collector (best-effort — see
the honesty note):

```bash
LD_PRELOAD=./libgc.so ./some_program
```

## How it works

**`gc.c`** — the collector (heavily commented; read it top to bottom). The section
numbers below match the section banners in the file.

- *Heap layout (§ intro, §6).* One contiguous window is `mmap`'d **`PROT_NONE`** at
  startup — addresses reserved, no pages, no RAM. As the bump pointer advances,
  `commit_at_least()` turns the next chunk readable/writable with **`mprotect`**.
  Because the heap is one range, `in_heap(w)` is just `heap_lo <= w < alloc_top`.
- *Object table (§7).* One `{base, size, atomic}` descriptor per live object, in an
  `mmap`-backed array grown with **`mremap`** (never libc `malloc` — that would
  recurse under `LD_PRELOAD`). Sorted by base at the start of each collection with
  an in-place, non-recursive **heapsort**, so `obj_containing()` is a binary search
  that also resolves **interior pointers**.
- *Mark bitmap (§5).* One bit per 16-byte granule; `mark_set`/`mark_test` are the
  `g>>6` / `g&63` bit idiom. Marks stay out of the objects entirely.
- *Free list (§8).* Swept blocks are pushed intrusively (in their own first 16
  bytes) and reused first-fit by later `gc_malloc`, so churn stays bounded.
- *Root scanning (§10–§12).* `gc_collect()` reads `%rsp`, spills registers with
  `setjmp`, then `scan_range()`s three regions — the `jmp_buf`, the stack
  `[sp, bottom)`, and `[__data_start, _end)` — treating every 8-byte word as a
  candidate through `mark_word()`.
- *Marking (§10).* `mark_word()` gates on the heap range, locates the containing
  object, tests/sets its bit, and pushes it on an **explicit mark stack**;
  `mark_drain()` pops objects and scans their interiors transitively. The stack is
  explicit (not recursion) precisely so a long pointer chain cannot overflow the C
  stack — a collector must never crash its host.
- *Sweep (§12).* Walk the table; unmarked objects go to the free list, survivors
  are compacted forward. Mark bits are wiped in bulk at the *start* of the next
  cycle, not per-object here.
- *Auto-collection (§13).* `gc_malloc` collects before carving a block once
  `bytes_since_gc` crosses a threshold retuned to `2 × live` each cycle.

**`gc.h`** — the thin public API: `gc_init`, `gc_malloc`, `gc_malloc_atomic`,
`gc_collect`, and `gc_get_stats`/`gc_dump`.

**`gctest.c`** — the demo. It uses the `gc_*` API directly and libc `printf`
freely (it is the *application*). Its one subtlety is `scrub_stack()`: before any
collection that is *supposed* to reclaim, it overwrites ~16 KiB of stack to erase
stale pointer bit-patterns left by returned frames — the flip side of
conservatism, made visible.

**`asm/demo.c`** — a self-contained extraction (no headers) of the pointer-candidate
test and mark-bitmap logic, for the assembly deliverable.

### Why conservative — and why it can keep garbage

We have no type map for C stack frames or structs, so we cannot know which words
are pointers. The conservative bet is: **if a word, read as an address, lands
inside a live object, assume it is a pointer to that object and keep the object.**
This is always *safe for correctness* — every real pointer is a word that lands in
its object, so no reachable object is ever freed. The cost is **false retention**:
a `long` holding, say, `0x00007f2ac0001040` that merely *looks* like a heap
address will pin whatever object contains that address until the deceptive value
changes. In practice this is rare and bounded, which is why conservative GC is
viable for real C/C++ programs. `gc_malloc_atomic` shrinks the risk further:
pointer-free blocks are never scanned, so random bytes in a string can never
resurrect anything (demo test 4 proves it).

### The register/stack-scanning trick, precisely

A root can hide in three places when `gc_collect()` runs:

1. **On the stack**, in some caller's frame — covered by scanning `[sp, bottom)`.
2. **In a caller-saved register** the compiler was using — the SysV ABI forced the
   caller to spill those to *its* stack frame before it `call`ed `gc_collect`, so
   they are already in `[sp, bottom)` too.
3. **In a callee-saved register** (`rbx`, `r12`–`r15`) the compiler kept a pointer
   in across the call — these are *not* on the stack. `setjmp(regs)` writes exactly
   these into the `jmp_buf`, which we then scan.

So after the `setjmp` spill, **every** register/stack root is somewhere in the
three ranges we scan. (glibc mangles the saved `rsp`/`rip` slots with a pointer
guard, so those read as garbage and simply fail the range test — harmless.) The
stack *bottom* — the highest address, since the stack grows down — is read once at
`gc_init()` from `/proc/self/stat`.

## Assembly notes

The real collector can't compile to assembly standalone (it needs the Linux
headers for `mmap`/`mprotect`/`setjmp`), so `asm/demo.c` lifts out the part that is
100% register-and-pointer math — which is also its hot path. It is compiled with
the repo's exact flags:

```bash
make asm     # regenerates asm/demo.{O0.s, s, O2.s} (needs clang; cross-targets Linux)
```

[`asm/demo.annotated.s`](asm/demo.annotated.s) is the hand-written, per-instruction
walkthrough of the `-O1` output. What it highlights for this project:

- **The pointer test is astonishingly cheap.** `in_heap` is two compares and an
  `and`; it rejects nearly every stack/register word before the expensive part.
- **A mark bit is one instruction.** clang lowers the bitmap *test* to `btq` and the
  *set* to `btsq` — it chose the x86 bit-string ops over shift+mask on its own.
- **Interior pointers cost nothing extra.** The same binary search that finds an
  exact base also finds the object a mid-pointer lands inside (`w < base+size`),
  with `lea (%r,%r,2)` + scale-8 addressing the 24-byte descriptors.
- **Shrink-wrapping.** `mark_word` runs *frameless* on every fast path (a bare
  `retq`) and only builds a stack frame on the rare "newly marked" path that must
  read its 7th argument off the stack — a decision invisible in the C.

Compare [`asm/demo.O0.s`](asm/demo.O0.s) (naive, every value spilled to the stack)
with [`asm/demo.O2.s`](asm/demo.O2.s) (same logic, scheduled for the branch
predictor). The logic in `asm/demo.c` is exercised by an executable test during
development, so the annotated instructions correspond to code proven correct.

## Going further

The **`Stretch:`** goal for this project is a **precise, moving, generational GC
for a toy language** — the opposite end of the design space from this conservative
collector. What that buys, and what production collectors add beyond this core:

- **Precise** (exact) GC: the compiler emits *stack maps* and *object layout maps*
  so the collector knows exactly which slots are pointers. No false retention, and
  it becomes safe to **move** objects.
- **Moving / compacting**: with precise roots you can relocate live objects and
  update every pointer to them, eliminating fragmentation and enabling **bump
  allocation** (allocation becomes "increment a pointer"). Conservative GC cannot
  move, because it might "update" an integer it only *guessed* was a pointer.
- **Generational**: most objects die young, so collect a small **nursery** often
  and the **old** generation rarely. This needs a **write barrier** to record
  old→young pointers (a *remembered set*), so a minor collection can scan only the
  nursery plus that set instead of the whole heap.
- **Incremental / concurrent**: interleave marking with the mutator using
  read/write barriers (tri-color invariant) so pauses stay short — what a language
  runtime (Go, the JVM, .NET) actually ships.
- **Thread-aware**: stop every mutator thread and scan *each* thread's registers and
  stack. This core scans only the main thread; a real collector walks all of them.

This core's own simplifications, stated honestly: no splitting on the reuse free
list (some internal fragmentation); an `O(n log n)` sort + binary-search object
lookup where production uses an `O(1)` page/block table (Boehm's `HDR` scheme); the
`LD_PRELOAD` build scans only the main executable's `data/bss` (not other shared
libraries' roots) and is a demonstration, not a drop-in for arbitrary programs;
and the `/proc/self/stat` stack-bottom lookup is main-thread only.

## References

- **Boehm & Weiser, "Garbage Collection in an Uncooperative Environment"** (SP&E
  1988) — the founding paper for conservative mark-and-sweep; the design here is a
  small, readable descendant.
- **The Boehm–Demers–Weiser GC** (`bdwgc`) — read `mark.c`, `mark_rts.c` (root
  scanning), `os_dep.c` (`GC_get_stack_base`), and `os_dep`'s `PUSH_REGS`/`setjmp`
  for the production version of every trick in this file.
- **Jones, Hosking & Moss, *The Garbage Collection Handbook*** — the standard text
  for mark-sweep, generational, and moving collectors (the stretch goal).
- **`man 2 mmap` / `man 2 mprotect` / `man 3 setjmp` / `man 5 proc`** — the exact
  syscalls and the `/proc/self/stat` field list (field 28 = `startstack`).
- **System V AMD64 ABI** — which registers are callee-saved (why `setjmp` catches
  the roots it does) and the argument/stack conventions the annotated asm relies on.
