/* ===========================================================================
 * btree.c — a persistent B+-tree over slotted 4 KiB pages.
 * ===========================================================================
 *
 * SHAPE OF THE TREE
 * -----------------
 * This is a B+-*tree*: all key/value pairs live in the LEAF level; internal
 * nodes hold only separator keys that route a search downward. The leaves are
 * chained left-to-right by a sibling pointer (each leaf's NH_RIGHTMOST), so an
 * in-order scan is "find the leftmost leaf, then walk the chain" — no tree walk.
 *
 * SLOTTED PAGE (the on-page layout every node shares — see db.h for offsets)
 * -------------------------------------------------------------------------
 *      +--------------------+ 0
 *      | node header (16 B) |   crc, type, nslots, cell_top, rightmost
 *      +--------------------+ 16
 *      | slot[0] slot[1] .. |   nslots little-endian u16 cell offsets, SORTED
 *      |   ...   (grows ->) |   by key. Binary search runs over THIS array.
 *      +--------------------+ <- slots_end = 16 + nslots*2
 *      |                    |
 *      |    free space      |
 *      |                    |
 *      +--------------------+ <- cell_top  (cells grow DOWN from the page end)
 *      | cell   cell   cell |   variable-length records, in ARBITRARY order;
 *      +--------------------+ 4096   only the slot array is kept sorted.
 *
 *   Leaf cell   : u16 key_len, u32 val_len, key bytes, val bytes   (6 + k + v)
 *   Internal cell: u32 child,  u16 key_len,           key bytes    (6 + k)
 *
 *   Internal routing: cell[i] pairs separator key k[i] with a LEFT child; plus
 *   the node has one extra `rightmost` child. Child c[i] holds keys < k[i]
 *   (and >= k[i-1]); keys >= k[n-1] live under `rightmost`. A search key equal
 *   to a separator goes RIGHT (separators are copies of the right subtree's
 *   smallest key), which is why descent uses UPPER-bound and leaf lookup uses
 *   LOWER-bound.
 *
 * INSERT STRATEGY (why it's written as "decode -> modify -> re-emit")
 * ------------------------------------------------------------------
 * Rather than shuffle bytes inside a page (fiddly, and a nightmare for the
 * split case), each insert:
 *   1. snapshots the page,
 *   2. decodes its cells into a small array of descriptors that POINT INTO the
 *      snapshot (no key/value copying),
 *   3. inserts/replaces the new entry in sorted order,
 *   4. if it all still fits on one page, re-emits into the page; otherwise
 *      splits into two pages and promotes a separator key to the parent.
 * The snapshot is what makes step 4 safe: we read old bytes from it while
 * writing new bytes into the live page. This is O(page) per insert — a real
 * engine does in-place slot surgery for the common no-split case — but it is
 * dramatically easier to see is correct, which is the point here.
 *
 * DELETE (honest scope): we remove the pair from its leaf and DO NOT rebalance
 * or merge underfull nodes (no borrow/merge, no page reclamation). The tree
 * stays a correct search structure; it can just get taller/sparser than a
 * textbook B-tree. Rebalancing is the classic "left as an exercise" — see
 * README "what it omits".
 * ===========================================================================
 */
#include "db.h"

#include <stdint.h>
#include <string.h>

/* Max cell descriptors we hold while rebuilding one page. A page is 4096 bytes;
 * the smallest possible cell is 6 header + 1-byte key (+ 0-byte value) plus a
 * 2-byte slot = 9 bytes, so at most ~453 cells fit. 512 gives head-room for the
 * one extra entry an insert adds. */
#define MAX_ENTS 512

/* Descriptor for a decoded leaf pair. Pointers reference a stable buffer (the
 * page snapshot, or the caller's key/value) for the duration of one operation. */
typedef struct { const uint8_t *k; uint16_t klen; const uint8_t *v; uint32_t vlen; } LeafEnt;
/* Descriptor for a decoded internal entry: a separator key and its LEFT child. */
typedef struct { uint32_t child; const uint8_t *k; uint16_t klen; } IntEnt;

/* A split that must propagate to the parent: the promoted separator key and the
 * page number of the newly-created right sibling/subtree. */
typedef struct {
    uint8_t  key[MAX_KEY];
    uint16_t klen;
    uint32_t right;
    int      split;      /* 1 == a split happened and key/right are set */
} Split;

/* ---------------------------------------------------------------------------
 * key_cmp — total order on keys: unsigned lexicographic, shorter-is-smaller on
 * a shared prefix (like memcmp, then length as the tiebreak). Bytes are
 * compared UNSIGNED so 0x80.. sorts after 0x7f.. as raw binary keys should.
 * Returns <0, 0, >0.
 * --------------------------------------------------------------------------- */
static int key_cmp(const uint8_t *a, uint16_t alen, const uint8_t *b, uint16_t blen)
{
    uint16_t m = alen < blen ? alen : blen;
    for (uint16_t i = 0; i < m; i++) {
        if (a[i] != b[i]) return a[i] < b[i] ? -1 : 1;   /* unsigned bytes */
    }
    if (alen != blen) return alen < blen ? -1 : 1;       /* prefix -> shorter first */
    return 0;
}

/* Convenience: byte offset of slot i within a page. */
static inline uint16_t slot_off(const uint8_t *pg, uint16_t i)
{
    return rd16(pg + NODE_HDR + (uint32_t)i * SLOT_SZ);
}

/* ===========================================================================
 * Page emitters: write a fresh, compacted page image from a descriptor array.
 * Both zero the whole page first so the free gap is deterministic (identical
 * logical pages produce identical bytes -> identical CRCs), then lay out cells
 * from the top down and the sorted slot array from just past the header.
 * ===========================================================================
 */
static void leaf_emit(uint8_t *dst, const LeafEnt *e, int lo, int hi, uint32_t sibling)
{
    memset(dst, 0, PAGE_SIZE);
    dst[NH_TYPE] = PT_LEAF;
    uint16_t top = (uint16_t)PAGE_SIZE;                 /* cell area grows down */
    for (int i = lo; i < hi; i++) {
        uint32_t csz = 6u + e[i].klen + e[i].vlen;
        top = (uint16_t)(top - csz);                    /* carve this cell      */
        wr16(dst + top,     e[i].klen);                 /* [top+0] u16 key_len  */
        wr32(dst + top + 2, e[i].vlen);                 /* [top+2] u32 val_len  */
        memcpy(dst + top + 6,             e[i].k, e[i].klen);          /* key   */
        memcpy(dst + top + 6 + e[i].klen, e[i].v, e[i].vlen);          /* value */
        wr16(dst + NODE_HDR + (uint32_t)(i - lo) * SLOT_SZ, top);      /* slot  */
    }
    wr16(dst + NH_NSLOTS,  (uint16_t)(hi - lo));
    wr16(dst + NH_CELLTOP, top);
    wr32(dst + NH_RIGHTMOST, sibling);                  /* leaf: next-leaf link */
}

static void intl_emit(uint8_t *dst, const IntEnt *e, int lo, int hi, uint32_t rightmost)
{
    memset(dst, 0, PAGE_SIZE);
    dst[NH_TYPE] = PT_INTERNAL;
    uint16_t top = (uint16_t)PAGE_SIZE;
    for (int i = lo; i < hi; i++) {
        uint32_t csz = 6u + e[i].klen;
        top = (uint16_t)(top - csz);
        wr32(dst + top,     e[i].child);                /* [top+0] u32 child    */
        wr16(dst + top + 4, e[i].klen);                 /* [top+4] u16 key_len  */
        memcpy(dst + top + 6, e[i].k, e[i].klen);       /* separator key bytes  */
        wr16(dst + NODE_HDR + (uint32_t)(i - lo) * SLOT_SZ, top);
    }
    wr16(dst + NH_NSLOTS,  (uint16_t)(hi - lo));
    wr16(dst + NH_CELLTOP, top);
    wr32(dst + NH_RIGHTMOST, rightmost);                /* internal: rightmost child */
}

/* On-page byte cost of a range of entries (slot + cell for each). A range fits
 * on one page iff this is <= PAGE_USABLE. */
static uint32_t leaf_bytes(const LeafEnt *e, int lo, int hi)
{
    uint32_t s = 0;
    for (int i = lo; i < hi; i++) s += SLOT_SZ + 6u + e[i].klen + e[i].vlen;
    return s;
}
static uint32_t intl_bytes(const IntEnt *e, int lo, int hi)
{
    uint32_t s = 0;
    for (int i = lo; i < hi; i++) s += SLOT_SZ + 6u + e[i].klen;
    return s;
}

/* Balanced split point for a leaf: the index m in [1,cnt) that minimises the
 * larger of the two halves' byte sizes. Because a single cell always fits and
 * the overflowing total is < 2*PAGE_USABLE, the most-balanced split guarantees
 * both halves fit. */
static int leaf_split_point(const LeafEnt *e, int cnt)
{
    uint32_t total = leaf_bytes(e, 0, cnt);
    uint32_t pre = 0, bestmax = 0xFFFFFFFFu;
    int best = 1;
    for (int m = 1; m < cnt; m++) {
        pre += SLOT_SZ + 6u + e[m - 1].klen + e[m - 1].vlen;  /* bytes of [0,m) */
        uint32_t left = pre, right = total - pre;
        uint32_t mx = left > right ? left : right;
        if (mx < bestmax) { bestmax = mx; best = m; }
    }
    return best;
}

/* Median key index to PROMOTE when an internal node overflows. Left keeps keys
 * [0,mid), right keeps (mid,nk); keys[mid] moves up and lives in neither child. */
static int intl_split_point(const IntEnt *e, int nk)
{
    uint32_t total = intl_bytes(e, 0, nk);
    uint32_t pre = 0, bestmax = 0xFFFFFFFFu;
    int best = 0;
    for (int mid = 0; mid < nk; mid++) {
        uint32_t cost = SLOT_SZ + 6u + e[mid].klen;
        uint32_t left = pre;                          /* sum of [0,mid)      */
        uint32_t right = total - pre - cost;          /* sum of (mid,nk)     */
        uint32_t mx = left > right ? left : right;
        if (mx < bestmax) { bestmax = mx; best = mid; }
        pre += cost;
    }
    return best;
}

/* ===========================================================================
 * leaf_do_insert — insert/replace (key,val) into a LEAF page, possibly splitting.
 * On split, *out carries the promoted separator (== first key of the new right
 * leaf, B+-tree style) and the new right page number.
 * ===========================================================================
 */
static int leaf_do_insert(DB *db, Page *pg,
                          const uint8_t *k, uint16_t klen,
                          const uint8_t *v, uint32_t vlen, Split *out)
{
    uint8_t snap[PAGE_SIZE];
    memcpy(snap, pg->data, PAGE_SIZE);            /* read old bytes from here     */
    uint16_t n = rd16(snap + NH_NSLOTS);
    uint32_t sibling = rd32(snap + NH_RIGHTMOST); /* this leaf's next-leaf link   */

    /* Decode existing cells in sorted (slot) order. */
    LeafEnt e[MAX_ENTS];
    int cnt = 0;
    for (uint16_t i = 0; i < n; i++) {
        uint16_t off = slot_off(snap, i);
        e[cnt].klen = rd16(snap + off);
        e[cnt].vlen = rd32(snap + off + 2);
        e[cnt].k    = snap + off + 6;
        e[cnt].v    = snap + off + 6 + e[cnt].klen;
        cnt++;
    }

    /* LOWER-bound: first entry with key >= our key. */
    int lo = 0, hi = cnt;
    while (lo < hi) {
        int mid = (lo + hi) >> 1;
        if (key_cmp(e[mid].k, e[mid].klen, k, klen) < 0) lo = mid + 1; else hi = mid;
    }
    int pos = lo;

    if (pos < cnt && key_cmp(e[pos].k, e[pos].klen, k, klen) == 0) {
        e[pos].k = k; e[pos].klen = klen;         /* same key: overwrite value    */
        e[pos].v = v; e[pos].vlen = vlen;
    } else {
        if (cnt >= MAX_ENTS) return -1;           /* defensive: never hit at 4 KiB */
        for (int j = cnt; j > pos; j--) e[j] = e[j - 1];  /* open a gap at pos     */
        e[pos].k = k; e[pos].klen = klen; e[pos].v = v; e[pos].vlen = vlen;
        cnt++;
    }

    if (leaf_bytes(e, 0, cnt) <= PAGE_USABLE) {   /* --- fits: no split --------- */
        leaf_emit(pg->data, e, 0, cnt, sibling);
        pager_mark_dirty(db, pg);
        out->split = 0;
        return 0;
    }

    /* --- overflow: split into pg (left) + a fresh right leaf ------------------ */
    int m = leaf_split_point(e, cnt);
    Page *rp = pager_alloc(db, PT_LEAF);          /* new right leaf (already dirty) */
    if (!rp) return -1;
    leaf_emit(pg->data, e, 0,  m,   rp->pgno);    /* left half; link left->right   */
    leaf_emit(rp->data, e, m,  cnt, sibling);     /* right half; right->old sibling */
    pager_mark_dirty(db, pg);

    out->split = 1;
    out->right = rp->pgno;
    out->klen  = e[m].klen;                       /* separator = first key of right */
    memcpy(out->key, e[m].k, e[m].klen);          /* copy OUT of snap before return */
    return 0;
}

/* ===========================================================================
 * internal_do_insert — a child at descent index `d` split; splice its promoted
 * separator (cs->key) and new right child (cs->right) into THIS internal node,
 * re-emitting (and re-splitting if the node itself overflows).
 *
 * We reconstruct the logical arrays children'[] and keys'[] then re-emit:
 *   insert key cs->key at key-index d, and child cs->right at child-index d+1.
 * ===========================================================================
 */
static int internal_do_insert(DB *db, Page *pg, int d, Split *cs, Split *out)
{
    uint8_t snap[PAGE_SIZE];
    memcpy(snap, pg->data, PAGE_SIZE);
    uint16_t n = rd16(snap + NH_NSLOTS);
    uint32_t rightmost = rd32(snap + NH_RIGHTMOST);

    /* children'[]: original c_0..c_d, then cs->right, then c_{d+1}..c_n.
     * (c_i = cell[i].child for i<n; c_n = rightmost.) */
    uint32_t      ch[MAX_ENTS + 2];
    const uint8_t *kk[MAX_ENTS + 2];
    uint16_t      kl[MAX_ENTS + 2];
    int nch = 0, nk = 0;

    for (int i = 0; i <= d; i++)
        ch[nch++] = (i < n) ? rd32(snap + slot_off(snap, (uint16_t)i)) : rightmost;
    ch[nch++] = cs->right;                                    /* new right child */
    for (int i = d + 1; i <= n; i++)
        ch[nch++] = (i < n) ? rd32(snap + slot_off(snap, (uint16_t)i)) : rightmost;

    /* keys'[]: original k_0..k_{d-1}, then cs->key, then k_d..k_{n-1}. */
    for (int i = 0; i < d; i++) {
        uint16_t off = slot_off(snap, (uint16_t)i);
        kk[nk] = snap + off + 6; kl[nk] = rd16(snap + off + 4); nk++;
    }
    kk[nk] = cs->key; kl[nk] = cs->klen; nk++;               /* the promoted key */
    for (int i = d; i < n; i++) {
        uint16_t off = slot_off(snap, (uint16_t)i);
        kk[nk] = snap + off + 6; kl[nk] = rd16(snap + off + 4); nk++;
    }
    /* Now nch == n+2, nk == n+1, and children == keys + 1 (as it must). */

    IntEnt F[MAX_ENTS + 2];
    for (int i = 0; i < nk; i++) { F[i].child = ch[i]; F[i].k = kk[i]; F[i].klen = kl[i]; }
    uint32_t rmost = ch[nk];                                  /* == ch[nch-1]    */

    if (intl_bytes(F, 0, nk) <= PAGE_USABLE) {    /* --- fits: no split --------- */
        intl_emit(pg->data, F, 0, nk, rmost);
        pager_mark_dirty(db, pg);
        out->split = 0;
        return 0;
    }

    /* --- overflow: split, promoting F[mid].key (which leaves both halves). ---- */
    int mid = intl_split_point(F, nk);
    Page *rp = pager_alloc(db, PT_INTERNAL);
    if (!rp) return -1;
    intl_emit(pg->data, F, 0,      mid, F[mid].child);  /* left: rightmost=c_mid  */
    intl_emit(rp->data, F, mid + 1, nk, rmost);         /* right: rightmost=rmost */
    pager_mark_dirty(db, pg);

    out->split = 1;
    out->right = rp->pgno;
    out->klen  = F[mid].klen;
    memcpy(out->key, F[mid].k, F[mid].klen);            /* copy OUT before return */
    return 0;
}

/* Recursive descent insert. Returns 0 ok / <0 error; sets out->split if a new
 * right sibling must be adopted by the caller. */
static int bt_insert_rec(DB *db, uint32_t pgno,
                         const uint8_t *k, uint16_t klen,
                         const uint8_t *v, uint32_t vlen, Split *out)
{
    Page *pg = pager_get(db, pgno);
    if (!pg) return -1;

    if (pg->data[NH_TYPE] == PT_LEAF)
        return leaf_do_insert(db, pg, k, klen, v, vlen, out);

    /* Internal: UPPER-bound over separators picks the child to descend into.
     * d = number of separators <= our key. */
    uint16_t n = rd16(pg->data + NH_NSLOTS);
    int lo = 0, hi = n;
    while (lo < hi) {
        int mid = (lo + hi) >> 1;
        uint16_t off = slot_off(pg->data, (uint16_t)mid);
        uint16_t ml  = rd16(pg->data + off + 4);
        int c = key_cmp(pg->data + off + 6, ml, k, klen);
        if (c <= 0) lo = mid + 1; else hi = mid;       /* <=0 : go right of it */
    }
    int d = lo;
    uint32_t child = (d < n) ? rd32(pg->data + slot_off(pg->data, (uint16_t)d))
                             : rd32(pg->data + NH_RIGHTMOST);

    Split cs; cs.split = 0;
    int rc = bt_insert_rec(db, child, k, klen, v, vlen, &cs);
    if (rc < 0) return rc;
    if (!cs.split) { out->split = 0; return 0; }       /* nothing propagated up  */

    /* pg was untouched by the recursion (splits only add NEW pages), so the
     * descent index d we computed above is still valid. */
    return internal_do_insert(db, pg, d, &cs, out);
}

/* ===========================================================================
 * Public B-tree API. These mutate the page cache + dirty set only; db.c wraps
 * each with the WAL commit that makes it durable.
 * ===========================================================================
 */
int bt_put(DB *db, const void *key, uint16_t klen, const void *val, uint32_t vlen)
{
    if (klen == 0 || klen > MAX_KEY) return -1;        /* forbid empty/huge keys */
    if (vlen > MAX_VAL) return -1;
    if ((uint32_t)SLOT_SZ + 6u + klen + vlen > PAGE_USABLE) return -1;  /* one page */

    Split s; s.split = 0;
    uint32_t root = meta_root(db);
    int rc = bt_insert_rec(db, root, (const uint8_t *)key, klen,
                           (const uint8_t *)val, vlen, &s);
    if (rc < 0) return rc;

    if (s.split) {
        /* Root split: grow the tree by one level. The new root routes with a
         * single separator between the old root (now the left subtree) and the
         * promoted right subtree. */
        Page *nr = pager_alloc(db, PT_INTERNAL);
        if (!nr) return -1;
        IntEnt e = { .child = root, .k = s.key, .klen = s.klen };
        intl_emit(nr->data, &e, 0, 1, s.right);
        pager_mark_dirty(db, nr);
        meta_set_root(db, nr->pgno);                   /* publish the new root   */
    }
    return 0;
}

int bt_get(DB *db, const void *key, uint16_t klen,
           void *valbuf, uint32_t valcap, uint32_t *vallen_out)
{
    const uint8_t *k = (const uint8_t *)key;
    uint32_t pgno = meta_root(db);
    for (;;) {
        Page *pg = pager_get(db, pgno);
        if (!pg) return -1;
        uint16_t n = rd16(pg->data + NH_NSLOTS);

        if (pg->data[NH_TYPE] == PT_LEAF) {
            /* LOWER-bound: find the first slot whose key >= our key, then check
             * that slot for an exact match. Reads go straight off pg->data (no
             * snapshot) because nothing here mutates the page. */
            int lo = 0, hi = n;
            while (lo < hi) {
                int mid = (lo + hi) >> 1;
                uint16_t off = slot_off(pg->data, (uint16_t)mid);  /* slot -> cell offset */
                /* leaf cell: key_len at off, key bytes at off+6 */
                int c = key_cmp(pg->data + off + 6, rd16(pg->data + off), k, klen);
                if (c < 0) lo = mid + 1; else hi = mid;   /* key < target: go right */
            }
            if (lo >= n) return 0;                      /* past the end: miss     */
            uint16_t off = slot_off(pg->data, (uint16_t)lo);
            uint16_t ml  = rd16(pg->data + off);         /* candidate's key_len    */
            if (key_cmp(pg->data + off + 6, ml, k, klen) != 0) return 0;  /* miss  */
            uint32_t vl = rd32(pg->data + off + 2);      /* val_len at off+2        */
            if (vallen_out) *vallen_out = vl;            /* report full length      */
            uint32_t cpy = vl < valcap ? vl : valcap;    /* copy at most valcap      */
            /* value bytes start after the key: off + 6 + key_len */
            if (valbuf && cpy) memcpy(valbuf, pg->data + off + 6 + ml, cpy);
            return 1;
        }
        /* Internal node: UPPER-bound descent. d = number of separators <= key;
         * equality routes RIGHT because a separator equals the smallest key of
         * the subtree to its right (B+-tree invariant). */
        int lo = 0, hi = n;
        while (lo < hi) {
            int mid = (lo + hi) >> 1;
            uint16_t off = slot_off(pg->data, (uint16_t)mid);
            /* internal cell: child at off, key_len at off+4, key at off+6 */
            int c = key_cmp(pg->data + off + 6, rd16(pg->data + off + 4), k, klen);
            if (c <= 0) lo = mid + 1; else hi = mid;      /* sep <= key: go right */
        }
        /* Descend child[lo]: cell[lo].child if lo<n, else the rightmost child. */
        pgno = (lo < n) ? rd32(pg->data + slot_off(pg->data, (uint16_t)lo))
                        : rd32(pg->data + NH_RIGHTMOST);
    }
}

int bt_del(DB *db, const void *key, uint16_t klen)
{
    const uint8_t *k = (const uint8_t *)key;
    uint32_t pgno = meta_root(db);
    for (;;) {
        Page *pg = pager_get(db, pgno);
        if (!pg) return -1;
        uint16_t n = rd16(pg->data + NH_NSLOTS);

        if (pg->data[NH_TYPE] == PT_LEAF) {
            int lo = 0, hi = n;
            while (lo < hi) {
                int mid = (lo + hi) >> 1;
                uint16_t off = slot_off(pg->data, (uint16_t)mid);
                int c = key_cmp(pg->data + off + 6, rd16(pg->data + off), k, klen);
                if (c < 0) lo = mid + 1; else hi = mid;
            }
            if (lo >= n) return 0;                       /* absent */
            uint16_t off = slot_off(pg->data, (uint16_t)lo);
            if (key_cmp(pg->data + off + 6, rd16(pg->data + off), k, klen) != 0)
                return 0;                                /* absent */

            /* Rebuild the leaf without entry `lo`. Removing can only shrink, so
             * this never splits. No merge/rebalance (see file header). */
            uint8_t snap[PAGE_SIZE];
            memcpy(snap, pg->data, PAGE_SIZE);
            uint32_t sibling = rd32(snap + NH_RIGHTMOST);
            LeafEnt e[MAX_ENTS];
            int cnt = 0;
            for (uint16_t i = 0; i < n; i++) {
                if (i == (uint16_t)lo) continue;
                uint16_t o = slot_off(snap, i);
                e[cnt].klen = rd16(snap + o);
                e[cnt].vlen = rd32(snap + o + 2);
                e[cnt].k    = snap + o + 6;
                e[cnt].v    = snap + o + 6 + e[cnt].klen;
                cnt++;
            }
            leaf_emit(pg->data, e, 0, cnt, sibling);
            pager_mark_dirty(db, pg);
            return 1;
        }
        int lo = 0, hi = n;
        while (lo < hi) {
            int mid = (lo + hi) >> 1;
            uint16_t off = slot_off(pg->data, (uint16_t)mid);
            int c = key_cmp(pg->data + off + 6, rd16(pg->data + off + 4), k, klen);
            if (c <= 0) lo = mid + 1; else hi = mid;
        }
        pgno = (lo < n) ? rd32(pg->data + slot_off(pg->data, (uint16_t)lo))
                        : rd32(pg->data + NH_RIGHTMOST);
    }
}

int bt_scan(DB *db, bt_scan_fn fn, void *ctx)
{
    /* Descend to the leftmost leaf. */
    uint32_t pgno = meta_root(db);
    for (;;) {
        Page *pg = pager_get(db, pgno);
        if (!pg) return -1;
        if (pg->data[NH_TYPE] == PT_LEAF) break;
        uint16_t n = rd16(pg->data + NH_NSLOTS);
        pgno = (n > 0) ? rd32(pg->data + slot_off(pg->data, 0))   /* child[0] */
                       : rd32(pg->data + NH_RIGHTMOST);
    }
    /* Walk the leaf sibling chain, yielding every pair in sorted order. */
    while (pgno != 0) {
        Page *pg = pager_get(db, pgno);
        if (!pg) return -1;
        uint16_t n = rd16(pg->data + NH_NSLOTS);
        for (uint16_t i = 0; i < n; i++) {
            uint16_t off = slot_off(pg->data, i);
            uint16_t kl  = rd16(pg->data + off);
            uint32_t vl  = rd32(pg->data + off + 2);
            int rc = fn(pg->data + off + 6, kl, pg->data + off + 6 + kl, vl, ctx);
            if (rc) return rc;                    /* caller asked to stop         */
        }
        pgno = rd32(pg->data + NH_RIGHTMOST);     /* next leaf                    */
    }
    return 0;
}
