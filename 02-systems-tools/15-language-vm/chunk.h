/* ===========================================================================
 * chunk.h — the bytecode container and the instruction set (the ISA).
 * ===========================================================================
 *
 * A "chunk" is one function's worth of compiled code: a flat byte array of
 * instructions, a pool of constants those instructions reference by index, and
 * a parallel array mapping each byte back to its source line (for error
 * messages). This is the boundary between the front end (scanner+compiler,
 * which WRITE a chunk) and the back end (the VM, which READS one).
 *
 * INSTRUCTION ENCODING
 * --------------------
 * Every instruction is one opcode byte, optionally followed by operand bytes:
 *   - most ops take no operand (OP_ADD, OP_POP, OP_RETURN, ...);
 *   - OP_CONSTANT / the GET/SET/DEFINE ops take ONE byte: an index into the
 *     constant pool or a local-variable stack slot (so max 256 constants per
 *     chunk and 256 locals per function — fine for a teaching VM; clox adds a
 *     _LONG 24-bit form to lift the limit);
 *   - the jump ops take TWO bytes: a 16-bit unsigned BIG-ENDIAN offset. Jumps
 *     are relative so a chunk is position-independent.
 * A stack VM needs no register operands: operands live implicitly on the value
 * stack, which is why the encoding is so dense.
 * ===========================================================================
 */
#ifndef CLOXI_CHUNK_H
#define CLOXI_CHUNK_H

#include "common.h"
#include "value.h"

/* The opcode set. Order is irrelevant to correctness but stable so the
 * disassembler and the computed-goto dispatch table can index by it. */
typedef enum {
    OP_CONSTANT,       /* [idx] : push constants[idx]                          */
    OP_NIL,            /* push nil                                             */
    OP_TRUE,           /* push true                                            */
    OP_FALSE,          /* push false                                           */
    OP_POP,            /* discard the top of stack                             */

    OP_GET_LOCAL,      /* [slot] : push frame slot `slot` (a local variable)   */
    OP_SET_LOCAL,      /* [slot] : store TOS into slot (leaves TOS in place)   */
    OP_GET_GLOBAL,     /* [idx]  : push global named constants[idx]            */
    OP_DEFINE_GLOBAL,  /* [idx]  : pop TOS, bind it to global name constants[idx] */
    OP_SET_GLOBAL,     /* [idx]  : assign TOS to existing global (error if new) */

    OP_EQUAL,          /* pop b,a ; push (a == b)                              */
    OP_GREATER,        /* pop b,a ; push (a >  b)   (ints only)                */
    OP_LESS,           /* pop b,a ; push (a <  b)   (ints only)                */

    OP_ADD,            /* pop b,a ; push a+b  (ints) OR string concatenation   */
    OP_SUBTRACT,       /* pop b,a ; push a-b  (ints)                           */
    OP_MULTIPLY,       /* pop b,a ; push a*b  (ints)                           */
    OP_DIVIDE,         /* pop b,a ; push a/b  (ints; traps on divide-by-zero)  */
    OP_NEGATE,         /* replace TOS with -TOS (int)                          */
    OP_NOT,            /* replace TOS with its boolean negation                */

    OP_PRINT,          /* pop TOS and print it followed by a newline           */

    OP_JUMP,           /* [hi][lo] : ip += offset             (unconditional)  */
    OP_JUMP_IF_FALSE,  /* [hi][lo] : if TOS is falsey, ip += offset (peeks TOS) */
    OP_LOOP,           /* [hi][lo] : ip -= offset             (backward jump)  */

    OP_CALL,           /* [argc] : call the callable `argc` slots below TOS    */
    OP_RETURN,         /* return TOS from the current function frame           */
} OpCode;

/* ---------------------------------------------------------------------------
 * Chunk — three parallel growable arrays.
 *   code[i]      : the i-th byte of bytecode
 *   lines[i]     : the source line that byte came from (same index space as
 *                  code — deliberately simple; clox shows a run-length encoding
 *                  that compresses this ~10x since lines repeat)
 *   constants    : the constant pool this chunk's OP_CONSTANT indices point at
 * --------------------------------------------------------------------------- */
typedef struct {
    int        count;      /* bytes of code in use                            */
    int        capacity;   /* bytes allocated                                 */
    uint8_t   *code;       /* the bytecode, owned                             */
    int       *lines;      /* per-byte source line, owned; parallel to code   */
    ValueArray constants;  /* literals referenced by index                    */
} Chunk;

void initChunk(Chunk *chunk);
void freeChunk(Chunk *chunk);

/* Append one byte (`byte`) of bytecode, recording the source `line`. */
void writeChunk(Chunk *chunk, uint8_t byte, int line);

/* Append `value` to the constant pool and return its index. The value is
 * pushed onto the VM stack across the append so an in-progress GC (which the
 * append's reallocation can trigger) treats it as a root — see chunk.c. */
int addConstant(Chunk *chunk, Value value);

#endif /* CLOXI_CHUNK_H */
