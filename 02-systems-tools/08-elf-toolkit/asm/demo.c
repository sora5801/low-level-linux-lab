/* ===========================================================================
 * asm/demo.c — SELF-CONTAINED extraction of the ELF toolkit's two hottest
 *              pure-logic routines:
 *                (1) sym_by_addr  — the address->symbol BINARY SEARCH that
 *                                    powers addr2line / objdump annotation
 *                                    (objdump.c::sym_lookup), and
 *                (2) x86_insn_len — the OPCODE-LENGTH DECODER that keeps the
 *                                    linear sweep in sync (the length half of
 *                                    disasm.c::x86_decode).
 * ===========================================================================
 *
 * NO #includes, OWN types: a bare cross-compiler turns this into x86-64 System V
 * assembly with no libc or kernel headers in sight, so every instruction the two
 * routines compile to can be annotated (see demo.annotated.s).
 *
 * WHY THESE TWO
 * -------------
 * They are the archetypes of the two skills an ELF tool needs:
 *
 *   - sym_by_addr is a textbook binary search. clang keeps it as a tight branchy
 *     loop (a compare + two conditional jumps per step) even at -O2 — a clean
 *     look at how the "find rightmost element <= key" idiom lowers, and why the
 *     midpoint is computed as lo + (hi-lo)/2 (an `sar` after an unsigned-shift
 *     round-toward-zero fixup you can spot in the asm).
 *   - x86_insn_len is pure control flow over bytes: it shows how the ModRM/SIB/
 *     displacement rules that decide an instruction's length become a cascade of
 *     compares and adds — and it is where the optimizer's tricks show up: the
 *     legacy-prefix set becomes a 64-bit BITMAP tested with `btq`, the nibble
 *     checks become `sete`, and the "return 0 on truncation" guard becomes a
 *     branchless CONDITIONAL MOVE (`cmov`). Getting the length right is what
 *     stops a linear-sweep disassembler from desynchronizing.
 * ===========================================================================
 */

/* --- our own fixed-width types (no <stdint.h>) ----------------------------- */
typedef unsigned char      u8;
typedef unsigned int       u32;
typedef unsigned long long u64;

/* A minimal symbol record: just the address and size the search needs. In the
 * real tool (objdump.c) this also carries a name pointer, but the search only
 * ever compares addresses, so the extraction keeps only those. */
struct sym {
    u64 addr;   /* virtual address of the symbol                                */
    u64 size;   /* byte size (0 if the toolchain didn't record it)              */
};

/* ---------------------------------------------------------------------------
 * sym_by_addr — return the index of the symbol whose range covers `addr`,
 * i.e. the GREATEST entry with tab[i].addr <= addr, or -1 if addr is below all
 * of them. `tab` must be sorted ascending by addr (objdump.c sorts it once).
 *
 * This is the exact algorithm in objdump.c::sym_lookup. It is a "find the
 * rightmost element <= key" search — a small but easy-to-get-wrong variant of
 * binary search (note the `ans = mid; lo = mid+1` instead of returning on an
 * exact hit).
 * --------------------------------------------------------------------------- */
int sym_by_addr(const struct sym *tab, int n, u64 addr)
{
    int lo = 0, hi = n - 1, ans = -1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;   /* midpoint without lo+hi overflow       */
        if (tab[mid].addr <= addr) {    /* mid is a candidate; a bigger one may  */
            ans = mid;                  /*   still qualify, so search the right   */
            lo = mid + 1;               /*   half.                                */
        } else {
            hi = mid - 1;               /* mid too big; discard it and its right  */
        }
    }
    return ans;
}

/* ---------------------------------------------------------------------------
 * modrm_bytes — how many bytes a ModRM addressing form occupies, counting the
 * ModRM byte itself plus any SIB byte and displacement (but NOT the immediate,
 * which the opcode decides separately).
 *
 * `p` points at the ModRM byte; `avail` is how many bytes remain readable.
 * `*ok` is cleared if we would need more bytes than are available. This is the
 * single most bug-prone rule in x86 length decoding; see the comments.
 * --------------------------------------------------------------------------- */
unsigned modrm_bytes(const u8 *p, unsigned avail, int *ok)
{
    if (avail < 1) { *ok = 0; return 0; }
    u8 m = p[0];
    int mod = (m >> 6) & 3;             /* 3 = register-direct, 0/1/2 = memory   */
    int rm  = m & 7;                    /* which base form / whether a SIB follows*/
    unsigned n = 1;                     /* the ModRM byte itself                 */

    if (mod == 3)                       /* operand is a register: nothing more   */
        return 1;

    if (rm == 4) {                      /* rm==100b => a SIB byte follows        */
        if (avail < 2) { *ok = 0; return n; }
        u8 sib = p[1];
        n = 2;                          /* ModRM + SIB                           */
        if (mod == 0 && (sib & 7) == 5) /* SIB base==101b with mod==0 => no base */
            n += 4;                     /*   register, a disp32 takes its place   */
    } else if (mod == 0 && rm == 5) {   /* mod==0, rm==101b => RIP-relative      */
        n += 4;                         /*   disp32 relative to the next IP       */
    }

    if (mod == 1)                       /* mod==01b => 8-bit displacement        */
        n += 1;
    else if (mod == 2)                  /* mod==10b => 32-bit displacement       */
        n += 4;

    return n;
}

/* Immediate size (bytes) for a "z" immediate: 2 with a 0x66 operand-size
 * override, otherwise 4. (In 64-bit mode these are sign-extended; the LENGTH is
 * still 2 or 4.) */
static unsigned imm_z(int p66) { return p66 ? 2u : 4u; }

/* ---------------------------------------------------------------------------
 * x86_insn_len — total encoded length of ONE x86-64 instruction at `p`, for the
 * common subset a C compiler emits. Returns 0 if it would read past `avail`
 * (truncation) or hits an opcode the length model does not cover.
 *
 * This is the length-only skeleton of disasm.c::x86_decode: prefixes, REX, the
 * opcode, then ModRM/SIB/disp (via modrm_bytes) and the immediate.
 * --------------------------------------------------------------------------- */
unsigned x86_insn_len(const u8 *p, unsigned avail)
{
    unsigned i = 0;                     /* cursor / running length               */
    int p66 = 0;                        /* saw a 0x66 operand-size prefix?       */
    int rexW = 0;                       /* saw REX.W (64-bit operand)?           */

    /* 1) Legacy prefixes, any number, any order. */
    for (;;) {
        if (i >= avail) return 0;
        u8 b = p[i];
        if (b == 0x66) { p66 = 1; i++; }
        else if (b == 0x67 || b == 0xf0 || b == 0xf2 || b == 0xf3 ||
                 b == 0x2e || b == 0x36 || b == 0x3e ||
                 b == 0x26 || b == 0x64 || b == 0x65) { i++; }
        else break;
    }

    /* 2) Optional REX byte (0x40..0x4F), the last prefix before the opcode. */
    if (i < avail && p[i] >= 0x40 && p[i] <= 0x4f) {
        rexW = (p[i] >> 3) & 1;
        i++;
    }

    if (i >= avail) return 0;
    u8 op = p[i++];                     /* 3) the opcode byte                    */

    /* 4) Two-byte opcodes (0x0F xx). */
    if (op == 0x0f) {
        if (i >= avail) return 0;
        u8 op2 = p[i++];
        if (op2 >= 0x80 && op2 <= 0x8f) /* jcc rel32                             */
            return (i + 4 <= avail) ? i + 4 : 0;
        /* endbr64 etc. (0F 1E /modrm) and the vast majority of two-byte ops take
         * a ModRM and no immediate; a handful of imm8 forms are omitted here for
         * brevity (this is the teaching subset — see disasm.c for the rest). */
        int ok = 1;
        unsigned mb = modrm_bytes(p + i, avail - i, &ok);
        if (!ok) return 0;
        return i + mb;
    }

    /* 5) One-byte opcodes. Group them by how they consume trailing bytes. */

    /* 5a) No operands: push/pop r (0x50-0x5F), ret/leave/nop/int3/hlt/cwde/cdq. */
    if ((op >= 0x50 && op <= 0x5f) ||
        op == 0xc3 || op == 0xc9 || op == 0x90 || op == 0xcc ||
        op == 0xf4 || op == 0x98 || op == 0x99)
        return i;

    /* 5b) The eight ALU ops (0x00-0x3F, low 3 bits < 6). */
    if (op < 0x40 && (op & 7) < 6) {
        int form = op & 7;
        if (form == 4) return (i + 1 <= avail) ? i + 1 : 0;          /* al,imm8  */
        if (form == 5) { unsigned z = imm_z(p66);                    /* eAX,immz */
                         return (i + z <= avail) ? i + z : 0; }
        int ok = 1;                                                  /* modrm    */
        unsigned mb = modrm_bytes(p + i, avail - i, &ok);
        return ok ? i + mb : 0;
    }

    /* 5c) ModRM, no immediate: mov(0x88-0x8B), lea(0x8D), test(0x84/85),
     *     group2 by cl/1 (0xD0-0xD3), group5 (0xFF), movsxd (0x63). */
    if ((op >= 0x88 && op <= 0x8b) || op == 0x8d || op == 0x84 || op == 0x85 ||
        op == 0x63 || op == 0xff || (op >= 0xd0 && op <= 0xd3) ||
        op == 0x86 || op == 0x87 || op == 0x8f) {
        int ok = 1;
        unsigned mb = modrm_bytes(p + i, avail - i, &ok);
        return ok ? i + mb : 0;
    }

    /* 5d) ModRM + imm8: group1 0x80 and 0x83, group2 shifts 0xC0/0xC1,
     *     group11 byte mov 0xC6, imul r,r/m,imm8 0x6B. */
    if (op == 0x80 || op == 0x83 || op == 0xc0 || op == 0xc1 ||
        op == 0xc6 || op == 0x6b) {
        int ok = 1;
        unsigned mb = modrm_bytes(p + i, avail - i, &ok);
        if (!ok) return 0;
        return (i + mb + 1 <= avail) ? i + mb + 1 : 0;
    }

    /* 5e) ModRM + imm(z): group1 0x81, group11 word/dword mov 0xC7,
     *     imul r,r/m,immz 0x69. */
    if (op == 0x81 || op == 0xc7 || op == 0x69) {
        int ok = 1;
        unsigned mb = modrm_bytes(p + i, avail - i, &ok);
        if (!ok) return 0;
        unsigned z = imm_z(p66);
        return (i + mb + z <= avail) ? i + mb + z : 0;
    }

    /* 5f) group3 0xF6/0xF7: ModRM, plus an immediate only for the /0 and /1
     *     (test) sub-opcodes. We must peek the ModRM.reg field to know. */
    if (op == 0xf6 || op == 0xf7) {
        int ok = 1;
        unsigned mb = modrm_bytes(p + i, avail - i, &ok);
        if (!ok) return 0;
        int reg = (p[i] >> 3) & 7;              /* the /digit                    */
        unsigned imm = 0;
        if (reg <= 1) imm = (op == 0xf6) ? 1u : imm_z(p66);
        return (i + mb + imm <= avail) ? i + mb + imm : 0;
    }

    /* 5g) Immediate-only relative branches and moves. */
    if (op >= 0x70 && op <= 0x7f)               /* jcc rel8                       */
        return (i + 1 <= avail) ? i + 1 : 0;
    if (op == 0xeb)                              /* jmp rel8                       */
        return (i + 1 <= avail) ? i + 1 : 0;
    if (op == 0xe8 || op == 0xe9)                /* call/jmp rel32                 */
        return (i + 4 <= avail) ? i + 4 : 0;
    if (op >= 0xb0 && op <= 0xb7)                /* mov r8, imm8                   */
        return (i + 1 <= avail) ? i + 1 : 0;
    if (op >= 0xb8 && op <= 0xbf) {              /* mov r, imm(v): 8 if REX.W      */
        unsigned z = rexW ? 8u : imm_z(p66);
        return (i + z <= avail) ? i + z : 0;
    }
    if (op == 0x68)                              /* push imm32                     */
        return (i + 4 <= avail) ? i + 4 : 0;
    if (op == 0x6a)                              /* push imm8                      */
        return (i + 1 <= avail) ? i + 1 : 0;

    return 0;                                    /* opcode outside the subset      */
}

/* ---------------------------------------------------------------------------
 * demo_run — a complete, self-contained exercise so the unit links and nothing
 * is optimized away when compiled alone. It:
 *   (a) binary-searches a 4-entry sorted symbol table for address 0x1090,
 *       expecting index 2 (the symbol at 0x1080) and offset 0x10, and
 *   (b) length-decodes the four-instruction prologue
 *          55                push rbp        (1 byte)
 *          48 89 e5          mov  rbp, rsp   (3 bytes)
 *          48 83 ec 10       sub  rsp, 0x10  (4 bytes)
 *          c3                ret             (1 byte)
 *       summing to 9 bytes.
 * It returns offset(0x10=16) + total(9) == 25, a value you can verify by eye.
 * No libc, no I/O.
 * --------------------------------------------------------------------------- */
int demo_run(void)
{
    static const struct sym tab[4] = {
        { 0x1000, 0x40 }, { 0x1040, 0x40 }, { 0x1080, 0x40 }, { 0x1100, 0x20 },
    };
    int idx = sym_by_addr(tab, 4, 0x1090ULL);   /* expect 2                       */
    u64 off = (idx >= 0) ? (0x1090ULL - tab[idx].addr) : 0; /* expect 0x10        */

    static const u8 code[9] = { 0x55, 0x48, 0x89, 0xe5,
                                0x48, 0x83, 0xec, 0x10, 0xc3 };
    unsigned total = 0, p = 0;
    while (p < sizeof code) {
        unsigned len = x86_insn_len(code + p, (unsigned)(sizeof code - p));
        if (len == 0) break;                    /* stop on a byte we can't decode */
        total += len;
        p += len;
    }                                           /* expect total == 9              */

    return (int)(off + total);                  /* expect 16 + 9 == 25            */
}
