/* ===========================================================================
 * asm/demo.c — SELF-CONTAINED extraction of the VM's most instructive routine:
 *              the COMPUTED-GOTO instruction dispatch loop.
 * ===========================================================================
 *
 * This file exists purely to generate teaching assembly. It has NO #includes
 * and declares its own types, so a bare cross-compiler turns it into x86-64
 * SysV assembly with no libc or system headers present. `vm_run` below is the
 * SAME dispatch technique as the real interpreter in ../vm.c — a tiny stack
 * machine — reproduced in isolation so every instruction it compiles to can be
 * annotated (see demo.annotated.s).
 *
 * WHAT TO WATCH FOR IN THE GENERATED ASSEMBLY
 * -------------------------------------------
 * The whole point of computed goto is the shape of the DISPATCH. Look for:
 *   1. a jump TABLE in .rodata: 6 eight-byte code addresses, one per opcode;
 *   2. the recurring three-instruction tail at the end of EACH handler:
 *          movzbl (%rREG), %eREG      # fetch the next opcode byte, zero-extend
 *          inc    %rREG               # advance the instruction pointer
 *          jmp    *(%rTABLE,%rREG,8)  # INDIRECT JUMP through the table
 *      That `jmp *table(,idx,8)` is the entire mechanism: because it is
 *      duplicated at every handler, each site gets its OWN branch-predictor
 *      history, which is why threaded dispatch beats a single shared `switch`.
 *   3. the stack machine itself — `push` is a store then a pointer bump, `pop`
 *      a pointer decrement then a load — becoming plain mov/add/sub on registers.
 *
 * Compare against a `switch`-based build (define VM_USE_SWITCH) to see clang
 * emit ONE shared indirect jump instead of many. With -fno-jump-tables (which
 * this repo passes) the switch even degrades to a chain of compares.
 * ===========================================================================
 */

/* --- our own fixed-width types (no <stdint.h>) ----------------------------- */
typedef unsigned char      u8;    /* one bytecode byte                          */
typedef long long          i64;   /* a 64-bit stack value (LP64: long long==64) */

/* The mini-ISA. PUSH carries a one-byte signed immediate operand; the rest are
 * zero-operand stack ops. HALT ends the program and yields the stack top. The
 * numeric values matter: they are the INDICES into the dispatch table. */
enum {
    OP_PUSH = 0,   /* [imm8] : push the sign-extended immediate                 */
    OP_ADD  = 1,   /* pop b,a ; push a+b                                         */
    OP_SUB  = 2,   /* pop b,a ; push a-b                                         */
    OP_MUL  = 3,   /* pop b,a ; push a*b                                         */
    OP_NEG  = 4,   /* pop a   ; push -a                                          */
    OP_HALT = 5,   /* stop; result is the current stack top                     */
};

/* ---------------------------------------------------------------------------
 * vm_run — execute `code` on an internal value stack, return the final top.
 *
 * SysV ABI:  code arrives in %rdi (arg0); the i64 return leaves in %rax.
 * The two hot pointers are `sp` (next-free stack slot) and `ip` (next bytecode
 * byte). Both want to live in registers across the whole loop; watch the
 * optimizer pin them there.
 * --------------------------------------------------------------------------- */
i64 vm_run(const u8 *code)
{
    i64  stack[64];          /* the value stack (lives in this frame)           */
    i64 *sp = stack;         /* points at the NEXT FREE slot (push = *sp++ = v) */
    const u8 *ip = code;     /* the interpreter's program counter               */

#if !defined(VM_USE_SWITCH) && (defined(__GNUC__) || defined(__clang__))
    /* ---- COMPUTED-GOTO ("threaded") DISPATCH ---------------------------- */
    /* The jump table: opcode -> handler address. `&&label` is the GNU "label as
     * value" extension; a static array of them is explicitly permitted and is
     * emitted into .rodata as a table of 8-byte code pointers. Designated
     * initializers keep the rows aligned to the enum. */
    static const void *const table[] = {
        [OP_PUSH] = &&do_push,
        [OP_ADD]  = &&do_add,
        [OP_SUB]  = &&do_sub,
        [OP_MUL]  = &&do_mul,
        [OP_NEG]  = &&do_neg,
        [OP_HALT] = &&do_halt,
    };

    /* The dispatch: fetch the opcode byte, advance ip, jump to its handler.
     * Emitted at the end of EVERY handler (see each NEXT() below) — that
     * replication is what threaded dispatch is all about. */
#define NEXT() goto *table[*ip++]

    NEXT();   /* kick off the loop by dispatching the first opcode              */

do_push:
    /* Read the immediate operand byte, sign-extend it (so small negatives work)
     * and push. Two ip advances happen: one for the opcode (in NEXT) and one
     * here for the operand. */
    *sp++ = (i64)(signed char)*ip++;
    NEXT();

do_add:
    /* pop b, pop a, push a+b. `*--sp` is pop; the temporaries a,b become
     * register moves. */
    { i64 b = *--sp; i64 a = *--sp; *sp++ = a + b; }
    NEXT();

do_sub:
    { i64 b = *--sp; i64 a = *--sp; *sp++ = a - b; }
    NEXT();

do_mul:
    { i64 b = *--sp; i64 a = *--sp; *sp++ = a * b; }
    NEXT();

do_neg:
    { i64 a = *--sp; *sp++ = -a; }
    NEXT();

do_halt:
    return sp[-1];   /* the result is whatever is on top of the stack           */

#undef NEXT

#else
    /* ---- PORTABLE SWITCH DISPATCH (the fallback) ------------------------ */
    /* Identical semantics. clang lowers this to ONE shared indirect branch (or,
     * with -fno-jump-tables, a compare chain) — the very thing computed goto
     * spreads out. Build with -DVM_USE_SWITCH to generate this variant. */
    for (;;) {
        switch (*ip++) {
            case OP_PUSH: *sp++ = (i64)(signed char)*ip++; break;
            case OP_ADD:  { i64 b = *--sp; i64 a = *--sp; *sp++ = a + b; } break;
            case OP_SUB:  { i64 b = *--sp; i64 a = *--sp; *sp++ = a - b; } break;
            case OP_MUL:  { i64 b = *--sp; i64 a = *--sp; *sp++ = a * b; } break;
            case OP_NEG:  { i64 a = *--sp;                *sp++ = -a;    } break;
            case OP_HALT: return sp[-1];
        }
    }
#endif
}

/* ---------------------------------------------------------------------------
 * demo_run — a self-contained driver so the translation unit is complete and
 * vm_run cannot be optimized away when this file is compiled on its own.
 *
 * It hand-assembles the program for  (2 + 3) * 4 - 1 :
 *     PUSH 2, PUSH 3, ADD, PUSH 4, MUL, PUSH 1, SUB, HALT
 * runs it, and returns the result, which must be 19.
 * --------------------------------------------------------------------------- */
int demo_run(void)
{
    static const u8 program[] = {
        OP_PUSH, 2,
        OP_PUSH, 3,
        OP_ADD,           /* 2 + 3 = 5    */
        OP_PUSH, 4,
        OP_MUL,           /* 5 * 4 = 20   */
        OP_PUSH, 1,
        OP_SUB,           /* 20 - 1 = 19  */
        OP_HALT,
    };
    return (int)vm_run(program);   /* expect 19                                  */
}
