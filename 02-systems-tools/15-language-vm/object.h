/* ===========================================================================
 * object.h — heap objects: the GC header, strings, and functions.
 * ===========================================================================
 *
 * A `Value` of type VAL_OBJ points to something on the GC heap. Every heap
 * object begins with the SAME `Obj` header, so the collector can walk a raw
 * `Obj*` without knowing the concrete type: it reads `obj->type` to learn what
 * it is and `obj->next` to find the next object in the global allocation list.
 * This is manual C "inheritance" — the concrete structs (ObjString, ObjFunction)
 * embed `Obj obj;` as their FIRST field, so a pointer to the concrete struct is
 * also a valid pointer to its Obj header (the two share address 0). That
 * coincidence is what makes the `(Obj*)string` upcast in OBJ_VAL sound.
 * ===========================================================================
 */
#ifndef CLOXI_OBJECT_H
#define CLOXI_OBJECT_H

#include "common.h"
#include "chunk.h"
#include "value.h"

/* Extract the concrete object type from a Value known to be an object. */
#define OBJ_TYPE(value)     (AS_OBJ(value)->type)

#define IS_STRING(value)    isObjType(value, OBJ_STRING)
#define IS_FUNCTION(value)  isObjType(value, OBJ_FUNCTION)

/* Downcasts. Sound only after the matching IS_* check — the payload really is
 * an ObjString (or ObjFunction) because its Obj header said so. */
#define AS_STRING(value)    ((ObjString *)AS_OBJ(value))
#define AS_CSTRING(value)   (((ObjString *)AS_OBJ(value))->chars)
#define AS_FUNCTION(value)  ((ObjFunction *)AS_OBJ(value))

typedef enum {
    OBJ_STRING,
    OBJ_FUNCTION,
} ObjType;

/* ---------------------------------------------------------------------------
 * Obj — the common header. Kept deliberately tiny (16 bytes on x86-64):
 *     type      : which concrete object this is (4 bytes + 4 padding)
 *     isMarked  : the GC's "reachable" bit, set during marking, cleared by sweep
 *     next      : intrusive singly-linked list threading EVERY live object, so
 *                 the sweep phase can iterate the whole heap with no side table.
 * The list is intrusive (the link lives inside the object) precisely so the GC
 * needs no auxiliary allocation to track what it must later free.
 * --------------------------------------------------------------------------- */
struct Obj {
    ObjType type;
    bool    isMarked;
    Obj    *next;      /* next object in the VM's global allocation list       */
};

/* ---------------------------------------------------------------------------
 * ObjString — an immutable, heap-allocated, INTERNED string.
 *
 * `length` is stored so we never re-scan for a NUL, and `chars` is a separately
 * owned char[] with a trailing NUL (so it can be handed to C's printf too).
 * `hash` caches the FNV-1a hash computed once at creation; the globals table
 * and the intern table both need it, and strings are immutable so it can never
 * go stale. Because we intern (see object.c/tableFindString), two source
 * strings with the same bytes become the SAME ObjString*, which makes string
 * equality a pointer comparison and table keys comparable by identity.
 * --------------------------------------------------------------------------- */
struct ObjString {
    Obj      obj;      /* MUST be first: enables the Obj*<->ObjString* pun     */
    int      length;   /* byte length, excluding the terminator               */
    char    *chars;    /* owned; length+1 bytes; chars[length] == '\0'        */
    uint32_t hash;     /* cached FNV-1a hash of the bytes                      */
};

/* ---------------------------------------------------------------------------
 * ObjFunction — a compiled function: its own bytecode chunk plus metadata.
 *
 * The top-level program is itself an ObjFunction (an implicit "main"), so the
 * VM has exactly one execution mechanism: call frames over functions. `arity`
 * is the declared parameter count, checked against the argument count at each
 * call. `name` is nil for the top-level script. Functions are first-class
 * Values (VAL_OBJ), stored in the constant pool and pushed like any other.
 * NOTE: this VM has no closures/upvalues, so a function captures nothing beyond
 * globals — see the README scope statement.
 * --------------------------------------------------------------------------- */
typedef struct {
    Obj        obj;
    int        arity;    /* number of declared parameters                     */
    Chunk      chunk;    /* the function body's bytecode + constants          */
    ObjString *name;     /* function name for stack traces; NULL for <script> */
} ObjFunction;

/* Allocate an empty function (arity 0, empty chunk, no name). The compiler
 * fills it in as it parses the body. */
ObjFunction *newFunction(void);

/* Interning constructors. Both return an ObjString that is guaranteed unique
 * for its byte content (existing one reused if present):
 *   copyString  — COPIES the bytes (caller keeps ownership of the source),
 *                 used for source-text lexemes that live in the scanner buffer.
 *   takeString  — TAKES ownership of an already-heap-allocated buffer, used by
 *                 string concatenation, which built the buffer itself; if an
 *                 identical string is already interned it frees the redundant
 *                 buffer and returns the canonical one.
 */
ObjString *copyString(const char *chars, int length);
ObjString *takeString(char *chars, int length);

/* Print an object payload (dispatches on obj->type). */
void printObject(Value value);

/* Inline so the IS_* macros stay branch-cheap. Guards the tag test with an
 * IS_OBJ check first: a non-object Value has no obj->type to read. */
static inline bool isObjType(Value value, ObjType type)
{
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif /* CLOXI_OBJECT_H */
