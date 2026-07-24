# A linker 🟥

**What it is.** `minild` is a minimal **static linker** for x86-64 Linux: it
takes one or more relocatable objects (`.o`, `ET_REL` — the things a compiler
emits), lays their sections out into loadable segments, resolves symbols across
the objects, applies relocations, assigns final virtual addresses, writes ELF
program headers, and produces a **runnable static executable** (`ET_EXEC`) the
kernel can `execve(2)` directly. It is the counterpart to this section's
assembler (`06-assembler`) and disassembler (`05-x86-64-disassembler`): the
assembler makes one `.o`; the linker fuses several `.o`s into a program.

This is a 🟥 **teaching core**, and the scope line matters — read
[Going further](#going-further-the-dynamic-case) for exactly what is and isn't
built:

- **Built and working:** a *static* link of several `ET_REL` objects into a
  non-PIE `ET_EXEC`, with cross-object symbol resolution (strong/weak
  precedence), three W^X `PT_LOAD` segments, and the five relocation types that
  freestanding x86-64 code actually emits: `R_X86_64_64`, `PC32`, `PLT32`,
  `32`, `32S`. The demo links two objects into a program that prints and exits.
- **Explained, not built:** the whole **dynamic** machine — `PT_INTERP`, a
  GOT/PLT, `.dynsym`/`.dynamic`, `R_X86_64_GLOB_DAT`/`JUMP_SLOT`/`RELATIVE`, and
  `ld.so` at run time. That is a much larger system; wiring it in would bury the
  core lesson. The README says precisely how it differs.

> **On your own machines only.** Nothing here is an exploit, but this project
> and its neighbours are about how programs are *laid out in memory*, which is
> where exploit mitigations live. Build and link your own objects; inspect your
> own binaries. The "defense" notes below explain each mitigation a *real*
> linker places and what turning it off would cost you.

## What you'll learn

- The **ELF object model**: sections vs. segments, `Elf64_Ehdr` / `Shdr` /
  `Sym` / `Rela` / `Phdr`, and why the loader reads *program headers* while the
  linker reads *section headers*.
- **Symbol resolution**: defined vs. undefined, `STB_LOCAL`/`GLOBAL`/`WEAK`
  precedence (why two initialised globals collide but a weak yields to a
  strong), and `STT_SECTION` symbols (how "a string somewhere in `.rodata`" is
  named).
- **Relocations** — the arithmetic core: `S + A` for absolute, `S + A - P` for
  PC-relative, the signed/unsigned 32-bit range checks behind "relocation
  truncated to fit", and why a `call`'s addend is `-4`.
- **Address assignment & program headers**: picking a load base, page-aligning
  segments so `p_vaddr % p_align == p_offset % p_align`, and mapping the ELF
  header itself into the first segment.
- **Where mitigations live**: W^X segment permissions, PIE/ASLR, RELRO, and the
  `PT_GNU_STACK` executable-stack flag are all *linker* decisions.

## Build & run

The linker is a portable host tool; the example objects and the linked output
are Linux ELF. On **any** host you can link and inspect; to **run** the output
you need **Linux or WSL**.

```bash
make link            # build minild, cross-compile the examples, link demo.elf
make run             # Linux/WSL: prints the greeting twice, then exit=0
make check           # readelf -h/-l + objdump -d of demo.elf (any host)
```

Under the hood `make link` runs:

```bash
# 1. build the linker (native)
clang -Wall -Wextra -std=c11 -O2 minild.c -o minild
# 2. compile two Linux objects (no PIC, no common, nolibc)
clang --target=x86_64-pc-linux-gnu -ffreestanding -fno-pic -fno-stack-protector \
      -fno-common -c examples/start.c -o start.o
clang --target=x86_64-pc-linux-gnu -ffreestanding -fno-pic -fno-stack-protector \
      -fno-common -c examples/msg.c   -o msg.o
# 3. LINK them into a static executable (start.o first => _start leads .text)
./minild -o demo.elf start.o msg.o
```

Then verify it is a real, self-contained static executable:

```bash
readelf -h demo.elf     # Type: EXEC, no PT_INTERP, entry = _start
readelf -l demo.elf     # three PT_LOAD: R E / R / RW  (W^X holds)
objdump -d demo.elf     # the call/lea sites now hold real addresses, not 0
./demo.elf ; echo $?    # Linux: "Hello from a minild-linked program!" x2, 0
```

## How it works

Five source files, each doing one job:

- **`elf.h`** — every ELF64 structure and constant we touch, with field byte
  offsets in comments, plus explicit little-endian `rd*/wr*` accessors. We parse
  and emit by hand (never by casting raw bytes to a struct pointer) so the code
  is correct on any host and the *format itself* is legible.

- **`minild.c`** — the linker, as a straight pipeline (`main`):
  1. **`parse_object`** validates each `.o` (magic, ELF64, LE, `ET_REL`,
     `EM_X86_64`) and decodes its section headers and `.symtab`.
  2. **`layout`** bins every allocatable input section into one of four output
     sections **by its flags, not its name** (so `.text.*`, `.rodata.str1.1`,
     `.data.rel.ro`, … all land correctly — this is "orphan placement"):
     `.text` (R|X), `.rodata` (R), `.data` (R|W), `.bss` (R|W, no file bytes).
     It then assigns each section a final file offset and virtual address and
     builds one `PT_LOAD` per non-empty segment. The ELF header + program
     headers are mapped into the front of the text segment (the classic trick).
  3. **`resolve_symbols`** registers every defined global/weak symbol at its
     final address, enforcing precedence (one strong definition; weak yields to
     strong) and finding `_start` for `e_entry`.
  4. **`relocate`** walks every `SHT_RELA` section and applies each entry
     (`reloc_symbol_value` computes `S`; the `switch` computes `S+A` or
     `S+A-P` and patches 4 or 8 bytes, with range checks). **This is the
     linker.**
  5. **`write_output`** assembles the image — ELF header, program headers,
     copied section bytes (relocated in place), and a small section-header table
     + `.shstrtab` for inspectability — and writes it out.

- **`examples/start.c`** and **`examples/msg.c`** — two nolibc objects that
  reference each other. `start.c` defines `_start`; `msg.c` defines
  `print_hello`, `sys_exit`, and a `minild_hook` function pointer. Between them
  they force all five relocation types (see the table below), so the link
  exercises the whole engine.

The demo's relocations, straight from `objdump -r`:

| Site (object)             | Type              | Formula   | What it patches                       |
|---------------------------|-------------------|-----------|---------------------------------------|
| `call print_hello` (start)| `R_X86_64_PLT32`  | `S+A-P`   | direct cross-object call displacement |
| `minild_hook` (start)     | `R_X86_64_32S`    | `S+A`     | absolute addr of the pointer variable |
| `call sys_exit` (start)   | `R_X86_64_PLT32`  | `S+A-P`   | another cross-object call             |
| `hello` string (msg)      | `R_X86_64_64`     | `S+A`     | 64-bit address of the `.rodata` string|
| `minild_hook =` (msg)     | `R_X86_64_64`     | `S+A`     | pointer's initial value = `print_hello`|

After the link, `objdump -d demo.elf` shows `call 0x400110` (bound to
`print_hello`), `movabs $0x401000,%rdx` (the string), and `.data` holding
`10 01 40 00 …` = `0x400110` — real addresses where the `.o` held zeros.

## Assembly notes

The assembly deliverable is `asm/demo.c`: the **relocation-application core**
lifted out standalone (own types, no system headers) — `apply_reloc(loc, type,
S, A, P)`, exactly the `switch` in `minild.c`'s `relocate()`. It is the piece of
a linker worth reading as machine code.

- [`asm/demo.annotated.s`](asm/demo.annotated.s) — hand-annotated `-O1`, one
  comment per instruction, with the ABI header block. Two optimizer tricks are
  called out: the **"fits in signed 32"** test done as `(i64)(i32)v == v` via
  `movslq`, and the **merged tail** where `put32`'s 4th store and `put64`'s 8th
  store become one `movb %cl,(%rdi,%rax)` indexed by 3 or 7. The `demo_selfcheck`
  half shows the optimizer **constant-folding both relocations at compile time**
  — the linker's arithmetic vanishes into precomputed bytes, leaving only the
  checksum loop. Reading asm is how you *see* that.
- [`asm/demo.s`](asm/demo.s) — the untouched `-O1` baseline.
- [`asm/demo.O0.s`](asm/demo.O0.s) — `-O0`, literal statement-by-statement map.
- [`asm/demo.O2.s`](asm/demo.O2.s) — `-O2`, for comparison.

Regenerate with `make asm` (clang cross-targets Linux from any host).

## Going further (the dynamic case)

The teaching core stops at a **static** link. A *dynamic* executable adds a
second machine that this project deliberately omits — here is the honest gap:

- **`PT_INTERP`** — a dynamic executable carries the path of the dynamic loader
  (`/lib64/ld-linux-x86-64.so.2`) in a `PT_INTERP` segment; the kernel maps and
  jumps to *it* first, not to `_start`. `minild` writes no `PT_INTERP`, so the
  kernel runs our entry directly.
- **GOT / PLT** — cross-*library* calls can't be bound at link time (the callee's
  address isn't known until load), so the linker emits a **PLT** stub per callee
  and a **GOT** slot the loader fills in. We resolve calls *directly* because in
  a static link every address is final; that is why we treat `PLT32` exactly
  like `PC32`.
- **`.dynsym` / `.dynamic` / dynamic relocations** — `R_X86_64_GLOB_DAT`,
  `JUMP_SLOT`, and `RELATIVE` are applied by `ld.so` at startup against a
  `.dynamic` table and a dynamic symbol table. `minild` applies only the
  *static* relocation set, at link time.
- **PIE** — a position-independent executable is an `ET_DYN` with a `_start` and
  is loaded at a randomised base (that is ASLR for the main binary). We emit a
  fixed-address `ET_EXEC` at `0x400000` on purpose, so the addresses in
  `objdump` are stable and the `R_X86_64_32S` absolute relocation is even legal.

`Stretch:` grow the core toward dynamic — write a `PT_INTERP`, synthesise a
minimal PLT/GOT for one external call, and emit `R_X86_64_JUMP_SLOT` relocations
so `ld.so` binds a libc symbol. Read `lld`'s `Writer.cpp` and `Relocations.cpp`,
or GNU `ld`'s `bfd`, to see how the real thing scales this up.

### Defense: a linker is where mitigations are placed

Exploitation is about seizing control of a process's memory; a linker *decides*
that memory map, so it is exactly where the classic mitigations are installed.
This project makes them visible by choosing the insecure/simple option and
saying so:

- **W^X (no writable+executable segment).** `minild` groups sections by
  permission into R|X, R, and R|W `PT_LOAD`s — never R|W|X. This is the
  foundation of **NX/DEP**: injected shellcode on the stack or heap can't
  execute because those pages aren't executable. See `09-shellcode` and
  `10-memory-corruption-ladder` for what NX forces an attacker to do instead
  (ROP/ret2libc). A linker that emits an executable, writable segment (or an
  executable stack via `PT_GNU_STACK` with `PF_X`) throws that away — which is
  why `06-assembler`/`01-nolibc` record a **non-executable** `.note.GNU-stack`.
- **ASLR / PIE.** We build a **fixed-base `ET_EXEC`** for legibility, which
  means *no* address randomisation: every run puts `_start` at `0x4000f0`. A
  production link defaults to **PIE** (`-pie`, `ET_DYN`), so the loader picks a
  random base and an attacker can no longer hardcode gadget addresses. The cost
  to the attacker is an **info-leak requirement**; the cost to you is one GOT
  indirection. This project shows the "before".
- **RELRO.** With a GOT (dynamic case), **full RELRO** has the linker place the
  GOT so the loader can `mprotect` it **read-only after startup**, closing GOT-
  overwrite attacks. `minild` has no GOT, so there is nothing to protect — but
  the *reason* production linkers add a `PT_GNU_RELRO` segment is this.
- **Stack canaries.** Not a linker feature per se, but the linker must resolve
  `__stack_chk_fail`; our examples use `-fno-stack-protector` precisely because
  they are nolibc and there is no such symbol to bind. Canaries are covered in
  `10-memory-corruption-ladder`.

The lesson: every one of these is a property the linker *writes into the ELF*.
Knowing that is what lets you audit a binary (`readelf -l`, `checksec`) and say
"this segment is RWX — that's a problem," or "no PIE, no RELRO — this is a soft
target."

## References

- **System V AMD64 psABI** — the relocation table (`R_X86_64_*` and their
  `S`/`A`/`P` formulae) and the small/large code models. Ground truth.
- **ELF spec (gABI)** — `Elf64_Ehdr/Shdr/Sym/Rela/Phdr`, section vs. segment,
  symbol binding/precedence. `man 5 elf` is a good condensed version.
- **`man ld`, `man 8 ld.so`** — real static/dynamic linking; the mitigation
  flags (`-z relro`, `-z noexecstack`, `-pie`).
- **Read the source of the real thing:** LLVM `lld` (`lld/ELF/`, especially
  `Relocations.cpp`, `Writer.cpp`, `SyntheticSections.cpp`), or GNU `ld`/`bfd`.
- John R. Levine, *Linkers and Loaders* — the book on exactly this.
- Neighbours in this section: `05-x86-64-disassembler`, `06-assembler`,
  `09-shellcode`, `10-memory-corruption-ladder`.
