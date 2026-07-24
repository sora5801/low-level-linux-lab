/* ===========================================================================
 * hashring.c — the consistent-hash ring (policy "hash").
 * ===========================================================================
 *
 * WHY CONSISTENT HASHING (the whole point of this file)
 * -----------------------------------------------------
 * Sticky routing means "the same client always lands on the same backend" — it
 * keeps a client's warmed TLS session, cache, or upstream connection state on
 * one server. The naive way, `backend = hash(client) % N`, is sticky only while
 * N is constant: the instant a backend dies (N: 4 -> 3) or you add one (4 -> 5),
 * `hash % N` changes for almost every client, so almost everyone is re-pinned to
 * a different backend and all that warmed state is thrown away at once.
 *
 * Consistent hashing removes the dependence on N. Picture a ring of all 2^32
 * hash values. Each backend is placed at several points on the ring (its
 * "virtual nodes"). A client is hashed to a point too, and served by the FIRST
 * backend point you meet walking clockwise. Now remove a backend: only the
 * clients that fell on ITS arcs move — to the next backend clockwise — and every
 * other client is undisturbed. On average only K/N keys remap when a backend
 * joins or leaves, instead of nearly all K of them.
 *
 * VIRTUAL NODES (why RING_VNODES points per backend, not one)
 * -----------------------------------------------------------
 * A single point per backend would split the ring into N arcs of wildly unequal
 * length (hashing is random, not fair), so one backend might get 3x the load of
 * another. Hashing each backend under many labels ("host:port#0" .. "#159")
 * scatters ~160 tiny arcs per backend around the ring; by the law of large
 * numbers the total arc length per backend evens out, and when a backend leaves
 * its 160 little arcs are inherited by MANY different survivors rather than
 * dumped entirely on one unlucky neighbour.
 *
 * We keep the vnodes in one array SORTED by hash, so "first point clockwise of
 * the key" is a binary search (ring_lookup) — O(log V) per incoming connection.
 * ===========================================================================
 */
#include <stdlib.h>     /* malloc, realloc, free, qsort                       */
#include <stdio.h>      /* snprintf                                           */
#include <string.h>     /* (implicit via snprintf)                            */

#include "lb.h"

/* FNV-1a: a fast non-cryptographic hash with good dispersion. We do not need
 * cryptographic strength, only that similar inputs (host:port#0, #1, #2, ...)
 * land at well-scattered ring positions and that client IPs spread evenly.
 *
 * FNV-1a per byte: XOR the byte into the accumulator, THEN multiply by the FNV
 * prime. The multiply is modulo 2^32 (it simply overflows a uint32_t), which is
 * why the output is already a coordinate on the 2^32 ring. Using this same hash
 * for both vnode labels and client IPs keeps them in one comparable space. */
uint32_t lb_hash(const void *data, size_t len)
{
    const unsigned char *p = (const unsigned char *)data;
    uint32_t h = 2166136261u;               /* FNV offset basis (the seed)     */
    for (size_t i = 0; i < len; i++) {
        h ^= (uint32_t)p[i];                /* XOR the byte in first ...        */
        h *= 16777619u;                     /* ... then multiply by FNV prime.  */
    }
    return h;
}

/* qsort comparator: order vnodes by ascending hash. We return -1/0/+1 via two
 * boolean subtractions rather than `(int)(a->hash - b->hash)`, because the
 * latter can overflow when two unsigned 32-bit hashes differ by more than
 * INT_MAX and would then sort backwards. This form is overflow-safe. */
static int rnode_cmp(const void *va, const void *vb)
{
    const struct rnode *a = (const struct rnode *)va;
    const struct rnode *b = (const struct rnode *)vb;
    return (a->hash > b->hash) - (a->hash < b->hash);
}

/* ---------------------------------------------------------------------------
 * ring_build — (re)populate `r` with the virtual nodes of every ELIGIBLE backend.
 *
 * Called at startup and again whenever eligibility changes (a health flip, or an
 * admin drain/undrain). Rebuilding the whole ring on a change is O(V log V) but V
 * is a few thousand and changes are rare, so simplicity wins over incremental
 * edits. Only eligible backends contribute vnodes, so a lookup can NEVER return a
 * down/draining backend — that is how ejection works for policy "hash".
 * --------------------------------------------------------------------------- */
void ring_build(struct ring *r, const struct backend *be, int nbe, int vnodes)
{
    /* How many vnodes will we emit? Only eligible backends contribute. */
    int elig = 0;
    for (int i = 0; i < nbe; i++)
        if (be_eligible(&be[i]))
            elig++;

    int need = elig * vnodes;

    /* Grow the node array if needed. We never shrink it (keeps allocation churn
     * down across rebuilds); `r->n` bounds what is actually valid, and the extra
     * capacity is harmless. On a real config the initial malloc is the only one. */
    if (need > r->cap) {
        struct rnode *grown = (struct rnode *)realloc(r->nodes,
                                                      (size_t)need * sizeof(*grown));
        if (!grown) {
            /* Out of memory rebuilding the ring: keep the OLD ring intact rather
             * than corrupting it. Selection keeps using the previous topology,
             * which is stale but safe. A production LB would alarm here. */
            return;
        }
        r->nodes = grown;
        r->cap   = need;
    }

    /* Fill: for each eligible backend, hash "name#v" for v in [0, vnodes). */
    int k = 0;
    for (int i = 0; i < nbe; i++) {
        if (!be_eligible(&be[i]))
            continue;
        for (int v = 0; v < vnodes; v++) {
            char label[NAME_MAX_LEN + 16];
            /* snprintf returns the length it WOULD have written; clamp so we hash
             * exactly the bytes we produced even if the label were truncated. */
            int len = snprintf(label, sizeof(label), "%s#%d", be[i].name, v);
            if (len < 0)
                continue;               /* encoding error: skip this vnode      */
            if (len > (int)sizeof(label))
                len = (int)sizeof(label);
            r->nodes[k].hash = lb_hash(label, (size_t)len);
            r->nodes[k].idx  = i;       /* this vnode routes to backend i       */
            k++;
        }
    }
    r->n = k;

    /* Sort by hash so ring_lookup can binary-search. */
    qsort(r->nodes, (size_t)r->n, sizeof(r->nodes[0]), rnode_cmp);
}

/* ---------------------------------------------------------------------------
 * ring_lookup — the first vnode clockwise of `key`, wrapping around the ring.
 *
 * `r->nodes` is sorted ascending, so this is a left-most binary search
 * (lower_bound): find the first index whose hash >= key. If the key sits past
 * the largest vnode, it belongs to the arc that wraps over the top back to
 * nodes[0]. Returns the backend index that owns that arc, or -1 if the ring is
 * empty (every backend down/draining) so the caller can reject the connection.
 *
 * This mirrors the standalone ring_lookup in asm/demo.c, whose annotated .s
 * shows exactly these compares and the branchless wrap.
 * --------------------------------------------------------------------------- */
int ring_lookup(const struct ring *r, uint32_t key)
{
    if (r->n <= 0)
        return -1;                      /* empty ring: no eligible backend      */

    int lo = 0, hi = r->n;              /* half-open search range [lo, hi)      */
    while (lo < hi) {
        int mid = lo + ((hi - lo) >> 1);        /* midpoint, overflow-safe      */
        if (r->nodes[mid].hash < key)
            lo = mid + 1;               /* answer is strictly to the right      */
        else
            hi = mid;                   /* mid may be the answer; keep it        */
    }
    if (lo == r->n)                     /* past the last vnode ...              */
        lo = 0;                         /* ... wrap to the smallest (ring is round)*/
    return r->nodes[lo].idx;
}

/* Release the ring's backing array (called at shutdown). */
void ring_free(struct ring *r)
{
    free(r->nodes);
    r->nodes = NULL;
    r->n = r->cap = 0;
}
