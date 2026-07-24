/* ===========================================================================
 * demo.c — the collector's pointer-candidate test and mark-bitmap logic,
 * extracted self-contained for annotated assembly.
 * ===========================================================================
 *
 * This file includes NO headers and declares its own types, so
 * `clang --target=x86_64-pc-linux-gnu -S` turns it into clean Linux/SysV
 * assembly with nothing from libc in the way. The real collector (../gc.c)
 * cannot be compiled to asm standalone — it needs the Linux headers for
 * mmap/mprotect/setjmp — so we lift out the part that is 100% register-and-
 * pointer math, which is also the most instructive to read as assembly: the
 * hot path of every conservative collection.
 *
 * Each routine below is a faithful copy of the same-named logic in gc.c:
 *
 *   1. in_heap        — the cheap range gate: is a word a heap address at all?
 *   2. granule_of     — address -> mark-bitmap bit index (a shift, no divide).
 *   3. mark_set/test  — the classic word[i>>6] |= 1<<(i&63) bitmap primitives.
 *   4. obj_containing — binary search of the sorted object table, INTERIOR-
 *                       pointer aware (finds the object a mid-object pointer is
 *                       inside of).
 *   5. mark_word      — the whole conservative-marking decision, tying 1-4
 *                       together: gate, locate, test-and-set, report "newly
 *                       marked". Watch how the compiler fuses these.
 * ===========================================================================
 */

/* Our own fixed-width types (LP64: long and pointers are 64-bit on x86-64). */
typedef unsigned long      u64;
typedef unsigned long      uptr;    /* stand-in for uintptr_t, no <stdint.h>    */
typedef unsigned long      usize;   /* stand-in for size_t,     no <stddef.h>   */

#define GC_GRANULE  16u

/* Same descriptor the real object table holds: base + size (+ atomic flag). */
typedef struct gc_obj {
    char  *base;
    usize  size;
    int    atomic;
} gc_obj;

/* ---- 1. the cheap range gate ----------------------------------------------
 * Could w be a pointer into the in-use heap [lo, hi)? One pair of unsigned
 * compares throws out almost every non-pointer word. No alignment requirement,
 * because interior pointers can land on any byte. */
int in_heap(uptr w, uptr lo, uptr hi) {
    return w >= lo && w < hi;
}

/* ---- 2. address -> mark-bitmap bit index ----------------------------------
 * Object bases are 16-aligned, so (p - heap_lo) / 16 is an exact right shift by
 * 4 — the compiler emits `shr $4`, never a divide. */
usize granule_of(char *p, char *heap_lo) {
    return (usize)(p - heap_lo) / GC_GRANULE;
}

/* ---- 3. the mark-bitmap primitives ----------------------------------------
 * One bit per granule, packed 64 to a 64-bit word. `g >> 6` picks the word,
 * `g & 63` the bit. These two functions are the entire "mark memory" mechanism
 * of a bitmap collector; everything else just decides which bit to touch. */
void mark_set(u64 *bitmap, usize g) {
    bitmap[g >> 6] |= (u64)1 << (g & 63);
}
int mark_test(u64 *bitmap, usize g) {
    return (bitmap[g >> 6] >> (g & 63)) & 1u;
}

/* ---- 4. locate the object containing an address (interior-aware) -----------
 * Binary search a table sorted by base for the greatest base <= w, then confirm
 * w falls within that object's extent. Returns the index, or -1 for a word that
 * lands in a gap / freed block. This is what upgrades a raw "in the heap" word
 * into "inside THIS object", and what makes a pointer to the middle of an array
 * keep the array alive. */
long obj_containing(uptr w, const gc_obj *objs, usize n) {
    usize lo = 0, hi = n;                 /* search the half-open range [lo, hi) */
    while (lo < hi) {
        usize mid = lo + (hi - lo) / 2;
        if ((uptr)objs[mid].base <= w) lo = mid + 1;
        else                           hi = mid;
    }
    if (lo == 0) return -1;               /* every base > w: not in any object   */
    usize i = lo - 1;                     /* greatest base <= w                   */
    if (w < (uptr)objs[i].base + objs[i].size) return (long)i;
    return -1;                            /* past the end of that object (a gap)  */
}

/* ---- 5. the conservative-marking decision ---------------------------------
 * The complete hot path for one candidate word w:
 *   gate on the heap range; locate the containing object; if found and not
 *   already marked, set its bit and report 1 ("newly marked, go scan it").
 * The caller pushes the returned index onto the mark stack. This function is the
 * distilled essence of the whole collector and the star of the annotation. */
int mark_word(uptr w, char *heap_lo, char *heap_hi,
              const gc_obj *objs, usize n, u64 *bitmap, long *out_index) {
    if (!in_heap(w, (uptr)heap_lo, (uptr)heap_hi)) return 0;   /* not a heap ptr  */
    long i = obj_containing(w, objs, n);
    if (i < 0) return 0;                                       /* gap/free block  */
    usize g = granule_of(objs[i].base, heap_lo);
    if (mark_test(bitmap, g)) return 0;                        /* already marked  */
    mark_set(bitmap, g);                                       /* prove it live   */
    *out_index = i;                                            /* hand back which */
    return 1;
}
