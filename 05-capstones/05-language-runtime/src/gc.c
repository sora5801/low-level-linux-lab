/* ===========================================================================
 * gc.c — precise tri-color mark-sweep garbage collector.
 * ===========================================================================
 *
 * TRI-COLOR INVARIANT (the property the mark phase maintains):
 *   white = not yet reached (candidate garbage)
 *   gray  = reached, but its own references not yet scanned  (on the gray stack)
 *   black = reached AND fully scanned                        (isMarked, off stack)
 * The one rule that must never be violated: "no black object points to a white
 * object." We keep it trivially because we mark-then-scan atomically (the whole
 * collection is one uninterruptible step in this cooperative, single-threaded
 * runtime), so there is no mutator running between mark and sweep to install a
 * black->white edge.
 *
 * We use an EXPLICIT gray stack (a libc-managed Obj*[]) instead of recursion, so
 * tracing a deep/cyclic object graph costs heap memory, not C-stack frames, and
 * cannot stack-overflow. Cycles terminate because marking is idempotent:
 * markObject() returns immediately if the object is already marked.
 */
#include <stdio.h>
#include <stdlib.h>

#include "gc.h"
#include "heap.h"      /* heap.objects, heap.bytesAllocated, heapFree            */
#include "object.h"
#include "table.h"
#include "vm.h"        /* the root set: vm.stack, vm.frames, vm.globals          */

/* The gray worklist. libc-managed (realloc) — it must NOT be a GC object, or
 * collecting would perturb the very structure driving the collection. */
static Obj **grayStack    = NULL;
static int   grayCount    = 0;
static int   grayCapacity = 0;

/* Mark one object gray: flip its bit and push it for later scanning. Idempotent
 * (already-marked returns early), which is what makes cyclic graphs terminate. */
void markObject(Obj *object)
{
    if (object == NULL)   return;          /* e.g. an unnamed <script> function  */
    if (object->isMarked) return;          /* already gray/black: don't re-push  */
#ifdef DEBUG_LOG_GC
    printf("%p mark object (type %d)\n", (void *)object, (int)object->type);
#endif
    object->isMarked = true;

    if (grayCapacity < grayCount + 1) {
        grayCapacity = GROW_CAPACITY(grayCapacity);
        /* Raw realloc on purpose: see note above. Abort on failure — we cannot
         * finish a collection we can't record. */
        grayStack = (Obj **)realloc(grayStack, sizeof(Obj *) * (size_t)grayCapacity);
        if (grayStack == NULL) {
            fprintf(stderr, "lumen: fatal: OOM growing GC gray stack\n");
            exit(1);
        }
    }
    grayStack[grayCount++] = object;
}

void markValue(Value value)
{
    if (IS_OBJ(value)) markObject(AS_OBJ(value));   /* inline values have no refs */
}

static void markArray(ValueArray *array)
{
    for (int i = 0; i < array->count; i++) markValue(array->values[i]);
}

/* Marking the globals table marks BOTH the key strings and the value objects.
 * (Because we don't intern, these keys are ordinary strong references — there is
 * no weak-reference bookkeeping here, which is the whole payoff of table.c's
 * content-compare design.) */
static void markTable(Table *table)
{
    for (int i = 0; i < table->capacity; i++) {
        Entry *entry = &table->entries[i];
        if (entry->key != NULL) markObject((Obj *)entry->key);
        markValue(entry->value);
    }
}

/* Blacken: scan a gray object's outgoing references, greying anything new. This
 * is where the object graph's edges live — one case per object kind. */
static void blackenObject(Obj *object)
{
#ifdef DEBUG_LOG_GC
    printf("%p blacken (type %d)\n", (void *)object, (int)object->type);
#endif
    switch (object->type) {
    case OBJ_STRING:                        /* leaf: owns no other objects        */
    case OBJ_NATIVE:                        /* leaf: just a C function pointer     */
        break;
    case OBJ_FUNCTION: {
        ObjFunction *function = (ObjFunction *)object;
        markObject((Obj *)function->name);          /* its name string            */
        markArray(&function->chunk.constants);      /* every literal it references */
        break;
    }
    }
}

/* Phase 1: the roots — everything reachable without going through another object.
 *   - the whole value stack [stack, stackTop): operands and locals in flight;
 *   - each active call frame's function (holds the code + constants we run);
 *   - the globals table (names and their values).
 * Note there is NO "compiler roots" case: the collector only runs once execution
 * has begun (heap.gcEnabled is false during compilation), so no half-built
 * compiler state is live. */
static void markRoots(void)
{
    for (Value *slot = vm.stack; slot < vm.stackTop; slot++)
        markValue(*slot);
    for (int i = 0; i < vm.frameCount; i++)
        markObject((Obj *)vm.frames[i].function);
    markTable(&vm.globals);
}

/* Phase 2: drain the gray stack to a fixpoint. */
static void traceReferences(void)
{
    while (grayCount > 0)
        blackenObject(grayStack[--grayCount]);
}

/* Phase 3: walk the intrusive all-objects list; free every white (unmarked)
 * object and unmark survivors for the next cycle. Classic pointer-to-pointer
 * unlink so we can splice out a node mid-list in O(1). */
static void sweep(void)
{
    Obj *previous = NULL;
    Obj *object   = heap.objects;
    while (object != NULL) {
        if (object->isMarked) {
            object->isMarked = false;       /* reset to white for the next cycle  */
            previous = object;
            object   = object->next;
        } else {
            Obj *unreached = object;
            object = object->next;
            if (previous != NULL) previous->next = object;
            else                  heap.objects   = object;
            freeObject(unreached);
        }
    }
}

/* Release one object: free any libc-owned sidecars first (a function's chunk
 * arrays), then return the object's own block to the allocator's free list. */
void freeObject(Obj *object)
{
#ifdef DEBUG_LOG_GC
    printf("%p free (type %d)\n", (void *)object, (int)object->type);
#endif
    switch (object->type) {
    case OBJ_FUNCTION:
        freeChunk(&((ObjFunction *)object)->chunk);   /* code/lines/constants      */
        break;
    case OBJ_STRING:                                   /* chars are inline: nothing */
    case OBJ_NATIVE:
        break;
    }
    heapFree(object, object->size);                    /* block -> free list        */
}

void collectGarbage(void)
{
#ifdef DEBUG_LOG_GC
    printf("-- gc begin\n");
    size_t before = heap.bytesAllocated;
#endif
    markRoots();          /* 1: color the roots gray                             */
    traceReferences();    /* 2: transitively blacken everything reachable        */
    sweep();              /* 3: free the rest                                    */

    /* Re-arm: next collection when the live set has grown by GC_HEAP_GROW_FACTOR.
     * A floor keeps us from collecting constantly when very little is live. */
    heap.nextGC = heap.bytesAllocated * GC_HEAP_GROW_FACTOR;
    if (heap.nextGC < GC_HEAP_GROW_FACTOR * (size_t)(1u << 16))
        heap.nextGC = GC_HEAP_GROW_FACTOR * (size_t)(1u << 16);

#ifdef DEBUG_LOG_GC
    printf("-- gc end: reclaimed %zu bytes (%zu -> %zu), next at %zu\n",
           before - heap.bytesAllocated, before, heap.bytesAllocated, heap.nextGC);
#endif
}

/* Teardown path (freeVM): free every remaining object regardless of reachability,
 * then drop the gray stack. Called before heapShutdown() unmaps the arenas. */
void freeAllObjects(void)
{
    Obj *object = heap.objects;
    while (object != NULL) {
        Obj *next = object->next;
        freeObject(object);
        object = next;
    }
    heap.objects = NULL;
    free(grayStack);
    grayStack    = NULL;
    grayCount    = 0;
    grayCapacity = 0;
}
