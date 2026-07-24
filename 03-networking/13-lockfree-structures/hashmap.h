/* ===========================================================================
 * hashmap.h — a lock-free, fixed-capacity, open-addressing hash map
 *             (linear probing) built purely from per-slot CAS.
 * ===========================================================================
 *
 * WHAT IT IS (and, honestly, what it is NOT)
 * ------------------------------------------
 * A concurrent map from a nonzero machine-word key to a nonzero machine-word
 * value. Each slot is two atomics {key, val}. Insertion claims an empty slot
 * with a single CAS on the key; lookups and updates are lock-free wait-bounded
 * loads/stores. There is deliberately NO resize and NO slot reclamation — those
 * are the genuinely hard parts of a production lock-free map (Cliff Click's
 * design is a state machine over "copy in progress" slots). We ship the
 * teaching core — concurrent probing with correct publication ordering — and
 * call out the omissions in the README rather than hand-wave them.
 *
 * SLOT STATES (encoded in the two words; both key and value are machine words)
 * ---------------------------------------------------------------------------
 *   key = 0            : slot empty (never used)         -> HM_EMPTY
 *   key = K (nonzero)  : slot owns key K forever after   (open addressing: a key
 *                        is never moved or erased, or probe chains would break)
 *   val = 0            : "claimed but value not yet published" -> HM_BUSY
 *   val = ~0           : tombstone (logically deleted)   -> HM_TOMBSTONE
 *   val = V (other)    : the live value
 *
 * KEY PUBLICATION RACE
 * --------------------
 * A writer must claim the slot (publish the key) BEFORE it can store the value,
 * so there is a window where key==K but val==0. A reader that sees the key must
 * therefore wait for the value to appear (it spins while val==HM_BUSY). This is
 * the ONE spot that is not strictly lock-free: a reader can be delayed by a
 * single writer's store latency (NOT by a lock, and NOT unboundedly by other
 * readers). We flag it precisely instead of hiding it.
 * ===========================================================================
 */
#ifndef HASHMAP_H
#define HASHMAP_H

#include "lockfree.h"

#define HM_EMPTY     ((uintptr_t)0)          /* empty key slot                 */
#define HM_BUSY      ((uintptr_t)0)          /* value not yet published        */
#define HM_TOMBSTONE (~(uintptr_t)0)         /* logically-deleted value        */

typedef struct hm_slot {
    _Atomic(uintptr_t) key;   /* HM_EMPTY or a permanent nonzero key           */
    _Atomic(uintptr_t) val;   /* HM_BUSY, HM_TOMBSTONE, or a live value        */
} hm_slot;

typedef struct hashmap {
    hm_slot         *slots;     /* capacity entries, calloc'd to all-zero        */
    size_t           mask;      /* capacity - 1 (capacity is a power of two)     */
    size_t           capacity;
    _Atomic(size_t)  count;     /* live keys claimed (for load-factor reporting) */
} hashmap;

/* Initialize with `capacity` slots. `capacity` MUST be a power of two (so the
 * index wrap is a mask, not a modulo). Returns 0, or -1 on allocation failure /
 * bad capacity. */
int  hm_init(hashmap *m, size_t capacity);

/* Insert or update key->val. Requires key != 0, and val != 0 and
 * val != HM_TOMBSTONE (those are reserved sentinels). Returns 0 on success, -1
 * if the table is full (no resize). */
int  hm_put(hashmap *m, uintptr_t key, uintptr_t val);

/* Look up key. On hit writes the value through *out and returns 1; returns 0 if
 * the key is absent or tombstoned. */
int  hm_get(hashmap *m, uintptr_t key, uintptr_t *out);

/* Logically delete key (tombstone its value). Returns 1 if the key was present,
 * 0 otherwise. The slot is NOT reclaimed (see header note). */
int  hm_remove(hashmap *m, uintptr_t key);

void hm_destroy(hashmap *m);

#endif /* HASHMAP_H */
