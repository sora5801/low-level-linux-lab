/* ===========================================================================
 * emit.c — hand-encode x86-64 instructions, one byte at a time.
 * ===========================================================================
 *
 * This is where a JIT stops being magic. Every function here appends the exact
 * bytes an x86-64 CPU decodes into an instruction. To read it you need three
 * facts about the encoding, then the rest is lookup:
 *
 *   1. LEGACY-PREFIX / REX / OPCODE / MODRM / SIB / DISP / IMM
 *      An instruction is a sequence of optional fields in that fixed order. We
 *      only ever use: an optional REX byte, one or two opcode bytes, a ModRM
 *      byte, and an immediate/displacement. No SIB (we never use scaled-index
 *      addressing) and no legacy prefixes (we stay in 64-bit operand land or
 *      use the natural 8-bit forms).
 *
 *   2. THE REX PREFIX  = 0100 WRXB  (0x40 | W<<3 | R<<2 | X<<1 | B)
 *      - W=1 promotes the operation to 64-bit operand size (e.g. rbx, not ebx).
 *      - R extends the ModRM.reg field to 4 bits (to reach r8..r15).
 *      - X extends a SIB index (unused here).
 *      - B extends the ModRM.rm field (to reach r8..r15 as the base/rm).
 *      Example: `call *%r15` needs B=1 because r15's low 3 bits (111) collide
 *      with rdi's; REX.B disambiguates -> prefix 0x41.
 *
 *   3. MODRM  = [ mod:2 | reg:3 | rm:3 ]
 *      - mod=0b11 : rm names a *register*      -> operand is that register.
 *      - mod=0b00 : rm names a *memory base*   -> operand is [rm], no displacement
 *                   (with the well-known exceptions rm=100 => SIB, rm=101 =>
 *                   RIP-relative; we use rm=011 = rbx, which has neither quirk).
 *      - reg      : either the other register, or an /digit opcode extension.
 *
 * Every encoding below was cross-checked against `llvm-mc --show-encoding`; the
 * expected bytes are written in the comments so you can verify by eye.
 * ===========================================================================
 */
#include "emit.h"   /* no system headers on purpose — see emit.h              */

/* --- register numbers (the 3-bit field values) ----------------------------
 * These are the architectural encodings, NOT arbitrary. rax=0 rcx=1 rdx=2
 * rbx=3 rsp=4 rbp=5 rsi=6 rdi=7, then r8..r15 = 0..7 again but with REX.R/.B
 * set. The JIT uses rbx, r14, r15 (plus rdi for outgoing call args). */
enum { RAX=0, RCX=1, RDX=2, RBX=3, RSP=4, RBP=5, RSI=6, RDI=7 };
/* r14 low-3 bits = 6, r15 low-3 bits = 7; the "extended" bit goes in REX.    */

/* ------------------------------------------------------------------------- */
/* primitive writers                                                         */
/* ------------------------------------------------------------------------- */

void emit_u8(code_buf *c, u8 b)
{
    /* Bounds check BEFORE writing. A code generator that overruns its output
     * buffer would corrupt whatever lives after it and then hand the CPU that
     * corruption to execute — so refusing the write and flagging it is the only
     * safe failure mode. `len` is not advanced on refusal, keeping the buffer
     * internally consistent. */
    if (c->len >= c->cap) { c->overflow = 1; return; }
    c->buf[c->len++] = b;
}

void emit_u32(code_buf *c, u32 v)
{
    /* Little-endian: least-significant byte first. This is how x86-64 stores
     * every immediate and displacement, so a rel32 of 0x11223344 goes out as
     * 44 33 22 11. We funnel through emit_u8 so the bounds check applies to
     * each byte (a partial write at the very end still can't run past cap). */
    emit_u8(c, (u8)(v        & 0xff));
    emit_u8(c, (u8)((v >> 8) & 0xff));
    emit_u8(c, (u8)((v >> 16)& 0xff));
    emit_u8(c, (u8)((v >> 24)& 0xff));
}

/* ------------------------------------------------------------------------- */
/* ModRM                                                                      */
/* ------------------------------------------------------------------------- */

u8 modrm(u8 mod, u8 reg, u8 rm)
{
    /* Pack the three fields: mod in bits 7-6, reg in 5-3, rm in 2-0. Masking
     * keeps each field in range even if a caller passes a full register number
     * whose high bit belongs in REX. */
    return (u8)(((mod & 3) << 6) | ((reg & 7) << 3) | (rm & 7));
}

/* ------------------------------------------------------------------------- */
/* prologue / epilogue — establish the generated function's frame + ABI       */
/* ------------------------------------------------------------------------- */
/*
 * The generated function's C-visible prototype (see jit.c) is:
 *
 *     void fn(unsigned char *tape,          // arg0 -> rdi  (SysV)
 *             long (*read_byte)(void),      // arg1 -> rsi
 *             void (*write_byte)(long));    // arg2 -> rdx
 *
 * We move those three arguments into CALLEE-SAVED registers so they survive the
 * calls we make to read_byte/write_byte:
 *     rbx <- tape        (the Brainfuck data pointer `p`)
 *     r14 <- read_byte
 *     r15 <- write_byte
 * Because they are callee-saved, we must push them first and pop them last —
 * that is the contract that lets *our* caller keep using them.
 *
 * STACK ALIGNMENT (the subtle part). SysV requires rsp % 16 == 0 at the moment
 * a `call` executes. At our entry, the caller's `call` already pushed an 8-byte
 * return address, so rsp % 16 == 8. We then push rbp, rbx, r14, r15 (four
 * 8-byte pushes) which leaves rsp % 16 == 8 again, so a single `sub $8,%rsp`
 * pad brings it to 0. The epilogue undoes exactly this.
 */
void emit_prologue(code_buf *c)
{
    emit_u8(c, 0x55);                              /* push %rbp          55    */
    emit_u8(c, 0x48); emit_u8(c, 0x89); emit_u8(c, 0xE5); /* mov %rsp,%rbp 48 89 E5 */
    emit_u8(c, 0x53);                              /* push %rbx          53    */
    emit_u8(c, 0x41); emit_u8(c, 0x56);            /* push %r14       41 56    */
    emit_u8(c, 0x41); emit_u8(c, 0x57);            /* push %r15       41 57    */
    /* sub $8,%rsp — the 16-byte alignment pad (REX.W 48, opcode 83 /5, imm8) */
    emit_u8(c, 0x48); emit_u8(c, 0x83); emit_u8(c, 0xEC); emit_u8(c, 0x08);
    /* mov %rdi,%rbx  -> rbx = tape           48 89 FB (REX.W, 89 /r, ModRM C3? )
     * ModRM here is 0xFB = mod11 reg111(rdi) rm011(rbx): "rbx = rdi". */
    emit_u8(c, 0x48); emit_u8(c, 0x89); emit_u8(c, 0xFB);
    /* mov %rsi,%r14  -> r14 = read_byte      49 89 F6 (REX.W+B, reg=rsi rm=r14) */
    emit_u8(c, 0x49); emit_u8(c, 0x89); emit_u8(c, 0xF6);
    /* mov %rdx,%r15  -> r15 = write_byte     49 89 D7 (REX.W+B, reg=rdx rm=r15) */
    emit_u8(c, 0x49); emit_u8(c, 0x89); emit_u8(c, 0xD7);
}

void emit_epilogue(code_buf *c)
{
    /* Mirror the prologue exactly so rsp returns to the incoming value that
     * points at the return address, then `ret` pops it. */
    emit_u8(c, 0x48); emit_u8(c, 0x83); emit_u8(c, 0xC4); emit_u8(c, 0x08); /* add $8,%rsp */
    emit_u8(c, 0x41); emit_u8(c, 0x5F);            /* pop %r15        41 5F    */
    emit_u8(c, 0x41); emit_u8(c, 0x5E);            /* pop %r14        41 5E    */
    emit_u8(c, 0x5B);                              /* pop %rbx           5B    */
    emit_u8(c, 0x5D);                              /* pop %rbp           5D    */
    emit_u8(c, 0xC3);                              /* ret                C3    */
}

/* ------------------------------------------------------------------------- */
/* data-pointer and cell arithmetic                                           */
/* ------------------------------------------------------------------------- */

void emit_add_ptr(code_buf *c, i32 delta)
{
    /* rbx += delta, encoded as `add r/m64, imm32` = REX.W 48 / opcode 81 /0 /
     * ModRM 0xC3 (mod11 reg000(/0) rm011(rbx)) / imm32.
     *
     * We ALWAYS use the imm32 form (never the shorter imm8 form) so a single
     * encoding covers any run length in either direction: a negative delta is
     * emitted as a 32-bit two's-complement value and the CPU sign-extends imm32
     * to 64 bits, so `add $-3` correctly decrements the pointer. That is why
     * this one function implements BOTH Brainfuck '>' (delta>0) and '<'
     * (delta<0): subtraction is just adding a negative. */
    emit_u8(c, 0x48);
    emit_u8(c, 0x81);
    emit_u8(c, modrm(0x3, 0 /*/0*/, RBX));   /* 0xC3 */
    emit_u32(c, (u32)delta);
}

void emit_add_cell(code_buf *c, u8 delta)
{
    /* [rbx] += delta on a single BYTE = `add r/m8, imm8` = opcode 80 /0 /
     * ModRM 0x03 (mod00 reg000(/0) rm011(rbx) = memory at [rbx]) / imm8.
     *
     * No REX: an 8-bit memory op on rbx needs none. Brainfuck cells are bytes
     * that wrap mod 256, and 8-bit add wraps for free. As with the pointer,
     * ONE function covers '+' and '-': to subtract k we add the byte (256-k),
     * i.e. the two's-complement of k, which the caller has already folded in. */
    emit_u8(c, 0x80);
    emit_u8(c, modrm(0x0, 0 /*/0*/, RBX));    /* 0x03 */
    emit_u8(c, delta);
}

void emit_set_cell(code_buf *c, u8 val)
{
    /* [rbx] = val  = `mov r/m8, imm8` = opcode C6 /0 / ModRM 0x03 / imm8.
     * This is the peephole optimizer's payoff: the extremely common idiom
     * "[-]" (loop: decrement until zero) is a no-op-shaped way of writing
     * "set this cell to 0", and we compile the whole loop to this ONE 3-byte
     * store instead of a branchy decrement loop. */
    emit_u8(c, 0xC6);
    emit_u8(c, modrm(0x0, 0 /*/0*/, RBX));    /* 0x03 */
    emit_u8(c, val);
}

/* ------------------------------------------------------------------------- */
/* I/O — call back into C helpers using the SysV convention                   */
/* ------------------------------------------------------------------------- */

void emit_output(code_buf *c)
{
    /* Brainfuck '.': write_byte(cell). Steps:
     *   movzbl (%rbx), %edi   ; rdi = zero-extended cell value  (SysV arg0)
     *   call   *%r15          ; indirect call through write_byte pointer
     *
     * movzbl zero-extends the byte to 32 bits AND (per x86-64 rules) clears the
     * upper 32 bits of rdi, so rdi holds a clean 0..255 argument. Encoding:
     *   0F B6 /r, ModRM 0x3B = mod00 reg111(edi) rm011([rbx]).
     * The call is `FF /2` with REX.B (r15 is an extended reg): 41 FF D7. rbx and
     * r15 are callee-saved, so the C helper is guaranteed to hand them back
     * intact and the loop can keep using them. */
    emit_u8(c, 0x0F); emit_u8(c, 0xB6); emit_u8(c, 0x3B);  /* movzbl (%rbx),%edi */
    emit_u8(c, 0x41); emit_u8(c, 0xFF); emit_u8(c, 0xD7);  /* call *%r15         */
}

void emit_input(code_buf *c)
{
    /* Brainfuck ',': cell = read_byte(). Steps:
     *   call *%r14        ; read_byte() -> result in rax (SysV return reg)
     *   mov  %al, (%rbx)  ; store the low byte into the current cell
     *
     * read_byte returns 0 on EOF (the convention this JIT documents), so the
     * cell simply becomes 0 at end of input. Encodings:
     *   call *%r14 = 41 FF D6 ;  mov %al,(%rbx) = 88 /r ModRM 0x03 = 88 03. */
    emit_u8(c, 0x41); emit_u8(c, 0xFF); emit_u8(c, 0xD6);  /* call *%r14      */
    emit_u8(c, 0x88); emit_u8(c, 0x03);                    /* mov %al,(%rbx)  */
}

/* ------------------------------------------------------------------------- */
/* loops — emit conditional jumps with a placeholder, return the patch site   */
/* ------------------------------------------------------------------------- */

u64 emit_loop_open(code_buf *c)
{
    /* Brainfuck '[': if the current cell is 0, jump PAST the matching ']'.
     *   cmpb $0, (%rbx)   ; sets ZF according to the byte at [rbx]
     *   je   rel32        ; taken when cell==0 (ZF=1)  -> exit the loop
     *
     * cmpb $0,(%rbx) = 80 /7 ModRM 0x3B imm8(00) = 80 3B 00.
     * je near = two-byte opcode 0F 84 followed by rel32. We do not yet know the
     * loop's end, so we emit rel32 = 0 and return the offset of that field for
     * the matching ']' to patch. Using the NEAR (rel32) form unconditionally
     * means loop bodies of any size are reachable — no short/near guessing. */
    emit_u8(c, 0x80); emit_u8(c, 0x3B); emit_u8(c, 0x00); /* cmpb $0,(%rbx) */
    emit_u8(c, 0x0F); emit_u8(c, 0x84);                   /* je near ...    */
    u64 rel_site = c->len;   /* offset of the rel32 field we are about to write */
    emit_u32(c, 0);          /* placeholder displacement, patched later         */
    return rel_site;
}

u64 emit_loop_close(code_buf *c)
{
    /* Brainfuck ']': if the current cell is NON-zero, jump BACK to the loop
     * body (just after the matching '['s je).
     *   cmpb $0, (%rbx)
     *   jne  rel32        ; taken when cell!=0 (ZF=0) -> repeat the loop
     * jne near = 0F 85 + rel32. Returns the rel32 field offset so the caller
     * can patch it to point back at the loop body. */
    emit_u8(c, 0x80); emit_u8(c, 0x3B); emit_u8(c, 0x00); /* cmpb $0,(%rbx) */
    emit_u8(c, 0x0F); emit_u8(c, 0x85);                   /* jne near ...   */
    u64 rel_site = c->len;
    emit_u32(c, 0);
    return rel_site;
}

void patch_rel32(code_buf *c, u64 at, u64 target)
{
    /* Fill in a rel32 field at buffer offset `at` so the branch lands on
     * `target`. x86 measures a rel32 from the address of the NEXT instruction,
     * which begins immediately after the 4-byte field: next = at + 4.
     * Therefore displacement = target - (at + 4). The subtraction is done in
     * signed 32-bit, which is exactly the width the CPU sign-extends. */
    if (at + 4 > c->cap) { c->overflow = 1; return; }  /* never patch OOB */
    i32 disp = (i32)((i32)target - (i32)(at + 4));
    /* Write the 4 bytes directly (little-endian) rather than via emit_u32,
     * because we are back-patching an already-written region, not appending. */
    u32 v = (u32)disp;
    c->buf[at + 0] = (u8)(v        & 0xff);
    c->buf[at + 1] = (u8)((v >> 8) & 0xff);
    c->buf[at + 2] = (u8)((v >> 16)& 0xff);
    c->buf[at + 3] = (u8)((v >> 24)& 0xff);
}
