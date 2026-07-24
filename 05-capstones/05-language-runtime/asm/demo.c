/* ===========================================================================
 * asm/demo.c — the bytecode DISPATCH CORE, extracted and made self-contained.
 * ===========================================================================
 *
 * This is the single most instructive pure-logic routine in the project: the
 * computed-goto interpreter loop from src/vm.c, shrunk to a tiny stack machine
 * with its own opcodes and NO system headers (so clang can cross-compile it to
 * Linux asm on any host — see the project README "Assembly notes").
 *
 * `vm_run()` executes a fixed bytecode program that computes  sum(1..10) = 55
 * on a stack machine with a handful of local "slots", using the SAME
 * computed-goto dispatch technique as the real VM: each handler ends with its own
 * `goto *table[code[ip++]]`, giving the CPU one indirect-branch site per opcode
 * (better prediction than a shared switch). The generated assembly for this file
 * is what asm/demo.annotated.s explains instruction by instruction.
 *
 * The program's exit status is the result (55), so on Linux:
 *     clang demo.c -o demo && ./demo ; echo $?     ->     55
 * ===========================================================================
 */

/* Own fixed-width types — no <stdint.h>. On the x86-64 SysV LP64 model `long
 * long` is 64-bit and `unsigned char` is one byte, which is all we rely on. */
typedef unsigned char u8;
typedef long long     i64;

/* The instruction set of this miniature VM. Values are the byte encodings used
 * in the program below. */
enum {
    OP_CONST,   /* operand: imm8         push (i64)imm8                          */
    OP_LOAD,    /* operand: slot#        push slot[slot#]                        */
    OP_STORE,   /* operand: slot#        slot[slot#] = pop()                     */
    OP_ADD,     /*                       b=pop; a=pop; push(a + b)               */
    OP_SUB,     /*                       b=pop; a=pop; push(a - b)               */
    OP_LE,      /*                       b=pop; a=pop; push(a <= b ? 1 : 0)      */
    OP_JMPF,    /* operand: rel8(signed) if pop()==0: ip += rel                  */
    OP_JMP,     /* operand: rel8(signed) ip += rel                              */
    OP_RET      /*                       return pop()                           */
};

/* Execute `code` from ip=0 and return the value left by OP_RET. This is the
 * routine whose assembly we study. */
i64 vm_run(const u8 *code)
{
    i64 stack[32];          /* operand stack                                     */
    i64 slot[8];            /* local variables (like a call frame's slots)       */
    int sp = 0;             /* stack pointer (index of next free slot)           */
    int ip = 0;             /* instruction pointer (index into code[])           */

    for (int k = 0; k < 8; k++) slot[k] = 0;   /* clear locals (no memset here)  */

#if defined(__GNUC__)
    /* Address-of-label dispatch table, indexed by opcode. `&&label` is the GNU
     * extension; each entry is a code address we can `goto *`. */
    static const void *table[] = {
        &&L_CONST, &&L_LOAD, &&L_STORE, &&L_ADD, &&L_SUB,
        &&L_LE, &&L_JMPF, &&L_JMP, &&L_RET,
    };
    /* Fetch the next opcode and jump straight to its handler. Replicated at the
     * end of every handler, so there is one indirect branch per opcode. */
#  define NEXT() goto *table[code[ip++]]

    NEXT();                                     /* prime the loop                 */
    L_CONST: { stack[sp++] = (i64)code[ip++];                       NEXT(); }
    L_LOAD:  { stack[sp++] = slot[code[ip++]];                      NEXT(); }
    L_STORE: { slot[code[ip++]] = stack[--sp];                     NEXT(); }
    L_ADD:   { i64 b = stack[--sp]; i64 a = stack[--sp]; stack[sp++] = a + b; NEXT(); }
    L_SUB:   { i64 b = stack[--sp]; i64 a = stack[--sp]; stack[sp++] = a - b; NEXT(); }
    L_LE:    { i64 b = stack[--sp]; i64 a = stack[--sp]; stack[sp++] = (a <= b); NEXT(); }
    L_JMPF:  { int rel = (signed char)code[ip++]; if (stack[--sp] == 0) ip += rel; NEXT(); }
    L_JMP:   { int rel = (signed char)code[ip++]; ip += rel;        NEXT(); }
    L_RET:   { return stack[--sp]; }
#else
    /* Portable fallback: the same handlers behind a decode-and-switch loop. */
    for (;;) {
        u8 op = code[ip++];
        switch (op) {
        case OP_CONST: stack[sp++] = (i64)code[ip++];                     break;
        case OP_LOAD:  stack[sp++] = slot[code[ip++]];                    break;
        case OP_STORE: slot[code[ip++]] = stack[--sp];                   break;
        case OP_ADD: { i64 b = stack[--sp], a = stack[--sp]; stack[sp++] = a + b; break; }
        case OP_SUB: { i64 b = stack[--sp], a = stack[--sp]; stack[sp++] = a - b; break; }
        case OP_LE:  { i64 b = stack[--sp], a = stack[--sp]; stack[sp++] = (a <= b); break; }
        case OP_JMPF: { int rel = (signed char)code[ip++]; if (stack[--sp] == 0) ip += rel; break; }
        case OP_JMP:  { int rel = (signed char)code[ip++]; ip += rel;    break; }
        case OP_RET:  return stack[--sp];
        }
    }
#endif
    return 0;   /* not reached: OP_RET always exits the loop */
}

/* The program: acc = 0; i = 1; while (i <= 10) { acc += i; i += 1; } return acc.
 * slot[0] = acc, slot[1] = i. Byte offsets are annotated so the two jump
 * displacements (OP_JMPF forward +16, OP_JMP backward -23) can be verified. */
int main(void)
{
    static const u8 prog[] = {
        /* 0 */  OP_CONST, 0,   OP_STORE, 0,     /* acc = 0                       */
        /* 4 */  OP_CONST, 1,   OP_STORE, 1,     /* i   = 1                       */
        /* 8 (loop): */
        /* 8 */  OP_LOAD, 1,    OP_CONST, 10,  OP_LE,   /* push (i <= 10)         */
        /* 13*/  OP_JMPF, 16,                    /* if false, jump to end (+16)   */
        /* 15*/  OP_LOAD, 0,    OP_LOAD, 1,  OP_ADD,  OP_STORE, 0,  /* acc += i   */
        /* 22*/  OP_LOAD, 1,    OP_CONST, 1,  OP_ADD,  OP_STORE, 1,  /* i += 1    */
        /* 29*/  OP_JMP, (u8)-23,               /* jump back to loop (-23)        */
        /* 31 (end): */
        /* 31*/  OP_LOAD, 0,    OP_RET           /* return acc  (== 55)           */
    };
    return (int)vm_run(prog);   /* exit status = 55 */
}
