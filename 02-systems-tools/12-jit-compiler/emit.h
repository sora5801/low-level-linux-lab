/* ===========================================================================
 * emit.h — a tiny, self-contained x86-64 machine-code *encoder*.
 * ===========================================================================
 *
 * WHAT THIS IS
 * ------------
 * A JIT compiler's job is, at its core, to write the exact bytes the CPU will
 * later fetch and execute. There is no assembler in the loop at runtime: WE are
 * the assembler. This header declares an "append bytes to a growing buffer"
 * abstraction plus one small function per x86-64 instruction we need.
 *
 * WHY IT HAS NO #includes
 * -----------------------
 * This file (and emit.c) deliberately pull in *no* system or libc headers. That
 * has two payoffs:
 *   1. It is the project's "pure logic" core, so `tools/gen_asm.sh emit.c`
 *      produces clean teaching assembly with nothing from libc mixed in.
 *   2. It makes the trust boundary obvious: the encoder only ever *writes into a
 *      caller-provided buffer*. It never mmaps, never mprotects, never calls
 *      into the code it builds. Those privileged/dangerous steps live in jit.c.
 *
 * We therefore spell out our own fixed-width integer types below. On the LP64
 * data model Linux uses for x86-64: char=8, int=32, long=64 bits.
 * ===========================================================================
 */
#ifndef BF_EMIT_H
#define BF_EMIT_H

typedef unsigned char  u8;   /* one machine-code byte                          */
typedef unsigned int   u32;  /* a 32-bit immediate / displacement (rel32)      */
typedef signed   int   i32;  /* signed 32-bit (pointer deltas, jump rel32)     */
typedef unsigned long  u64;  /* a byte count / buffer offset (64-bit on LP64)  */

/* ---------------------------------------------------------------------------
 * code_buf — an append-only cursor over a fixed-capacity byte buffer.
 *
 * OWNERSHIP: `buf` is NOT owned here. The caller (jit.c) allocates it — for a
 * real JIT that means an mmap'd, page-aligned region that will later be flipped
 * to executable. The encoder only advances `len` and writes bytes; it never
 * frees, grows, or reallocates. That keeps the encoder allocation-free and
 * trivially auditable.
 *
 * `overflow` is a sticky error flag. Rather than aborting (we have no libc to
 * abort *with*), every writer checks capacity first and, if a write would run
 * past `cap`, sets `overflow` and drops the write. The JIT sizes the buffer
 * with a proven upper bound, so in correct operation `overflow` stays 0 — but
 * checking it after emission turns "silent buffer overrun that corrupts the
 * heap and then gets executed" (a catastrophic bug in a code generator) into a
 * clean, detectable error.
 * --------------------------------------------------------------------------- */
typedef struct code_buf {
    u8  *buf;       /* base of the caller-owned output buffer                  */
    u64  len;       /* bytes written so far == offset of the next write        */
    u64  cap;       /* capacity in bytes; writes past this are refused         */
    int  overflow;  /* sticky: set to 1 the first time a write is refused      */
} code_buf;

/* ---- primitive writers ---------------------------------------------------- */

/* Append one raw opcode/ModRM/immediate byte. Every other emit_* is built on
 * this. Little-endian host assumed (x86-64 is always little-endian). */
void emit_u8 (code_buf *c, u8  b);

/* Append a 32-bit value in LITTLE-ENDIAN order (low byte first). x86-64 encodes
 * every multi-byte immediate and displacement little-endian, so this is the
 * only byte order we ever need. */
void emit_u32(code_buf *c, u32 v);

/* ---- ModRM/REX construction (the shared grammar of x86-64) ---------------- */

/* Build a ModRM byte:  [ mod:2 | reg:3 | rm:3 ].  The ModRM byte is how almost
 * every instruction names its operands. `mod` selects the addressing form
 * (0b11 = operand is a register; 0b00 = memory at [rm] with no displacement),
 * `reg` is usually the register operand OR an opcode extension (/0../7), and
 * `rm` is the second register or the base of a memory operand. */
u8   modrm(u8 mod, u8 reg, u8 rm);

/* ---- the specific instructions the Brainfuck JIT emits --------------------
 * Each of these appends a *complete* instruction. The register choices are the
 * JIT's calling convention (documented in jit.c): rbx = data pointer `p`,
 * r14 = read_byte fn ptr, r15 = write_byte fn ptr. These encoders bake those
 * registers in — they are a JIT-internal micro-ISA, not a general assembler.
 * --------------------------------------------------------------------------- */

void emit_prologue(code_buf *c);   /* save callee-saved regs, load the 3 args  */
void emit_epilogue(code_buf *c);   /* restore regs and `ret`                   */

void emit_add_ptr (code_buf *c, i32 delta); /* rbx += delta   (BF '>' and '<') */
void emit_add_cell(code_buf *c, u8  delta); /* [rbx] += delta (BF '+' and '-') */
void emit_set_cell(code_buf *c, u8  val);   /* [rbx]  = val   (peephole "[-]")  */
void emit_output  (code_buf *c);            /* write_byte([rbx])   (BF '.')     */
void emit_input   (code_buf *c);            /* [rbx] = read_byte() (BF ',')     */

/* Loop brackets need *backpatching*: when we emit '[' we do not yet know where
 * its matching ']' will land, so we emit the conditional jump with a zero
 * placeholder displacement and remember where to fix it up. These two return
 * the buffer offset of the 4-byte rel32 field that must later be patched. */
u64  emit_loop_open (code_buf *c);          /* cmpb $0,[rbx]; je  rel32(=0)     */
u64  emit_loop_close(code_buf *c);          /* cmpb $0,[rbx]; jne rel32(=0)     */

/* Patch a previously-emitted rel32 field (at buffer offset `at`) so the jump
 * lands on buffer offset `target`. rel32 on x86-64 is measured from the END of
 * the jump instruction, i.e. from the 4 bytes right after the field: so the
 * value stored is target - (at + 4). This is the single most error-prone number
 * in a JIT, which is why it lives in exactly one place. */
void patch_rel32(code_buf *c, u64 at, u64 target);

#endif /* BF_EMIT_H */
