/* ===========================================================================
 * table.c — open-addressing hash table with linear probing and tombstones.
 * ===========================================================================
 *
 * Load factor is capped at 75% so probe sequences stay short. Capacity is always
 * a power of two, which lets us replace the modulo `hash % capacity` with the
 * single-cycle mask `hash & (capacity - 1)`.
 *
 * Deletion cannot just blank a slot — that would break the probe chain of any key
 * that collided past it. Instead we plant a TOMBSTONE (key == NULL, value ==
 * BOOL_VAL(true)); probing walks through tombstones, and insertion may reuse one.
 */
#include <string.h>

#include "object.h"
#include "table.h"

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

/* Find the slot for `key`: either the entry holding it, or the slot where it
 * would be inserted. Keys are compared by CONTENT (hash, then length, then bytes)
 * because we do not intern strings — see table.h. `entries`/`capacity` are passed
 * explicitly so adjustCapacity() can probe into a freshly allocated array. */
static Entry *findEntry(Entry *entries, int capacity, ObjString *key)
{
    uint32_t index = key->hash & (uint32_t)(capacity - 1);
    Entry   *tombstone = NULL;

    for (;;) {
        Entry *entry = &entries[index];
        if (entry->key == NULL) {
            if (IS_NIL(entry->value)) {
                /* Truly empty slot. If we passed a tombstone, reuse it so the
                 * table doesn't grow forever under insert/delete churn. */
                return tombstone != NULL ? tombstone : entry;
            }
            /* A tombstone (key NULL, value true): remember the first one. */
            if (tombstone == NULL) tombstone = entry;
        } else if (entry->key->hash == key->hash &&
                   entry->key->length == key->length &&
                   memcmp(entry->key->chars, key->chars,
                          (size_t)key->length) == 0) {
            return entry;                 /* content match: found the key       */
        }
        index = (index + 1) & (uint32_t)(capacity - 1);  /* linear probe, wraps */
    }
}

/* Grow to `capacity` and REHASH: with a new mask, every key's home slot changes,
 * so we can't memcpy. Tombstones are dropped during the rebuild (count reset to
 * live entries only). */
static void adjustCapacity(Table *table, int capacity)
{
    Entry *entries = GROW_ARRAY(Entry, NULL, 0, capacity);
    for (int i = 0; i < capacity; i++) {
        entries[i].key   = NULL;
        entries[i].value = NIL_VAL;       /* NIL marks "empty", not "tombstone" */
    }

    table->count = 0;
    for (int i = 0; i < table->capacity; i++) {
        Entry *src = &table->entries[i];
        if (src->key == NULL) continue;   /* skip empties AND tombstones        */
        Entry *dst = findEntry(entries, capacity, src->key);
        dst->key   = src->key;
        dst->value = src->value;
        table->count++;
    }

    FREE_ARRAY(Entry, table->entries, table->capacity);
    table->entries  = entries;
    table->capacity = capacity;
}

bool tableGet(Table *table, ObjString *key, Value *value)
{
    if (table->count == 0) return false;              /* no allocation yet      */
    Entry *entry = findEntry(table->entries, table->capacity, key);
    if (entry->key == NULL) return false;             /* empty or tombstone     */
    *value = entry->value;
    return true;
}

/* Insert or update. Returns true iff the key was NOT already present. */
bool tableSet(Table *table, ObjString *key, Value value)
{
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
        int capacity = GROW_CAPACITY(table->capacity);
        adjustCapacity(table, capacity);
    }
    Entry *entry = findEntry(table->entries, table->capacity, key);
    bool isNewKey = entry->key == NULL;
    /* Only bump `count` for a brand-new key landing on a truly-empty slot;
     * reusing a tombstone keeps count the same (it already counted it). */
    if (isNewKey && IS_NIL(entry->value)) table->count++;

    entry->key   = key;
    entry->value = value;
    return isNewKey;
}

bool tableDelete(Table *table, ObjString *key)
{
    if (table->count == 0) return false;
    Entry *entry = findEntry(table->entries, table->capacity, key);
    if (entry->key == NULL) return false;
    /* Tombstone: NULL key but a non-nil value, so findEntry probes past it. */
    entry->key   = NULL;
    entry->value = BOOL_VAL(true);
    return true;
}
