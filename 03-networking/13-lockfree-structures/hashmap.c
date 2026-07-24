/* ===========================================================================
 * hashmap.c — lock-free open-addressing hash map implementation.
 * ===========================================================================
 */
#include "hashmap.h"
#include <stdlib.h>   /* calloc, free */
#include <stdint.h>

/* hm_mix — a SplitMix64-style finalizer used as the hash. A raw key (often a
 * small integer or an aligned pointer with low zero bits) has terrible spread
 * for linear probing; this avalanches it so probe clusters do not pile up. */
static inline size_t hm_mix(uintptr_t key, size_t mask)
{
    uint64_t h = (uint64_t)key;
    h ^= h >> 30;
    h *= 0xBF58476D1CE4E5B9ULL;
    h ^= h >> 27;
    h *= 0x94D049BB133111EBULL;
    h ^= h >> 31;
    return (size_t)h & mask;      /* mask == capacity-1, so this is index % cap */
}

int hm_init(hashmap *m, size_t capacity)
{
    /* Require a power of two so wrap-around is a single AND. (x & (x-1)) == 0
     * is the standard power-of-two test; also reject zero. */
    if (capacity == 0 || (capacity & (capacity - 1)) != 0)
        return -1;

    /* calloc zeroes every slot, i.e. key=HM_EMPTY(0), val=HM_BUSY(0). An
     * all-zero slot is correctly "empty," so no per-slot init loop is needed.
     * (C11 guarantees an all-zero-bytes atomic is a valid representation of the
     * value 0 for these integer atomics, so we may treat calloc'd memory as
     * already-initialized atomics on every real target.) */
    m->slots = (hm_slot *)calloc(capacity, sizeof *m->slots);
    if (m->slots == NULL)
        return -1;
    m->mask     = capacity - 1;
    m->capacity = capacity;
    atomic_init(&m->count, 0);
    return 0;
}

int hm_put(hashmap *m, uintptr_t key, uintptr_t val)
{
    /* Probe from the hashed home slot, at most `capacity` slots (the whole
     * table) before declaring it full. */
    size_t i = hm_mix(key, m->mask);
    for (size_t probes = 0; probes <= m->mask; probes++, i = (i + 1) & m->mask) {
        /* ACQUIRE: if we see a nonzero key we may go on to read/return its
         * value, so we must synchronize with the writer that published it. */
        uintptr_t k = atomic_load_explicit(&m->slots[i].key, memory_order_acquire);

        if (k == key) {
            /* Key already lives here — this is an UPDATE. RELEASE so a reader
             * that later acquire-loads this value observes it fully. */
            atomic_store_explicit(&m->slots[i].val, val, memory_order_release);
            return 0;
        }

        if (k == HM_EMPTY) {
            /* Empty slot: try to CLAIM it for our key with one CAS.
             * success = ACQ_REL: release publishes the key to future readers;
             *   acquire lets us observe the winner's key on failure.
             * failure = ACQUIRE: `expected` receives whatever key won the race
             *   so we can check if it happens to be ours. */
            uintptr_t expected = HM_EMPTY;
            if (atomic_compare_exchange_strong_explicit(
                    &m->slots[i].key, &expected, key,
                    memory_order_acq_rel, memory_order_acquire)) {
                /* We own the slot. Publish the value (RELEASE), then bump the
                 * live-key counter (RELAXED: it is only a statistic, not used
                 * to order anything). */
                atomic_store_explicit(&m->slots[i].val, val, memory_order_release);
                atomic_fetch_add_explicit(&m->count, 1, memory_order_relaxed);
                return 0;
            }
            /* Lost the claim. If the winner claimed OUR key, treat it as an
             * update; otherwise a different key landed here — keep probing. */
            if (expected == key) {
                atomic_store_explicit(&m->slots[i].val, val, memory_order_release);
                return 0;
            }
        }
        /* Slot holds a different, nonzero key: linear-probe to the next slot. */
    }
    return -1;   /* table full: no resize in this teaching core */
}

int hm_get(hashmap *m, uintptr_t key, uintptr_t *out)
{
    size_t i = hm_mix(key, m->mask);
    for (size_t probes = 0; probes <= m->mask; probes++, i = (i + 1) & m->mask) {
        uintptr_t k = atomic_load_explicit(&m->slots[i].key, memory_order_acquire);

        if (k == key) {
            /* Found the key. The value may not be published yet (writer claimed
             * the key but has not stored the value). Wait for it. This is the
             * one bounded wait in the map — bounded by a single writer's store,
             * never by a lock. ACQUIRE pairs with the writer's release store. */
            uintptr_t v;
            do {
                v = atomic_load_explicit(&m->slots[i].val, memory_order_acquire);
            } while (v == HM_BUSY);      /* HM_BUSY == 0: value not yet written */

            if (v == HM_TOMBSTONE)
                return 0;                /* logically deleted */
            *out = v;
            return 1;
        }

        if (k == HM_EMPTY)
            return 0;   /* first empty slot ends the probe chain: key absent.
                         * (Valid because keys are never erased — a tombstone
                         * only nulls the VALUE, the key stays, so it never
                         * punches a hole in a probe run.) */
    }
    return 0;   /* probed the whole table without finding it */
}

int hm_remove(hashmap *m, uintptr_t key)
{
    size_t i = hm_mix(key, m->mask);
    for (size_t probes = 0; probes <= m->mask; probes++, i = (i + 1) & m->mask) {
        uintptr_t k = atomic_load_explicit(&m->slots[i].key, memory_order_acquire);

        if (k == key) {
            /* Tombstone the value. The key remains so the probe chain past this
             * slot is preserved. A later hm_put of the same key finds k==key and
             * overwrites the tombstone (resurrecting the entry). RELEASE so the
             * deletion is visible to acquiring readers. */
            atomic_store_explicit(&m->slots[i].val, HM_TOMBSTONE, memory_order_release);
            return 1;
        }
        if (k == HM_EMPTY)
            return 0;   /* absent */
    }
    return 0;
}

void hm_destroy(hashmap *m)
{
    free(m->slots);
    m->slots    = NULL;
    m->mask     = 0;
    m->capacity = 0;
}
