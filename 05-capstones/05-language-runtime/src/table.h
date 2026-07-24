/* ===========================================================================
 * table.h — an open-addressing hash table mapping ObjString* -> Value.
 * ===========================================================================
 *
 * Used for the VM's GLOBAL variables: name -> value. Open addressing (all entries
 * in one flat array, collisions resolved by linear probing) is chosen over
 * chaining because it is cache-friendly and needs no per-entry allocation.
 *
 * KEY COMPARISON — a deliberate simplification worth understanding:
 * Production VMs (and the sibling 02-systems-tools/15-language-vm) *intern*
 * strings so table keys can be compared by pointer identity, which in turn forces
 * the interning table to hold WEAK references the GC must special-case. We instead
 * compare keys by (hash, then length, then memcmp of bytes) — content equality.
 * That costs a memcmp on a hash hit but buys a huge simplification: the GC has NO
 * weak references at all. Its root set is just stack + frames + globals. Honesty
 * over cleverness; the tradeoff is documented in the README.
 *
 * The entry array is VM-internal (libc realloc), not a GC object; but the KEYS it
 * points at ARE GC objects, so the collector marks them via markTable() (gc.c).
 */
#ifndef LUMEN_TABLE_H
#define LUMEN_TABLE_H

#include "common.h"
#include "value.h"

typedef struct ObjString ObjString;

typedef struct {
    ObjString *key;    /* NULL == empty-or-tombstone slot                      */
    Value      value;  /* for a tombstone (deleted): key==NULL, value==true    */
} Entry;

typedef struct {
    int    count;      /* live entries + tombstones (drives the load factor)   */
    int    capacity;   /* always a power of two, so `hash & (cap-1)` == `% cap`*/
    Entry *entries;
} Table;

void initTable(Table *table);
void freeTable(Table *table);

/* Look up `key`; on hit copy the value to *value and return true. */
bool tableGet(Table *table, ObjString *key, Value *value);
/* Insert or update key->value. Returns true if the key was NEW (not an update). */
bool tableSet(Table *table, ObjString *key, Value value);
/* Delete `key` by planting a tombstone (so probe chains stay intact). */
bool tableDelete(Table *table, ObjString *key);

#endif /* LUMEN_TABLE_H */
