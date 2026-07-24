# mmake — a make(1) clone with a POSIX jobserver 🟧

**What it is.** A working build tool. It parses a Makefile-like format (variables,
rules, prerequisites, recipes), builds the dependency **DAG**, decides what is out
of date by comparing file **mtimes**, and runs the stale targets' recipes in
**dependency order** — sequentially, or up to *N* at a time in **parallel** using
`fork(2)`/`execve(2)` and a **POSIX jobserver** (a pipe of tokens), with correct
child reaping and signal handling. It is a genuine *teaching core*: it really
builds real dependency graphs in parallel. The subset it implements — and the
GNU-make features it deliberately leaves out — are listed under *Scope* below.

**Platform: Linux / WSL.** The engine uses `fork`, `execve`, `pipe2`, `stat`,
`waitpid`, `sigaction`, and `killpg`. It does **not** build on native Windows.
The committed teaching assembly under `asm/` regenerates on any host (clang
cross-targets Linux).

---

## What you'll learn

- **`stat(2)`** and the staleness rule at the heart of every build system:
  fold `st_mtim` (seconds + nanoseconds) into one comparable integer, then a
  target is stale iff it is missing or older than a prerequisite.
- **DAG algorithms**: topological ordering via **Kahn's algorithm** run *live*
  as a parallel scheduler (in-degree = "unbuilt prerequisites", decremented as
  jobs finish), and **cycle detection** with a three-color DFS.
- **Process fan-out**: `fork(2)` + `execve(2)` to run recipes under `/bin/sh -c`,
  two fork levels deep (a job child per target, a shell per recipe line), and
  `waitpid(2)` reaping with `WIFEXITED`/`WIFSIGNALED`.
- **The POSIX jobserver protocol**: representing a global `-jN` budget as bytes
  ("tokens") in a **pipe**, plus the always-available *implicit* token that keeps
  the scheme deadlock-free, and passing the pipe fds to sub-makes via `MAKEFLAGS`.
- **Signal handling done right**: `sigaction` without `SA_RESTART` so a blocking
  `waitpid` is interrupted; a `volatile sig_atomic_t` flag; process groups so one
  `killpg` tears down a whole recipe subtree on Ctrl-C.
- In the assembly: how the **SysV AMD64 ABI** passes a 7th argument on the
  **stack**, why an mtime compare is a **signed** `cmpq`/`jg`, and how the same
  `int`/`long long` arrays are indexed with **scale 4 vs 8**.

---

## Build & run (Linux / WSL)

```bash
make                      # builds ./mmake  (-Wall -Wextra, C11)
make test                 # builds, then runs a real parallel build in examples/
```

`make test` runs three scenarios against `examples/Makefile` (which uses only
shell built-ins, so no compiler is needed):

```bash
./mmake -C examples -j3 all     # 1st build: a.o/b.o/c.o compile in PARALLEL, then link
./mmake -C examples    all      # 2nd build: everything up to date, nothing runs
touch examples/b.src
./mmake -C examples -j3 all     # only b.o and app rebuild (staleness propagation)
```

Useful flags (a practical subset of make's):

```
-f FILE   use FILE instead of ./Makefile        -n   dry run (print, don't execute)
-jN       run up to N recipes in parallel        -s   silent (don't echo recipes)
-k        keep going after a failed target       -B   force everything out of date
-q        question: exit 1 iff a goal is stale   -C DIR  chdir before building
VAR=val   command-line variable override         target ...  goals (default: first rule)
```

---

## How it works

A tour, in pipeline order:

- **`mk.h`** — the data model and the map of the whole program: `variable`,
  `rule`, the graph `node` (with forward *and* reverse edges), the `jobserver`,
  and the `mk` context. Read it first.
- **`util.c`** — die-on-OOM allocators (so the algorithmic code never null-checks),
  a growable `strbuf`, and `read_file` (a `open`/`fstat`/`read` slurp that handles
  short reads and `EINTR`).
- **`parse.c`** — the reader. Assembles logical lines (gluing `\`-continuations),
  distinguishes a **TAB-indented recipe** line from an ordinary one, tells a
  `VAR := val` assignment from a `target: prereq` rule, and performs `$()`/`${}`
  **variable expansion** — including the deferred (`=`) vs immediate (`:=`)
  flavors and the automatic variables `$@`, `$<`, `$^`.
- **`graph.c`** — turns rules into a `node` graph in two phases (create all nodes,
  *then* wire edges, so a table realloc can't dangle a pointer), runs the
  **cycle-detection DFS**, `stat(2)`s every file node, and computes **staleness**
  bottom-up so a rebuilt prerequisite forces its dependents to rebuild too.
- **`jobserver.c`** — the token pipe. Creates the pool (`jobs-1` byte tokens +
  one implicit slot), acquires/releases tokens with non-blocking `read`/`write`,
  and attaches to an inherited pool via `MAKEFLAGS` for recursive sub-makes.
- **`job.c`** — the scheduler. Kahn's algorithm run live: launch every READY node
  for which a token is free, `fork` a job child per target (its own process
  group), reap finished children with `waitpid`, return their tokens, and wake
  dependents whose in-degree just hit zero. Handles `-n`, `-s`, `-k`, `-B`, the
  `@`/`-`/`+` recipe prefixes, and SIGINT/SIGTERM teardown.
- **`main.c`** — argument parsing, makefile discovery (`Makefile`, then
  `makefile`), command-line variable overrides, default-goal selection, and the
  per-goal drive loop. Also `mk_free` (the ownership map for teardown).

### Scope — what this core does and does not do

**Implemented:** `=`, `:=`, `?=`, `+=` variables and `$()/${}/$X/$$` expansion;
`$@ $< $^`; multiple targets and prerequisites per rule; `.PHONY`; recipe
prefixes `@` (silent), `-` (ignore error), `+` (always); `\` continuations;
`# comments`; parallel `-jN` with a real jobserver pipe; staleness by mtime;
cycle detection; `-n -s -k -B -q -C -f` and `VAR=val` overrides.

**Deliberately omitted** (so the core stays readable): pattern/implicit rules
(`%.o: %.c`) and built-in rules; make functions (`$(wildcard)`, `$(patsubst)`,
`$(shell)`, …); conditionals (`ifeq`/`ifdef`) and `include`; target-specific and
automatic-per-target variables beyond `$@ $< $^`; `.ONESHELL`; and the modern
named-pipe (`fifo:`) jobserver. These are noted where the code would grow to
handle them.

---

## Assembly notes

`asm/demo.c` is a **self-contained** extraction (no system headers, its own
types) of the two decisions mmake makes that are interesting to the *CPU* rather
than the *kernel*, lifted from `graph.c`:

1. **`mk_needs_rebuild`** — the staleness test. It is the annotated **star**: a
   leaf function with **seven** integer parameters, so it is the clearest place
   in the lab to watch the SysV AMD64 ABI put the 7th argument (`nprereq`) on the
   **stack** and read it back via `16(%rbp)`. It also shows a **signed** 64-bit
   mtime compare (`cmpq`/`jg`, because the "missing" sentinel is `-1`) and two
   prerequisite arrays indexed at different **scales** (4 for `int`, 8 for
   `long long`).
2. **`mk_toposort`** — Kahn's algorithm over an adjacency matrix. Not a leaf (the
   compiler clears an array with `memset`), so it demonstrates parking live
   values in **callee-saved** registers and padding the frame `128 → 136` to keep
   `rsp` 16-byte aligned at the call.

The committed `.s` files are **genuine clang 20.1.8 output**, produced by:

```bash
make asm      # runs the three exact clang --target=x86_64-pc-linux-gnu -S commands
```

- [`asm/demo.O0.s`](asm/demo.O0.s) — naive, every value spilled to the stack.
- [`asm/demo.s`](asm/demo.s) — `-O1`, the baseline that is annotated.
- [`asm/demo.O2.s`](asm/demo.O2.s) — `-O2`, heavier scheduling.
- [`asm/demo.annotated.s`](asm/demo.annotated.s) — the hand-written, per-instruction
  walkthrough (this is the one to read).

> Note: the real translation units (`parse.c`, `graph.c`, `job.c`, …) need Linux
> libc headers that a cross-targeting clang may not find on a non-Linux host, so
> — per the lab convention — the didactic assembly is taken from the header-free
> `asm/demo.c` extraction instead.

---

## Going further

- **Recursive sub-makes** already share the token pool: `jobserver_export` writes
  `--jobserver-auth=R,W` into `MAKEFLAGS`, and `jobserver_init` reattaches to it.
  The remaining work is what real make does for the multi-reader race: **blocking**
  token reads guarded by `poll`/`pselect`, and returning the implicit token while
  blocked so the pool never deadlocks. See the header note in `jobserver.c`.
- **Pattern rules and a `$(wildcard)`/`$(shell)` function layer** would turn this
  from "run the recipes I wrote" into "infer how to build `%.o` from `%.c`". That
  is mostly a parser and an implicit-rule search on top of the same graph.
- **What production does:** GNU make keeps a hash table of variables and files,
  supports second-expansion, order-only prerequisites (`|`), directory-change
  bookkeeping, and the fifo jobserver; `ninja` drops make's language entirely and
  reads a pre-computed build graph for speed. Both still come down to the same
  three primitives this project implements: *stat for staleness, topo-order the
  DAG, fan out under a job budget.*

---

## References

- **POSIX jobserver**: the GNU make manual, "Communicating Options to a
  Sub-`make`" and "POSIX Jobserver Interaction"; make's `src/posixos.c`.
- `man 2 stat`, `man 2 fork`, `man 2 execve`, `man 2 waitpid`, `man 2 pipe2`,
  `man 2 sigaction`, `man 7 signal-safety` (why only `sig_atomic_t` in a handler).
- **Kahn's algorithm** (topological sort) and three-color DFS cycle detection —
  any algorithms text (CLRS, "Topological Sort").
- GNU make source: `src/read.c` (parsing), `src/remake.c` (staleness), `src/job.c`
  (the scheduler and jobserver) — "read the source of the real thing."
- System V AMD64 ABI (the psABI document) — ground truth for the register/stack
  argument passing dissected in `asm/demo.annotated.s`.
