# An ELF toolkit 🟧

**What it is.** `elftk` is a from-scratch reimplementation of the parts of
`readelf`, `nm`, and `objdump -d` that matter most for understanding a Linux
binary — plus a tiny `addr2line`. It memory-maps an ELF64 object and parses the
**ELF header, section headers, program headers, the symbol and string tables,
the dynamic section, and relocations**, printing them the way `readelf -a` does.
It then linear-sweeps `.text` with its own **x86-64 disassembler backend** that
decodes the common instruction subset a C compiler emits, annotating branch
targets with symbol names. Difficulty: 🟧 — a real, working teaching *core* (it
covers the mainstream cases and is honest, below, about what it omits).

Every structure is parsed straight out of the mapped file with bounds checks, so
the code doubles as a tour of the on-disk ELF format and of the one rule every
file parser lives by: **never trust a length field from an untrusted file.**

## What you'll learn

- The **ELF64 object model** end to end: how `Elf64_Ehdr` → section headers →
  symbol tables → string tables → relocations reference each other by index and
  offset, and the two *views* of a file (the linker's **sections** vs. the
  kernel's **segments**/program headers).
- The **ELF quirks** real toolchains actually emit: the `e_shnum == 0` escape
  (real count in `sh[0].sh_size`) and `e_shstrndx == SHN_XINDEX`.
- How **`nm` derives its one-letter type code** (`T/t D/d B/b R/r U W C …`) from
  a symbol's binding crossed with the flags of the section that defines it.
- How **relocations** encode "patch here with S + A": `r_info` packs a symbol
  index and an architecture-specific type (`R_X86_64_PC32`, `PLT32`, …).
- The **x86-64 instruction format** — legacy prefixes, REX, opcode, ModRM, SIB,
  displacement, immediate — and why **linear-sweep** disassembly lives or dies on
  getting each instruction's *length* exactly right.
- **Symbolization by binary search**: sort symbols by address once, then answer
  "which function owns this PC?" in `O(log n)` — the core of every backtrace,
  profiler, and `addr2line`.
- `mmap(2)` as a zero-copy way to treat a file as a `struct` (and why the fixed
  widths and natural alignment in `elf.h` make that cast sound).

## Build & run (Linux / WSL)

```bash
make                       # builds ./elftk  (needs a Linux host: uses mmap/open)
make check                 # smoke test: elftk reads its OWN binary
```

Usage mirrors the real tools' flag names:

```bash
./elftk -h  <file>         # ELF header            (readelf -h)
./elftk -S  <file>         # section headers       (readelf -S)
./elftk -l  <file>         # program headers + section→segment map (readelf -l)
./elftk -d  <file>         # .dynamic array        (readelf -d)
./elftk -r  <file>         # relocations           (readelf -r)
./elftk -s  <file>         # .symtab and .dynsym   (readelf -s)
./elftk -a  <file>         # all of the above      (readelf -a)
./elftk nm  <file> [-n]    # symbol list, nm-style (-n: sort by address)
./elftk -D  <file>         # disassemble .text     (objdump -d, Intel syntax)
./elftk addr2line <file> <hex-addr>   # symbol containing an address
```

Point it at anything: `./elftk -a /bin/ls`, a `.o` from `clang -c`, or a
`.so`. Diff against the real tool to check us: `diff <(./elftk -h /bin/ls)
<(readelf -h /bin/ls)`.

To regenerate the committed teaching assembly on any host (clang cross-targets
Linux): `make asm`.

## How it works (file by file)

- **`elf.h`** — a self-contained ELF64 layout written from the gABI/x86-64 psABI:
  every `Elf64_*` struct, the `SHT_/PHT_/STT_/STB_/DT_/R_X86_64_*` constants, and
  `_Static_assert`s that the struct sizes match the spec (64/56/24/16 bytes) so a
  bad layout fails the build, not at run time.
- **`elftk.h` / `elftk.c`** — the loaded-file model (`struct elf_file`) and the
  loader: `mmap` the file read-only, validate the `e_ident` magic/class/endianness,
  then resolve the section header table, `.shstrtab`, and both symbol tables once
  (handling the `SHN_XINDEX`/`e_shnum==0` escapes). `ef_fits`/`ef_ptr`/`ef_str`
  are the bounds-checked primitives every other file uses. `main()` is the CLI
  dispatcher.
- **`readelf.c`** — the `readelf -a` dumps: header, section table, program headers
  with the section→segment mapping, the dynamic array (resolving `DT_NEEDED`
  library names through `.dynstr`), the RELA relocation tables (resolving symbol
  names, including `STT_SECTION` symbols), and the symbol tables. Mostly integer →
  mnemonic-name lookup tables.
- **`nm.c`** — the `nm` letter algorithm (`nm_letter`) and a name/address sort.
- **`disasm.h` / `disasm.c`** — the linear-sweep x86-64 decoder. **No system
  headers**, its own types and string builder, so it is a pure function over a
  byte buffer (and its teaching asm is generated too). `x86_decode` walks
  prefixes → REX → opcode → ModRM/SIB/disp/imm, computing the exact length and
  rendering Intel-syntax text; RIP-relative operands get their absolute target
  resolved into a trailing `# 0x…` comment.
- **`objdump.c`** — builds the address-sorted symbol index, does the linear sweep
  over `.text` (printing `<function>:` labels and annotating resolved branch
  targets), and implements `addr2line` via the shared `sym_lookup` binary search.

## Assembly notes

`asm/demo.c` extracts the two most instructive pure-logic routines and
`asm/demo.annotated.s` walks the `-O1` output instruction by instruction:

- **`sym_by_addr`** — the "find the rightmost symbol ≤ addr" **binary search**
  (identical to `objdump.c::sym_lookup`). The annotation shows clang lowering the
  signed midpoint `lo + (hi-lo)/2` with the `x>>31` round-toward-zero fixup, and
  why it stays branchy (both arms move the bounds, not just one value).
- **`x86_insn_len`** — the **opcode-length decoder** (the length half of
  `x86_decode`), plus its helper `modrm_bytes`. This is where the optimizer
  shows off: the legacy-prefix set becomes a 64-bit **bitmap tested with `btq`**,
  the ModRM "add a disp32 iff RIP-relative" becomes branchless `sete`+`lea`, and
  the truncation guard becomes a `cmov`. `modrm_bytes` and the two clean routines
  are annotated in full; the large optimized `x86_insn_len` body has its every
  *distinct* block annotated with the duplicated tails clearly marked (see the
  header of the annotated file, and `asm/demo.O0.s` for the 1:1 statement map).

`disasm.c` is itself self-contained, so `asm/disasm.{O0,,O2}.s` are generated
from the real backend too.

## Going further (the `Stretch:` from the list)

- **DWARF line info for a true `addr2line`.** Our `addr2line` is *symbol-based*
  (`function+0xoffset`); a real one parses `.debug_line`'s bytecode program to map
  an address to `file:line`. That means decoding the line-number state machine
  (`DW_LNS_*` opcodes, the `is_stmt`/`address`/`line` registers) — a great next
  project on the same mapped file.
- **A fuller disassembler.** Extend `disasm.c` past the C-compiler subset: the
  complete SSE/AVX (VEX/EVEX) maps, the 3-byte `0F 38/3A` opcodes, and AT&T
  output. Then add **recursive-descent** decoding to avoid disassembling inline
  data (jump tables, literal pools) as instructions.
- **Big-endian / ELFCLASS32.** We only handle little-endian ELF64 (x86-64). A
  production `readelf` byte-swaps every field and handles 32-bit structs.
- What production does: GNU binutils (`bfd`), LLVM's `llvm-readobj`/`llvm-objdump`
  (which share the decoder with the assembler), and `elfutils`.

### Scope / honesty

This is a teaching core, so it deliberately omits: ELF32 and big-endian; note
(`SHT_NOTE`) and version (`GNU_versym`/`verneed`) section decoding; the `REL`
(addend-less) relocation form (x86-64 uses `RELA`); DWARF; and the long tail of
the x86 ISA. The disassembler targets the instructions `clang`/`gcc` actually
emit into `.text`; unrecognized encodings are length-decoded where their class is
clear and shown as `(bad)` otherwise, so a linear sweep resynchronizes rather
than runs away. Bounds are checked everywhere: a malformed file yields an error,
never an out-of-bounds read.

## References

- **System V gABI** (ELF container) and the **x86-64 psABI** (relocations,
  `EM_X86_64`) — ground truth for every struct and constant in `elf.h`.
- Intel SDM Vol. 2, ch. 2 (instruction format: prefixes/REX/ModRM/SIB) — the
  rules `disasm.c` implements.
- `man 5 elf`, `man 1 readelf`, `man 1 nm`, `man 1 objdump`, `man 2 mmap`.
- The real sources: GNU **binutils** (`bfd/`, `binutils/readelf.c`) and LLVM
  (`llvm-readobj`, `llvm-objdump`, `X86Disassembler`).
