/* ===========================================================================
 * jit.h — a tiny just-in-time compiler for one hot loop (add-on demo).
 * ===========================================================================
 *
 * Stand-in for sibling 02-systems-tools/12-jit-compiler. jitDemo() takes the same
 * "sum 1..n" loop the interpreter can run in bytecode, hand-emits x86-64 machine
 * code for it into an mmap'd page, flips the page from writable to executable
 * (W^X, via mprotect), calls into it through a function pointer using the SysV
 * AMD64 ABI, and checks the native result against the closed-form answer. It
 * prints what it did so you can see the emitted bytes.
 *
 * This is Linux/x86-64 only (it calls mmap/mprotect and executes raw bytes).
 * Returns 0 on success, non-zero if a syscall failed or the result mismatched.
 */
#ifndef LUMEN_JIT_H
#define LUMEN_JIT_H

#include <stdint.h>

int     jitDemo(int64_t n);                 /* compile+run "sum 1..n" natively  */
int64_t jitReferenceSum(int64_t n);         /* the answer the JIT must match    */

#endif /* LUMEN_JIT_H */
