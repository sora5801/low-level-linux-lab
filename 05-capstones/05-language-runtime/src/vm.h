/* ===========================================================================
 * vm.h — the virtual machine: a stack-based bytecode interpreter + call stack.
 * ===========================================================================
 *
 * The VM is the runtime core (stand-in for the "back end" of sibling
 * 02-systems-tools/15-language-vm). It owns:
 *   - one shared VALUE STACK on which all computation happens;
 *   - a CALL-FRAME stack: each active function call is a window into the value
 *     stack, so a call allocates nothing on the C heap;
 *   - the GLOBALS table (name -> Value).
 *
 * There is ONE global VM instance, matching the one global Heap. A single global
 * keeps the GC's root enumeration simple (it reads `vm` directly) and mirrors how
 * clox and many teaching VMs are structured.
 */
#ifndef LUMEN_VM_H
#define LUMEN_VM_H

#include "object.h"
#include "table.h"
#include "value.h"

#define FRAMES_MAX 64                              /* max call depth (recursion) */
#define STACK_MAX  (FRAMES_MAX * UINT8_COUNT)      /* value-stack slots           */

/* A live function call. Because functions here do NOT capture their environment
 * (no closures — see README Scope), a frame needs only:
 *   function — the callee (its chunk holds the code + constants we execute);
 *   ip       — this call's instruction pointer (saved/restored across nested
 *              calls); kept in the frame so returning resumes exactly where we left;
 *   slots    — the base of this call's window on the value stack: slots[0] is the
 *              callee itself, slots[1..arity] are the arguments, and locals extend
 *              above them. Compile-time local "slots" are indices off this base. */
typedef struct {
    ObjFunction *function;
    uint8_t     *ip;
    Value       *slots;
} CallFrame;

typedef struct {
    CallFrame frames[FRAMES_MAX];   /* the call stack                          */
    int       frameCount;           /* number of active frames                 */

    Value  stack[STACK_MAX];        /* the value stack (grows upward)          */
    Value *stackTop;                /* points ONE PAST the top element         */

    Table  globals;                 /* global variables: name -> Value         */
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

extern VM vm;                       /* the one and only VM                     */

void            initVM(void);
void            freeVM(void);
InterpretResult interpret(const char *source);   /* compile + run one program  */

/* Stack primitives — also used by native functions and the GC's root walk. */
void  push(Value value);
Value pop(void);

#endif /* LUMEN_VM_H */
