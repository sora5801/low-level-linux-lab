/* ===========================================================================
 * disasm.h — public interface of the linear-sweep x86-64 disassembler backend.
 * ===========================================================================
 *
 * This header is deliberately FREE OF SYSTEM HEADERS so that disasm.c can be
 * compiled as a standalone translation unit (and its teaching assembly can be
 * generated the same way asm/demo.c's is). It defines its own fixed-width types.
 *
 * WHAT "LINEAR SWEEP" MEANS
 * -------------------------
 * There are two classic ways to disassemble machine code:
 *
 *   - RECURSIVE DESCENT follows control flow: start at an entry point, decode,
 *     and when you hit a branch, queue its target(s). It never decodes data,
 *     but it can miss code only reached indirectly.
 *   - LINEAR SWEEP just decodes byte after byte from the start of .text to the
 *     end, assuming it is ALL instructions. Simple, complete, and what
 *     `objdump -d` does by default — but it will happily misdecode inline data
 *     or jump tables as if they were instructions.
 *
 * We implement linear sweep. Its one hard correctness requirement is getting
 * each instruction's LENGTH exactly right: if we miscount by even one byte, the
 * decoder desynchronizes and every following instruction is garbage. So the
 * bulk of disasm.c is about counting bytes (prefixes, ModRM, SIB, displacement,
 * immediate), and rendering a readable mnemonic is secondary.
 *
 * SCOPE (be honest — see the README): we decode the common subset of x86-64
 * that a C compiler actually emits into .text (mov/lea/add/sub/cmp/test/xor/
 * push/pop/call/jmp/jcc/ret/leave/nop/endbr64/syscall and the ALU immediate
 * groups), in Intel syntax. Anything outside that subset is length-decoded when
 * its encoding class is recognizable and rendered as "(bad)" otherwise, so the
 * sweep stays in sync as often as possible.
 * ===========================================================================
 */
#ifndef ELFTK_DISASM_H
#define ELFTK_DISASM_H

/* Our own primitives — same widths on every x86-64 target, no <stdint.h>. */
typedef unsigned char      d_u8;
typedef unsigned int       d_u32;
typedef unsigned long long d_u64;
typedef long long          d_i64;

/* Maximum length of any single x86-64 instruction, by architectural rule. The
 * CPU faults (#GP) on anything longer, so no valid instruction exceeds this and
 * we never need to buffer more than 15 bytes to decode one. */
#define X86_MAX_INSN_LEN 15

/* One decoded instruction. `text` is a rendered, human-readable form; the
 * numeric fields let the caller (objdump.c) annotate branch targets with symbol
 * names without re-parsing the string. */
struct insn {
    d_u64    addr;        /* virtual address where this instruction starts       */
    unsigned len;         /* total encoded length in bytes (always >= 1)         */
    int      has_target;  /* 1 if this is a relative branch/call with a target   */
    d_u64    target;      /* absolute destination = addr + len + rel (if above)  */
    char     text[64];    /* "mnemonic op, op" — NUL-terminated, Intel syntax    */
};

/* ---------------------------------------------------------------------------
 * x86_decode — decode ONE instruction.
 *
 *   code    : pointer to the first byte of the instruction.
 *   maxlen  : how many bytes are safely readable at `code` (we never read past
 *             this — important, the last instruction may butt against .text's
 *             end or the mapped file's end).
 *   addr    : the virtual address of code[0] (so relative targets resolve to
 *             absolute addresses for display).
 *   out     : filled in on return.
 *
 * Returns the instruction length (1..15). On an encoding we do not recognize it
 * returns a best-effort length with out->text = "(bad)"; on truncation (the
 * instruction needs more bytes than maxlen) it returns the bytes remaining and
 * marks it "(bad)". The return value is ALWAYS >= 1 so a sweep loop advances.
 * --------------------------------------------------------------------------- */
unsigned x86_decode(const d_u8 *code, unsigned maxlen, d_u64 addr, struct insn *out);

#endif /* ELFTK_DISASM_H */
