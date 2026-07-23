# Conventions

This repo is a **teaching lab**, not a product. Every file is written to be read.
These conventions are what make ~65 independently-built projects feel like one
book. If you contribute (or if an automated agent extends the lab), follow them.

---

## 1. Directory layout

Each project lives at `NN-section/NN-project-name/` and is self-contained:

```
NN-project-name/
├── README.md            # concept, ABI/syscalls, build & run, "what to learn"
├── Makefile             # `make` builds; `make asm` regenerates teaching asm
├── src...               # heavily-commented source (.c/.h, .rs, .S, .py)
└── asm/                 # committed teaching assembly (for C projects)
    ├── <unit>.s             # clang -O1 Linux SysV output (the baseline)
    ├── <unit>.O0.s          # -O0: literal C-statement -> asm mapping
    ├── <unit>.O2.s          # -O2: what the optimizer really does
    └── <unit>.annotated.s   # hand-written, heavily-commented, human-readable
```

A project may add `docs/`, `tests/`, `img/`, or a `demo/` as needed.

## 2. Language choice

The source list is C-and-assembly centric, so **C is the default** and is what
the assembly requirement below applies to. Use `.S` (GAS) or NASM where the
project *is* assembly (shellcode, `_start`, SIMD, crypto, bootloader). Rust/Zig
/Go may appear where they teach the concept better, but a C project must stay C
so its generated assembly is meaningful.

## 3. Comments: explain the *why*, relentlessly

This is the one rule that matters most. **Comment as much as possible.** Assume
the reader knows C syntax but not this subsystem. For every non-obvious line,
say *why* it exists, not *what* it does. Specifically, always call out:

- **Every syscall**: its number, its arguments in register order, what the
  kernel does with them, and every error it can return that we handle.
- **Every ABI decision**: why a value is in *this* register, why the stack is
  aligned here, which registers are clobbered.
- **Every allocation** (per the reader's interest): size class, alignment,
  `mmap` vs `brk`, and who frees it. Flag ownership at each boundary.
- **Every `unsafe` / raw pointer / FFI boundary**: the invariant that makes it
  sound and what breaks if the invariant is violated.
- **Every concurrency primitive**: the memory ordering and *why* that ordering
  (acquire/release/seq_cst), and the race it prevents.

Prefer a short paragraph above a tricky block over a hundred end-of-line notes.
A file that is 50% comments by line count is normal and good here.

## 4. The assembly deliverable (C projects)

> "for c files, generate the assembly language files. In the generated assembly
> language files, transform it into something human readable and heavily comment
> that as well."

For each C translation unit that can be compiled standalone, run:

```bash
../../tools/gen_asm.sh path/to/unit.c        # writes asm/unit.{s,O0.s,O2.s}
```

Then hand-write `asm/unit.annotated.s`: take the `-O1` output and add a comment
to essentially **every instruction or small group of instructions**, plus a
header block explaining the function's ABI contract. The annotated file must be
*human readable*: normalize the compiler's `.LBB0_3` labels to meaningful names
in comments, group instructions by the C construct they came from, and explain
prologue/epilogue, argument marshalling, the red zone, spills, and any syscall
or vector instruction. See `04-security-asm/01-nolibc-programs/asm/` for the
reference example.

If a translation unit cannot compile standalone on this host (it needs Linux
headers not present here, common for kernel modules), extract its core routine
into a self-contained `asm/demo.c` and annotate *that*, so every project still
ships didactic assembly. Note the substitution in the project README.

Annotation style, per instruction:

```asm
    push %rbp                #  PROLOGUE: save caller's frame pointer
    mov  %rsp, %rbp          #  establish our frame; rbp now = frame base
    mov  %edi, -4(%rbp)      #  spill arg0 (n, in edi per SysV) to the stack
```

## 5. READMEs

Every project README answers, in this order:

1. **What it is** — one paragraph, plus the difficulty tag (🟩/🟧/🟥) from the list.
2. **What you'll learn** — the syscalls, kernel APIs, or instructions it exercises.
3. **Build & run** — exact commands. State the platform (most need Linux/WSL).
4. **How it works** — a tour of the code, file by file, with the key ideas.
5. **Assembly notes** — what the annotated asm highlights for this project.
6. **Going further** — the `Stretch:` goal from the list, and what production does.
7. **References** — the "read the source of the real thing" pointers.

Be honest about scope: if a 🟥 project ships a teaching *core* rather than a
complete implementation, say so explicitly and describe the gap.

## 6. Portability & honesty

- Most projects are **Linux-only** (that's the whole point). Say so; don't
  pretend a kernel module builds on Windows. The assembly is generated by a
  cross-targeting clang and *is* host-portable.
- Never fake output. If something isn't runnable in this environment, the README
  says how to run it on Linux/QEMU and the code is written to be correct there.
- Security/offensive projects are for your own machines and deliberately
  vulnerable targets only. Where the list pairs a red-team tool with a blue-team
  detector, **ship the detector too** — it's the more instructive half.

## 7. Build hygiene

- Warnings are errors in spirit: compile with `-Wall -Wextra`.
- No global state you can avoid; free what you allocate; check every syscall's
  return value (the projects are partly *about* the error paths).
- Keep each project buildable in isolation (`cd` in, `make`). No repo-wide build.
