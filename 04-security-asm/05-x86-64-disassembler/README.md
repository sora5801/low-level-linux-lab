# x86-64 disassembler 🟥

**What it is.** A from-scratch **linear-sweep disassembler** for the integer
subset of x86-64: it walks a raw byte stream and turns each instruction back
into text, in **AT&T** (objdump's default) or **Intel** syntax. It decodes legacy
prefixes, the REX prefix, the primary and `0F` opcode maps, and — the hard part —
the **ModR/M + SIB + displacement** machinery that makes x86 a variable-length
encoding, including RIP-relative addressing and the RSP/RBP encoding "escapes."
Every instruction it prints has been checked against `objdump -d` as the oracle.

This is a **teaching core**, not a complete decoder: it aims to be *correct and
fully explained* on the instructions a C compiler actually emits, and *honest*
about everything it leaves out (see the coverage table). It is a 🟥 because a
production decoder (see the LLVM/Zydis pointers) is an order of magnitude larger.

> **On your own machines / authorized targets only.** A disassembler is a
> *defensive* and reverse-engineering tool. Point it at binaries **you** built or
> are **authorized to analyze** (your own programs, CTF targets, malware in a lab
> you control). The whole lab is legal, on-your-own-box learning.

## What you'll learn

- **x86-64 instruction anatomy**, in decode order:
  `prefixes → REX → opcode → ModR/M → SIB → displacement → immediate`.
- **The REX prefix** (`0100 WRXB`): how `W` selects 64-bit operands and how
  `R/X/B` each extend a 3-bit register field to reach `r8..r15`.
- **ModR/M + SIB** — the crux — and its three "stolen" encodings:
  - `mod≠11 && rm==100` → a **SIB** byte follows (RSP can't be a plain base);
  - `mod==00 && rm==101` → **RIP-relative** `disp32` (no base register);
  - in SIB, `base==101 && mod==00` → **no base**, a lone `disp32`; and
    `index==100 && REX.X==0` → **no index** (but `REX.X==1` makes it `r12`).
- **Operand-size rules**: the 32-bit default, `REX.W`→64, `0x66`→16, and the
  `d64` stack/branch ops (`push`/`pop`/`call`/`jmp`) that default to 64.
- **Displacement/immediate sign-extension** and how objdump prints it.
- **AT&T vs Intel**: operand order, the `%`/`$` sigils, AT&T size **suffixes**
  (`movl`, `incq`) vs Intel **`ptr` hints**, and RIP-relative target comments.
- **Why disassembly is hard to trust** — the defense lesson below.

## Build & run

Builds anywhere clang runs (the decoder is pure computation — no OS calls). The
committed teaching assembly cross-targets Linux; the tool itself is host-portable.

```bash
make                 # build ./disasm
make run             # decode a small demo stream
make test            # self-check key encodings (no external tools) — a gate
make oracle          # diff our output vs `objdump -d` (needs binutils + xxd)
make asm             # regenerate asm/demo.{O0.s,s,O2.s}
```

Use it directly — hex on the command line, hex on stdin, or a raw blob:

```bash
./disasm 48 89 e5                 # mov %rsp,%rbp
./disasm --intel 48 8b 45 f8      # mov rax,[rbp-0x8]
echo "0f 05" | ./disasm           # syscall
./disasm --base 0x401000 -f code.bin   # sweep a flat binary at a chosen VA
```

To carve `.text` out of an ELF and feed it in:

```bash
objcopy -O binary -j .text ./a.out text.bin
./disasm --base $(readelf -h ./a.out | awk '/Entry/{print $NF}') -f text.bin
```

## How it works

Four files, split so the gnarly encoding logic lives in one place and the two
output syntaxes in another:

- **`disasm.h`** — the data model: `Insn` (prefixes, REX, opcode, ModR/M/SIB, and
  a decoded `Operand[3]`) and the `OpKind` operand tag. Reading this header is the
  fastest way to see the shape of an x86 instruction.
- **`disasm.c`** — the decoder + formatter, in six sections:
  1. **name tables** (register/segment/condition-code text);
  2. **the opcode model** — `lookup_primary`/`lookup_0f` map an opcode byte to a
     mnemonic + operand *types*. The regular families (the 8×8 ALU grid, `push`/
     `pop`, `jcc`, `mov r,imm`) are handled by range so you can *see* the
     regularity; the irregular opcodes are a `switch`. Reg-field **groups**
     (`80/81/83`, shifts, `F6/F7`, `FE/FF`, `C6/C7`) pick their mnemonic from the
     ModR/M `reg` field via `resolve_group`.
  3. byte-reading helpers (bounds-checked little-endian + sign-extend);
  4. **`decode_rm` — the heart**: ModR/M + SIB + displacement (the three escapes);
  5. **`disasm_one`** — the one linear pass that ties it together;
  6. **formatting** — AT&T and Intel over the same decoded operands, with the
     size-suffix / `ptr`-hint logic and RIP-relative target comments.
- **`main.c`** — the CLI and the **linear sweep**: decode, print, advance by the
  instruction's length, repeat (exactly what `objdump -d` does).

### Coverage (honest scope)

| Area | Decoded | Not decoded (yet) |
|---|---|---|
| Prefixes | legacy groups 1–4, REX | VEX/EVEX/XOP (AVX), mandatory-prefix SSE forms |
| ALU | `add/or/adc/sbb/and/sub/xor/cmp` (all forms), `inc/dec/neg/not` | — |
| Data move | `mov`, `movabs`, `movzx/movsx/movsxd`, `lea`, `xchg`, `push/pop` | string ops (`movs/stos/...`), `bswap` |
| Mul/div | `mul/imul` (incl. 3-operand), `div/idiv` | — |
| Shifts | `rol/ror/rcl/rcr/shl/shr/sar` (`imm8`, `1`, `cl`) | `shld/shrd`, `bt*` |
| Control | `jmp/jcc` (short+near), `call/ret`, `leave`, indirect `call/jmp` | far/`retf`, `loop`, `enter` |
| Compare | `test`, `cmp`, `setcc`, `cmovcc` | — |
| Misc | `nop` (incl. multi-byte), `syscall`, `cpuid`, `rdtsc`, `int3`, `hlt`, `ud2`, `cltq/cqto` family | x87, SSE/AVX, MMX, `0F 38`/`0F 3A` maps |
| Addressing | ModR/M, SIB, `disp8/32`, RIP-relative, segment overrides, `0x67` | — |

Unsupported bytes print `(bad)` and the sweep advances one byte — the same
graceful-degradation `objdump` uses.

**Validation.** Against `objdump -d … -M att`, the AT&T output matches on the
whole sample set (`make oracle`), including the R13/RBP/RSP/R12 quirks, REX
extensions, `movabs`, three-operand `imul`, `endbr64`, and RIP-relative comments.
Two deliberate, purely-cosmetic differences remain:

- When a **SIB byte encodes no index** but a shorter non-SIB encoding of the same
  address exists (e.g. `44 25 10` for `0x10(%rbp)`), objdump prints a phantom
  `%riz`/`%eiz` "zero index" (`0x10(%rbp,%riz,1)`) to flag that the longer
  encoding was used. We render the effective address directly (`0x10(%rbp)`),
  which is equally correct; the byte count and semantics are identical.
- Intel output follows the LLVM/NASM convention (lowercase `qword ptr`, signed
  `[rip-0x7]`) rather than objdump's uppercase `QWORD PTR` / unsigned form.

## Assembly notes

The generated assembly is for **`asm/demo.c`**, a standalone, header-free copy of
the decoder's heart — `decode_modrm`, which answers "is there a SIB? a
displacement? which base/index/scale?" It is annotated in full in
[`asm/demo.annotated.s`](asm/demo.annotated.s). Highlights the `-O1` asm teaches:

- **`sret`**: the 64-byte `amode` struct is returned via a hidden pointer in
  `%rdi`, so every real argument shifts to `rsi/edx/ecx/r8d/r9d`.
- The optimizer **zero-inits the struct with three SSE stores** and writes the
  two adjacent `int` fields `has_disp`+`disp_size` as **one 8-byte `movabsq`**.
- **Sign-extension two ways**: `movslq`+high-half-mask for the `disp32`, and a
  branchless `setns` trick for the `disp8`.
- The three ModR/M **escapes** are exactly the branches at `.LBB0_3` (SIB),
  `.LBB0_18` (RIP-relative), and `.LBB0_9/.LBB0_10` (no-base/no-index).

Compare [`asm/demo.O0.s`](asm/demo.O0.s) (naive, everything spilled),
[`asm/demo.s`](asm/demo.s) (`-O1`, annotated baseline), and
[`asm/demo.O2.s`](asm/demo.O2.s). Regenerate with `make asm`.

## Going further

**Stretch: linear sweep vs recursive descent, and a tiny CFG.** Linear sweep
(what we do) decodes bytes in order and cannot tell code from data — so a jump
table or an inline constant in `.text` will **desynchronize** the stream and
every subsequent line is garbage until it happens to re-align. **Recursive
descent** instead *follows control flow*: start at known entry points, decode
until a branch, then queue the branch targets (and the fall-through) as new
starting points; bytes never reached by control flow are treated as data. Chain
the decoded blocks by their edges and you have a **control-flow graph** — the
foundation of every real analysis tool. To build it here: change `main.c` to keep
a work-list of addresses, mark visited ranges, and for each `jmp/jcc/call` push
`insn.ops[0].target` (already computed) and, for conditionals/calls, the
fall-through `addr+len`. Stop a block at `ret/jmp/hlt/ud2`.

### DEFENSE — why a decoder is a blue-team tool

Disassembly is the first step of malware analysis, vulnerability research, and
detection engineering — you cannot reason about a binary you can't read. But the
*variable-length* encoding this project exposes is also an **anti-analysis
weapon**, and knowing the decoder from the inside is what lets you defend:

- **Anti-disassembly via desync.** An attacker inserts a junk byte (e.g. a lone
  `0xEB`/`0xE8` opcode, or a jump *into the middle* of a longer instruction) so a
  linear sweep locks onto the wrong byte boundary and shows innocuous-looking
  (or nonsensical) code, while the CPU — which only ever executes from real entry
  points — runs something else. **Overlapping instructions** (two valid decodings
  of the same bytes depending on where you start) are the classic trick. The
  defense is exactly the *Going further* item: **recursive-descent** disassembly
  that follows real control flow resynchronizes where linear sweep is fooled;
  disagreement between the two is itself a strong signal to flag for review.
- **Encoding-level detection.** Because you decode ModR/M/SIB yourself, you can
  spot *how* something is written, not just *what* it does: RWX-adjacent
  `syscall` sequences, `mov`-to-`%cr`/`%dr` (privileged), unusual segment
  overrides (`%fs`/`%gs` stack pivots), or shellcode that avoids NUL bytes. These
  are patterns an opaque tool hides from you.
- **It pairs with the lab's mitigations.** A disassembler is how you *verify*
  them: `endbr64` at every indirect-call target (CET), no `-z execstack` segment,
  PIE (position-independent, hence the RIP-relative addressing you see here), and
  RELRO. Reading the bytes is how you confirm a build actually got hardened.

Treat any single disassembly as a *hypothesis*, not ground truth — the encoding
is adversarial, and the same bytes can mean different things from different
offsets. That humility is the real lesson.

## References

- **Intel SDM Vol. 2**, §2.1–2.2 (instruction format, ModR/M, SIB, REX) and the
  opcode-map appendix — ground truth for every table here.
- **AMD64 APM Vol. 3** — the other authoritative encoding reference.
- **System V AMD64 ABI** (psABI) — register/stack conventions the asm relies on.
- `objdump -d` / **GNU binutils `opcodes/i386-dis.c`** — the oracle, and a real
  linear-sweep decoder to read.
- **LLVM `MC` disassembler** and **Zydis** / **Capstone** — production decoders;
  see how much table machinery a *complete* implementation needs.
- Kruegel et al., *"Static Disassembly of Obfuscated Binaries"* (USENIX '04) — the
  canonical treatment of anti-disassembly and linear-sweep vs recursive-descent.
