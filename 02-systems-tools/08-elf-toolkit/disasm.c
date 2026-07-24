/* ===========================================================================
 * disasm.c — a linear-sweep x86-64 instruction decoder (teaching subset).
 * ===========================================================================
 *
 * This file has NO system headers on purpose (see disasm.h): it is pure logic
 * over a byte buffer, so it compiles standalone and its teaching assembly can be
 * generated the same way asm/demo.c's is. Everything it needs — a tiny string
 * builder, hex formatting, register-name tables — it defines itself.
 *
 * THE x86-64 INSTRUCTION FORMAT (what we are counting our way through)
 * -------------------------------------------------------------------
 *   [legacy prefixes 0..4] [REX 0..1] opcode(1-3) [ModRM] [SIB] [disp] [imm]
 *
 *   legacy prefixes : 0x66 (operand-size), 0x67 (address-size), 0xF0 (lock),
 *                     0xF2/0xF3 (repne/rep — also SSE opcode selectors),
 *                     0x2E/36/3E/26/64/65 (segment overrides). Any order, then
 *   REX             : one optional byte 0x40..0x4F in 64-bit mode. Its four low
 *                     bits are W (64-bit operand), R (extend ModRM.reg),
 *                     X (extend SIB.index), B (extend ModRM.rm / SIB.base / opcode
 *                     reg). REX must be the LAST prefix, immediately before the
 *                     opcode — that is what lets `push r15` exist at all.
 *   opcode          : 1 byte, or 0x0F + 1 byte (two-byte), or 0x0F 38/3A + 1.
 *   ModRM           : mod(7:6) reg(5:3) rm(2:0). Present for most opcodes; picks
 *                     a register or a memory operand. mod==3 => register direct.
 *   SIB             : scale(7:6) index(5:3) base(2:0). Present only when a memory
 *                     ModRM has rm==100b — it encodes base+index*scale.
 *   disp            : 0, 1, or 4 bytes, sign-extended, chosen by mod/rm/SIB.
 *   imm             : 0..8 bytes, chosen by the opcode.
 *
 * Getting the SUM of those lengths right is the whole ball game for linear
 * sweep. The rendering (Intel-syntax text) is a bonus on top.
 * ===========================================================================
 */
#include "disasm.h"

/* ===========================================================================
 * Section 1 — a minimal string builder writing into insn.text (no libc).
 * =========================================================================== */
struct sbuf {
    char    *buf;   /* destination (insn.text)                                  */
    unsigned cap;   /* capacity in bytes, including room for the NUL            */
    unsigned len;   /* current length (index of the NUL)                        */
};

/* Append one char, but never overflow the buffer (drop on overflow). We always
 * keep a terminating NUL so the result is a valid C string even if truncated. */
static void sb_putc(struct sbuf *s, char c)
{
    if (s->len + 1 < s->cap) {   /* +1 leaves space for the trailing NUL        */
        s->buf[s->len++] = c;
        s->buf[s->len] = '\0';
    }
}

static void sb_puts(struct sbuf *s, const char *p)
{
    while (*p) sb_putc(s, *p++);
}

/* Append `v` as minimal-width lowercase hex WITHOUT a leading "0x". Callers add
 * the prefix, because sometimes we want "-0x.." (negative displacement). */
static void sb_hex(struct sbuf *s, d_u64 v)
{
    char tmp[16];               /* 16 hex digits is the max for a 64-bit value  */
    int  n = 0;
    if (v == 0) { sb_putc(s, '0'); return; }
    while (v) {                 /* peel hex digits least-significant first       */
        d_u32 nib = (d_u32)(v & 0xf);
        tmp[n++] = (char)(nib < 10 ? '0' + nib : 'a' + (nib - 10));
        v >>= 4;
    }
    while (n) sb_putc(s, tmp[--n]);  /* emit in the correct (reversed) order     */
}

/* "0x" + hex, the common case. */
static void sb_0xhex(struct sbuf *s, d_u64 v) { sb_puts(s, "0x"); sb_hex(s, v); }

/* ===========================================================================
 * Section 2 — register name tables. Same GPR encoding, four operand widths.
 * The index 0..15 is the architectural register number; REX.R/X/B supply bit 3.
 * =========================================================================== */
static const char *const R64[16] = {
    "rax","rcx","rdx","rbx","rsp","rbp","rsi","rdi",
    "r8","r9","r10","r11","r12","r13","r14","r15" };
static const char *const R32[16] = {
    "eax","ecx","edx","ebx","esp","ebp","esi","edi",
    "r8d","r9d","r10d","r11d","r12d","r13d","r14d","r15d" };
static const char *const R16[16] = {
    "ax","cx","dx","bx","sp","bp","si","di",
    "r8w","r9w","r10w","r11w","r12w","r13w","r14w","r15w" };
/* Byte registers WITH any REX prefix present: the "new" uniform low-byte set. */
static const char *const R8L[16] = {
    "al","cl","dl","bl","spl","bpl","sil","dil",
    "r8b","r9b","r10b","r11b","r12b","r13b","r14b","r15b" };
/* Byte registers WITHOUT REX: indices 4..7 are the legacy high-byte regs. */
static const char *const R8H[8] = { "al","cl","dl","bl","ah","ch","dh","bh" };
static const char *const XMM[16] = {
    "xmm0","xmm1","xmm2","xmm3","xmm4","xmm5","xmm6","xmm7",
    "xmm8","xmm9","xmm10","xmm11","xmm12","xmm13","xmm14","xmm15" };

/* size is one of 1/2/4/8 bytes. have_rex disambiguates the byte-register set. */
static const char *reg_name(int idx, int size, int have_rex)
{
    switch (size) {
    case 8: return R64[idx & 15];
    case 4: return R32[idx & 15];
    case 2: return R16[idx & 15];
    default: /* 1 */
        if (have_rex) return R8L[idx & 15];
        return R8H[idx & 7];       /* legacy: only 0..7 reachable without REX    */
    }
}

/* The size keyword objdump prints when the width is not implied by a register
 * operand, e.g. `mov QWORD PTR [rax], 0x1`. */
static const char *ptr_kw(int size)
{
    switch (size) { case 1: return "BYTE PTR ";  case 2: return "WORD PTR ";
                    case 4: return "DWORD PTR "; default: return "QWORD PTR "; }
}

/* Condition-code suffixes, indexed by the low nibble of a jcc/setcc/cmovcc
 * opcode. `jne` is index 5 (0x75), etc. */
static const char *const CC[16] = {
    "o","no","b","ae","e","ne","be","a","s","ns","p","np","l","ge","le","g" };

/* The eight ALU operations that share opcode structure across the low 0x00..0x3F
 * map and the 0x80/81/83 immediate groups. */
static const char *const ALU[8] = { "add","or","adc","sbb","and","sub","xor","cmp" };
/* Shift/rotate group (0xC0/C1/D0-D3), indexed by ModRM.reg. */
static const char *const SHIFT[8] = { "rol","ror","rcl","rcr","shl","shr","sal","sar" };

/* ===========================================================================
 * Section 3 — a byte cursor with bounds checking. Every read goes through here
 * so we can NEVER read past `max` (the last instruction may sit right at the end
 * of .text or of the mapped file).
 * =========================================================================== */
struct dec {
    const d_u8 *c;      /* base pointer (== insn's first byte)                   */
    unsigned    max;    /* number of safely-readable bytes at c                  */
    unsigned    pos;    /* cursor / bytes consumed so far                        */
    int         trunc;  /* set if a read ran off the end                        */
};

static d_u32 rd8(struct dec *d)
{
    if (d->pos >= d->max) { d->trunc = 1; return 0; }
    return d->c[d->pos++];
}
static d_u32 rd16(struct dec *d) { d_u32 a = rd8(d); return a | (rd8(d) << 8); }
static d_u32 rd32(struct dec *d)
{
    d_u32 a = rd8(d), b = rd8(d), e = rd8(d), f = rd8(d);
    return a | (b << 8) | (e << 16) | (f << 24);
}
static d_u64 rd64(struct dec *d)
{
    d_u64 lo = rd32(d), hi = rd32(d);
    return lo | (hi << 32);
}
/* Read an immediate of `sz` bytes and SIGN-EXTEND it to 64 bits (x86 immediates
 * for arithmetic are sign-extended into the wider operand). */
static d_i64 rd_imm_sext(struct dec *d, int sz)
{
    switch (sz) {
    case 1: return (d_i64)(signed char)rd8(d);
    case 2: return (d_i64)(short)rd16(d);
    case 4: return (d_i64)(int)rd32(d);
    default: return (d_i64)rd64(d);
    }
}

/* ===========================================================================
 * Section 4 — ModRM/SIB/displacement decoding.
 * =========================================================================== */
struct modrm {
    int   is_reg;      /* mod==3: operand is a register, not memory             */
    int   reg_field;   /* the /r field, REX.R-extended (0..15) — the "reg" opnd */
    int   rm_reg;      /* if is_reg: the r/m register, REX.B-extended           */
    /* memory form: */
    int   rip_rel;     /* RIP-relative (mod==0, rm==101b, no SIB)               */
    int   have_base, base;
    int   have_index, index, scale;
    d_i64 disp;
    int   ext;         /* the raw ModRM.reg (0..7) — used as a group /digit     */
};

/* Parse the ModRM byte (and SIB/disp as required), advancing the cursor. rex_*
 * are the extension bits; addr32 is set by a 0x67 prefix (32-bit addressing). */
static void parse_modrm(struct dec *d, int rex_R, int rex_X, int rex_B,
                        struct modrm *m)
{
    d_u32 modrm = rd8(d);
    int mod = (modrm >> 6) & 3;
    int reg = (modrm >> 3) & 7;
    int rm  = modrm & 7;

    m->is_reg = 0; m->rip_rel = 0;
    m->have_base = 0; m->have_index = 0; m->scale = 1; m->disp = 0;
    m->ext = reg;
    m->reg_field = reg | (rex_R ? 8 : 0);

    if (mod == 3) {                         /* register-direct r/m               */
        m->is_reg = 1;
        m->rm_reg = rm | (rex_B ? 8 : 0);
        return;
    }

    int disp_bytes = 0;
    if (rm == 4) {                          /* a SIB byte follows                */
        d_u32 sib = rd8(d);
        int ss   = (sib >> 6) & 3;
        int idx  = (sib >> 3) & 7;
        int base = sib & 7;
        /* index==100b means "no index register" (unless REX.X promotes it to
         * r12, which IS a valid index). */
        if (!(idx == 4 && !rex_X)) {
            m->have_index = 1;
            m->index = idx | (rex_X ? 8 : 0);
            m->scale = 1 << ss;
        }
        if (base == 5 && mod == 0) {        /* no base register, disp32 follows  */
            disp_bytes = 4;
        } else {
            m->have_base = 1;
            m->base = base | (rex_B ? 8 : 0);
        }
    } else if (rm == 5 && mod == 0) {       /* RIP-relative: [rip + disp32]      */
        m->rip_rel = 1;
        disp_bytes = 4;
    } else {                                /* plain [reg (+ disp)]              */
        m->have_base = 1;
        m->base = rm | (rex_B ? 8 : 0);
    }

    /* mod selects the displacement width (overriding the mod==0 special cases,
     * which already set disp_bytes = 4 where needed). */
    if (mod == 1) disp_bytes = 1;
    else if (mod == 2) disp_bytes = 4;

    if (disp_bytes) m->disp = rd_imm_sext(d, disp_bytes);
}

/* Render a memory operand `[base + index*scale + disp]` in Intel syntax. In
 * 64-bit mode the base/index registers are 64-bit names. */
static void render_mem(struct sbuf *s, const struct modrm *m)
{
    sb_putc(s, '[');
    int first = 1;
    if (m->rip_rel) { sb_puts(s, "rip"); first = 0; }
    if (m->have_base) { sb_puts(s, R64[m->base & 15]); first = 0; }
    if (m->have_index) {
        if (!first) sb_putc(s, '+');
        sb_puts(s, R64[m->index & 15]);
        sb_putc(s, '*');
        sb_putc(s, (char)('0' + m->scale));   /* scale is 1,2,4,8               */
        first = 0;
    }
    if (m->disp || first) {                   /* show disp (always if nothing else)*/
        if (first) {                          /* absolute [disp32]              */
            sb_0xhex(s, (d_u64)m->disp);
        } else if (m->disp < 0) {
            sb_puts(s, "-0x"); sb_hex(s, (d_u64)(-m->disp));
        } else {
            sb_puts(s, "+0x"); sb_hex(s, (d_u64)m->disp);
        }
    }
    sb_putc(s, ']');
}

/* Render the r/m operand: a register (width `size`) or a memory reference. When
 * `size` is 0 we treat rm-as-register as an xmm register (SSE forms). If the
 * operand is memory and `ptr_size` != 0, prepend the "DWORD PTR"-style keyword. */
static void render_rm(struct sbuf *s, const struct modrm *m, int size,
                      int have_rex, int ptr_size, int xmm)
{
    if (m->is_reg) {
        if (xmm) sb_puts(s, XMM[m->rm_reg & 15]);
        else     sb_puts(s, reg_name(m->rm_reg, size, have_rex));
    } else {
        if (ptr_size) sb_puts(s, ptr_kw(ptr_size));
        render_mem(s, m);
    }
}

/* ===========================================================================
 * Section 5 — the main decoder.
 * =========================================================================== */
unsigned x86_decode(const d_u8 *code, unsigned maxlen, d_u64 addr, struct insn *out)
{
    struct dec d = { code, maxlen < X86_MAX_INSN_LEN ? maxlen : X86_MAX_INSN_LEN, 0, 0 };
    struct sbuf s = { out->text, sizeof out->text, 0 };
    out->text[0] = '\0';
    out->addr = addr;
    out->has_target = 0;
    out->target = 0;

    /* --- collect legacy prefixes -------------------------------------------- */
    /* We track operand-size (0x66), rep/repne (0xF3/0xF2 — also SSE selectors),
     * and lock. The address-size override (0x67) and segment overrides only
     * affect memory-operand rendering we don't specialize, so we consume them
     * for correct LENGTH but don't store a flag. */
    int p66 = 0, pF3 = 0, pF2 = 0, lock = 0;
    for (;;) {
        d_u32 b = (d.pos < d.max) ? d.c[d.pos] : 0x100; /* 0x100 == "no byte"    */
        if (b == 0x66) { p66 = 1; d.pos++; }
        else if (b == 0x67) { d.pos++; }
        else if (b == 0xF0) { lock = 1; d.pos++; }
        else if (b == 0xF3) { pF3 = 1; d.pos++; }
        else if (b == 0xF2) { pF2 = 1; d.pos++; }
        else if (b == 0x2E || b == 0x36 || b == 0x3E ||
                 b == 0x26 || b == 0x64 || b == 0x65) { d.pos++; } /* seg: ignore */
        else break;
    }

    /* --- REX prefix (must be the byte right before the opcode) --------------- */
    int rexW = 0, rexR = 0, rexX = 0, rexB = 0, have_rex = 0;
    {
        d_u32 b = (d.pos < d.max) ? d.c[d.pos] : 0x100;
        if (b >= 0x40 && b <= 0x4f) {
            have_rex = 1;
            rexW = (b >> 3) & 1; rexR = (b >> 2) & 1;
            rexX = (b >> 1) & 1; rexB = b & 1;
            d.pos++;
        }
    }

    /* The default integer operand width in 64-bit mode: REX.W => 8, 0x66 => 2,
     * otherwise 4. (push/pop/branches override this to 8 locally.) */
    int osz = rexW ? 8 : (p66 ? 2 : 4);
    /* A RIP-relative operand needs the final instruction length to resolve its
     * absolute target; remember it and patch a comment in at the very end. */
    int   rip_used = 0; d_i64 rip_disp = 0;

    if (lock) sb_puts(&s, "lock ");

    d_u32 op = rd8(&d);

    /* ======================================================================
     * TWO-BYTE OPCODES (0x0F xx)
     * ====================================================================== */
    if (op == 0x0f) {
        d_u32 op2 = rd8(&d);

        /* endbr64 / endbr32: F3 0F 1E FA / FB — control-flow-integrity landing
         * pads the compiler puts at function entries. */
        if (op2 == 0x1e && pF3) {
            d_u32 m = rd8(&d);
            if (m == 0xfa) sb_puts(&s, "endbr64");
            else if (m == 0xfb) sb_puts(&s, "endbr32");
            else sb_puts(&s, "nop");
            goto done;
        }
        if (op2 == 0x05) { sb_puts(&s, "syscall"); goto done; }
        if (op2 == 0x0b) { sb_puts(&s, "ud2"); goto done; }
        if (op2 == 0x31) { sb_puts(&s, "rdtsc"); goto done; }
        if (op2 == 0xa2) { sb_puts(&s, "cpuid"); goto done; }

        /* multi-byte NOP: 0F 1F /r — a ModRM with no operands rendered. */
        if (op2 == 0x1f) {
            struct modrm m; parse_modrm(&d, rexR, rexX, rexB, &m);
            sb_puts(&s, "nop");
            goto done;
        }
        /* jcc rel32 (0F 80..8F): the long-form conditional branch. */
        if (op2 >= 0x80 && op2 <= 0x8f) {
            d_i64 rel = rd_imm_sext(&d, 4);
            sb_putc(&s, 'j'); sb_puts(&s, CC[op2 & 0xf]);
            sb_putc(&s, ' ');
            out->has_target = 1; out->target = addr + d.pos + (d_u64)rel;
            sb_0xhex(&s, out->target);
            goto done;
        }
        /* setcc r/m8 (0F 90..9F). */
        if (op2 >= 0x90 && op2 <= 0x9f) {
            struct modrm m; parse_modrm(&d, rexR, rexX, rexB, &m);
            sb_puts(&s, "set"); sb_puts(&s, CC[op2 & 0xf]); sb_putc(&s, ' ');
            render_rm(&s, &m, 1, have_rex, 1, 0);
            goto done;
        }
        /* cmovcc r, r/m (0F 40..4F). */
        if (op2 >= 0x40 && op2 <= 0x4f) {
            struct modrm m; parse_modrm(&d, rexR, rexX, rexB, &m);
            sb_puts(&s, "cmov"); sb_puts(&s, CC[op2 & 0xf]); sb_putc(&s, ' ');
            sb_puts(&s, reg_name(m.reg_field, osz, have_rex)); sb_puts(&s, ", ");
            render_rm(&s, &m, osz, have_rex, 0, 0);
            goto done;
        }
        /* movzx / movsx: zero/sign extend a byte or word into a wider reg. */
        if (op2 == 0xb6 || op2 == 0xb7 || op2 == 0xbe || op2 == 0xbf) {
            int srcsz = (op2 & 1) ? 2 : 1;          /* b6/be: byte; b7/bf: word  */
            struct modrm m; parse_modrm(&d, rexR, rexX, rexB, &m);
            sb_puts(&s, (op2 & 0x08) ? "movsx " : "movzx ");
            sb_puts(&s, reg_name(m.reg_field, osz, have_rex)); sb_puts(&s, ", ");
            render_rm(&s, &m, srcsz, have_rex, m.is_reg ? 0 : srcsz, 0);
            if (m.rip_rel) { rip_used = 1; rip_disp = m.disp; }
            goto done;
        }
        /* imul r, r/m (0F AF). */
        if (op2 == 0xaf) {
            struct modrm m; parse_modrm(&d, rexR, rexX, rexB, &m);
            sb_puts(&s, "imul ");
            sb_puts(&s, reg_name(m.reg_field, osz, have_rex)); sb_puts(&s, ", ");
            render_rm(&s, &m, osz, have_rex, 0, 0);
            if (m.rip_rel) { rip_used = 1; rip_disp = m.disp; }
            goto done;
        }
        /* A broad band of SSE/SSE2 opcodes that all share the "ModRM, no
         * immediate" shape. We do not fully name every one, but decoding the
         * ModRM keeps the SWEEP in sync, which is the point. We render a best-
         * effort mnemonic for the common movement ops the compiler emits. */
        if (op2 == 0x10 || op2 == 0x11 || op2 == 0x28 || op2 == 0x29 ||
            op2 == 0x2a || op2 == 0x2c || op2 == 0x2d || op2 == 0x2e ||
            op2 == 0x2f || op2 == 0x51 || op2 == 0x54 || op2 == 0x57 ||
            op2 == 0x58 || op2 == 0x59 || op2 == 0x5a || op2 == 0x5c ||
            op2 == 0x5d || op2 == 0x5e || op2 == 0x5f || op2 == 0x6e ||
            op2 == 0x6f || op2 == 0x7e || op2 == 0x7f || op2 == 0xd6 ||
            op2 == 0xef) {
            struct modrm m; parse_modrm(&d, rexR, rexX, rexB, &m);
            const char *mn = "(sse)";
            /* Name the handful a C compiler routinely emits for float/double. */
            if (op2 == 0x10 || op2 == 0x11) mn = pF3 ? "movss" : (pF2 ? "movsd" : "movups");
            else if (op2 == 0x28 || op2 == 0x29) mn = "movaps";
            else if (op2 == 0x2a) mn = pF3 ? "cvtsi2ss" : (pF2 ? "cvtsi2sd" : "cvtpi2ps");
            else if (op2 == 0x2c) mn = pF3 ? "cvttss2si" : "cvttsd2si";
            else if (op2 == 0x2e) mn = "ucomiss";
            else if (op2 == 0x2f) mn = "comiss";
            else if (op2 == 0x57) mn = "xorps";
            else if (op2 == 0x54) mn = "andps";
            else if (op2 == 0x58) mn = pF3 ? "addss" : (pF2 ? "addsd" : "addps");
            else if (op2 == 0x59) mn = pF3 ? "mulss" : (pF2 ? "mulsd" : "mulps");
            else if (op2 == 0x5c) mn = pF3 ? "subss" : (pF2 ? "subsd" : "subps");
            else if (op2 == 0x5e) mn = pF3 ? "divss" : (pF2 ? "divsd" : "divps");
            else if (op2 == 0x5a) mn = pF3 ? "cvtss2sd" : (pF2 ? "cvtsd2ss" : "cvtps2pd");
            else if (op2 == 0x6e) mn = "movd";
            else if (op2 == 0x7e) mn = pF3 ? "movq" : "movd";
            else if (op2 == 0x6f || op2 == 0x7f) mn = pF3 ? "movdqu" : "movdqa";
            else if (op2 == 0xef) mn = "pxor";
            sb_puts(&s, mn); sb_putc(&s, ' ');
            /* reg field is an xmm register; render it, then the r/m (xmm or mem). */
            sb_puts(&s, XMM[m.reg_field & 15]); sb_puts(&s, ", ");
            render_rm(&s, &m, osz, have_rex, 0, /*xmm=*/1);
            if (m.rip_rel) { rip_used = 1; rip_disp = m.disp; }
            goto done;
        }
        /* Anything else in the two-byte map we do not model: emit (bad) and let
         * the sweep advance by the two opcode bytes already consumed. */
        sb_puts(&s, "(bad)");
        goto done;
    }

    /* ======================================================================
     * ONE-BYTE OPCODES
     * ====================================================================== */

    /* ---- the eight ALU ops sharing the 0x00..0x3F pattern ------------------ */
    if (op < 0x40 && (op & 7) < 6) {
        const char *mn = ALU[(op >> 3) & 7];
        int form = op & 7;
        if (form == 4) {                         /* al, imm8                    */
            d_i64 imm = rd_imm_sext(&d, 1);
            sb_puts(&s, mn); sb_puts(&s, " al, "); sb_0xhex(&s, (d_u64)imm);
        } else if (form == 5) {                  /* eAX/rAX, imm(z)             */
            d_i64 imm = rd_imm_sext(&d, osz == 2 ? 2 : 4);
            sb_puts(&s, mn); sb_putc(&s, ' ');
            sb_puts(&s, reg_name(0, osz, have_rex)); sb_puts(&s, ", ");
            sb_0xhex(&s, (d_u64)imm);
        } else {                                 /* the four ModRM forms        */
            int bytesz = (form == 0 || form == 2) ? 1 : osz;
            int dir_reg = (form == 2 || form == 3);   /* dst is the reg field?   */
            struct modrm m; parse_modrm(&d, rexR, rexX, rexB, &m);
            sb_puts(&s, mn); sb_putc(&s, ' ');
            if (dir_reg) {
                sb_puts(&s, reg_name(m.reg_field, bytesz, have_rex));
                sb_puts(&s, ", ");
                render_rm(&s, &m, bytesz, have_rex, 0, 0);
            } else {
                render_rm(&s, &m, bytesz, have_rex, 0, 0);
                sb_puts(&s, ", ");
                sb_puts(&s, reg_name(m.reg_field, bytesz, have_rex));
            }
            if (m.rip_rel) { rip_used = 1; rip_disp = m.disp; }
        }
        goto done;
    }

    /* ---- push/pop r64 (0x50..0x5F). Stack ops default to 64-bit operands. -- */
    if (op >= 0x50 && op <= 0x57) {
        sb_puts(&s, "push "); sb_puts(&s, R64[(op - 0x50) | (rexB ? 8 : 0)]);
        goto done;
    }
    if (op >= 0x58 && op <= 0x5f) {
        sb_puts(&s, "pop "); sb_puts(&s, R64[(op - 0x58) | (rexB ? 8 : 0)]);
        goto done;
    }

    /* ---- movsxd r64, r/m32 (0x63): sign-extend 32->64, the LP64 idiom. ----- */
    if (op == 0x63) {
        struct modrm m; parse_modrm(&d, rexR, rexX, rexB, &m);
        sb_puts(&s, "movsxd ");
        sb_puts(&s, reg_name(m.reg_field, 8, have_rex)); sb_puts(&s, ", ");
        render_rm(&s, &m, 4, have_rex, 0, 0);
        if (m.rip_rel) { rip_used = 1; rip_disp = m.disp; }
        goto done;
    }

    /* ---- push imm (0x68 imm32, 0x6A imm8) ---------------------------------- */
    if (op == 0x68 || op == 0x6a) {
        d_i64 imm = rd_imm_sext(&d, op == 0x68 ? 4 : 1);
        sb_puts(&s, "push "); sb_0xhex(&s, (d_u64)imm);
        goto done;
    }
    /* ---- imul r, r/m, imm (0x69 imm32, 0x6B imm8) -------------------------- */
    if (op == 0x69 || op == 0x6b) {
        struct modrm m; parse_modrm(&d, rexR, rexX, rexB, &m);
        d_i64 imm = rd_imm_sext(&d, op == 0x69 ? (osz == 2 ? 2 : 4) : 1);
        sb_puts(&s, "imul ");
        sb_puts(&s, reg_name(m.reg_field, osz, have_rex)); sb_puts(&s, ", ");
        render_rm(&s, &m, osz, have_rex, 0, 0); sb_puts(&s, ", ");
        sb_0xhex(&s, (d_u64)imm);
        if (m.rip_rel) { rip_used = 1; rip_disp = m.disp; }
        goto done;
    }

    /* ---- jcc rel8 (0x70..0x7F): the short conditional branch -------------- */
    if (op >= 0x70 && op <= 0x7f) {
        d_i64 rel = rd_imm_sext(&d, 1);
        sb_putc(&s, 'j'); sb_puts(&s, CC[op & 0xf]); sb_putc(&s, ' ');
        out->has_target = 1; out->target = addr + d.pos + (d_u64)rel;
        sb_0xhex(&s, out->target);
        goto done;
    }

    /* ---- group1: ALU r/m, imm (0x80 imm8, 0x81 imm(z), 0x83 imm8-sext) ----- */
    if (op == 0x80 || op == 0x81 || op == 0x83) {
        int bytesz = (op == 0x80) ? 1 : osz;
        struct modrm m; parse_modrm(&d, rexR, rexX, rexB, &m);
        int immsz = (op == 0x81) ? (osz == 2 ? 2 : 4) : 1;   /* 0x83 is imm8    */
        d_i64 imm = rd_imm_sext(&d, immsz);
        sb_puts(&s, ALU[m.ext]); sb_putc(&s, ' ');
        render_rm(&s, &m, bytesz, have_rex, m.is_reg ? 0 : bytesz, 0);
        sb_puts(&s, ", "); sb_0xhex(&s, (d_u64)imm);
        if (m.rip_rel) { rip_used = 1; rip_disp = m.disp; }
        goto done;
    }

    /* ---- test r/m, r (0x84 byte, 0x85 v) ---------------------------------- */
    if (op == 0x84 || op == 0x85) {
        int bytesz = (op == 0x84) ? 1 : osz;
        struct modrm m; parse_modrm(&d, rexR, rexX, rexB, &m);
        sb_puts(&s, "test ");
        render_rm(&s, &m, bytesz, have_rex, 0, 0); sb_puts(&s, ", ");
        sb_puts(&s, reg_name(m.reg_field, bytesz, have_rex));
        if (m.rip_rel) { rip_used = 1; rip_disp = m.disp; }
        goto done;
    }
    /* ---- xchg r/m, r (0x86 byte, 0x87 v) ---------------------------------- */
    if (op == 0x86 || op == 0x87) {
        int bytesz = (op == 0x86) ? 1 : osz;
        struct modrm m; parse_modrm(&d, rexR, rexX, rexB, &m);
        sb_puts(&s, "xchg ");
        render_rm(&s, &m, bytesz, have_rex, 0, 0); sb_puts(&s, ", ");
        sb_puts(&s, reg_name(m.reg_field, bytesz, have_rex));
        goto done;
    }

    /* ---- mov r/m,r and r,r/m (0x88..0x8B) --------------------------------- */
    if (op >= 0x88 && op <= 0x8b) {
        int bytesz = (op & 1) ? osz : 1;         /* even opcode => byte         */
        int dir_reg = (op & 2);                  /* 0x8A/0x8B: reg is dst       */
        struct modrm m; parse_modrm(&d, rexR, rexX, rexB, &m);
        sb_puts(&s, "mov ");
        if (dir_reg) {
            sb_puts(&s, reg_name(m.reg_field, bytesz, have_rex)); sb_puts(&s, ", ");
            render_rm(&s, &m, bytesz, have_rex, 0, 0);
        } else {
            render_rm(&s, &m, bytesz, have_rex, 0, 0); sb_puts(&s, ", ");
            sb_puts(&s, reg_name(m.reg_field, bytesz, have_rex));
        }
        if (m.rip_rel) { rip_used = 1; rip_disp = m.disp; }
        goto done;
    }

    /* ---- lea r, m (0x8D): compute an ADDRESS, no memory access ------------- */
    if (op == 0x8d) {
        struct modrm m; parse_modrm(&d, rexR, rexX, rexB, &m);
        sb_puts(&s, "lea ");
        sb_puts(&s, reg_name(m.reg_field, osz, have_rex)); sb_puts(&s, ", ");
        render_mem(&s, &m);                      /* lea's r/m is always memory  */
        if (m.rip_rel) { rip_used = 1; rip_disp = m.disp; }
        goto done;
    }

    /* ---- pop r/m (0x8F /0) ------------------------------------------------- */
    if (op == 0x8f) {
        struct modrm m; parse_modrm(&d, rexR, rexX, rexB, &m);
        sb_puts(&s, "pop "); render_rm(&s, &m, 8, have_rex, m.is_reg ? 0 : 8, 0);
        goto done;
    }

    /* ---- nop / pause / xchg eAX,r (0x90..0x97) ----------------------------- */
    if (op == 0x90) { sb_puts(&s, pF3 ? "pause" : "nop"); goto done; }
    if (op >= 0x91 && op <= 0x97) {
        sb_puts(&s, "xchg ");
        sb_puts(&s, reg_name(0, osz, have_rex)); sb_puts(&s, ", ");
        sb_puts(&s, reg_name((op - 0x90) | (rexB ? 8 : 0), osz, have_rex));
        goto done;
    }
    /* ---- cwde/cdqe (0x98), cdq/cqo (0x99): sign-extension idioms ----------- */
    if (op == 0x98) { sb_puts(&s, rexW ? "cdqe" : (p66 ? "cbw" : "cwde")); goto done; }
    if (op == 0x99) { sb_puts(&s, rexW ? "cqo"  : (p66 ? "cwd" : "cdq"));  goto done; }

    /* ---- string ops movs/cmps/stos/lods/scas (0xA4..0xA7, 0xAA..0xAF) ------ */
    if (op == 0xa4 || op == 0xa5 || op == 0xa6 || op == 0xa7 ||
        (op >= 0xaa && op <= 0xaf)) {
        if (pF3) sb_puts(&s, "rep ");
        else if (pF2) sb_puts(&s, "repnz ");
        int b = !(op & 1);                       /* even => byte variant        */
        const char *mn = "movs";
        if (op == 0xa6 || op == 0xa7) mn = "cmps";
        else if (op == 0xaa || op == 0xab) mn = "stos";
        else if (op == 0xac || op == 0xad) mn = "lods";
        else if (op == 0xae || op == 0xaf) mn = "scas";
        sb_puts(&s, mn); sb_putc(&s, b ? 'b' : (osz == 8 ? 'q' : 'd'));
        goto done;
    }

    /* ---- test al/eAX, imm (0xA8/0xA9) ------------------------------------- */
    if (op == 0xa8 || op == 0xa9) {
        int bytesz = (op == 0xa8) ? 1 : osz;
        d_i64 imm = rd_imm_sext(&d, bytesz == 1 ? 1 : (osz == 2 ? 2 : 4));
        sb_puts(&s, "test ");
        sb_puts(&s, reg_name(0, bytesz, have_rex)); sb_puts(&s, ", ");
        sb_0xhex(&s, (d_u64)imm);
        goto done;
    }

    /* ---- mov r, imm : 0xB0..0xB7 (byte), 0xB8..0xBF (v / movabs) ----------- */
    if (op >= 0xb0 && op <= 0xb7) {
        d_i64 imm = rd_imm_sext(&d, 1);
        sb_puts(&s, "mov "); sb_puts(&s, reg_name((op - 0xb0) | (rexB ? 8 : 0), 1, have_rex));
        sb_puts(&s, ", "); sb_0xhex(&s, (d_u64)(imm & 0xff));
        goto done;
    }
    if (op >= 0xb8 && op <= 0xbf) {
        int reg = (op - 0xb8) | (rexB ? 8 : 0);
        if (rexW) {                              /* movabs r64, imm64           */
            d_u64 imm = rd64(&d);
            sb_puts(&s, "movabs "); sb_puts(&s, R64[reg]); sb_puts(&s, ", ");
            sb_0xhex(&s, imm);
        } else {
            d_i64 imm = rd_imm_sext(&d, osz == 2 ? 2 : 4);
            sb_puts(&s, "mov "); sb_puts(&s, reg_name(reg, osz, have_rex));
            sb_puts(&s, ", "); sb_0xhex(&s, (d_u64)(d_u32)imm);
        }
        goto done;
    }

    /* ---- group2 shifts: 0xC0/C1 imm8, 0xD0/D1 by 1, 0xD2/D3 by cl ---------- */
    if (op == 0xc0 || op == 0xc1 || op == 0xd0 || op == 0xd1 ||
        op == 0xd2 || op == 0xd3) {
        int bytesz = (op & 1) ? osz : 1;
        struct modrm m; parse_modrm(&d, rexR, rexX, rexB, &m);
        sb_puts(&s, SHIFT[m.ext]); sb_putc(&s, ' ');
        render_rm(&s, &m, bytesz, have_rex, m.is_reg ? 0 : bytesz, 0);
        if (op == 0xc0 || op == 0xc1) {          /* shift count is an imm8      */
            d_i64 imm = rd_imm_sext(&d, 1);
            sb_puts(&s, ", "); sb_0xhex(&s, (d_u64)(imm & 0xff));
        } else if (op == 0xd0 || op == 0xd1) {
            sb_puts(&s, ", 1");
        } else {
            sb_puts(&s, ", cl");
        }
        if (m.rip_rel) { rip_used = 1; rip_disp = m.disp; }
        goto done;
    }

    /* ---- ret (0xC2 imm16, 0xC3), leave (0xC9), int3 (0xCC), int (0xCD) ----- */
    if (op == 0xc2) { d_u32 n = rd16(&d); sb_puts(&s, "ret "); sb_0xhex(&s, n); goto done; }
    if (op == 0xc3) { sb_puts(&s, "ret"); goto done; }
    if (op == 0xc9) { sb_puts(&s, "leave"); goto done; }
    if (op == 0xcc) { sb_puts(&s, "int3"); goto done; }
    if (op == 0xcd) { d_u32 n = rd8(&d); sb_puts(&s, "int "); sb_0xhex(&s, n); goto done; }

    /* ---- group11 mov r/m, imm (0xC6 byte, 0xC7 v) ------------------------- */
    if (op == 0xc6 || op == 0xc7) {
        int bytesz = (op == 0xc6) ? 1 : osz;
        struct modrm m; parse_modrm(&d, rexR, rexX, rexB, &m);
        int immsz = (op == 0xc6) ? 1 : (osz == 2 ? 2 : 4);
        d_i64 imm = rd_imm_sext(&d, immsz);
        sb_puts(&s, "mov ");
        render_rm(&s, &m, bytesz, have_rex, m.is_reg ? 0 : bytesz, 0);
        sb_puts(&s, ", "); sb_0xhex(&s, (d_u64)imm);
        if (m.rip_rel) { rip_used = 1; rip_disp = m.disp; }
        goto done;
    }

    /* ---- call/jmp rel32 (0xE8/0xE9), jmp rel8 (0xEB) ---------------------- */
    if (op == 0xe8 || op == 0xe9) {
        d_i64 rel = rd_imm_sext(&d, 4);
        sb_puts(&s, op == 0xe8 ? "call " : "jmp ");
        out->has_target = 1; out->target = addr + d.pos + (d_u64)rel;
        sb_0xhex(&s, out->target);
        goto done;
    }
    if (op == 0xeb) {
        d_i64 rel = rd_imm_sext(&d, 1);
        sb_puts(&s, "jmp ");
        out->has_target = 1; out->target = addr + d.pos + (d_u64)rel;
        sb_0xhex(&s, out->target);
        goto done;
    }

    /* ---- hlt/cmc (0xF4/0xF5) --------------------------------------------- */
    if (op == 0xf4) { sb_puts(&s, "hlt"); goto done; }
    if (op == 0xf5) { sb_puts(&s, "cmc"); goto done; }

    /* ---- group3 (0xF6 byte / 0xF7 v): test/not/neg/mul/imul/div/idiv ------- */
    if (op == 0xf6 || op == 0xf7) {
        int bytesz = (op == 0xf6) ? 1 : osz;
        struct modrm m; parse_modrm(&d, rexR, rexX, rexB, &m);
        static const char *const G3[8] =
            { "test","test","not","neg","mul","imul","div","idiv" };
        sb_puts(&s, G3[m.ext]); sb_putc(&s, ' ');
        render_rm(&s, &m, bytesz, have_rex, m.is_reg ? 0 : bytesz, 0);
        if (m.ext <= 1) {                        /* test takes an immediate     */
            int immsz = (op == 0xf6) ? 1 : (osz == 2 ? 2 : 4);
            d_i64 imm = rd_imm_sext(&d, immsz);
            sb_puts(&s, ", "); sb_0xhex(&s, (d_u64)imm);
        }
        if (m.rip_rel) { rip_used = 1; rip_disp = m.disp; }
        goto done;
    }

    /* ---- group4 (0xFE): inc/dec r/m8 ------------------------------------- */
    if (op == 0xfe) {
        struct modrm m; parse_modrm(&d, rexR, rexX, rexB, &m);
        sb_puts(&s, m.ext == 0 ? "inc " : (m.ext == 1 ? "dec " : "(bad) "));
        render_rm(&s, &m, 1, have_rex, m.is_reg ? 0 : 1, 0);
        goto done;
    }
    /* ---- group5 (0xFF): inc/dec/call/jmp/push r/m ------------------------- */
    if (op == 0xff) {
        struct modrm m; parse_modrm(&d, rexR, rexX, rexB, &m);
        static const char *const G5[8] =
            { "inc","dec","call","callf","jmp","jmpf","push","(bad)" };
        int sz = osz;
        if (m.ext == 2 || m.ext == 4 || m.ext == 6) sz = 8;  /* call/jmp/push=64*/
        sb_puts(&s, G5[m.ext]); sb_putc(&s, ' ');
        /* indirect call/jmp print a leading '*' in AT&T; in Intel we just show
         * the operand (a register or memory holding the target address). */
        render_rm(&s, &m, sz, have_rex, m.is_reg ? 0 : sz, 0);
        if (m.rip_rel) { rip_used = 1; rip_disp = m.disp; }
        goto done;
    }

    /* Anything not matched above: unknown 1-byte opcode. */
    sb_puts(&s, "(bad)");

done:
    /* If we ran off the end while reading operands, the sweep can't trust this
     * instruction — mark it and consume whatever remained so the caller stops. */
    if (d.trunc) {
        out->text[0] = '\0';
        struct sbuf b = { out->text, sizeof out->text, 0 };
        sb_puts(&b, "(bad)");
        out->has_target = 0;
        out->len = maxlen ? maxlen : 1;
        return out->len;
    }

    /* A RIP-relative operand's absolute target is only known now that we have
     * the full instruction length: next_ip + disp. Append it as a comment, the
     * way objdump does — this is how you spot GOT / .rodata references. */
    if (rip_used) {
        d_u64 tgt = addr + d.pos + (d_u64)rip_disp;
        sb_puts(&s, "        # ");
        sb_0xhex(&s, tgt);
    }

    out->len = d.pos ? d.pos : 1;   /* never report 0: the sweep must advance    */
    return out->len;
}
