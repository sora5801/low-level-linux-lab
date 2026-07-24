/* ===========================================================================
 * value.h — the Value: a single 16-byte tagged union every slot on the VM
 *           stack holds. This is the "word" of our dynamically-typed language.
 * ===========================================================================
 *
 * A dynamic language has no static types, so every runtime value must carry its
 * own type tag. We use the simplest, most legible representation: a struct with
 * an enum tag and a C union payload. (Real VMs pack this into 8 bytes with
 * NaN-boxing; we keep 16 bytes for clarity — see README "Going further".)
 *
 * Memory layout on x86-64 (LP64):
 *     offset 0: ValueType type;   // enum == int, 4 bytes
 *     offset 4: (4 bytes padding so the union is 8-byte aligned)
 *     offset 8: union { bool; double; Obj*; }  // 8 bytes (double/pointer)
 *   => sizeof(Value) == 16, alignof == 8. The stack is an array of these.
 *
 * `bool`, `double`, and `Obj*` share storage because a Value is exactly one of
 * them at a time; the `type` tag says which union member is live. Reading the
 * wrong member is undefined — the AS_* macros below are only ever used after the
 * matching IS_* check (the compiler emits the checks; the VM trusts them).
 */
#ifndef LUMEN_VALUE_H
#define LUMEN_VALUE_H

#include "common.h"

/* Forward declaration: a heap object. Its full shape lives in object.h. A Value
 * only ever holds a *pointer* to one, so a forward declaration suffices here and
 * breaks what would otherwise be a value.h <-> object.h include cycle. */
typedef struct Obj Obj;

typedef enum {
    VAL_BOOL,     /* payload.boolean is live                                   */
    VAL_NIL,      /* no payload; the language's "nothing" value                */
    VAL_NUMBER,   /* payload.number  is live (IEEE-754 double)                 */
    VAL_OBJ       /* payload.obj     is live (points into the GC heap)         */
} ValueType;

typedef struct {
    ValueType type;
    union {
        bool   boolean;
        double number;
        Obj   *obj;
    } as;
} Value;

/* ---- Type predicates: is this Value a T? ----------------------------------
 * Cheap tag comparisons. Always call these before the matching AS_* extractor. */
#define IS_BOOL(value)   ((value).type == VAL_BOOL)
#define IS_NIL(value)    ((value).type == VAL_NIL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)
#define IS_OBJ(value)    ((value).type == VAL_OBJ)

/* ---- Extractors: read the live union member (UNCHECKED — guard with IS_*). */
#define AS_BOOL(value)   ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)
#define AS_OBJ(value)    ((value).as.obj)

/* ---- Constructors: wrap a C value into a tagged Value. --------------------
 * Compound literals `(Value){...}` build the struct inline with zero ceremony. */
#define BOOL_VAL(b)      ((Value){VAL_BOOL,   {.boolean = (b)}})
#define NIL_VAL          ((Value){VAL_NIL,    {.number  = 0}})
#define NUMBER_VAL(n)    ((Value){VAL_NUMBER, {.number  = (n)}})
#define OBJ_VAL(object)  ((Value){VAL_OBJ,    {.obj     = (Obj *)(object)}})

/* ---- ValueArray: a growable Value[] used for a chunk's constant pool. ------
 * This is VM-internal bookkeeping (see common.h's "two memory domains"), so it
 * grows with libc realloc, not the GC. */
typedef struct {
    int    capacity;   /* allocated slots                                      */
    int    count;      /* used slots                                           */
    Value *values;     /* the backing array                                    */
} ValueArray;

void initValueArray(ValueArray *array);
void writeValueArray(ValueArray *array, Value value);   /* append, grow if full */
void freeValueArray(ValueArray *array);

void  printValue(Value value);          /* human-readable, no trailing newline  */
bool  valuesEqual(Value a, Value b);    /* language `==`: strings by CONTENT    */

#endif /* LUMEN_VALUE_H */
