/* ===========================================================================
 * mymalloc.c — a real, working malloc/free/realloc/calloc for Linux/x86-64.
 * ===========================================================================
 *
 * WHAT THIS IS
 * ------------
 * A teaching allocator that is nonetheless a genuine one: it manages memory it
 * gets from the kernel with brk/sbrk (the main heap) and mmap (large blocks),
 * hands out 16-byte-aligned payloads, coalesces adjacent free blocks in O(1)
 * using boundary tags, sorts free blocks into segregated free lists by size
 * class, detects several kinds of heap corruption via a header magic and a
 * header/footer redundancy check, and offers a tcmalloc-style per-thread cache
 * for the small-allocation fast path. Compiled as a shared object it drops in
 * under LD_PRELOAD and replaces glibc's allocator for any dynamically-linked
 * program.
 *
 * It is a *core*, honestly scoped. See the README's "Going further" for what a
 * production allocator (glibc's ptmalloc, jemalloc, tcmalloc, mimalloc) adds
 * that we deliberately leave out: multiple arenas, fine-grained locking, decay-
 * based page reclaim (MADV_FREE), rigorous security hardening, per-size-class
 * page runs, and so on.
 *
 * ---------------------------------------------------------------------------
 * BLOCK LAYOUT  (boundary-tagged, so coalescing is O(1) in both directions)
 * ---------------------------------------------------------------------------
 * Every block, allocated or free, looks like this in memory:
 *
 *      +--------------------- header_t (16 bytes) --------------------+
 *      | size_t size    : total block size incl header+footer,        |
 *      |                  always a multiple of 16 so the low 4 bits    |
 *      |                  are stolen for FLAG_ALLOC / FLAG_MMAP.       |
 *      | u32    magic    : HDR_MAGIC — a canary that catches wild      |
 *      |                  writes, double frees, and free(bad_ptr).     |
 *      | u32    req      : exact bytes the user asked for (diagnostic).|
 *      +--------------- payload (16-byte aligned) --------------------+
 *      |  ... user bytes ...      (for a FREE block, the first 16      |
 *      |                           bytes instead hold two list links)  |
 *      +--------------------- footer_t (8 bytes) ---------------------+
 *      | size_t size    : a MIRROR of the header's size+flags. Lets a  |
 *      |                  block find its physically-previous neighbour |
 *      |                  by reading (this_header - 8). That is the     |
 *      |                  "boundary tag" that makes backward coalescing |
 *      |                  O(1) with no search.                         |
 *      +--------------------------------------------------------------+
 *
 * ALIGNMENT MATH.  The payload must be 16-aligned (that satisfies every scalar
 * and SSE type on x86-64, i.e. _Alignof(max_align_t) == 16). The header is 16
 * bytes, so payload = header + 16 is 16-aligned *iff the header is*. We keep
 * every header 16-aligned by (a) aligning the very first header up to 16, and
 * (b) making every block's total size a multiple of 16 — then header N+1 =
 * header N + size is 16-aligned by induction. The 8-byte footer therefore lands
 * at an address that is 8 mod 16, which is fine: only payloads must be 16-aligned,
 * not footers.
 *
 * FRAGMENTATION.  Two enemies. *External* fragmentation = free memory exists but
 * is scattered in pieces too small to satisfy a request; we fight it by coalescing
 * neighbours on every free (boundary tags) and by segregating free blocks into
 * size classes so a fit is found quickly and a good size is reused. *Internal*
 * fragmentation = the slack between what the user asked for and the block we gave
 * them; we minimise it by rounding requests to a 16-byte grain (not to the next
 * power of two) and by splitting a larger free block when the leftover is itself
 * a usable block (>= MIN_BLOCK).
 *
 * THE mmap THRESHOLD.  Small/medium requests come from the contiguous brk heap;
 * requests >= MMAP_THRESHOLD (128 KiB, matching glibc's default) are served by a
 * dedicated mmap(2) region and returned to the kernel with munmap(2) on free.
 * Why the split? (1) A huge block on the brk heap could never be released until
 * everything above it is freed too (the break only moves as a stack); mmap lets
 * us hand the pages straight back. (2) It bounds worst-case fragmentation for big
 * objects. (3) mmap'd pages are demand-zeroed by the kernel, which calloc exploits.
 * ===========================================================================
 */

/* sbrk/brk and MAP_ANONYMOUS are glibc "default source" extensions; ask for them
 * explicitly and BEFORE any header so their prototypes/macros are visible under
 * -Wall -Wextra (otherwise sbrk would be implicitly declared -> a warning/error). */
#define _DEFAULT_SOURCE 1

#include <unistd.h>     /* sbrk(2), brk(2), sysconf(3)                            */
#include <sys/mman.h>   /* mmap(2), munmap(2), MAP_ANONYMOUS, PROT_*             */
#include <pthread.h>    /* pthread_mutex_*, pthread_key_* (thread-exit flush)    */
#include <string.h>     /* memcpy, memset — pure libc, no allocation             */
#include <stdint.h>     /* uint32_t, uintptr_t, SIZE_MAX                         */
#include <errno.h>      /* ENOMEM, EINVAL                                        */
#include <stddef.h>     /* size_t, NULL                                          */

#include "mymalloc.h"

/* Some libcs spell the anonymous-mapping flag MAP_ANON instead of MAP_ANONYMOUS. */
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

/* ===========================================================================
 * 1. TUNABLES AND CONSTANTS
 * ===========================================================================
 */

#define ALIGNMENT       16u                 /* payload alignment (max_align_t)      */
#define HDR_SIZE        16u                 /* sizeof(header_t), keeps payload 16-al*/
#define FTR_SIZE        8u                  /* sizeof(footer_t), a size mirror      */

/* Smallest whole block we will ever create. A *free* block must be able to hold
 * two list links (16 bytes) in its payload, so: header(16) + links(16) + footer(8)
 * = 40, rounded up to the 16-byte grain = 48. */
#define MIN_BLOCK       48u

/* Flags packed into the low 4 bits of header.size (size is 16-aligned => low 4
 * bits are always zero and free for our use). */
#define FLAG_ALLOC      0x1u                /* 1 = in use, 0 = on a free list       */
#define FLAG_MMAP       0x2u                /* 1 = backed by its own mmap region    */
#define FLAG_MASK       0xFu
#define SIZE_MASK       (~(size_t)FLAG_MASK)

/* Header/footer canaries. HDR_MAGIC catches almost every "this pointer is not
 * one of ours / has been stomped" situation on free() and realloc(). */
#define HDR_MAGIC       0xC0DEFEEDu

/* Requests this large skip the brk heap and get their own mmap region. 128 KiB
 * is glibc's default M_MMAP_THRESHOLD; matching it keeps behaviour familiar. */
#define MMAP_THRESHOLD  (128u * 1024u)

/* When the brk heap is exhausted we grow it in big chunks (amortising the sbrk
 * syscall cost) rather than by the exact request. */
#define HEAP_CHUNK      (256u * 1024u)

/* x86-64 base page size. We could ask sysconf(_SC_PAGESIZE) at startup, and do
 * once for mmap sizing, but 4096 is the constant every reader expects to see. */
#define PAGE_SIZE       4096u

/* Number of segregated free lists (size classes). Class i holds blocks whose
 * total size is in [2^(i+5), 2^(i+6)). i=0 => [32,64) which in practice means the
 * 48-byte minimum; the top class is a catch-all for everything large. */
#define NUM_BINS        16

/* Per-thread cache (tcache) covers small, EXACT block sizes 48,64,...,48+16*(N-1).
 * These are the allocations that dominate real workloads and where lock traffic
 * hurts most, so we serve them without touching the global lock at all. */
#define TCACHE_BINS     16                  /* exact classes 48 .. 288 bytes        */
#define TCACHE_CAP      64                  /* max parked blocks per class, per thd */

/* ===========================================================================
 * 2. BLOCK TYPES
 * ===========================================================================
 */

typedef struct header {
    size_t   size;   /* total block bytes (hdr+payload+ftr); low 4 bits are flags  */
    uint32_t magic;  /* HDR_MAGIC when valid                                       */
    uint32_t req;    /* exact bytes requested (clamped to u32; diagnostic only)    */
} header_t;          /* exactly 16 bytes                                          */

typedef struct footer {
    size_t   size;   /* mirror of header.size (with flags) for O(1) back-coalesce  */
} footer_t;          /* exactly 8 bytes                                           */

/* For a FREE block, the first 16 bytes of the payload are reinterpreted as two
 * intrusive doubly-linked-list pointers. This costs zero extra space: a free
 * block is not holding user data anyway. That is why MIN_BLOCK reserves 16 bytes
 * of payload. */
typedef struct links {
    header_t *next;  /* next free block in the same size class                     */
    header_t *prev;  /* previous free block in the same size class                 */
} links_t;

/* ===========================================================================
 * 3. GLOBAL STATE  (all guarded by g_lock unless noted __thread)
 * ===========================================================================
 */

/* The heads of the segregated free lists. NULL = empty class. */
static header_t *g_bins[NUM_BINS];

/* Bounds of the single contiguous brk heap. Everything in [g_heap_base, g_brk_end)
 * is a chain of blocks; these bounds let us know when a physical neighbour would
 * fall off either end so we never coalesce past the edge. */
static char *g_heap_base = NULL;   /* first (16-aligned) header on the brk heap    */
static char *g_brk_end   = NULL;   /* one past the last byte we own from sbrk       */

/* One big lock. It serialises all mutation of g_bins, the heap bounds, and stats.
 * A single global lock is the simplest thing that is *correct* under multithreading;
 * the race it prevents is two threads splicing the same free list concurrently
 * (which would corrupt the list and hand the same block to two callers). Production
 * allocators shard this into per-arena or per-thread locks — see the tcache below,
 * which removes the lock from the hot path entirely for small sizes. */
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

/* Coarse statistics for diagnostics and the benchmark. Updated under g_lock. */
static struct my_stats g_stats;

/* ===========================================================================
 * 4. TINY HELPERS  (pure arithmetic + typed pointer navigation)
 * ===========================================================================
 * These are the boundary-tag arithmetic in one place. They compile to a couple
 * of instructions each; asm/demo.c extracts the interesting ones for annotation.
 */

/* Round `n` up to the next multiple of `a` (a must be a power of two). The trick:
 * adding (a-1) pushes us into the next multiple's "bucket", and masking off the
 * low bits floors back down to the multiple. No division, no branch. */
static inline size_t align_up(size_t n, size_t a) {
    return (n + (a - 1)) & ~(a - 1);
}

/* Total block bytes needed to satisfy a user request of `n` bytes: header +
 * payload + footer, rounded to the 16-byte grain, never below MIN_BLOCK. */
static inline size_t round_total(size_t n) {
    size_t t = HDR_SIZE + n + FTR_SIZE;
    t = align_up(t, ALIGNMENT);
    return t < MIN_BLOCK ? MIN_BLOCK : t;
}

static inline size_t   blk_size(const header_t *h) { return h->size & SIZE_MASK; }
static inline int      is_alloc(const header_t *h) { return (h->size & FLAG_ALLOC) != 0; }
static inline int      is_mmap (const header_t *h) { return (h->size & FLAG_MMAP)  != 0; }

/* payload <-> header conversions. The payload is HDR_SIZE past the header. */
static inline void    *payload_of(header_t *h)  { return (char *)h + HDR_SIZE; }
static inline header_t *header_of(void *p)       { return (header_t *)((char *)p - HDR_SIZE); }

/* The free-list links live at the start of a free block's payload. */
static inline links_t *links_of(header_t *h)     { return (links_t *)((char *)h + HDR_SIZE); }

/* The footer is the last FTR_SIZE bytes of the block. */
static inline footer_t *footer_of(header_t *h)   { return (footer_t *)((char *)h + blk_size(h) - FTR_SIZE); }

/* The physically-adjacent neighbours, found purely by arithmetic on sizes — this
 * is what boundary tags buy us. next_hdr walks forward by our own size; prev via
 * the previous block's footer (which sits immediately below our header). */
static inline header_t *next_hdr(header_t *h)    { return (header_t *)((char *)h + blk_size(h)); }
static inline footer_t *prev_ftr(header_t *h)    { return (footer_t *)((char *)h - FTR_SIZE); }

/* Usable payload capacity of a live block (what malloc_usable_size returns). */
static inline size_t usable_of(header_t *h) {
    return is_mmap(h) ? blk_size(h) - HDR_SIZE       /* mmap blocks carry no footer */
                      : blk_size(h) - HDR_SIZE - FTR_SIZE;
}

/* ===========================================================================
 * 5. CORRUPTION REPORTER
 * ===========================================================================
 * We must NOT use printf/fprintf here: stdio can call malloc, and reporting a
 * heap problem by re-entering the very allocator that is corrupt is a great way
 * to deadlock or crash worse. So we format nothing and write a fixed string with
 * the raw write(2) syscall (SYS_write, number 1: rax=1, rdi=fd, rsi=buf, rdx=len;
 * kernel copies the bytes to the fd and returns the count or -errno). fd 2 = stderr.
 */
static void report(const char *msg) {
    const char *p = msg;
    size_t len = 0;
    while (p[len]) len++;
    /* Best-effort: ignore EINTR/short writes here — we are already on a fatal path. */
    ssize_t rc = write(2, msg, len);
    (void)rc;
}

/* Validate that `h` looks like one of our headers. Called on the free/realloc
 * paths where a bad pointer or an overflow of the *previous* allocation would
 * have trashed this header. On failure we abort loudly rather than corrupt further. */
static void check_header(header_t *h, const char *where) {
    if (h->magic != HDR_MAGIC) {
        report("mymalloc: heap corruption or invalid pointer at ");
        report(where);
        report(" (bad header magic)\n");
        /* SIGABRT via the raw path: production would call abort(); we deliberately
         * do not longjmp or try to recover — the heap invariants are already gone. */
        __builtin_trap();
    }
}

/* ===========================================================================
 * 6. SIZE-CLASS INDEX  (segregated free lists)
 * ===========================================================================
 * Map a block's *total* size to a bin. We use the position of the highest set
 * bit — floor(log2(size)) — so bins grow geometrically and any size lands in
 * O(1). __builtin_clzll(x) returns the count of leading zero bits of a 64-bit x
 * (undefined for x==0, which we never pass), so 63 - clz(x) = index of the top
 * set bit. This exact bit trick is the star of asm/demo.c.
 */
static int bin_index(size_t total) {
    if (total < MIN_BLOCK) total = MIN_BLOCK;                 /* clamp; clz(0) is UB */
    unsigned floor_log2 = 63u - (unsigned)__builtin_clzll((unsigned long long)total);
    int idx = (int)floor_log2 - 5;   /* total>=32 => floor_log2>=5 => idx>=0        */
    if (idx < 0) idx = 0;
    if (idx >= NUM_BINS) idx = NUM_BINS - 1;                  /* top bin is catch-all */
    return idx;
}

/* Insert a free block at the head of its size class (LIFO: recently-freed blocks
 * are handed back first, which tends to be cache-friendly). */
static void bin_insert(header_t *h) {
    int b = bin_index(blk_size(h));
    links_t *L = links_of(h);
    L->prev = NULL;
    L->next = g_bins[b];
    if (g_bins[b]) links_of(g_bins[b])->prev = h;
    g_bins[b] = h;
    g_stats.free_blocks++;
}

/* Unlink a free block from whatever size class it is in. O(1) because the list
 * is doubly linked and the block stores both neighbours. */
static void bin_remove(header_t *h) {
    int b = bin_index(blk_size(h));
    links_t *L = links_of(h);
    if (L->prev) links_of(L->prev)->next = L->next;
    else         g_bins[b] = L->next;                        /* h was the head      */
    if (L->next) links_of(L->next)->prev = L->prev;
    g_stats.free_blocks--;
}

/* First-fit search: start at the exact size class and walk to larger classes.
 * Within a class we take the first block big enough. First-fit-within-segregated-
 * lists is a good speed/fragmentation compromise; best-fit would scan the whole
 * class for the tightest block (less waste, more time) — noted in the README. */
static header_t *find_fit(size_t total) {
    for (int b = bin_index(total); b < NUM_BINS; b++) {
        for (header_t *h = g_bins[b]; h; h = links_of(h)->next) {
            if (blk_size(h) >= total) return h;
        }
    }
    return NULL;
}

/* ===========================================================================
 * 7. WRITING BLOCKS AND SPLITTING
 * ===========================================================================
 */

/* Stamp a block's header AND footer with `total` size and `flags`. Writing both
 * tags is what keeps the boundary-tag invariant (header==footer) that both the
 * coalescer and the corruption checker rely on. */
static void set_block(header_t *h, size_t total, unsigned flags) {
    h->size = total | flags;
    h->magic = HDR_MAGIC;
    footer_of(h)->size = total | flags;      /* footer_of reads h->size, so set first */
}

/* Given a free block `h` big enough for `total`, mark `total` bytes of it
 * allocated and, if the leftover is itself a usable block (>= MIN_BLOCK), split
 * that tail off as a new free block on its own list. The tail's neighbours are
 * `h` (now allocated) and whatever followed `h` (already allocated, because `h`
 * was a fully-coalesced free block), so the tail needs no coalescing. */
static void place_and_split(header_t *h, size_t total) {
    size_t whole = blk_size(h);
    size_t extra = whole - total;
    if (extra >= MIN_BLOCK) {
        set_block(h, total, FLAG_ALLOC);
        header_t *rem = (header_t *)((char *)h + total);
        set_block(rem, extra, 0);            /* free remainder */
        bin_insert(rem);
    } else {
        /* Leftover too small to be its own block: hand out the whole thing. The
         * few wasted bytes become internal fragmentation — cheaper than a runt. */
        set_block(h, whole, FLAG_ALLOC);
    }
}

/* ===========================================================================
 * 8. GROWING THE BRK HEAP  (sbrk / the brk(2) syscall)
 * ===========================================================================
 * sbrk(n) moves the program break — the top of the process data segment — up by
 * n bytes and returns the PREVIOUS break (i.e. the base of the fresh memory), or
 * (void*)-1 on failure with errno=ENOMEM. Under the hood this is brk(2)
 * (syscall number 12: rdi = requested new break; the kernel extends the anonymous
 * mapping for the data segment and returns the resulting break). Successive sbrk
 * calls return CONTIGUOUS memory, which is exactly why the brk heap is one
 * unbroken run of blocks and forward/backward coalescing is just pointer math.
 */
static header_t *extend_heap(size_t need) {
    /* On the very first call, align the break up to 16 so our first header — and
     * therefore every payload — is 16-aligned. The break at startup is whatever
     * libc left it at and is not guaranteed aligned. */
    if (g_heap_base == NULL) {
        char *cur = (char *)sbrk(0);                 /* query current break, no move */
        if (cur == (char *)-1) return NULL;
        size_t misalign = (uintptr_t)cur & (ALIGNMENT - 1);
        if (misalign) {
            if (sbrk((intptr_t)(ALIGNMENT - misalign)) == (void *)-1) return NULL;
        }
    }

    /* Grow by at least `need`, but amortise the syscall by taking a big chunk.
     * Round to a page so the kernel bookkeeping is page-granular. */
    size_t grow = need > HEAP_CHUNK ? need : HEAP_CHUNK;
    grow = align_up(grow, PAGE_SIZE);

    char *p = (char *)sbrk((intptr_t)grow);
    if (p == (char *)-1) { errno = ENOMEM; return NULL; }    /* out of address space */

    if (g_heap_base == NULL) g_heap_base = p;                /* first region base    */
    g_brk_end = p + grow;
    g_stats.bytes_from_os += grow;

    /* The fresh region becomes one big free block. */
    header_t *h = (header_t *)p;
    set_block(h, grow, 0);

    /* Immediately coalesce backward with the old top block if it was free — this
     * is what lets a stream of frees-then-a-big-malloc reuse the tail instead of
     * stacking two adjacent free blocks. We read the previous block's footer,
     * which sits at (h - 8); its mirrored size+flags tell us everything in O(1). */
    if ((char *)h > g_heap_base) {
        footer_t *pf = prev_ftr(h);
        if (!(pf->size & FLAG_ALLOC)) {
            size_t psize = pf->size & SIZE_MASK;
            header_t *pv = (header_t *)((char *)h - psize);
            bin_remove(pv);
            set_block(pv, psize + grow, 0);
            h = pv;
        }
    }
    return h;   /* a free block, NOT yet on a list and NOT yet split */
}

/* ===========================================================================
 * 9. LARGE ALLOCATIONS  (mmap / munmap)
 * ===========================================================================
 * mmap(2) (syscall number 9: rdi=addr hint, rsi=length, rdx=prot, r10=flags,
 * r8=fd, r9=offset) asks the kernel for a fresh anonymous, demand-zeroed, private
 * mapping. We use it for big blocks so free() can hand the pages straight back to
 * the kernel with munmap(2) (syscall 11: rdi=addr, rsi=length), instead of trapping
 * them below a high-water brk mark forever.
 */
static void *mmap_alloc(size_t n) {
    /* We only need a header (no footer: an mmap block has no neighbours to
     * coalesce with). Round the whole mapping up to a page. */
    size_t map_len = align_up(HDR_SIZE + n, PAGE_SIZE);

    void *p = mmap(NULL, map_len, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) { errno = ENOMEM; return NULL; }

    header_t *h = (header_t *)p;
    h->size  = map_len | FLAG_ALLOC | FLAG_MMAP;   /* map_len is page-aligned => 16-al */
    h->magic = HDR_MAGIC;
    h->req   = n > 0xFFFFFFFFu ? 0xFFFFFFFFu : (uint32_t)n;

    pthread_mutex_lock(&g_lock);
    g_stats.bytes_from_os += map_len;
    g_stats.mmap_blocks++;
    g_stats.blocks_live++;
    g_stats.bytes_payload += usable_of(h);
    pthread_mutex_unlock(&g_lock);

    /* mmap returns page-aligned memory, so payload = p + 16 is 16-aligned. */
    return payload_of(h);
}

static void mmap_free(header_t *h) {
    size_t map_len = blk_size(h);
    pthread_mutex_lock(&g_lock);
    g_stats.mmap_blocks--;
    g_stats.blocks_live--;
    g_stats.bytes_payload -= usable_of(h);
    pthread_mutex_unlock(&g_lock);
    /* munmap can fail only with EINVAL (misaligned/length 0), which for one of our
     * own mappings would itself indicate corruption; report but don't loop. */
    if (munmap(h, map_len) != 0) report("mymalloc: munmap failed\n");
}

/* ===========================================================================
 * 10. RETURNING A BLOCK TO THE GLOBAL HEAP  (coalesce + insert)
 * ===========================================================================
 * Precondition: g_lock is held and `h` is a brk-heap block currently marked
 * allocated. We clear the alloc bit, merge with any free physical neighbour on
 * each side (removing them from their size classes first), then file the merged
 * block on its class. This is the O(1) boundary-tag coalescing.
 */
static void heap_free_locked(header_t *h) {
    size_t size = blk_size(h);
    g_stats.blocks_live--;
    g_stats.bytes_payload -= usable_of(h);

    /* Forward: is the next physical block within the heap and free? */
    header_t *nx = next_hdr(h);
    if ((char *)nx < g_brk_end && !is_alloc(nx)) {
        bin_remove(nx);
        size += blk_size(nx);
    }

    /* Backward: is the previous physical block free? Read its footer at (h-8). */
    if ((char *)h > g_heap_base) {
        footer_t *pf = prev_ftr(h);
        if (!(pf->size & FLAG_ALLOC)) {
            size_t psize = pf->size & SIZE_MASK;
            header_t *pv = (header_t *)((char *)h - psize);
            bin_remove(pv);
            h = pv;                 /* the merged block now starts at the predecessor */
            size += psize;
        }
    }

    set_block(h, size, 0);          /* one free block spanning the merged range */
    bin_insert(h);
}

/* ===========================================================================
 * 11. PER-THREAD CACHE  (tcmalloc-style small-object fast path)
 * ===========================================================================
 * The global lock is fine for correctness but is a scalability bottleneck: every
 * malloc/free of a 32-byte node would serialise all threads. The fix that
 * tcmalloc/jemalloc/glibc-tcache all use is a THREAD-LOCAL cache of recently-freed
 * small blocks. Because the cache is `__thread`, each thread has its own copy and
 * touching it needs no lock at all — the data race (two threads mutating one free
 * list) cannot happen because there is no shared list to race on.
 *
 * We cache only EXACT small block sizes 48,64,...,48+16*(TCACHE_BINS-1). A parked
 * block keeps its ALLOC bit set and stays OUT of the global free lists, so the
 * global coalescer never touches it — it is logically owned by this thread. On
 * malloc we pop an exact-size block (no lock); on free we push (no lock) until the
 * class is full, then fall through to the locked global path.
 *
 * THREAD EXIT.  A thread that dies with blocks still parked would leak them, so we
 * register a pthread_key destructor that flushes the whole tcache back to the
 * global heap (under the lock) when the thread exits.
 */
typedef struct tc_bin { header_t *head; int count; } tc_bin_t;

/* __thread => one independent instance per thread, in that thread's TLS block.
 * No synchronisation needed to read or write our own cache. */
static __thread tc_bin_t g_tc[TCACHE_BINS];
static __thread int      g_tc_registered;     /* have we hooked thread-exit yet?     */

static pthread_key_t  g_tc_key;
static pthread_once_t g_tc_once = PTHREAD_ONCE_INIT;

/* Map a total size to an EXACT tcache class, or -1 if it is not cacheable.
 * class k <-> size (MIN_BLOCK + 16*k). */
static int tc_index(size_t total) {
    if (total < MIN_BLOCK) return -1;
    size_t k = (total - MIN_BLOCK) / ALIGNMENT;
    return k < TCACHE_BINS ? (int)k : -1;
}

/* Push every parked block of the calling thread back to the global heap. Runs
 * either from the thread-exit destructor or could be called manually. */
static void tc_flush_all(void) {
    for (int i = 0; i < TCACHE_BINS; i++) {
        header_t *h = g_tc[i].head;
        while (h) {
            header_t *nx = links_of(h)->next;   /* single-linked via links.next */
            pthread_mutex_lock(&g_lock);
            heap_free_locked(h);
            pthread_mutex_unlock(&g_lock);
            h = nx;
        }
        g_tc[i].head = NULL;
        g_tc[i].count = 0;
    }
}

static void tc_destructor(void *unused) { (void)unused; tc_flush_all(); }
static void tc_make_key(void)          { pthread_key_create(&g_tc_key, tc_destructor); }

/* Ensure this thread's exit will flush its cache. We set the guard BEFORE calling
 * into pthread so that if pthread_* were ever to re-enter our allocator, it sees
 * "already registered" and cannot recurse here. (On glibc pthread_key_create uses
 * a static table for the first 1024 keys and does not allocate, so in practice no
 * re-entry happens — but the ordering makes it safe regardless.) */
static void tc_register(void) {
    if (g_tc_registered) return;
    g_tc_registered = 1;
    pthread_once(&g_tc_once, tc_make_key);
    /* A non-NULL value is required for the destructor to be invoked at exit. */
    pthread_setspecific(g_tc_key, (void *)1);
}

/* ===========================================================================
 * 12. PUBLIC ALLOCATOR ENTRY POINTS
 * ===========================================================================
 */

void *my_malloc(size_t n) {
    /* malloc(0): the standard permits NULL or a unique freeable pointer. We return
     * a real minimum block so that programs which free() the result are happy. */
    if (n == 0) n = 1;

    /* Overflow guard: a request so large that header+footer+alignment would wrap
     * cannot be satisfied. Fail like the real thing (NULL + ENOMEM). */
    if (n > SIZE_MAX - HDR_SIZE - FTR_SIZE - ALIGNMENT) { errno = ENOMEM; return NULL; }

    /* Big request => private mmap region (no lock on the heap structures). */
    if (n >= MMAP_THRESHOLD) return mmap_alloc(n);

    size_t total = round_total(n);

    /* -- FAST PATH: per-thread cache, no global lock -- */
    int ti = tc_index(total);
    if (ti >= 0 && g_tc[ti].head) {
        header_t *h = g_tc[ti].head;
        g_tc[ti].head = links_of(h)->next;     /* pop */
        g_tc[ti].count--;
        h->req = (uint32_t)n;                   /* header already ALLOC + right size */
        return payload_of(h);
    }

    /* -- SLOW PATH: global segregated lists, under the lock -- */
    pthread_mutex_lock(&g_lock);

    header_t *h = find_fit(total);
    if (h) {
        bin_remove(h);                          /* take it off its free list */
    } else {
        h = extend_heap(total);                 /* grow the brk heap via sbrk */
        if (!h) { pthread_mutex_unlock(&g_lock); errno = ENOMEM; return NULL; }
    }
    place_and_split(h, total);                   /* mark allocated, split any tail */
    h->req = (uint32_t)n;
    g_stats.blocks_live++;
    g_stats.bytes_payload += usable_of(h);

    pthread_mutex_unlock(&g_lock);
    return payload_of(h);
}

void my_free(void *p) {
    if (!p) return;                              /* free(NULL) is a no-op */

    header_t *h = header_of(p);
    check_header(h, "free");                      /* magic canary: catch bad/double free */

    if (is_mmap(h)) { mmap_free(h); return; }     /* big block: hand pages back */

    size_t total = blk_size(h);

    /* -- FAST PATH: park in the per-thread cache, no lock -- */
    int ti = tc_index(total);
    if (ti >= 0 && g_tc[ti].count < TCACHE_CAP) {
        tc_register();                            /* arrange thread-exit flush once */
        links_of(h)->next = g_tc[ti].head;        /* push (single-linked) */
        g_tc[ti].head = h;
        g_tc[ti].count++;
        /* Keep FLAG_ALLOC set: the block is logically owned by this thread and must
         * stay invisible to the global coalescer until it is flushed. */
        return;
    }

    /* -- SLOW PATH: return to the global heap (coalesce + file) -- */
    pthread_mutex_lock(&g_lock);
    heap_free_locked(h);
    pthread_mutex_unlock(&g_lock);
}

void *my_calloc(size_t nmemb, size_t size) {
    size_t total;
    /* calloc must guard the multiply: nmemb*size can overflow, and a wrapped small
     * value would under-allocate and invite a heap overflow. __builtin_mul_overflow
     * does the checked multiply in one instruction group. */
    if (__builtin_mul_overflow(nmemb, size, &total)) { errno = ENOMEM; return NULL; }

    void *p = my_malloc(total);
    if (!p) return NULL;

    /* We must zero unconditionally: a block reused from a free list holds stale
     * bytes. (Freshly sbrk'd/mmap'd pages are already zero, so a smarter allocator
     * tracks provenance and skips the memset for pristine pages — noted in README.) */
    memset(p, 0, total);
    return p;
}

void *my_realloc(void *p, size_t n) {
    if (!p)          return my_malloc(n);         /* realloc(NULL,n) == malloc(n)     */
    if (n == 0)    { my_free(p); return NULL; }   /* realloc(p,0): free, return NULL  */

    header_t *h = header_of(p);
    check_header(h, "realloc");
    size_t oldcap = usable_of(h);

    /* Shrink or same size: keep the block. We could split off a large tail here to
     * return memory; we keep it simple and only split on the grow-in-place path. */
    if (n <= oldcap) { h->req = n > 0xFFFFFFFFu ? 0xFFFFFFFFu : (uint32_t)n; return p; }

    /* Try to grow IN PLACE by swallowing a free physical successor (brk blocks only).
     * This avoids a copy for the common "buffer that keeps appending" pattern. */
    if (!is_mmap(h)) {
        size_t need = round_total(n);
        pthread_mutex_lock(&g_lock);
        header_t *nx = next_hdr(h);
        if ((char *)nx < g_brk_end && !is_alloc(nx) &&
            blk_size(h) + blk_size(nx) >= need) {
            bin_remove(nx);
            size_t merged = blk_size(h) + blk_size(nx);
            g_stats.bytes_payload -= usable_of(h);
            set_block(h, merged, FLAG_ALLOC);
            place_and_split(h, need);             /* trim any excess back to a free block */
            h->req = (uint32_t)n;
            g_stats.bytes_payload += usable_of(h);
            pthread_mutex_unlock(&g_lock);
            return p;
        }
        pthread_mutex_unlock(&g_lock);
    }

    /* Fallback: allocate fresh, copy the smaller of old/new, free the old. */
    void *q = my_malloc(n);
    if (!q) return NULL;                          /* old block left intact on failure */
    memcpy(q, p, oldcap < n ? oldcap : n);
    my_free(p);
    return q;
}

/* ---- Aligned allocation ----------------------------------------------------
 * Our blocks are already 16-aligned, so alignment <= 16 is just malloc. For a
 * larger power-of-two alignment we allocate a bigger brk block and CARVE it so
 * that a *real* header lands exactly 16 bytes before the aligned payload. That is
 * the trick that keeps free()/realloc() special-case-free: the returned pointer
 * is an ordinary block whose header is at (ptr - 16); the leading slack in front
 * becomes a normal free block that the aligned block will coalesce with when freed.
 *
 * We keep aligned allocations on the brk heap (never mmap) on purpose: an mmap
 * payload is page_base + 16, which is only 16-aligned and would NOT satisfy a
 * larger alignment, and its leading slack could not be a coalescable heap block. */
void *my_aligned_alloc(size_t alignment, size_t n) {
    if (alignment <= ALIGNMENT) return my_malloc(n);          /* 16-align is automatic */
    if (alignment & (alignment - 1)) { errno = EINVAL; return NULL; }   /* not 2^k     */
    if (n > SIZE_MAX - alignment - MIN_BLOCK - HDR_SIZE - FTR_SIZE - ALIGNMENT) {
        errno = ENOMEM; return NULL;                          /* request would overflow */
    }

    /* Over-allocate: worst case we skip up to (alignment-16) bytes to reach an
     * aligned address AND must leave a >= MIN_BLOCK free block in front of it. */
    size_t big   = n + alignment + MIN_BLOCK;
    size_t total = round_total(big);

    pthread_mutex_lock(&g_lock);

    /* Get one free brk block >= total, exactly like my_malloc's slow path but we
     * do the placement ourselves so we control where the aligned split falls. */
    header_t *rawh = find_fit(total);
    if (rawh) {
        bin_remove(rawh);
    } else {
        rawh = extend_heap(total);
        if (!rawh) { pthread_mutex_unlock(&g_lock); errno = ENOMEM; return NULL; }
    }
    size_t whole = blk_size(rawh);

    /* First alignment-aligned payload whose header (aligned-16) sits at least
     * MIN_BLOCK past rawh, so the leading gap is itself a legal free block. Both
     * raw_payload and `aligned` are multiples of 16, so `lead` is too. */
    char     *raw_payload = (char *)payload_of(rawh);
    uintptr_t aligned     = align_up((uintptr_t)raw_payload + MIN_BLOCK, alignment);
    header_t *newh        = (header_t *)(aligned - HDR_SIZE);
    size_t    lead        = (size_t)((char *)newh - (char *)rawh);   /* >= MIN_BLOCK */
    size_t    rest        = whole - lead;

    /* Leading gap -> a free block on its size class. Its neighbours are rawh's old
     * (allocated) predecessor and newh (about to be allocated), so no coalescing. */
    set_block(rawh, lead, 0);
    bin_insert(rawh);

    /* The aligned block gets the rest; trim any large tail back to a free block.
     * rest >= n + 40 by construction, so round_total(n) always fits. */
    set_block(newh, rest, FLAG_ALLOC);
    place_and_split(newh, round_total(n));
    newh->req = n > 0xFFFFFFFFu ? 0xFFFFFFFFu : (uint32_t)n;

    g_stats.blocks_live++;
    g_stats.bytes_payload += usable_of(newh);

    pthread_mutex_unlock(&g_lock);
    return payload_of(newh);
}

int my_posix_memalign(void **out, size_t alignment, size_t n) {
    /* POSIX: alignment must be a power of two AND a multiple of sizeof(void*). */
    if (alignment < sizeof(void *) || (alignment & (alignment - 1))) return EINVAL;
    void *p = my_aligned_alloc(alignment, n);
    if (!p) return ENOMEM;
    *out = p;
    return 0;
}

size_t my_malloc_usable_size(void *p) {
    if (!p) return 0;
    header_t *h = header_of(p);
    if (h->magic != HDR_MAGIC) return 0;
    return usable_of(h);
}

/* ===========================================================================
 * 13. DIAGNOSTICS
 * ===========================================================================
 */
int my_heap_check(void) {
    int rc = 0;
    pthread_mutex_lock(&g_lock);
    for (header_t *h = (header_t *)g_heap_base;
         g_heap_base && (char *)h < g_brk_end;
         h = next_hdr(h)) {
        if (h->magic != HDR_MAGIC) { report("heap_check: bad header magic\n"); rc = 1; break; }
        size_t sz = blk_size(h);
        if (sz < MIN_BLOCK || (sz & (ALIGNMENT - 1))) { report("heap_check: bad size\n"); rc = 2; break; }
        if (footer_of(h)->size != h->size) { report("heap_check: header/footer mismatch\n"); rc = 3; break; }
    }
    pthread_mutex_unlock(&g_lock);
    return rc;
}

void my_heap_stats(struct my_stats *out) {
    pthread_mutex_lock(&g_lock);
    *out = g_stats;
    pthread_mutex_unlock(&g_lock);
}

/* ===========================================================================
 * 14. STANDARD-NAME INTERPOSERS  (what LD_PRELOAD actually overrides)
 * ===========================================================================
 * Exporting these strong symbols means the dynamic linker resolves the program's
 * calls to `malloc`/`free`/... to us instead of to libc. Guard with a macro so a
 * unit-test build can pull in mymalloc.c for the my_* API without colliding with
 * the host libc's malloc.
 */
#ifndef MYMALLOC_NO_INTERPOSE

void  *malloc(size_t n)                     { return my_malloc(n); }
void   free(void *p)                        { my_free(p); }
void  *calloc(size_t nmemb, size_t size)    { return my_calloc(nmemb, size); }
void  *realloc(void *p, size_t n)           { return my_realloc(p, n); }
void  *aligned_alloc(size_t a, size_t n)    { return my_aligned_alloc(a, n); }
int    posix_memalign(void **o, size_t a, size_t n) { return my_posix_memalign(o, a, n); }
size_t malloc_usable_size(void *p)          { return my_malloc_usable_size(p); }

#endif /* MYMALLOC_NO_INTERPOSE */
