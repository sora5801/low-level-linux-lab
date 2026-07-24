/* ===========================================================================
 * debug.h — the disassembler: turn a chunk of bytecode back into text.
 * ===========================================================================
 *
 * Reading raw opcode bytes is miserable; this module prints them in the
 * familiar "offset  LINE  OP_NAME  operand" form. It is the single best tool
 * for understanding what the compiler emitted and (with DEBUG_TRACE_EXECUTION)
 * for watching the VM step through it. It reads the chunk only — no side
 * effects — so it is always safe to call.
 * ===========================================================================
 */
#ifndef CLOXI_DEBUG_H
#define CLOXI_DEBUG_H

#include "chunk.h"

/* Disassemble a whole chunk, prefixing with `name` (e.g. the function name). */
void disassembleChunk(Chunk *chunk, const char *name);

/* Disassemble the single instruction at byte `offset`; return the offset of the
 * NEXT instruction (so a caller can walk the chunk). The length is data-driven:
 * a 1-byte op returns offset+1, a constant op offset+2, a jump op offset+3. */
int disassembleInstruction(Chunk *chunk, int offset);

#endif /* CLOXI_DEBUG_H */
