/* ===========================================================================
 * dict.c — incremental-rehashing hash table implementation.
 * ===========================================================================
 * Read dict.h first: it states the two-table invariant that every function
 * here maintains. The comments below focus on WHY each step is needed to keep
 * that invariant true, especially around the rehash boundary.
 * =========================================================================== */
#include "dict.h"
#include "zmalloc.h"

#include <string.h>   /* memcpy   */
#include <stdint.h>
#include <stdlib.h>   /* rand     */
#include <limits.h>   /* LONG_MAX */

/* --------------------------------------------------------------------------
 * The hash function: MurmurHash2, 64-bit ("64A"), by Austin Appleby.
 *
 * This is the exact routine the asm/ demo extracts, so it is commented for the
 * arithmetic it will become in assembly. It reads the key 8 bytes at a time
 * (little-endian on x86-64, matching the load the CPU does natively), mixes
 * each block through a multiply + xor-shift avalanche, then folds the 1..7
 * trailing bytes. The magic constant `m` and shift `r` are chosen so that a
 * one-bit change in the input flips ~half the output bits (the avalanche
 * property that makes buckets fill evenly).
 *
 * `hash_seed` is XORed into the initial state. A *random* seed (set once at
 * startup) means an attacker cannot precompute a set of keys that all land in
 * the same bucket and degrade the table to an O(n) list — the classic
 * "hash-flooding" denial-of-service. This is why Redis moved to SipHash; we
 * keep Murmur for didactic clarity but retain the seed defense.
 * -------------------------------------------------------------------------- */
static uint64_t hash_seed = 0;

void dictSetHashSeed(uint64_t seed) { hash_seed = seed; }

uint64_t dictGenHashFunction(const void *key, size_t len)
{
    const uint64_t m = 0xc6a4a7935bd1e995ULL; /* Murmur's mixing multiplier     */
    const int      r = 47;                     /* Murmur's mixing shift          */
    uint64_t h = hash_seed ^ (len * m);        /* seed the state with the length */

    const uint8_t *data = (const uint8_t *)key;
    const uint8_t *end  = data + (len - (len & 7)); /* last 8-byte-aligned point */

    /* Body: consume full 8-byte blocks. */
    while (data != end) {
        /* Assemble one little-endian 64-bit word from 8 bytes. Doing it byte by
         * byte (rather than casting to uint64_t*) keeps the hash identical on
         * big-endian hosts and avoids unaligned-load UB — the compiler folds
         * this back into a single load on x86-64. */
        uint64_t k;
        memcpy(&k, data, sizeof(k));

        k *= m;             /* scramble the block                                */
        k ^= k >> r;        /* fold the high bits down (avalanche)               */
        k *= m;             /* scramble again                                    */

        h ^= k;             /* mix the block into the running hash               */
        h *= m;             /* and diffuse                                       */
        data += 8;
    }

    /* Tail: the final len%8 bytes that don't fill a whole block. This is a
     * fallthrough switch — each case ORs in one more byte at its shift. With
     * -fno-jump-tables it compiles to a compare-and-branch ladder. */
    switch (len & 7) {
    case 7: h ^= (uint64_t)data[6] << 48; /* fallthrough */
    case 6: h ^= (uint64_t)data[5] << 40; /* fallthrough */
    case 5: h ^= (uint64_t)data[4] << 32; /* fallthrough */
    case 4: h ^= (uint64_t)data[3] << 24; /* fallthrough */
    case 3: h ^= (uint64_t)data[2] << 16; /* fallthrough */
    case 2: h ^= (uint64_t)data[1] << 8;  /* fallthrough */
    case 1: h ^= (uint64_t)data[0];
            h *= m;                       /* mix the assembled tail             */
    };

    /* Final avalanche: guarantee the last bytes affect every output bit. */
    h ^= h >> r;
    h *= m;
    h ^= h >> r;
    return h;
}

/* --------------------------------------------------------------------------
 * Small internal helpers.
 * -------------------------------------------------------------------------- */

/* Zero a dictht to the "empty, unallocated" state. */
static void _dictReset(dictht *ht)
{
    ht->table    = NULL;
    ht->size     = 0;
    ht->sizemask = 0;
    ht->used     = 0;
}

/* Round `size` up to the next power of two >= DICT_HT_INITIAL_SIZE. Powers of
 * two let us mask instead of modulo, and the doubling growth keeps amortized
 * insertion O(1). */
static unsigned long _dictNextPower(unsigned long size)
{
    unsigned long i = DICT_HT_INITIAL_SIZE;
    if (size >= (unsigned long)LONG_MAX + 1UL) return (unsigned long)LONG_MAX + 1UL;
    while (i < size) i <<= 1;   /* double until we cover `size`                  */
    return i;
}

/* --------------------------------------------------------------------------
 * dictCreate / dictRelease
 * -------------------------------------------------------------------------- */
dict *dictCreate(dictType *type)
{
    dict *d = zmalloc(sizeof(*d));
    _dictReset(&d->ht[0]);      /* both tables start empty; ht[0] fills lazily   */
    _dictReset(&d->ht[1]);
    d->type      = type;
    d->rehashidx = -1;          /* not rehashing                                 */
    return d;
}

/* Free every entry in one physical table (invoking the destructors), then the
 * bucket array itself. */
static void _dictClear(dict *d, dictht *ht)
{
    for (unsigned long i = 0; i < ht->size && ht->used > 0; i++) {
        dictEntry *he = ht->table[i];
        while (he) {
            dictEntry *next = he->next;       /* grab next BEFORE freeing he     */
            if (d->type->keyDestructor) d->type->keyDestructor(he->key);
            if (d->type->valDestructor) d->type->valDestructor(he->val);
            zfree(he);
            ht->used--;
            he = next;
        }
    }
    zfree(ht->table);           /* the array of head pointers                    */
    _dictReset(ht);
}

void dictRelease(dict *d)
{
    _dictClear(d, &d->ht[0]);
    _dictClear(d, &d->ht[1]);   /* usually empty, but not during a rehash        */
    zfree(d);
}

/* --------------------------------------------------------------------------
 * Rehashing: the incremental resize engine.
 * -------------------------------------------------------------------------- */

/* Move up to `n` NON-EMPTY buckets from ht[0] to ht[1]. Empty buckets are
 * skipped cheaply but we cap the number of empty buckets we're willing to visit
 * (10*n) so a table that is mostly empty can't make one call scan forever.
 *
 * For every migrated entry we recompute its index in ht[1] — the same key hashes
 * to a DIFFERENT bucket because ht[1] has a different (larger) sizemask. We
 * re-link the entry into ht[1]'s chain head; no key/val bytes are copied, only
 * pointers move, so migration is cheap.
 *
 * Returns 1 if there is still more to migrate, 0 when the resize is complete. */
int dictRehash(dict *d, int n)
{
    int empty_visits = n * 10;
    if (!dictIsRehashing(d)) return 0;

    while (n-- && d->ht[0].used != 0) {
        /* Find the next non-empty ht[0] bucket at or after rehashidx. Everything
         * below rehashidx is guaranteed already migrated (== NULL). */
        dictEntry *de, *nextde;
        while (d->ht[0].table[d->rehashidx] == NULL) {
            d->rehashidx++;
            if (--empty_visits == 0) return 1;  /* budget spent: resume later    */
        }
        de = d->ht[0].table[d->rehashidx];
        /* Migrate every entry in THIS bucket's chain to ht[1]. */
        while (de) {
            nextde = de->next;
            /* New index: same hash, new (bigger) mask -> new bucket. */
            uint64_t h = d->type->hashFunction(de->key,
                                               d->type->keyLen(de->key));
            unsigned long idx = h & d->ht[1].sizemask;
            de->next = d->ht[1].table[idx];   /* push at ht[1] chain head        */
            d->ht[1].table[idx] = de;
            d->ht[0].used--;
            d->ht[1].used++;
            de = nextde;
        }
        d->ht[0].table[d->rehashidx] = NULL;  /* the source bucket is now empty  */
        d->rehashidx++;
    }

    /* Did we just finish? ht[0] fully drained -> promote ht[1] to primary. */
    if (d->ht[0].used == 0) {
        zfree(d->ht[0].table);                /* free the old (smaller) array    */
        d->ht[0] = d->ht[1];                  /* ht[1] becomes the new primary   */
        _dictReset(&d->ht[1]);                /* ht[1] goes back to empty        */
        d->rehashidx = -1;                    /* rehashing complete              */
        return 0;
    }
    return 1;   /* still buckets left in ht[0] */
}

/* One unit of "piggy-backed" rehash work, run on every mutating op so a resize
 * makes steady progress driven by traffic. Kept to a single bucket to bound the
 * per-operation latency. */
static void _dictRehashStep(dict *d)
{
    dictRehash(d, 1);
}

/* Allocate ht[1] sized for `size` entries and enter the rehashing state. If the
 * dict is empty this is really just the first allocation of ht[0]. */
static int _dictExpand(dict *d, unsigned long size)
{
    if (dictIsRehashing(d)) return DICT_ERR;   /* already resizing               */

    unsigned long realsize = _dictNextPower(size);
    if (realsize == d->ht[0].size) return DICT_ERR; /* no-op                      */

    dictht n;
    n.size     = realsize;
    n.sizemask = realsize - 1;
    n.table    = zcalloc(realsize * sizeof(dictEntry *)); /* all buckets NULL    */
    n.used     = 0;

    /* First-ever allocation: no rehash needed, ht[0] simply becomes `n`. */
    if (d->ht[0].table == NULL) {
        d->ht[0] = n;
        return DICT_OK;
    }
    /* Otherwise `n` is the resize target: park it in ht[1] and start migrating. */
    d->ht[1]     = n;
    d->rehashidx = 0;
    return DICT_OK;
}

/* Grow the table when the load factor reaches 1.0 (used >= size), doubling the
 * bucket count. Redis also supports a higher ratio when a fork is in progress
 * (to avoid CoW page churn); we keep the simple 1:1 threshold. */
static int _dictExpandIfNeeded(dict *d)
{
    if (dictIsRehashing(d)) return DICT_OK;               /* already growing      */
    if (d->ht[0].size == 0) return _dictExpand(d, DICT_HT_INITIAL_SIZE);
    if (d->ht[0].used >= d->ht[0].size)
        return _dictExpand(d, d->ht[0].used * 2);
    return DICT_OK;
}

/* --------------------------------------------------------------------------
 * Insertion.
 * -------------------------------------------------------------------------- */

/* Compute the bucket index for `key`, or return -1 if the key already exists.
 * When rehashing, an insert must target ht[1] (so ht[0] only shrinks), so we
 * return the index into whichever table new entries belong in; `*existing`, if
 * non-NULL, is set to the found entry when the key is present. */
static long _dictKeyIndex(dict *d, const void *key, uint64_t hash,
                          dictEntry **existing)
{
    if (existing) *existing = NULL;
    if (_dictExpandIfNeeded(d) == DICT_ERR) return -2; /* OOM path is abort'd     */

    long idx = -1;
    for (int table = 0; table <= 1; table++) {
        idx = (long)(hash & d->ht[table].sizemask);
        /* Scan this bucket's chain for an equal key (a duplicate). */
        dictEntry *he = d->ht[table].table[idx];
        while (he) {
            if (he->key == key ||
                d->type->keyCompare(key, he->key) == 0) {
                if (existing) *existing = he;
                return -1;                 /* key present: caller must not add    */
            }
            he = he->next;
        }
        if (!dictIsRehashing(d)) break;    /* only ht[0] exists -> stop           */
    }
    /* Not found: new entries go to ht[1] while rehashing, else ht[0]. */
    return (long)(hash & d->ht[dictIsRehashing(d) ? 1 : 0].sizemask);
}

int dictAdd(dict *d, void *key, void *val)
{
    if (dictIsRehashing(d)) _dictRehashStep(d);  /* pay a little rehash "tax"     */

    uint64_t hash = d->type->hashFunction(key, d->type->keyLen(key));
    long idx = _dictKeyIndex(d, key, hash, NULL);
    if (idx < 0) return DICT_ERR;                /* -1 duplicate / -2 error       */

    int table = dictIsRehashing(d) ? 1 : 0;      /* insert into the right table   */
    dictEntry *entry = zmalloc(sizeof(*entry));
    entry->key  = key;                           /* dict now OWNS key             */
    entry->val  = val;                           /* dict now OWNS val (per type)  */
    entry->next = d->ht[table].table[idx];       /* push at chain head (O(1))     */
    d->ht[table].table[idx] = entry;
    d->ht[table].used++;
    return DICT_OK;
}

int dictReplace(dict *d, void *key, void *val)
{
    /* Try a plain add first; if the key was new we're done and it was inserted. */
    if (dictAdd(d, key, val) == DICT_OK) return 1;

    /* Key already exists: find it and swap the value (freeing the old one). We
     * pass the caller's `key` to dictFind, then free that redundant key copy
     * because the entry keeps its original key. */
    dictEntry *entry = dictFind(d, key);
    void *oldval = entry->val;
    entry->val = val;
    if (d->type->valDestructor) d->type->valDestructor(oldval);
    if (d->type->keyDestructor) d->type->keyDestructor(key); /* drop dup key      */
    return 0;
}

/* --------------------------------------------------------------------------
 * Lookup.
 * -------------------------------------------------------------------------- */
dictEntry *dictFind(dict *d, const void *key)
{
    if (dictSize(d) == 0) return NULL;
    if (dictIsRehashing(d)) _dictRehashStep(d);

    uint64_t hash = d->type->hashFunction(key, d->type->keyLen(key));
    for (int table = 0; table <= 1; table++) {
        unsigned long idx = hash & d->ht[table].sizemask;
        dictEntry *he = d->ht[table].table[idx];
        while (he) {
            if (he->key == key || d->type->keyCompare(key, he->key) == 0)
                return he;
            he = he->next;
        }
        if (!dictIsRehashing(d)) return NULL;  /* ht[1] not in play               */
    }
    return NULL;
}

void *dictFetchValue(dict *d, const void *key)
{
    dictEntry *he = dictFind(d, key);
    return he ? he->val : NULL;
}

/* --------------------------------------------------------------------------
 * Deletion.
 * -------------------------------------------------------------------------- */
int dictDelete(dict *d, const void *key)
{
    if (dictSize(d) == 0) return DICT_ERR;
    if (dictIsRehashing(d)) _dictRehashStep(d);

    uint64_t hash = d->type->hashFunction(key, d->type->keyLen(key));
    for (int table = 0; table <= 1; table++) {
        unsigned long idx = hash & d->ht[table].sizemask;
        dictEntry *he   = d->ht[table].table[idx];
        dictEntry *prev = NULL;
        while (he) {
            if (he->key == key || d->type->keyCompare(key, he->key) == 0) {
                /* Unlink from the chain: patch the predecessor (or the bucket
                 * head) to skip `he`. */
                if (prev) prev->next = he->next;
                else      d->ht[table].table[idx] = he->next;
                if (d->type->keyDestructor) d->type->keyDestructor(he->key);
                if (d->type->valDestructor) d->type->valDestructor(he->val);
                zfree(he);
                d->ht[table].used--;
                return DICT_OK;
            }
            prev = he;
            he = he->next;
        }
        if (!dictIsRehashing(d)) break;
    }
    return DICT_ERR;   /* not found */
}

/* --------------------------------------------------------------------------
 * Iteration and sampling.
 * -------------------------------------------------------------------------- */

/* Visit every entry. Because an entry lives in exactly one table, walking BOTH
 * bucket arrays in full visits each entry once (buckets already migrated out of
 * ht[0] are NULL). Safe to call mid-rehash; must not mutate the dict. */
void dictForEach(dict *d, void (*fn)(void *priv, const void *key, void *val),
                 void *priv)
{
    for (int table = 0; table <= 1; table++) {
        dictht *ht = &d->ht[table];
        for (unsigned long i = 0; i < ht->size; i++) {
            dictEntry *he = ht->table[i];
            while (he) {
                fn(priv, he->key, he->val);
                he = he->next;
            }
        }
    }
}

/* Return a uniformly-ish random entry (or NULL if empty). Used by the expiry
 * sampler: Redis's active expiration deletes a handful of random volatile keys
 * per cycle rather than scanning the whole keyspace. */
dictEntry *dictGetRandomKey(dict *d)
{
    if (dictSize(d) == 0) return NULL;
    if (dictIsRehashing(d)) _dictRehashStep(d);

    dictEntry *he;
    unsigned long h;
    if (dictIsRehashing(d)) {
        /* The live key space spans ht[0][rehashidx..size) and all of ht[1].
         * Pick a slot in that combined range, then map it to a table. */
        unsigned long s0 = d->ht[0].size;
        do {
            h = (unsigned long)rand() %
                (s0 + d->ht[1].size - (unsigned long)d->rehashidx);
            he = (h >= s0 - (unsigned long)d->rehashidx)
                 ? d->ht[1].table[h - (s0 - (unsigned long)d->rehashidx)]
                 : d->ht[0].table[(unsigned long)d->rehashidx + h];
        } while (he == NULL);
    } else {
        do {
            h  = (unsigned long)rand() & d->ht[0].sizemask;
            he = d->ht[0].table[h];
        } while (he == NULL);
    }

    /* `he` is the head of a chain; walk a random distance into it so long chains
     * don't bias toward their heads. */
    unsigned long listlen = 0;
    dictEntry *orig = he;
    while (he) { listlen++; he = he->next; }
    unsigned long listele = (unsigned long)rand() % listlen;
    he = orig;
    while (listele--) he = he->next;
    return he;
}
