/* ===========================================================================
 * compiler.c — a single-pass Pratt (precedence-climbing) compiler.
 * ===========================================================================
 *
 * There is NO abstract syntax tree. The compiler pulls tokens from the scanner
 * and emits bytecode as it goes, in one left-to-right pass. Expressions are
 * handled by "Pratt parsing": every token type has a rule saying how it behaves
 * as a PREFIX operator (a literal, a unary minus, a grouping paren) and/or as an
 * INFIX operator (binary +, function-call parens), plus a PRECEDENCE. The single
 * function parsePrecedence() drives everything by the elegant rule: parse a
 * prefix, then keep consuming infix operators whose precedence is >= the level
 * we were asked for. That one loop replaces the dozen mutually-recursive
 * grammar functions a hand-written recursive-descent expression parser needs.
 *
 * Statements are ordinary recursive descent (declaration -> statement -> ...).
 *
 * VARIABLE RESOLUTION happens here, at compile time:
 *   - a name found among the current function's LOCALS becomes a stack-slot
 *     index (OP_GET_LOCAL 3) — no hashing at runtime;
 *   - otherwise it is a GLOBAL, looked up by name string in a hash table at
 *     runtime (OP_GET_GLOBAL "foo"). Locals are the fast path.
 * ===========================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "memory.h"
#include "scanner.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

/* 256: local slots and constant indices are single bytes, so a function may
 * have at most 256 of each. UINT8_COUNT is that limit + 1 used as an array size. */
#define UINT8_COUNT (UINT8_MAX + 1)

/* ---------------------------------------------------------------------------
 * Parser state. We keep only a one-token lookahead (`current`) plus the token
 * we just consumed (`previous`, whose lexeme the emit helpers read). Errors are
 * accumulated into `hadError`; `panicMode` suppresses the cascade of spurious
 * errors that follow the first one until we resynchronize at a statement
 * boundary. Both are file-static because there is exactly one parse in flight.
 * --------------------------------------------------------------------------- */
typedef struct {
    Token current;
    Token previous;
    bool  hadError;
    bool  panicMode;
} Parser;

/* Precedence ladder, lowest to highest. parsePrecedence(PREC_X) parses any
 * expression binding at least as tightly as X. The numeric ORDER is the whole
 * mechanism — `PREC_TERM + 1` is how a left-associative operator asks for a
 * right operand that binds tighter than itself. */
typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,   /* =                     (lowest)                        */
    PREC_OR,           /* or                                                    */
    PREC_AND,          /* and                                                   */
    PREC_EQUALITY,     /* == !=                                                 */
    PREC_COMPARISON,   /* < > <= >=                                             */
    PREC_TERM,         /* + -                                                   */
    PREC_FACTOR,       /* * /                                                   */
    PREC_UNARY,        /* ! -                                                   */
    PREC_CALL,         /* . ()                                                  */
    PREC_PRIMARY,      /* literals, grouping    (highest)                       */
} Precedence;

/* A parse function. `canAssign` tells prefix parsers whether an `=` may follow
 * (so `a = 1` assigns but `a + b = 1` reports "invalid assignment target"). */
typedef void (*ParseFn)(bool canAssign);

typedef struct {
    ParseFn    prefix;      /* how this token starts an expression, or NULL     */
    ParseFn    infix;       /* how this token continues one, or NULL            */
    Precedence precedence;  /* precedence when used as an infix operator        */
} ParseRule;

/* A compile-time local variable: its name (a borrowed source lexeme) and the
 * block SCOPE DEPTH at which it was declared. depth == -1 marks "declared but
 * not yet initialized", which catches `var a = a;` (using a in its own
 * initializer). */
typedef struct {
    Token name;
    int   depth;
} Local;

typedef enum {
    TYPE_FUNCTION,   /* a user function body                                   */
    TYPE_SCRIPT,     /* the implicit top-level function                        */
} FunctionType;

/* ---------------------------------------------------------------------------
 * Compiler — per-function compilation state, chained by `enclosing` so nested
 * function declarations form a stack. Each Compiler owns the ObjFunction it is
 * building and its own local-variable array (locals are function-scoped: a
 * nested function does NOT see the enclosing function's locals in this VM,
 * because we have no upvalues — see the README scope note).
 * --------------------------------------------------------------------------- */
typedef struct Compiler {
    struct Compiler *enclosing;
    ObjFunction     *function;
    FunctionType     type;

    Local locals[UINT8_COUNT];  /* index == runtime stack slot within the frame */
    int   localCount;           /* number of locals currently in scope          */
    int   scopeDepth;           /* 0 == global scope; each { } bumps it          */
} Compiler;

static Parser    parser;
static Compiler *current = NULL;   /* the innermost function being compiled     */

/* The chunk we are currently emitting into is always the current function's. */
static Chunk *currentChunk(void)
{
    return &current->function->chunk;
}

/* ============================ error handling ============================== */

static void errorAt(Token *token, const char *message)
{
    /* Once panicking, swallow further errors: they are almost always noise
     * caused by the first real error leaving the parser mid-expression. */
    if (parser.panicMode) return;
    parser.panicMode = true;

    fprintf(stderr, "[line %d] Error", token->line);
    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {
        /* scanner error: the "lexeme" is actually the message pointer          */
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }
    fprintf(stderr, ": %s\n", message);
    parser.hadError = true;
}

static void error(const char *message)          { errorAt(&parser.previous, message); }
static void errorAtCurrent(const char *message) { errorAt(&parser.current,  message); }

/* ============================ token cursor =============================== */

/* Advance one token, skipping over (and reporting) scanner error tokens so the
 * rest of the compiler only ever sees valid tokens. */
static void advance(void)
{
    parser.previous = parser.current;
    for (;;) {
        parser.current = scanToken();
        if (parser.current.type != TOKEN_ERROR) break;
        errorAtCurrent(parser.current.start);   /* start == the error message   */
    }
}

/* Consume a token of the expected type or report `message`. */
static void consume(TokenType type, const char *message)
{
    if (parser.current.type == type) { advance(); return; }
    errorAtCurrent(message);
}

static bool check(TokenType type) { return parser.current.type == type; }

/* Consume the token if it matches; report whether it did. */
static bool match(TokenType type)
{
    if (!check(type)) return false;
    advance();
    return true;
}

/* ============================ bytecode emit ============================== */

/* Every emitted byte records parser.previous's line, so a runtime error can be
 * blamed on the source line of the operator/operand that produced the op. */
static void emitByte(uint8_t byte)
{
    writeChunk(currentChunk(), byte, parser.previous.line);
}
static void emitBytes(uint8_t b1, uint8_t b2) { emitByte(b1); emitByte(b2); }

/* Emit a backward jump (OP_LOOP) to `loopStart`. The operand is how far BACK to
 * move ip, computed now because we already know the target. +2 accounts for the
 * two operand bytes themselves, which ip will have advanced past. */
static void emitLoop(int loopStart)
{
    emitByte(OP_LOOP);
    int offset = currentChunk()->count - loopStart + 2;
    if (offset > UINT16_MAX) error("Loop body too large.");
    emitByte((uint8_t)((offset >> 8) & 0xff));   /* big-endian: high byte first */
    emitByte((uint8_t)(offset & 0xff));
}

/* Emit a forward jump with a PLACEHOLDER operand (0xffff) and return the offset
 * of the operand so patchJump can fill in the real distance once we know it.
 * This two-step "emit then backpatch" is how single-pass compilers handle
 * forward branches whose target is not yet known. */
static int emitJump(uint8_t instruction)
{
    emitByte(instruction);
    emitByte(0xff);
    emitByte(0xff);
    return currentChunk()->count - 2;   /* index of the first placeholder byte  */
}

/* A function with no explicit `return` returns nil implicitly. */
static void emitReturn(void)
{
    emitByte(OP_NIL);
    emitByte(OP_RETURN);
}

/* Intern `value` into the current chunk's constant pool and return its index,
 * erroring if we blow past the 256-constant single-byte limit. */
static uint8_t makeConstant(Value value)
{
    int constant = addConstant(currentChunk(), value);
    if (constant > UINT8_MAX) {
        error("Too many constants in one chunk.");
        return 0;
    }
    return (uint8_t)constant;
}

static void emitConstant(Value value)
{
    emitBytes(OP_CONSTANT, makeConstant(value));
}

/* Backpatch: overwrite the placeholder at `offset` with the distance from just
 * after the operand to the current position (the jump's landing site). */
static void patchJump(int offset)
{
    int jump = currentChunk()->count - offset - 2;   /* -2: skip the operand    */
    if (jump > UINT16_MAX) error("Too much code to jump over.");
    currentChunk()->code[offset]     = (uint8_t)((jump >> 8) & 0xff);
    currentChunk()->code[offset + 1] = (uint8_t)(jump & 0xff);
}

/* ============================ compiler lifecycle ========================= */

static void initCompiler(Compiler *compiler, FunctionType type)
{
    compiler->enclosing  = current;   /* push onto the compiler chain           */
    compiler->function   = NULL;      /* set below AFTER newFunction (GC order)  */
    compiler->type       = type;
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    compiler->function   = newFunction();
    current = compiler;

    /* Record the function's name (except the top-level script). We copy it from
     * the source lexeme now, before further allocation might invalidate it. */
    if (type != TYPE_SCRIPT) {
        current->function->name =
            copyString(parser.previous.start, parser.previous.length);
    }

    /* SLOT ZERO is reserved by the calling convention for the callee itself
     * (the function being called sits just below its arguments on the stack).
     * We claim it as an unnamed local so user locals start at slot 1 and the
     * frame's slots[0] is never mistaken for a variable. */
    Local *local = &current->locals[current->localCount++];
    local->depth      = 0;
    local->name.start = "";
    local->name.length = 0;
}

/* Finish the current function: emit the implicit return, optionally dump the
 * disassembly, pop the compiler chain, and hand back the completed function. */
static ObjFunction *endCompiler(void)
{
    emitReturn();
    ObjFunction *function = current->function;

#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError) {
        disassembleChunk(currentChunk(),
            function->name != NULL ? function->name->chars : "<script>");
    }
#endif

    current = current->enclosing;   /* pop back to the enclosing function        */
    return function;
}

/* ============================ scopes & locals =========================== */

static void beginScope(void) { current->scopeDepth++; }

/* Leaving a block: pop every local declared at the depth we are exiting. Each
 * local occupies one runtime stack slot, so we emit one OP_POP per local to
 * discard them — the compile-time localCount and the runtime stack stay in
 * lockstep. */
static void endScope(void)
{
    current->scopeDepth--;
    while (current->localCount > 0 &&
           current->locals[current->localCount - 1].depth > current->scopeDepth) {
        emitByte(OP_POP);
        current->localCount--;
    }
}

/* Forward declarations for the recursive grammar. */
static void       expression(void);
static void       statement(void);
static void       declaration(void);
static ParseRule *getRule(TokenType type);
static void       parsePrecedence(Precedence precedence);

/* Add `name` to the current chunk's constants as a string, returning its index.
 * Global variable names are stored as string constants and looked up by name at
 * runtime; this is the shared helper for defining and referencing them. */
static uint8_t identifierConstant(Token *name)
{
    return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}

static bool identifiersEqual(Token *a, Token *b)
{
    if (a->length != b->length) return false;
    return memcmp(a->start, b->start, (size_t)a->length) == 0;
}

/* Resolve `name` to a local slot in `compiler`, or -1 if it is not a local
 * (hence a global). Searches INNERMOST-first so a shadowing inner local wins. */
static int resolveLocal(Compiler *compiler, Token *name)
{
    for (int i = compiler->localCount - 1; i >= 0; i--) {
        Local *local = &compiler->locals[i];
        if (identifiersEqual(name, &local->name)) {
            if (local->depth == -1)   /* still being initialized: `var a = a;`  */
                error("Can't read local variable in its own initializer.");
            return i;   /* the slot index == OP_GET_LOCAL operand               */
        }
    }
    return -1;
}

/* Record a new local at the current scope depth (initially "uninitialized",
 * depth -1). No bytecode is emitted: a local's value is simply whatever the
 * initializer left on the stack, which already lives in this slot. */
static void addLocal(Token name)
{
    if (current->localCount == UINT8_COUNT) {
        error("Too many local variables in function.");
        return;
    }
    Local *local = &current->locals[current->localCount++];
    local->name  = name;
    local->depth = -1;   /* mark uninitialized until the initializer completes  */
}

/* At a variable DECLARATION, register the name. Globals are late-bound by name,
 * so nothing to do here for them. Locals are added to the compiler's array,
 * after checking the same scope does not already declare this name. */
static void declareVariable(void)
{
    if (current->scopeDepth == 0) return;   /* globals handled elsewhere         */

    Token *name = &parser.previous;
    /* Reject a redeclaration in the SAME scope (an outer-scope shadow is fine
     * and stops the search). */
    for (int i = current->localCount - 1; i >= 0; i--) {
        Local *local = &current->locals[i];
        if (local->depth != -1 && local->depth < current->scopeDepth) break;
        if (identifiersEqual(name, &local->name))
            error("Already a variable with this name in this scope.");
    }
    addLocal(*name);
}

/* Parse a variable NAME after a keyword like `var`/`fun`/a parameter, returning
 * the constant index for a global name (0 for a local, which needs no name). */
static uint8_t parseVariable(const char *errorMessage)
{
    consume(TOKEN_IDENTIFIER, errorMessage);
    declareVariable();
    if (current->scopeDepth > 0) return 0;   /* local: no name constant needed  */
    return identifierConstant(&parser.previous);
}

/* Mark the most recent local as initialized (give it the real scope depth) now
 * that its initializer has been compiled. For functions this is called BEFORE
 * the body so the function can call itself recursively. */
static void markInitialized(void)
{
    if (current->scopeDepth == 0) return;   /* globals have no depth slot        */
    current->locals[current->localCount - 1].depth = current->scopeDepth;
}

/* Emit the definition of a variable. Globals get OP_DEFINE_GLOBAL (which binds
 * the name in the runtime table and pops the value). Locals need NO instruction
 * at all — the value the initializer produced is already sitting in the right
 * stack slot, so we just flip the local to initialized. */
static void defineVariable(uint8_t global)
{
    if (current->scopeDepth > 0) { markInitialized(); return; }
    emitBytes(OP_DEFINE_GLOBAL, global);
}

/* Parse a comma-separated argument list and return the count (which becomes the
 * OP_CALL operand). Each argument is a full expression left on the stack. */
static uint8_t argumentList(void)
{
    uint8_t argCount = 0;
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            expression();
            if (argCount == 255) error("Can't have more than 255 arguments.");
            argCount++;
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
    return argCount;
}

/* ============================ expression parsers ========================= */
/* Each of these is a ParseFn referenced by the rule table at the bottom. */

/* `and`: short-circuit. The left operand is already on the stack. If it is
 * falsey we jump PAST the right operand, leaving the (false) left value as the
 * result; otherwise we pop it and evaluate the right, whose value becomes the
 * result. No dedicated opcode — control flow alone implements the semantics. */
static void and_(bool canAssign)
{
    (void)canAssign;
    int endJump = emitJump(OP_JUMP_IF_FALSE);   /* peeks TOS; skips RHS if false */
    emitByte(OP_POP);                           /* discard the truthy LHS        */
    parsePrecedence(PREC_AND);
    patchJump(endJump);
}

/* `or`: the mirror image. If the left is falsey we jump over a small jump to
 * evaluate the right; if it is truthy we take that small jump PAST the right,
 * keeping the truthy left as the result. */
static void or_(bool canAssign)
{
    (void)canAssign;
    int elseJump = emitJump(OP_JUMP_IF_FALSE);  /* LHS falsey -> try RHS         */
    int endJump  = emitJump(OP_JUMP);           /* LHS truthy -> skip RHS        */
    patchJump(elseJump);
    emitByte(OP_POP);                           /* discard the falsey LHS        */
    parsePrecedence(PREC_OR);
    patchJump(endJump);
}

static void binary(bool canAssign)
{
    (void)canAssign;
    TokenType  operatorType = parser.previous.type;
    ParseRule *rule = getRule(operatorType);

    /* Left operand is already compiled and on the stack. Parse the RIGHT operand
     * at ONE HIGHER precedence, which makes all these operators LEFT-associative:
     * `a - b - c` parses as `(a - b) - c` because the recursive call stops
     * before the second '-'. */
    parsePrecedence((Precedence)(rule->precedence + 1));

    switch (operatorType) {
        /* The "compound" comparisons are synthesized from the three primitive
         * ops plus OP_NOT — e.g. a != b  ==  !(a == b). Fewer opcodes to
         * implement in the VM, identical semantics. */
        case TOKEN_BANG_EQUAL:    emitBytes(OP_EQUAL, OP_NOT);   break;
        case TOKEN_EQUAL_EQUAL:   emitByte(OP_EQUAL);            break;
        case TOKEN_GREATER:       emitByte(OP_GREATER);          break;
        case TOKEN_GREATER_EQUAL: emitBytes(OP_LESS, OP_NOT);    break;  /* a>=b == !(a<b) */
        case TOKEN_LESS:          emitByte(OP_LESS);             break;
        case TOKEN_LESS_EQUAL:    emitBytes(OP_GREATER, OP_NOT); break;  /* a<=b == !(a>b) */
        case TOKEN_PLUS:          emitByte(OP_ADD);              break;
        case TOKEN_MINUS:         emitByte(OP_SUBTRACT);         break;
        case TOKEN_STAR:          emitByte(OP_MULTIPLY);         break;
        case TOKEN_SLASH:         emitByte(OP_DIVIDE);           break;
        default: return;   /* unreachable                                        */
    }
}

/* A function call. The callee expression has already been compiled (it is the
 * prefix that this infix `(` follows). We compile the arguments and emit
 * OP_CALL argCount. */
static void call(bool canAssign)
{
    (void)canAssign;
    uint8_t argCount = argumentList();
    emitBytes(OP_CALL, argCount);
}

/* Literal keywords compile to a single push opcode — no constant needed. */
static void literal(bool canAssign)
{
    (void)canAssign;
    switch (parser.previous.type) {
        case TOKEN_FALSE: emitByte(OP_FALSE); break;
        case TOKEN_NIL:   emitByte(OP_NIL);   break;
        case TOKEN_TRUE:  emitByte(OP_TRUE);  break;
        default: return;   /* unreachable                                        */
    }
}

/* Grouping is purely syntactic: parentheses change precedence, not runtime
 * behavior, so we just compile the inner expression and emit nothing extra. */
static void grouping(bool canAssign)
{
    (void)canAssign;
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

/* An integer literal. strtoll parses the lexeme (which is NOT NUL-terminated,
 * but it is followed by non-digit source, so strtoll stops correctly at the
 * lexeme's end). We box it as a VAL_INT constant. */
static void number(bool canAssign)
{
    (void)canAssign;
    long long value = strtoll(parser.previous.start, NULL, 10);
    emitConstant(INT_VAL((int64_t)value));
}

/* A string literal. The lexeme includes the surrounding quotes, so we copy the
 * interior [start+1, length-2). copyString interns it and manages its memory. */
static void string(bool canAssign)
{
    (void)canAssign;
    emitConstant(OBJ_VAL(copyString(parser.previous.start + 1,
                                    parser.previous.length - 2)));
}

/* Emit a load or store for `name`, choosing local vs global at COMPILE time.
 * `canAssign` gates whether a trailing `=` turns this into a store: this is
 * what makes `a = 1` legal but `a + b = 1` a compile error (the `+` context
 * parses `a` with canAssign == false). */
static void namedVariable(Token name, bool canAssign)
{
    uint8_t getOp, setOp;
    int arg = resolveLocal(current, &name);
    if (arg != -1) {
        getOp = OP_GET_LOCAL;   setOp = OP_SET_LOCAL;   /* fast: a stack slot    */
    } else {
        arg   = identifierConstant(&name);              /* slow: a global name   */
        getOp = OP_GET_GLOBAL;  setOp = OP_SET_GLOBAL;
    }

    if (canAssign && match(TOKEN_EQUAL)) {
        expression();                       /* compile the right-hand side       */
        emitBytes(setOp, (uint8_t)arg);     /* store; assignment yields the value */
    } else {
        emitBytes(getOp, (uint8_t)arg);     /* load                              */
    }
}

static void variable(bool canAssign)
{
    namedVariable(parser.previous, canAssign);
}

static void unary(bool canAssign)
{
    (void)canAssign;
    TokenType operatorType = parser.previous.type;

    /* Compile the operand at UNARY precedence, so `-a.b` negates the whole
     * postfix expression but `-a + b` negates only `a`. */
    parsePrecedence(PREC_UNARY);

    switch (operatorType) {
        case TOKEN_BANG:  emitByte(OP_NOT);    break;
        case TOKEN_MINUS: emitByte(OP_NEGATE); break;
        default: return;   /* unreachable                                        */
    }
}

/* ---------------------------------------------------------------------------
 * THE RULE TABLE — the data that drives Pratt parsing. Indexed by TokenType,
 * each row is { prefix parser, infix parser, infix precedence }. A NULL prefix
 * means "this token can't START an expression" (e.g. a stray `+`). This table
 * IS the expression grammar; parsePrecedence is its tiny interpreter.
 * --------------------------------------------------------------------------- */
static ParseRule rules[] = {
    [TOKEN_LEFT_PAREN]    = { grouping, call,   PREC_CALL       },
    [TOKEN_RIGHT_PAREN]   = { NULL,     NULL,   PREC_NONE       },
    [TOKEN_LEFT_BRACE]    = { NULL,     NULL,   PREC_NONE       },
    [TOKEN_RIGHT_BRACE]   = { NULL,     NULL,   PREC_NONE       },
    [TOKEN_COMMA]         = { NULL,     NULL,   PREC_NONE       },
    [TOKEN_DOT]           = { NULL,     NULL,   PREC_NONE       },
    [TOKEN_MINUS]         = { unary,    binary, PREC_TERM       },
    [TOKEN_PLUS]          = { NULL,     binary, PREC_TERM       },
    [TOKEN_SEMICOLON]     = { NULL,     NULL,   PREC_NONE       },
    [TOKEN_SLASH]         = { NULL,     binary, PREC_FACTOR     },
    [TOKEN_STAR]          = { NULL,     binary, PREC_FACTOR     },
    [TOKEN_BANG]          = { unary,    NULL,   PREC_NONE       },
    [TOKEN_BANG_EQUAL]    = { NULL,     binary, PREC_EQUALITY   },
    [TOKEN_EQUAL]         = { NULL,     NULL,   PREC_NONE       },
    [TOKEN_EQUAL_EQUAL]   = { NULL,     binary, PREC_EQUALITY   },
    [TOKEN_GREATER]       = { NULL,     binary, PREC_COMPARISON },
    [TOKEN_GREATER_EQUAL] = { NULL,     binary, PREC_COMPARISON },
    [TOKEN_LESS]          = { NULL,     binary, PREC_COMPARISON },
    [TOKEN_LESS_EQUAL]    = { NULL,     binary, PREC_COMPARISON },
    [TOKEN_IDENTIFIER]    = { variable, NULL,   PREC_NONE       },
    [TOKEN_STRING]        = { string,   NULL,   PREC_NONE       },
    [TOKEN_NUMBER]        = { number,   NULL,   PREC_NONE       },
    [TOKEN_AND]           = { NULL,     and_,   PREC_AND        },
    [TOKEN_ELSE]          = { NULL,     NULL,   PREC_NONE       },
    [TOKEN_FALSE]         = { literal,  NULL,   PREC_NONE       },
    [TOKEN_FOR]           = { NULL,     NULL,   PREC_NONE       },
    [TOKEN_FUN]           = { NULL,     NULL,   PREC_NONE       },
    [TOKEN_IF]            = { NULL,     NULL,   PREC_NONE       },
    [TOKEN_NIL]           = { literal,  NULL,   PREC_NONE       },
    [TOKEN_OR]            = { NULL,     or_,    PREC_OR         },
    [TOKEN_PRINT]         = { NULL,     NULL,   PREC_NONE       },
    [TOKEN_RETURN]        = { NULL,     NULL,   PREC_NONE       },
    [TOKEN_TRUE]          = { literal,  NULL,   PREC_NONE       },
    [TOKEN_VAR]           = { NULL,     NULL,   PREC_NONE       },
    [TOKEN_WHILE]         = { NULL,     NULL,   PREC_NONE       },
    [TOKEN_ERROR]         = { NULL,     NULL,   PREC_NONE       },
    [TOKEN_EOF]           = { NULL,     NULL,   PREC_NONE       },
};

/* ---------------------------------------------------------------------------
 * parsePrecedence — the Pratt engine.
 *   1. Consume one token and run its PREFIX rule (every expression starts with
 *      a prefix: a literal, an identifier, a unary op, or a '(').
 *   2. Then, while the NEXT token is an infix operator whose precedence is at
 *      least the level we were asked to parse, consume it and run its INFIX
 *      rule (which will itself recurse to grab its right operand).
 * The `canAssign` flag is threaded so only a low-precedence context permits an
 * assignment target.
 * --------------------------------------------------------------------------- */
static void parsePrecedence(Precedence precedence)
{
    advance();
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == NULL) {
        error("Expect expression.");
        return;
    }

    bool canAssign = precedence <= PREC_ASSIGNMENT;
    prefixRule(canAssign);

    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule(canAssign);
    }

    /* If we parsed an assignable target but never consumed the '=', the '=' is
     * still pending and is a syntax error (e.g. `a * b = c`). */
    if (canAssign && match(TOKEN_EQUAL))
        error("Invalid assignment target.");
}

static ParseRule *getRule(TokenType type) { return &rules[type]; }

/* An expression is just "parse at the lowest real precedence". */
static void expression(void) { parsePrecedence(PREC_ASSIGNMENT); }

/* ============================ statements ================================ */

static void block(void)
{
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF))
        declaration();
    consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

/* Compile a whole function: a fresh Compiler, a new scope for its parameters
 * and body, then wrap the finished ObjFunction as a constant and push it. With
 * no closures, the function object itself is the runtime callable — we simply
 * OP_CONSTANT it onto the stack where defineVariable will bind it. */
static void function(FunctionType type)
{
    Compiler compiler;
    initCompiler(&compiler, type);
    beginScope();   /* NOTE: no matching endScope — endCompiler discards the
                     *   whole frame at once when the function returns.          */

    consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            current->function->arity++;
            if (current->function->arity > 255)
                errorAtCurrent("Can't have more than 255 parameters.");
            /* Each parameter is a local in slot order; it is initialized by the
             * caller placing the argument there, so mark it defined at once. */
            uint8_t constant = parseVariable("Expect parameter name.");
            defineVariable(constant);
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
    consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
    block();

    ObjFunction *function = endCompiler();
    emitBytes(OP_CONSTANT, makeConstant(OBJ_VAL(function)));
}

static void funDeclaration(void)
{
    uint8_t global = parseVariable("Expect function name.");
    /* Mark the name initialized BEFORE compiling the body so the function may
     * refer to itself (recursion). For a global this is a no-op; recursion
     * works there because globals are resolved by name at call time. */
    markInitialized();
    function(TYPE_FUNCTION);
    defineVariable(global);
}

static void varDeclaration(void)
{
    uint8_t global = parseVariable("Expect variable name.");
    if (match(TOKEN_EQUAL)) expression();   /* `var a = expr;`                   */
    else                    emitByte(OP_NIL); /* `var a;` defaults to nil        */
    consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
    defineVariable(global);
}

/* An "expression statement" is an expression whose value is discarded (a call
 * for its side effect, an assignment). We emit OP_POP so it is net stack-
 * neutral — every statement must leave the stack as it found it. */
static void expressionStatement(void)
{
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
    emitByte(OP_POP);
}

static void printStatement(void)
{
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    emitByte(OP_PRINT);
}

static void returnStatement(void)
{
    if (current->type == TYPE_SCRIPT)
        error("Can't return from top-level code.");

    if (match(TOKEN_SEMICOLON)) {
        emitReturn();   /* `return;` -> return nil                              */
    } else {
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after return value.");
        emitByte(OP_RETURN);
    }
}

/* if (cond) thenStmt [else elseStmt]. Compiled with two forward jumps. The
 * OP_POP after each branch discards the condition value, which OP_JUMP_IF_FALSE
 * only peeked. */
static void ifStatement(void)
{
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int thenJump = emitJump(OP_JUMP_IF_FALSE);   /* skip THEN when false         */
    emitByte(OP_POP);                            /* pop condition (then-branch)  */
    statement();

    int elseJump = emitJump(OP_JUMP);            /* THEN done -> skip ELSE        */
    patchJump(thenJump);
    emitByte(OP_POP);                            /* pop condition (else-branch)  */

    if (match(TOKEN_ELSE)) statement();
    patchJump(elseJump);
}

/* while (cond) body. `loopStart` is the byte offset we jump BACK to each
 * iteration; the exit jump is patched to just past the OP_LOOP. */
static void whileStatement(void)
{
    int loopStart = currentChunk()->count;
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);                 /* pop condition before running the body   */
    statement();
    emitLoop(loopStart);              /* jump back to re-test the condition      */

    patchJump(exitJump);
    emitByte(OP_POP);                 /* pop condition after the loop exits       */
}

/* for (init; cond; incr) body — desugared to a while loop with jumps. The
 * trickiest part is the INCREMENT: in source it appears before the body but must
 * RUN AFTER it, so we compile it, jump over it to the body, and have the body
 * loop back to the increment. A whole scope wraps the loop so a `var` in the
 * initializer is local to the loop. */
static void forStatement(void)
{
    beginScope();
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");

    /* --- initializer --- */
    if (match(TOKEN_SEMICOLON)) {
        /* no initializer */
    } else if (match(TOKEN_VAR)) {
        varDeclaration();
    } else {
        expressionStatement();
    }

    int loopStart = currentChunk()->count;

    /* --- condition (optional) --- */
    int exitJump = -1;
    if (!match(TOKEN_SEMICOLON)) {
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");
        exitJump = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP);   /* pop condition                                     */
    }

    /* --- increment (optional): jump over it to the body, run body, then jump
     * BACK here so the increment runs at the end of each iteration. --- */
    if (!match(TOKEN_RIGHT_PAREN)) {
        int bodyJump       = emitJump(OP_JUMP);
        int incrementStart = currentChunk()->count;
        expression();
        emitByte(OP_POP);   /* discard the increment expression's value          */
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

        emitLoop(loopStart);            /* after increment, go test the condition */
        loopStart = incrementStart;     /* the body now loops back to increment   */
        patchJump(bodyJump);
    }

    statement();
    emitLoop(loopStart);

    if (exitJump != -1) {
        patchJump(exitJump);
        emitByte(OP_POP);   /* pop condition when the loop exits                  */
    }

    endScope();
}

/* Panic-mode recovery: after an error, skip tokens until a likely statement
 * boundary (a ';' just passed, or a keyword that starts a statement), so we can
 * report further INDEPENDENT errors without an avalanche of noise from being
 * mid-expression. */
static void synchronize(void)
{
    parser.panicMode = false;
    while (parser.current.type != TOKEN_EOF) {
        if (parser.previous.type == TOKEN_SEMICOLON) return;
        switch (parser.current.type) {
            case TOKEN_FUN: case TOKEN_VAR: case TOKEN_FOR:
            case TOKEN_IF:  case TOKEN_WHILE: case TOKEN_PRINT:
            case TOKEN_RETURN:
                return;
            default: ;   /* keep skipping                                        */
        }
        advance();
    }
}

/* A declaration is a statement that may introduce a name. It is the top of the
 * grammar and the resync point. */
static void declaration(void)
{
    if      (match(TOKEN_FUN)) funDeclaration();
    else if (match(TOKEN_VAR)) varDeclaration();
    else                       statement();

    if (parser.panicMode) synchronize();
}

static void statement(void)
{
    if (match(TOKEN_PRINT)) {
        printStatement();
    } else if (match(TOKEN_IF)) {
        ifStatement();
    } else if (match(TOKEN_RETURN)) {
        returnStatement();
    } else if (match(TOKEN_WHILE)) {
        whileStatement();
    } else if (match(TOKEN_FOR)) {
        forStatement();
    } else if (match(TOKEN_LEFT_BRACE)) {
        beginScope();
        block();
        endScope();
    } else {
        expressionStatement();
    }
}

/* ============================ public entry =============================== */

ObjFunction *compile(const char *source)
{
    initScanner(source);

    Compiler compiler;
    /* The outermost compilation is the implicit top-level function <script>. */
    initCompiler(&compiler, TYPE_SCRIPT);

    parser.hadError  = false;
    parser.panicMode = false;

    advance();   /* prime `current` with the first token                        */

    /* A program is a sequence of declarations until EOF. */
    while (!match(TOKEN_EOF))
        declaration();

    ObjFunction *function = endCompiler();
    /* Return NULL on any error so the VM never tries to run half-built code. */
    return parser.hadError ? NULL : function;
}

/* ---------------------------------------------------------------------------
 * markCompilerRoots — GC integration. A collection can fire in the MIDDLE of
 * compilation (compiling allocates strings and function objects). Those objects
 * are reachable only through the compiler chain, not the VM stack, so the GC
 * would wrongly free them. We mark every function currently under construction.
 * --------------------------------------------------------------------------- */
void markCompilerRoots(void)
{
    Compiler *compiler = current;
    while (compiler != NULL) {
        markObject((Obj *)compiler->function);
        compiler = compiler->enclosing;
    }
}
