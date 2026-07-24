/* ===========================================================================
 * asm.h — shared declarations for `masm`, a tiny x86-64 assembler.
 * ===========================================================================
 *
 * masm parses a SUBSET of AT&T-syntax x86-64 assembly and emits a RELOCATABLE
 * ELF64 object file (.o) that the system linker (`ld`) can consume. The whole
 * point is to make three normally-hidden things concrete:
 *
 *   1. INSTRUCTION ENCODING — how `mov %rsi,%rdi` becomes the bytes 48 89 f7:
 *      the REX prefix, the opcode, and the ModR/M / SIB / displacement bytes.
 *   2. THE SYMBOL TABLE — how labels become addresses, and how names the file
 *      does not define (e.g. `call printf`) become UNDEFINED symbols the linker
 *      must later resolve.
 *   3. RELOCATIONS — the "fill this in at link time" notes the assembler leaves
 *      whenever it cannot know a final address yet (a cross-section data
 *      reference, an external call).
 *
 * WHY AT&T SYNTAX? The rest of this lab (and objdump's default output) is AT&T,
 * so masm matches it for a clean round-trip with the disassembler in ../05.
 * AT&T means: `op  src, dst`   (source first, destination last),
 *   %reg  register    $imm  immediate    disp(%base)  memory    sym(%rip) RIP-rel.
 *
 * DEFENSE / SAFETY NOTE: an assembler is dual-use — it is the same tool whether
 * you are building a hardened service or a piece of shellcode. Everything here
 * is for assembling code on YOUR OWN machine for programs YOU wrote. The
 * instructive security lesson lives in the encoder: seeing exactly which bytes
 * an instruction becomes is precisely the skill a defender uses to READ a
 * disassembly, spot a smuggled `syscall` (0F 05), or understand why a byte
 * pattern in memory is or is not a valid instruction. See the README.
 *
 * This header is deliberately heavy on comments about the ELF on-disk structs,
 * because getting a byte wrong there produces a file `ld` silently rejects.
 * ===========================================================================
 */
#ifndef MASM_H
#define MASM_H

#include <stdint.h>
#include <stddef.h>

/* --- fixed capacities -------------------------------------------------------
 * This is teaching code: we favour flat, bounded arrays with explicit checks
 * over dynamic growth, so the data structures stay easy to read. Every limit
 * is checked at use; overflowing one is a hard error, never silent corruption.
 */
#define MAXNAME      64      /* max symbol / mnemonic length (incl. NUL)       */
#define MAX_STMTS  8192      /* max parsed statements in one source file       */
#define MAX_SYMS   2048      /* max distinct symbols                           */
#define MAX_RELOCS 2048      /* max relocation entries                         */
#define MAX_DARGS   256      /* max operands to one .byte/.quad directive      */
#define MAX_OPS       2      /* every instruction here has <= 2 operands       */

/* Two output sections. A real assembler has many (.rodata, .bss, ...); the
 * teaching core keeps exactly the two the ELF psABI examples always show. */
enum { SEC_TEXT = 0, SEC_DATA = 1, NUM_SECS = 2 };

/* ===========================================================================
 * A growable byte buffer. Used for each output section and for building the
 * ELF file image. All multi-byte writes are LITTLE-ENDIAN because x86-64 is a
 * little-endian architecture and ELF fields on this target are LE too. We write
 * bytes explicitly (never memcpy of a struct) so the on-disk layout is visible
 * and correct regardless of the host's endianness or struct padding.
 * ===========================================================================
 */
typedef struct {
    uint8_t *data;
    size_t   len;
    size_t   cap;
} Buf;

void     buf_init(Buf *b);
void     buf_free(Buf *b);
void     buf_u8 (Buf *b, uint8_t  v);              /* append 1 byte           */
void     buf_u16(Buf *b, uint16_t v);              /* append 2 bytes, LE      */
void     buf_u32(Buf *b, uint32_t v);              /* append 4 bytes, LE      */
void     buf_u64(Buf *b, uint64_t v);              /* append 8 bytes, LE      */
void     buf_bytes(Buf *b, const void *p, size_t n);
void     buf_align(Buf *b, size_t align, uint8_t pad); /* pad to a boundary   */

/* ===========================================================================
 * Operands. AT&T syntax has four operand shapes we care about.
 * ===========================================================================
 */
typedef enum {
    OP_NONE = 0,
    OP_REG,     /* %rax ...            -> reg                                  */
    OP_IMM,     /* $123, $0x10         -> imm                                  */
    OP_MEM,     /* disp(%base) / sym(%rip)                                     */
    OP_SYM      /* a bare label: jmp/call/lea target                          */
} OpKind;

typedef struct {
    OpKind  kind;
    int     reg;             /* OP_REG: reg number 0-15; OP_MEM: base reg 0-15 */
    int64_t imm;             /* OP_IMM: value; OP_MEM: displacement            */
    int     rip;             /* OP_MEM: 1 => base is %rip (PC-relative)        */
    int     have_sym;        /* OP_MEM: displacement is a symbol name          */
    char    sym[MAXNAME];    /* OP_SYM name, or OP_MEM sym(%rip) name          */
} Operand;
/* NOTE ON int64_t: immediates and .quad values must hold a full 64-bit word.
 * We deliberately do NOT use `long`, because `long` is only 32 bits under the
 * Windows LLP64 model — the assembler tool itself may be built on Windows, and
 * a 32-bit `long` would silently truncate `.quad 0x1122334455667788`. */

/* ===========================================================================
 * A parsed statement. A single source line can yield a label AND an
 * instruction (`loop: mov ...`), so we split lines into >=1 of these.
 * ===========================================================================
 */
typedef enum { STK_LABEL, STK_INSN, STK_DIR } StmtKind;

typedef struct {
    StmtKind kind;
    int      line;                 /* 1-based source line, for error messages */

    /* STK_LABEL */
    char     label[MAXNAME];

    /* STK_INSN */
    char     mnem[16];
    Operand  ops[MAX_OPS];
    int      nops;

    /* STK_DIR */
    char     dir[16];              /* ".text" ".data" ".globl" ".byte" ".quad"*/
    char     dsym[MAXNAME];        /* .globl NAME                             */
    int64_t  dargs[MAX_DARGS];     /* .byte/.quad numeric operands            */
    int      ndargs;

    /* filled by pass 1 (layout) */
    int      section;              /* SEC_TEXT / SEC_DATA this stmt lands in  */
    uint64_t offset;               /* byte offset within that section         */
    int      size;                 /* encoded size in bytes                   */
} Stmt;

/* ===========================================================================
 * Symbols. Every label defines one; every referenced-but-undefined name
 * becomes one too (an external the linker must resolve).
 * ===========================================================================
 */
typedef struct {
    char     name[MAXNAME];
    int      defined;   /* 1 if a label in THIS file defines it               */
    int      is_global; /* 1 if `.globl name` was seen (or it is undefined)   */
    int      section;   /* SEC_TEXT/SEC_DATA if defined; -1 if undefined      */
    uint64_t value;     /* offset within `section` (the label's address)      */
    int      symidx;    /* index assigned in the ELF .symtab (filled at emit) */
} Symbol;

/* ===========================================================================
 * Relocations we could not resolve at assemble time. Each says: "at byte
 * `offset` in .text there is a 4- or 8-byte field; the linker must patch it
 * using symbol `sym` and the given relocation `type` and `addend`."
 *
 * For x86-64 (which uses RELA / explicit-addend relocations) the linker
 * computes, for a PC-relative type:   value = S + A - P
 *   S = final address of the symbol
 *   A = addend  (we store -4: the rel32 field sits at the END of the
 *                instruction, so the CPU measures from field+4 = next insn)
 *   P = final address of the field itself
 * ===========================================================================
 */
typedef struct {
    uint64_t offset;         /* offset of the field within .text              */
    char     sym[MAXNAME];   /* symbol the relocation targets                 */
    uint32_t type;           /* R_X86_64_* (see below)                        */
    int64_t  addend;         /* r_addend (usually -4)                         */
} Reloc;

/* x86-64 relocation types we emit (from the psABI). */
#define R_X86_64_64     1    /* absolute 64-bit: S + A   (for .quad symbol)   */
#define R_X86_64_PC32   2    /* PC-relative 32-bit: S + A - P  (lea data)     */
#define R_X86_64_PLT32  4    /* like PC32 but may route via PLT (call func)   */

/* ===========================================================================
 * The assembler context: the two section buffers, the symbol and relocation
 * tables, and which section we are currently emitting into.
 * ===========================================================================
 */
typedef struct {
    Buf    sec[NUM_SECS];        /* .text and .data byte images               */
    int    cur;                  /* current section (SEC_TEXT/SEC_DATA)       */

    Symbol syms[MAX_SYMS];
    int    nsyms;

    /* names named in `.globl`; matched against defined labels at emit time   */
    char   globals[MAX_SYMS][MAXNAME];
    int    nglobals;

    Reloc  relocs[MAX_RELOCS];
    int    nrelocs;

    const char *srcname;         /* for error messages                        */
    int    errors;               /* count; nonzero => do not write output     */
} Assembler;

/* --- symbol-table helpers (assemble.c) ------------------------------------ */
Symbol *sym_find(Assembler *A, const char *name);
Symbol *sym_define(Assembler *A, const char *name, int section, uint64_t value, int line);
Symbol *sym_ref_undef(Assembler *A, const char *name); /* find or add external */
void    add_global(Assembler *A, const char *name);
void    add_reloc(Assembler *A, uint64_t off, const char *sym, uint32_t type, int64_t add);

/* --- lexer / parser (lex.c) ----------------------------------------------- */
/* Parse `src` (whole file text) into `out[0..*count]`. Returns 0 on success,
 * nonzero on a syntax error (message already printed). */
int parse_source(const char *srcname, char *src, Stmt *out, int max, int *count);

/* --- encoder + two-pass driver (encode.c / assemble.c) -------------------- */
/* Encode one statement. `emit`==0 => only compute & return its byte size
 * (pass 1); `emit`==1 => actually append bytes to the current section and
 * resolve/record symbol references (pass 2). The SAME code path is used for
 * both, which guarantees pass-1 sizes always match pass-2 output. */
int  encode_stmt(Assembler *A, Stmt *s, int emit);
int  assemble(Assembler *A, Stmt *stmts, int n);   /* runs pass 1 then pass 2 */

/* --- ELF object writer (elf.c) -------------------------------------------- */
int  write_elf(Assembler *A, const char *outpath);

/* --- small shared utils --------------------------------------------------- */
int  reg_number(const char *name);   /* "%"-stripped name -> 0..15, or -1     */

#endif /* MASM_H */
