/* ===========================================================================
 * table.h — an open-addressing hash table:  ObjString* key -> Value.
 * ===========================================================================
 *
 * The VM needs a hash map in two places:
 *   1. GLOBAL VARIABLES: name string -> current value.
 *   2. STRING INTERNING: the set of all live strings (values are nil; only the
 *      keys matter), so identical byte-sequences collapse to one ObjString*.
 *
 * We use OPEN ADDRESSING with linear probing (all entries live in one flat
 * array, collisions resolved by walking to the next slot) rather than separate
 * chaining. Open addressing is cache-friendly — a probe sequence is a linear
 * scan of contiguous memory — and needs no per-entry allocation, which also
 * means the table itself allocates nothing the GC must trace except the one
 * backing array. Deletions leave a TOMBSTONE (a sentinel entry) so probe
 * sequences that ran *through* the deleted slot are not broken.
 * ===========================================================================
 */
#ifndef CLOXI_TABLE_H
#define CLOXI_TABLE_H

#include "common.h"
#include "value.h"

/* One slot. A slot is:
 *   EMPTY     : key == NULL, value == NIL   (never used; stops a probe)
 *   TOMBSTONE : key == NULL, value == true  (deleted; a probe walks PAST it)
 *   LIVE      : key != NULL                 (a real key/value pair)
 * The tombstone encoding (NULL key + boolean-true value) is why findEntry must
 * distinguish the two NULL-key cases. */
typedef struct {
    ObjString *key;
    Value      value;
} Entry;

typedef struct {
    int    count;     /* live entries PLUS tombstones (drives the load factor) */
    int    capacity;  /* number of slots; always a power of two (mask, not mod) */
    Entry *entries;   /* the flat slot array, owned                            */
} Table;

void initTable(Table *table);
void freeTable(Table *table);

/* Look up `key`; on hit copy the stored value into *value and return true. */
bool tableGet(Table *table, ObjString *key, Value *value);

/* Insert or overwrite key->value. Returns true if the key was NEW (used by the
 * compiler to detect redefining a global). May grow (and rehash) the table. */
bool tableSet(Table *table, ObjString *key, Value value);

/* Delete `key`, leaving a tombstone. Returns true if it was present. */
bool tableDelete(Table *table, ObjString *key);

/* Copy every live entry of `from` into `to` (not used by the core VM but handy
 * for method inheritance in extensions; kept for completeness). */
void tableAddAll(Table *from, Table *to);

/* The interning workhorse: find an EXISTING key whose bytes match, comparing by
 * hash then by content. Returns the canonical ObjString* or NULL. This is the
 * one lookup that must compare characters (everywhere else keys are already
 * interned and compared by pointer). */
ObjString *tableFindString(Table *table, const char *chars, int length,
                           uint32_t hash);

/* GC support: drop interned strings that the collector is about to free (a
 * table used as a weak set must not keep its keys alive), and mark the table's
 * keys+values as roots (for the globals table, which is a strong reference). */
void tableRemoveWhite(Table *table);
void markTable(Table *table);

#endif /* CLOXI_TABLE_H */
