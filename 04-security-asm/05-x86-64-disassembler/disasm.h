/* ===========================================================================
 * disasm.h — public interface + data model for a teaching x86-64 disassembler.
 * ===========================================================================
 *
 * WHY A DISASSEMBLER TEACHES YOU x86-64
 * -------------------------------------
 * x86-64 is a *variable-length* CISC encoding: an instruction is 1..15 bytes,
 * and you cannot know where instruction N+1 begins until you have fully decoded
 * instruction N. That is the whole game. This header defines the small pile of
 * structs the decoder fills in while it walks the byte stream:
 *
 *   [ legacy prefixes 0..4 ][ REX ][ opcode 1..3 ][ ModR/M ][ SIB ]
 *                                   [ displacement 0/1/2/4 ][ immediate 0/1/2/4/8 ]
 *
 * Each field below mirrors one box in that layout. Nothing here is x86-specific
 * magic — it is just careful bookkeeping over a byte array.
 *
 * SCOPE (be honest — this is a teaching CORE, not a complete decoder):
 *   We decode the *integer* subset a compiler actually emits: the eight classic
 *   ALU ops, mov/movzx/movsx/movsxd/lea, push/pop, test/cmp, the shift group,
 *   inc/dec/neg/not/mul/imul/div/idiv, jmp/jcc/call/ret/leave, setcc/cmovcc,
 *   nop, syscall/cpuid/rdtsc. We do NOT decode the SSE/AVX/x87 maps, the VEX/EVEX
 *   prefixes, far pointers, or most of the 0F 38 / 0F 3A three-byte maps. The
 *   README's coverage table says exactly what is in and what is out.
 * ===========================================================================
 */
#ifndef DISASM_H
#define DISASM_H

#include <stdint.h>   /* uint8_t .. uint64_t: exact-width types for byte work  */
#include <stdbool.h>  /* bool                                                  */
#include <stddef.h>   /* size_t                                                */

/* Output flavor. AT&T is `op src,dst` with %reg/$imm sigils and size suffixes
 * (movl); Intel is `op dst,src` with `dword ptr` hints. objdump defaults to
 * AT&T, so AT&T is what we validate against most closely. */
typedef enum { SYN_ATT = 0, SYN_INTEL = 1 } Syntax;

/* ---------------------------------------------------------------------------
 * Operand model.
 *
 * After decoding we describe each operand abstractly, then a formatter turns it
 * into text. Keeping decode and print separate is the single biggest clarity
 * win: the gnarly encoding rules live in one place, the two syntaxes in another.
 * --------------------------------------------------------------------------- */
typedef enum {
    OPK_NONE = 0, /* slot unused                                               */
    OPK_REG,      /* a general-purpose register (direct)                       */
    OPK_MEM,      /* a memory reference: seg:[base + index*scale + disp]        */
    OPK_IMM,      /* an immediate constant                                     */
    OPK_REL       /* a branch target = rip_after_insn + signed displacement    */
} OpKind;

/* A general-purpose register operand is (index 0..15, width in bits). The
 * high-byte legacy regs ah/ch/dh/bh have no REX and are flagged specially,
 * because REX-encoded byte regs (spl/bpl/sil/dil) reuse indices 4..7. */
typedef struct {
    OpKind   kind;

    /* OPK_REG ------------------------------------------------------------- */
    int      reg;        /* 0..15  (rax..r15 numbering)                       */
    int      reg_bits;   /* 8/16/32/64                                        */
    bool     reg_high8;  /* true => ah/ch/dh/bh (no-REX high byte)            */

    /* OPK_MEM ------------------------------------------------------------- */
    int      seg;        /* segment override: -1 none, else 0..5 es/cs/ss/ds/fs/gs */
    bool     has_base;   /* is there a base register?                          */
    int      base;       /* 0..15 base register index                          */
    bool     has_index;  /* is there a scaled index register?                  */
    int      index;      /* 0..15 index register index                         */
    int      scale;      /* 1, 2, 4 or 8                                        */
    bool     rip_rel;    /* RIP-relative: address = rip_after_insn + disp       */
    bool     has_disp;   /* is a displacement present?                          */
    int64_t  disp;       /* the (sign-extended) displacement value              */
    int      mem_bits;   /* operand size at this memory ref (for the AT&T suffix
                          *   / Intel `ptr` hint): 8/16/32/64                   */
    bool     indirect;   /* AT&T prints `*` before an indirect call/jmp target  */

    /* OPK_IMM ------------------------------------------------------------- */
    uint64_t imm;        /* the value, already sign/zero-extended + masked to   */
    int      imm_bits;   /*   imm_bits, so the formatter just prints it in hex  */

    /* OPK_REL ------------------------------------------------------------- */
    uint64_t target;     /* absolute branch target (for jmp/jcc/call rel8/rel32)*/
} Operand;

/* ---------------------------------------------------------------------------
 * A fully-decoded instruction. `raw`/`len` let a caller re-print the bytes; the
 * prefix/REX/opcode fields are kept so the disassembly can *explain* itself.
 * --------------------------------------------------------------------------- */
typedef struct {
    uint64_t     addr;        /* virtual address assigned to this instruction   */
    const uint8_t *raw;       /* pointer into the caller's byte buffer          */
    int          len;         /* total encoded length in bytes (1..15)          */

    /* --- legacy prefixes (group 1..4). Up to one from each group. --------- */
    bool         lock;        /* F0                                             */
    bool         rep;         /* F3 (REP/REPE) — also the SSE mandatory prefix  */
    bool         repne;       /* F2 (REPNE)                                     */
    int          seg_pfx;     /* segment override byte, or 0 if none            */
    bool         p66;         /* 66: operand-size override (=> 16-bit)          */
    bool         p67;         /* 67: address-size override (=> 32-bit addresses)*/

    /* --- REX prefix (0x40..0x4F). Present only in 64-bit-ish encodings. ---- */
    bool         rex;         /* was a REX byte seen at all? (matters for byte  */
                              /*   regs: REX present => spl/bpl/sil/dil)        */
    bool         rexW;        /* W: 1 => 64-bit operand size                    */
    bool         rexR;        /* R: extends ModR/M.reg  (high bit of reg)       */
    bool         rexX;        /* X: extends SIB.index                           */
    bool         rexB;        /* B: extends ModR/M.rm / SIB.base / opcode reg   */
    uint8_t      rex_byte;    /* the raw REX byte (for display)                 */

    /* --- opcode --------------------------------------------------------- */
    int          map;         /* 0 = primary 1-byte map, 1 = the 0F two-byte map*/
    uint8_t      opcode;      /* the final opcode byte                          */

    /* --- ModR/M + SIB (the addressing bytes) ---------------------------- */
    bool         has_modrm;   /* did this opcode consume a ModR/M byte?         */
    uint8_t      modrm;       /* the raw ModR/M byte                            */
    bool         has_sib;     /* did ModR/M imply a SIB byte?                   */
    uint8_t      sib;         /* the raw SIB byte                               */

    /* --- decoded result ------------------------------------------------- */
    const char  *mnem;        /* mnemonic ("add", "mov", ...) or NULL if bad    */
    Operand      ops[3];      /* operands in INTEL order (ops[0] = destination) */
    int          nops;        /* how many of ops[] are used                     */
    bool         valid;       /* false => undecodable/truncated                 */
} Insn;

/* ---------------------------------------------------------------------------
 * Decode exactly ONE instruction beginning at `code` (a buffer of `n` bytes).
 * `addr` is the virtual address to assign it (used to resolve rel/RIP targets).
 * On success fills *out and returns the instruction length (>=1). On a bad or
 * truncated encoding it still fills what it could, sets out->valid=false, and
 * returns the number of bytes to skip (>=1) so a linear sweep can make progress.
 * --------------------------------------------------------------------------- */
int disasm_one(const uint8_t *code, size_t n, uint64_t addr, Insn *out);

/* Format an already-decoded instruction into `buf` (size `bufsz`) in the chosen
 * syntax. Returns the number of characters written (excluding the NUL). */
int disasm_format(const Insn *in, Syntax syn, char *buf, size_t bufsz);

#endif /* DISASM_H */
