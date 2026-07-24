/* ===========================================================================
 * gc.h — the mark-sweep garbage collector's public surface.
 * ===========================================================================
 *
 * Stand-in for sibling 02-systems-tools/06-garbage-collector, but PRECISE rather
 * than conservative: because this is our own language, we know the exact type of
 * every object and every root, so we trace pointers exactly (no "treat any
 * pointer-shaped word as a pointer" guessing). The algorithm is classic tri-color
 * mark-sweep with an explicit gray worklist (no recursion, so a deep object graph
 * can't overflow the C stack):
 *
 *   1. MARK ROOTS      — value stack, call frames' functions, globals table.
 *   2. TRACE (blacken) — pop gray objects, mark everything they reference,
 *                        pushing newly-marked objects gray. Repeat to fixpoint.
 *   3. SWEEP           — walk heap.objects; free the unmarked (white) ones back
 *                        to the allocator; clear the mark on survivors.
 *
 * markObject/markValue are also called from heap.c (via collectGarbage) and are
 * exposed so the VM could add temporary roots if needed.
 */
#ifndef LUMEN_GC_H
#define LUMEN_GC_H

#include "value.h"

void collectGarbage(void);      /* run one full mark-sweep cycle                */
void markObject(Obj *object);   /* root/greying primitive                       */
void markValue(Value value);    /* mark the object inside a Value, if any        */
void freeObject(Obj *object);   /* release one object (chunk arrays + block)     */
void freeAllObjects(void);      /* teardown: free every remaining object         */

#endif /* LUMEN_GC_H */
