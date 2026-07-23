/* ===========================================================================
 * demo.c — the allocator's pure arithmetic core, extracted for annotated asm.
 * ===========================================================================
 *
 * This file is DELIBERATELY self-contained: it includes no headers and declares
 * its own types, so `clang --target=x86_64-pc-linux-gnu -S` turns it into clean
 * Linux/SysV assembly with nothing from libc in the way. The real allocator
 * (../mymalloc.c) cannot be compiled to asm standalone because it needs the Linux
 * headers for sbrk/mmap/pthread — so we lift out the part that is 100% register-
 * and-pointer math, which is also the most instructive part to read as assembly:
 *
 *   1. align_up      — the "add (a-1), mask" alignment trick (no divide).
 *   2. bin_index     — size-class index from a size using count-leading-zeros
 *                      (floor(log2)). This is the bit trick the README calls out;
 *                      watch it become a single `bsr`/`lzcnt` in the asm.
 *   3. round_total   — request -> whole-block size (header + payload + footer).
 *   4. split_block   — carve an allocation out of a bigger free block.
 *   5. coalesce_next / coalesce_prev — O(1) boundary-tag merging with a neighbour,
 *                      reading the previous block's FOOTER at (header - 8).
 *
 * These mirror the same-named routines in mymalloc.c exactly, minus the syscalls.
 * ===========================================================================
 */

/* Our own fixed-width types (LP64: long and pointers are 64-bit on x86-64). */
typedef unsigned long      u64;
typedef unsigned int       u32;
typedef unsigned long      usize;   /* stand-in for size_t, no <stddef.h> here */

#define ALIGNMENT   16u
#define HDR_SIZE    16u
#define FTR_SIZE    8u
#define MIN_BLOCK   48u
#define NUM_BINS    16
#define FLAG_ALLOC  0x1u
#define SIZE_MASK   (~(usize)0xF)

/* Same 16-byte header as the real allocator. */
typedef struct block {
    usize size;    /* total block size incl header+footer; low 4 bits = flags */
    u32   magic;
    u32   req;
} block;

/* ---- 1. alignment rounding -------------------------------------------------
 * Round n up to the next multiple of a (a is a power of two). Adding (a-1) then
 * masking off the low bits floors to a multiple after having bumped past it. */
usize align_up(usize n, usize a) {
    return (n + (a - 1)) & ~(a - 1);
}

/* ---- 2. size-class index (the count-leading-zeros trick) -------------------
 * floor(log2(total)) is the index of the highest set bit. __builtin_clzll counts
 * the leading zero bits of a 64-bit value, so 63 - clz(total) is that index. The
 * compiler lowers this to a single bit-scan-reverse instruction. */
int bin_index(usize total) {
    if (total < MIN_BLOCK) total = MIN_BLOCK;          /* clamp: clz(0) is UB */
    unsigned floor_log2 = 63u - (unsigned)__builtin_clzll((u64)total);
    int idx = (int)floor_log2 - 5;                     /* total>=32 => >=5 */
    if (idx < 0) idx = 0;
    if (idx >= NUM_BINS) idx = NUM_BINS - 1;
    return idx;
}

/* ---- 3. request -> whole block size ---------------------------------------- */
usize round_total(usize n) {
    usize t = HDR_SIZE + n + FTR_SIZE;
    t = align_up(t, ALIGNMENT);
    return t < MIN_BLOCK ? MIN_BLOCK : t;
}

static usize blk_size(const block *b) { return b->size & SIZE_MASK; }

/* ---- 4. split a free block -------------------------------------------------
 * Mark `total` bytes of `b` allocated. If the leftover is itself a usable block
 * (>= MIN_BLOCK), split it off and return its header; otherwise hand out the
 * whole block and return 0. Pure pointer arithmetic on the boundary tags. */
block *split_block(block *b, usize total) {
    usize whole = blk_size(b);
    usize extra = whole - total;
    if (extra < MIN_BLOCK) {
        b->size = whole | FLAG_ALLOC;              /* no split: take the whole block */
        return (block *)0;
    }
    b->size = total | FLAG_ALLOC;                  /* shrink b to exactly `total`     */
    block *rem = (block *)((char *)b + total);     /* remainder starts right after b  */
    rem->size = extra;                             /* free remainder (flags = 0)      */
    return rem;
}

/* ---- 5a. coalesce with the physically-next block --------------------------- */
block *coalesce_next(block *b, const void *heap_end) {
    block *nx = (block *)((char *)b + blk_size(b));
    if ((const void *)nx >= heap_end) return b;    /* at the top: nothing above    */
    if (nx->size & FLAG_ALLOC) return b;           /* neighbour busy: cannot merge */
    b->size = blk_size(b) + blk_size(nx);          /* absorb it, stay free         */
    return b;
}

/* ---- 5b. coalesce with the physically-previous block (footer tag) ----------
 * The key boundary-tag move: the previous block's footer sits at (b - 8) and
 * mirrors its size+flags, so we learn the neighbour's size WITHOUT any search. */
block *coalesce_prev(block *b, const void *heap_lo) {
    if ((const void *)b <= heap_lo) return b;      /* at the bottom: nothing below */
    usize *prev_ftr = (usize *)((char *)b - FTR_SIZE);
    if (*prev_ftr & FLAG_ALLOC) return b;          /* neighbour busy: cannot merge */
    usize psize = *prev_ftr & SIZE_MASK;           /* previous block's total size  */
    block *pv = (block *)((char *)b - psize);       /* step back to its header      */
    pv->size = blk_size(pv) + blk_size(b);         /* grow it forward over b       */
    return pv;                                      /* merged block starts at pv    */
}
