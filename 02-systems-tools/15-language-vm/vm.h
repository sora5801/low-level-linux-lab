/* ===========================================================================
 * vm.h — the virtual machine: the value stack, call frames, and the globals.
 * ===========================================================================
 *
 * This is a STACK MACHINE. There are no registers in the bytecode; every
 * instruction takes its operands from, and leaves its result on, a single
 * value stack. `1 + 2 * 3` compiles to: push 1, push 2, push 3, MULTIPLY,
 * ADD — the stack does all the bookkeeping the operator precedence implied.
 *
 * FUNCTION CALLS use a frame stack layered ON TOP of the value stack. Each
 * CallFrame records which function is running, its instruction pointer, and a
 * `slots` pointer to the function's WINDOW into the shared value stack. Local
 * variable N is simply slots[N]; arguments are the first locals. Because frames
 * carve windows out of one contiguous stack, a call pushes no new allocation —
 * it just advances a pointer. That is why a deep-but-bounded recursion is cheap.
 * ===========================================================================
 */
#ifndef CLOXI_VM_H
#define CLOXI_VM_H

#include "chunk.h"
#include "object.h"
#include "table.h"
#include "value.h"

/* Frame depth cap and the derived value-stack size. 64 frames is plenty for a
 * teaching VM and gives a definite, testable "Stack overflow" instead of
 * smashing C's own stack. STACK_MAX is sized so every frame can use a full
 * 256-slot local window without the value stack overrunning. */
#define FRAMES_MAX 64
#define STACK_MAX  (FRAMES_MAX * 256)

/* ---------------------------------------------------------------------------
 * CallFrame — one activation record.
 *   function : the ObjFunction whose chunk this frame executes (its chunk holds
 *              the bytecode and constants the ip/READ_CONSTANT walk).
 *   ip       : the RETURN address into `function->chunk.code` — i.e. the next
 *              instruction to execute when control comes back to this frame.
 *              While a frame is the *current* one, the VM caches ip in a local
 *              register for speed and writes it back here before any call.
 *   slots    : points at this frame's first slot in vm.stack; slots[0] is the
 *              callee itself, slots[1..arity] are the arguments, and locals
 *              declared in the body extend above them.
 * --------------------------------------------------------------------------- */
typedef struct {
    ObjFunction *function;
    uint8_t     *ip;
    Value       *slots;
} CallFrame;

typedef struct {
    /* The call stack of active frames. frames[frameCount-1] is current. */
    CallFrame frames[FRAMES_MAX];
    int       frameCount;

    /* The value stack. `stackTop` points at the NEXT free slot (so an empty
     * stack has stackTop == stack, and *(-1) is the top element). Pointing at
     * the free slot makes push = "*stackTop++ = v" and pop = "*--stackTop". */
    Value  stack[STACK_MAX];
    Value *stackTop;

    Table globals;   /* name -> value for global variables                    */
    Table strings;   /* the intern table (a weak set of every live ObjString) */

    /* GC bookkeeping. `objects` heads the intrusive list of every allocation.
     * The gray stack is the mark phase's worklist of reachable-but-not-yet-
     * scanned objects (an explicit stack instead of recursion, so a deep object
     * graph cannot blow the C stack). It is managed with raw malloc/realloc on
     * purpose: growing it must NOT itself trigger a collection. */
    Obj   *objects;
    size_t bytesAllocated;   /* live bytes, maintained by reallocate()         */
    size_t nextGC;           /* collect once bytesAllocated exceeds this       */
    int    grayCount;
    int    grayCapacity;
    Obj  **grayStack;
} VM;

/* The interpreter reports one of these to the caller (main.c sets the process
 * exit code from it, matching the `sysexits.h` convention). */
typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,   /* front-end error; nothing ran                 */
    INTERPRET_RUNTIME_ERROR,   /* a trap during execution (type error, etc.)   */
} InterpretResult;

/* One global VM instance. A singleton keeps the GC's root-walking simple (it
 * knows exactly one stack and one globals table to scan) and lets allocation
 * helpers reach the VM without threading a context pointer everywhere. The
 * `extern` here + one definition in vm.c is the standard shared-global idiom. */
extern VM vm;

void initVM(void);
void freeVM(void);

/* Compile `source` and run it. The top-level entry point used by main.c. */
InterpretResult interpret(const char *source);

/* The stack primitives, exposed because the compiler-adjacent code (string
 * concatenation, constant interning) pushes temporaries to keep them rooted
 * across allocations. push has no overflow check on this fast path — the
 * compiler statically bounds each function's stack usage, and CALL enforces the
 * frame cap, so a well-formed chunk cannot overrun. */
void  push(Value value);
Value pop(void);

#endif /* CLOXI_VM_H */
