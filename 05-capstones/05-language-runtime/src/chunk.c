/* ===========================================================================
 * chunk.c — build and free a Chunk of bytecode.
 * =========================================================================== */
#include "chunk.h"

void initChunk(Chunk *chunk)
{
    chunk->count    = 0;
    chunk->capacity = 0;
    chunk->code     = NULL;
    chunk->lines    = NULL;
    initValueArray(&chunk->constants);
}

/* Append one byte of code plus its source line. `code` and `lines` grow together
 * so index i always describes the same instruction byte in both. */
void writeChunk(Chunk *chunk, uint8_t byte, int line)
{
    if (chunk->capacity < chunk->count + 1) {
        int old = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(old);
        chunk->code  = GROW_ARRAY(uint8_t, chunk->code,  old, chunk->capacity);
        chunk->lines = GROW_ARRAY(int,     chunk->lines, old, chunk->capacity);
    }
    chunk->code[chunk->count]  = byte;
    chunk->lines[chunk->count] = line;
    chunk->count++;
}

void freeChunk(Chunk *chunk)
{
    FREE_ARRAY(uint8_t, chunk->code,  chunk->capacity);
    FREE_ARRAY(int,     chunk->lines, chunk->capacity);
    freeValueArray(&chunk->constants);
    initChunk(chunk);
}

/* Intern a literal into the pool and return its index. The compiler emits that
 * 1-byte index after OP_CONSTANT, so a chunk may hold at most 256 constants (a
 * limit the compiler checks). */
int addConstant(Chunk *chunk, Value value)
{
    writeValueArray(&chunk->constants, value);
    return chunk->constants.count - 1;
}
