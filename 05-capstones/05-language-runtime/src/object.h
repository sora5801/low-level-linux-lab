/* ===========================================================================
 * object.h — heap objects: the values that live in the GC-managed heap.
 * ===========================================================================
 *
 * Numbers/bools/nil live *inline* in a Value (see value.h). Anything of unbounded
 * or shared size lives on the heap behind a pointer and is owned by the garbage
 * collector. We have three heap object kinds:
 *
 *     ObjString    — an immutable UTF-8/byte string (from a literal or concat).
 *     ObjFunction  — a compiled function: its own Chunk of bytecode + arity.
 *     ObjNative    — a builtin implemented in C (e.g. clock()).
 *
 * THE OBJECT HEADER (`struct Obj`) is the first member of every object, so a
 * `Obj*` and an `ObjString*`/`ObjFunction*` point at the same address and we can
 * "upcast" with a plain pointer cast. The header carries exactly what the GC and
 * allocator need and nothing else:
 *
 *     type      — which kind, so the collector knows what to trace/free.
 *     isMarked  — the GC mark bit (tri-color: set == reachable this cycle).
 *     size      — the block's *rounded* byte size, recorded by the allocator so
 *                 sweep can return the exact block to the right free list.
 *     next      — intrusive singly-linked list threading EVERY live object, so
 *                 the sweep phase can walk the whole heap without a separate
 *                 table. (heap.objects is the head.)
 */
#ifndef LUMEN_OBJECT_H
#define LUMEN_OBJECT_H

#include "common.h"
#include "chunk.h"
#include "value.h"

/* Forward-declare the string type: ObjFunction (below) and copyString() name it
 * before its full `struct ObjString { ... }` definition appears later in this
 * header. A Value/field only ever holds a POINTER to one, so the tag is enough. */
typedef struct ObjString ObjString;

/* Given a Value known to be an object, its kind. */
#define OBJ_TYPE(value)    (AS_OBJ(value)->type)

#define IS_STRING(value)   isObjType(value, OBJ_STRING)
#define IS_FUNCTION(value) isObjType(value, OBJ_FUNCTION)
#define IS_NATIVE(value)   isObjType(value, OBJ_NATIVE)

#define AS_STRING(value)   ((ObjString *)AS_OBJ(value))
#define AS_CSTRING(value)  (((ObjString *)AS_OBJ(value))->chars)
#define AS_FUNCTION(value) ((ObjFunction *)AS_OBJ(value))
#define AS_NATIVE(value)   (((ObjNative *)AS_OBJ(value))->function)

typedef enum { OBJ_STRING, OBJ_FUNCTION, OBJ_NATIVE } ObjType;

struct Obj {
    ObjType     type;       /* 4 bytes: object kind                            */
    bool        isMarked;   /* 1 byte:  GC mark bit for the current cycle       */
    uint32_t    size;       /* rounded allocation size (bytes) — for the freelist*/
    struct Obj *next;       /* intrusive all-objects list (for sweep)          */
};

/* A compiled function. `chunk` is embedded by value: freeing the function frees
 * its bytecode. `name` is NULL for the implicit top-level <script> function. */
typedef struct {
    Obj        obj;
    int        arity;       /* parameter count (checked at OP_CALL)            */
    Chunk      chunk;       /* this function's own bytecode                    */
    ObjString *name;        /* function name for stack traces (may be NULL)    */
} ObjFunction;

/* A builtin. The C function receives argc and a pointer to its args on the VM
 * stack and returns a Value. */
typedef Value (*NativeFn)(int argCount, Value *args);
typedef struct {
    Obj      obj;
    NativeFn function;
} ObjNative;

/* A string. We use a C99 FLEXIBLE ARRAY MEMBER: the characters live in the SAME
 * heap block right after the struct, so one allocation holds header + bytes and
 * one free reclaims both (better locality, simpler ownership than a separate
 * char* buffer). `hash` is precomputed (FNV-1a) so hash-table probing and value
 * equality never re-scan the bytes. `length` excludes the trailing '\0' we keep
 * for C-interop convenience. */
struct ObjString {
    Obj      obj;
    int      length;
    uint32_t hash;
    char     chars[];       /* length+1 bytes (NUL-terminated) follow inline   */
};

ObjFunction *newFunction(void);                     /* fresh, empty function    */
ObjNative   *newNative(NativeFn function);
ObjString   *copyString(const char *chars, int length);  /* copies the bytes    */

void printObject(Value value);   /* used by printValue for VAL_OBJ             */

/* Inline predicate: is `value` an object of kind `type`? Written as a function
 * (not a macro) so `value` is evaluated exactly once — the macros above expand
 * it, and double-evaluating a compound-literal argument would be a bug. */
static inline bool isObjType(Value value, ObjType type)
{
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif /* LUMEN_OBJECT_H */
