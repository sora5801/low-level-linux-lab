# A language + runtime (CAPSTONE) 🟥

**What it is.** A complete small language runtime for a dynamically-typed language
called **Lumen** (`.lum` files), built by stitching together six sibling projects
from this lab into one program you can compile and run. The teaching core is a
genuinely working interpreter: a hand-written **lexer**, a single-pass **Pratt
compiler** that emits **bytecode**, and a **stack VM** with **computed-goto
dispatch**, sitting on top of its own **mark-sweep garbage collector** and a
custom **bump / segregated-free-list allocator**. Two further subsystems are
demonstrated as runnable add-ons: an **x86-64 JIT** that compiles a hot loop to
native code in a `PROT_EXEC` page, and cooperative **green threads** (coroutines)
that switch stacks in userspace. An **io_uring** async-I/O runtime is documented
as the one add-on that ships as a design sketch rather than compiled code.

This is a 🟥 giant. It ships a **genuinely working teaching core end to end** —
`make run` compiles and executes real programs with functions, recursion, loops,
strings, and automatic memory management. Read [Scope](#scope-what-the-core-runs-vs-what-a-full-system-needs)
for an honest account of what the core runs versus what a production runtime adds.

---

## Architecture

```
  SOURCE (.lum)
      │
      ▼
 ┌──────────┐   tokens    ┌──────────────┐   ObjFunction   ┌──────────────────┐
 │ scanner  │────────────▶│  compiler    │────(bytecode)──▶│   bytecode VM    │
 │ (lexer)  │  on demand  │ (Pratt parse │                 │  computed-goto   │
 └──────────┘             │  → bytecode) │                 │  dispatch loop   │
                          └──────────────┘                 └───────┬──────────┘
                                                                   │ allocates
                                            values / call frames   │ objects
                                          ┌────────────────────────┼────────────────────┐
                                          ▼                        ▼                     │
                                 ┌─────────────────┐      ┌──────────────────┐           │
                                 │ mark-sweep GC   │◀────▶│ heap allocator   │           │
                                 │ (gc.c)          │ free │ bump + freelist  │           │
                                 │ roots: stack,   │ list │ (heap.c, mmap)   │           │
                                 │ frames, globals │      └──────────────────┘           │
                                 └─────────────────┘                                     │
                                                                                         │
        ADD-ONS (demonstrated on the side, wired loosely to the language):               │
        ┌───────────────────────────┐   ┌──────────────────────────┐   ┌────────────────▼──────────┐
        │ JIT tier (jit.c)          │   │ green threads (sched.c)   │   │ async I/O (io_uring)      │
        │ hot loop → x86-64 in a    │   │ ucontext coroutines on    │   │ documented add-on:        │
        │ PROT_EXEC page (W^X)      │   │ guarded mmap stacks       │   │ SQ/CQ rings, epoll fallbk │
        └───────────────────────────┘   └──────────────────────────┘   └───────────────────────────┘
```

Each subsystem is a compact re-implementation of a full sibling project. Read the
sibling for the deep version; read the file here for how it plugs into a runtime:

| Subsystem              | This capstone         | Implemented in depth by the sibling project |
|------------------------|-----------------------|---------------------------------------------|
| Lexer + Pratt compiler + bytecode VM (computed goto) | `src/scanner.c`, `src/compiler.c`, `src/vm.c`, `src/chunk.c` | [`../../02-systems-tools/15-language-vm`](../../02-systems-tools/15-language-vm) |
| JIT: hot function → native x86-64 in `PROT_EXEC` memory | `src/jit.c` | [`../../02-systems-tools/12-jit-compiler`](../../02-systems-tools/12-jit-compiler) |
| Mark-sweep garbage collector | `src/gc.c` | [`../../02-systems-tools/06-garbage-collector`](../../02-systems-tools/06-garbage-collector) |
| Bump / segregated-free-list allocator (`mmap` arenas) | `src/heap.c` | [`../../02-systems-tools/05-malloc`](../../02-systems-tools/05-malloc) |
| Green threads / coroutines (userspace stack switch) | `src/sched.c` | [`../../02-systems-tools/18-green-threads`](../../02-systems-tools/18-green-threads) |
| Async I/O runtime on io_uring (documented add-on) | *sketch below* | [`../../03-networking/05-io-uring-server`](../../03-networking/05-io-uring-server) |

---

## What you'll learn

- **A whole front end with no AST:** maximal-munch lexing, **Pratt (precedence-
  climbing) parsing** driven by one rule table, resolving locals to **stack slots**
  at compile time, and **backpatching** forward jumps (`compiler.c`).
- **A stack machine:** why `1 + 2 * 3` needs no registers, and how a function call
  becomes a **frame** — a window carved out of one shared value stack, so a call
  allocates nothing (`vm.c`).
- **Computed-goto dispatch:** the single most performance-relevant trick in a
  bytecode interpreter, and *why* one `goto *table[op]` per handler beats a shared
  `switch` for branch prediction — visible in `asm/demo.annotated.s`.
- **A precise mark-sweep GC:** the tri-color invariant, an **explicit gray
  worklist** (no recursion), the exact **root set** (value stack, call frames,
  globals), and the allocate-then-root discipline that keeps a half-built object
  from being collected mid-flight (`gc.c`).
- **A GC-backed allocator:** getting pages from the kernel with **`mmap`**, bump
  allocation, and O(1) reuse of swept blocks via **size-classed free lists**
  (`heap.c`).
- **Runtime code generation:** hand-emitting **x86-64 machine-code bytes** (REX /
  opcode / ModRM), `mmap` + `mprotect` and the **W^X** discipline, and calling
  generated code through a function pointer (`jit.c`).
- **Cooperative concurrency:** a userspace **context switch** with `ucontext`, an
  `mmap`'d stack with a `PROT_NONE` **guard page**, and a round-robin scheduler
  with `spawn`/`yield` — no kernel involvement per switch (`sched.c`).

---

## Build & run (Linux / WSL)

The **runtime is Linux/x86-64** — it uses `mmap`/`mprotect` (GC heap, JIT page)
and `<ucontext.h>` (green threads). It builds on Linux and WSL. `make asm` works
on **any** host (clang cross-targets Linux).

```bash
make run           # build ./lumen, then run examples/demo.lum
make repl          # interactive REPL
make jit           # JIT-compile sum(1..100) to native x86-64 and run it
make coro          # two cooperative green threads interleaving
./lumen examples/fib.lum

make debug         # per-instruction execution trace + code disassembly
make gc-test       # collect on EVERY allocation — a missing GC root crashes here
```

Expected tail of `make run` (numbers are exact; addresses/timings vary):

```
== recursion: fib(10) ==
55
== iteration: the hot loop ... ==
5050
...
== done ==
```

`make jit` prints the emitted bytes and checks the native result:

```
jit: emitted 18 bytes of x86-64 for sum(1..100):
  31 c0 31 c9 48 39 f9 7d 08 48 ff c1 48 01 c8 eb f3 c3
jit: native result = 5050,  reference = 5050  [OK]
```

## The language, in one screen

```
// examples/demo.lum (excerpt)
var a = 10;                 // globals and lexically-scoped locals
fun fib(n) {                // first-class functions + recursion
  if (n < 2) return n;
  return fib(n - 1) + fib(n - 2);
}
print fib(10);              // 55

fun repeat(s, n) {          // strings; each `+` allocates a new object
  var out = "";
  var i = 0;
  while (i < n) { out = out + s; i = i + 1; }   // garbage for the GC to reclaim
  return out;
}
print repeat("ab", 6);      // abababababab
```

Grammar in brief: numbers (IEEE doubles), booleans, `nil`, strings; `+ - * /`,
comparisons, `and`/`or`/`!`; `var`, assignment, blocks and scoping; `if`/`else`,
`while`, `for`; `fun`/`return` with recursion; `print`; a native `clock()`.

---

## How it works (a tour of the code)

Two memory domains run throughout (see `common.h`): **GC-managed objects**
(strings, functions) go through `heap.c`/`gc.c`; **VM-internal growable arrays**
(bytecode, line table, constant pool, hash-table entries) use plain libc realloc.
Keeping them separate is what keeps the GC's root set tiny and legible.

- **`common.h`** — shared config switches and the checked `GROW_ARRAY`/`FREE_ARRAY`
  helpers for VM-internal arrays.
- **`value.h` / `value.c`** — the 16-byte tagged-union `Value`; printing and `==`
  (strings compare by **content**, not identity — see the table note below).
- **`object.h` / `object.c`** — heap objects (`ObjString` with a **flexible array
  member**, `ObjFunction`, `ObjNative`) and their single allocation choke point.
- **`chunk.h` / `chunk.c`** — a `Chunk`: the bytecode stream, a parallel line
  table, and the constant pool. The `OpCode` enum is the instruction set.
- **`scanner.h` / `scanner.c`** — the pull-based lexer: one cursor, maximal munch,
  a keyword trie; tokens point into the source and own nothing.
- **`compiler.h` / `compiler.c`** — the Pratt parser/compiler. One rule table +
  `parsePrecedence()`; locals resolved to slots; jumps backpatched. Emits an
  `ObjFunction`. (Compiles with the GC disabled — see Scope.)
- **`table.h` / `table.c`** — open-addressing hash table (linear probing +
  tombstones) for globals. **Keys compare by content**, so — unlike the sibling
  VM — we need **no string interning and thus no weak references in the GC**. That
  trade (a `memcmp` on a hash hit, versus a whole class of GC bookkeeping) is the
  most consequential simplification in the project.
- **`heap.h` / `heap.c`** — arenas from `mmap`, bump allocation, size-classed free
  lists, and the GC trigger (`bytesAllocated` crossing `nextGC`).
- **`gc.h` / `gc.c`** — tri-color mark-sweep with an explicit gray stack. Roots are
  the value stack, each call frame's function, and the globals table.
- **`vm.h` / `vm.c`** — the interpreter: value stack, call frames, and the
  computed-goto dispatch loop (with a portable `switch` fallback). Also the
  disassembler used by the debug builds.
- **`jit.h` / `jit.c`** — the JIT add-on (below).
- **`sched.h` / `sched.c`** — the green-thread add-on (below).
- **`main.c`** — CLI: run a file, REPL, `--jit-demo [n]`, `--coro-demo`.

### The JIT tier (`jit.c`)

`jitDemo(n)` emits the exact 18 machine-code bytes for `sum(1..n)` (arg in `%rdi`,
result in `%rax`), `mmap`s an RW page, copies the bytes in, `mprotect`s it to R+X
(**never writable-and-executable at once**), calls it through a function pointer,
and checks the result against a reference C loop. Every byte is explained in the
file header (REX.W `0x48`, the `cmp`/`inc`/`add` ModRM bytes, the `rel8`
displacements). It is a *complete, correct* piece of runtime codegen; wiring it
into the VM as a real tier is the [full-system](#scope-what-the-core-runs-vs-what-a-full-system-needs) work.

### Green threads (`sched.c`)

`coroDemo()` spawns two tasks that hand control back and forth with `gtYield()`,
driven by a round-robin scheduler in **one OS thread**. Each task has its own
`mmap`'d stack with a `PROT_NONE` **guard page** at the low end (stacks grow down,
so overflow faults immediately). Switching is `swapcontext()` — userspace only, no
syscall. Because scheduling is cooperative and single-threaded, there is **no
preemption and therefore no data race** on the shared queue between yields; that
is the whole concurrency model, and why it needs no locks.

### Async I/O on io_uring (documented add-on)

This is the one subsystem shipped as a design sketch, not compiled code, so the
build stays dependency-free. In the full runtime, a blocking language call like
`read(fd)` would suspend the current green thread and register the I/O with the
kernel, resuming the coroutine when the completion arrives — turning the
cooperative scheduler into an async runtime:

```c
// sketch — see ../../03-networking/05-io-uring-server for the real ring code.
struct io_uring ring;
io_uring_queue_init(QD, &ring, 0);
struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
io_uring_prep_read(sqe, fd, buf, len, 0);
io_uring_sqe_set_data(sqe, current_task);   // remember who to resume
io_uring_submit(&ring);
gtYield();                                   // scheduler runs other tasks…
// scheduler's event loop reaps completions and re-enqueues the tagged task:
struct io_uring_cqe *cqe;
io_uring_wait_cqe(&ring, &cqe);
Task *t = io_uring_cqe_get_data(cqe);
t->io_result = cqe->res;  enqueue(t);  io_uring_cqe_seen(&ring, cqe);
```

The **epoll fallback** (for kernels < 5.1) is the same idea with readiness rather
than completion: arm `EPOLLIN`, yield, and resume the task when the fd is ready.
The sibling project ships both an io_uring and an epoll server speaking the same
protocol, and measures the difference that matters: **syscalls per message.**

---

## Assembly notes

Per repo convention, the assembly deliverable is a **self-contained extraction**
of the project's most instructive pure-logic routine: the **bytecode dispatch
core**. `asm/demo.c` is a miniature stack VM (own types, no system headers) that
runs a hand-assembled bytecode program computing `sum(1..10) = 55` using the *same
computed-goto dispatch* as `src/vm.c`. It is self-contained because the real
`.c` files pull in `<stdio.h>`/`<sys/mman.h>`/`<ucontext.h>`, whose Linux headers
aren't present when cross-targeting Linux from another host.

[`asm/demo.annotated.s`](asm/demo.annotated.s) is the hand-commented `-O1` output.
The lesson is right there in the machine code: **every opcode handler ends with
its own `jmpq *(%rcx,%rsi,8)`** — one indirect-branch site per opcode, which is
the entire performance argument for computed-goto dispatch. It also shows the hot
interpreter state (`ip`, `sp`, the code pointer, the table base) pinned in
registers for the whole loop, and the signed `rel8` branch operands that implement
`while`/`for` back-edges — the exact bytes `compiler.c` backpatches. Regenerate
with `make asm`:

- [`asm/demo.O0.s`](asm/demo.O0.s) — the naive mapping (every value spilled).
- [`asm/demo.s`](asm/demo.s) — `-O1`, the annotated baseline.
- [`asm/demo.O2.s`](asm/demo.O2.s) — `-O2`, the tightened loop.

---

## Scope: what the core runs vs. what a full system needs

**The teaching core really runs**, end to end: lex → compile → bytecode → execute,
with globals and lexically-scoped locals, all the arithmetic/comparison/logical
operators, `if`/`while`/`for`, first-class functions with recursion and real call
frames, strings with concatenation, a native `clock()`, and an integrated
mark-sweep GC over a custom `mmap`-backed allocator. The JIT and green-thread
add-ons run via `--jit-demo` / `--coro-demo`.

**Deliberately omitted so the core stays legible** (each is the sibling's or a
"full system's" job):

- **Closures / upvalues.** Functions are first-class values but do **not** capture
  their enclosing scope. This removes `ObjClosure`/`ObjUpvalue` and their GC edges.
  (The sibling `15-language-vm` shows closures.)
- **JIT integration.** `jit.c` compiles and runs a fixed hot kernel correctly, but
  the VM does not yet detect hot functions, generate their bytecode to native, and
  patch call sites. That tiering (plus deopt guards) is the real JIT engineering.
- **io_uring is a sketch,** not compiled code (see above), and green threads are
  not yet suspended on real I/O.
- **GC sophistication.** The collector is stop-the-world, non-moving, non-
  generational, and single-threaded. No compaction, no write barriers, no
  incremental marking.
- **Allocator sophistication.** No coalescing, no per-thread caches, no `MADV_FREE`
  decay; retired-arena tail space is abandoned. (The sibling `05-malloc` has these.)
- **Value packing.** `Value` is a 16-byte struct for clarity; production VMs
  NaN-box it into 8 bytes.
- **Preemption.** Green threads are cooperative; a task that never yields starves
  the rest. No timer-driven preemption, no work stealing across OS threads.

Every one of these is called out at its site in the source as well.

---

## Going further (the `Stretch:` goal)

- **Make the JIT a real tier.** Add a call counter to `ObjFunction`; when it
  crosses a threshold, translate the bytecode to native (start with the
  arithmetic/branch ops), install it, and add a guard that falls back to the
  interpreter on type mismatch. `jit.c` already has the emit-and-execute lifecycle.
- **Suspend green threads on io_uring.** Give each `Task` an `io_result` field,
  tag SQEs with the task pointer, and have the scheduler's loop reap CQEs and
  re-enqueue tagged tasks — the sketch above made concrete.
- **Add closures**, then **NaN-box `Value`**, then make the GC **generational**.
- **Read the real things:** CPython's `ceval.c` (computed goto), LuaJIT (tracing
  JIT + NaN boxing), V8's Orinoco GC, Go's goroutine scheduler, `tokio`/`libuv`
  (async runtimes over io_uring/epoll).

## References

- Bob Nystrom, *Crafting Interpreters* — the clox architecture this core is a
  re-commented, capstone-integrated relative of.
- System V AMD64 ABI (registers, stack alignment, the red zone) — for `jit.c`,
  `sched.c`, and `asm/demo.annotated.s`.
- `man 2 mmap`, `man 2 mprotect`, `man 3 makecontext`/`swapcontext`,
  `man 7 io_uring`.
- The six sibling projects linked in the [Architecture](#architecture) table —
  each is the deep dive for one box in the diagram.
