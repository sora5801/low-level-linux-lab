# A language VM 🟥

**What it is.** A complete bytecode virtual machine for a small dynamically-typed
language, built the way real ones are: a hand-written **lexer**, a single-pass
**Pratt (precedence-climbing) parser/compiler** that emits **bytecode**, and a
**stack-based interpreter** whose instruction dispatch uses **computed goto**
(the GNU "labels as values" extension). It has integers, booleans, strings,
globals and lexically-scoped locals, all the arithmetic/comparison/logical
operators, `if`/`while`/`for`, first-class functions with recursion and real
call frames, and an integrated **mark-sweep garbage collector**. It is the
integer-valued sibling of Bob Nystrom's `clox` from *Crafting Interpreters*,
re-commented from scratch for this lab and switched to computed-goto dispatch.

This is a 🟥 giant, and it ships a **genuinely working teaching core end to
end** — you can compile it and run real programs (see `examples/demo.lox`). What
it deliberately omits, so the core stays legible, is listed honestly under
[Scope](#scope-what-this-core-does-and-doesnt-do).

## What you'll learn

- **Lexing** without a regex engine: a single forward cursor, maximal munch
  (`!` vs `!=`), and a keyword trie (`scanner.c`).
- **Pratt parsing**: how one tiny `parsePrecedence()` loop plus a table of
  `{prefix, infix, precedence}` rules replaces a dozen mutually-recursive
  grammar functions, and how left-vs-right associativity falls out of asking for
  the right operand at `precedence + 1` (`compiler.c`).
- **Single-pass compilation**: emitting bytecode with no AST, resolving locals
  to **stack slots** at compile time, and **backpatching** forward jumps whose
  targets aren't known yet.
- **A stack machine**: why `1 + 2 * 3` needs no registers, and how function
  calls become **frames** — windows carved out of one shared value stack, so a
  call allocates nothing.
- **Computed-goto dispatch**: the single most performance-relevant trick in a
  bytecode interpreter, and *why* replicating the indirect branch beats a shared
  `switch` (branch prediction) — visible instruction-by-instruction in the asm.
- **Mark-sweep GC integration**: the tri-color invariant, an explicit gray
  worklist (no recursion), roots (stack, frames, globals, the in-progress
  compiler), **weak** references for string interning, and the allocation-site
  discipline that keeps a half-built object from being collected mid-flight.
- **Systems-level edge cases**: two's-complement wraparound done with defined
  behavior (unsigned round-trip), and trapping the two `IDIV` faults
  (`x/0` and `INT64_MIN / -1`) instead of taking a `SIGFPE`.

## Build & run

**Platform:** portable ISO/GNU C11 — builds on Linux, WSL, and clang on Windows.
No root, no special capabilities, no kernel features needed. Dispatch uses a GNU
extension (clang/gcc); an MSVC build automatically falls back to an equivalent
portable `switch` (see `common.h`).

```bash
make            # build ./lvm
make run        # build, then run examples/demo.lox
./lvm           # interactive REPL  (Ctrl-D / Ctrl-Z to exit)
./lvm file.lox  # run a program file
```

Two instructive debug builds:

```bash
make debug      # trace every instruction + disassemble each function as it compiles
make gc-test    # force a full GC on EVERY allocation, then run the demo:
                # a missing GC root would crash here, deterministically
```

Regenerate the teaching assembly (works on any host; clang cross-targets Linux):

```bash
make asm        # writes asm/demo.{O0.s,s,O2.s}; leaves demo.annotated.s alone
```

## How it works

The pipeline is **source → tokens → bytecode → execution**, and the files follow
it. (Header/impl pairs are described together.)

| File | Role |
|------|------|
| `common.h` | Fixed-width typedefs and the two feature switches (`VM_COMPUTED_GOTO`, the `DEBUG_*` toggles). |
| `value.{h,c}` | The universal `Value` — a 16-byte tagged union of `nil`/`bool`/`int64`/`Obj*`. Constructors, printing, cross-type equality. |
| `chunk.{h,c}` | A **chunk**: the flat bytecode array, a parallel line table, and a constant pool. Defines the whole opcode set (the ISA). |
| `object.{h,c}` | Heap objects behind a shared `Obj` header (manual C "inheritance"): `ObjString` (immutable, **interned**, FNV-1a hashed) and `ObjFunction`. |
| `table.{h,c}` | An open-addressing hash table (linear probing, tombstones). Backs globals **and** the string-intern set. |
| `scanner.{h,c}` | The lexer: allocation-free, tokens borrow pointers into the source. |
| `compiler.{h,c}` | The Pratt parser + single-pass code generator. Resolves locals to slots, emits and backpatches jumps, compiles functions. **The front end.** |
| `vm.{h,c}` | The stack machine: the value stack, call frames, and `run()` — the computed-goto fetch-decode-execute loop. **The back end.** |
| `memory.{h,c}` | The single `reallocate()` choke point and the **mark-sweep collector** (mark roots → trace gray worklist → sweep the object list). |
| `debug.{h,c}` | The disassembler used by `make debug` and `DEBUG_TRACE_EXECUTION`. |
| `main.c` | The CLI: REPL and file runner, mapping results to `sysexits.h` codes. |

**The two ideas worth reading closely:**

1. **The rule table drives everything** (`compiler.c`, `rules[]`). Each token
   type says how it behaves as a prefix operator, as an infix operator, and at
   what precedence. `parsePrecedence(level)` parses a prefix, then consumes infix
   operators while their precedence ≥ `level`. That one loop *is* the expression
   grammar. `if`/`while`/`for` are ordinary recursive descent on top, with
   `for` desugared to a `while` (note the increment jump dance, comment-by-comment
   in `forStatement`).

2. **`run()` is the engine** (`vm.c`). It caches the frame's instruction pointer
   in a register-resident local `ip` and dispatches with
   `goto *dispatchTable[*ip++]` at the tail of *every* handler. The call/return
   path shows frames as stack windows: `OP_CALL` writes `ip` back, sets up the
   callee frame; `OP_RETURN` collapses the callee's window and pushes the result.

## Assembly notes

The real translation units include `<stdio.h>` and friends, which are libc
headers **absent for the Linux target on a non-Linux host**, so they cannot be
cross-compiled to teaching asm here. Per the lab convention, the project's
hottest pure-logic routine is therefore extracted into a self-contained
`asm/demo.c` (no `#include`s, its own types) — a miniature stack VM whose
`vm_run` uses the **same computed-goto dispatch** as `vm.c`.

`asm/demo.annotated.s` walks the `-O1` output instruction by instruction. The
payoff is seeing the dispatch in the flesh:

- a **jump table** of 8-byte code pointers in `.data.rel.ro`, indexed by opcode;
- the recurring three-instruction tail `movzbl (%rcx),%esi ; incq %rcx ;
  jmpq *(%rax,%rsi,8)` at the end of **every** handler — the replicated indirect
  branch that gives each opcode its own predictor history;
- the stack machine dissolving into pointer math (`push` = store + `addq $8`),
  with clang fusing `pop b; pop a; push a+b` into one in-place `addq`;
- `ip` living in `%rcx` and `sp` in `%rdx` across the whole loop, with the table
  base hoisted once into `%rax`;
- a nice detail: because `vm_run` calls nothing, 128 bytes of its 512-byte value
  stack live in the **red zone**, shrinking the explicit `subq`.

Compare the levels: `demo.O0.s` spills `ip`/`sp` to memory each step (clearest
C-to-asm mapping); `demo.s` (`-O1`, annotated) pins them in registers;
`demo.O2.s` drops the frame pointer. Notably, at every level `demo_run` is
**not** constant-folded to `mov $19` — the indirect jump hides the program from
the optimizer, which is the very property that makes the *branch predictor*, not
the compiler, responsible for dispatch speed.

## Scope (what this core does and doesn't do)

**Implements:** integer (`int64`) arithmetic with defined overflow and trapped
division faults; booleans, `nil`, and interned strings with `+` concatenation;
`==`/`!=`/`<`/`<=`/`>`/`>=`; `!`, `and`, `or` (short-circuit); global and
block-scoped local variables with assignment; `print`; `if`/`else`,
`while`, `for`, `{ }` blocks; functions with parameters, `return`, recursion,
and real call frames; and a working mark-sweep GC that survives collection on
every allocation (`make gc-test`).

**Omits, on purpose** (each is a natural stretch):
- **Closures / upvalues.** Functions capture no enclosing locals; a nested
  function sees only globals and its own scope. This is the single biggest
  simplification vs. full Lox and keeps frames a flat stack window.
- **Classes / methods / inheritance.**
- **Floating point.** Numbers are `int64` by design (a systems-lab choice: the
  interesting cases are overflow and truncating division).
- **String escapes** (`\n`, `\"`): a string literal is the raw bytes between the
  quotes.
- **Constant/jump operands wider than one/two bytes** (max 256 constants and 256
  locals per function; `clox` adds `_LONG` forms to lift this).
- **Incremental/generational GC.** The collector is a simple stop-the-world
  mark-sweep.

## Going further (the `Stretch:`)

- **Add closures.** Introduce `ObjClosure` + `ObjUpvalue`, the `OP_CLOSURE`/
  `OP_GET_UPVALUE`/`OP_CLOSE_UPVALUE` opcodes, and upvalue capture in the
  compiler. This is the marquee chapter of *Crafting Interpreters* part III and
  turns this into a real functional language.
- **NaN-box the Value.** Collapse the 16-byte tagged union into a single 64-bit
  word by hiding pointers and small integers inside the payload of a quiet NaN.
  Halves every stack push/pop's memory traffic; measure the win.
- **A real benchmark + a `switch` A/B.** Build both dispatch styles from the
  same sources (`common.h` already has the toggle) and time `fib(35)` to
  *measure* the computed-goto speedup on your CPU instead of taking it on faith.
- **What production does.** LuaJIT and V8 go far past a portable interpreter:
  trace/method JITs, inline caches, hidden classes, generational GC. Lua 5.x's
  `lvm.c` is the canonical readable production interpreter and uses exactly this
  computed-goto dispatch.

## References

- **Bob Nystrom, *Crafting Interpreters*** (Part III, "A Bytecode Virtual
  Machine") — the on-ramp this project follows; `clox` is its reference code.
- **Lua source**: `lvm.c` (the dispatch loop), `lgc.c` (an incremental
  collector) — the production version of these ideas.
- **GCC manual**, "Labels as Values" — the computed-goto extension, including
  the `static void *array[] = { &&foo, ... }` pattern used here.
- **System V AMD64 ABI** (the psABI) — registers, red zone, stack alignment; the
  contract `asm/demo.annotated.s` explains.
- The lab's own `12-jit-compiler` (turns dispatch into native code) and
  `06-garbage-collector` (a standalone GC) are natural companions.
