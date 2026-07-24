/* ===========================================================================
 * disasm.c — the x86-64 instruction decoder + AT&T/Intel formatter.
 * ===========================================================================
 *
 * READ ME FIRST. The decode pipeline is one linear walk over the byte stream,
 * in exactly the order the CPU's own front-end consumes bytes:
 *
 *   1. legacy prefixes   (0..4 bytes; groups 1-4, any order)
 *   2. REX prefix        (0..1 byte, 0x40..0x4F; MUST be the last prefix)
 *   3. opcode            (1 byte, or 0x0F + 1 byte for the two-byte map)
 *   4. ModR/M            (0..1 byte; present iff the opcode needs an operand
 *                         addressed by mod/reg/rm)
 *   5. SIB               (0..1 byte; present iff ModR/M says "rm == 100" in a
 *                         memory form — i.e. "I need a scale/index/base byte")
 *   6. displacement      (0/1/2/4 bytes; size dictated by mod + the RIP/SIB rules)
 *   7. immediate         (0/1/2/4/8 bytes; size dictated by the opcode + REX.W/66)
 *
 * The single hardest part — and the thing this project exists to teach — is
 * steps 4-6: how ModR/M+SIB decide whether a SIB byte and a displacement follow,
 * and how the RSP/RBP encoding "slots" get stolen for the SIB and RIP-relative
 * escapes. That logic lives in decode_rm() below and is mirrored, standalone,
 * in asm/demo.c.
 * ===========================================================================
 */
#include "disasm.h"
#include <string.h>   /* memset, strcmp, strlen                                */
#include <stdio.h>    /* snprintf                                              */
#include <stdarg.h>   /* va_list, va_start (used by the Sink formatter helper) */

/* ===========================================================================
 * SECTION 1 — name tables. Pure data: how a register index prints.
 * ===========================================================================
 * The 16 GPRs are numbered rax=0,rcx=1,rdx=2,rbx=3,rsp=4,rbp=5,rsi=6,rdi=7,
 * r8=8..r15=15. That ordering is the ABI's, and it is NOT alphabetical — it is
 * the historical 8086 order (a,c,d,b) with the new regs appended. Every width
 * is the SAME physical register seen through a different-sized window.
 */
static const char *R64[16] = {"rax","rcx","rdx","rbx","rsp","rbp","rsi","rdi",
                              "r8","r9","r10","r11","r12","r13","r14","r15"};
static const char *R32[16] = {"eax","ecx","edx","ebx","esp","ebp","esi","edi",
                              "r8d","r9d","r10d","r11d","r12d","r13d","r14d","r15d"};
static const char *R16[16] = {"ax","cx","dx","bx","sp","bp","si","di",
                              "r8w","r9w","r10w","r11w","r12w","r13w","r14w","r15w"};
/* Byte regs WHEN a REX prefix is present: indices 4..7 become spl/bpl/sil/dil. */
static const char *R8[16]  = {"al","cl","dl","bl","spl","bpl","sil","dil",
                              "r8b","r9b","r10b","r11b","r12b","r13b","r14b","r15b"};
/* Byte regs with NO REX: indices 4..7 are the legacy HIGH bytes ah/ch/dh/bh.
 * This split is why the REX prefix is "sticky": once any REX byte appears, you
 * can no longer name ah/ch/dh/bh in that instruction. */
static const char *R8L[8]  = {"al","cl","dl","bl","ah","ch","dh","bh"};

/* Segment registers, indexed by the override-prefix decode below. */
static const char *SEG[6]  = {"es","cs","ss","ds","fs","gs"};

/* Condition-code suffixes, indexed by the low nibble of a jcc/setcc/cmovcc
 * opcode. This 16-entry table is the whole reason those instructions come in
 * blocks of 16: the condition IS the low 4 opcode bits. */
static const char *CC[16]  = {"o","no","b","ae","e","ne","be","a",
                              "s","ns","p","np","l","ge","le","g"};

/* Map a segment-override prefix byte to a 0..5 index into SEG[], or -1. */
static int seg_index(int pfx) {
    switch (pfx) {
        case 0x26: return 0; /* es */   case 0x2E: return 1; /* cs */
        case 0x36: return 2; /* ss */   case 0x3E: return 3; /* ds */
        case 0x64: return 4; /* fs */   case 0x65: return 5; /* gs */
        default:   return -1;
    }
}

/* ===========================================================================
 * SECTION 2 — the opcode model.
 * ===========================================================================
 * Each opcode maps to an OpEntry: a mnemonic plus up to three operand *type*
 * codes (in Intel order: op1 = destination). The type codes below are the
 * classic Intel "addressing method + operand type" letters, pared to what we
 * decode. The decoder reads these to know how many bytes to eat and how to
 * print them.
 */
enum OpType {
    OT_NONE = 0,
    /* r/m operand (register OR memory, selected by ModR/M.mod). Letter 'E'. */
    OT_Eb, OT_Ev, OT_Ew, OT_Ed,      /* byte / operand-size / word / dword     */
    /* register operand from ModR/M.reg. Letter 'G'. */
    OT_Gb, OT_Gv,
    /* immediate. Letter 'I'. z = 16/32 (never 64); v = operand size incl 64. */
    OT_Ib, OT_Iz, OT_Iv, OT_Iw,
    /* branch displacement relative to the *next* instruction. Letter 'J'. */
    OT_Jb, OT_Jz,
    OT_M,                            /* memory-only (lea): 'M'                 */
    OT_AL, OT_rAX,                   /* implied accumulator, byte / op-size    */
    OT_ONE, OT_CL,                   /* implied shift count: the literal 1, or cl */
    OT_rEMB,                         /* register in the low 3 opcode bits, op-size (push/pop) */
    OT_rEMB_b,                       /* ditto, byte  (mov r8, imm8)            */
    OT_rEMB_v                        /* ditto, op-size incl 64 (mov r, imm; movabs) */
};

/* Group ids: opcodes whose *mnemonic* is chosen by the ModR/M.reg field. */
enum { GRP_NONE=0, GRP_ALU, GRP_SHIFT, GRP_F6, GRP_FE, GRP_FF, GRP_MOV };

/* F_D64: in 64-bit mode these default to a 64-bit operand regardless of REX.W
 * (push/pop and near call/jmp operate on 64-bit stack/branch pointers). A 0x66
 * prefix still shrinks them to 16-bit. */
enum { F_D64 = 1 };

typedef struct {
    const char *mnem;   /* NULL until a group resolves it, or if unsupported   */
    int  op1, op2, op3; /* operand type codes, Intel order (op1 = destination) */
    int  group;         /* GRP_* or GRP_NONE                                   */
    int  flags;         /* F_D64                                               */
    bool indirect;      /* call/jmp through a register/memory operand (AT&T '*')*/
} OpEntry;

/* Mnemonic tables for the reg-field groups. Index = ModR/M.reg (0..7). */
static const char *G_ALU[8]   = {"add","or","adc","sbb","and","sub","xor","cmp"};
/* /4 and /6 are both "shift left"; objdump prints "shl" for each. */
static const char *G_SHIFT[8] = {"rol","ror","rcl","rcr","shl","shr","shl","sar"};
static const char *G_F6[8]    = {"test","test","not","neg","mul","imul","div","idiv"};

/* --------------------------------------------------------------------------
 * lookup_primary — decode a 1-byte-map opcode into an OpEntry.
 * Returns false for opcodes outside our teaching subset (caller prints "(bad)").
 * The four *ranges* (ALU grid, push/pop, jcc, mov-imm) are handled first because
 * they are perfectly regular — seeing that regularity in code IS the lesson.
 * -------------------------------------------------------------------------- */
static bool lookup_primary(uint8_t op, OpEntry *e) {
    memset(e, 0, sizeof *e);

    /* (a) The eight classic ALU ops tile 0x00..0x3F in a 8x8 grid: the high
     *     bits pick the op (add/or/adc/sbb/and/sub/xor/cmp), the low 3 pick the
     *     operand form. Columns 6/7 of each row are the old push/pop-segment
     *     encodings, illegal in 64-bit mode — we let them fall through to bad. */
    if (op < 0x40 && (op & 7) < 6) {
        e->mnem = G_ALU[op >> 3];
        switch (op & 7) {
            case 0: e->op1=OT_Eb;  e->op2=OT_Gb; break; /* ALU r/m8,  reg8      */
            case 1: e->op1=OT_Ev;  e->op2=OT_Gv; break; /* ALU r/m,   reg       */
            case 2: e->op1=OT_Gb;  e->op2=OT_Eb; break; /* ALU reg8,  r/m8      */
            case 3: e->op1=OT_Gv;  e->op2=OT_Ev; break; /* ALU reg,   r/m       */
            case 4: e->op1=OT_AL;  e->op2=OT_Ib; break; /* ALU al,    imm8      */
            case 5: e->op1=OT_rAX; e->op2=OT_Iz; break; /* ALU eax/rax, imm32   */
        }
        return true;
    }
    /* (b) push/pop a register, encoded in the low 3 opcode bits (+REX.B). */
    if (op >= 0x50 && op <= 0x57) { e->mnem="push"; e->op1=OT_rEMB; e->flags=F_D64; return true; }
    if (op >= 0x58 && op <= 0x5F) { e->mnem="pop";  e->op1=OT_rEMB; e->flags=F_D64; return true; }
    /* (c) short conditional jumps: 16 opcodes, condition in the low nibble. */
    if (op >= 0x70 && op <= 0x7F) {
        static char buf[16][4];                 /* build "j"+cc once, lazily    */
        char *m = buf[op & 0xF];
        if (!m[0]) { m[0]='j'; strcpy(m+1, CC[op & 0xF]); }
        e->mnem = m; e->op1 = OT_Jb; return true;
    }
    /* (d) mov a register with an immediate; B8+r with REX.W is `movabs` (imm64).*/
    if (op >= 0xB0 && op <= 0xB7) { e->mnem="mov"; e->op1=OT_rEMB_b; e->op2=OT_Ib; return true; }
    if (op >= 0xB8 && op <= 0xBF) { e->mnem="mov"; e->op1=OT_rEMB_v; e->op2=OT_Iv; return true; }

    /* (e) the irregular singletons. */
    switch (op) {
        /* MOVSXD r64, r/m32 (0x63): the one instruction that sign-extends a
         * 32-bit source to 64 bits. AT&T prints it "movslq". */
        case 0x63: e->mnem="movsxd"; e->op1=OT_Gv; e->op2=OT_Ed; return true;

        case 0x68: e->mnem="push"; e->op1=OT_Iz; e->flags=F_D64; return true;
        case 0x6A: e->mnem="push"; e->op1=OT_Ib; e->flags=F_D64; return true;
        /* three-operand imul: dst = src * imm. */
        case 0x69: e->mnem="imul"; e->op1=OT_Gv; e->op2=OT_Ev; e->op3=OT_Iz; return true;
        case 0x6B: e->mnem="imul"; e->op1=OT_Gv; e->op2=OT_Ev; e->op3=OT_Ib; return true;

        /* Group 1 (immediate ALU). Mnemonic from reg field; see resolve_group. */
        case 0x80: e->group=GRP_ALU; e->op1=OT_Eb; e->op2=OT_Ib; return true;
        case 0x81: e->group=GRP_ALU; e->op1=OT_Ev; e->op2=OT_Iz; return true;
        case 0x83: e->group=GRP_ALU; e->op1=OT_Ev; e->op2=OT_Ib; return true; /* imm8 sign-extended */

        case 0x84: e->mnem="test"; e->op1=OT_Eb; e->op2=OT_Gb; return true;
        case 0x85: e->mnem="test"; e->op1=OT_Ev; e->op2=OT_Gv; return true;
        case 0x86: e->mnem="xchg"; e->op1=OT_Eb; e->op2=OT_Gb; return true;
        case 0x87: e->mnem="xchg"; e->op1=OT_Ev; e->op2=OT_Gv; return true;
        case 0x88: e->mnem="mov";  e->op1=OT_Eb; e->op2=OT_Gb; return true;
        case 0x89: e->mnem="mov";  e->op1=OT_Ev; e->op2=OT_Gv; return true;
        case 0x8A: e->mnem="mov";  e->op1=OT_Gb; e->op2=OT_Eb; return true;
        case 0x8B: e->mnem="mov";  e->op1=OT_Gv; e->op2=OT_Ev; return true;
        case 0x8D: e->mnem="lea";  e->op1=OT_Gv; e->op2=OT_M;  return true;
        case 0x8F: e->mnem="pop";  e->op1=OT_Ev; e->flags=F_D64; return true; /* group 1A /0 */

        case 0x90: e->mnem="nop"; return true;                 /* xchg eax,eax */
        case 0x98: e->mnem="cwde"; return true;   /* size/syntax fixed up later */
        case 0x99: e->mnem="cdq";  return true;   /* size/syntax fixed up later */

        case 0xA8: e->mnem="test"; e->op1=OT_AL;  e->op2=OT_Ib; return true;
        case 0xA9: e->mnem="test"; e->op1=OT_rAX; e->op2=OT_Iz; return true;

        /* Group 2 (shifts/rotates). */
        case 0xC0: e->group=GRP_SHIFT; e->op1=OT_Eb; e->op2=OT_Ib;  return true;
        case 0xC1: e->group=GRP_SHIFT; e->op1=OT_Ev; e->op2=OT_Ib;  return true;
        case 0xD0: e->group=GRP_SHIFT; e->op1=OT_Eb; e->op2=OT_ONE; return true;
        case 0xD1: e->group=GRP_SHIFT; e->op1=OT_Ev; e->op2=OT_ONE; return true;
        case 0xD2: e->group=GRP_SHIFT; e->op1=OT_Eb; e->op2=OT_CL;  return true;
        case 0xD3: e->group=GRP_SHIFT; e->op1=OT_Ev; e->op2=OT_CL;  return true;

        case 0xC2: e->mnem="ret"; e->op1=OT_Iw; return true;   /* ret imm16     */
        case 0xC3: e->mnem="ret"; return true;
        case 0xC6: e->group=GRP_MOV; e->op1=OT_Eb; e->op2=OT_Ib; return true;
        case 0xC7: e->group=GRP_MOV; e->op1=OT_Ev; e->op2=OT_Iz; return true;
        case 0xC9: e->mnem="leave"; return true;
        case 0xCC: e->mnem="int3";  return true;

        case 0xE8: e->mnem="call"; e->op1=OT_Jz; return true;
        case 0xE9: e->mnem="jmp";  e->op1=OT_Jz; return true;
        case 0xEB: e->mnem="jmp";  e->op1=OT_Jb; return true;

        case 0xF4: e->mnem="hlt"; return true;
        /* Group 3 (test/not/neg/mul/imul/div/idiv). */
        case 0xF6: e->group=GRP_F6; e->op1=OT_Eb; return true;
        case 0xF7: e->group=GRP_F6; e->op1=OT_Ev; return true;
        /* Group 4/5 (inc/dec byte; inc/dec/call/jmp/push). */
        case 0xFE: e->group=GRP_FE; e->op1=OT_Eb; return true;
        case 0xFF: e->group=GRP_FF; e->op1=OT_Ev; return true;
    }
    return false;   /* not in our subset */
}

/* --------------------------------------------------------------------------
 * lookup_0f — decode a 0x0F two-byte-map opcode. Same contract as above.
 * -------------------------------------------------------------------------- */
static bool lookup_0f(uint8_t op, OpEntry *e) {
    memset(e, 0, sizeof *e);

    /* cmovcc r, r/m  (16 opcodes) */
    if (op >= 0x40 && op <= 0x4F) {
        static char buf[16][8];
        char *m = buf[op & 0xF];
        if (!m[0]) { strcpy(m, "cmov"); strcat(m, CC[op & 0xF]); }
        e->mnem = m; e->op1 = OT_Gv; e->op2 = OT_Ev; return true;
    }
    /* jcc rel32 (near conditional jumps) */
    if (op >= 0x80 && op <= 0x8F) {
        static char buf[16][4];
        char *m = buf[op & 0xF];
        if (!m[0]) { m[0]='j'; strcpy(m+1, CC[op & 0xF]); }
        e->mnem = m; e->op1 = OT_Jz; return true;
    }
    /* setcc r/m8 */
    if (op >= 0x90 && op <= 0x9F) {
        static char buf[16][8];
        char *m = buf[op & 0xF];
        if (!m[0]) { strcpy(m, "set"); strcat(m, CC[op & 0xF]); }
        e->mnem = m; e->op1 = OT_Eb; return true;
    }
    switch (op) {
        case 0x05: e->mnem="syscall"; return true;   /* the whole point of x86-64 */
        case 0x0B: e->mnem="ud2";     return true;
        case 0x1F: e->mnem="nop"; e->op1=OT_Ev; return true;  /* multi-byte nop  */
        case 0x31: e->mnem="rdtsc";   return true;
        case 0xA2: e->mnem="cpuid";   return true;
        case 0xAF: e->mnem="imul"; e->op1=OT_Gv; e->op2=OT_Ev; return true;
        case 0xB6: e->mnem="movzx"; e->op1=OT_Gv; e->op2=OT_Eb; return true;
        case 0xB7: e->mnem="movzx"; e->op1=OT_Gv; e->op2=OT_Ew; return true;
        case 0xBE: e->mnem="movsx"; e->op1=OT_Gv; e->op2=OT_Eb; return true;
        case 0xBF: e->mnem="movsx"; e->op1=OT_Gv; e->op2=OT_Ew; return true;
    }
    return false;
}

/* resolve_group — a group opcode's mnemonic lives in the ModR/M.reg field.
 * `reg` is the raw 3-bit reg field (NOT REX-extended: the extension names GPRs,
 * not opcode sub-functions). */
static void resolve_group(OpEntry *e, int reg, uint8_t opcode) {
    switch (e->group) {
        case GRP_ALU:   e->mnem = G_ALU[reg];   break;
        case GRP_SHIFT: e->mnem = G_SHIFT[reg]; break;
        case GRP_MOV:   e->mnem = "mov";        break; /* only /0 is defined     */
        case GRP_F6:
            e->mnem = G_F6[reg];
            /* Only test (/0,/1) carries an immediate; the rest are 1-operand. */
            if (reg < 2) e->op2 = (opcode == 0xF6) ? OT_Ib : OT_Iz;
            else         e->op2 = OT_NONE;
            break;
        case GRP_FE:
            e->mnem = (reg == 0) ? "inc" : (reg == 1) ? "dec" : NULL;
            break;
        case GRP_FF:
            switch (reg) {
                case 0: e->mnem="inc"; break;
                case 1: e->mnem="dec"; break;
                case 2: e->mnem="call"; e->flags |= F_D64; e->indirect = true; break;
                case 4: e->mnem="jmp";  e->flags |= F_D64; e->indirect = true; break;
                case 6: e->mnem="push"; e->flags |= F_D64; break;
                default: e->mnem = NULL; break;  /* far call/jmp (3,5), bad (7)  */
            }
            break;
    }
}

/* ===========================================================================
 * SECTION 3 — little byte-reading helpers (all bounds-checked).
 * ===========================================================================
 */
static bool read_le(const uint8_t *c, size_t n, int *p, int size, uint64_t *out) {
    if ((size_t)(*p + size) > n) return false;   /* would read past the buffer  */
    uint64_t v = 0;
    for (int i = 0; i < size; i++) v |= (uint64_t)c[*p + i] << (8 * i); /* LE     */
    *out = v; *p += size; return true;
}
/* Sign-extend the low `bits` of v to a full 64-bit two's-complement value. */
static int64_t sext(uint64_t v, int bits) {
    if (bits >= 64) return (int64_t)v;
    uint64_t m = (uint64_t)1 << (bits - 1);      /* the sign bit                 */
    uint64_t lo = ((uint64_t)1 << bits) - 1;     /* mask of the value bits       */
    v &= lo;
    if (v & m) v |= ~lo;                         /* set all the bits above it    */
    return (int64_t)v;
}
static uint64_t mask_bits(uint64_t v, int bits) {
    if (bits >= 64) return v;
    return v & (((uint64_t)1 << bits) - 1);
}
/* Effective operand size in BITS: REX.W wins (64), else 0x66 shrinks to 16,
 * else the long-mode default of 32. */
static int op_bits(const Insn *in) {
    if (in->rexW) return 64;
    if (in->p66)  return 16;
    return 32;
}

/* Fill a register operand, honoring the REX / high-byte rule. */
static void set_reg(const Insn *in, Operand *o, int idx, int bits) {
    o->kind = OPK_REG;
    o->reg_bits = bits;
    o->reg = idx;
    /* ah/ch/dh/bh only exist when NO REX prefix is present and idx is 4..7. */
    o->reg_high8 = (bits == 8 && !in->rex && idx >= 4 && idx < 8);
}

/* Size in bits of an operand *type* code, given the current operand size and
 * the F_D64 flag (which forces the op-size operand to 64 for stack/branch ops).*/
static int ot_bits(int ot, const Insn *in, int flags) {
    switch (ot) {
        case OT_Eb: case OT_Gb: case OT_AL: return 8;
        case OT_Ew:                          return 16;
        case OT_Ed:                          return 32;   /* movsxd source       */
        case OT_Ev: case OT_Gv: case OT_M:
        case OT_rAX: case OT_rEMB:
            if (flags & F_D64) return in->p66 ? 16 : 64;
            return op_bits(in);
        default: return op_bits(in);
    }
}
static bool ot_is_rm(int ot) {
    return ot==OT_Eb||ot==OT_Ev||ot==OT_Ew||ot==OT_Ed||ot==OT_M;
}
static bool ot_is_reg(int ot) { return ot==OT_Gb||ot==OT_Gv; }
static bool ot_is_imm(int ot) {
    return ot==OT_Ib||ot==OT_Iz||ot==OT_Iv||ot==OT_Iw;
}

/* ===========================================================================
 * SECTION 4 — decode_rm: the ModR/M + SIB + displacement core. THE HEART.
 * ===========================================================================
 * On entry `*p` indexes the ModR/M byte. We fill `o` with either a register
 * (mod==11) or a fully-resolved memory reference, consuming the ModR/M, an
 * optional SIB, and 0/1/2/4 displacement bytes. Returns the new cursor, or -1
 * if the stream is truncated. `rm_bits` is the operand size at this location
 * (drives byte/word/dword register naming and the AT&T suffix / Intel ptr hint).
 *
 * The three "stolen encodings" every decoder must get right:
 *   - mod!=11 && rm==100  ->  a SIB byte follows (RSP can't be a plain base).
 *   - mod==00 && rm==101  ->  RIP-relative disp32 (no base reg) in 64-bit mode.
 *   - in SIB: base==101 && mod==00 -> no base, a disp32 follows instead.
 *             index==100 && REX.X==0 -> no index register (RSP can't be index).
 * ===========================================================================
 */
static int decode_rm(Insn *in, const uint8_t *code, size_t n, int p,
                     int rm_bits, Operand *o) {
    memset(o, 0, sizeof *o);
    o->seg = seg_index(in->seg_pfx);   /* segment override, or -1               */
    o->scale = 1;
    o->mem_bits = rm_bits;

    uint8_t modrm = code[p++];          /* consume ModR/M                        */
    in->has_modrm = true; in->modrm = modrm;
    int mod = (modrm >> 6) & 3;
    int rm  =  modrm       & 7;

    /* mod==11: r/m is a REGISTER, not memory. Extend it with REX.B. Done. */
    if (mod == 3) {
        set_reg(in, o, rm | (in->rexB << 3), rm_bits);
        return p;
    }

    /* Address-size override (0x67) selects 32-bit base/index registers. Rare;
     * long-mode code virtually always uses 64-bit addressing. */
    int addr_bits_unused = in->p67 ? 32 : 64; (void)addr_bits_unused;
    o->kind = OPK_MEM;

    bool need_sib = (rm == 4);          /* rm==100 in a memory form => SIB       */

    if (need_sib) {
        if ((size_t)p >= n) return -1;
        uint8_t sib = code[p++];
        in->has_sib = true; in->sib = sib;
        int ss    = (sib >> 6) & 3;             /* scale exponent: 1<<ss = 1/2/4/8 */
        int idx3  = (sib >> 3) & 7;             /* index field (pre-REX)          */
        int base3 =  sib       & 7;             /* base  field (pre-REX)          */

        /* Index: field 100 with REX.X==0 means "no index" (RSP is not indexable).
         * With REX.X==1 the same field becomes r12, which IS a valid index. */
        if (idx3 == 4 && !in->rexX) {
            o->has_index = false;
        } else {
            o->has_index = true;
            o->index = idx3 | (in->rexX << 3);
            o->scale = 1 << ss;
        }

        /* Base: field 101 with mod==00 means "no base" — a disp32 stands alone
         * (optionally plus the scaled index). REX.B does NOT rescue the base
         * here; r13 as a base needs mod!=00 (that is the famous [r13] -> [r13+0]
         * quirk). Otherwise the base is base3, extended by REX.B. */
        if (base3 == 5 && mod == 0) {
            o->has_base = false;
            uint64_t d;
            if (!read_le(code, n, &p, 4, &d)) return -1;
            o->disp = sext(d, 32); o->has_disp = true;
            return p;                            /* disp already consumed         */
        }
        o->has_base = true;
        o->base = base3 | (in->rexB << 3);
        /* fall through to the mod-driven displacement below */
    } else if (mod == 0 && rm == 5) {
        /* RIP-relative: the disp32 is added to the address of the NEXT
         * instruction. This is how position-independent code reaches globals. */
        o->rip_rel = true;
        uint64_t d;
        if (!read_le(code, n, &p, 4, &d)) return -1;
        o->disp = sext(d, 32); o->has_disp = true;
        return p;
    } else {
        /* plain [reg]: rm names the base register (extended by REX.B). */
        o->has_base = true;
        o->base = rm | (in->rexB << 3);
    }

    /* Displacement dictated by mod (for the base-register and SIB-with-base
     * cases; the no-base and RIP cases returned already). */
    if (mod == 1) {
        uint64_t d; if (!read_le(code, n, &p, 1, &d)) return -1;
        o->disp = sext(d, 8);  o->has_disp = true;    /* disp8, sign-extended    */
    } else if (mod == 2) {
        uint64_t d; if (!read_le(code, n, &p, 4, &d)) return -1;
        o->disp = sext(d, 32); o->has_disp = true;    /* disp32                  */
    }
    /* mod==0 with a base register => no displacement (implicit 0). */
    return p;
}

/* ===========================================================================
 * SECTION 5 — disasm_one: orchestrate the whole decode.
 * ===========================================================================
 */
int disasm_one(const uint8_t *code, size_t n, uint64_t addr, Insn *out) {
    memset(out, 0, sizeof *out);
    out->addr = addr;
    out->raw  = code;
    if (n == 0) { out->valid = false; out->len = 1; return 1; }

    int p = 0;

    /* --- (1) legacy prefixes: consume a run, one from each group. --------- */
    for (bool more = true; more && (size_t)p < n; ) {
        switch (code[p]) {
            case 0xF0: out->lock  = true; p++; break;      /* group 1: LOCK      */
            case 0xF2: out->repne = true; p++; break;      /* group 1: REPNE     */
            case 0xF3: out->rep   = true; p++; break;      /* group 1: REP       */
            case 0x2E: case 0x36: case 0x3E: case 0x26:    /* group 2: seg       */
            case 0x64: case 0x65: out->seg_pfx = code[p]; p++; break;
            case 0x66: out->p66 = true; p++; break;        /* group 3: op size   */
            case 0x67: out->p67 = true; p++; break;        /* group 4: addr size */
            default:   more = false; break;                /* not a prefix       */
        }
    }

    /* --- (2) REX prefix: must be the LAST prefix, immediately before opcode. */
    if ((size_t)p < n && (code[p] & 0xF0) == 0x40) {
        out->rex = true; out->rex_byte = code[p];
        out->rexW = (code[p] >> 3) & 1;
        out->rexR = (code[p] >> 2) & 1;
        out->rexX = (code[p] >> 1) & 1;
        out->rexB = (code[p] >> 0) & 1;
        p++;
    }

    /* --- (3) opcode: primary map, or 0x0F escape to the two-byte map. ----- */
    if ((size_t)p >= n) { out->valid = false; out->len = p ? p : 1; return out->len; }
    OpEntry e;
    bool ok;
    if (code[p] == 0x0F) {
        out->map = 1; p++;
        if ((size_t)p >= n) { out->valid = false; out->len = p; return p; }
        out->opcode = code[p++];
        ok = lookup_0f(out->opcode, &e);
    } else {
        out->map = 0;
        out->opcode = code[p++];
        ok = lookup_primary(out->opcode, &e);
    }
    /* Special case: the CET markers endbr64/endbr32 = F3 0F 1E FA/FB. They are
     * directly relevant to the defense lesson (every valid indirect-branch
     * target in a CET-hardened binary begins with one), so we decode them by
     * name. Any other 0F 1E form is a reserved multi-byte NOP. */
    if (out->map == 1 && out->opcode == 0x1E) {
        if (out->rep && (size_t)p < n && (code[p] == 0xFA || code[p] == 0xFB)) {
            out->mnem = (code[p] == 0xFA) ? "endbr64" : "endbr32";
            out->has_modrm = true; out->modrm = code[p]; p++;
            out->len = p; out->nops = 0; out->valid = true;
            return out->len;
        }
        e.mnem = "nop"; e.op1 = OT_Ev; e.op2 = e.op3 = OT_NONE;
        e.group = 0; e.flags = 0; e.indirect = false; ok = true;
    }

    if (!ok) { out->valid = false; out->mnem = NULL; out->len = p; return p; }

    /* --- (4/5/6) ModR/M + SIB + displacement, if this form uses them. ----- */
    bool uses_modrm = e.group || ot_is_rm(e.op1) || ot_is_rm(e.op2) || ot_is_rm(e.op3)
                              || ot_is_reg(e.op1) || ot_is_reg(e.op2) || ot_is_reg(e.op3);
    int  reg_field = 0;         /* ModR/M.reg, REX.R-extended (a GPR number)     */
    int  reg_raw   = 0;         /* ModR/M.reg, raw 3 bits (a group sub-opcode)   */
    Operand rm_op; memset(&rm_op, 0, sizeof rm_op); rm_op.seg = -1;

    if (uses_modrm) {
        if ((size_t)p >= n) { out->valid = false; out->len = p; return p; }
        reg_raw   = (code[p] >> 3) & 7;
        reg_field = reg_raw | (out->rexR << 3);

        /* Resolve a group's mnemonic BEFORE we size the r/m operand, because
         * the group can flip F_D64 (FF /2 call, /6 push) which changes the size. */
        if (e.group) resolve_group(&e, reg_raw, out->opcode);
        if (!e.mnem) { out->valid = false; out->len = p + 1; return p + 1; }

        /* Which operand slot is the r/m one? That decides its size. */
        int rm_ot = ot_is_rm(e.op1) ? e.op1 : ot_is_rm(e.op2) ? e.op2 :
                    ot_is_rm(e.op3) ? e.op3 : OT_Ev;
        int rm_bits = ot_bits(rm_ot, out, e.flags);

        int np = decode_rm(out, code, n, p, rm_bits, &rm_op);
        if (np < 0) { out->valid = false; out->len = n; return (int)n; }
        p = np;
        if (e.indirect) rm_op.indirect = true;
    }

    /* --- read the single immediate / relative field, if any. -------------- */
    int osz = op_bits(out);
    /* The immediate's *printed width* follows the destination operand's size,
     * except: shift counts print as a small byte; a sole immediate (push/ret)
     * follows d64 / its own width. */
    int imm_ref_bits;
    if (ot_is_imm(e.op1) || e.op1==OT_Jb || e.op1==OT_Jz) {
        imm_ref_bits = (e.op1==OT_Iw) ? 16
                     : (e.flags & F_D64) ? (out->p66 ? 16 : 64)
                     : osz;
    } else {
        imm_ref_bits = ot_bits(e.op1, out, e.flags);
        if (e.group == GRP_SHIFT) imm_ref_bits = 8;
    }

    Operand imm_op; memset(&imm_op, 0, sizeof imm_op); imm_op.seg = -1;
    bool have_imm = false;
    int  imm_slot_ot = ot_is_imm(e.op1) ? e.op1 :
                       ot_is_imm(e.op2) ? e.op2 :
                       ot_is_imm(e.op3) ? e.op3 : OT_NONE;
    int  rel_slot_ot = (e.op1==OT_Jb||e.op1==OT_Jz) ? e.op1 : OT_NONE;

    if (imm_slot_ot != OT_NONE) {
        int rsz = (imm_slot_ot==OT_Ib) ? 1
                : (imm_slot_ot==OT_Iw) ? 2
                : (imm_slot_ot==OT_Iz) ? (osz==16 ? 2 : 4)   /* z never 64       */
                : /* OT_Iv */            (osz/8);            /* v = 2/4/8         */
        uint64_t raw;
        if (!read_le(code, n, &p, rsz, &raw)) { out->valid=false; out->len=n; return (int)n; }
        int64_t s = sext(raw, rsz * 8);                      /* sign-extend...    */
        imm_op.kind = OPK_IMM;
        imm_op.imm  = mask_bits((uint64_t)s, imm_ref_bits);  /* ...then mask      */
        imm_op.imm_bits = imm_ref_bits;
        have_imm = true;
    } else if (rel_slot_ot != OT_NONE) {
        int rsz = (rel_slot_ot==OT_Jb) ? 1 : (osz==16 ? 2 : 4);
        uint64_t raw;
        if (!read_le(code, n, &p, rsz, &raw)) { out->valid=false; out->len=n; return (int)n; }
        int64_t rel = sext(raw, rsz * 8);
        imm_op.kind = OPK_REL;
        imm_op.target = addr + (uint64_t)p + (uint64_t)rel;  /* p == final length */
        have_imm = true;
    }

    out->len = p;

    /* --- assemble ops[] in Intel order (op1 = destination). --------------- */
    int slots[3] = { e.op1, e.op2, e.op3 };
    out->nops = 0;
    for (int i = 0; i < 3; i++) {
        int ot = slots[i];
        if (ot == OT_NONE) continue;
        Operand o; memset(&o, 0, sizeof o); o.seg = -1;
        if (ot_is_rm(ot)) {
            o = rm_op; o.mem_bits = ot_bits(ot, out, e.flags);
        } else if (ot_is_reg(ot)) {
            set_reg(out, &o, reg_field, ot_bits(ot, out, e.flags));
        } else if (ot_is_imm(ot) || ot==OT_Jb || ot==OT_Jz) {
            o = imm_op; (void)have_imm;
        } else if (ot == OT_AL) {
            set_reg(out, &o, 0, 8);
        } else if (ot == OT_rAX) {
            set_reg(out, &o, 0, ot_bits(OT_rAX, out, e.flags));
        } else if (ot == OT_CL) {
            set_reg(out, &o, 1, 8);
        } else if (ot == OT_ONE) {
            /* The implicit shift-by-1: objdump prints it as bare "$1" (decimal,
             * no 0x). We flag that with imm_bits==0 so the formatter knows. */
            o.kind = OPK_IMM; o.imm = 1; o.imm_bits = 0;
        } else if (ot == OT_rEMB || ot == OT_rEMB_b || ot == OT_rEMB_v) {
            int idx  = (out->opcode & 7) | (out->rexB << 3);   /* reg in opcode   */
            int bits = (ot==OT_rEMB_b) ? 8 : ot_bits(OT_rEMB, out, e.flags);
            if (ot==OT_rEMB_v) bits = op_bits(out);            /* incl 64 (movabs)*/
            set_reg(out, &o, idx, bits);
        }
        out->ops[out->nops++] = o;
    }

    out->mnem = e.mnem;
    out->valid = (out->mnem != NULL);
    if (out->len < 1) out->len = 1;
    return out->len;
}

/* ===========================================================================
 * SECTION 6 — formatting. Two syntaxes over the same decoded operands.
 * ===========================================================================
 */
typedef struct { char *b; size_t cap; size_t len; } Sink;
static void app(Sink *s, const char *fmt, ...) {
    if (s->len >= s->cap) return;
    va_list ap; va_start(ap, fmt);
    int w = vsnprintf(s->b + s->len, s->cap - s->len, fmt, ap);
    va_end(ap);
    if (w > 0) s->len += (size_t)w;
    if (s->len >= s->cap) s->len = s->cap - 1;
}

static const char *rname(int idx, int bits, bool high8) {
    if (bits == 8)  return high8 ? R8L[idx & 7] : R8[idx & 15];
    if (bits == 16) return R16[idx & 15];
    if (bits == 32) return R32[idx & 15];
    return R64[idx & 15];
}
/* Signed hex the way objdump prints displacements: "-0x4", "0x10". */
static void app_disp(Sink *s, int64_t d) {
    if (d < 0) app(s, "-0x%llx", (unsigned long long)(-d));
    else       app(s, "0x%llx",  (unsigned long long)d);
}

/* Print a memory reference in AT&T: seg:disp(base,index,scale). */
static void mem_att(Sink *s, const Insn *in, const Operand *o) {
    if (o->seg >= 0) app(s, "%%%s:", SEG[o->seg]);
    if (o->rip_rel) {                        /* disp(%rip) + objdump-style target */
        app_disp(s, o->disp);
        app(s, "(%%rip)");
        return;
    }
    if (!o->has_base && !o->has_index) {     /* absolute: bare disp, no parens    */
        app(s, "0x%llx", (unsigned long long)o->disp);
        return;
    }
    int areg = in->p67 ? 32 : 64;            /* address-size of base/index regs   */
    if (o->has_disp) app_disp(s, o->disp);
    app(s, "(");
    if (o->has_base) app(s, "%%%s", rname(o->base, areg, false));
    if (o->has_index) app(s, ",%%%s,%d", rname(o->index, areg, false), o->scale);
    app(s, ")");
}

/* Print a memory reference in Intel: seg:[base+index*scale+disp]. */
static void mem_intel(Sink *s, const Insn *in, const Operand *o) {
    if (o->seg >= 0) app(s, "%s:", SEG[o->seg]);
    app(s, "[");
    int areg = in->p67 ? 32 : 64;
    bool wrote = false;
    if (o->rip_rel) { app(s, "rip"); wrote = true; }
    if (o->has_base) { app(s, "%s", rname(o->base, areg, false)); wrote = true; }
    if (o->has_index) {
        if (wrote) app(s, "+");
        app(s, "%s*%d", rname(o->index, areg, false), o->scale); wrote = true;
    }
    if (o->has_disp || !wrote) {
        if (wrote && o->disp >= 0) app(s, "+0x%llx", (unsigned long long)o->disp);
        else if (wrote)           app(s, "-0x%llx", (unsigned long long)(-o->disp));
        else                       app(s, "0x%llx", (unsigned long long)o->disp);
    }
    app(s, "]");
}

/* The AT&T mnemonic sometimes needs a size suffix (movl, addq) when no register
 * operand pins the size. Returns the suffix char, or 0 for none. */
static char att_suffix(const Insn *in) {
    /* Branch/stack ops never take a suffix in objdump's output. */
    if (!strcmp(in->mnem,"call")||!strcmp(in->mnem,"jmp")||
        !strcmp(in->mnem,"push")||!strcmp(in->mnem,"pop")) return 0;
    bool any_reg = false, any_mem = false; int mb = 0;
    for (int i = 0; i < in->nops; i++) {
        if (in->ops[i].kind == OPK_REG) any_reg = true;
        if (in->ops[i].kind == OPK_MEM) { any_mem = true; mb = in->ops[i].mem_bits; }
    }
    if (any_reg || !any_mem) return 0;       /* a register already fixes the size */
    switch (mb) { case 8: return 'b'; case 16: return 'w'; case 32: return 'l'; case 64: return 'q'; }
    return 0;
}

/* movzx/movsx/movsxd print with two size suffixes in AT&T (e.g. movzbl). Build
 * that name from the source and destination operand sizes. */
static const char *ext_name_att(const Insn *in, char *tmp) {
    char sd = 'b', dd = 'l';
    int ssz = in->ops[1].reg_bits ? in->ops[1].reg_bits : in->ops[1].mem_bits;
    int dsz = in->ops[0].reg_bits;
    sd = ssz==8?'b':ssz==16?'w':ssz==32?'l':'q';
    dd = dsz==16?'w':dsz==32?'l':'q';
    const char *base = (in->mnem[3]=='z') ? "movz" : "movs";  /* movZx vs movSx  */
    tmp[0]=base[0];tmp[1]=base[1];tmp[2]=base[2];tmp[3]=base[3];
    tmp[4]=sd; tmp[5]=dd; tmp[6]=0;
    return tmp;
}

/* Map the size/syntax-dependent sign-extend mnemonics to their printed form. */
static const char *fixup_mnem(const Insn *in, Syntax syn, char *tmp) {
    /* B8+r with REX.W loads a full 64-bit immediate — objdump names it movabs. */
    if (in->map==0 && in->opcode>=0xB8 && in->opcode<=0xBF && in->rexW)
        return "movabs";
    if (in->map==0 && in->opcode==0x98) {   /* cbw/cwde/cdqe                    */
        int b = op_bits(in);
        if (syn==SYN_ATT) return b==16?"cbtw":b==32?"cwtl":"cltq";
        return b==16?"cbw":b==32?"cwde":"cdqe";
    }
    if (in->map==0 && in->opcode==0x99) {   /* cwd/cdq/cqo                      */
        int b = op_bits(in);
        if (syn==SYN_ATT) return b==16?"cwtd":b==32?"cltd":"cqto";
        return b==16?"cwd":b==32?"cdq":"cqo";
    }
    if (syn==SYN_ATT && (!strcmp(in->mnem,"movzx")||!strcmp(in->mnem,"movsx")))
        return ext_name_att(in, tmp);
    if (syn==SYN_ATT && !strcmp(in->mnem,"movsxd")) {  /* -> movslq             */
        /* treat like movsx for the AT&T two-suffix name */
        Insn t = *in; t.mnem = "movsx"; return ext_name_att(&t, tmp);
    }
    return in->mnem;
}

static void print_operand(Sink *s, const Insn *in, const Operand *o, Syntax syn) {
    if (syn == SYN_ATT) {
        if (o->indirect) app(s, "*");
        switch (o->kind) {
            case OPK_REG: app(s, "%%%s", rname(o->reg, o->reg_bits, o->reg_high8)); break;
            case OPK_MEM: mem_att(s, in, o); break;
            case OPK_IMM:
                if (o->imm_bits == 0) app(s, "$%llu", (unsigned long long)o->imm);
                else app(s, "$0x%llx", (unsigned long long)o->imm);
                break;
            case OPK_REL: app(s, "0x%llx",  (unsigned long long)o->target); break;
            default: break;
        }
    } else { /* Intel */
        switch (o->kind) {
            case OPK_REG: app(s, "%s", rname(o->reg, o->reg_bits, o->reg_high8)); break;
            case OPK_MEM: {
                /* Intel shows a size hint when no register fixes the size. */
                bool any_reg = false;
                for (int i=0;i<in->nops;i++) if (in->ops[i].kind==OPK_REG) any_reg=true;
                if (!any_reg) {
                    const char *pt = o->mem_bits==8?"byte":o->mem_bits==16?"word":
                                     o->mem_bits==32?"dword":"qword";
                    app(s, "%s ptr ", pt);
                }
                mem_intel(s, in, o);
                break;
            }
            case OPK_IMM:
                if (o->imm_bits == 0) app(s, "%llu", (unsigned long long)o->imm);
                else app(s, "0x%llx", (unsigned long long)o->imm);
                break;
            case OPK_REL: app(s, "0x%llx", (unsigned long long)o->target); break;
            default: break;
        }
    }
}

int disasm_format(const Insn *in, Syntax syn, char *buf, size_t bufsz) {
    Sink s = { buf, bufsz, 0 };
    if (bufsz) buf[0] = 0;

    if (!in->valid || !in->mnem) {           /* undecodable: mimic objdump       */
        app(&s, "(bad)");
        return (int)s.len;
    }

    /* Prefixes that print as their own token (lock; rep/repne on string ops). */
    if (in->lock) app(&s, "lock ");
    /* (rep/repne are only meaningful on string ops, which are outside our
     *  subset, so we do not emit them as standalone tokens here.) */

    char tmp[16];
    const char *m = fixup_mnem(in, syn, tmp);

    if (syn == SYN_ATT) {
        char suf = att_suffix(in);
        if (suf) app(&s, "%s%c", m, suf);
        else     app(&s, "%s", m);
        /* AT&T lists operands source-first: iterate ops[] in REVERSE. */
        for (int i = in->nops - 1; i >= 0; i--) {
            app(&s, i == in->nops - 1 ? "\t" : ",");
            print_operand(&s, in, &in->ops[i], syn);
        }
    } else {
        app(&s, "%s", m);
        for (int i = 0; i < in->nops; i++) {
            app(&s, i == 0 ? "\t" : ",");
            print_operand(&s, in, &in->ops[i], syn);
        }
    }

    /* objdump-style helper: annotate a RIP-relative target with its absolute. */
    for (int i = 0; i < in->nops; i++) {
        if (in->ops[i].kind == OPK_MEM && in->ops[i].rip_rel) {
            uint64_t tgt = in->addr + (uint64_t)in->len + (uint64_t)in->ops[i].disp;
            app(&s, "        # 0x%llx", (unsigned long long)tgt);
        }
    }
    return (int)s.len;
}
