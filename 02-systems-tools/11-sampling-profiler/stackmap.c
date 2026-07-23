/* ===========================================================================
 * stackmap.c — the fold table: symbolized callchain -> count.
 * ===========================================================================
 *
 * DATA STRUCTURE
 * --------------
 * A fixed-bucket hash table with separate chaining. The key is the collapsed
 * stack STRING (e.g. "main;wl_compute;wl_heavy_a;wl_hash"); the value is a
 * count. We hash the string with FNV-1a (the same hash demonstrated in
 * asm/demo.c) and chain collisions in a linked list. A string key is the natural
 * choice here because two syntactically identical stacks must fold together even
 * when their raw return addresses differ slightly (two samples a few bytes apart
 * inside the same function), and symbolizing to names erases that difference.
 *
 * OWNERSHIP / ALLOCATION (called out per the repo's rules)
 * --------------------------------------------------------
 *   * The bucket array is one calloc in stackmap_new (NBUCKETS pointers, zeroed).
 *   * Each distinct stack costs one malloc(node) + one strdup(key). The MAP OWNS
 *     both and frees them in stackmap_free. Callers never free keys.
 *   * stackmap_add allocates nothing on the hot path once a stack is known — it
 *     just walks a short chain and bumps a counter. New stacks are rare after
 *     warmup, so the allocator is off the steady-state path.
 * =========================================================================== */
#include "stackmap.h"
#include "symbolize.h"

#include <stdlib.h>     /* malloc, calloc, free, qsort                          */
#include <string.h>     /* strcmp, strlen, memcpy, strdup                       */

/* Power of two so the hash-to-bucket step is a mask, not a modulo (see the
 * `& (NBUCKETS-1)` below — the exact trick asm/demo.c's rb_offset shows). */
#define NBUCKETS 8192u

/* Upper bound on a collapsed key. Deep stacks of long C++-mangled names could in
 * principle exceed this; we truncate rather than grow, because a truncated hot
 * stack is still a useful (if slightly merged) flame-graph frame, and bounding
 * the buffer keeps stackmap_add allocation-free. */
#define KEY_MAX 4096
#define NAME_MAX 256    /* per-frame symbol name cap                            */

struct node {
    char        *key;   /* strdup'd collapsed stack; owned by the map           */
    uint64_t     count; /* how many samples folded to this exact stack          */
    struct node *next;  /* chain within the bucket                              */
};

struct stackmap {
    struct node  *buckets[NBUCKETS];
    unsigned long total;    /* samples folded (sum of all counts)               */
    unsigned long unique;   /* distinct stacks (number of nodes)                */
};

/* FNV-1a over a NUL-terminated string. Identical mixing to asm/demo.c's fnv1a:
 * xor the byte in, then multiply by the 64-bit FNV prime. */
static uint64_t fold_hash(const char *s)
{
    uint64_t h = 14695981039346656037ULL;       /* offset basis                 */
    for (; *s; s++) {
        h ^= (unsigned char)*s;
        h *= 1099511628211ULL;                   /* FNV prime                    */
    }
    return h;
}

struct stackmap *stackmap_new(void)
{
    /* calloc zeroes the bucket pointers to NULL (empty chains) for us. */
    return calloc(1, sizeof(struct stackmap));
}

/* Build the collapsed key for a callchain and upsert it. `sm` is void* so this
 * function's type matches prof_stack_fn exactly and can be handed to a backend
 * as the callback with no wrapper. */
void stackmap_add(void *smv, const uint64_t *ips, int n)
{
    struct stackmap *sm = smv;
    if (n <= 0)
        return;

    /* --- Fold: emit frames OUTERMOST-first (ips[n-1] down to ips[0]), joined by
     * ';'. Collapsed-stack format lists the root on the left and the leaf on the
     * right, which is the opposite of our innermost-first callchain, so we walk
     * it in reverse. Everything is bounds-checked against KEY_MAX so a
     * pathologically deep stack truncates instead of overflowing. */
    char key[KEY_MAX];
    size_t len = 0;
    key[0] = '\0';

    for (int i = n - 1; i >= 0; i--) {
        char name[NAME_MAX];
        sym_name(ips[i], name, sizeof name);     /* address -> function name     */

        if (len && len + 1 < sizeof key)         /* separator between frames     */
            key[len++] = ';';

        size_t nl = strlen(name);
        if (len + nl >= sizeof key)              /* clamp to remaining space     */
            nl = sizeof key - 1 - len;
        memcpy(key + len, name, nl);
        len += nl;
        key[len] = '\0';
    }

    sm->total++;

    /* --- Upsert: hash to a bucket, scan the chain for an exact match. */
    uint64_t h = fold_hash(key) & (NBUCKETS - 1);
    for (struct node *p = sm->buckets[h]; p; p = p->next) {
        if (strcmp(p->key, key) == 0) {
            p->count++;                          /* seen before: just count it   */
            return;
        }
    }

    /* --- New stack: allocate a node and take ownership of a copy of the key.
     * On OOM we simply drop the sample (a profiler must never crash the run it
     * is measuring); total was already counted, so the drop is visible as a
     * small discrepancy rather than a lie. */
    struct node *nn = malloc(sizeof *nn);
    if (!nn)
        return;
    nn->key = strdup(key);
    if (!nn->key) { free(nn); return; }
    nn->count = 1;
    nn->next  = sm->buckets[h];                  /* push onto the chain head     */
    sm->buckets[h] = nn;
    sm->unique++;
}

unsigned long stackmap_total(const struct stackmap *sm)  { return sm->total;  }
unsigned long stackmap_unique(const struct stackmap *sm) { return sm->unique; }

/* qsort comparator: count DESC, then key ASC for a stable, readable order. */
static int by_count_desc(const void *a, const void *b)
{
    const struct node *x = *(const struct node *const *)a;
    const struct node *y = *(const struct node *const *)b;
    if (x->count != y->count)
        return (x->count < y->count) ? 1 : -1;   /* larger count first           */
    return strcmp(x->key, y->key);               /* tie-break for determinism    */
}

void stackmap_print_folded(struct stackmap *sm, FILE *out)
{
    if (sm->unique == 0)
        return;

    /* Gather every node into a flat array so we can sort. One malloc, freed at
     * the end; the nodes themselves stay owned by the map. */
    struct node **all = malloc(sm->unique * sizeof *all);
    if (!all) {
        /* Sorting is a nicety, not correctness — if we cannot allocate the index
         * we still emit every stack, just unsorted. */
        for (unsigned b = 0; b < NBUCKETS; b++)
            for (struct node *p = sm->buckets[b]; p; p = p->next)
                fprintf(out, "%s %llu\n", p->key, (unsigned long long)p->count);
        return;
    }

    size_t k = 0;
    for (unsigned b = 0; b < NBUCKETS; b++)
        for (struct node *p = sm->buckets[b]; p; p = p->next)
            all[k++] = p;

    qsort(all, k, sizeof *all, by_count_desc);

    for (size_t i = 0; i < k; i++)
        fprintf(out, "%s %llu\n", all[i]->key, (unsigned long long)all[i]->count);

    free(all);
}

void stackmap_free(struct stackmap *sm)
{
    if (!sm)
        return;
    for (unsigned b = 0; b < NBUCKETS; b++) {
        struct node *p = sm->buckets[b];
        while (p) {
            struct node *next = p->next;
            free(p->key);                        /* the strdup'd key             */
            free(p);                             /* the node                     */
            p = next;
        }
    }
    free(sm);
}
