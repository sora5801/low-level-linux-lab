/* ===========================================================================
 * common.h — shared types, config switches, and checked-allocation helpers.
 * ===========================================================================
 *
 * This capstone stitches six sibling projects into ONE small language runtime
 * for a dynamically-typed language we call "Lumen" (files end in `.lum`). The
 * pipeline is:
 *
 *     source text ──▶ scanner ──▶ compiler ──▶ bytecode chunk ──▶ VM ──▶ result
 *                     (lexer)     (Pratt)       (OpCode[])      (goto*)
 *
 * with a mark-sweep GC + custom bump/free-list allocator underneath the object
 * model, and two demonstrated add-ons: an x86-64 JIT of a hot loop and cooperative
 * green threads. See README.md's Architecture section for the subsystem map.
 *
 * TWO MEMORY DOMAINS — this is the single most important design fact:
 *   1. The GC-MANAGED HEAP (heap.c/gc.c) holds *language objects* the program
 *      can create at run time and lose track of: strings and functions. These
 *      are reclaimed by the mark-sweep collector.
 *   2. VM-INTERNAL GROWABLE ARRAYS (the bytecode buffer, the line table, the
 *      constant pool, hash-table entry arrays) are compiler/VM bookkeeping with
 *      clear single ownership. They use plain libc realloc/free via the xrealloc
 *      helpers below — they are NOT GC objects and never move.
 * Keeping these domains separate is what lets the GC stay legible: its root set
 * is just "the value stack, the call frames, and the globals table."
 * ===========================================================================
 */
#ifndef LUMEN_COMMON_H
#define LUMEN_COMMON_H

#include <stdbool.h>   /* bool, true, false                                    */
#include <stddef.h>    /* size_t, NULL                                         */
#include <stdint.h>    /* uint8_t, uint32_t, int64_t — fixed-width guarantees  */
#include <stdlib.h>    /* malloc/realloc/free/exit                             */
#include <stdio.h>     /* fprintf(stderr,...) for the fatal-OOM path           */

/* ---- Compile-time configuration (flip with -D on the compiler line) --------
 * DEBUG_TRACE_EXECUTION : print the stack + disassembled op before each step.
 * DEBUG_PRINT_CODE      : disassemble each function right after it compiles.
 * DEBUG_STRESS_GC       : run a full collection on EVERY heap allocation. This
 *                         is the strongest test of the GC root set: if any live
 *                         object is missing from the roots, the program crashes
 *                         here deterministically instead of "sometimes".
 * DEBUG_LOG_GC          : narrate the collector (mark/sweep/free, byte totals).
 * All are OFF unless defined; the Makefile's `debug`/`gc-test` targets set them. */

/* A byte operand can index 0..255, so many tables are sized 256. */
#define UINT8_COUNT (UINT8_MAX + 1)

/* --------------------------------------------------------------------------
 * Checked libc allocation for VM-internal arrays (NOT the GC heap).
 *
 * Every real allocation in this repo checks its return value (CONVENTIONS.md).
 * A NULL from realloc here is unrecoverable bookkeeping failure, so we print and
 * abort rather than limp on with a corrupt data structure. `static inline` in a
 * shared header gives each translation unit its own copy with no linker clash,
 * and `inline` suppresses the "unused function" warning where a TU doesn't call it.
 * -------------------------------------------------------------------------- */

/* Growth policy for dynamic arrays: start at 8, then double. Doubling makes N
 * appends amortize to O(1) each (the geometric series of copies sums to < 2N). */
#define GROW_CAPACITY(cap) ((cap) < 8 ? 8 : (cap) * 2)

/* Resize `ptr` (holding `oldCount` elems of `elemSize`) to `newCount` elems.
 * newCount==0 frees and returns NULL. oldCount is unused here but kept in the
 * signature so call sites document the transition and a future arena could use it. */
static inline void *reallocArray(void *ptr, size_t oldCount, size_t newCount,
                                 size_t elemSize)
{
    (void)oldCount;                       /* documented-but-unused (see above)  */
    if (newCount == 0) {                  /* shrink-to-nothing is a free        */
        free(ptr);
        return NULL;
    }
    void *result = realloc(ptr, newCount * elemSize);
    if (result == NULL) {                 /* realloc failed: cannot continue    */
        fprintf(stderr, "lumen: fatal: out of memory (realloc %zu bytes)\n",
                newCount * elemSize);
        exit(1);
    }
    return result;
}

/* Typed convenience wrappers around reallocArray. `type` is the element type so
 * the byte math is written once and correctly. */
#define GROW_ARRAY(type, ptr, oldCount, newCount) \
    (type *)reallocArray(ptr, (oldCount) * sizeof(type), (newCount), sizeof(type))

#define FREE_ARRAY(type, ptr, oldCount) \
    (void)reallocArray(ptr, (oldCount) * sizeof(type), 0, sizeof(type))

#endif /* LUMEN_COMMON_H */
