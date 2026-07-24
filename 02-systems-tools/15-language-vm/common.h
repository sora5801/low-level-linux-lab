/* ===========================================================================
 * common.h — project-wide typedefs and compile-time switches.
 * ===========================================================================
 *
 * Every translation unit includes this first. It pulls in the freestanding
 * fixed-width integer / bool headers (these are part of the C standard library
 * but header-only, so they are safe even in the leanest environments) and
 * defines the two feature toggles that shape how the VM behaves.
 *
 * The whole project is a *teaching* bytecode VM in the spirit of Bob Nystrom's
 * "Crafting Interpreters" (clox), adapted for this lab in three ways:
 *   1. numbers are 64-bit SIGNED INTEGERS, not doubles — we are a systems lab,
 *      integer overflow and division-by-zero are the interesting edge cases;
 *   2. the interpreter's instruction dispatch uses COMPUTED GOTO (the GNU
 *      "labels as values" extension) when the compiler supports it, which is
 *      the single most performance-relevant trick in a bytecode VM;
 *   3. we deliberately stop short of closures/upvalues so the "teaching core"
 *      stays legible end to end (see README for the honest scope statement).
 * ===========================================================================
 */
#ifndef CLOXI_COMMON_H
#define CLOXI_COMMON_H

#include <stdbool.h>  /* bool, true, false — a typedef+macros header, no runtime */
#include <stddef.h>   /* size_t, NULL                                           */
#include <stdint.h>   /* uint8_t, int64_t, uintptr_t — exact-width integers     */

/* ---------------------------------------------------------------------------
 * DEBUG_TRACE_EXECUTION — when defined, the VM disassembles and prints every
 * instruction (and the live stack) as it runs. Invaluable for *seeing* the
 * stack machine work; catastrophically slow, so it is off by default. Flip it
 * on from the Makefile (make debug) or by editing this line.
 * --------------------------------------------------------------------------- */
/* #define DEBUG_TRACE_EXECUTION */

/* ---------------------------------------------------------------------------
 * DEBUG_STRESS_GC — when defined, the garbage collector runs on EVERY single
 * allocation instead of waiting for the heap to grow past a threshold. This is
 * how you flush out use-after-collect bugs: if any root is missing, a stress
 * run collects an object that is still live and the VM crashes immediately
 * instead of "one time in a thousand". Off by default (it makes the VM ~100x
 * slower); the GC self-test target turns it on.
 * --------------------------------------------------------------------------- */
/* #define DEBUG_STRESS_GC */

/* DEBUG_LOG_GC — narrate every mark/sweep/free so you can watch the collector
 * reclaim memory. Independent of STRESS_GC. */
/* #define DEBUG_LOG_GC */

/* ---------------------------------------------------------------------------
 * VM_COMPUTED_GOTO — 1 when we can use `&&label` / `goto *ptr`.
 *
 * "Labels as values" is a GCC extension also implemented by clang. It lets us
 * build a table of code addresses (one per opcode) and jump *directly* to the
 * next handler, replicating the dispatch branch at the tail of EVERY handler.
 * That spreads the single hard-to-predict indirect branch of a `switch` across
 * many sites, each with its own branch-predictor history, so the CPU actually
 * predicts "what usually follows an ADD" correctly. On a real workload this is
 * commonly a 15-25% speedup over a switch. MSVC lacks the extension, so we fall
 * back to a portable switch there — same semantics, slower dispatch.
 * --------------------------------------------------------------------------- */
#if defined(__GNUC__) || defined(__clang__)
#  define VM_COMPUTED_GOTO 1
#else
#  define VM_COMPUTED_GOTO 0
#endif

#endif /* CLOXI_COMMON_H */
