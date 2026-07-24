/* ===========================================================================
 * chunk.c — build and tear down a bytecode chunk.
 * ===========================================================================
 */
#include "chunk.h"
#include "memory.h"
#include "vm.h"     /* for push/pop — see addConstant's GC note below          */

void initChunk(Chunk *chunk)
{
    chunk->count    = 0;
    chunk->capacity = 0;
    chunk->code     = NULL;
    chunk->lines    = NULL;
    initValueArray(&chunk->constants);
}

void freeChunk(Chunk *chunk)
{
    FREE_ARRAY(uint8_t, chunk->code,  chunk->capacity);
    FREE_ARRAY(int,     chunk->lines, chunk->capacity);
    freeValueArray(&chunk->constants);
    initChunk(chunk);   /* reset to the valid empty state                      */
}

void writeChunk(Chunk *chunk, uint8_t byte, int line)
{
    /* code[] and lines[] grow in lockstep and share capacity, so one bounds
     * check guards both. Keeping them parallel means lines[i] is always the
     * source line of the byte code[i]. */
    if (chunk->capacity < chunk->count + 1) {
        int oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->code  = GROW_ARRAY(uint8_t, chunk->code,  oldCapacity, chunk->capacity);
        chunk->lines = GROW_ARRAY(int,     chunk->lines, oldCapacity, chunk->capacity);
    }
    chunk->code[chunk->count]  = byte;
    chunk->lines[chunk->count] = line;
    chunk->count++;
}

int addConstant(Chunk *chunk, Value value)
{
    /* GC HAZARD: writeValueArray may realloc the constants array, and that
     * reallocation can trigger a garbage collection. If `value` is a freshly
     * created object reachable ONLY through this argument, the collector would
     * free it mid-append. Pushing it onto the VM stack first makes it a root
     * for the duration; we pop it right after it is safely stored in the array
     * (which is itself reachable from the chunk, hence from a live function). */
    push(value);
    writeValueArray(&chunk->constants, value);
    pop();
    return chunk->constants.count - 1;   /* the index the caller will encode   */
}
