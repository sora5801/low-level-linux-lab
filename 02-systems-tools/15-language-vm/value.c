/* ===========================================================================
 * value.c — Value array growth, printing, and equality.
 * ===========================================================================
 */
#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "value.h"

void initValueArray(ValueArray *array)
{
    array->values   = NULL;   /* NULL + 0/0 is the canonical "empty" state:   */
    array->capacity = 0;      /*   GROW_ARRAY(NULL,0,8) is a fresh malloc     */
    array->count    = 0;
}

void writeValueArray(ValueArray *array, Value value)
{
    /* Grow first if the next slot would be out of bounds. The geometric growth
     * (see GROW_CAPACITY) is what makes a run of appends amortized O(1). */
    if (array->capacity < array->count + 1) {
        int oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        array->values   = GROW_ARRAY(Value, array->values,
                                     oldCapacity, array->capacity);
    }
    array->values[array->count++] = value;   /* 16-byte struct copy           */
}

void freeValueArray(ValueArray *array)
{
    FREE_ARRAY(Value, array->values, array->capacity);
    initValueArray(array);   /* leave it in the valid "empty" state, not dangling */
}

void printValue(Value value)
{
    /* One switch over the tag. Reading the correct union member is only sound
     * because the tag and payload are set together by the *_VAL constructors. */
    switch (value.type) {
        case VAL_NIL:   printf("nil");                              break;
        case VAL_BOOL:  printf(AS_BOOL(value) ? "true" : "false");  break;
        /* PRId64 would be the portable spelling; %lld + a cast is simpler and
         * works everywhere int64_t fits in long long (all our targets). */
        case VAL_INT:   printf("%lld", (long long)AS_INT(value));   break;
        case VAL_OBJ:   printObject(value);                         break;
    }
}

bool valuesEqual(Value a, Value b)
{
    /* Cross-type values are never equal — this is why `0 == false` is false and
     * `nil == 0` is false, matching Lox's strict-ish equality. */
    if (a.type != b.type) return false;

    switch (a.type) {
        case VAL_NIL:   return true;                       /* nil == nil       */
        case VAL_BOOL:  return AS_BOOL(a) == AS_BOOL(b);
        case VAL_INT:   return AS_INT(a)  == AS_INT(b);
        /* Objects compare by IDENTITY. For strings that is also content
         * equality, because we intern every string: equal bytes => same
         * pointer. So we never walk characters here. */
        case VAL_OBJ:   return AS_OBJ(a) == AS_OBJ(b);
        default:        return false;   /* unreachable; silences -Wswitch on
                                         *   a hypothetical future tag          */
    }
}
