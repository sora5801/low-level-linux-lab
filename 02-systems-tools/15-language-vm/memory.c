/* ===========================================================================
 * memory.c — the central allocator and a precise, tri-color-ish mark-sweep GC.
 * ===========================================================================
 *
 * THE COLLECTOR IN ONE PARAGRAPH
 * ------------------------------
 * A mark-sweep collector answers "which heap objects are still reachable?" by:
 *   MARK:  start from the ROOTS (everything the running program can touch
 *          directly: the value stack, the call frames, the globals table, and
 *          any object the compiler is mid-construction on) and paint every
 *          object reachable from them.
 *   SWEEP: walk the intrusive list of ALL objects; free the ones still unpainted
 *          (unreachable = garbage) and un-paint the survivors for next time.
 *
 * We use the classic three-color abstraction to avoid recursion:
 *   WHITE = not yet reached (isMarked == false)
 *   GRAY  = reached but its own references not yet scanned (on the gray stack)
 *   BLACK = reached AND fully scanned (isMarked == true, off the gray stack)
 * markObject turns white->gray (mark + push); processing the gray stack turns
 * gray->black (scan its children, which may turn more white->gray). When the
 * gray stack empties, every reachable object is black and everything still
 * white is provably unreachable. Using an explicit gray STACK instead of C
 * recursion means an arbitrarily deep object graph cannot overflow the C stack.
 * ===========================================================================
 */
#include <stdlib.h>

#include "compiler.h"
#include "memory.h"
#include "object.h"
#include "vm.h"

#ifdef DEBUG_LOG_GC
#include <stdio.h>
#include "debug.h"
#endif

/* After a collection, schedule the next one when the live set has grown by this
 * factor. A multiplicative schedule keeps GC's amortized cost a constant
 * fraction of allocation regardless of the program's absolute heap size. */
#define GC_HEAP_GROW_FACTOR 2

void *reallocate(void *pointer, size_t oldSize, size_t newSize)
{
    /* Maintain the live-bytes total at the single choke point. A shrink/free
     * subtracts; a grow/alloc adds. This total is the GC's trigger metric. */
    vm.bytesAllocated += newSize - oldSize;

    /* Decide whether to collect. We only consider it on a GROWTH (newSize >
     * oldSize): freeing memory never makes collection more urgent. */
    if (newSize > oldSize) {
#ifdef DEBUG_STRESS_GC
        /* Stress mode: collect on EVERY allocation. If any root is missing, an
         * object that is still live gets swept here, and the very next use is a
         * crash — turning a rare heisenbug into a deterministic one. */
        collectGarbage();
#endif
        if (vm.bytesAllocated > vm.nextGC)
            collectGarbage();
    }

    if (newSize == 0) {          /* free path                                   */
        free(pointer);
        return NULL;
    }

    /* realloc handles malloc (pointer==NULL), grow, and shrink uniformly. If it
     * returns NULL the allocation failed; a teaching VM treats OOM as fatal
     * rather than unwinding an error path through every call site. */
    void *result = realloc(pointer, newSize);
    if (result == NULL) exit(1);
    return result;
}

/* WHITE -> GRAY: mark the object and push it on the gray worklist to have its
 * own references scanned later. Idempotent: an already-marked object is
 * skipped, which is what makes cyclic object graphs terminate. */
void markObject(Obj *object)
{
    if (object == NULL) return;        /* e.g. an anonymous function's NULL name */
    if (object->isMarked) return;      /* already gray or black — stop          */

#ifdef DEBUG_LOG_GC
    printf("%p mark ", (void *)object);
    printValue(OBJ_VAL(object));
    printf("\n");
#endif

    object->isMarked = true;

    /* Grow the gray stack with the SYSTEM allocator (raw realloc), NOT our
     * reallocate(): the gray stack is GC scaffolding, and routing it through
     * reallocate() could recursively trigger a collection mid-collection. */
    if (object->isMarked && vm.grayCapacity < vm.grayCount + 1) {
        vm.grayCapacity = GROW_CAPACITY(vm.grayCapacity);
        vm.grayStack = (Obj **)realloc(vm.grayStack,
                                       sizeof(Obj *) * (size_t)vm.grayCapacity);
        if (vm.grayStack == NULL) exit(1);   /* cannot collect without it        */
    }
    vm.grayStack[vm.grayCount++] = object;
}

void markValue(Value value)
{
    /* Only heap objects are collectable; unboxed ints/bools/nil need no marking. */
    if (IS_OBJ(value)) markObject(AS_OBJ(value));
}

/* Mark every Value in an array (used for a chunk's constant pool). */
static void markArray(ValueArray *array)
{
    for (int i = 0; i < array->count; i++)
        markValue(array->values[i]);
}

/* GRAY -> BLACK: scan `object`'s outgoing references, marking each (white->gray).
 * This is where the object graph's EDGES are followed. The set of edges per
 * type is the crux of GC correctness: forget one and you free a live object. */
static void blackenObject(Obj *object)
{
#ifdef DEBUG_LOG_GC
    printf("%p blacken ", (void *)object);
    printValue(OBJ_VAL(object));
    printf("\n");
#endif

    switch (object->type) {
        case OBJ_FUNCTION: {
            /* A function references its name string and every constant in its
             * chunk (which itself may hold nested function objects and strings). */
            ObjFunction *function = (ObjFunction *)object;
            markObject((Obj *)function->name);
            markArray(&function->chunk.constants);
            break;
        }
        case OBJ_STRING:
            /* A string owns only its char buffer, which is NOT a separate Obj —
             * it is freed together with the string in freeObject. No edges. */
            break;
    }
}

/* Free one object and everything it uniquely owns. Called only by sweep and by
 * freeObjects at shutdown — never while the object might still be reachable. */
static void freeObject(Obj *object)
{
#ifdef DEBUG_LOG_GC
    printf("%p free type %d\n", (void *)object, object->type);
#endif

    switch (object->type) {
        case OBJ_STRING: {
            ObjString *string = (ObjString *)object;
            /* The char buffer (length + 1 for the NUL) then the header itself. */
            FREE_ARRAY(char, string->chars, string->length + 1);
            FREE(ObjString, object);
            break;
        }
        case OBJ_FUNCTION: {
            ObjFunction *function = (ObjFunction *)object;
            freeChunk(&function->chunk);   /* code[], lines[], constants[]       */
            FREE(ObjFunction, object);     /* the header (name is shared/interned,
                                            *   freed via the string sweep)       */
            break;
        }
    }
}

/* Mark all ROOTS: the objects the program can reach without going through
 * another object. Miss one and the collector frees something still in use. */
static void markRoots(void)
{
    /* 1. Every live slot of the value stack. stackTop points at the first FREE
     *    slot, so [stack, stackTop) are the live values. */
    for (Value *slot = vm.stack; slot < vm.stackTop; slot++)
        markValue(*slot);

    /* 2. Every active call frame's function (its chunk holds the running code
     *    and constants). */
    for (int i = 0; i < vm.frameCount; i++)
        markObject((Obj *)vm.frames[i].function);

    /* 3. The globals table — a strong reference set of name->value. */
    markTable(&vm.globals);

    /* 4. Objects the COMPILER is mid-construction on but hasn't handed to the VM
     *    yet (a GC can fire during compilation). */
    markCompilerRoots();
}

/* Drain the gray stack: repeatedly pop a gray object and blacken it (which may
 * push more grays). Terminates because each object is marked at most once, so
 * it is pushed at most once. */
static void traceReferences(void)
{
    while (vm.grayCount > 0) {
        Obj *object = vm.grayStack[--vm.grayCount];
        blackenObject(object);
    }
}

/* SWEEP: walk the whole object list. Free white (unreached) objects; un-mark
 * black survivors so they start the next cycle white. We keep a `previous`
 * pointer to splice freed nodes out of the singly-linked list in O(1). */
static void sweep(void)
{
    Obj *previous = NULL;
    Obj *object   = vm.objects;
    while (object != NULL) {
        if (object->isMarked) {
            object->isMarked = false;   /* reset for the next collection         */
            previous = object;
            object   = object->next;
        } else {
            Obj *unreached = object;
            object = object->next;
            /* Unlink `unreached` from the list, then free it. */
            if (previous != NULL) previous->next = object;
            else                  vm.objects     = object;
            freeObject(unreached);
        }
    }
}

void collectGarbage(void)
{
#ifdef DEBUG_LOG_GC
    printf("-- gc begin\n");
    size_t before = vm.bytesAllocated;
#endif

    markRoots();
    traceReferences();

    /* WEAK REFERENCES: the string intern table must not keep strings alive. Now
     * that marking is done, any interned string still white is about to be
     * freed, so remove it from the table FIRST to avoid a dangling key that a
     * later intern lookup would dereference. Order matters: after mark/trace,
     * before sweep. */
    tableRemoveWhite(&vm.strings);

    sweep();

    /* Recompute the next trigger from the surviving live set. */
    vm.nextGC = vm.bytesAllocated * GC_HEAP_GROW_FACTOR;

#ifdef DEBUG_LOG_GC
    printf("-- gc end\n");
    printf("   collected %zu bytes (from %zu to %zu) next at %zu\n",
           before - vm.bytesAllocated, before, vm.bytesAllocated, vm.nextGC);
#endif
}

/* Shutdown: free every remaining object (mark bits are irrelevant now) and the
 * gray-stack scaffolding. After this a leak checker should report zero VM
 * allocations outstanding. */
void freeObjects(void)
{
    Obj *object = vm.objects;
    while (object != NULL) {
        Obj *next = object->next;
        freeObject(object);
        object = next;
    }
    free(vm.grayStack);   /* raw free: it was raw-realloc'd in markObject        */
}
