/* ===========================================================================
 * table.c — the open-addressing hash table.
 * ===========================================================================
 */
#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

/* Grow when the table is 75% full. Open addressing degrades sharply as it
 * fills (probe sequences lengthen), so we keep plenty of empty slots. Below
 * this load factor, average probe length stays near 1-2. */
#define TABLE_MAX_LOAD 0.75

void initTable(Table *table)
{
    table->count    = 0;
    table->capacity = 0;
    table->entries  = NULL;
}

void freeTable(Table *table)
{
    FREE_ARRAY(Entry, table->entries, table->capacity);
    initTable(table);
}

/* ---------------------------------------------------------------------------
 * findEntry — the heart of the table. Given a slot array and a key, return the
 * slot where that key lives OR the slot where it should be inserted.
 *
 * Linear probing: start at `hash & (capacity-1)` (a mask, valid because
 * capacity is always a power of two — much cheaper than `% capacity`) and walk
 * forward, wrapping around, until we find the key or an empty slot.
 *
 * Tombstones make deletion safe. A deleted slot is EMPTY-key but true-value.
 * We must probe PAST tombstones (a later key may have been inserted after this
 * slot was deleted), but we also want to REUSE the first tombstone we passed if
 * the key turns out to be absent. `tombstone` remembers that first reusable
 * slot; returning it keeps the table from filling with dead slots.
 * --------------------------------------------------------------------------- */
static Entry *findEntry(Entry *entries, int capacity, ObjString *key)
{
    uint32_t index = key->hash & (uint32_t)(capacity - 1);
    Entry   *tombstone = NULL;

    for (;;) {
        Entry *entry = &entries[index];
        if (entry->key == NULL) {
            if (IS_NIL(entry->value)) {
                /* Truly EMPTY: the key is not here. Prefer to hand back a
                 * tombstone we saw earlier so the insert reclaims it. */
                return tombstone != NULL ? tombstone : entry;
            } else {
                /* A TOMBSTONE (NULL key, non-nil value). Remember the first one
                 * but keep probing — the key might still be further along. */
                if (tombstone == NULL) tombstone = entry;
            }
        } else if (entry->key == key) {
            /* Pointer equality is sufficient: all keys are interned strings, so
             * the same bytes are always the same pointer. */
            return entry;
        }
        /* Wrap with the mask instead of a modulo. */
        index = (index + 1) & (uint32_t)(capacity - 1);
    }
}

bool tableGet(Table *table, ObjString *key, Value *value)
{
    if (table->count == 0) return false;   /* entries may be NULL; guard it     */

    Entry *entry = findEntry(table->entries, table->capacity, key);
    if (entry->key == NULL) return false;  /* landed on empty/tombstone: absent */

    *value = entry->value;
    return true;
}

/* Reallocate to `capacity` slots and REHASH every live entry into the new
 * array. Rehashing is mandatory: an entry's home slot is hash & (cap-1), so
 * changing the capacity changes where it belongs. Tombstones are dropped (not
 * copied) here, which is when the count is recomputed to exclude them. */
static void adjustCapacity(Table *table, int capacity)
{
    Entry *entries = ALLOCATE(Entry, capacity);
    for (int i = 0; i < capacity; i++) {
        entries[i].key   = NULL;
        entries[i].value = NIL_VAL;   /* NULL+NIL == the EMPTY sentinel         */
    }

    table->count = 0;                 /* recount live entries as we copy        */
    for (int i = 0; i < table->capacity; i++) {
        Entry *entry = &table->entries[i];
        if (entry->key == NULL) continue;   /* skip empties AND tombstones      */

        /* findEntry on the fresh array can only return an empty slot (no
         * tombstones exist yet), so this places each key at its new home. */
        Entry *dest = findEntry(entries, capacity, entry->key);
        dest->key   = entry->key;
        dest->value = entry->value;
        table->count++;
    }

    FREE_ARRAY(Entry, table->entries, table->capacity);
    table->entries  = entries;
    table->capacity = capacity;
}

bool tableSet(Table *table, ObjString *key, Value value)
{
    /* Grow before inserting if we would exceed the load factor. count includes
     * tombstones on purpose: they still lengthen probe chains, so they should
     * still push us toward a resize. */
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
        int capacity = GROW_CAPACITY(table->capacity);
        adjustCapacity(table, capacity);
    }

    Entry *entry    = findEntry(table->entries, table->capacity, key);
    bool   isNewKey = (entry->key == NULL);

    /* Only bump count for a brand-new key landing on a TRULY empty slot. If we
     * are reusing a tombstone (isNewKey but value non-nil), count already
     * included it, so incrementing would double-count. */
    if (isNewKey && IS_NIL(entry->value)) table->count++;

    entry->key   = key;
    entry->value = value;
    return isNewKey;
}

bool tableDelete(Table *table, ObjString *key)
{
    if (table->count == 0) return false;

    Entry *entry = findEntry(table->entries, table->capacity, key);
    if (entry->key == NULL) return false;   /* not present                      */

    /* Place a TOMBSTONE: NULL key, value=true. This keeps any probe sequence
     * that passed through this slot intact, at the cost of a dead slot that a
     * future resize or insert can reclaim. */
    entry->key   = NULL;
    entry->value = BOOL_VAL(true);
    return true;
}

void tableAddAll(Table *from, Table *to)
{
    for (int i = 0; i < from->capacity; i++) {
        Entry *entry = &from->entries[i];
        if (entry->key != NULL) tableSet(to, entry->key, entry->value);
    }
}

/* ---------------------------------------------------------------------------
 * tableFindString — the interning lookup. Unlike findEntry it compares CONTENT,
 * because the caller has raw bytes, not an interned ObjString* yet. We still
 * probe by hash; the (hash == && length == && memcmp ==) triple avoids a memcmp
 * on every probed slot — the hash and length filter out mismatches cheaply.
 * --------------------------------------------------------------------------- */
ObjString *tableFindString(Table *table, const char *chars, int length,
                           uint32_t hash)
{
    if (table->count == 0) return NULL;

    uint32_t index = hash & (uint32_t)(table->capacity - 1);
    for (;;) {
        Entry *entry = &table->entries[index];
        if (entry->key == NULL) {
            /* Stop only on a TRULY empty (non-tombstone) slot: a tombstone
             * might sit before the string we want. */
            if (IS_NIL(entry->value)) return NULL;
        } else if (entry->key->length == length &&
                   entry->key->hash   == hash   &&
                   memcmp(entry->key->chars, chars, (size_t)length) == 0) {
            return entry->key;   /* found the canonical interned string         */
        }
        index = (index + 1) & (uint32_t)(table->capacity - 1);
    }
}

/* GC: the intern table is a WEAK set — it must not keep strings alive. After
 * marking, any string still white (unmarked) is unreachable and about to be
 * freed, so remove it from the table first to avoid a dangling key. */
void tableRemoveWhite(Table *table)
{
    for (int i = 0; i < table->capacity; i++) {
        Entry *entry = &table->entries[i];
        if (entry->key != NULL && !entry->key->obj.isMarked)
            tableDelete(table, entry->key);
    }
}

/* GC: the globals table IS a strong root set — mark every key and value so the
 * things globals refer to survive the sweep. */
void markTable(Table *table)
{
    for (int i = 0; i < table->capacity; i++) {
        Entry *entry = &table->entries[i];
        markObject((Obj *)entry->key);   /* markObject tolerates NULL slots     */
        markValue(entry->value);
    }
}
