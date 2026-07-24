/* ===========================================================================
 * gc.c — a working conservative mark-and-sweep garbage collector for Linux/x86-64.
 * ===========================================================================
 *
 * WHAT THIS IS
 * ------------
 * A Boehm-style CONSERVATIVE collector. You call gc_malloc() and never free.
 * When memory runs low the collector finds every object still reachable from the
 * program's "roots" — the CPU registers, the whole call stack, and the global
 * data/bss — and reclaims the rest. It is *conservative* because it does not
 * know your types: it treats any aligned machine word that points into the heap
 * as if it were a live pointer. That policy can never free something that is
 * actually in use (the safety we want), but it can occasionally retain true
 * garbage (an integer that merely looks like a heap address). The whole point of
 * this file is to make that trade-off, and the register/stack-scanning trick
 * that powers it, concrete and readable.
 *
 * It is a *teaching core*, honestly scoped. It really works: it scans registers
 * via a setjmp spill, scans the exact live stack range, scans data/bss, marks
 * with an explicit (non-recursive) mark stack, and sweeps to a reuse free list,
 * and the bundled demo shows objects dying on cue and memory staying bounded.
 * What it deliberately is NOT: precise, generational, moving/compacting,
 * incremental, or thread-aware. The README's "Going further" lists the gap and
 * the stretch goal (a precise moving GC for a toy language).
 *
 * ---------------------------------------------------------------------------
 * HEAP LAYOUT: one contiguous reserve, committed on demand ("page tricks")
 * ---------------------------------------------------------------------------
 * A conservative collector's hottest operation is "does this random word point
 * into the heap?". We make that a single range compare by giving the heap ONE
 * contiguous virtual address range:
 *
 *   1. RESERVE. At startup we mmap GC_RESERVE bytes with PROT_NONE. PROT_NONE
 *      means "reserve the addresses but map no readable/writable pages" — the
 *      kernel just records the VMA; nothing is backed by physical memory and
 *      nothing is charged against RAM or swap. This nails down a fixed
 *      [heap_lo, heap_lo+RESERVE) window that will never move.
 *   2. COMMIT. As the bump pointer advances we mprotect(PROT_READ|PROT_WRITE)
 *      the next chunk, turning reserved-but-dead addresses into real, zero-filled
 *      pages on first touch (demand paging). So committed memory grows a chunk at
 *      a time, but the heap's *address range* was fixed at reserve time.
 *
 * Because the heap is contiguous, "is w a heap pointer?" is just
 *      heap_lo <= w < commit_top
 * and a mark bit for an object is indexed by its granule number
 *      (base - heap_lo) / GC_GRANULE
 * into a single flat bitmap. No per-arena lookup, no page table.
 *
 * ---------------------------------------------------------------------------
 * OBJECT TABLE + MARK BITMAP + FREE LIST
 * ---------------------------------------------------------------------------
 *   * Object table  — one descriptor {base, size, atomic} per LIVE object. It is
 *     the authority on "which object, if any, does address w fall inside?".
 *     Sorted by base at the start of each collection so the lookup is a binary
 *     search that also handles INTERIOR pointers (a pointer into the middle of
 *     an array still keeps it alive).
 *   * Mark bitmap   — one bit per granule; set when an object is proven reachable.
 *     Kept separate from the objects so marking never dirties an object's own
 *     cache lines, and so asm/demo.c can show the exact bit-twiddling.
 *   * Free list     — swept blocks are pushed here (intrusively, in their own
 *     first 16 bytes) and reused by later gc_malloc, so a program that allocates
 *     forever but keeps a bounded live set uses bounded memory.
 *
 * All of the collector's OWN bookkeeping (bitmap, object table, mark stack) is
 * obtained straight from mmap, never from libc malloc. That matters because the
 * LD_PRELOAD build (bottom of this file) *replaces* libc malloc — if our
 * internals called malloc they would recurse into ourselves.
 * ===========================================================================
 */

/* MAP_ANONYMOUS, MREMAP_MAYMOVE and friends are "default source" extensions;
 * request them before any header so the prototypes are visible under -Wall. */
#define _GNU_SOURCE 1

#include <sys/mman.h>   /* mmap(2), mprotect(2), munmap(2), mremap(2)            */
#include <unistd.h>     /* read(2), write(2), close(2)                           */
#include <fcntl.h>      /* open(2) for /proc/self/stat                           */
#include <setjmp.h>     /* setjmp(3) — spills callee-saved registers to memory   */
#include <string.h>     /* memset, memcpy (pure libc, never allocate)            */
#include <stdint.h>     /* uintptr_t, uint64_t                                   */
#include <stddef.h>     /* size_t, NULL                                          */

#include "gc.h"

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

/* ===========================================================================
 * 1. TUNABLES
 * ===========================================================================
 */

/* Allocation granularity AND alignment. 16 bytes satisfies _Alignof(max_align_t)
 * on x86-64, and because every object base is a multiple of 16 the low 4 bits of
 * a base are always zero — handy, and it means one mark bit covers 16 bytes. */
#define GC_GRANULE      16u

/* The heap's fixed virtual window. 256 MiB of ADDRESS SPACE, not RAM: the
 * PROT_NONE reservation costs nothing physical until we commit and touch pages.
 * Enlarge if a workload needs a bigger live set. */
#define GC_RESERVE      ((size_t)256 * 1024 * 1024)

/* How much we mprotect into existence at a time when the bump pointer runs off
 * the committed end. Bigger chunks = fewer mprotect syscalls, more slack. */
#define GC_COMMIT_CHUNK ((size_t)1 * 1024 * 1024)

/* x86-64 base page size. We align commits to this; the kernel works in pages. */
#define GC_PAGE         4096u

/* Low bit of an object's granule-count word is stolen as the "atomic" flag in
 * the object table's packed size field. (We keep it in a separate struct field
 * here for clarity; the flag idea is explained where used.) */

/* GC is triggered when bytes allocated since the last collection exceed this.
 * After each collection we retune it to a multiple of the live set (Boehm's
 * "free space divisor" idea): collect roughly when the heap has grown by as much
 * as is currently live, so GC cost stays proportional to live data. */
#define GC_INITIAL_THRESHOLD ((size_t)4 * 1024 * 1024)

/* ===========================================================================
 * 2. TYPES
 * ===========================================================================
 */

/* One live object. The mark bit is NOT here — it lives in the mark bitmap keyed
 * by granule — so that marking (a hot, random-access pass) never writes into the
 * table and never shares a cache line with object payload. */
typedef struct gc_obj {
    char  *base;    /* object start; always GC_GRANULE-aligned                   */
    size_t size;    /* usable bytes, a multiple of GC_GRANULE                    */
    int    atomic;  /* 1 => pointer-free: mark it, but never scan its interior   */
} gc_obj;

/* A reclaimed block, threaded intrusively through its own first bytes. A free
 * block is NOT in the object table, so the pointer test never "finds" it — which
 * is exactly why a stale pointer into freed memory cannot resurrect it. */
typedef struct free_node {
    struct free_node *next;
    size_t            size;   /* capacity of this block, a multiple of GRANULE   */
} free_node;

/* ===========================================================================
 * 3. GLOBAL STATE
 * ===========================================================================
 * Single-threaded teaching core: no lock. A real collector would stop the world
 * (signal every mutator thread, scan each thread's registers+stack) — see README.
 */

static char   *heap_lo;        /* base of the reserved window (fixed for life)   */
static char   *heap_reserved_end; /* heap_lo + GC_RESERVE                        */
static char   *commit_top;     /* first uncommitted byte (grows via mprotect)    */
static char   *alloc_top;      /* bump pointer: next fresh byte to hand out      */

static uint64_t *mark_bits;    /* one bit per granule of the reserve             */
static size_t    mark_words;   /* length of mark_bits in 64-bit words            */

static gc_obj   *objs;         /* object table (mmap-backed, grows via mremap)   */
static size_t    objs_len;     /* number of live descriptors                     */
static size_t    objs_cap;     /* capacity in descriptors                        */

static uint32_t *mark_stack;   /* explicit mark stack of object indices          */
static size_t    mark_sp;      /* mark-stack depth                               */
static size_t    mark_stack_cap;

static free_node *free_list;   /* singly-linked list of reclaimed blocks         */

static char   *stack_bottom;   /* highest stack address (captured at gc_init)    */
static int     gc_ready;       /* gc_init done?                                  */

static size_t  bytes_since_gc; /* allocation pressure since last collection      */
static size_t  gc_threshold = GC_INITIAL_THRESHOLD;

static struct gc_stats stats;

/* Linker-provided bounds of the program's global data + bss. On ELF/glibc,
 * __data_start marks the start of initialized data and _end marks one-past the
 * end of bss, so [__data_start, _end) covers every global/static variable — the
 * place global roots live. These are addresses, so we take &symbol. */
extern char __data_start[];
extern char _end[];

/* ===========================================================================
 * 4. A libc-free error/diagnostic writer
 * ===========================================================================
 * We must not printf from here: under LD_PRELOAD, stdio can call malloc, i.e.
 * back into us, mid-collection. write(2) (SYS_write, number 1: rax=1, rdi=fd,
 * rsi=buf, rdx=len; the kernel copies the bytes and returns the count or -errno)
 * touches no allocator, so it is always safe. fd 2 is stderr.
 */
static void ewrite(const char *s) {
    size_t n = 0;
    while (s[n]) n++;
    ssize_t rc = write(2, s, n);   /* best-effort: a diagnostic, not load-bearing */
    (void)rc;
}

static void ewrite_hex(unsigned long v) {
    char buf[19]; /* "0x" + 16 nybbles + NUL */
    buf[0] = '0'; buf[1] = 'x';
    for (int i = 0; i < 16; i++) {
        unsigned nyb = (v >> ((15 - i) * 4)) & 0xF;
        buf[2 + i] = (char)(nyb < 10 ? '0' + nyb : 'a' + (nyb - 10));
    }
    buf[18] = '\0';
    ewrite(buf);
}

static void die(const char *msg) {
    ewrite("gc: fatal: ");
    ewrite(msg);
    ewrite("\n");
    __builtin_trap();   /* SIGILL; we deliberately do not try to limp onward     */
}

/* ===========================================================================
 * 5. SMALL ARITHMETIC HELPERS  (these are what asm/demo.c extracts)
 * ===========================================================================
 */

/* Round n up to the next multiple of a (a a power of two). Adding (a-1) bumps
 * past the current multiple; masking with ~(a-1) floors back to it. No divide. */
static inline size_t align_up(size_t n, size_t a) {
    return (n + (a - 1)) & ~(a - 1);
}

/* The pointer-candidate range test: could w be a pointer into a live object?
 * A single unsigned compare against the in-use window rejects the vast majority
 * of stack/register words (integers, small counts, other segments). The upper
 * bound is alloc_top, the bump pointer: no object can exist at or above it, so
 * this is the tightest correct gate. We do NOT require w to be aligned: interior
 * pointers land anywhere. This is the first, cheapest gate of conservative scan. */
static inline int in_heap(uintptr_t w) {
    return w >= (uintptr_t)heap_lo && w < (uintptr_t)alloc_top;
}

/* Granule index of a heap address — its bit position in the mark bitmap. Object
 * bases are 16-aligned, so this is an exact shift, no division. */
static inline size_t granule_of(const char *p) {
    return (size_t)((p - heap_lo) / GC_GRANULE);
}

/* The mark-bitmap primitives: a bit per granule, packed 64 to a word. `g >> 6`
 * selects the word, `g & 63` the bit. These three are the classic bitmap idiom
 * and are exactly what asm/demo.c compiles to annotated assembly. */
static inline void mark_set(size_t g)  { mark_bits[g >> 6] |=  ((uint64_t)1 << (g & 63)); }
static inline int  mark_test(size_t g) { return (mark_bits[g >> 6] >> (g & 63)) & 1u; }

/* ===========================================================================
 * 6. RAW MEMORY FROM THE KERNEL  (mmap / mprotect / mremap)
 * ===========================================================================
 */

/* mmap an anonymous, private, demand-zeroed region for the collector's OWN
 * metadata (bitmap / table / mark stack). mmap(2) is syscall 9:
 *   rdi=addr(NULL: kernel picks), rsi=len, rdx=prot, r10=flags, r8=fd(-1), r9=off.
 * MAP_ANONYMOUS => not backed by a file; the pages read as zero on first touch. */
static void *xmmap(size_t len) {
    void *p = mmap(NULL, len, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) die("mmap failed");
    return p;
}

/* Reserve the heap's virtual window with PROT_NONE: addresses are claimed but no
 * accessible pages exist yet, so this is free in RAM terms. Returns the base. */
static char *reserve_heap(size_t len) {
    void *p = mmap(NULL, len, PROT_NONE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) die("heap reserve failed");
    return (char *)p;
}

/* Commit up to `need` more bytes past commit_top by making them readable/
 * writable. mprotect(2) is syscall 10 (rdi=addr, rsi=len, rdx=prot); it flips the
 * protection on whole pages of the existing PROT_NONE reservation, so the pages
 * become real (zero-filled on first touch). Returns 0 on success, -1 if we would
 * run past the reserve (true OOM). */
static int commit_at_least(size_t need) {
    if ((size_t)(commit_top - alloc_top) >= need)
        return 0;                       /* already enough committed head-room     */

    size_t want = align_up(need, GC_COMMIT_CHUNK);
    if ((size_t)(heap_reserved_end - commit_top) < want)
        want = (size_t)(heap_reserved_end - commit_top);   /* clamp to reserve    */
    if (want < need)
        return -1;                      /* reserve exhausted: cannot satisfy      */

    if (mprotect(commit_top, want, PROT_READ | PROT_WRITE) != 0)
        return -1;
    commit_top += want;
    stats.heap_committed = (size_t)(commit_top - heap_lo);
    return 0;
}

/* Grow an mmap'd metadata region to `new_len` bytes, preserving contents.
 * mremap(2) (Linux; syscall 25) can often extend a mapping in place, and with
 * MREMAP_MAYMOVE it relocates + copies if it must — either way the old data is
 * carried over, which is why we can hold the returned pointer in a global. */
static void *xmremap(void *old, size_t old_len, size_t new_len) {
    void *p = mremap(old, old_len, new_len, MREMAP_MAYMOVE);
    if (p == MAP_FAILED) die("mremap failed");
    return p;
}

/* ===========================================================================
 * 7. OBJECT TABLE
 * ===========================================================================
 */

/* Append a live descriptor, growing the mmap-backed table if needed. Growth uses
 * mremap (not realloc) to stay off libc's allocator. */
static void objs_push(char *base, size_t size, int atomic) {
    if (objs_len == objs_cap) {
        size_t old_bytes = objs_cap * sizeof(gc_obj);
        size_t new_cap   = objs_cap ? objs_cap * 2 : (GC_PAGE / sizeof(gc_obj));
        size_t new_bytes = new_cap * sizeof(gc_obj);
        objs = objs_cap ? (gc_obj *)xmremap(objs, old_bytes, new_bytes)
                        : (gc_obj *)xmmap(new_bytes);
        objs_cap = new_cap;
    }
    objs[objs_len].base   = base;
    objs[objs_len].size   = size;
    objs[objs_len].atomic = atomic;
    objs_len++;
}

/* Heapsort the object table by base address, ascending. We sort at the start of
 * every collection so the pointer lookup below can be a binary search. Heapsort,
 * not qsort: it is in-place, needs no recursion (so it cannot overflow the very
 * stack we are about to scan), and pulls in no libc. O(n log n). */
static void objs_sort(void) {
    size_t n = objs_len;
    /* build a max-heap (siftdown from the last internal node) */
    for (size_t start = n / 2; start-- > 0; ) {
        size_t root = start;
        for (;;) {
            size_t child = 2 * root + 1;
            if (child >= n) break;
            if (child + 1 < n && objs[child].base < objs[child + 1].base) child++;
            if (objs[root].base >= objs[child].base) break;
            gc_obj t = objs[root]; objs[root] = objs[child]; objs[child] = t;
            root = child;
        }
    }
    /* pop the max to the end, shrink, re-heap */
    for (size_t end = n; end-- > 1; ) {
        gc_obj t = objs[0]; objs[0] = objs[end]; objs[end] = t;
        size_t root = 0;
        for (;;) {
            size_t child = 2 * root + 1;
            if (child >= end) break;
            if (child + 1 < end && objs[child].base < objs[child + 1].base) child++;
            if (objs[root].base >= objs[child].base) break;
            gc_obj s = objs[root]; objs[root] = objs[child]; objs[child] = s;
            root = child;
        }
    }
}

/* Binary search the (sorted) table for the object CONTAINING address w, allowing
 * interior pointers. We find the greatest base <= w, then verify w is inside that
 * object's extent. Returns the object index, or -1 if w hits a gap / free block.
 * This is the precise half of the pointer test; in_heap() was the cheap gate. */
static long obj_containing(uintptr_t w) {
    size_t lo = 0, hi = objs_len;      /* search [lo, hi)                         */
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if ((uintptr_t)objs[mid].base <= w) lo = mid + 1;
        else                                hi = mid;
    }
    if (lo == 0) return -1;            /* every base > w: not in any object       */
    size_t i = lo - 1;                /* greatest base <= w                       */
    if (w < (uintptr_t)objs[i].base + objs[i].size) return (long)i;
    return -1;                        /* falls past this object (in a gap)        */
}

/* ===========================================================================
 * 8. FREE LIST  (reuse of swept blocks)
 * ===========================================================================
 */

static void free_push(char *base, size_t size) {
    free_node *f = (free_node *)base;   /* store links inside the dead block       */
    f->size = size;
    f->next = free_list;
    free_list = f;
    stats.bytes_free += size;
}

/* First-fit: return a reclaimed block of at least `need` bytes and report its
 * true capacity in *cap_out, or NULL. We do not split (a teaching simplification
 * — noted in the README); the whole block is reused and its full capacity
 * becomes the new object's scannable size. */
static char *free_pop(size_t need, size_t *cap_out) {
    free_node **pp = &free_list;
    while (*pp) {
        if ((*pp)->size >= need) {
            free_node *f = *pp;
            *pp = f->next;
            stats.bytes_free -= f->size;
            *cap_out = f->size;
            return (char *)f;
        }
        pp = &(*pp)->next;
    }
    return NULL;
}

/* ===========================================================================
 * 9. LOW-LEVEL BLOCK ALLOCATION
 * ===========================================================================
 * Returns a GRANULE-aligned block of exactly `size` bytes (already rounded) with
 * its actual capacity in *cap_out. Tries the reuse free list first, then bumps
 * the pointer, committing more of the reserve as needed.
 */
static char *alloc_block(size_t size, size_t *cap_out) {
    char *reused = free_pop(size, cap_out);        /* cap_out = block's capacity   */
    if (reused) return reused;
    if (commit_at_least(size) != 0) return NULL;   /* OOM: reserve exhausted       */
    char *p = alloc_top;
    alloc_top += size;
    *cap_out = size;
    return p;
}

/* ===========================================================================
 * 10. ROOT / OBJECT SCANNING
 * ===========================================================================
 * mark_word(w): the heart of conservative marking. Given a machine word w:
 *   (1) cheap gate: is it inside the committed heap window?      (in_heap)
 *   (2) precise:    does it fall inside a live object?           (obj_containing)
 *   (3) is that object already marked?                           (mark_test)
 *   (4) if not: set its mark bit and push it for interior scanning.
 * Steps 1-4 are the exact logic asm/demo.c isolates for annotation.
 */
static void mark_stack_push(uint32_t idx) {
    if (mark_sp == mark_stack_cap) {
        size_t old = mark_stack_cap * sizeof(uint32_t);
        size_t nc  = mark_stack_cap ? mark_stack_cap * 2 : (GC_PAGE / sizeof(uint32_t));
        mark_stack = mark_stack_cap ? (uint32_t *)xmremap(mark_stack, old, nc * sizeof(uint32_t))
                                    : (uint32_t *)xmmap(nc * sizeof(uint32_t));
        mark_stack_cap = nc;
    }
    mark_stack[mark_sp++] = idx;
}

static void mark_word(uintptr_t w) {
    if (!in_heap(w)) return;                 /* (1) not a heap address at all       */
    long i = obj_containing(w);
    if (i < 0) return;                       /* (2) points into a gap/free block    */
    size_t g = granule_of(objs[i].base);
    if (mark_test(g)) return;                /* (3) already reached this cycle      */
    mark_set(g);                             /* (4) prove it live...                */
    mark_stack_push((uint32_t)i);            /*     ...and remember to scan it      */
}

/* Scan a raw memory range [lo, hi) for pointer candidates. We step by 8 bytes
 * (sizeof(void*)) and assume pointers are word-aligned — true for every pointer
 * the C compiler stores on x86-64. Reading each word and testing it is the same
 * operation whether the range is the stack, the data segment, or an object's
 * interior. lo is rounded up to an 8-byte boundary first. */
static void scan_range(const char *lo, const char *hi) {
    uintptr_t p = align_up((uintptr_t)lo, sizeof(void *));
    for (; p + sizeof(void *) <= (uintptr_t)hi; p += sizeof(void *))
        mark_word(*(const uintptr_t *)p);
}

/* Drain the mark stack: pop a marked object and scan its interior for more
 * pointers, marking and pushing anything newly found. This is the transitive
 * closure of reachability, done ITERATIVELY. We use an explicit stack precisely
 * because recursion here could be as deep as the longest pointer chain and would
 * overflow the C stack — a collector must never crash the program it serves.
 * "atomic" objects are skipped: they were promised to hold no pointers. */
static void mark_drain(void) {
    while (mark_sp) {
        uint32_t i = mark_stack[--mark_sp];
        if (objs[i].atomic) continue;
        scan_range(objs[i].base, objs[i].base + objs[i].size);
    }
}

/* ===========================================================================
 * 11. FINDING THE STACK BOTTOM  (the /proc/self/stat trick)
 * ===========================================================================
 * To scan the stack we need both ends. The current top (lowest address) we read
 * from rsp at collection time. The bottom (highest address) is fixed for the
 * main thread and the kernel records it: field 28 of /proc/self/stat is
 * `startstack`, the address of the start of the stack. Parsing it is robust
 * across libc versions. The comm field (field 2) is wrapped in parentheses and
 * may itself contain spaces or ')', so we scan to the LAST ')' and count fields
 * from there. Fallback: the caller's frame address, good enough if gc_init() is
 * called early from main.
 */
static char *find_stack_bottom(void) {
    int fd = open("/proc/self/stat", O_RDONLY);   /* open(2): rdi=path, rsi=flags */
    if (fd < 0) return NULL;

    char buf[1024];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);   /* read(2): syscall 0           */
    close(fd);
    if (n <= 0) return NULL;
    buf[n] = '\0';

    /* skip past the "(comm)" field: find the last ')' in the buffer */
    char *p = buf;
    char *last_paren = NULL;
    for (char *q = buf; *q; q++) if (*q == ')') last_paren = q;
    if (!last_paren) return NULL;
    p = last_paren + 1;

    /* After ')' the fields are (3) state (4) ppid ... (28) startstack. So from
     * here startstack is the (28 - 2) = 26th whitespace-separated token. */
    int field = 3;               /* the token right after ')' is field 3         */
    while (*p == ' ') p++;
    for (; field < 28 && *p; field++) {
        while (*p && *p != ' ') p++;   /* skip this token                        */
        while (*p == ' ') p++;         /* skip spaces                            */
    }
    if (field != 28 || !*p) return NULL;

    /* parse the unsigned decimal startstack value */
    unsigned long v = 0;
    int any = 0;
    for (; *p >= '0' && *p <= '9'; p++) { v = v * 10u + (unsigned)(*p - '0'); any = 1; }
    if (!any) return NULL;
    return (char *)(uintptr_t)v;
}

/* ===========================================================================
 * 12. THE COLLECTOR PROPER
 * ===========================================================================
 */

/* Sweep: every object whose mark bit is clear is unreachable. Reclaim it to the
 * free list and drop its descriptor; keep the survivors (compacting the table in
 * place). We DO NOT clear mark bits here — the next collection wipes the whole
 * bitmap up front, which is cheaper than clearing bit by bit. */
static size_t sweep(void) {
    size_t reclaimed = 0, live_bytes = 0, w = 0;
    for (size_t r = 0; r < objs_len; r++) {
        size_t g = granule_of(objs[r].base);
        if (mark_test(g)) {
            live_bytes += objs[r].size;
            objs[w++] = objs[r];              /* keep: compact toward the front    */
        } else {
            free_push(objs[r].base, objs[r].size);   /* dead: recycle the memory   */
            reclaimed += objs[r].size;
        }
    }
    objs_len = w;
    stats.bytes_live   = live_bytes;
    stats.objects_live = objs_len;
    return reclaimed;
}

/* The public collection entry point. Everything about correctness lives in the
 * ORDER and the register spill here, so read the comments closely. */
size_t gc_collect(void) {
    if (!gc_ready) return 0;

    /* -- 0. Snapshot the stack top NOW, before we perturb the stack further. rsp
     *       is the lowest live address; the stack occupies [rsp, stack_bottom).
     *       Reading it with inline asm (rather than a local's address) guarantees
     *       we include every local of THIS frame, among them `regs` below. -- */
    char *stack_top;
    __asm__ volatile ("movq %%rsp, %0" : "=r"(stack_top));

    /* -- 1. Spill callee-saved registers to memory. setjmp writes rbx, rbp,
     *       r12-r15 (and rsp/rip) into `regs`. Those callee-saved registers are
     *       the ones the compiler may use to hold a live pointer ACROSS our call,
     *       so a root could exist only there. Caller-saved registers holding live
     *       roots were already spilled to the caller's stack frame by the ABI when
     *       it called gc_collect(), and that frame is inside [rsp, stack_bottom),
     *       so scanning the stack covers them. Between setjmp and the stack scan,
     *       every register root is therefore also in memory we are about to read.
     *       (glibc mangles the saved rsp/rip with a pointer guard, so those two
     *       slots read as garbage and simply fail in_heap — harmless.) -- */
    jmp_buf regs;
    (void)setjmp(regs);                   /* spills rbx,rbp,r12-r15 into `regs`     */
    __asm__ volatile ("" ::: "memory");   /* barrier: keep the spill before scans   */

    /* -- 2. Fresh cycle: wipe the mark bitmap over the in-use granule range
     *       (heap_lo .. alloc_top). Cheaper than clearing per-object in sweep. -- */
    size_t used_granules = granule_of(alloc_top);
    size_t clear_words = (used_granules + 63) / 64;
    if (clear_words > mark_words) clear_words = mark_words;
    memset(mark_bits, 0, clear_words * sizeof(uint64_t));
    mark_sp = 0;

    /* -- 3. Sort the object table so obj_containing() is a binary search that
     *       also resolves interior pointers. Objects never move, so one sort per
     *       collection suffices. -- */
    objs_sort();

    /* -- 4. Scan every root source. Order does not matter; each scan_range just
     *       seeds the mark stack. -- */
    scan_range((char *)&regs, (char *)&regs + sizeof(regs));  /* CPU registers      */
    scan_range(stack_top, stack_bottom);                      /* the whole stack    */
    scan_range(__data_start, _end);                           /* globals: data+bss  */

    /* -- 5. Transitively mark everything reachable from those roots. -- */
    mark_drain();

    /* -- 6. Reclaim the unmarked. -- */
    size_t reclaimed = sweep();

    /* -- 7. Retune the trigger: aim to collect again after the heap grows by
     *       about the current live size, keeping GC work proportional to live
     *       data rather than to garbage. -- */
    bytes_since_gc = 0;
    gc_threshold = stats.bytes_live * 2;
    if (gc_threshold < GC_INITIAL_THRESHOLD) gc_threshold = GC_INITIAL_THRESHOLD;

    stats.collections++;
    stats.last_reclaimed = reclaimed;
    return reclaimed;
}

/* ===========================================================================
 * 13. ALLOCATION FRONT DOOR
 * ===========================================================================
 */
static void *gc_alloc(size_t nbytes, int atomic) {
    if (!gc_ready) gc_init();
    if (nbytes == 0) nbytes = 1;

    /* Round the request up to a whole number of granules so every base stays
     * 16-aligned and every mark bit covers exactly one object's worth of grain. */
    size_t need = align_up(nbytes, GC_GRANULE);
    /* A free block must be able to hold its own {next,size} link (16 bytes). */
    if (need < sizeof(free_node)) need = sizeof(free_node);

    /* Allocation pressure crossed the trigger: collect BEFORE carving the new
     * block, so the collector sees a consistent heap and the not-yet-returned
     * object does not need special handling (it does not exist yet). The caller's
     * existing roots are safely in registers/stack, which we scan. */
    if (bytes_since_gc >= gc_threshold)
        gc_collect();

    size_t cap;
    char *base = alloc_block(need, &cap);
    if (!base) {
        /* Committed reserve is full: try one collection to reclaim space, then
         * retry once. If it still fails, this is a genuine OOM. */
        gc_collect();
        base = alloc_block(need, &cap);
        if (!base) { ewrite("gc: out of reserved address space\n"); return NULL; }
    }

    /* Zero the block. Fresh committed pages are already zero, but a REUSED free
     * block still holds the dead object's bytes (including our free_node links),
     * which could look like stray pointers on a later scan; zeroing removes that
     * false-retention source and matches Boehm's GC_malloc contract. */
    memset(base, 0, cap);

    objs_push(base, cap, atomic);
    bytes_since_gc      += cap;
    stats.total_allocated += cap;
    return base;
}

void *gc_malloc(size_t n)        { return gc_alloc(n, 0); }
void *gc_malloc_atomic(size_t n) { return gc_alloc(n, 1); }

/* ===========================================================================
 * 14. INITIALIZATION
 * ===========================================================================
 */
void gc_init(void) {
    if (gc_ready) return;

    /* Reserve the contiguous heap window (PROT_NONE — no RAM cost yet). */
    heap_lo          = reserve_heap(GC_RESERVE);
    heap_reserved_end = heap_lo + GC_RESERVE;
    commit_top       = heap_lo;
    alloc_top        = heap_lo;
    stats.heap_reserved = GC_RESERVE;

    /* One mark bit per granule of the whole reserve, packed 64 bits per word.
     * mmap gives demand-zeroed pages, so only the parts we touch cost RAM. */
    size_t granules = GC_RESERVE / GC_GRANULE;
    mark_words = (granules + 63) / 64;
    mark_bits  = (uint64_t *)xmmap(mark_words * sizeof(uint64_t));

    /* Find the stack bottom (highest address). Prefer the kernel's record; fall
     * back to this function's caller frame if /proc is unavailable. */
    stack_bottom = find_stack_bottom();
    if (!stack_bottom)
        stack_bottom = (char *)__builtin_frame_address(0);

    gc_ready = 1;
}

/* ===========================================================================
 * 15. DIAGNOSTICS
 * ===========================================================================
 */
void gc_get_stats(struct gc_stats *out) { *out = stats; }

void gc_dump(const char *label) {
    ewrite("gc["); ewrite(label ? label : "?"); ewrite("]  ");
    ewrite("live="); ewrite_hex(stats.bytes_live);
    ewrite(" objs="); ewrite_hex(stats.objects_live);
    ewrite(" committed="); ewrite_hex(stats.heap_committed);
    ewrite(" free="); ewrite_hex(stats.bytes_free);
    ewrite(" gcs="); ewrite_hex(stats.collections);
    ewrite("\n");
}

/* ===========================================================================
 * 16. OPTIONAL LD_PRELOAD INTERPOSERS
 * ===========================================================================
 * Compiling with -DGC_INTERPOSE exports strong malloc/free/... so the dynamic
 * linker routes a program's allocations through the collector. free() becomes a
 * NO-OP — the whole point is that the GC reclaims memory, not the program — and
 * gc_malloc's automatic-collection trigger keeps the heap bounded. This is how
 * Boehm's libgc can be preloaded. It is intentionally simple: it does not chase
 * memory allocated inside libc before we were loaded, and roots living in other
 * shared libraries' data segments are not scanned (only the main program's
 * data/bss is). Treat it as a demonstration, not a drop-in for arbitrary
 * programs — see the README's honesty note.
 */
#ifdef GC_INTERPOSE

void *malloc(size_t n)              { return gc_malloc(n); }
void  free(void *p)                 { (void)p; /* reclaimed by gc_collect */ }
void *calloc(size_t nm, size_t sz)  {
    size_t total;
    if (__builtin_mul_overflow(nm, sz, &total)) return NULL;
    return gc_malloc(total);        /* gc_malloc already zeroes                    */
}
void *realloc(void *p, size_t n) {
    if (!p) return gc_malloc(n);
    if (n == 0) return NULL;
    void *q = gc_malloc(n);
    if (!q) return NULL;
    /* We do not know the old size precisely from here; the object table does. Look
     * it up so we copy the right amount and never read past the old block. */
    long i = obj_containing((uintptr_t)p);
    size_t oldsz = (i >= 0) ? objs[i].size : 0;
    memcpy(q, p, oldsz < n ? oldsz : n);
    return q;
}

#endif /* GC_INTERPOSE */
