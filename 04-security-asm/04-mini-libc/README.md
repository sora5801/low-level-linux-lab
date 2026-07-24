# A minimal libc 🟧

**What it is.** A tiny, from-scratch C runtime you can link real programs
against with `-nostdlib`. It supplies its own `_start` (hand-written assembly
that reads `argc`/`argv`/`envp`/`auxv` off the initial stack), a
`__libc_start_main` that calls your `main` and turns its return into
`exit_group`, typed syscall wrappers with `errno` handling, a small first-fit
`malloc`, a varargs `printf`, and the core `<string.h>` primitives. Two demo
programs (`demo/hello.c`, `demo/args.c`) look like ordinary C and link only
against this — no glibc anywhere underneath. **musl** is the reference to read;
every routine here has a fuller cousin in musl's `src/` tree, and the comments
point at it.

> **On your own machines only.** This is educational systems code. The one
> "attack surface" it touches is your own: you toggle mitigations (`-no-pie`,
> `-fno-stack-protector`, NX) on binaries you compile yourself, to *see* what
> they do. Nothing here targets anyone else's system. The security payload of
> the project is **defensive** — understanding where the stack canary's entropy
> comes from, why the stack is marked non-executable, and why `printf(user)` is
> a bug — so you can recognize and prevent those failures. See
> [Security & defense notes](#security--defense-notes).

## What you'll learn

- **CRT startup**: what really happens between `execve` and `main`. The kernel
  jumps to `_start` (the ELF `e_entry`), *not* `main`; `main` is a libc
  convention that `__libc_start_main` implements.
- **The initial process stack**: `argc` at `[rsp]`, `argv` at `rsp+8`, then
  `envp`, then the **auxiliary vector** — and how a libc *walks* that layout to
  find each piece (the kernel doesn't hand you pointers to them).
- **The auxv**, including `AT_PAGESZ`, `AT_SECURE`, and `AT_RANDOM` (the
  kernel-supplied entropy a real libc turns into the **stack canary**).
- **The Linux x86-64 syscall ABI** (`rax`=number; args in
  `rdi,rsi,rdx,r10,r8,r9`; `rcx`/`r11` clobbered) and how it differs from the
  *function* call ABI (4th arg in `r10` vs `rcx`).
- **`errno`**: translating the kernel's negative-return convention into the
  POSIX `(-1, errno)` contract.
- **varargs** (`va_list`/`va_arg`) and **integer-to-ASCII** conversion — the
  guts of `printf`.
- **A heap allocator**: header-before-payload metadata, alignment, first-fit,
  splitting, and coalescing — grown by pushing the program `brk`.

## Build & run (Linux / WSL)

```bash
make            # builds libminilibc.a and links ./hello and ./args
make run        # runs both demos
./hello a b c ; echo "exit=$?"
./args foo bar
```

Inspect what you built (this is half the fun):

```bash
readelf -h hello         # Type: EXEC, no PT_INTERP — nothing dynamic, no libc
readelf -l hello         # note the GNU_STACK segment: RW, not RWE  => NX on
nm hello | grep _start   # our _start is the entry symbol
strace ./hello a b       # exactly: brk (malloc), write (printf), exit_group
```

On a non-Linux host you can't *run* the ELF, but you can still regenerate the
teaching assembly, which is the committed deliverable:

```bash
make asm        # clang cross-targets Linux; writes asm/demo.{O0.s,s,O2.s}
make disasm     # genuine encoded bytes of the _start star artifact
```

> This mini-libc was verified end-to-end by cross-linking both demos to static
> Linux ELF executables with `clang --target=x86_64-pc-linux-gnu ... -fuse-ld=lld`
> (Type `EXEC`, no dynamic section, entry = `_start`): every symbol resolves.

## How it works

Built bottom-up, one job per file:

| File | Role |
|------|------|
| `src/start.S` | **★ the star artifact.** `_start`: reads `argc`/`argv` off `[rsp]`, aligns the stack, and tail-calls `__libc_start_main`. Hand-written because at entry `rsp` points straight at raw kernel data a compiler would clobber. |
| `src/crt.c` | `__libc_start_main` — derives `envp` (just past `argv`'s NULL), publishes `environ`, walks to the `auxv`, calls `main`, then `exit`. Also `getauxval`, `exit`/`_exit`, and `write_all`. |
| `src/syscall.c` | `syscall1/3/6` inline-asm templates, the `errno` global, `__syscall_ret` (the `-1`/`errno` translation), and the POSIX wrappers `read/write/open/close/mmap/munmap` plus `brk`/`sbrk`. |
| `src/string.c` | `strlen/strcpy/strcmp/memcpy/memset` — byte-at-a-time, with the contracts (overlap, terminator, alignment) spelled out. |
| `src/malloc.c` | first-fit free list: header-before-payload blocks in address order, 16-byte aligned, grown with `sbrk`; `free` coalesces adjacent free blocks; `calloc` is overflow-checked; `realloc`. |
| `src/printf.c` | buffered varargs printer: `%d %i %u %x %X %c %s %p %%` + `l` modifier, one `write(2)` per flush. |
| `include/minilibc.h` | the entire public surface: types, `va_list`, syscall numbers, flags, `errno` codes, and every prototype. |
| `demo/hello.c`, `demo/args.c` | ordinary-looking C linked against the above. `args.c` prints the auxv and `AT_RANDOM`. |

The dependency spine: `printf`→`write_all`→`write`→`syscall3`; `malloc`→`sbrk`
→`brk`→`syscall1`; `_start`→`__libc_start_main`→`main`→`exit`→`exit_group`.

## Assembly notes

Two annotated artifacts, both to the lab's reference standard:

- **`src/start.S`** — the hand-written `_start`, commented per instruction. It
  is the place to *see* the initial stack: `argc` at `[rsp]`, `argv` = `rsp+8`,
  `envp` = `argv+argc+1`, `auxv` after `envp`'s NULL. It also shows the ABI
  discipline (`xor %ebp,%ebp` to end backtraces; `and $-16,%rsp` to satisfy the
  16-byte alignment a `call` requires; `ud2` as the never-return trap).
  [`asm/start.disasm.txt`](asm/start.disasm.txt) is the **genuine** `objdump`
  of it — note the zeroed `lea`/`call` displacements and the `R_X86_64_PC32`/
  `PLT32` relocations the linker fills for `main` and `__libc_start_main`.

- **`asm/demo.annotated.s`** — hand-annotated from the real `-O1` output of
  [`asm/demo.c`](asm/demo.c), which isolates `printf`'s integer-to-decimal
  core. Its lesson is one instruction: the compiler refuses to emit a hardware
  `div` for `/10` and instead multiplies by the magic reciprocal
  `0xCCCCCCCCCCCCCCCD` and shifts. Even better, it recovers the remainder in a
  single `imull $246` — because `246 ≡ -10 (mod 256)`, so `q*246 + v` yields
  `v - 10q` in the low byte with no second divide. Compare
  [`asm/demo.O0.s`](asm/demo.O0.s) (literal, spill-everything) and
  [`asm/demo.O2.s`](asm/demo.O2.s) (fully unrolled) with the annotated `-O1`.

Regenerate the compiler outputs with `make asm`; the annotated files and
`start.S` are authored and never overwritten.

## Security & defense notes

This project *builds* the machinery, so it is the right place to understand the
defenses it interacts with — each is called out in the code:

- **NX / W^X (`src/start.S`).** Every hand-written `.S` ends with
  `.section .note.GNU-stack,"",@progbits`. The linker unions these notes; if any
  object omits it or requests `"x"`, the whole program gets an **executable
  stack**, re-enabling "inject shellcode onto the stack and jump to it." With NX
  on, a stack overflow can no longer run injected bytes — the attacker must pivot
  to ROP against existing code. `readelf -l hello` shows `GNU_STACK` as `RW`.
- **Stack canary ↔ `AT_RANDOM` (`src/crt.c`, `demo/args.c`).** The canary's
  secret isn't magic: it's 16 random bytes the kernel places on the stack and
  advertises via `getauxval(AT_RANDOM)`. A real libc copies them into its canary
  global; each prologue stores the value below the return address and the
  epilogue checks it before `ret`. `demo/args.c` prints those bytes so the chain
  is concrete. (We build the demos with `-fno-stack-protector` because the
  canary path calls `__stack_chk_fail`, which lives in a libc we don't link.)
- **Format-string safety (`src/printf.c`).** `printf(user_input)` is the classic
  format-string bug: `%x%x...` leaks stack memory and `%n` *writes* to it. Our
  printf implements **no `%n`** (no write primitive) and the demos always pass a
  literal format with data as separate args — the safe pattern the
  `__attribute__((format(printf,...)))` annotations help enforce at compile time.
- **Heap metadata (`src/malloc.c`).** Block headers sit *before* the payload, so
  a heap overflow lands in the next block's `size`/`next` — the shape of every
  heap-exploitation primitive. `calloc` rejects `nmemb*size` overflow (a
  historically common under-allocation bug), and `free` sanity-checks a magic
  sentinel to turn a double-free into a no-op rather than list corruption.

## Going further

- **Stretch: TLS and thread-local `errno`.** Our `errno` is a single global —
  two threads racing on syscalls would clobber it. Set up the `%fs`-based Thread
  Local Storage block in `_start`/`__libc_start_main` (allocate the TLS image,
  point `%fs` at it with `arch_prctl(ARCH_SET_FS, ...)`) and make `errno` a
  `__thread`. This is the single biggest gap between this and a usable libc.
- **Run the ELF init-array.** A real `__libc_start_main` invokes
  `__attribute__((constructor))` functions (the `.init_array`) before `main`.
  Walk `__init_array_start`..`__init_array_end` and call each.
- **A real `malloc`.** Add size-class bins, an `mmap` fast path for large
  requests, and `munmap` on free; then read glibc `malloc/malloc.c` and musl's
  `mallocng` to see arenas, tcache, and safe-linking.
- **Width/precision in `printf`** (`%5d`, `%08x`, `%.3s`) — a small state
  machine; musl's `src/stdio/vfprintf.c` is the map.

What production does differently, in one line: real libcs are **dynamically
linked** (the `_start` you get is `crt1.o`, which calls `__libc_start_main` in
`libc.so`), set up **TLS + the canary + locale + stdio buffering** before
`main`, and make every primitive here 10× faster with SIMD and lock-free fast
paths.

## References

- System V AMD64 ABI (psABI), §3.4 "Process Initialization" — the initial stack
  layout and the `argc`/`argv`/`envp`/`auxv` figure this project reads.
- `man 2 syscall`, `man 3 getauxval`, `man 5 elf` (the `Elf64_auxv_t` types).
- **musl**: `crt/crt1.c` (the real `_start`), `src/env/__libc_start_main.c`,
  `arch/x86_64/syscall_arch.h`, `src/stdio/vfprintf.c`, `src/malloc/`.
- Linux `tools/include/nolibc/` — a header-only libc built on exactly this idea.
- Granlund & Montgomery, "Division by Invariant Integers using Multiplication"
  (1994) — why `/10` compiles to that `0xCCC…CCD` multiply.
