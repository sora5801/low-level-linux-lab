/* ===========================================================================
 * vm.c — the stack-based bytecode interpreter, with computed-goto dispatch.
 * ===========================================================================
 *
 * This is the engine. run() is a fetch-decode-execute loop over one function's
 * bytecode. Everything else in this file (calls, errors, the stack, string
 * concatenation) exists to support that loop.
 *
 * WHY COMPUTED GOTO IS THE HEADLINE
 * ---------------------------------
 * A naive interpreter is `for (;;) switch (*ip++) { ... }`. The `switch`
 * compiles to ONE indirect branch shared by every opcode. That single branch
 * site has one branch-predictor slot, and since "the next opcode" is data, the
 * CPU mispredicts almost every time — and a mispredict on a modern core costs
 * ~15-20 cycles, often more than the opcode's real work.
 *
 * Computed goto ("labels as values", a GNU/clang extension) instead ends EACH
 * handler with its own `goto *table[*ip++]`. Now there are N branch sites, one
 * per opcode, and each accumulates its own history: the branch that follows
 * OP_GET_LOCAL learns that OP_GET_LOCAL is very often followed by OP_ADD, and
 * predicts it. Spreading the indirect branch across the handlers is the entire
 * trick, and it commonly buys 15-25% on real bytecode. We keep a portable
 * `switch` fallback (VM_COMPUTED_GOTO == 0) with identical semantics.
 *
 * THE ip CACHE
 * ------------
 * The current frame's instruction pointer is read on every single instruction,
 * so we cache it in a LOCAL variable `ip` that the compiler can keep in a
 * register, instead of chasing frame->ip through memory each time. The cost of
 * that speed is discipline: whenever control leaves this frame (a call) or an
 * error must report a line, we WRITE the local back to frame->ip first, and we
 * RELOAD it after. Every such site is commented.
 * ===========================================================================
 */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "memory.h"
#include "object.h"
#include "vm.h"

/* The single VM instance (declared extern in vm.h). One global keeps the GC's
 * root set trivially locatable and lets allocation helpers reach the stack. */
VM vm;

/* Reset the value stack and call stack to empty. stackTop == stack means the
 * stack holds nothing; frameCount 0 means no function is running. */
static void resetStack(void)
{
    vm.stackTop   = vm.stack;
    vm.frameCount = 0;
}

/* ---------------------------------------------------------------------------
 * runtimeError — print a message plus a full stack trace, then reset the stack.
 * Variadic so callers can format in the offending values. The trace walks the
 * frames from innermost out, mapping each frame's ip back to a source line via
 * the chunk's line table. The `-1` is because frame->ip already points at the
 * NEXT instruction, so the faulting one is the byte before it.
 * --------------------------------------------------------------------------- */
static void runtimeError(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    for (int i = vm.frameCount - 1; i >= 0; i--) {
        CallFrame   *frame    = &vm.frames[i];
        ObjFunction *function = frame->function;
        size_t instruction = (size_t)(frame->ip - function->chunk.code - 1);
        fprintf(stderr, "[line %d] in ", function->chunk.lines[instruction]);
        if (function->name == NULL) fprintf(stderr, "script\n");
        else                        fprintf(stderr, "%s()\n", function->name->chars);
    }

    resetStack();
}

void initVM(void)
{
    resetStack();
    vm.objects = NULL;

    /* GC accounting. Start the first collection threshold at 1 MiB so trivial
     * programs never collect, but a growing heap eventually does. */
    vm.bytesAllocated = 0;
    vm.nextGC         = 1024 * 1024;
    vm.grayCount      = 0;
    vm.grayCapacity   = 0;
    vm.grayStack      = NULL;

    initTable(&vm.globals);
    initTable(&vm.strings);
}

void freeVM(void)
{
    freeTable(&vm.globals);
    freeTable(&vm.strings);
    freeObjects();   /* frees every heap object AND the gray stack              */
}

/* --- the stack primitives -------------------------------------------------
 * push/pop point at the NEXT-FREE slot convention (see vm.h). No overflow guard
 * on push: the compiler statically bounds each function's stack depth and CALL
 * enforces the frame cap, so well-formed bytecode cannot overrun STACK_MAX. */
void push(Value value)  { *vm.stackTop = value; vm.stackTop++; }
Value pop(void)         { vm.stackTop--; return *vm.stackTop; }

/* Look at a value without popping. distance 0 == top, 1 == just below it. Used
 * by operators that must TYPE-CHECK operands before consuming them, and to keep
 * operands rooted across an allocation (concatenate). */
static Value peek(int distance) { return vm.stackTop[-1 - distance]; }

/* ---------------------------------------------------------------------------
 * call — push a new frame for `function`, checking arity and depth.
 * The frame's slot window begins at the callee itself: stackTop - argCount - 1
 * points at the function value, and the arguments sit just above it, so
 * slots[0] is the callee and slots[1..argCount] are the parameters — exactly
 * the layout OP_GET_LOCAL expects.
 * --------------------------------------------------------------------------- */
static bool call(ObjFunction *function, int argCount)
{
    if (argCount != function->arity) {
        runtimeError("Expected %d arguments but got %d.",
                     function->arity, argCount);
        return false;
    }
    if (vm.frameCount == FRAMES_MAX) {
        /* A definite, catchable overflow instead of smashing the C stack. */
        runtimeError("Stack overflow.");
        return false;
    }

    CallFrame *frame = &vm.frames[vm.frameCount++];
    frame->function = function;
    frame->ip       = function->chunk.code;          /* start at the first byte  */
    frame->slots    = vm.stackTop - argCount - 1;     /* window over the args     */
    return true;
}

/* Dispatch a call on a runtime value. Only functions are callable in this VM. */
static bool callValue(Value callee, int argCount)
{
    if (IS_OBJ(callee)) {
        switch (OBJ_TYPE(callee)) {
            case OBJ_FUNCTION: return call(AS_FUNCTION(callee), argCount);
            default: break;   /* strings etc. are not callable                  */
        }
    }
    runtimeError("Can only call functions.");
    return false;
}

/* Lox truthiness: only `nil` and `false` are falsey. Everything else — 0, "",
 * any object — is truthy. This is what OP_JUMP_IF_FALSE and OP_NOT consult. */
static bool isFalsey(Value value)
{
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

/* ---------------------------------------------------------------------------
 * concatenate — implement string `+`. Both operands are PEEKED (not popped) so
 * they stay on the stack, and therefore stay rooted, while we allocate the
 * result buffer and intern it — either allocation can trigger a GC. Only after
 * takeString returns do we pop the operands and push the result.
 * --------------------------------------------------------------------------- */
static void concatenate(void)
{
    ObjString *b = AS_STRING(peek(0));   /* right operand (top of stack)         */
    ObjString *a = AS_STRING(peek(1));   /* left operand                         */

    int   length = a->length + b->length;
    char *chars  = ALLOCATE(char, length + 1);          /* +1 for the NUL        */
    memcpy(chars,             a->chars, (size_t)a->length);
    memcpy(chars + a->length, b->chars, (size_t)b->length);
    chars[length] = '\0';

    ObjString *result = takeString(chars, length);       /* adopts `chars`       */
    pop();   /* b */
    pop();   /* a */
    push(OBJ_VAL(result));
}

#ifdef DEBUG_TRACE_EXECUTION
/* Print the live stack and disassemble the instruction about to run. Only
 * compiled in when tracing; keeps the hot loop clean otherwise. */
static void traceInstruction(CallFrame *frame, uint8_t *ip)
{
    printf("          ");
    for (Value *slot = vm.stack; slot < vm.stackTop; slot++) {
        printf("[ ");
        printValue(*slot);
        printf(" ]");
    }
    printf("\n");
    disassembleInstruction(&frame->function->chunk,
                           (int)(ip - frame->function->chunk.code));
}
#endif

/* ---------------------------------------------------------------------------
 * run — the interpreter core.
 * --------------------------------------------------------------------------- */
static InterpretResult run(void)
{
    /* The current frame and its cached instruction pointer (see the file header
     * for the write-back discipline). `frame` changes on call/return; `ip` is
     * the register-resident program counter. */
    CallFrame *frame = &vm.frames[vm.frameCount - 1];
    register uint8_t *ip = frame->ip;

    /* --- fetch helpers, all operating on the cached ip --- */
    /* READ_BYTE  : fetch one opcode/operand byte and advance.
     * READ_SHORT : fetch a 16-bit big-endian operand (jumps). The comma
     *              expression advances ip by 2 THEN reconstructs the value from
     *              the two bytes just passed (ip[-2] high, ip[-1] low).
     * READ_CONSTANT/READ_STRING : index the current function's constant pool. */
#define READ_BYTE()     (*ip++)
#define READ_SHORT()    (ip += 2, (uint16_t)((ip[-2] << 8) | ip[-1]))
#define READ_CONSTANT() (frame->function->chunk.constants.values[READ_BYTE()])
#define READ_STRING()   AS_STRING(READ_CONSTANT())

    /* Type-check then apply a binary INTEGER ARITHMETIC op. We compute in
     * uint64_t and cast back so signed overflow WRAPS with defined behavior
     * (signed overflow is UB in C; the unsigned round-trip is the standard,
     * warning-free way to get two's-complement wraparound). */
#define ARITH_OP(op)                                                         \
    do {                                                                     \
        if (!IS_INT(peek(0)) || !IS_INT(peek(1)))                            \
            RUNTIME_ERROR("Operands must be integers.");                     \
        int64_t b = AS_INT(pop());                                           \
        int64_t a = AS_INT(pop());                                           \
        push(INT_VAL((int64_t)((uint64_t)a op (uint64_t)b)));                \
    } while (0)

    /* Type-check then apply a binary INTEGER COMPARISON, pushing a bool. */
#define COMPARE_OP(op)                                                       \
    do {                                                                     \
        if (!IS_INT(peek(0)) || !IS_INT(peek(1)))                            \
            RUNTIME_ERROR("Operands must be integers.");                     \
        int64_t b = AS_INT(pop());                                           \
        int64_t a = AS_INT(pop());                                           \
        push(BOOL_VAL(a op b));                                              \
    } while (0)

    /* Raise a runtime error from inside the loop: sync the cached ip back so
     * runtimeError blames the right line, then bail out of run(). */
#define RUNTIME_ERROR(...)                                                   \
    do {                                                                     \
        frame->ip = ip;                                                      \
        runtimeError(__VA_ARGS__);                                           \
        return INTERPRET_RUNTIME_ERROR;                                      \
    } while (0)

    /* ---- DISPATCH MACHINERY ---------------------------------------------- */
#if VM_COMPUTED_GOTO
    /* A jump table from opcode -> handler address. `&&label` is the label's
     * address (GNU extension); a static array of them is explicitly permitted.
     * Designated initializers keep the table aligned to the OpCode enum even if
     * the enum is reordered. Any opcode missing here would be a NULL entry and
     * an instant crash, which is a useful "you forgot to wire up an op" tripwire. */
    static void *dispatchTable[] = {
        [OP_CONSTANT]      = &&L_OP_CONSTANT,
        [OP_NIL]           = &&L_OP_NIL,
        [OP_TRUE]          = &&L_OP_TRUE,
        [OP_FALSE]         = &&L_OP_FALSE,
        [OP_POP]           = &&L_OP_POP,
        [OP_GET_LOCAL]     = &&L_OP_GET_LOCAL,
        [OP_SET_LOCAL]     = &&L_OP_SET_LOCAL,
        [OP_GET_GLOBAL]    = &&L_OP_GET_GLOBAL,
        [OP_DEFINE_GLOBAL] = &&L_OP_DEFINE_GLOBAL,
        [OP_SET_GLOBAL]    = &&L_OP_SET_GLOBAL,
        [OP_EQUAL]         = &&L_OP_EQUAL,
        [OP_GREATER]       = &&L_OP_GREATER,
        [OP_LESS]          = &&L_OP_LESS,
        [OP_ADD]           = &&L_OP_ADD,
        [OP_SUBTRACT]      = &&L_OP_SUBTRACT,
        [OP_MULTIPLY]      = &&L_OP_MULTIPLY,
        [OP_DIVIDE]        = &&L_OP_DIVIDE,
        [OP_NEGATE]        = &&L_OP_NEGATE,
        [OP_NOT]           = &&L_OP_NOT,
        [OP_PRINT]         = &&L_OP_PRINT,
        [OP_JUMP]          = &&L_OP_JUMP,
        [OP_JUMP_IF_FALSE] = &&L_OP_JUMP_IF_FALSE,
        [OP_LOOP]          = &&L_OP_LOOP,
        [OP_CALL]          = &&L_OP_CALL,
        [OP_RETURN]        = &&L_OP_RETURN,
    };

#  ifdef DEBUG_TRACE_EXECUTION
#    define TRACE() traceInstruction(frame, ip)
#  else
#    define TRACE() ((void)0)
#  endif

    /* The heart: trace (maybe), fetch the next opcode, and jump straight to its
     * handler. This exact three-line sequence is emitted at the end of EVERY
     * handler (via VM_NEXT), which is what gives each opcode its own predicted
     * indirect branch. */
#  define DISPATCH()   do { TRACE(); goto *dispatchTable[READ_BYTE()]; } while (0)
#  define VM_CASE(name) L_##name:
#  define VM_NEXT       DISPATCH()

    DISPATCH();                     /* enter the loop by dispatching op #0        */

#else  /* portable fallback: one shared switch --------------------------------*/
#  ifdef DEBUG_TRACE_EXECUTION
#    define TRACE() traceInstruction(frame, ip)
#  else
#    define TRACE() ((void)0)
#  endif
#  define VM_CASE(name) case name:
#  define VM_NEXT       break                  /* fall back to the for/switch top */

    for (;;) {
        TRACE();
        switch (READ_BYTE()) {
#endif

    /* ===================== THE OPCODE HANDLERS ==========================
     * From here to the closing of the switch/labels, the code is IDENTICAL for
     * both dispatch strategies — only the CASE/NEXT macros differ. Each handler
     * consumes its operands from the stack and leaves its result there. */

        VM_CASE(OP_CONSTANT) {
            Value constant = READ_CONSTANT();
            push(constant);                     /* load a literal from the pool   */
            VM_NEXT;
        }
        VM_CASE(OP_NIL)   { push(NIL_VAL);          VM_NEXT; }
        VM_CASE(OP_TRUE)  { push(BOOL_VAL(true));   VM_NEXT; }
        VM_CASE(OP_FALSE) { push(BOOL_VAL(false));  VM_NEXT; }
        VM_CASE(OP_POP)   { pop();                  VM_NEXT; }

        VM_CASE(OP_GET_LOCAL) {
            /* A local is just a stack slot in this frame's window. We PUSH a
             * copy to the top because every value an expression consumes must be
             * on top — locals live at a fixed depth and are read by copying up. */
            uint8_t slot = READ_BYTE();
            push(frame->slots[slot]);
            VM_NEXT;
        }
        VM_CASE(OP_SET_LOCAL) {
            /* Assignment is an expression whose value is the assigned value, so
             * we PEEK (leave it on top) and write it into the slot. */
            uint8_t slot = READ_BYTE();
            frame->slots[slot] = peek(0);
            VM_NEXT;
        }

        VM_CASE(OP_GET_GLOBAL) {
            ObjString *name = READ_STRING();
            Value value;
            if (!tableGet(&vm.globals, name, &value))
                RUNTIME_ERROR("Undefined variable '%s'.", name->chars);
            push(value);
            VM_NEXT;
        }
        VM_CASE(OP_DEFINE_GLOBAL) {
            /* Bind name->value. We read the value with peek and only pop AFTER
             * tableSet completes: tableSet can allocate (grow the table) and
             * thus GC, so the value must stay rooted on the stack until it is
             * safely stored in the globals table. */
            ObjString *name = READ_STRING();
            tableSet(&vm.globals, name, peek(0));
            pop();
            VM_NEXT;
        }
        VM_CASE(OP_SET_GLOBAL) {
            /* Assigning to an UNDEFINED global is an error (Lox has no implicit
             * global creation via assignment). tableSet returns true when it
             * created a NEW key — so if it did, we must undo it and complain. */
            ObjString *name = READ_STRING();
            if (tableSet(&vm.globals, name, peek(0))) {
                tableDelete(&vm.globals, name);
                RUNTIME_ERROR("Undefined variable '%s'.", name->chars);
            }
            /* NB: no pop — assignment yields the value, leaving it on the stack. */
            VM_NEXT;
        }

        VM_CASE(OP_EQUAL) {
            Value b = pop();
            Value a = pop();
            push(BOOL_VAL(valuesEqual(a, b)));   /* works across all types        */
            VM_NEXT;
        }
        VM_CASE(OP_GREATER) { COMPARE_OP(>); VM_NEXT; }
        VM_CASE(OP_LESS)    { COMPARE_OP(<); VM_NEXT; }

        VM_CASE(OP_ADD) {
            /* `+` is overloaded: numeric add OR string concatenation, chosen by
             * the operand types. Any other combination is a runtime type error. */
            if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                concatenate();
            } else if (IS_INT(peek(0)) && IS_INT(peek(1))) {
                int64_t b = AS_INT(pop());
                int64_t a = AS_INT(pop());
                push(INT_VAL((int64_t)((uint64_t)a + (uint64_t)b)));
            } else {
                RUNTIME_ERROR("Operands must be two integers or two strings.");
            }
            VM_NEXT;
        }
        VM_CASE(OP_SUBTRACT) { ARITH_OP(-); VM_NEXT; }
        VM_CASE(OP_MULTIPLY) { ARITH_OP(*); VM_NEXT; }
        VM_CASE(OP_DIVIDE) {
            if (!IS_INT(peek(0)) || !IS_INT(peek(1)))
                RUNTIME_ERROR("Operands must be integers.");
            int64_t b = AS_INT(pop());
            int64_t a = AS_INT(pop());
            /* Two hardware traps made explicit. On x86-64 the IDIV instruction
             * raises #DE both for a zero divisor AND for INT64_MIN / -1 (whose
             * true quotient, +2^63, is unrepresentable). We check for both so
             * the language reports a clean runtime error instead of taking a
             * SIGFPE — and so we avoid the C-level undefined behavior. */
            if (b == 0)
                RUNTIME_ERROR("Division by zero.");
            if (a == INT64_MIN && b == -1)
                RUNTIME_ERROR("Integer overflow in division.");
            push(INT_VAL(a / b));
            VM_NEXT;
        }
        VM_CASE(OP_NEGATE) {
            if (!IS_INT(peek(0)))
                RUNTIME_ERROR("Operand must be an integer.");
            /* Negate via unsigned wrap so -INT64_MIN is defined (it wraps back
             * to INT64_MIN, matching two's-complement hardware NEG). */
            int64_t v = AS_INT(pop());
            push(INT_VAL((int64_t)(0 - (uint64_t)v)));
            VM_NEXT;
        }
        VM_CASE(OP_NOT) {
            push(BOOL_VAL(isFalsey(pop())));     /* logical !                     */
            VM_NEXT;
        }

        VM_CASE(OP_PRINT) {
            printValue(pop());
            printf("\n");
            VM_NEXT;
        }

        VM_CASE(OP_JUMP) {
            /* Unconditional forward jump: move ip by the 16-bit operand. */
            uint16_t offset = READ_SHORT();
            ip += offset;
            VM_NEXT;
        }
        VM_CASE(OP_JUMP_IF_FALSE) {
            /* Conditional jump. Reads the operand ALWAYS (so ip advances past it
             * either way) and only moves ip when the condition (which it PEEKS,
             * leaving it for a following OP_POP) is falsey. */
            uint16_t offset = READ_SHORT();
            if (isFalsey(peek(0))) ip += offset;
            VM_NEXT;
        }
        VM_CASE(OP_LOOP) {
            uint16_t offset = READ_SHORT();
            ip -= offset;                        /* backward jump (loops)         */
            VM_NEXT;
        }

        VM_CASE(OP_CALL) {
            int argCount = READ_BYTE();
            /* WRITE-BACK: the callee will run with its OWN ip; persist ours into
             * the frame so that (a) runtimeError can read a correct line if the
             * call fails and (b) we resume here correctly after the callee
             * returns. */
            frame->ip = ip;
            if (!callValue(peek(argCount), argCount))
                return INTERPRET_RUNTIME_ERROR;   /* callValue already reported   */
            /* SWITCH FRAMES: the new current frame is the callee's. Reload the
             * cached ip from it. */
            frame = &vm.frames[vm.frameCount - 1];
            ip    = frame->ip;
            VM_NEXT;
        }
        VM_CASE(OP_RETURN) {
            Value result = pop();                /* the function's result value   */
            vm.frameCount--;
            if (vm.frameCount == 0) {
                /* Returned from the top-level <script>: pop its slot and we are
                 * done. */
                pop();
                return INTERPRET_OK;
            }
            /* Discard the callee's whole window (its slot 0 + args + locals) by
             * resetting stackTop to where the frame began, then push the result
             * so the caller finds it on top. */
            vm.stackTop = frame->slots;
            push(result);
            frame = &vm.frames[vm.frameCount - 1];   /* back to the caller        */
            ip    = frame->ip;                        /* resume where it left off  */
            VM_NEXT;
        }

#if VM_COMPUTED_GOTO
    /* No default label is needed: an out-of-range opcode would index past the
     * table, which cannot happen for compiler-emitted code. In the switch
     * version we DO add a default for -Wswitch friendliness. */
#else
        default:
            RUNTIME_ERROR("Unknown opcode %d.", ip[-1]);
        }   /* close switch */
    }       /* close for(;;) */
#endif

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
#undef ARITH_OP
#undef COMPARE_OP
#undef RUNTIME_ERROR
#undef TRACE
#undef DISPATCH
#undef VM_CASE
#undef VM_NEXT
}

/* ---------------------------------------------------------------------------
 * interpret — the public entry point: compile source to a function, then run.
 * --------------------------------------------------------------------------- */
InterpretResult interpret(const char *source)
{
    ObjFunction *function = compile(source);
    if (function == NULL) return INTERPRET_COMPILE_ERROR;   /* front-end failed  */

    /* Root the script function on the stack, then set up its call frame. It
     * occupies slot 0 of its own frame (the reserved callee slot), exactly like
     * any called function. */
    push(OBJ_VAL(function));
    call(function, 0);

    return run();
}
