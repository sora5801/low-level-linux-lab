/* ===========================================================================
 * malloc.c — a tiny first-fit free-list allocator on top of brk/sbrk.
 * ===========================================================================
 *
 * This is the classic teaching allocator (the shape of K&R malloc and of
 * glibc's earliest ancestor): one singly-linked list of block headers in
 * ADDRESS order, each tagged free/used. malloc does first-fit + split; free
 * marks a block and coalesces physically-adjacent free neighbors. The heap
 * grows by pushing the program break up with sbrk().
 *
 * WHAT IT TEACHES
 *   - The header-before-payload trick: given the user's pointer, the metadata
 *     is at a fixed negative offset. This is exactly why heap overflows are so
 *     dangerous — a write past one allocation lands in the NEXT block's header,
 *     letting an attacker forge `size`/`next` and corrupt the allocator itself.
 *   - Alignment: malloc must return memory aligned for ANY type (16 bytes on
 *     x86-64, enough for max_align_t and SSE). We pad the header to 16 and keep
 *     the break 16-aligned so every payload is aligned.
 *   - Fragmentation: first-fit + split + coalesce is the minimum viable defense.
 *
 * WHAT IT OMITS (honest scope): no size-class bins, no per-thread arenas, no
 * mmap fast-path for huge requests, no security hardening (glibc's tcache
 * poisoning checks, safe-linking). A single global list is not thread-safe.
 * Read glibc malloc/malloc.c and musl's mallocng for the real machinery.
 * ===========================================================================
 */
#include "minilibc.h"

/* All allocations are 16-byte aligned: the strictest alignment any scalar or
 * SSE type needs on x86-64, so a payload is safe to hold anything. */
#define ALIGN       16UL
#define ALIGN_UP(n) (((n) + (ALIGN - 1)) & ~(ALIGN - 1))

/* Per-allocation bookkeeping, stored immediately BEFORE the payload we hand
 * out. Reachable from the user pointer as ((block_header_t*)ptr) - 1 in spirit,
 * but we use an explicit padded offset (HDR) so the payload stays 16-aligned. */
typedef struct block_header {
	size_t               size;   /* usable payload bytes (already ALIGN_UP'd) */
	struct block_header *next;   /* next block in ascending address order     */
	int                  free;   /* 1 = available, 0 = handed out             */
	int                  magic;  /* sentinel: catches double-free / bad ptr   */
} block_header_t;

/* Header size rounded up to ALIGN, so payload = block + HDR is 16-aligned. */
#define HDR         ALIGN_UP(sizeof(block_header_t))
#define MAGIC_USED  0x1155ED    /* "is used"  — set while allocated           */
#define MAGIC_FREE  0x0F7EE0    /* "is free"  — set once freed                */

/* Payload <-> header conversions (the fixed-offset trick). */
#define PAYLOAD(b)  ((void *)((char *)(b) + HDR))
#define HEADER(p)   ((block_header_t *)((char *)(p) - HDR))

/* The head of the heap's block list. NULL until the first malloc. A real libc
 * would never use a mutable global like this without a lock — noted in README. */
static block_header_t *g_base = NULL;

/* ---------------------------------------------------------------------------
 * request_space — carve a fresh block off the top of the heap via sbrk.
 *
 * We grow the break by `pad + HDR + size`: `pad` first re-aligns the break to
 * 16 (only ever needed on the very first call, since every subsequent chunk is
 * a multiple of 16), then HDR + payload. `last`, if given, is the current tail
 * of the list, which we link the new block onto.
 * --------------------------------------------------------------------------- */
static block_header_t *request_space(block_header_t *last, size_t size)
{
	void *base = sbrk(0);                       /* where the break is now      */
	uintptr_t mis = (uintptr_t)base & (ALIGN - 1);
	size_t pad = mis ? (ALIGN - mis) : 0;       /* bytes to reach 16-alignment */

	void *p = sbrk((intptr_t)(pad + HDR + size));
	if (p == (void *)-1)                        /* kernel refused (ENOMEM set) */
		return NULL;

	block_header_t *block = (block_header_t *)((char *)p + pad);
	block->size  = size;
	block->next  = NULL;
	block->free  = 0;
	block->magic = MAGIC_USED;
	if (last)
		last->next = block;                     /* append in address order     */
	return block;
}

/* first-fit: return the first free block big enough, recording the tail in
 * *last so the caller can extend the heap if the search fails. */
static block_header_t *find_free_block(block_header_t **last, size_t size)
{
	block_header_t *cur = g_base;
	while (cur && !(cur->free && cur->size >= size)) {
		*last = cur;
		cur = cur->next;
	}
	return cur;
}

/* split — if `block` is much larger than `size`, cut the tail into a new free
 * block so the surplus can be reused. We only split when the remainder can
 * hold a header PLUS at least one aligned payload; otherwise the sliver would
 * be unusable and we leave it as internal slack (a small waste, but safe). */
static void split_block(block_header_t *block, size_t size)
{
	if (block->size >= size + HDR + ALIGN) {
		block_header_t *rest =
			(block_header_t *)((char *)block + HDR + size);
		rest->size  = block->size - size - HDR;
		rest->next  = block->next;
		rest->free  = 1;
		rest->magic = MAGIC_FREE;
		block->size = size;
		block->next = rest;
	}
}

/* coalesce — merge each run of physically-adjacent free blocks into one. This
 * is what keeps free() from leaving the heap as confetti. We require true
 * address adjacency ((char*)cur + HDR + cur->size == cur->next) so we never
 * merge across the alignment `pad` gap that can sit between two sbrk chunks. */
static void coalesce(void)
{
	block_header_t *cur = g_base;
	while (cur && cur->next) {
		if (cur->free && cur->next->free &&
		    (char *)cur + HDR + cur->size == (char *)cur->next) {
			cur->size += HDR + cur->next->size;  /* absorb neighbor + its hdr */
			cur->next  = cur->next->next;
			continue;                            /* retry: chains of frees     */
		}
		cur = cur->next;
	}
}

/* ---------------------------------------------------------------------------
 * malloc — return a 16-aligned block of at least `size` usable bytes, or NULL.
 * --------------------------------------------------------------------------- */
void *malloc(size_t size)
{
	if (size == 0)
		return NULL;                 /* 0-byte request: NULL is a valid answer */
	size = ALIGN_UP(size);           /* round the request up to 16             */

	block_header_t *block;
	if (g_base == NULL) {            /* first ever allocation: start the heap  */
		block = request_space(NULL, size);
		if (!block)
			return NULL;
		g_base = block;
	} else {
		block_header_t *last = g_base;
		block = find_free_block(&last, size);
		if (block) {                 /* reuse a freed block                    */
			block->free  = 0;
			block->magic = MAGIC_USED;
			split_block(block, size);
		} else {                     /* no fit: grow the heap                  */
			block = request_space(last, size);
			if (!block)
				return NULL;
		}
	}
	return PAYLOAD(block);
}

/* free — return a block to the list and coalesce. Ignores NULL (per C). The
 * magic check turns a double-free or a bogus pointer into a silent no-op
 * instead of corrupting the list — a tiny nod to the hardening real allocators
 * do; a production allocator would abort() loudly. */
void free(void *ptr)
{
	if (ptr == NULL)
		return;
	block_header_t *block = HEADER(ptr);
	if (block->magic != MAGIC_USED)  /* not a live allocation of ours          */
		return;
	block->free  = 1;
	block->magic = MAGIC_FREE;
	coalesce();
}

/* calloc — zeroed array allocation with the mandatory overflow check. If
 * nmemb*size overflows size_t, a naive multiply would wrap to a small value,
 * we'd allocate too little, and the caller's writes would overflow the heap —
 * a historically common exploit. We reject the overflow up front. */
void *calloc(size_t nmemb, size_t size)
{
	if (nmemb != 0 && size > (size_t)-1 / nmemb) {  /* would overflow?         */
		errno = ENOMEM;
		return NULL;
	}
	size_t total = nmemb * size;
	void *p = malloc(total);
	if (p)
		memset(p, 0, total);        /* calloc's contract: memory is zeroed     */
	return p;
}

/* realloc — grow/shrink an allocation, preserving contents.
 *   realloc(NULL, n)  == malloc(n)      realloc(p, 0) == free(p), returns NULL
 * If the existing block is already big enough we return it unchanged (no
 * shrink-split here, for simplicity). Otherwise allocate fresh, copy the old
 * payload, and free the original. */
void *realloc(void *ptr, size_t size)
{
	if (ptr == NULL)
		return malloc(size);
	if (size == 0) {
		free(ptr);
		return NULL;
	}
	block_header_t *block = HEADER(ptr);
	size_t need = ALIGN_UP(size);
	if (block->size >= need)         /* current block already suffices         */
		return ptr;

	void *np = malloc(size);
	if (!np)
		return NULL;                 /* old block left intact on failure        */
	memcpy(np, ptr, block->size);    /* copy only the valid old payload bytes  */
	free(ptr);
	return np;
}
