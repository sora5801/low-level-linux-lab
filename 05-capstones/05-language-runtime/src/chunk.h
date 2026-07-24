/* ===========================================================================
 * chunk.h — a "chunk" of compiled bytecode: the instruction stream a function
 *           runs, plus its constant pool and a source-line side table.
 * ===========================================================================
 *
 * The compiler emits into a Chunk; the VM executes one. A chunk is three
 * parallel resources:
 *   - code[]      : the opcode/operand byte stream (what the VM's `ip` walks).
 *   - lines[]     : lines[i] is the source line that produced code[i], used ONLY
 *                   to print "line N" in a runtime error. Kept as a separate
 *                   array so the hot code[] stream stays dense (one byte/op).
 *   - constants   : a pool of literal Values; instructions reference a constant
 *                   by a 1-byte index rather than embedding a 16-byte Value.
 *
 * All three are VM-internal growable arrays (libc realloc), not GC objects.
 */
#ifndef LUMEN_CHUNK_H
#define LUMEN_CHUNK_H

#include "common.h"
#include "value.h"

/* ---- The instruction set ---------------------------------------------------
 * A stack machine: operands come from and results go to the value stack, so
 * most ops take no register arguments. Bytecodes that DO take an inline operand
 * note its width in the comment. Keep this enum in lockstep with:
 *   (1) the dispatch table + handlers in vm.c, and
 *   (2) disassembleInstruction() in vm.c (used by the debug builds).
 */
typedef enum {
    OP_CONSTANT,        /* push constants[readByte()]                          */
    OP_NIL,             /* push nil                                            */
    OP_TRUE,            /* push true                                           */
    OP_FALSE,           /* push false                                          */
    OP_POP,             /* discard top of stack                                */
    OP_GET_LOCAL,       /* push slots[readByte()]        (local read)          */
    OP_SET_LOCAL,       /* slots[readByte()] = peek(0)   (local write, no pop) */
    OP_GET_GLOBAL,      /* push globals[constants[readByte()]]                 */
    OP_DEFINE_GLOBAL,   /* globals[name] = pop()                               */
    OP_SET_GLOBAL,      /* globals[name] = peek(0)  (assignment is an expr)    */
    OP_EQUAL,           /* b=pop; a=pop; push(a == b)                          */
    OP_GREATER,         /* numeric a > b                                       */
    OP_LESS,            /* numeric a < b                                       */
    OP_ADD,             /* number+number OR string+string (concat)            */
    OP_SUBTRACT,        /* number - number                                     */
    OP_MULTIPLY,        /* number * number                                     */
    OP_DIVIDE,          /* number / number                                     */
    OP_NOT,             /* push(isFalsey(pop))                                 */
    OP_NEGATE,          /* push(-pop)  (number only)                           */
    OP_PRINT,           /* print(pop) + newline                               */
    OP_JUMP,            /* ip += readShort()            (unconditional)        */
    OP_JUMP_IF_FALSE,   /* if isFalsey(peek(0)) ip += readShort()              */
    OP_LOOP,            /* ip -= readShort()            (backward jump)        */
    OP_CALL,            /* call peek(argc) with readByte() args                */
    OP_RETURN           /* return top of stack from the current function       */
} OpCode;

typedef struct {
    int        count;       /* bytes of code used                             */
    int        capacity;    /* bytes allocated                                */
    uint8_t   *code;        /* the instruction stream                         */
    int       *lines;       /* lines[i] == source line of code[i]             */
    ValueArray constants;   /* literal pool, indexed by OP_CONSTANT's operand */
} Chunk;

void initChunk(Chunk *chunk);
void freeChunk(Chunk *chunk);
void writeChunk(Chunk *chunk, uint8_t byte, int line);  /* append one byte     */
int  addConstant(Chunk *chunk, Value value);            /* -> index of the new */
                                                        /*    constant         */
#endif /* LUMEN_CHUNK_H */
