/* ===========================================================================
 * object.c — constructors for heap objects, and their printers.
 * ===========================================================================
 *
 * Every object is born here through allocateObject(), which is the single choke
 * point that (a) asks the allocator for storage, (b) fills the GC header, and
 * (c) threads the new object onto heap.objects so the sweep phase can find it.
 */
#include <stdio.h>
#include <string.h>

#include "heap.h"     /* heapAlloc, heapRoundUp, heap.objects                    */
#include "object.h"

/* Allocate an object of `size` bytes with the given kind. `size` is the logical
 * struct size; the allocator rounds it, and we record the ROUNDED size in the
 * header so freeObject() can hand the exact block back to the right free list. */
static Obj *allocateObject(size_t size, ObjType type)
{
    size_t rounded = heapRoundUp(size);
    Obj   *object  = (Obj *)heapAlloc(rounded);

    object->type     = type;
    object->isMarked = false;              /* white: unreachable until proven    */
    object->size     = (uint32_t)rounded;
    object->next     = heap.objects;       /* push onto the all-objects list     */
    heap.objects     = object;

#ifdef DEBUG_LOG_GC
    printf("%p allocate %zu for type %d\n", (void *)object, rounded, (int)type);
#endif
    return object;
}

/* Helper so each constructor names its concrete type once. */
#define ALLOCATE_OBJ(cType, objType) (cType *)allocateObject(sizeof(cType), objType)

ObjFunction *newFunction(void)
{
    ObjFunction *function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
    function->arity = 0;
    function->name  = NULL;                 /* set later for named functions      */
    initChunk(&function->chunk);            /* empty bytecode to compile into      */
    return function;
}

ObjNative *newNative(NativeFn function)
{
    ObjNative *native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
    native->function = function;
    return native;
}

/* FNV-1a: a fast, well-distributed byte hash. We compute it ONCE at construction
 * and cache it in the object, so hash-table probing and `==` never re-scan. */
static uint32_t hashString(const char *key, int length)
{
    uint32_t hash = 2166136261u;            /* FNV offset basis                   */
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619u;                  /* FNV prime                          */
    }
    return hash;
}

/* Copy `length` bytes into a NEW string object. Thanks to the flexible array
 * member, header + characters are ONE block: allocateObject reserves
 * sizeof(ObjString) + length + 1, and we write the bytes plus a NUL right into
 * the tail. The caller keeps ownership of `chars`. */
ObjString *copyString(const char *chars, int length)
{
    size_t     size   = sizeof(ObjString) + (size_t)length + 1;
    ObjString *string = (ObjString *)allocateObject(size, OBJ_STRING);
    string->length = length;
    string->hash   = hashString(chars, length);
    memcpy(string->chars, chars, (size_t)length);
    string->chars[length] = '\0';           /* NUL for C-interop / printf %s      */
    return string;
}

static void printFunction(ObjFunction *function)
{
    if (function->name == NULL) { printf("<script>"); return; }  /* top level    */
    printf("<fn %s>", function->name->chars);
}

void printObject(Value value)
{
    switch (OBJ_TYPE(value)) {
    case OBJ_STRING:   printf("%s", AS_CSTRING(value));    break;
    case OBJ_FUNCTION: printFunction(AS_FUNCTION(value));  break;
    case OBJ_NATIVE:   printf("<native fn>");              break;
    default:           printf("<obj>");                    break;
    }
}
