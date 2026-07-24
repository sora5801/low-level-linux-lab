/* ===========================================================================
 * asm/demo.c — the HEART of the disassembler, extracted to stand alone.
 * ===========================================================================
 *
 * This is the ModR/M + SIB + displacement decoder — the single hardest and most
 * instructive routine in the whole project — with NO system headers and its own
 * integer types, so `clang -S` turns it into clean, readable x86-64 assembly you
 * can study (see demo.annotated.s). It is a faithful copy of decode_rm() in
 * ../disasm.c, minus the printing/plumbing, so the generated asm is pure logic.
 *
 * WHAT IT COMPUTES
 * ----------------
 * Given the bytes right after an opcode, plus the REX.R/X/B extension bits, it
 * answers the four questions that let a decoder find the *next* instruction:
 *   1. Is the r/m a register (mod==11) or a memory reference?
 *   2. Is there a SIB byte? (only when it is memory and rm==100)
 *   3. Is there a displacement, and is it 1 or 4 bytes?
 *   4. Which base/index registers and scale describe the effective address?
 *
 * THE THREE "STOLEN" ENCODINGS — the whole reason this is subtle:
 *   A. mod!=11 && rm==100          -> a SIB byte follows (RSP can't be a base).
 *   B. mod==00 && rm==101          -> RIP-relative disp32, no base register.
 *   C. in the SIB byte:
 *        base==101 && mod==00      -> no base; a disp32 stands alone.
 *        index==100 && REX.X==0    -> no index (RSP can't be a scaled index;
 *                                     but REX.X==1 turns that slot into r12,
 *                                     which IS a legal index).
 * ===========================================================================
 */

/* Our own fixed-width types — no <stdint.h>, so this file is freestanding. On
 * the x86-64 LP64 ABI these widths are exact. */
typedef unsigned char      u8;
typedef unsigned int       u32;
typedef unsigned long long u64;
typedef signed   long long i64;

/* The decoded addressing mode. A plain struct returned by value; at -O1 the
 * compiler keeps most of it in registers, which the annotated asm shows. */
typedef struct {
    int is_reg;      /* 1 => mod==11: r/m is a register, there is no memory ref */
    int reg;         /* ModR/M.reg field, REX.R-extended (0..15) — the "other"
                      *   operand; carried through so a caller has it in hand.   */
    int rm_reg;      /* when is_reg: the r/m register index (0..15)             */

    int has_base;    /* memory form: is there a base register?                  */
    int base;        /* base register index (0..15)                            */
    int has_index;   /* is there a scaled index register?                       */
    int index;       /* index register index (0..15)                           */
    int scale;       /* 1, 2, 4, or 8                                           */
    int rip_rel;     /* RIP-relative? (effective addr = next_rip + disp)        */

    int has_disp;    /* is a displacement present?                              */
    int disp_size;   /* 0, 1, or 4 bytes                                        */
    i64 disp;        /* the sign-extended displacement value                    */

    int length;      /* bytes consumed here (ModR/M + SIB? + disp), or -1 if the
                      *   input was truncated                                    */
} amode;

/* Read `size` little-endian bytes at p and sign-extend to 64 bits. `size` is 1
 * or 4 here. Little-endian: byte 0 is the least-significant. */
static i64 read_disp(const u8 *p, int size) {
    u64 v = 0;
    for (int i = 0; i < size; i++)
        v |= (u64)p[i] << (8 * i);                 /* assemble low..high        */
    /* sign-extend: if the top bit of the `size`-byte value is set, fill above. */
    u64 sign = (u64)1 << (size * 8 - 1);
    u64 lo   = (sign << 1) - 1;                     /* mask of the value bits    */
    if (v & sign) v |= ~lo;                         /* propagate the sign        */
    return (i64)v;
}

/* Decode the ModR/M (and any SIB/displacement) beginning at p[0], with `n` bytes
 * available. rexR/rexX/rexB are the 0/1 REX extension bits. */
amode decode_modrm(const u8 *p, int n, int rexR, int rexX, int rexB) {
    amode a;                                        /* result, filled field by field */
    a.is_reg = 0; a.reg = 0; a.rm_reg = 0;
    a.has_base = 0; a.base = 0; a.has_index = 0; a.index = 0; a.scale = 1;
    a.rip_rel = 0; a.has_disp = 0; a.disp_size = 0; a.disp = 0; a.length = 0;

    if (n < 1) { a.length = -1; return a; }         /* need at least the ModR/M   */

    u8  modrm = p[0];
    int mod   = (modrm >> 6) & 3;                   /* bits 7-6: addressing mode  */
    int reg   = (modrm >> 3) & 7;                   /* bits 5-3: reg / sub-opcode */
    int rm    =  modrm       & 7;                   /* bits 2-0: r/m              */
    int pos   = 1;                                  /* we've consumed 1 byte      */

    a.reg = reg | (rexR << 3);                      /* REX.R is reg's high bit    */

    /* --- mod==11: register-direct. r/m names a register; no SIB, no disp. --- */
    if (mod == 3) {
        a.is_reg = 1;
        a.rm_reg = rm | (rexB << 3);                /* REX.B is rm's high bit     */
        a.length = pos;
        return a;
    }

    /* --- memory forms ---------------------------------------------------- */
    int need_sib = (rm == 4);                       /* rule A: rm==100 => SIB     */

    if (need_sib) {
        if (pos >= n) { a.length = -1; return a; }
        u8  sib   = p[pos++];
        int ss    = (sib >> 6) & 3;                 /* scale exponent (1<<ss)     */
        int idx3  = (sib >> 3) & 7;                 /* index field (pre-REX)      */
        int base3 =  sib       & 7;                 /* base  field (pre-REX)      */

        /* rule C (index): field 100 with REX.X==0 means "no index register".
         * With REX.X==1 the same slot is r12, a valid index. */
        if (idx3 == 4 && rexX == 0) {
            a.has_index = 0;
        } else {
            a.has_index = 1;
            a.index = idx3 | (rexX << 3);
            a.scale = 1 << ss;                      /* 1, 2, 4, or 8              */
        }

        /* rule C (base): field 101 with mod==00 means "no base"; a disp32 stands
         * alone. (REX.B does not rescue it — r13 as a base needs mod!=00.) */
        if (base3 == 5 && mod == 0) {
            a.has_base = 0;
            if (pos + 4 > n) { a.length = -1; return a; }
            a.disp = read_disp(p + pos, 4);
            a.disp_size = 4; a.has_disp = 1; pos += 4;
            a.length = pos;
            return a;
        }
        a.has_base = 1;
        a.base = base3 | (rexB << 3);
        /* fall through: the displacement (if any) is chosen by `mod` below */
    } else if (mod == 0 && rm == 5) {
        /* rule B: RIP-relative disp32; no base/index register at all. */
        a.rip_rel = 1;
        if (pos + 4 > n) { a.length = -1; return a; }
        a.disp = read_disp(p + pos, 4);
        a.disp_size = 4; a.has_disp = 1; pos += 4;
        a.length = pos;
        return a;
    } else {
        /* plain [reg]: rm itself is the base register (REX.B-extended). */
        a.has_base = 1;
        a.base = rm | (rexB << 3);
    }

    /* Displacement size follows mod for the base / SIB-with-base cases. */
    if (mod == 1) {                                 /* disp8, sign-extended       */
        if (pos + 1 > n) { a.length = -1; return a; }
        a.disp = read_disp(p + pos, 1);
        a.disp_size = 1; a.has_disp = 1; pos += 1;
    } else if (mod == 2) {                          /* disp32                     */
        if (pos + 4 > n) { a.length = -1; return a; }
        a.disp = read_disp(p + pos, 4);
        a.disp_size = 4; a.has_disp = 1; pos += 4;
    }
    /* mod==0 with a base register: no displacement (implicit 0). */

    a.length = pos;
    return a;
}
