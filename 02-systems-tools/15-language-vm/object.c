/* ===========================================================================
 * object.c — heap-object allocation, string interning, and hashing.
 * ===========================================================================
 */
#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include "vm.h"

/* ---------------------------------------------------------------------------
 * allocateObject — allocate `size` bytes for an object of `type` and splice it
 * into the VM's intrusive allocation list.
 *
 * The macro wrapper below lets callers write the concrete C type once and get
 * a correctly-typed pointer back. Every new object starts UNMARKED: it becomes
 * a root the instant it is on the stack, and if a collection happens before
 * then the caller is responsible for keeping it reachable (that is why the
 * string/function constructors push their result or work through addConstant).
 * --------------------------------------------------------------------------- */
#define ALLOCATE_OBJ(type, objectType) \
    ((type *)allocateObject(sizeof(type), (objectType)))

static Obj *allocateObject(size_t size, ObjType type)
{
    Obj *object = (Obj *)reallocate(NULL, 0, size);
    object->type     = type;
    object->isMarked = false;       /* sweep clears this each cycle; new = white */

    /* Push onto the front of the global object list (O(1)). The GC sweep walks
     * this list; freeObjects() walks it at shutdown. Threading the link through
     * the object itself means the bookkeeping costs zero extra allocation. */
    object->next = vm.objects;
    vm.objects   = object;

#ifdef DEBUG_LOG_GC
    printf("%p allocate %zu for type %d\n", (void *)object, size, type);
#endif
    return object;
}

ObjFunction *newFunction(void)
{
    ObjFunction *function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
    function->arity = 0;
    function->name  = NULL;         /* set later for named functions           */
    initChunk(&function->chunk);    /* empty body; the compiler fills it        */
    return function;
}

/* ---------------------------------------------------------------------------
 * allocateString — wrap an OWNED char buffer in an ObjString and INTERN it.
 *
 * Interning: we record every string in vm.strings (used as a set — the value
 * is nil). Callers must have already checked that no identical string exists
 * (copyString/takeString do that), so here we just insert. The result is that
 * pointer identity == content identity for all strings, which is what makes
 * table lookups and `==` O(1) pointer compares.
 * --------------------------------------------------------------------------- */
static ObjString *allocateString(char *chars, int length, uint32_t hash)
{
    ObjString *string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars  = chars;         /* takes ownership of the buffer            */
    string->hash   = hash;

    /* GC HAZARD: tableSet may allocate (to grow the intern table), which can
     * collect. `string` is not yet on the value stack, so push it to root it
     * across the insert, then pop. */
    push(OBJ_VAL(string));
    tableSet(&vm.strings, string, NIL_VAL);
    pop();
    return string;
}

/* FNV-1a: a fast, well-distributed non-cryptographic hash. For each byte:
 * XOR into the accumulator, then multiply by the 32-bit FNV prime. The magic
 * offset basis (2166136261) and prime (16777619) are the published constants.
 * We compute it ONCE per string and cache it in ObjString->hash. */
static uint32_t hashString(const char *key, int length)
{
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619u;
    }
    return hash;
}

ObjString *takeString(char *chars, int length)
{
    uint32_t hash = hashString(chars, length);

    /* If an identical string is already interned, we now own a REDUNDANT
     * buffer: free it and hand back the canonical object. This is the branch
     * that makes "a"+"b"+"a" not leak the second "a". */
    ObjString *interned = tableFindString(&vm.strings, chars, length, hash);
    if (interned != NULL) {
        FREE_ARRAY(char, chars, length + 1);
        return interned;
    }
    return allocateString(chars, length, hash);   /* adopt the caller's buffer */
}

ObjString *copyString(const char *chars, int length)
{
    uint32_t hash = hashString(chars, length);

    /* Reuse an existing interned string if present — no allocation at all. */
    ObjString *interned = tableFindString(&vm.strings, chars, length, hash);
    if (interned != NULL) return interned;

    /* Otherwise make our OWN NUL-terminated copy (the source lexeme lives in a
     * buffer we do not own and may be transient). length+1 for the terminator,
     * which lets the chars be handed to C string functions and printf %s. */
    char *heapChars = ALLOCATE(char, length + 1);
    memcpy(heapChars, chars, (size_t)length);
    heapChars[length] = '\0';
    return allocateString(heapChars, length, hash);
}

static void printFunction(ObjFunction *function)
{
    /* The top-level script has no name; print a recognizable placeholder. */
    if (function->name == NULL) {
        printf("<script>");
        return;
    }
    printf("<fn %s>", function->name->chars);
}

void printObject(Value value)
{
    switch (OBJ_TYPE(value)) {
        case OBJ_STRING:   printf("%s", AS_CSTRING(value)); break;
        case OBJ_FUNCTION: printFunction(AS_FUNCTION(value)); break;
    }
}
