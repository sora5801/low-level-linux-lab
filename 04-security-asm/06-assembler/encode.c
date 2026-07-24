/* ===========================================================================
 * encode.c — x86-64 instruction encoder (the heart of masm).
 * ===========================================================================
 *
 * Given a parsed statement, produce the exact machine-code bytes. Everything
 * here targets the 64-bit operand size, so almost every instruction carries a
 * REX prefix with REX.W set (0x48 base). The bytes of an x86-64 instruction,
 * in order, are:
 *
 *     [legacy prefixes] [REX] opcode [ModR/M] [SIB] [disp] [imm]
 *
 * and this file builds exactly those, in that order, for each supported form.
 *
 * THE THREE BYTES YOU MUST UNDERSTAND
 * -----------------------------------
 *  REX  = 0100 WRXB          (a prefix, 0x40..0x4F)
 *         W=1 -> 64-bit operand size
 *         R   -> high (4th) bit of the ModR/M.reg field
 *         X   -> high bit of the SIB.index field
 *         B   -> high bit of the ModR/M.rm / SIB.base / opcode-embedded reg
 *
 *  ModR/M = mm rrr bbb       (mod<<6 | reg<<3 | rm)
 *         mod=11 -> rm is a register (register-direct)
 *         mod=00 -> [rm], with two escapes: rm=101 => RIP+disp32, rm=100 => SIB
 *         mod=01 -> [rm + disp8]
 *         mod=10 -> [rm + disp32]
 *
 *  SIB  = ss iii bbb         (scale<<6 | index<<3 | base) — only when rm=100
 *         index=100 means "no index register".
 *
 * SINGLE CODE PATH FOR SIZING AND EMISSION
 * ----------------------------------------
 * Every emitter takes (A, emit, *n): it always advances the byte counter *n,
 * and only writes bytes when emit==1. Pass 1 calls with emit==0 to LEARN each
 * instruction's size (so labels get addresses); pass 2 calls with emit==1 to
 * WRITE. Because both passes run the identical code, a size can never disagree
 * with the bytes actually produced — a classic assembler bug we design out.
 * ===========================================================================
 */
#include "asm.h"
#include <stdio.h>
#include <string.h>

/* --- primitive emitters: count always, write only when emit ---------------- */
static void ib(Assembler *A, int emit, int *n, uint8_t b)
{
    if (emit) buf_u8(&A->sec[A->cur], b);       /* instructions land in .cur   */
    (*n)++;
}
static void i32(Assembler *A, int emit, int *n, uint32_t v)
{
    if (emit) buf_u32(&A->sec[A->cur], v);
    (*n) += 4;
}
static void i64(Assembler *A, int emit, int *n, uint64_t v)
{
    if (emit) buf_u64(&A->sec[A->cur], v);
    (*n) += 8;
}

/* Does a 64-bit value fit in a signed 8-/32-bit field? Immediates and
 * displacements are sign-extended by the CPU, so the test is signed range. */
static int fits_s8 (int64_t v) { return v >= -128 && v <= 127; }
static int fits_s32(int64_t v) { return v >= -2147483648LL && v <= 2147483647LL; }

static int emit_error(Assembler *A, Stmt *s, const char *msg)
{
    fprintf(stderr, "%s:%d: error: %s (in `%s`)\n",
            A->srcname, s->line, msg, s->mnem[0] ? s->mnem : s->dir);
    A->errors++;
    return 0;
}

/* ---------------------------------------------------------------------------
 * emit_symref — write the 4-byte PC-relative field of a jmp/call/lea and
 * decide, per the assembler's resolution rule, whether we can fill it in now
 * or must leave a relocation for the linker.
 *
 * RESOLUTION RULE (this is exactly what a real assembler does):
 *   * If the target is a LOCAL, DEFINED label in the SAME section as the field
 *     (both .text here), the distance is invariant under where the section
 *     finally loads, so we compute rel32 = target - (field + 4) NOW. No reloc.
 *   * Otherwise — undefined (external), or global (may be interposed at link
 *     time), or in another section — we cannot know the final distance, so we
 *     write 0 and record a relocation. The addend is -4 because the rel32 sits
 *     at the END of the instruction: the CPU measures from field+4.
 * --------------------------------------------------------------------------- */
static void emit_symref(Assembler *A, int emit, int *n,
                        const char *name, uint32_t reloc_type)
{
    if (emit) {
        uint64_t field = A->sec[A->cur].len;    /* offset of this field in .text */
        Symbol  *sy    = sym_find(A, name);
        if (sy && sy->defined && !sy->is_global && sy->section == A->cur) {
            /* Resolve locally: PC-relative within one section. */
            int64_t rel = (int64_t)sy->value - (int64_t)(field + 4);
            buf_u32(&A->sec[A->cur], (uint32_t)(int32_t)rel);
        } else {
            if (!sy) sym_ref_undef(A, name);    /* make it an external symbol   */
            add_reloc(A, field, name, reloc_type, -4);
            buf_u32(&A->sec[A->cur], 0);        /* linker overwrites this        */
        }
    }
    (*n) += 4;
}

/* ---------------------------------------------------------------------------
 * REGISTER-DIRECT two-operand form:  op  %src, %dst   (mod=11)
 * Used by mov (0x89), add (0x01), sub (0x29), cmp (0x39). In AT&T these all
 * take the destination in ModR/M.rm and the source in ModR/M.reg — i.e. the
 * "store to r/m from reg" direction — which is why the register order looks
 * reversed compared to the bytes.
 * --------------------------------------------------------------------------- */
static void enc_rr(Assembler *A, int emit, int *n, uint8_t opcode, int src, int dst)
{
    /* REX.W always (64-bit). REX.R extends the reg field (src); REX.B the rm
     * field (dst). Low 3 bits of each go in the ModR/M byte. */
    uint8_t rex = 0x48 | (src >= 8 ? 0x04 : 0) | (dst >= 8 ? 0x01 : 0);
    ib(A, emit, n, rex);
    ib(A, emit, n, opcode);
    ib(A, emit, n, (uint8_t)(0xC0 | ((src & 7) << 3) | (dst & 7)));
}

/* ---------------------------------------------------------------------------
 * MEMORY operand encoder, shared by mov (load 0x8B / store 0x89) and lea 0x8D.
 * `reg` is the register operand (goes in ModR/M.reg); `m` is the memory operand.
 * Handles the notorious special cases:
 *   * rsp/r12 (rm==100) FORCE a SIB byte, because rm==100 is the SIB escape.
 *   * rbp/r13 (rm==101) cannot use mod==00 (that encoding means RIP+disp32),
 *     so a zero displacement there must be spelled mod==01 disp8=0.
 *   * %rip base => mod==00, rm==101 => RIP+disp32 (a symbol reloc or literal).
 * --------------------------------------------------------------------------- */
static void enc_mem(Assembler *A, int emit, int *n, uint8_t opcode,
                    int reg, const Operand *m)
{
    int     rexR = (reg >= 8) ? 1 : 0;
    int     rexX = 0, rexB = 0;
    int     mod = 0, rm = 0, sib_used = 0;
    uint8_t sib = 0;
    enum { D_NONE, D_8, D_32, D_RELOC } dk = D_NONE;

    if (m->rip) {
        /* RIP-relative: mod=00, rm=101 => next-instruction-relative disp32. */
        mod = 0; rm = 5;
        dk  = m->have_sym ? D_RELOC : D_32;   /* symbol -> reloc; number -> lit */
    } else {
        int base = m->reg;
        int b3   = base & 7;
        rexB = (base >= 8) ? 1 : 0;

        if (b3 == 4) {                        /* rsp/r12: need a SIB byte      */
            sib_used = 1;
            sib = (uint8_t)((0 << 6) | (4 << 3) | 4); /* scale0,index=none,base */
            rm  = 4;
        } else {
            rm = b3;
        }

        if (m->imm == 0 && b3 != 5) {         /* [base] with no displacement   */
            mod = 0; dk = D_NONE;
        } else if (fits_s8(m->imm)) {         /* [base + disp8]                */
            mod = 1; dk = D_8;
        } else {                              /* [base + disp32]               */
            mod = 2; dk = D_32;
        }
    }

    uint8_t rex = 0x48 | (rexR << 2) | (rexX << 1) | rexB;   /* W=1 always     */
    ib(A, emit, n, rex);
    ib(A, emit, n, opcode);
    ib(A, emit, n, (uint8_t)((mod << 6) | ((reg & 7) << 3) | rm));
    if (sib_used) ib(A, emit, n, sib);

    switch (dk) {
        case D_NONE:  break;
        case D_8:     ib (A, emit, n, (uint8_t)(int8_t)m->imm);            break;
        case D_32:    i32(A, emit, n, (uint32_t)(int32_t)m->imm);          break;
        case D_RELOC: emit_symref(A, emit, n, m->sym, R_X86_64_PC32);      break;
    }
}

/* mov $imm, %reg : two encodings, chosen by immediate width.
 *   fits int32 -> C7 /0 id  (7 bytes, sign-extended to 64)
 *   otherwise  -> B8+rd io  (movabs, 10 bytes, full imm64)                    */
static void enc_mov_imm(Assembler *A, int emit, int *n, int dst, int64_t v)
{
    uint8_t rexB = (dst >= 8) ? 1 : 0;
    if (fits_s32(v)) {
        ib (A, emit, n, (uint8_t)(0x48 | rexB));
        ib (A, emit, n, 0xC7);
        ib (A, emit, n, (uint8_t)(0xC0 | (dst & 7)));   /* reg field /0 = MOV  */
        i32(A, emit, n, (uint32_t)(int32_t)v);
    } else {
        ib (A, emit, n, (uint8_t)(0x48 | rexB));
        ib (A, emit, n, (uint8_t)(0xB8 | (dst & 7)));   /* opcode-embedded reg */
        i64(A, emit, n, (uint64_t)v);
    }
}

/* add/sub/cmp  $imm, %reg : group-1 opcode with an extension in ModR/M.reg.
 *   ext: ADD=0, SUB=5, CMP=7.  imm8 form (83 /ext ib) when it fits, else
 *   imm32 form (81 /ext id). A 64-bit immediate is not encodable here.        */
static int enc_grp1_imm(Assembler *A, int emit, int *n, Stmt *s, int ext, int dst, int64_t v)
{
    uint8_t rexB = (dst >= 8) ? 1 : 0;
    if (fits_s8(v)) {
        ib(A, emit, n, (uint8_t)(0x48 | rexB));
        ib(A, emit, n, 0x83);
        ib(A, emit, n, (uint8_t)(0xC0 | (ext << 3) | (dst & 7)));
        ib(A, emit, n, (uint8_t)(int8_t)v);
    } else if (fits_s32(v)) {
        ib (A, emit, n, (uint8_t)(0x48 | rexB));
        ib (A, emit, n, 0x81);
        ib (A, emit, n, (uint8_t)(0xC0 | (ext << 3) | (dst & 7)));
        i32(A, emit, n, (uint32_t)(int32_t)v);
    } else {
        return emit_error(A, s, "immediate too large for add/sub/cmp (max 32-bit)");
    }
    return 0;
}

/* push/pop %reg : opcode 0x50/0x58 + (reg&7). 64-bit is the default operand
 * size for push/pop, so NO REX.W — only a REX.B when the reg is r8..r15.       */
static void enc_pushpop(Assembler *A, int emit, int *n, uint8_t base, int reg)
{
    if (reg >= 8) ib(A, emit, n, 0x41);          /* REX.B to reach r8..r15      */
    ib(A, emit, n, (uint8_t)(base | (reg & 7)));
}

/* Jcc second opcode byte (after 0x0F). NULL name => not a conditional jump. */
static int jcc_opcode(const char *m)
{
    if (!strcmp(m,"je")  || !strcmp(m,"jz"))  return 0x84;
    if (!strcmp(m,"jne") || !strcmp(m,"jnz")) return 0x85;
    if (!strcmp(m,"jb")  || !strcmp(m,"jc")  || !strcmp(m,"jnae")) return 0x82;
    if (!strcmp(m,"jae") || !strcmp(m,"jnb") || !strcmp(m,"jnc"))  return 0x83;
    if (!strcmp(m,"jbe") || !strcmp(m,"jna"))  return 0x86;
    if (!strcmp(m,"ja")  || !strcmp(m,"jnbe")) return 0x87;
    if (!strcmp(m,"jl")  || !strcmp(m,"jnge")) return 0x8C;
    if (!strcmp(m,"jge") || !strcmp(m,"jnl"))  return 0x8D;
    if (!strcmp(m,"jle") || !strcmp(m,"jng"))  return 0x8E;
    if (!strcmp(m,"jg")  || !strcmp(m,"jnle")) return 0x8F;
    if (!strcmp(m,"js"))  return 0x88;
    if (!strcmp(m,"jns")) return 0x89;
    return -1;
}

/* Operand-kind shorthand for readable dispatch. */
#define K(i) (s->ops[i].kind)

/* ---------------------------------------------------------------------------
 * encode_insn — dispatch one instruction to the right form above.
 * --------------------------------------------------------------------------- */
static int encode_insn(Assembler *A, Stmt *s, int emit, int *n)
{
    const char *m = s->mnem;

    /* ---- data movement ---------------------------------------------------- */
    if (!strcmp(m, "mov")) {
        if (s->nops != 2) return emit_error(A, s, "mov needs 2 operands");
        if (K(0)==OP_REG && K(1)==OP_REG)
            enc_rr(A, emit, n, 0x89, s->ops[0].reg, s->ops[1].reg);       /* r,r */
        else if (K(0)==OP_IMM && K(1)==OP_REG)
            enc_mov_imm(A, emit, n, s->ops[1].reg, s->ops[0].imm);        /* i,r */
        else if (K(0)==OP_MEM && K(1)==OP_REG)
            enc_mem(A, emit, n, 0x8B, s->ops[1].reg, &s->ops[0]);         /* m,r load  */
        else if (K(0)==OP_REG && K(1)==OP_MEM)
            enc_mem(A, emit, n, 0x89, s->ops[0].reg, &s->ops[1]);         /* r,m store */
        else return emit_error(A, s, "unsupported mov operand combination");
        return *n;
    }

    /* ---- lea: load EFFECTIVE ADDRESS (never dereferences memory) ---------- */
    if (!strcmp(m, "lea")) {
        if (s->nops != 2 || K(0) != OP_MEM || K(1) != OP_REG)
            return emit_error(A, s, "lea needs  mem, %reg");
        enc_mem(A, emit, n, 0x8D, s->ops[1].reg, &s->ops[0]);
        return *n;
    }

    /* ---- arithmetic / compare (reg,reg and imm,reg) ----------------------- */
    if (!strcmp(m,"add") || !strcmp(m,"sub") || !strcmp(m,"cmp")) {
        uint8_t rr = !strcmp(m,"add") ? 0x01 : !strcmp(m,"sub") ? 0x29 : 0x39;
        int     ext= !strcmp(m,"add") ? 0    : !strcmp(m,"sub") ? 5    : 7;
        if (s->nops != 2) return emit_error(A, s, "needs 2 operands");
        if (K(0)==OP_REG && K(1)==OP_REG)
            enc_rr(A, emit, n, rr, s->ops[0].reg, s->ops[1].reg);
        else if (K(0)==OP_IMM && K(1)==OP_REG)
            enc_grp1_imm(A, emit, n, s, ext, s->ops[1].reg, s->ops[0].imm);
        else return emit_error(A, s, "unsupported operands (want reg,reg or imm,reg)");
        return *n;
    }

    /* ---- stack ------------------------------------------------------------ */
    if (!strcmp(m, "push")) {
        if (s->nops != 1 || K(0) != OP_REG) return emit_error(A, s, "push needs %reg");
        enc_pushpop(A, emit, n, 0x50, s->ops[0].reg);
        return *n;
    }
    if (!strcmp(m, "pop")) {
        if (s->nops != 1 || K(0) != OP_REG) return emit_error(A, s, "pop needs %reg");
        enc_pushpop(A, emit, n, 0x58, s->ops[0].reg);
        return *n;
    }

    /* ---- control flow ----------------------------------------------------- */
    if (!strcmp(m, "jmp")) {
        if (s->nops != 1 || K(0) != OP_SYM) return emit_error(A, s, "jmp needs a label");
        ib(A, emit, n, 0xE9);                    /* JMP rel32                   */
        emit_symref(A, emit, n, s->ops[0].sym, R_X86_64_PLT32);
        return *n;
    }
    if (!strcmp(m, "call")) {
        if (s->nops != 1 || K(0) != OP_SYM) return emit_error(A, s, "call needs a label");
        ib(A, emit, n, 0xE8);                    /* CALL rel32                  */
        emit_symref(A, emit, n, s->ops[0].sym, R_X86_64_PLT32);
        return *n;
    }
    int cc = jcc_opcode(m);
    if (cc >= 0) {
        if (s->nops != 1 || K(0) != OP_SYM) return emit_error(A, s, "jcc needs a label");
        ib(A, emit, n, 0x0F);                    /* two-byte opcode escape      */
        ib(A, emit, n, (uint8_t)cc);             /* 0x8x = the condition        */
        emit_symref(A, emit, n, s->ops[0].sym, R_X86_64_PLT32);
        return *n;
    }

    /* ---- zero-operand ----------------------------------------------------- */
    if (!strcmp(m, "ret"))     { ib(A, emit, n, 0xC3); return *n; }
    if (!strcmp(m, "syscall")) { ib(A, emit, n, 0x0F); ib(A, emit, n, 0x05); return *n; }
    if (!strcmp(m, "nop"))     { ib(A, emit, n, 0x90); return *n; }

    return emit_error(A, s, "unknown instruction");
}

/* Data directive: append raw bytes to the CURRENT section (.byte 8-bit each,
 * .quad 64-bit little-endian each). .text/.data switch the current section;
 * .globl is recorded once (pass 1) so it can mark a defined label global. */
static int encode_dir(Assembler *A, Stmt *s, int emit)
{
    int n = 0;
    if (!strcmp(s->dir, ".text")) { A->cur = SEC_TEXT; return 0; }
    if (!strcmp(s->dir, ".data")) { A->cur = SEC_DATA; return 0; }
    if (!strcmp(s->dir, ".globl")) { if (!emit) add_global(A, s->dsym); return 0; }
    if (!strcmp(s->dir, ".byte")) {
        for (int i = 0; i < s->ndargs; i++) ib(A, emit, &n, (uint8_t)s->dargs[i]);
        return n;
    }
    if (!strcmp(s->dir, ".quad")) {
        for (int i = 0; i < s->ndargs; i++) i64(A, emit, &n, (uint64_t)s->dargs[i]);
        return n;
    }
    return 0;                                    /* lexer already validated dir */
}

/* Public: size (emit==0) or emit (emit==1) one statement. */
int encode_stmt(Assembler *A, Stmt *s, int emit)
{
    int n = 0;
    switch (s->kind) {
        case STK_LABEL: return 0;                /* handled by the driver       */
        case STK_DIR:   return encode_dir(A, s, emit);
        case STK_INSN:
            if (A->cur != SEC_TEXT)              /* code only makes sense in .text */
                return emit_error(A, s, "instruction outside .text section");
            return encode_insn(A, s, emit, &n);
    }
    return 0;
}
