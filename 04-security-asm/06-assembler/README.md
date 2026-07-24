# An assembler 🟧

**What it is.** `masm` is a small but real x86-64 assembler. It parses a subset
of **AT&T-syntax** assembly — labels, the directives `.text` / `.data` /
`.globl` / `.byte` / `.quad`, and a working instruction set (`mov`, `add`/`sub`,
`cmp`, `push`/`pop`, `lea`, `jmp`/`jcc`, `call`, `ret`, `syscall`) — and emits a
**relocatable ELF64 object file** (`.o`) with a real symbol table and
relocation entries. The object links with the system `ld` into a runnable
program, and it round-trips through `objdump`/`readelf` (and the disassembler in
[`../05-x86-64-disassembler`](../05-x86-64-disassembler)) byte for byte. Forward
label references (`jmp done` before `done:` exists) are handled with classic
**two-pass** assembly.

> **On your own machine, for code you wrote.** An assembler is a neutral build
> tool, but this section is security-focused, so it is worth stating: assemble
> and run only programs you authored, on hardware you control. Nothing here
> weakens a mitigation — the object it writes even carries the standard
> non-executable-stack marker (see the defense note below).

## What you'll learn

- **x86-64 instruction encoding** from the metal up: the `[REX] opcode [ModR/M]
  [SIB] [disp] [imm]` byte stream, how `mov %rsi,%rdi` becomes `48 89 f7`, and
  the ModR/M gotchas that trip everyone (`%rsp`/`%r12` force a SIB byte;
  `%rbp`/`%r13` cannot use the zero-displacement form; `r8`–`r15` need REX.R/B).
- **The symbol table**: how labels become `(section, offset)` pairs and how a
  name you *reference* but never *define* (e.g. `call printf`) becomes an
  **undefined global** the linker must satisfy.
- **Relocations**: why the assembler *cannot* fill in a cross-section data
  address or an external call, and what note it leaves instead —
  `R_X86_64_PC32` / `R_X86_64_PLT32` with the `S + A − P` arithmetic and the
  `−4` addend that accounts for a rel32 sitting at the end of the instruction.
- **Two-pass assembly**: pass 1 lays out sizes and addresses; pass 2 emits bytes
  and resolves the forward references pass 1 discovered.
- **The ELF64 object container**: the header, section headers, `.symtab` /
  `.strtab` / `.shstrtab`, and `.rela.text`, written field-by-field.

## Build & run

`masm` is a host tool that *writes* Linux ELF bytes, so it **builds on any OS**
(it never calls a Linux-only API). **Linking and running** the produced object
needs an ELF toolchain — **Linux or WSL**.

```bash
make                       # builds ./masm   (clang -Wall -Wextra)
make run                   # dots.s -> dots.o -> (ld) -> ./dots   [Linux/WSL]
#   prints:  .......... (exit=0)
make test                  # cross-check encodings vs GNU `as`; objdump round-trip
make asm                   # regenerate asm/demo.{O0.s,s,O2.s}
```

Assemble and inspect by hand:

```bash
./masm examples/dots.s -o dots.o -v      # -v prints symbols + relocations
readelf -SsrW dots.o                      # sections, symbols, relocations
objdump -dr dots.o                        # disassemble; note the R_X86_64_PC32
ld dots.o -o dots && ./dots               # link (no libc) and run   [Linux]
```

The `-v` listing on `dots.s` shows exactly what an assembler produces:

```
-- symbols --
  _start           GLOBAL  .text+0x0
  loop             LOCAL   .text+0x11
  done             LOCAL   .text+0x3d
  dot              LOCAL   .data+0x0
-- relocations (.text) --
  0x0022  PC32   dot  addend -4
```

## How it works

The pipeline is linear — `read → parse → assemble → write_elf` — split across
small files so each idea stands alone:

- **`asm.h`** — the shared types (operands, statements, symbols, relocations,
  the `Assembler` context) and a heavily-commented reference for the ELF and
  encoding constants used throughout.
- **`buf.c`** — a growable byte buffer with explicit **little-endian** writers.
  Every multi-byte value on disk is decomposed here, byte by byte — never a
  `memcpy` of a C struct — so the layout is correct on any host.
- **`lex.c`** — the line-oriented parser: it peels off `label:` prefixes, reads
  a mnemonic or directive, and parses each operand into one of `%reg`, `$imm`,
  `disp(%base)` / `sym(%rip)`, or a bare target symbol.
- **`encode.c`** — the **encoder**, the heart of the project. One code path
  either *counts* bytes (pass 1) or *emits* them (pass 2), which guarantees a
  size can never disagree with the bytes produced. It builds REX/opcode/ModR/M/
  SIB/disp for every supported form and decides, per reference, whether a symbol
  reference resolves locally or becomes a relocation.
- **`assemble.c`** — the symbol table and the **two-pass driver**: pass 1 places
  every label; `apply_globals` fixes bindings; pass 2 emits and back-fills every
  forward branch.
- **`elf.c`** — the **ELF64 relocatable object writer**: it builds `.symtab`
  (locals before globals, `sh_info` at the first global), `.strtab`,
  `.rela.text`, and `.shstrtab`, computes aligned file offsets, and writes the
  header + 7 section headers. Opened in binary mode so no byte is ever
  translated.
- **`main.c`** — the CLI (`-o`, `-v` listing, `-d` hex dump of `.text`).

### The one resolution rule worth memorising

When a `jmp`/`call`/`lea` names a symbol, `masm` resolves it **in place** only
if the target is a **local, defined label in the same section** — because a
PC-relative distance *within one section* is invariant under wherever that
section finally loads. Everything else (an external name, a **global** label
that could be interposed at link time, or a **cross-section** data address like
`lea dot(%rip)`) gets a **relocation** instead. That single rule is why
`dots.s`'s local `je`/`jne` need no relocation, while its `lea dot(%rip)` emits
one — exactly matching what GNU `as` does.

### Honest scope (it is a teaching core, not GNU as)

- **Fixed-size branches.** Every `jmp`/`jcc`/`call` is a 5–6-byte rel32. Real
  assemblers do **branch relaxation** — they iterate passes to pick the 2-byte
  short form when the target is near. We trade that for a single, exact,
  non-iterating layout pass. (`make test` shows the *only* byte difference from
  GNU `as` on the harness is precisely this short-vs-long branch choice.)
- **64-bit operands only.** Instructions carry `REX.W`; there is no 8/16/32-bit
  form, no operand-size or address-size prefix handling.
- **`.byte`/`.quad` take integer literals** (no `.quad symbol`, which would need
  an `R_X86_64_64` in `.rela.data`); `add`/`sub`/`cmp` take reg,reg or imm,reg
  (no memory operands); one instruction per line.
- **Named-symbol relocations.** For a local cross-section reference, GNU `as`
  routes the relocation through the *section symbol* plus an addend; `masm`
  references the **named** symbol directly. Both are valid ELF and `ld` accepts
  either.

## Assembly notes

Because the source is C, the teaching assembly is generated from a self-
contained extraction, [`asm/demo.c`](asm/demo.c), which pulls out the two purest
ideas: `encode_mov_rr()` (the REX + opcode + ModR/M encoder for `mov %r,%r`) and
`backpatch_rel32()` (the two-pass forward-branch fill-in).

[`asm/demo.annotated.s`](asm/demo.annotated.s) walks the `-O1` output line by
line. Highlights the compiler reveals:

- The REX byte is built with `setge`/`shl`/`or`, and the **whole ModR/M byte is
  folded into a single `lea (%rdx,%rsi,8)` + `or $0xC0`** — a neat trick where
  the `0xC0` (mod=11) bits happen to mask away a stray high bit of the source.
- The rel32 backpatch is exactly `target − (field + 4)` stored little-endian —
  visible as the `movb %dl` / `%dh` / `(rel>>16)` / `(rel>>24)` stores.
- **`main` constant-folds to `return 0`.** clang ran both encoders on their
  literal arguments, checked every expected byte (`48 89 f7`, `4d 89 c7`,
  `05 00 00 00`), and proved the self-test passes — so the `-O1`/`-O2` files
  *are themselves* a correctness proof of `demo.c`. Compare with
  [`asm/demo.O0.s`](asm/demo.O0.s), where nothing is folded and every value is
  spilled to the stack.

## Defense note — why encoding is a blue-team skill

This assembler is the *constructive* half of the disassembler in `../05`, and
that pairing is the lesson. A defender who can encode can also **read**:

- **Spotting a smuggled syscall.** `syscall` is just the two bytes `0F 05`. A
  scanner that understands encoding can flag `0F 05` (or an `int 0x80` / `CD 80`)
  appearing in a data buffer or a JIT page — the raw signal behind many
  sandbox-escape and shellcode detections.
- **Understanding NX / W^X.** The object masm writes ends with
  `.note.GNU-stack` marking the stack **non-executable**. Knowing that the CPU
  will refuse to fetch instructions from a writable page — and that an attacker
  therefore has to *reuse existing bytes* (return-oriented programming, see
  `../10`) — starts with knowing what a valid instruction byte stream even looks
  like, which is exactly what this project makes concrete.
- **Byte-level equivalence checking.** Re-encoding a suspicious disassembly and
  diffing the bytes (as `make test` does against GNU `as`) is how you catch
  anti-disassembly tricks: overlapping instructions, or a jump into the *middle*
  of an encoded instruction that means something different on the second read.

## Going further

- **`Stretch:` branch relaxation.** Add the iterate-until-stable pass so nearby
  branches shrink to the 2-byte `rel8` form (`EB cb` / `7x cb`). This is where
  layout stops being a single pass — instruction sizes now depend on distances,
  which depend on sizes.
- **More of the ISA**: operand-size prefixes for 8/16/32-bit forms, memory
  operands for the ALU ops, a real SIB with index+scale, `.quad symbol` with an
  `R_X86_64_64` relocation, and `.section`/`.align`/`.ascii`.
- **What production does.** GNU `as` (`binutils/gas`) and LLVM's integrated
  assembler (`llvm/lib/MC`) turn the same tables into an `MCInst` stream, do
  relaxation and fixups, and emit via a full `MCObjectWriter`. Read
  `gas/config/tc-i386.c` for the encoder and `bfd/elf64-x86-64.c` for the
  relocation semantics.

## References

- Intel® 64 and IA-32 SDM, Vol. 2 — instruction encoding, ModR/M and SIB tables.
- System V AMD64 psABI — the ELF object format, the symbol/relocation tables,
  and the `R_X86_64_*` relocation computations.
- `man 5 elf`; `readelf`, `objdump -dr`, `objcopy` from GNU binutils.
- Sibling projects: [`../05-x86-64-disassembler`](../05-x86-64-disassembler)
  (the inverse) and [`../07-linker`](../07-linker) (what consumes this output).
