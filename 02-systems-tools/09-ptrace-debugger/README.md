# A ptrace debugger 🟥

**What it is.** A real, working command-line debugger for Linux/x86-64, built on
`ptrace(2)`. It forks a child that does `PTRACE_TRACEME` + `execve`, then in the
parent it plants **software breakpoints** (save the original byte, poke `0xCC` =
`int3`), handles the resulting **SIGTRAP** (restore the byte, rewind `RIP`),
**single-steps** instructions, reads/writes **registers** (`GETREGS`/`SETREGS`)
and **memory** (`PEEKTEXT`/`POKETEXT` and `/proc/<pid>/mem`), computes the load
base from **`/proc/<pid>/maps`** to support PIE/ASLR, parses the **DWARF
`.debug_line`** program to map addresses to `file:line`, reads the **ELF
`.symtab`** to map addresses to function names, and walks the **frame-pointer
chain** for backtraces — all driven from a small REPL (`break`, `continue`,
`step`, `regs`, `reg`, `mem`, `bt`, `list`, `delete`, `info`).

It is a **teaching core**, and honestly scoped. It genuinely works end to end —
the transcript below is real output — but it deliberately leaves out the machinery
that makes a production debugger complete: it decodes **DWARF v2–v4** line tables
only (not the restructured v5 header), it has **no expression evaluator or
`.debug_info` local-variable/type decoding** (`file:line` yes, "print `x`" no),
its `step` is a **single machine instruction** (not source-level `next`/`step`
that skips over calls), and its backtrace is a naive **frame-pointer walk** (no
DWARF CFI, so it is wrong at a function's first instruction before the prologue
runs). The [Going further](#going-further-the-stretch-from-the-list) section
spells out each gap.

## What you'll learn

- **`ptrace(2)`** end to end: `PTRACE_TRACEME`, `PTRACE_CONT`,
  `PTRACE_SINGLESTEP`, `PTRACE_GETREGS`/`SETREGS`, `PTRACE_PEEKTEXT`/`POKETEXT`,
  `PTRACE_SETOPTIONS` (`PTRACE_O_EXITKILL`), and the request/pid/addr/data calling
  convention.
- **The int3 breakpoint trick**: why a one-byte `0xCC` is all you need, why `RIP`
  is `addr+1` after it fires, and the restore-step-rearm dance to resume past a
  breakpoint transparently.
- **The `PEEKTEXT` errno gotcha**: `-1` is both a legal word and the error return,
  so you *must* clear `errno` first and check it after.
- **`waitpid(2)` status decoding**: `WIFSTOPPED`/`WSTOPSIG` vs
  `WIFEXITED`/`WIFSIGNALED`, and retrying on `EINTR`.
- **Address spaces & PIE**: link-time (DWARF) vs runtime addresses, and computing
  the load base from `/proc/<pid>/maps` so `runtime = link + base`.
- **ELF**: finding sections by name through the section-header string table, and
  reading `STT_FUNC` symbols out of `.symtab`/`.strtab`.
- **DWARF `.debug_line`**: that it is a *bytecode program* for a state machine
  (special/standard/extended opcodes, LEB128, `line_base`/`line_range`), not a
  table — and how running it materialises `address → file:line` rows.
- **Stack walking**: how the `%rbp` chain encodes caller frames and return
  addresses, and why you symbolise `return_addr − 1`.

## Build & run (Linux / WSL)

This is **Linux-only** — it uses `ptrace`, `/proc/<pid>/{mem,maps}`, and the
ELF/DWARF layout. On Windows use WSL2. (The teaching *assembly* in `asm/`
regenerates on any host; see below.)

```bash
make                 # builds ./ptrace-dbg and ./sample (with gcc:  make CC=gcc)
make run             # start an interactive session on ./sample
make test            # run a scripted session (break/continue/bt/regs) end to end
```

A real interactive session (recursion + backtrace):

```
$ ./ptrace-dbg ./sample
ptrace-dbg: inferior pid 621, load base 0x555555554000 (PIE)
(dbg) break sample.c:28          # the recursive call inside factorial()
  breakpoint #1 at 0x555555555165
(dbg) continue                    # hits at n=5
(dbg) continue                    # hits at n=4
(dbg) continue                    # hits at n=3
(dbg) bt
  #0  0x0000555555555165 factorial+0x1c at sample.c:28
  #1  0x0000555555555172 factorial+0x28 at sample.c:28
  #2  0x0000555555555172 factorial+0x28 at sample.c:28
  #3  0x00005555555551d6 main+0x1c at sample.c:44
  #4  0x00007ffff7c2a1ca ??       # into libc's __libc_start_call_main
(dbg) regs                        # rdi = 3  => this is factorial(3)
```

**See the breakpoint in the bytes.** Stop at a function, then dump its first
bytes — you can watch the planted `int3`:

```
(dbg) break sum_to
(dbg) continue
(dbg) mem 0x555555555182 8
  0x555555555182: cc 0f 1e fa 55 48 89 e5   |....UH..|
                  ^^ our int3 — the real first byte (0xf3 of endbr64) is saved
(dbg) delete 1                    # restores 0xf3; the program then runs clean
(dbg) continue
factorial(5) = 120
sum_to(5)    = 15
[inferior exited normally, status 0]
```

## How it works

Four translation units, layered bottom-up (all declared in `debugger.h`, which
is the data-model "vocabulary" — breakpoints, the line table, symbols, the
session struct):

- **`inferior.c`** — the syscall floor. `inferior_spawn` does the
  `fork`/`TRACEME`/`execvp` bootstrap and consumes the automatic post-`execve`
  SIGTRAP; `inferior_wait` decodes `waitpid` status (retrying on `EINTR`);
  `inferior_peek`/`poke` wrap `PEEKTEXT`/`POKETEXT` (with the `errno` dance);
  `inferior_read_mem`/`write_mem` use `/proc/<pid>/mem` for arbitrary ranges;
  `inferior_cont`/`step` resume the child; `proc_load_base` parses
  `/proc/<pid>/maps` for the PIE load base.
- **`breakpoint.c`** — the `int3` machinery. `bp_enable` reads the word, saves the
  low byte, and splices in `0xCC`; `bp_disable` splices the saved byte back;
  `step_over_breakpoint` lifts the `0xCC`, single-steps the real instruction, and
  re-arms it. This is the file the assembly deliverable extracts.
- **`debuginfo.c`** — symbolisation. `di_load` mmaps the ELF read-only, walks the
  section headers to find `.symtab`/`.strtab` (→ function symbols) and
  `.debug_line` (→ the line-number program), then runs the DWARF state machine
  (`di_run_line_program`) into a sorted `address → file:line` table. Queries:
  `di_addr_to_line`, `di_addr_to_func`, `di_line_to_addr`, `di_func_to_addr`.
- **`debugger.c`** — the REPL and control loop. It converts between link-time and
  runtime addresses at every boundary, handles a stop (breakpoint hit → rewind
  `RIP`; single-step; forwarded signal), and implements each command, including
  the frame-pointer `backtrace`.

`sample.c` is the tiny program we debug: a recursive `factorial` (multi-frame
backtraces), an iterative `sum_to` (loop + locals for single-stepping), and a
`main` that calls both. The Makefile builds it with `-g -gdwarf-4
-fno-omit-frame-pointer -O0` so the line table and `%rbp` chain are clean.

### The control loop, in one breath

`resume (CONT/SINGLESTEP) → waitpid → why did it stop?` A `SIGTRAP` at
`bp_addr + 1` is one of our breakpoints (rewind `RIP`, restore byte, report);
a `SIGTRAP` elsewhere is a single-step landing; any other signal was raised by
the program (report it, and forward it into the child on the next resume); an
exit ends the session.

## Assembly notes

`asm/demo.c` is a self-contained extraction of the two purest routines — the
`int3` byte-splice (`patch_int3`/`saved_byte_of`/`unpatch_byte`, mirroring
`bp_enable`/`bp_disable`) and the `addr_to_line` binary search (mirroring
`di_addr_to_line`) — because those are 100% register-and-pointer logic with no
system headers, so clang turns them into clean Linux/SysV assembly. (The real
sources can't compile to asm standalone: they pull in `<sys/ptrace.h>`,
`<elf.h>`, `<sys/user.h>`.)

[`asm/demo.annotated.s`](asm/demo.annotated.s) is the hand-commented `-O1`
output. The lessons it makes visible:

- **`patch_int3`** — `(word & ~0xFF) | 0xCC` becomes `andq $-256` then
  `leaq 204(%rdi)`: because the low byte is zeroed first, the compiler proves the
  `OR` can be a `+0xCC` and folds it into an **LEA displacement**. At `-O0`
  (`asm/demo.O0.s`) the same code is the literal `andq $-256; orq $204` — you can
  watch the fold appear as the optimiser turns on.
- **`rewind_rip`** — the whole "make a breakpoint invisible" correction is one
  `leaq -1(%rdi)`.
- **`addr_to_line`** — the classic "upper-bound, then step back one" binary search,
  lowered to register-only index math (`sizeof(line_row)==24` built as `×3` with an
  `×8` addressing scale). Its `return end ? -1 : lo-1;` compiles to a
  **branchless `cmp`/`sbb`/`or`** idiom, where `sbb %edx,%edx` smears the carry
  flag into a `0` or `-1` mask — a great example of the optimiser erasing a branch.

Regenerate the three levels with `make asm` (works on any host; clang
cross-targets Linux). The header block in the annotated file documents the SysV
AMD64 ABI (arg registers `rdi,rsi,rdx,rcx,r8,r9`, return in `rax`, callee-saved
`rbx,rbp,r12–r15`, the 128-byte red zone, 16-byte call alignment).

## Going further (the `Stretch:` from the list)

- **Hardware breakpoints (debug registers).** Instead of patching bytes, program
  the `DR0–DR3` address registers and `DR7` control via
  `PTRACE_POKEUSER` at the `offsetof(struct user, u_debugreg[...])`. These give you
  up to four breakpoints with **no code modification** (works on read-only/shared
  pages) and — crucially — **watchpoints** that trap on data reads/writes, which
  software `int3` cannot do.
- **Reverse debugging.** Record a log of every executed instruction's effects (or
  periodic snapshots via `fork`/CRIU) so `reverse-continue`/`reverse-step` can
  restore state. `rr` does this by recording nondeterminism (syscalls, signals,
  RDTSC) and replaying deterministically.
- **What production adds.** Source-level `step`/`next` (single-step while the PC
  stays on the same line, stepping *over* `call`s by breakpointing the return
  address); a `.debug_info` DWARF reader for **local variables, types, and
  `print expr`** (DIEs, DW_AT_location, DWARF expression evaluation); **DWARF CFI**
  (`.eh_frame`) for correct unwinding without frame pointers; multi-threaded and
  multi-process tracing (`PTRACE_O_TRACECLONE`/`FORK`/`EXEC`); conditional
  breakpoints; and **DWARF v5** line tables (the `directory_entry_format` /
  `file_name_entry_format` header this parser skips).

## References

- `man 2 ptrace`, `man 2 waitpid`, `man 5 proc` (`/proc/<pid>/{mem,maps}`).
- The **DWARF Debugging Standard**, §6.2 "Line Number Information" — the state
  machine and opcode set this project implements.
- The **ELF** spec / `man 5 elf` — `Elf64_Ehdr`, `Elf64_Shdr`, `Elf64_Sym`.
- Eli Bendersky, *"How debuggers work"* (parts 1–3) and Sy Brand's *"Writing a
  Linux Debugger"* series — the two canonical from-scratch walkthroughs.
- Read the real thing: **GDB** (`gdb/breakpoint.c`, `gdb/dwarf2/`), **lldb**, and
  **`rr`** (reverse execution).
