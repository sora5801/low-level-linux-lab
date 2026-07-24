/* ===========================================================================
 * value.h — the VM's universal Value type and the growable constant array.
 * ===========================================================================
 *
 * A dynamic language needs ONE C type that can hold any runtime value: a
 * number now, a string next, a boolean after that. We use a TAGGED UNION: an
 * 8-bit type tag plus a union of the possible payloads. This is the simplest
 * representation and the easiest to read; production VMs (LuaJIT, V8, clox's
 * later chapters) pack the tag and payload into a single 64-bit "NaN-boxed"
 * word to halve memory traffic. We keep the fat struct for clarity and note
 * the NaN-boxing alternative in the README's "going further".
 *
 * MEMORY LAYOUT of `Value` on x86-64 (LP64):
 *     offset 0 : ValueType type   (enum -> int, 4 bytes)
 *     offset 4 : (4 bytes padding, because the union needs 8-byte alignment)
 *     offset 8 : union { bool; int64_t; Obj*; }   (8 bytes)
 *     total    : 16 bytes, 8-byte aligned.
 * Every push/pop moves 16 bytes. That padding byte-for-byte cost is exactly
 * what NaN-boxing removes.
 * ===========================================================================
 */
#ifndef CLOXI_VALUE_H
#define CLOXI_VALUE_H

#include "common.h"

/* Forward declaration only. Value must be able to *hold* an Obj* without
 * knowing Obj's layout, and object.h in turn includes this file — a classic
 * circular dependency broken by naming the struct here and defining it there.
 * `Obj` is the header shared by every heap object; `ObjString` is one such. */
typedef struct Obj       Obj;
typedef struct ObjString ObjString;

/* The four value categories. NIL/BOOL/INT live entirely inside the Value
 * (they are "unboxed", no heap object). OBJ means the payload is a pointer to
 * a garbage-collected heap object whose first field is an `Obj` header. */
typedef enum {
    VAL_NIL,   /* the absence of a value; also the default/uninitialized state */
    VAL_BOOL,  /* true / false                                                 */
    VAL_INT,   /* a 64-bit signed integer (this VM has no floats)              */
    VAL_OBJ,   /* a pointer to a heap object (string, function, ...)           */
} ValueType;

typedef struct {
    ValueType type;
    union {
        bool     boolean;
        int64_t  integer;
        Obj     *obj;      /* only valid when type == VAL_OBJ                  */
    } as;                  /* named `as` so call sites read `v.as.integer`     */
} Value;

/* --- Constructors: C value  ->  boxed Value -------------------------------
 * These are the ONLY blessed way to build a Value, so the tag can never
 * disagree with the payload. Written as macros (not functions) so they are
 * free at -O0 and usable in static initializers. */
#define NIL_VAL           ((Value){ VAL_NIL,  { .integer = 0 } })
#define BOOL_VAL(b)       ((Value){ VAL_BOOL, { .boolean = (b) } })
#define INT_VAL(i)        ((Value){ VAL_INT,  { .integer = (i) } })
#define OBJ_VAL(object)   ((Value){ VAL_OBJ,  { .obj = (Obj *)(object) } })

/* --- Accessors: boxed Value -> C value ------------------------------------
 * These do NOT check the tag — the VM must test IS_* first. Reading the wrong
 * union member is undefined behavior, which is why every opcode handler that
 * touches a Value validates its type before extracting the payload. */
#define AS_BOOL(value)    ((value).as.boolean)
#define AS_INT(value)     ((value).as.integer)
#define AS_OBJ(value)     ((value).as.obj)

/* --- Type predicates ------------------------------------------------------ */
#define IS_NIL(value)     ((value).type == VAL_NIL)
#define IS_BOOL(value)    ((value).type == VAL_BOOL)
#define IS_INT(value)     ((value).type == VAL_INT)
#define IS_OBJ(value)     ((value).type == VAL_OBJ)

/* ---------------------------------------------------------------------------
 * ValueArray — a textbook growable array (a "vector") of Values.
 *
 * Every chunk owns one of these as its CONSTANT POOL: literals in the source
 * (numbers, string objects, function objects) are appended here once at
 * compile time and referenced by a small integer index in the bytecode. That
 * is why `OP_CONSTANT 3` is two bytes yet can load an arbitrary 16-byte value.
 * --------------------------------------------------------------------------- */
typedef struct {
    int    capacity;  /* allocated slots (a power-of-two-ish growth schedule)  */
    int    count;     /* slots in use; the next append lands at values[count]  */
    Value *values;    /* heap block, owned by this array, freed in freeValueArray */
} ValueArray;

void initValueArray(ValueArray *array);
/* Append `value`; amortized O(1). May call reallocate(), which may trigger GC,
 * so the caller must keep any not-yet-rooted objects reachable across it. */
void writeValueArray(ValueArray *array, Value value);
void freeValueArray(ValueArray *array);

/* Print a Value in source-like form (used by `print` and the disassembler). */
void printValue(Value value);

/* Deep value equality with LOX semantics: different types are never equal
 * (so 0 != false), ints compare by value, and objects compare by IDENTITY
 * except strings, which are interned so identity == content equality. */
bool valuesEqual(Value a, Value b);

#endif /* CLOXI_VALUE_H */
