# A JIT compiler 🟥

**What it is.** A **just-in-time compiler** for Brainfuck: it parses a program,
compiles it to **native x86-64 machine code at run time** — emitting the actual
opcode bytes by hand into an `mmap`'d buffer — flips that buffer from writable to
executable (`mprotect`, enforcing **W^X**), and then **calls into it through a
function pointer** using the System V AMD64 calling convention. The same program
also ships a plain interpreter over the identical IR, so you can measure exactly
what compiling-to-native buys you.

Difficulty 🟥 (giant). This is a genuinely working teaching **core**: it compiles
all eight Brainfuck instructions to correct native code, runs real programs, and
beats the interpreter. What it is *not* is a general-purpose or multi-language
JIT — see [Scope](#scope-what-this-does-and-doesnt-do) for the honest boundary.

## What you'll learn

- **x86-64 instruction encoding by hand**: the REX / opcode / ModRM byte
  structure, why `add $imm32,%rbx` is `48 81 C3 xx xx xx xx`, how immediates and
  jump displacements are little-endian, and how `rel32` conditional jumps are
  **back-patched** once their targets are known. Every encoding in `emit.c` was
  cross-checked with `llvm-mc --show-encoding`.
- **`mmap` / `mprotect` and W^X**: allocating anonymous RW memory, writing code
  into it, then dropping `PROT_WRITE` and adding `PROT_EXEC` so a page is never
  writable-and-executable at once — the invariant hardened kernels enforce.
- **The SysV AMD64 calling convention from the generator's side**: arguments in
  `rdi, rsi, rdx`; callee-saved `rbx/r14/r15` chosen so the data pointer and I/O
  callbacks survive the calls the generated code makes; and the 16-byte stack
  alignment discipline required at every `call`.
- **i-cache coherency**: why x86 needs no explicit flush (hardware keeps I- and
  D-caches coherent; the `mprotect` syscall serializes) but ARM/RISC-V do, and
  where `__builtin___clear_cache` fits.
- **Why a JIT is faster than an interpreter**: removing per-op dispatch and
  branch misprediction by turning each op into inline, statically-linked native
  instructions — plus a first taste of a **peephole optimizer** (run-folding and
  the `[-]` → *set-zero* rewrite).

## Build & run (Linux / WSL)

The runtime is **Linux-only** (it calls `mmap`/`mprotect` and executes generated
code). On Windows use WSL. The assembly deliverable builds anywhere (below).

```bash
make                # builds ./bfjit  (clang -Wall -Wextra)
make run            # -> Hello World!         (through the JIT)
make bench          # -> interpreter vs JIT timing + correctness check
./bfjit prog.bf     # run your own program with the JIT (reads stdin for ',')
./bfjit --interp prog.bf   # run it with the interpreter instead
```

`make bench` (also `./bfjit --bench`) prints something like:

```
hello world: interp OK, jit OK
benchmark: triple-nested countdown N=200 (~8.0M inner iters), 28 ops after folding
output agreement: MATCH (4 bytes each)
interpreter:    0.0421 s
jit compile:    0.0000 s
jit execute:    0.0126 s
speedup (execute-only): 3.3x
```

(Exact numbers vary by machine — the run above is a WSL2 box; the point is the
JIT's *execute* time is several times smaller than the interpreter's, while both
produce byte-identical output. Register-allocating the cell value, the first
stretch goal below, widens the gap substantially.)

## How it works

The pipeline is `source text → IR → {interpret | JIT}`. Both back ends consume
the **same** optimized IR, so the comparison isolates execution strategy.

- **`bf.h`** — the IR: a flat `op[]` array (`OP_ADD`, `OP_MOVE`, `OP_CLEAR`,
  `OP_OUT`, `OP_IN`, `OP_JZ`, `OP_JNZ`, `OP_HALT`) plus the `bf_io` callback pair
  that both back ends use for `.` and `,`.
- **`parse.c`** — the front end and first optimizer. It skips comment bytes,
  **folds** runs of `+`/`-` and `>`/`<` into a single op with a net delta, folds
  `.` runs into a repeat count, recognizes the **`[-]` / `[+]` idiom** and emits
  one `OP_CLEAR`, and matches brackets with an explicit stack (reporting
  unbalanced ones precisely).
- **`interp.c`** — the baseline: a `switch`-dispatch loop over the IR. Its whole
  cost is the per-op load + branch + data-dependent jump that the JIT deletes.
- **`emit.c` / `emit.h`** — the **instruction encoder**: one function per x86-64
  instruction the JIT emits, each appending exact bytes to a bounds-checked
  `code_buf`. Deliberately free of system headers (it only *writes bytes*), which
  is also why it compiles straight to teaching assembly.
- **`jit.c`** — the privileged half: `mmap` an RW buffer, run the emit sequence
  (maintaining a back-patch stack for loops), `mprotect` to RX (**W^X**), issue
  the i-cache barrier, and call the result via a laundered function pointer.
- **`main.c`** — the CLI, the tape (a 64 KiB zeroed cell array), the I/O layer
  (real terminal or an in-memory capture buffer for the benchmark), and the
  timing harness.

### The generated function's contract

The JIT emits a function equivalent to this C prototype and ABI:

```c
void fn(unsigned char *tape,        /* rdi -> rbx : the data pointer p        */
        long (*read_byte)(void),    /* rsi -> r14 : called for ','            */
        void (*write_byte)(long));  /* rdx -> r15 : called for '.'            */
```

`rbx`, `r14`, `r15` are **callee-saved**, so the data pointer and the two
callback pointers survive every `call *%r14` / `call *%r15` the body makes. The
prologue pushes `rbp, rbx, r14, r15` and does one `sub $8,%rsp` so that `rsp % 16
== 0` at each `call` (SysV requirement). Each Brainfuck op becomes a handful of
inline instructions; loops become `cmpb $0,(%rbx)` + a `rel32` conditional jump
whose displacement is back-patched when the matching bracket is emitted.

## Scope (what this does and doesn't do)

**Does:** compile all 8 Brainfuck ops to correct native x86-64; fold op runs and
the `[-]` clear idiom (a real peephole pass); enforce W^X; call generated code
with a correct SysV frame; match an interpreter byte-for-byte. Every emitted
encoding is verified against `llvm-mc`, and the generated machine code for the
built-in programs was disassembled and checked instruction-by-instruction.

**Does not:** allocate Brainfuck cells into CPU registers (the data pointer lives
in `rbx`, but each cell access still hits memory); do tape-bounds checking;
implement tiered/adaptive recompilation; or generate position-independent code
for a language richer than Brainfuck. Those are the stretch goals below.

## Assembly notes

`asm/demo.c` is the self-contained extraction of the **instruction encoder** —
the byte-assembly heart of the JIT (`modrm`, `emit_u8/u32`, `emit_add_ptr`,
`emit_add_cell`, `patch_rel32`). `asm/demo.annotated.s` walks clang's `-O1`
output with a comment on essentially every instruction. The lessons it makes
visible:

- **`modrm(3,0,3)` is constant-folded away.** In `emit_add_ptr` the compiler
  replaced the call with three immediate stores `0x48, 0x81, 0xC3` — you are
  watching the generic encoder specialize into the one instruction it emits.
- **The per-byte bounds check survives inlining.** Every `emit_u8` becomes a
  `cmpq len,cap; jae overflow` guard, so a code generator physically cannot run
  past its buffer and then execute the wreckage.
- **Aliasing has a cost.** In `patch_rel32`, clang reloads `c->buf` before each
  of the four stores because it can't prove they don't alias the struct — a
  concrete case where `restrict` would help.

Regenerate everything with `make asm`. We also commit the assembly for the
**real** `emit.c` (`asm/emit.{O0,,O2}.s`), since it too is self-contained; the
`--target=x86_64-pc-linux-gnu` flag makes clang emit Linux SysV assembly on any
host. Compare `asm/demo.O0.s` (naive, every value spilled) with `asm/demo.s`
(-O1, annotated) and `asm/demo.O2.s` (optimizer let loose).

## Going further (the `Stretch:` from the list)

- **Register allocation.** Cache the current cell's *value* in a register across
  a run of `+`/`-`/`.` and only spill to memory on a `>`/`<`, turning many
  `addb (%rbx)` (a load-modify-store) into register adds. A real allocator would
  track a small window of the tape in `r8`–`r12`.
- **A deeper peephole optimizer.** Recognize multiply/copy loops
  (`[->+<]`, `[->++<]`) and compile them to a single `mov`/`imul`+`add`; fold
  `[>]`/`[<]` scan loops to a `repnz scasb`. We already do run-folding and `[-]`;
  these are the next rungs.
- **Tiered compilation.** Start in the interpreter, count loop back-edges, and
  JIT only "hot" regions — exactly what JavaScript and JVM engines do to avoid
  paying compile latency for code that runs once.
- **What production does.** LuaJIT, V8/TurboFan, and the JVM add SSA IR,
  optimizing passes, and on-stack replacement; DynASM (used by LuaJIT) is a
  preprocessor for exactly the hand-encoding we do in `emit.c`.

## References

- Intel SDM Vol. 2 (instruction encoding: REX, ModRM, opcode maps) and Vol. 3
  §8.1.3 / §11.6 (self-modifying code & cache coherency rules).
- System V AMD64 ABI (the psABI document) — argument/return registers, stack
  alignment, callee-saved set, the red zone.
- `man 2 mmap`, `man 2 mprotect`; `man 7 signal` for what a bad jump gets you.
- DynASM (LuaJIT) and `sljit` — production takes on "encode instructions by
  hand"; Eli Bendersky's "Adventures in JIT compilation" for a gentle tour.
