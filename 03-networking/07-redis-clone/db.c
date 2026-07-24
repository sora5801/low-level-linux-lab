/* ===========================================================================
 * db.c — the keyspace: key lookup, mutation, and expiration.
 * ===========================================================================
 * Two dicts back a database (see server.h): `db.dict` maps keys to values and
 * `db.expires` maps a (subset of) those keys to an absolute millisecond
 * deadline. Expiration is implemented two ways, exactly like Redis:
 *
 *   1. LAZY (expireIfNeeded): whenever a command looks a key up, we first check
 *      whether it is already past its deadline and, if so, delete it and behave
 *      as though it were never there. This guarantees correctness — a client
 *      never sees an expired key — but on its own it would leak keys that are
 *      set-and-forgotten (never looked up again).
 *   2. ACTIVE (activeExpireCycle): a background cron samples random volatile
 *      keys and reclaims the expired ones, bounding wasted memory without ever
 *      scanning the whole keyspace.
 *
 * EXPIRES OWNERSHIP: to keep ownership crisp for teaching, db.expires stores its
 * OWN duplicate of each key (keyptrDictType frees it). Real Redis instead shares
 * the single key object between the two dicts to save memory; the trade-off is
 * that it must delete from expires before freeing the shared key. We note the
 * difference and take the safe, simple path.
 * =========================================================================== */
#include "server.h"
#include "zmalloc.h"

#include <stdint.h>   /* intptr_t */

/* Forward: on lazy/active expiry we log a synthetic DEL so the AOF and any
 * future replica stay consistent with what the master actually did. */
static void propagateExpire(sds key)
{
    /* Build a two-element argv: DEL <key>. These sds are transient; aofFeed
     * copies what it needs, so we free them right after. */
    sds argv[2];
    argv[0] = sdsnew("DEL");
    argv[1] = sdsdup(key);
    aofFeed(argv, 2);
    sdsfree(argv[0]);
    sdsfree(argv[1]);
}

/* ---------------------------------------------------------------------------
 * Expiration primitives.
 * ------------------------------------------------------------------------- */

/* Absolute deadline (ms) for `key`, or -1 if the key has no TTL. We use
 * dictFind (not dictFetchValue) because a stored deadline could legitimately be
 * 0, which dictFetchValue's "NULL means absent" convention could not represent. */
long long getExpire(sds key)
{
    if (dictSize(server.db.expires) == 0) return -1;
    dictEntry *de = dictFind(server.db.expires, key);
    if (de == NULL) return -1;
    return (long long)(intptr_t)de->val;   /* the ms deadline was packed here   */
}

/* Set (or replace) the deadline for `key`. We store an OWNED copy of the key so
 * the expires table's lifetime is independent of the main dict. The deadline is
 * packed directly into the value pointer — on x86-64 a void* is 64 bits, wide
 * enough to hold a millisecond timestamp (~1.7e12 today, ~44 bits). */
void setExpire(sds key, long long when_ms)
{
    sds copy = sdsdup(key);
    /* dictReplace frees `copy` for us if the key already had a TTL. */
    dictReplace(server.db.expires, copy, (void *)(intptr_t)when_ms);
}

/* Drop any TTL on `key`. Returns 1 if one existed. */
int removeExpire(sds key)
{
    return dictDelete(server.db.expires, key) == DICT_OK;
}

/* LAZY expiry check. Returns 1 if the key was expired (and thus deleted) now,
 * 0 otherwise. Callers treat a return of 1 as "the key does not exist." */
int expireIfNeeded(sds key)
{
    long long when = getExpire(key);
    if (when < 0) return 0;                 /* no TTL: nothing to do            */
    if (mstime() < when) return 0;          /* not due yet                      */

    /* Past the deadline: delete and account for it. Propagate BEFORE deleting so
     * `key` is still valid when we copy it into the DEL command. */
    propagateExpire(key);
    server.dirty++;
    dbDelete(key);
    return 1;
}

/* ACTIVE expiry: sample a bounded number of volatile keys and reclaim those
 * already expired. Called from serverCron. This is intentionally a *sample*,
 * not a scan — O(1) work per cron tick regardless of keyspace size. */
void activeExpireCycle(void)
{
    if (dictSize(server.db.expires) == 0) return;

    long long now = mstime();
    int checks = 20;                        /* keys sampled per cycle           */
    while (checks--) {
        dictEntry *de = dictGetRandomKey(server.db.expires);
        if (de == NULL) break;
        long long when = (long long)(intptr_t)de->val;
        if (now < when) continue;           /* still alive                      */

        /* Expired. de->key is owned by the expires dict and dbDelete will free
         * it, so we take our own copy to use as the lookup key and for the AOF
         * propagation, avoiding a use-after-free. */
        sds key = sdsdup(de->key);
        propagateExpire(key);
        server.dirty++;
        dbDelete(key);
        sdsfree(key);
    }
}

/* ---------------------------------------------------------------------------
 * Key lookup. Both variants run the lazy-expiry check first.
 * ------------------------------------------------------------------------- */
robj *lookupKeyRead(sds key)
{
    expireIfNeeded(key);
    return dictFetchValue(server.db.dict, key);   /* NULL if absent            */
}

robj *lookupKeyWrite(sds key)
{
    expireIfNeeded(key);
    return dictFetchValue(server.db.dict, key);
}

/* ---------------------------------------------------------------------------
 * Keyspace mutation. dbAdd/dbOverwrite/setKey all TAKE OWNERSHIP of the `key`
 * sds passed in — callers hand over a fresh sdsdup, never a borrowed argv slot.
 * ------------------------------------------------------------------------- */
void dbAdd(sds key, robj *val)
{
    /* Precondition: key absent. dictAdd takes ownership of both key and val. */
    dictAdd(server.db.dict, key, val);
}

void dbOverwrite(sds key, robj *val)
{
    /* Key exists: replace the value (freeing the old robj) and drop the extra
     * key copy dictReplace no longer needs. */
    dictReplace(server.db.dict, key, val);
}

/* Add-or-overwrite in one call (the SET semantics). */
void setKey(sds key, robj *val)
{
    dictReplace(server.db.dict, key, val);
}

/* Remove a key and any associated TTL. Returns 1 if the key existed.
 * Deletes from expires FIRST (Redis order); safe here because callers pass a
 * key that is independent of both dicts' stored copies. */
int dbDelete(sds key)
{
    if (dictSize(server.db.expires) > 0)
        dictDelete(server.db.expires, key);
    return dictDelete(server.db.dict, key) == DICT_OK;
}

int dbExists(sds key)
{
    expireIfNeeded(key);
    return dictFind(server.db.dict, key) != NULL;
}
