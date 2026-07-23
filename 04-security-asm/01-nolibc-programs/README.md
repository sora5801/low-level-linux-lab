# nolibc programs 🟩

**What it is.** A complete, statically-linked Linux program that uses **no libc
at all**: it provides its own `_start` entry point and talks to the kernel with
raw `syscall` instructions. This is the reference example for the whole lab —
its assembly is annotated in full as the template every other project follows.

## What you'll learn

- The **System V AMD64 ABI**: argument registers (`rdi, rsi, rdx, rcx, r8, r9`),
  the syscall convention (`rax` = number; args in `rdi, rsi, rdx, r10, r8, r9`;
  `rcx`/`r11` clobbered by the `syscall` instruction), and the return in `rax`.
- That a Linux process really starts at **`_start`**, not `main` — `main` is a
  libc convention, and `__libc_start_main` is the thing that calls it.
- How inline `asm` constraints (`"a"`, `"D"`, `"S"`, `"d"`, clobbers) pin C
  values into specific registers.
- What `-nostdlib -static -no-pie` actually remove from a binary.

## Build & run (Linux / WSL)

```bash
make run
# builds with:  clang -nostdlib -static -no-pie -fno-stack-protector -O2 hello.c -o hello
# prints:       Hello from a program with no libc!
#               exit=7
```

Inspect what you built:

```bash
readelf -h hello        # note: Type: EXEC, no PT_INTERP (no dynamic loader)
objdump -d hello        # the .text is tiny — no CRT, no libc
strace ./hello          # you will see exactly two syscalls: write, exit_group
```

## How it works

`hello.c` is one file, built bottom-up:

- `syscall3()` — an inline-asm wrapper that loads `rax` + three arg registers and
  executes `syscall`, declaring `rcx`/`r11`/`memory` as clobbered.
- `kstrlen()` — measures the string (no `<string.h>`).
- `sys_write()` / `sys_exit()` — thin typed wrappers over `syscall3`.
- `_start()` — the entry point: print the message, then `exit_group(7)`. It is
  marked `noreturn` because returning from `_start` would jump into garbage.

## Assembly notes

See [`asm/hello.annotated.s`](asm/hello.annotated.s) for the fully-commented
walkthrough. The key lesson is visible there: at `-O1` clang **inlined all five
functions into `_start`** and **constant-folded `kstrlen(msg)` to `35`**, so the
program becomes just "set up a `write` syscall, set up an `exit_group` syscall."
Compare:

- [`asm/hello.O0.s`](asm/hello.O0.s) — the naive mapping: every function is a
  real `call`, every value spilled to the stack. Easiest to trace statement by
  statement.
- [`asm/hello.s`](asm/hello.s) — `-O1`, the annotated baseline.
- [`asm/hello.O2.s`](asm/hello.O2.s) — `-O2`, for comparison.

Regenerate them with `make asm` (works on any host; clang cross-targets Linux).

## Going further (the `Stretch:` from the list)

- **Smallest possible ELF.** Hand-craft the ELF header, overlap fields, and get
  a runnable binary under ~200 bytes. Start from `readelf -h hello`, then write
  the 64-byte ELF header + one program header by hand in a `.S` file and use
  `objcopy`/a custom linker script to drop everything else.
- **Read the real thing.** The Linux kernel ships `tools/include/nolibc/` — a
  header-only libc replacement built on exactly this idea. `musl`'s `crt1.c`
  shows what a *real* `_start` does with `argc`/`argv`/`envp`/`auxv`.

## References

- System V AMD64 ABI (the psABI document) — ground truth for registers/stack.
- `man 2 syscall`, `man 2 write`, `man 2 exit_group`.
- Linux `tools/include/nolibc/`, `musl` `src/env/__libc_start_main.c`.
