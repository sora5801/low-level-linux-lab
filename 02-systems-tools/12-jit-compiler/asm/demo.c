/* ===========================================================================
 * asm/demo.c — the JIT's instruction ENCODER, extracted self-contained so it
 *              compiles to clean teaching assembly with no libc in the way.
 * ===========================================================================
 *
 * This is a faithful slice of emit.c: the "byte assembly" that turns an
 * intended x86-64 instruction into the exact bytes a CPU decodes. We keep just
 * the most instructive routines so the generated .s stays readable:
 *
 *   modrm()        — build the ModRM byte  [mod:2|reg:3|rm:3]
 *   emit_u8/u32()  — append raw bytes, little-endian
 *   emit_add_ptr() — encode `add $imm32, %rbx`   (REX.W + 81 /0 + ModRM + imm32)
 *   emit_add_cell()— encode `add $imm8,  (%rbx)` (80 /0 + ModRM + imm8)
 *   patch_rel32()  — the rel32 back-patch math    (target-(at+4), little-endian)
 *
 * No #includes: we declare our own fixed-width types, exactly as emit.h does.
 * That is what lets `clang --target=x86_64-pc-linux-gnu -S` emit pure Linux
 * SysV assembly for this file. Read asm/demo.annotated.s alongside it.
 * ===========================================================================
 */

typedef unsigned char  u8;
typedef unsigned int   u32;
typedef signed   int   i32;
typedef unsigned long  u64;

/* The append-only code buffer (identical shape to emit.h's code_buf). */
typedef struct code_buf {
    u8  *buf;       /* caller-owned output buffer (an mmap'd page in the JIT) */
    u64  len;       /* next write offset                                      */
    u64  cap;       /* capacity; writes past this are refused                 */
    int  overflow;  /* sticky out-of-space flag                               */
} code_buf;

/* Append one byte, refusing (and flagging) a write that would overrun cap. */
void emit_u8(code_buf *c, u8 b)
{
    if (c->len >= c->cap) { c->overflow = 1; return; }
    c->buf[c->len++] = b;
}

/* Append a 32-bit value little-endian (low byte first) — x86-64's byte order
 * for every immediate and displacement. */
void emit_u32(code_buf *c, u32 v)
{
    emit_u8(c, (u8)(v         & 0xff));
    emit_u8(c, (u8)((v >> 8)  & 0xff));
    emit_u8(c, (u8)((v >> 16) & 0xff));
    emit_u8(c, (u8)((v >> 24) & 0xff));
}

/* Pack a ModRM byte from its three bit-fields. */
u8 modrm(u8 mod, u8 reg, u8 rm)
{
    return (u8)(((mod & 3) << 6) | ((reg & 7) << 3) | (rm & 7));
}

/* Encode `add $imm32, %rbx` = REX.W(0x48) 81 /0 ModRM(0xC3) imm32.
 * One encoding serves both Brainfuck '>' and '<': a negative delta rides along
 * as a sign-extended imm32, so subtracting is just adding a negative. */
void emit_add_ptr(code_buf *c, i32 delta)
{
    emit_u8(c, 0x48);                       /* REX.W: 64-bit operand size      */
    emit_u8(c, 0x81);                       /* opcode: add r/m64, imm32        */
    emit_u8(c, modrm(0x3, 0, 3));           /* mod=11 reg=/0 rm=rbx(3) => 0xC3 */
    emit_u32(c, (u32)delta);                /* imm32, little-endian            */
}

/* Encode `add $imm8, (%rbx)` = 80 /0 ModRM(0x03) imm8. 8-bit memory add on the
 * current cell; wraps mod 256 for free, matching Brainfuck '+'/'-'. */
void emit_add_cell(code_buf *c, u8 delta)
{
    emit_u8(c, 0x80);                       /* opcode: add r/m8, imm8          */
    emit_u8(c, modrm(0x0, 0, 3));           /* mod=00 reg=/0 rm=[rbx] => 0x03  */
    emit_u8(c, delta);                      /* imm8                            */
}

/* Back-patch a rel32 jump field at offset `at` to land on `target`.
 * x86 measures rel32 from the byte after the field, so disp = target-(at+4). */
void patch_rel32(code_buf *c, u64 at, u64 target)
{
    i32 disp = (i32)((i32)target - (i32)(at + 4));
    u32 v = (u32)disp;
    c->buf[at + 0] = (u8)(v         & 0xff);
    c->buf[at + 1] = (u8)((v >> 8)  & 0xff);
    c->buf[at + 2] = (u8)((v >> 16) & 0xff);
    c->buf[at + 3] = (u8)((v >> 24) & 0xff);
}
