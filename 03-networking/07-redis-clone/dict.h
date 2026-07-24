/* ===========================================================================
 * dict.h — a hash table with INCREMENTAL rehashing (the heart of Redis).
 * ===========================================================================
 *
 * A hash table amortizes O(1) lookups, but resizing it is O(n): you must move
 * every entry into a bigger bucket array. In an interactive server you cannot
 * afford one request that suddenly stalls for tens of milliseconds while a
 * million keys are rehashed. Redis's answer, reproduced here, is INCREMENTAL
 * rehashing: keep TWO tables during a resize and move a few buckets at a time,
 * on the back of ordinary operations, so no single call pays the whole cost.
 *
 * THE TWO-TABLE INVARIANT (memorize this; the whole file depends on it):
 *   - ht[0] is the primary table. ht[1] is empty EXCEPT while rehashing.
 *   - `rehashidx == -1`  means "not rehashing"; only ht[0] is in use.
 *   - `rehashidx >= 0`   means "rehashing in progress"; it is the index of the
 *      NEXT ht[0] bucket to migrate. Buckets [0, rehashidx) of ht[0] are already
 *      empty (their entries now live in ht[1]); buckets [rehashidx, size) still
 *      hold their entries in ht[0].
 *   - Therefore, while rehashing:
 *       * LOOKUP / DELETE must check BOTH tables (an entry is in exactly one).
 *       * INSERT always goes into ht[1] (so ht[0] only ever shrinks, never grows,
 *         guaranteeing the migration terminates).
 *   - Each mutating operation calls _dictRehashStep(), moving ONE non-empty
 *     bucket. When the last ht[0] bucket is migrated we swap ht[1] into ht[0],
 *     clear ht[1], and set rehashidx = -1.
 *
 * COLLISIONS: separate chaining. Each bucket is the head of a singly linked
 * list of dictEntry. New entries are pushed at the head (O(1), and recent keys
 * are often hot). Table sizes are always powers of two so `hash & sizemask`
 * replaces an expensive modulo with one AND.
 * =========================================================================== */
#ifndef DICT_H
#define DICT_H

#include <stdint.h>   /* uint64_t */
#include <stddef.h>   /* size_t   */

#define DICT_OK  0
#define DICT_ERR 1

/* Initial bucket count for a table's first allocation. Power of two. Redis uses
 * 4; we match it. Small enough that empty dicts are cheap, big enough that the
 * first few inserts don't immediately trigger a resize. */
#define DICT_HT_INITIAL_SIZE 4

/* One key/value pair, plus the chain pointer for its bucket. */
typedef struct dictEntry {
    void *key;                 /* owned per dictType->keyDestructor (here: sds)  */
    void *val;                 /* owned per dictType->valDestructor (or borrowed)*/
    struct dictEntry *next;    /* next entry in the SAME bucket (separate chain) */
} dictEntry;

/* A dict is generic over its key/value semantics via this vtable of callbacks.
 * Different dicts (the keyspace, a hash value, the expires table) supply
 * different destructors, which is how ownership is expressed per table. */
typedef struct dictType {
    uint64_t (*hashFunction)(const void *key, size_t len); /* key bytes -> 64-bit*/
    size_t   (*keyLen)(const void *key);   /* length of key for hashing/compare  */
    int      (*keyCompare)(const void *a, const void *b);  /* 0 == equal         */
    void     (*keyDestructor)(void *key);  /* free a key   (NULL = borrowed)     */
    void     (*valDestructor)(void *val);  /* free a value (NULL = borrowed/int) */
} dictType;

/* One physical hash table: an array of `size` bucket-head pointers. */
typedef struct dictht {
    dictEntry **table;         /* array[size] of chain heads (NULL == empty)     */
    unsigned long size;        /* number of buckets; always a power of two       */
    unsigned long sizemask;    /* size - 1; `hash & sizemask` == `hash % size`   */
    unsigned long used;        /* number of entries stored in this table         */
} dictht;

/* The dict proper: two physical tables plus the rehash cursor. */
typedef struct dict {
    dictType *type;            /* the callback vtable (borrowed, static)         */
    dictht ht[2];              /* ht[0] primary; ht[1] the resize target         */
    long rehashidx;            /* -1: not rehashing; else next ht[0] bucket idx  */
} dict;

/* ---- lifecycle ---------------------------------------------------------- */
dict *dictCreate(dictType *type);
void  dictRelease(dict *d);                       /* free every entry + tables   */

/* ---- mutation ----------------------------------------------------------- */
int   dictAdd(dict *d, void *key, void *val);     /* fail (ERR) if key exists    */
/* dictReplace: insert, or overwrite the value of an existing key (freeing the
 * old value via valDestructor). Returns 1 if a new key was added, 0 if updated.*/
int   dictReplace(dict *d, void *key, void *val);
int   dictDelete(dict *d, const void *key);       /* ERR if absent               */

/* ---- lookup ------------------------------------------------------------- */
dictEntry *dictFind(dict *d, const void *key);    /* NULL if absent              */
void      *dictFetchValue(dict *d, const void *key); /* the val, or NULL         */

/* ---- iteration / sampling ---------------------------------------------- */
/* Visit every entry exactly once (walks BOTH tables, so it is correct mid-
 * rehash). `fn` must not mutate the dict. Used by RDB save and active expiry. */
void dictForEach(dict *d, void (*fn)(void *priv, const void *key, void *val),
                 void *priv);
/* Return a random entry, or NULL if empty. Used by the sampling expiry cycle. */
dictEntry *dictGetRandomKey(dict *d);

/* Total live entries across both tables. */
static inline unsigned long dictSize(const dict *d)
{
    return d->ht[0].used + d->ht[1].used;
}
/* True while a resize is in flight (both tables active). */
static inline int dictIsRehashing(const dict *d) { return d->rehashidx != -1; }

/* Advance an in-flight rehash by up to `n` non-empty buckets. Returns 1 if more
 * work remains, 0 if the table is now fully migrated. serverCron drives this to
 * make progress even when the dict is idle (no inserts to piggy-back on). */
int dictRehash(dict *d, int n);

/* The 64-bit hash used for sds keys (MurmurHash2-64A). Exposed because the
 * asm/ teaching demo extracts exactly this routine. `seed` defends against
 * hash-flooding DoS; set it once at startup from a random source. */
uint64_t dictGenHashFunction(const void *key, size_t len);
void     dictSetHashSeed(uint64_t seed);

#endif /* DICT_H */
