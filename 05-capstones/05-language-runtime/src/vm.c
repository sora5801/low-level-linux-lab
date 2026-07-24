/* ===========================================================================
 * vm.c — the stack-based bytecode interpreter, with computed-goto dispatch.
 * ===========================================================================
 *
 * THE DISPATCH LOOP is the performance heart of any bytecode VM. A naive
 * interpreter is one big `switch` inside a `for(;;)`: after every handler it
 * jumps back to the SAME switch, so the CPU's branch predictor sees a single
 * indirect branch and can't learn the opcode-to-opcode correlations real programs
 * have (a compare is usually followed by a jump, etc.). COMPUTED GOTO ("labels as
 * values", a GNU/clang extension) instead ends EACH handler with its own
 * `goto *table[opcode]`. Now there are N branch sites — one per opcode — so the
 * predictor can specialize each, typically a large speedup. We keep a portable
 * `switch` fallback for non-GNU compilers so the code still builds anywhere; the
 * two share one copy of every handler via the VM_CASE/VM_NEXT macros below.
 *
 * See asm/demo.annotated.s to watch the indirect jump in the generated assembly.
 */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "compiler.h"
#include "gc.h"
#include "heap.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include "vm.h"

/* The single global VM (declared extern in vm.h). */
VM vm;

/* ---- Stack primitives -----------------------------------------------------*/

static void resetStack(void)
{
    vm.stackTop   = vm.stack;   /* empty stack: top == base                     */
    vm.frameCount = 0;
}
void  push(Value value) { *vm.stackTop = value; vm.stackTop++; }
Value pop(void)         { vm.stackTop--; return *vm.stackTop; }
/* Peek `distance` items below the top WITHOUT popping — used so operands stay on
 * the stack (and thus remain GC roots) until an operation fully succeeds. */
static Value peek(int distance) { return vm.stackTop[-1 - distance]; }

/* ---- Errors ---------------------------------------------------------------*/

/* Print a runtime error plus a stack trace (innermost call first), then reset the
 * stack so the REPL can continue. */
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
        /* ip points at the NEXT instruction; -1 gets the one that faulted. */
        size_t instr = (size_t)(frame->ip - function->chunk.code - 1);
        fprintf(stderr, "[line %d] in ", function->chunk.lines[instr]);
        if (function->name == NULL) fprintf(stderr, "script\n");
        else                        fprintf(stderr, "%s()\n", function->name->chars);
    }
    resetStack();
}

/* ---- Native functions -----------------------------------------------------*/

/* clock(): CPU seconds since process start — enough to time the hot loop. */
static Value clockNative(int argCount, Value *args)
{
    (void)argCount; (void)args;
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

/* Register a builtin as a global. The name and native are pushed onto the stack
 * first so they stay rooted across tableSet's (possible) allocation. */
static void defineNative(const char *name, NativeFn function)
{
    push(OBJ_VAL(copyString(name, (int)strlen(name))));
    push(OBJ_VAL(newNative(function)));
    tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
    pop();
    pop();
}

/* ---- Calls ----------------------------------------------------------------*/

/* Push a new call frame for `function`. The frame's window on the value stack
 * starts at the callee itself (slot 0), so `slots = stackTop - argCount - 1`.
 * This carves a window out of the ONE shared stack — a call allocates nothing. */
static bool call(ObjFunction *function, int argCount)
{
    if (argCount != function->arity) {
        runtimeError("Expected %d arguments but got %d.", function->arity, argCount);
        return false;
    }
    if (vm.frameCount == FRAMES_MAX) {
        runtimeError("Stack overflow.");
        return false;
    }
    CallFrame *frame = &vm.frames[vm.frameCount++];
    frame->function = function;
    frame->ip       = function->chunk.code;      /* start at the first opcode    */
    frame->slots    = vm.stackTop - argCount - 1;
    return true;
}

static bool callValue(Value callee, int argCount)
{
    if (IS_OBJ(callee)) {
        switch (OBJ_TYPE(callee)) {
        case OBJ_FUNCTION:
            return call(AS_FUNCTION(callee), argCount);
        case OBJ_NATIVE: {
            NativeFn native = AS_NATIVE(callee);
            /* Native runs immediately over its args in place; then we drop the
             * args + the callee and push the single result. */
            Value result = native(argCount, vm.stackTop - argCount);
            vm.stackTop -= argCount + 1;
            push(result);
            return true;
        }
        default: break;   /* strings aren't callable */
        }
    }
    runtimeError("Can only call functions.");
    return false;
}

/* Everything except `false` and `nil` is truthy (0 and "" are truthy). */
static bool isFalsey(Value value)
{
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

/* Concatenate the two strings on top of the stack. We PEEK (not pop) the operands
 * so they stay rooted while copyString allocates — a GC mid-allocation must not
 * reclaim the inputs. Only after the result exists do we pop and push. */
static void concatenate(void)
{
    ObjString *b = AS_STRING(peek(0));
    ObjString *a = AS_STRING(peek(1));
    int   length = a->length + b->length;

    /* Build the joined bytes in a libc temp (not a GC object), then copy into a
     * fresh ObjString. The temp keeps concatenation from needing a half-built
     * object on the heap. */
    char *temp = (char *)malloc((size_t)length + 1);
    if (temp == NULL) { runtimeError("Out of memory (concatenate)."); return; }
    memcpy(temp, a->chars, (size_t)a->length);
    memcpy(temp + a->length, b->chars, (size_t)b->length);
    temp[length] = '\0';

    ObjString *result = copyString(temp, length);   /* may trigger GC; a,b rooted */
    free(temp);

    pop();                                           /* b */
    pop();                                           /* a */
    push(OBJ_VAL(result));
}

/* ===========================================================================
 * Disassembler (used by DEBUG_PRINT_CODE / DEBUG_TRACE_EXECUTION). Non-static so
 * compiler.c can call disassembleChunk under DEBUG_PRINT_CODE.
 * =========================================================================== */
static int simpleInstruction(const char *name, int offset)
{
    printf("%s\n", name);
    return offset + 1;
}
static int constantInstruction(const char *name, Chunk *chunk, int offset)
{
    uint8_t constant = chunk->code[offset + 1];
    printf("%-16s %4d '", name, constant);
    printValue(chunk->constants.values[constant]);
    printf("'\n");
    return offset + 2;
}
static int byteInstruction(const char *name, Chunk *chunk, int offset)
{
    uint8_t slot = chunk->code[offset + 1];
    printf("%-16s %4d\n", name, slot);
    return offset + 2;
}
static int jumpInstruction(const char *name, int sign, Chunk *chunk, int offset)
{
    uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
    jump |= chunk->code[offset + 2];
    printf("%-16s %4d -> %d\n", name, offset, offset + 3 + sign * (int)jump);
    return offset + 3;
}

int disassembleInstruction(Chunk *chunk, int offset)
{
    printf("%04d ", offset);
    if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1])
        printf("   | ");                    /* same source line as previous op    */
    else
        printf("%4d ", chunk->lines[offset]);

    uint8_t instruction = chunk->code[offset];
    switch (instruction) {
    case OP_CONSTANT:      return constantInstruction("OP_CONSTANT", chunk, offset);
    case OP_NIL:           return simpleInstruction("OP_NIL", offset);
    case OP_TRUE:          return simpleInstruction("OP_TRUE", offset);
    case OP_FALSE:         return simpleInstruction("OP_FALSE", offset);
    case OP_POP:           return simpleInstruction("OP_POP", offset);
    case OP_GET_LOCAL:     return byteInstruction("OP_GET_LOCAL", chunk, offset);
    case OP_SET_LOCAL:     return byteInstruction("OP_SET_LOCAL", chunk, offset);
    case OP_GET_GLOBAL:    return constantInstruction("OP_GET_GLOBAL", chunk, offset);
    case OP_DEFINE_GLOBAL: return constantInstruction("OP_DEFINE_GLOBAL", chunk, offset);
    case OP_SET_GLOBAL:    return constantInstruction("OP_SET_GLOBAL", chunk, offset);
    case OP_EQUAL:         return simpleInstruction("OP_EQUAL", offset);
    case OP_GREATER:       return simpleInstruction("OP_GREATER", offset);
    case OP_LESS:          return simpleInstruction("OP_LESS", offset);
    case OP_ADD:           return simpleInstruction("OP_ADD", offset);
    case OP_SUBTRACT:      return simpleInstruction("OP_SUBTRACT", offset);
    case OP_MULTIPLY:      return simpleInstruction("OP_MULTIPLY", offset);
    case OP_DIVIDE:        return simpleInstruction("OP_DIVIDE", offset);
    case OP_NOT:           return simpleInstruction("OP_NOT", offset);
    case OP_NEGATE:        return simpleInstruction("OP_NEGATE", offset);
    case OP_PRINT:         return simpleInstruction("OP_PRINT", offset);
    case OP_JUMP:          return jumpInstruction("OP_JUMP", 1, chunk, offset);
    case OP_JUMP_IF_FALSE: return jumpInstruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
    case OP_LOOP:          return jumpInstruction("OP_LOOP", -1, chunk, offset);
    case OP_CALL:          return byteInstruction("OP_CALL", chunk, offset);
    case OP_RETURN:        return simpleInstruction("OP_RETURN", offset);
    default:
        printf("Unknown opcode %d\n", instruction);
        return offset + 1;
    }
}
void disassembleChunk(Chunk *chunk, const char *name)
{
    printf("== %s ==\n", name);
    for (int offset = 0; offset < chunk->count; )
        offset = disassembleInstruction(chunk, offset);
}

/* ===========================================================================
 * run() — execute the current call frame's bytecode to completion.
 * =========================================================================== */
static InterpretResult run(void)
{
    /* Now that a program is loaded and its roots (frame 0's function, plus the
     * value stack) are live, allow the collector to fire on allocation. */
    heap.gcEnabled = true;

    CallFrame *frame = &vm.frames[vm.frameCount - 1];

    /* Operand/reader helpers, defined in terms of the CURRENT frame's ip. */
#define READ_BYTE()     (*frame->ip++)
#define READ_SHORT()    (frame->ip += 2, \
                         (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_CONSTANT() (frame->function->chunk.constants.values[READ_BYTE()])
#define READ_STRING()   AS_STRING(READ_CONSTANT())
    /* A numeric binary op: type-check both operands (they're peeked, so still
     * rooted), then pop, compute, push. `valueType` wraps the C result. */
#define BINARY_OP(valueType, op)                                            \
        do {                                                                \
            if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {               \
                runtimeError("Operands must be numbers.");                  \
                return INTERPRET_RUNTIME_ERROR;                             \
            }                                                               \
            double b = AS_NUMBER(pop());                                    \
            double a = AS_NUMBER(pop());                                    \
            push(valueType(a op b));                                        \
        } while (0)

#ifdef DEBUG_TRACE_EXECUTION
    /* Before each op: dump the stack and disassemble the instruction. */
#  define TRACE()                                                           \
        do {                                                                \
            printf("          ");                                           \
            for (Value *s = vm.stack; s < vm.stackTop; s++) {               \
                printf("[ "); printValue(*s); printf(" ]");                 \
            }                                                               \
            printf("\n");                                                   \
            disassembleInstruction(&frame->function->chunk,                 \
                (int)(frame->ip - frame->function->chunk.code));            \
        } while (0)
#else
#  define TRACE() ((void)0)
#endif

    /* ---- Dispatch machinery: computed goto if available, else a switch. --- */
#ifdef __GNUC__
    /* One label per opcode; `&&label` is the GNU "address of a label" operator.
     * Designated initializers key each slot by opcode so order can't drift. */
    static void *dispatchTable[] = {
        [OP_CONSTANT]      = &&do_OP_CONSTANT,
        [OP_NIL]           = &&do_OP_NIL,
        [OP_TRUE]          = &&do_OP_TRUE,
        [OP_FALSE]         = &&do_OP_FALSE,
        [OP_POP]           = &&do_OP_POP,
        [OP_GET_LOCAL]     = &&do_OP_GET_LOCAL,
        [OP_SET_LOCAL]     = &&do_OP_SET_LOCAL,
        [OP_GET_GLOBAL]    = &&do_OP_GET_GLOBAL,
        [OP_DEFINE_GLOBAL] = &&do_OP_DEFINE_GLOBAL,
        [OP_SET_GLOBAL]    = &&do_OP_SET_GLOBAL,
        [OP_EQUAL]         = &&do_OP_EQUAL,
        [OP_GREATER]       = &&do_OP_GREATER,
        [OP_LESS]          = &&do_OP_LESS,
        [OP_ADD]           = &&do_OP_ADD,
        [OP_SUBTRACT]      = &&do_OP_SUBTRACT,
        [OP_MULTIPLY]      = &&do_OP_MULTIPLY,
        [OP_DIVIDE]        = &&do_OP_DIVIDE,
        [OP_NOT]           = &&do_OP_NOT,
        [OP_NEGATE]        = &&do_OP_NEGATE,
        [OP_PRINT]         = &&do_OP_PRINT,
        [OP_JUMP]          = &&do_OP_JUMP,
        [OP_JUMP_IF_FALSE] = &&do_OP_JUMP_IF_FALSE,
        [OP_LOOP]          = &&do_OP_LOOP,
        [OP_CALL]          = &&do_OP_CALL,
        [OP_RETURN]        = &&do_OP_RETURN,
    };
    /* The indirect jump that IS the dispatch. Replicated at the end of every
     * handler (via VM_NEXT), giving the predictor one site per opcode. */
#  define DISPATCH()  do { TRACE(); goto *dispatchTable[READ_BYTE()]; } while (0)
#  define VM_LOOP     DISPATCH();
    /* Note: the ':' that turns this into a label is written at each call site,
     * e.g. `VM_CASE(OP_ADD): { ... }`. */
#  define VM_CASE(op) do_##op
#  define VM_NEXT     DISPATCH()
#else
    /* Portable fallback: a classic decode-and-switch loop. Same handler bodies. */
#  define VM_LOOP     vm_loop: TRACE(); switch (READ_BYTE())
#  define VM_CASE(op) case op
#  define VM_NEXT     goto vm_loop
#endif

    VM_LOOP
    {
        VM_CASE(OP_CONSTANT): { Value c = READ_CONSTANT(); push(c); VM_NEXT; }
        VM_CASE(OP_NIL):      { push(NIL_VAL);          VM_NEXT; }
        VM_CASE(OP_TRUE):     { push(BOOL_VAL(true));   VM_NEXT; }
        VM_CASE(OP_FALSE):    { push(BOOL_VAL(false));  VM_NEXT; }
        VM_CASE(OP_POP):      { pop();                  VM_NEXT; }

        VM_CASE(OP_GET_LOCAL): {
            uint8_t slot = READ_BYTE();
            push(frame->slots[slot]);        /* locals are stack slots off `slots` */
            VM_NEXT;
        }
        VM_CASE(OP_SET_LOCAL): {
            uint8_t slot = READ_BYTE();
            frame->slots[slot] = peek(0);    /* assignment is an expression: keep it*/
            VM_NEXT;
        }
        VM_CASE(OP_GET_GLOBAL): {
            ObjString *name = READ_STRING();
            Value value;
            if (!tableGet(&vm.globals, name, &value)) {
                runtimeError("Undefined variable '%s'.", name->chars);
                return INTERPRET_RUNTIME_ERROR;
            }
            push(value);
            VM_NEXT;
        }
        VM_CASE(OP_DEFINE_GLOBAL): {
            ObjString *name = READ_STRING();
            tableSet(&vm.globals, name, peek(0));  /* peek: keep rooted til stored  */
            pop();
            VM_NEXT;
        }
        VM_CASE(OP_SET_GLOBAL): {
            ObjString *name = READ_STRING();
            /* tableSet returns true if the key was NEW — assigning to a variable
             * that was never declared is an error, so undo and report. */
            if (tableSet(&vm.globals, name, peek(0))) {
                tableDelete(&vm.globals, name);
                runtimeError("Undefined variable '%s'.", name->chars);
                return INTERPRET_RUNTIME_ERROR;
            }
            VM_NEXT;
        }

        VM_CASE(OP_EQUAL): {
            Value b = pop(), a = pop();
            push(BOOL_VAL(valuesEqual(a, b)));
            VM_NEXT;
        }
        VM_CASE(OP_GREATER): { BINARY_OP(BOOL_VAL, >); VM_NEXT; }
        VM_CASE(OP_LESS):    { BINARY_OP(BOOL_VAL, <); VM_NEXT; }

        VM_CASE(OP_ADD): {
            /* `+` is overloaded: string concat OR numeric add. */
            if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                concatenate();
            } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                double b = AS_NUMBER(pop());
                double a = AS_NUMBER(pop());
                push(NUMBER_VAL(a + b));
            } else {
                runtimeError("Operands must be two numbers or two strings.");
                return INTERPRET_RUNTIME_ERROR;
            }
            VM_NEXT;
        }
        VM_CASE(OP_SUBTRACT): { BINARY_OP(NUMBER_VAL, -); VM_NEXT; }
        VM_CASE(OP_MULTIPLY): { BINARY_OP(NUMBER_VAL, *); VM_NEXT; }
        VM_CASE(OP_DIVIDE):   { BINARY_OP(NUMBER_VAL, /); VM_NEXT; }

        VM_CASE(OP_NOT): { push(BOOL_VAL(isFalsey(pop()))); VM_NEXT; }
        VM_CASE(OP_NEGATE): {
            if (!IS_NUMBER(peek(0))) {
                runtimeError("Operand must be a number.");
                return INTERPRET_RUNTIME_ERROR;
            }
            push(NUMBER_VAL(-AS_NUMBER(pop())));
            VM_NEXT;
        }
        VM_CASE(OP_PRINT): { printValue(pop()); printf("\n"); VM_NEXT; }

        VM_CASE(OP_JUMP): {
            uint16_t offset = READ_SHORT();
            frame->ip += offset;             /* unconditional forward jump         */
            VM_NEXT;
        }
        VM_CASE(OP_JUMP_IF_FALSE): {
            uint16_t offset = READ_SHORT();
            if (isFalsey(peek(0))) frame->ip += offset;   /* condition left on stack*/
            VM_NEXT;
        }
        VM_CASE(OP_LOOP): {
            uint16_t offset = READ_SHORT();
            frame->ip -= offset;             /* backward jump (loop back-edge)     */
            VM_NEXT;
        }

        VM_CASE(OP_CALL): {
            int argCount = READ_BYTE();
            if (!callValue(peek(argCount), argCount))
                return INTERPRET_RUNTIME_ERROR;
            frame = &vm.frames[vm.frameCount - 1];   /* switch to the callee's frame*/
            VM_NEXT;
        }
        VM_CASE(OP_RETURN): {
            Value result = pop();            /* the return value                   */
            vm.frameCount--;
            if (vm.frameCount == 0) {        /* returning from <script>: done      */
                pop();                       /* pop the script function itself     */
                return INTERPRET_OK;
            }
            vm.stackTop = frame->slots;      /* discard the callee's whole window  */
            push(result);                    /* hand the result to the caller      */
            frame = &vm.frames[vm.frameCount - 1];
            VM_NEXT;
        }
    }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
#undef TRACE
#undef DISPATCH
#undef VM_LOOP
#undef VM_CASE
#undef VM_NEXT
    /* Unreachable: every handler dispatches or returns. Present so the switch
     * fallback (and picky compilers) see a definite return on all paths. */
    return INTERPRET_RUNTIME_ERROR;
}

/* ---- Public API -----------------------------------------------------------*/

void initVM(void)
{
    heapInit();                 /* the GC heap must exist before any allocation  */
    resetStack();
    initTable(&vm.globals);
    defineNative("clock", clockNative);
}

void freeVM(void)
{
    freeTable(&vm.globals);     /* the entries array (libc)                      */
    freeAllObjects();           /* every heap object + the gray stack            */
    heapShutdown();             /* munmap the arenas                             */
}

InterpretResult interpret(const char *source)
{
    /* Compile with the collector OFF: half-built compiler objects aren't rooted,
     * so a GC here could free them. run() turns it back on once roots are live.
     * (This reset matters for the REPL, where interpret() is called repeatedly.) */
    heap.gcEnabled = false;

    ObjFunction *function = compile(source);
    if (function == NULL) return INTERPRET_COMPILE_ERROR;

    push(OBJ_VAL(function));    /* root the script function                      */
    call(function, 0);          /* set up frame 0 (arity 0, can't fail here)     */
    return run();
}
