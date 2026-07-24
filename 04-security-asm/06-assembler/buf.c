/* ===========================================================================
 * buf.c — a growable byte buffer with explicit little-endian writers.
 * ===========================================================================
 *
 * Two things in this file matter for correctness:
 *
 *   1. GROWTH / OWNERSHIP. buf_init() starts empty (NULL data); the first write
 *      mallocs, later writes realloc by DOUBLING (amortised O(1) append). The
 *      owner must call buf_free(). A realloc can move the block, so callers
 *      must never cache `b->data` across a write — they index by offset.
 *
 *   2. LITTLE-ENDIAN, BYTE-AT-A-TIME. x86-64 is little-endian and so are all
 *      ELF fields on this target. We NEVER memcpy a C struct into the file,
 *      because struct padding and host endianness would leak in. Instead every
 *      multi-byte value is decomposed into bytes here, low byte first. This is
 *      the single source of truth for "how a u32 becomes four bytes on disk".
 * ===========================================================================
 */
#include "asm.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void buf_init(Buf *b) { b->data = NULL; b->len = 0; b->cap = 0; }

void buf_free(Buf *b) { free(b->data); b->data = NULL; b->len = b->cap = 0; }

/* Ensure room for `extra` more bytes; grow geometrically so N appends cost
 * O(N) total, not O(N^2). On OOM we abort — this is a build tool, and there is
 * no useful recovery from "cannot hold the object we are assembling". */
static void buf_reserve(Buf *b, size_t extra)
{
    if (b->len + extra <= b->cap) return;
    size_t ncap = b->cap ? b->cap : 64;
    while (ncap < b->len + extra) ncap *= 2;
    uint8_t *nd = (uint8_t *)realloc(b->data, ncap);
    if (!nd) { fprintf(stderr, "masm: out of memory\n"); exit(1); }
    b->data = nd;
    b->cap  = ncap;
}

void buf_u8(Buf *b, uint8_t v)
{
    buf_reserve(b, 1);
    b->data[b->len++] = v;
}

/* Each of these emits low byte first (little-endian). Writing them out by hand
 * (rather than shifting in a loop) keeps the byte order unmistakable to a
 * reader — this is exactly the layout that ends up in the .o file. */
void buf_u16(Buf *b, uint16_t v)
{
    buf_u8(b, (uint8_t)(v      & 0xff));
    buf_u8(b, (uint8_t)(v >> 8 & 0xff));
}

void buf_u32(Buf *b, uint32_t v)
{
    buf_u8(b, (uint8_t)(v       & 0xff));
    buf_u8(b, (uint8_t)(v >> 8  & 0xff));
    buf_u8(b, (uint8_t)(v >> 16 & 0xff));
    buf_u8(b, (uint8_t)(v >> 24 & 0xff));
}

void buf_u64(Buf *b, uint64_t v)
{
    buf_u32(b, (uint32_t)(v         & 0xffffffffu));   /* low 32 bits first   */
    buf_u32(b, (uint32_t)((v >> 32) & 0xffffffffu));   /* then high 32 bits   */
}

void buf_bytes(Buf *b, const void *p, size_t n)
{
    buf_reserve(b, n);
    memcpy(b->data + b->len, p, n);
    b->len += n;
}

/* Pad the buffer up to the next multiple of `align` using `pad` bytes. Section
 * data in an ELF file is aligned (e.g. .symtab to 8) so the linker can map it
 * without unaligned access; we honour that when laying out the file image. */
void buf_align(Buf *b, size_t align, uint8_t pad)
{
    while (b->len % align != 0) buf_u8(b, pad);
}
