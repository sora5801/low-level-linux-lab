/* ===========================================================================
 * demo.c — the two instructive, PURE-LOGIC cores of masm, self-contained.
 * ===========================================================================
 *
 * This file is the assembly-teaching deliverable. It has NO #includes and
 * defines its own integer types, so `clang -S` turns it into clean, self-
 * contained assembly with nothing from libc in the way. It extracts the two
 * routines at the heart of the assembler:
 *
 *   1. encode_mov_rr()  — the instruction encoder for one form:
 *                         `mov %src, %dst` becomes REX + opcode + ModR/M.
 *   2. backpatch_rel32() — the two-pass trick: fill a forward branch's rel32
 *                         hole once the target's address is finally known.
 *
 * Everything here is a mirror of encode.c / assemble.c with the assembler's
 * bookkeeping stripped away, so you can read the bit math on its own. Compare
 * the generated demo.s / demo.O0.s / demo.O2.s to see how the compiler turns
 * these shifts and masks into real x86-64 instructions.
 * ===========================================================================
 */

/* Own fixed-width types — no <stdint.h>. On x86-64 Linux these widths hold. */
typedef unsigned char      u8;
typedef unsigned int       u32;
typedef int                i32;
typedef unsigned long long u64;

/* ---------------------------------------------------------------------------
 * encode_mov_rr — encode `mov %src, %dst` (64-bit, AT&T register order).
 *
 * The three bytes, and WHY each bit is where it is:
 *
 *   REX  = 0100 WRXB
 *          W=1 selects the 64-bit operand size (0x48 is REX with only W set).
 *          R is the HIGH bit of the ModR/M.reg field; we set it when the
 *            source register is r8..r15 (its number is >= 8).
 *          B is the HIGH bit of the ModR/M.rm field; set for a high dest reg.
 *          (X extends a SIB index — unused here, so 0.)
 *
 *   opcode 0x89 = MOV r/m64, r64. This is the "store to r/m from reg"
 *          direction: the DESTINATION goes in the rm field and the SOURCE in
 *          the reg field. That is exactly why AT&T writes `mov src, dst` yet
 *          the bytes carry dst in rm — the byte order is not reversed, the
 *          two fields simply have fixed roles.
 *
 *   ModR/M = mod(2) reg(3) rm(3)
 *          mod=11 (0xC0) means "rm names a register", i.e. register-direct.
 *          reg = low 3 bits of the source; rm = low 3 bits of the dest.
 *
 * Returns the number of bytes written (always 3 for this form).
 * Example: encode_mov_rr(out, 6, 7)  ->  48 89 f7   (`mov %rsi, %rdi`).
 * --------------------------------------------------------------------------- */
int encode_mov_rr(u8 *out, int src, int dst)
{
    int n = 0;

    u8 rex = 0x48;                       /* REX.W: 64-bit operand size         */
    if (src >= 8) rex |= 0x04;           /* REX.R: high bit of reg (source)    */
    if (dst >= 8) rex |= 0x01;           /* REX.B: high bit of rm  (dest)      */
    out[n++] = rex;

    out[n++] = 0x89;                     /* opcode MOV r/m64, r64              */

    /* mod=11, reg=src&7, rm=dst&7 : 0xC0 | (src<<3) | dst, masked to 3 bits. */
    out[n++] = (u8)(0xC0 | ((src & 7) << 3) | (dst & 7));

    return n;
}

/* ---------------------------------------------------------------------------
 * backpatch_rel32 — resolve a forward branch after two-pass layout.
 *
 * A jmp/call is `opcode` + a 4-byte little-endian rel32 displacement. When the
 * assembler first emitted the branch it did not yet know where the target was,
 * so it left the 4 bytes at `field_off` as a hole. Pass 1 has since recorded
 * `target_off` (the label's offset in the same section). Now we fill the hole.
 *
 * THE KEY ARITHMETIC: an x86 rel32 is measured from the address of the NEXT
 * instruction, which is the end of this branch = field_off + 4. So:
 *
 *       rel32 = target_off - (field_off + 4)
 *
 * A backward branch yields a negative rel (its two's-complement top bit set);
 * a forward branch a positive one. We write it little-endian, low byte first,
 * exactly as the CPU will read it.
 * --------------------------------------------------------------------------- */
void backpatch_rel32(u8 *code, u32 field_off, u32 target_off)
{
    i32 rel = (i32)target_off - (i32)(field_off + 4);

    code[field_off + 0] = (u8)( (u32)rel        & 0xff);   /* bits  0..7  */
    code[field_off + 1] = (u8)(((u32)rel >> 8)  & 0xff);   /* bits  8..15 */
    code[field_off + 2] = (u8)(((u32)rel >> 16) & 0xff);   /* bits 16..23 */
    code[field_off + 3] = (u8)(((u32)rel >> 24) & 0xff);   /* bits 24..31 */
}

/* ---------------------------------------------------------------------------
 * A tiny self-check so demo.c is also a runnable program (clang demo.c -o demo;
 * echo $?  -> 0 if the encodings are as expected). No libc calls are needed —
 * we fold the results into the process exit status. This keeps both routines
 * "live" so they appear in the generated assembly at every -O level.
 * --------------------------------------------------------------------------- */
int main(void)
{
    u8 code[16];

    /* `mov %rsi, %rdi` must encode as 48 89 f7. */
    int n = encode_mov_rr(code, 6 /*rsi*/, 7 /*rdi*/);
    int ok = (n == 3) && code[0] == 0x48 && code[1] == 0x89 && code[2] == 0xf7;

    /* `mov %r8, %r15` exercises both REX.R and REX.B: 4d 89 c7. */
    n  = encode_mov_rr(code, 8 /*r8*/, 15 /*r15*/);
    ok = ok && code[0] == 0x4d && code[1] == 0x89 && code[2] == 0xc7;

    /* Lay out a 5-byte `jmp` at offset 0 (opcode 0xE9 at 0, rel32 field at 1).
     * The target sits at offset 10. The rel32 is measured from the next
     * instruction = field(1) + 4 = 5, so rel = 10 - 5 = 5, i.e. 05 00 00 00. */
    code[0] = 0xE9;                       /* JMP rel32 opcode                  */
    backpatch_rel32(code, 1 /*field_off*/, 10 /*target_off*/);
    ok = ok && code[1] == 0x05 && code[2] == 0x00 && code[3] == 0x00 && code[4] == 0x00;

    return ok ? 0 : 1;                    /* 0 = all encodings correct         */
}
