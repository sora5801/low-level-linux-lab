/* ===========================================================================
 * gctest.c — a driver that exercises the conservative collector and shows,
 * on cue, each behaviour the README claims. It uses the gc_* API directly
 * (it is NOT the LD_PRELOAD build), and it uses libc printf freely because it
 * is the *application*, not part of the collector.
 * ===========================================================================
 *
 * The hard part of demonstrating a conservative GC reliably is the flip side of
 * what makes it conservative: stale copies of a pointer can linger in dead stack
 * frames or registers and accidentally keep an object alive. Whenever a test
 * WANTS an object reclaimed, we first call scrub_stack() to overwrite the slice
 * of stack that previous calls used, wiping those stale bit patterns before we
 * collect. Whenever a test wants an object KEPT, we simply hold a live root and
 * do not scrub. Read each test's comment for which case it is.
 * ===========================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "gc.h"

/* A pointer-bearing node: the collector must scan `next` to keep a list alive. */
typedef struct node {
    struct node *next;
    long         value;
    char         pad[16];   /* make each node a couple of granules, easier to see */
} node;

/* Overwrite ~16 KiB of stack with zeros. Called right before a collection that
 * is SUPPOSED to reclaim, to erase stale pointer bit-patterns that earlier
 * (now-returned) frames left behind in memory the scanner will look at. The
 * `volatile` stops the compiler from optimising the writes away. */
static void scrub_stack(void) {
    volatile char buf[16 * 1024];
    for (size_t i = 0; i < sizeof(buf); i++) buf[i] = 0;
}

/* ---------------------------------------------------------------------------
 * Test 1: reachability. A list rooted in a live local survives collection;
 * once the root is dropped (and stale copies scrubbed) the whole list dies.
 * --------------------------------------------------------------------------- */
static node *build_list(int n) {
    node *head = NULL;
    for (int i = 0; i < n; i++) {
        node *p = (node *)gc_malloc(sizeof(node));   /* scanned: holds a pointer */
        p->next = head;
        p->value = i;
        head = p;
    }
    return head;   /* only the head escapes; the rest are reachable through it   */
}

static void test_reachability(void) {
    struct gc_stats a, b, c;

    node *list = build_list(1000);       /* `list` is a live root in this frame  */
    gc_collect();                        /* everything reachable from it survives */
    gc_get_stats(&a);
    printf("  [reach] built 1000-node list, kept root -> live objects = %zu\n",
           a.objects_live);

    /* Walk the list to prove it is intact after a collection. */
    long sum = 0; int count = 0;
    for (node *p = list; p; p = p->next) { sum += p->value; count++; }
    printf("  [reach] walked survivors: count=%d sum=%ld (expect 1000, 499500)\n",
           count, sum);

    gc_get_stats(&b);
    list = NULL;                         /* drop the only root...                */
    scrub_stack();                       /* ...and wipe stale copies from stack  */
    size_t reclaimed = gc_collect();
    gc_get_stats(&c);
    printf("  [reach] dropped root + collected: live objects %zu -> %zu, "
           "reclaimed %zu bytes\n", b.objects_live, c.objects_live, reclaimed);
}

/* ---------------------------------------------------------------------------
 * Test 2: a root that lives ONLY in a register/stack local. No global, no heap
 * link points at it — the object is reachable purely because the scanner reads
 * the machine registers (spilled by setjmp) and the stack. This is the whole
 * reason gc_collect() does the register spill.
 * --------------------------------------------------------------------------- */
static void test_stack_root(void) {
    /* `secret` is a bare local. If the scanner did not read the stack/registers,
     * this object would be wrongly reclaimed and the write below would corrupt
     * freed memory. */
    node *secret = (node *)gc_malloc(sizeof(node));
    secret->value = 0xC0FFEE;

    /* Allocate a wave of garbage to force at least one automatic collection while
     * `secret` is only held in a local. */
    for (int i = 0; i < 5000; i++) { node *g = (node *)gc_malloc(sizeof(node)); g->value = i; (void)g; }

    gc_collect();                        /* explicit collection too              */
    /* If `secret` survived (it must), this write is safe and reads back. */
    secret->value = 0xBEEF;
    printf("  [stack] object rooted only in a local survived; value=0x%lX\n",
           secret->value);
}

/* ---------------------------------------------------------------------------
 * Test 3: interior pointers. We keep only a pointer into the MIDDLE of an array;
 * the base pointer never escapes. A conservative collector that supports interior
 * pointers must still keep the whole object alive.
 * --------------------------------------------------------------------------- */
static long *g_interior;                 /* global root (scanned as data/bss)    */

static void stash_interior(void) {
    long *arr = (long *)gc_malloc(100 * sizeof(long));  /* base is local only     */
    for (int i = 0; i < 100; i++) arr[i] = 1000 + i;
    g_interior = &arr[50];               /* ONLY an interior pointer escapes      */
    arr = NULL;
}

static void test_interior(void) {
    stash_interior();
    scrub_stack();                       /* erase any stale copy of the base ptr  */
    gc_collect();                        /* survives ONLY via the interior root   */
    /* Read back through the interior pointer and reach both ends of the object. */
    printf("  [interior] kept &arr[50]; arr[50]=%ld arr[0-via-mid]=%ld arr[99]=%ld\n",
           g_interior[0], g_interior[-50], g_interior[49]);
}

/* ---------------------------------------------------------------------------
 * Test 4: atomic (pointer-free) allocation is not scanned. We hide an object's
 * address inside an ATOMIC block as raw bytes. Because the collector never scans
 * atomic blocks, that hidden reference does NOT keep the object alive — so it is
 * reclaimed. Contrast this with a normal block, which would retain it.
 * --------------------------------------------------------------------------- */
static void *g_atomic_holder;            /* global root: keeps the atomic block   */

static void hide_in_atomic(void) {
    node *hidden = (node *)gc_malloc(sizeof(node));
    hidden->value = 0x1234;

    long *blk = (long *)gc_malloc_atomic(4 * sizeof(long));  /* not scanned        */
    blk[0] = (long)(size_t)hidden;       /* store address as data, not as a root  */
    g_atomic_holder = blk;               /* keep the atomic block alive           */
    hidden = NULL;
}

static void test_atomic(void) {
    struct gc_stats before, after;
    hide_in_atomic();
    scrub_stack();
    gc_get_stats(&before);
    gc_collect();
    gc_get_stats(&after);
    printf("  [atomic] address hidden in an atomic block is NOT a root: "
           "live objects %zu -> %zu (the hidden node was reclaimed)\n",
           before.objects_live, after.objects_live);
}

/* ---------------------------------------------------------------------------
 * Test 5: bounded memory under churn. Allocate far more than fits, but keep a
 * bounded live set; automatic collection must keep committed memory small.
 * --------------------------------------------------------------------------- */
static void test_churn(void) {
    const int rounds = 200, per = 3000;
    for (int r = 0; r < rounds; r++)
        for (int i = 0; i < per; i++) {
            node *p = (node *)gc_malloc(sizeof(node));
            p->value = i;                /* touch it, then forget it (garbage)    */
            (void)p;
        }
    struct gc_stats s; gc_get_stats(&s);
    printf("  [churn] allocated %zu bytes total; committed only %zu bytes; "
           "%zu collections\n",
           s.total_allocated, s.heap_committed, s.collections);
}

int main(void) {
    /* MUST be first: capture the stack bottom from as high a frame as possible so
     * every later frame is within the scanned range. */
    gc_init();

    printf("conservative mark-and-sweep GC — demo\n");
    printf("-------------------------------------\n");

    printf("[1] reachability (live root keeps it; dropped root frees it)\n");
    test_reachability();

    printf("[2] register/stack-only root\n");
    test_stack_root();

    printf("[3] interior pointer keeps the whole object alive\n");
    test_interior();

    printf("[4] atomic (pointer-free) blocks are not scanned\n");
    test_atomic();

    printf("[5] bounded memory under allocation churn\n");
    test_churn();

    gc_dump("final");
    printf("done.\n");
    return 0;
}
