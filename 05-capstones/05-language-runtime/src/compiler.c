/* ===========================================================================
 * compiler.c — a single-pass Pratt parser that emits bytecode directly.
 * ===========================================================================
 *
 * There is NO abstract syntax tree. As the parser recognizes each construct it
 * immediately writes bytecode into the function it is building. Two ideas carry
 * the whole file:
 *
 *  1. PRATT PARSING (precedence climbing). Instead of a dozen mutually-recursive
 *     grammar functions (one per precedence level), we have ONE loop,
 *     parsePrecedence(), plus a table `rules[]` giving each token a {prefix,
 *     infix, precedence} triple. To parse an expression at some minimum
 *     precedence: run the prefix rule of the first token, then keep consuming
 *     infix operators whose precedence is >= the minimum. Left-vs-right
 *     associativity falls out of whether an infix rule recurses at `prec` or
 *     `prec + 1`.
 *
 *  2. SINGLE-PASS LOCALS. Local variables are resolved to STACK SLOTS at compile
 *     time (resolveLocal), so at run time a local read/write is just an index —
 *     no hash lookup. Forward jumps (if/while/for/and/or) are emitted with a
 *     placeholder operand and BACKPATCHED once the target address is known.
 *
 * GC note: object allocation here (copyString, newFunction) happens with the
 * collector DISABLED (heap.gcEnabled is set true only when the VM starts
 * running), so we never have to root half-built compiler state. See README Scope.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compiler.h"
#include "object.h"
#include "scanner.h"

/* ---- Parser & compiler state (single global instances) --------------------*/

typedef struct {
    Token current;      /* the lookahead token                                 */
    Token previous;     /* the token we just consumed                          */
    bool  hadError;     /* sticky: any error makes compile() return NULL       */
    bool  panicMode;    /* suppress cascaded errors until we synchronize()     */
} Parser;

/* Pratt precedence, lowest to highest. parsePrecedence(p) parses everything with
 * precedence >= p. */
typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,   /* =        */
    PREC_OR,           /* or       */
    PREC_AND,          /* and      */
    PREC_EQUALITY,     /* == !=    */
    PREC_COMPARISON,   /* < > <= >=*/
    PREC_TERM,         /* + -      */
    PREC_FACTOR,       /* * /      */
    PREC_UNARY,        /* ! -      */
    PREC_CALL,         /* . ()     */
    PREC_PRIMARY
} Precedence;

/* A parse function. `canAssign` says whether an `=` may legally follow (it may
 * not on the left of a higher-precedence operator), which is how we reject
 * `a + b = c`. */
typedef void (*ParseFn)(bool canAssign);

typedef struct {
    ParseFn    prefix;
    ParseFn    infix;
    Precedence precedence;
} ParseRule;

/* A compile-time local variable: its name (a slice of source) and the scope depth
 * at which it was declared. depth == -1 means "declared but initializer not yet
 * compiled", which lets us reject `var a = a;`. */
typedef struct {
    Token name;
    int   depth;
} Local;

typedef enum { TYPE_FUNCTION, TYPE_SCRIPT } FunctionType;

/* One Compiler per function being compiled; they chain via `enclosing` so a
 * nested `fun` can finish and hand control back to its parent. */
typedef struct Compiler {
    struct Compiler *enclosing;
    ObjFunction     *function;              /* the function we are building        */
    FunctionType     type;
    Local            locals[UINT8_COUNT];   /* locals in scope, indexed by slot    */
    int              localCount;
    int              scopeDepth;            /* 0 == global scope                   */
} Compiler;

static Parser    parser;
static Compiler *current = NULL;

/* The bytecode we are currently writing into. */
static Chunk *currentChunk(void) { return &current->function->chunk; }

/* ---- Error reporting ------------------------------------------------------*/

static void errorAt(Token *token, const char *message)
{
    if (parser.panicMode) return;           /* one error per synchronize window   */
    parser.panicMode = true;
    fprintf(stderr, "[line %d] Error", token->line);
    if (token->type == TOKEN_EOF)        fprintf(stderr, " at end");
    else if (token->type == TOKEN_ERROR) { /* lexeme is the message; no location */ }
    else fprintf(stderr, " at '%.*s'", token->length, token->start);
    fprintf(stderr, ": %s\n", message);
    parser.hadError = true;
}
static void error(const char *message)          { errorAt(&parser.previous, message); }
static void errorAtCurrent(const char *message)  { errorAt(&parser.current,  message); }

/* ---- Token cursor ---------------------------------------------------------*/

static void advance(void)
{
    parser.previous = parser.current;
    for (;;) {                              /* skip over (and report) error tokens */
        parser.current = scanToken();
        if (parser.current.type != TOKEN_ERROR) break;
        errorAtCurrent(parser.current.start);
    }
}
static void consume(TokenType type, const char *message)
{
    if (parser.current.type == type) { advance(); return; }
    errorAtCurrent(message);
}
static bool check(TokenType type) { return parser.current.type == type; }
static bool match(TokenType type) { if (!check(type)) return false; advance(); return true; }

/* ---- Bytecode emission ----------------------------------------------------*/

static void emitByte(uint8_t byte) { writeChunk(currentChunk(), byte, parser.previous.line); }
static void emitBytes(uint8_t b1, uint8_t b2) { emitByte(b1); emitByte(b2); }

/* A backward jump (loops). The operand is the distance to subtract from ip. */
static void emitLoop(int loopStart)
{
    emitByte(OP_LOOP);
    int offset = currentChunk()->count - loopStart + 2;   /* +2 for the operand  */
    if (offset > UINT16_MAX) error("Loop body too large.");
    emitByte((uint8_t)((offset >> 8) & 0xff));            /* big-endian 16-bit    */
    emitByte((uint8_t)(offset & 0xff));
}

/* Emit a jump with a PLACEHOLDER operand; return the operand's offset so
 * patchJump() can fill in the real distance once we know it. */
static int emitJump(uint8_t instruction)
{
    emitByte(instruction);
    emitByte(0xff);
    emitByte(0xff);
    return currentChunk()->count - 2;
}
static void emitReturn(void) { emitByte(OP_NIL); emitByte(OP_RETURN); } /* implicit `return nil` */

/* Add `value` to the constant pool; the 1-byte operand can index only 0..255. */
static uint8_t makeConstant(Value value)
{
    int constant = addConstant(currentChunk(), value);
    if (constant > UINT8_MAX) { error("Too many constants in one chunk."); return 0; }
    return (uint8_t)constant;
}
static void emitConstant(Value value) { emitBytes(OP_CONSTANT, makeConstant(value)); }

/* Backpatch: compute the forward distance from just-after-the-operand to the
 * current end of code, and write it into the placeholder emitted earlier. */
static void patchJump(int offset)
{
    int jump = currentChunk()->count - offset - 2;
    if (jump > UINT16_MAX) error("Too much code to jump over.");
    currentChunk()->code[offset]     = (uint8_t)((jump >> 8) & 0xff);
    currentChunk()->code[offset + 1] = (uint8_t)(jump & 0xff);
}

/* ---- Compiler lifecycle ---------------------------------------------------*/

static void initCompiler(Compiler *compiler, FunctionType type)
{
    compiler->enclosing  = current;
    compiler->function   = NULL;            /* set below (allocation could GC…)    */
    compiler->type       = type;
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    compiler->function   = newFunction();
    current = compiler;

    /* Named functions capture their name for stack traces. (For TYPE_SCRIPT the
     * name stays NULL and prints as "<script>".) parser.previous is the name
     * token the caller already consumed. */
    if (type != TYPE_SCRIPT)
        current->function->name = copyString(parser.previous.start,
                                             parser.previous.length);

    /* Slot 0 is reserved for the function object itself (the callee lives at the
     * base of its own call window). Giving it an empty name means user code can
     * never refer to it. */
    Local *local = &current->locals[current->localCount++];
    local->depth      = 0;
    local->name.start = "";
    local->name.length = 0;
}

/* Forward decl: disassembler lives in vm.c, used only under DEBUG_PRINT_CODE. */
void disassembleChunk(Chunk *chunk, const char *name);

static ObjFunction *endCompiler(void)
{
    emitReturn();
    ObjFunction *function = current->function;
#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError)
        disassembleChunk(currentChunk(),
                         function->name != NULL ? function->name->chars : "<script>");
#endif
    current = current->enclosing;           /* pop back to the parent compiler     */
    return function;
}

static void beginScope(void) { current->scopeDepth++; }

/* Leaving a block pops every local it declared. Each pop is one OP_POP so the
 * runtime stack matches the compiler's slot accounting exactly. */
static void endScope(void)
{
    current->scopeDepth--;
    while (current->localCount > 0 &&
           current->locals[current->localCount - 1].depth > current->scopeDepth) {
        emitByte(OP_POP);
        current->localCount--;
    }
}

/* ---- Forward declarations for the rule table ------------------------------*/

static void expression(void);
static void statement(void);
static void declaration(void);
static ParseRule *getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

/* ---- Names: locals, globals, and variable references ----------------------*/

/* A variable name becomes a string CONSTANT in the pool; globals are looked up
 * by that name at run time (OP_GET/SET/DEFINE_GLOBAL). */
static uint8_t identifierConstant(Token *name)
{
    return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}
static bool identifiersEqual(Token *a, Token *b)
{
    return a->length == b->length && memcmp(a->start, b->start, (size_t)a->length) == 0;
}

/* Resolve a name to a local slot, or -1 if it's not a local (=> a global).
 * Searches inner-to-outer so a shadowing local wins. */
static int resolveLocal(Compiler *compiler, Token *name)
{
    for (int i = compiler->localCount - 1; i >= 0; i--) {
        Local *local = &compiler->locals[i];
        if (identifiersEqual(name, &local->name)) {
            if (local->depth == -1)
                error("Can't read local variable in its own initializer.");
            return i;
        }
    }
    return -1;
}

static void addLocal(Token name)
{
    if (current->localCount == UINT8_COUNT) {  /* one byte of slot index only     */
        error("Too many local variables in function.");
        return;
    }
    Local *local = &current->locals[current->localCount++];
    local->name  = name;
    local->depth = -1;                         /* "uninitialized" until defined    */
}

/* Record a local declaration (globals are late-bound, so they're skipped here).
 * Also rejects redeclaring a name in the SAME scope. */
static void declareVariable(void)
{
    if (current->scopeDepth == 0) return;      /* globals handled elsewhere        */
    Token *name = &parser.previous;
    for (int i = current->localCount - 1; i >= 0; i--) {
        Local *local = &current->locals[i];
        if (local->depth != -1 && local->depth < current->scopeDepth) break;  /* outer scope */
        if (identifiersEqual(name, &local->name))
            error("Already a variable with this name in this scope.");
    }
    addLocal(*name);
}

/* Parse a variable name (in a declaration). Returns a constant index for a
 * global name, or 0 for a local (whose slot the compiler already knows). */
static uint8_t parseVariable(const char *errorMessage)
{
    consume(TOKEN_IDENTIFIER, errorMessage);
    declareVariable();
    if (current->scopeDepth > 0) return 0;     /* local: no name constant needed   */
    return identifierConstant(&parser.previous);
}

/* A local becomes usable once its initializer is compiled. */
static void markInitialized(void)
{
    if (current->scopeDepth == 0) return;
    current->locals[current->localCount - 1].depth = current->scopeDepth;
}

static void defineVariable(uint8_t global)
{
    if (current->scopeDepth > 0) { markInitialized(); return; }  /* locals: nothing to emit */
    emitBytes(OP_DEFINE_GLOBAL, global);
}

/* Parse a comma-separated argument list after '('. Returns the arg count. */
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

/* ---- Expression parse functions (referenced by the rule table) ------------*/

/* Logical `and`: short-circuit. If the left is falsey we jump past the right,
 * leaving the (falsey) left as the result; otherwise pop it and evaluate right. */
static void and_(bool canAssign)
{
    (void)canAssign;
    int endJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    parsePrecedence(PREC_AND);
    patchJump(endJump);
}

/* Logical `or`: mirror image. If left is falsey, fall through to evaluate right;
 * if truthy, jump over the right and keep the (truthy) left. */
static void or_(bool canAssign)
{
    (void)canAssign;
    int elseJump = emitJump(OP_JUMP_IF_FALSE);
    int endJump  = emitJump(OP_JUMP);
    patchJump(elseJump);
    emitByte(OP_POP);
    parsePrecedence(PREC_OR);
    patchJump(endJump);
}

static void binary(bool canAssign)
{
    (void)canAssign;
    TokenType  operatorType = parser.previous.type;
    ParseRule *rule = getRule(operatorType);
    /* Right operand at precedence+1 => LEFT associativity (a-b-c == (a-b)-c). */
    parsePrecedence((Precedence)(rule->precedence + 1));
    switch (operatorType) {
    case TOKEN_BANG_EQUAL:    emitBytes(OP_EQUAL, OP_NOT); break;  /* !(a==b)      */
    case TOKEN_EQUAL_EQUAL:   emitByte(OP_EQUAL);          break;
    case TOKEN_GREATER:       emitByte(OP_GREATER);        break;
    case TOKEN_GREATER_EQUAL: emitBytes(OP_LESS, OP_NOT);  break;  /* !(a<b)       */
    case TOKEN_LESS:          emitByte(OP_LESS);           break;
    case TOKEN_LESS_EQUAL:    emitBytes(OP_GREATER, OP_NOT); break;/* !(a>b)       */
    case TOKEN_PLUS:          emitByte(OP_ADD);            break;
    case TOKEN_MINUS:         emitByte(OP_SUBTRACT);       break;
    case TOKEN_STAR:          emitByte(OP_MULTIPLY);       break;
    case TOKEN_SLASH:         emitByte(OP_DIVIDE);         break;
    default: return;   /* unreachable */
    }
}

static void call(bool canAssign)
{
    (void)canAssign;
    uint8_t argCount = argumentList();
    emitBytes(OP_CALL, argCount);
}

static void literal(bool canAssign)
{
    (void)canAssign;
    switch (parser.previous.type) {
    case TOKEN_FALSE: emitByte(OP_FALSE); break;
    case TOKEN_NIL:   emitByte(OP_NIL);   break;
    case TOKEN_TRUE:  emitByte(OP_TRUE);  break;
    default: return;   /* unreachable */
    }
}

static void grouping(bool canAssign)
{
    (void)canAssign;
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void number(bool canAssign)
{
    (void)canAssign;
    double value = strtod(parser.previous.start, NULL);   /* all numbers are double */
    emitConstant(NUMBER_VAL(value));
}

/* A string literal: strip the surrounding quotes and intern the bytes as an
 * ObjString constant. */
static void string(bool canAssign)
{
    (void)canAssign;
    emitConstant(OBJ_VAL(copyString(parser.previous.start + 1,
                                    parser.previous.length - 2)));
}

/* Emit a load or store for a name. If an `=` follows and assignment is allowed at
 * this precedence, compile the right-hand side and emit a store; else emit a load.
 * Locals resolve to a slot; everything else is a global. */
static void namedVariable(Token name, bool canAssign)
{
    uint8_t getOp, setOp;
    int arg = resolveLocal(current, &name);
    if (arg != -1) {
        getOp = OP_GET_LOCAL;  setOp = OP_SET_LOCAL;
    } else {
        arg   = identifierConstant(&name);
        getOp = OP_GET_GLOBAL; setOp = OP_SET_GLOBAL;
    }
    if (canAssign && match(TOKEN_EQUAL)) {
        expression();
        emitBytes(setOp, (uint8_t)arg);
    } else {
        emitBytes(getOp, (uint8_t)arg);
    }
}
static void variable(bool canAssign) { namedVariable(parser.previous, canAssign); }

static void unary(bool canAssign)
{
    (void)canAssign;
    TokenType operatorType = parser.previous.type;
    parsePrecedence(PREC_UNARY);            /* operand binds tighter than unary    */
    switch (operatorType) {
    case TOKEN_BANG:  emitByte(OP_NOT);    break;
    case TOKEN_MINUS: emitByte(OP_NEGATE); break;
    default: return;   /* unreachable */
    }
}

/* ---- The rule table -------------------------------------------------------
 * Designated initializers key each entry by TokenType, so this stays correct even
 * if the enum is reordered. {prefix, infix, infix-precedence}. */
static ParseRule rules[] = {
    [TOKEN_LEFT_PAREN]    = {grouping, call,   PREC_CALL},
    [TOKEN_RIGHT_PAREN]   = {NULL,     NULL,   PREC_NONE},
    [TOKEN_LEFT_BRACE]    = {NULL,     NULL,   PREC_NONE},
    [TOKEN_RIGHT_BRACE]   = {NULL,     NULL,   PREC_NONE},
    [TOKEN_COMMA]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_MINUS]         = {unary,    binary, PREC_TERM},
    [TOKEN_PLUS]          = {NULL,     binary, PREC_TERM},
    [TOKEN_SEMICOLON]     = {NULL,     NULL,   PREC_NONE},
    [TOKEN_SLASH]         = {NULL,     binary, PREC_FACTOR},
    [TOKEN_STAR]          = {NULL,     binary, PREC_FACTOR},
    [TOKEN_BANG]          = {unary,    NULL,   PREC_NONE},
    [TOKEN_BANG_EQUAL]    = {NULL,     binary, PREC_EQUALITY},
    [TOKEN_EQUAL]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_EQUAL_EQUAL]   = {NULL,     binary, PREC_EQUALITY},
    [TOKEN_GREATER]       = {NULL,     binary, PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL,     binary, PREC_COMPARISON},
    [TOKEN_LESS]          = {NULL,     binary, PREC_COMPARISON},
    [TOKEN_LESS_EQUAL]    = {NULL,     binary, PREC_COMPARISON},
    [TOKEN_IDENTIFIER]    = {variable, NULL,   PREC_NONE},
    [TOKEN_STRING]        = {string,   NULL,   PREC_NONE},
    [TOKEN_NUMBER]        = {number,   NULL,   PREC_NONE},
    [TOKEN_AND]           = {NULL,     and_,   PREC_AND},
    [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
    [TOKEN_FALSE]         = {literal,  NULL,   PREC_NONE},
    [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
    [TOKEN_NIL]           = {literal,  NULL,   PREC_NONE},
    [TOKEN_OR]            = {NULL,     or_,    PREC_OR},
    [TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE},
    [TOKEN_TRUE]          = {literal,  NULL,   PREC_NONE},
    [TOKEN_VAR]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_WHILE]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_ERROR]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_EOF]           = {NULL,     NULL,   PREC_NONE},
};

/* The Pratt core. Parse a prefix expression, then fold in every following infix
 * operator whose precedence is >= the requested minimum. */
static void parsePrecedence(Precedence precedence)
{
    advance();
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == NULL) { error("Expect expression."); return; }

    bool canAssign = precedence <= PREC_ASSIGNMENT;   /* assignment only at the top */
    prefixRule(canAssign);

    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule(canAssign);
    }
    /* If we could have assigned but didn't consume the `=`, it's a bad target. */
    if (canAssign && match(TOKEN_EQUAL)) error("Invalid assignment target.");
}

static ParseRule *getRule(TokenType type) { return &rules[type]; }
static void expression(void) { parsePrecedence(PREC_ASSIGNMENT); }

/* ---- Statements & declarations --------------------------------------------*/

static void block(void)
{
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) declaration();
    consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

/* Compile a function body into its OWN nested compiler, then emit a constant that
 * pushes the finished ObjFunction. (No OP_CLOSURE — functions here don't capture
 * their environment; see README Scope.) */
static void function(FunctionType type)
{
    Compiler compiler;
    initCompiler(&compiler, type);
    beginScope();       /* parameters and body live in the function's scope       */

    consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            current->function->arity++;
            if (current->function->arity > 255)
                errorAtCurrent("Can't have more than 255 parameters.");
            uint8_t constant = parseVariable("Expect parameter name.");
            defineVariable(constant);       /* a parameter is a pre-set local      */
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
    consume(TOKEN_LEFT_BRACE,  "Expect '{' before function body.");
    block();
    /* No endScope(): endCompiler() tears down the whole function frame at once. */

    ObjFunction *fn = endCompiler();
    emitBytes(OP_CONSTANT, makeConstant(OBJ_VAL(fn)));
}

static void funDeclaration(void)
{
    uint8_t global = parseVariable("Expect function name.");
    markInitialized();          /* allow the body to reference itself (recursion)  */
    function(TYPE_FUNCTION);
    defineVariable(global);
}

static void varDeclaration(void)
{
    uint8_t global = parseVariable("Expect variable name.");
    if (match(TOKEN_EQUAL)) expression();
    else                    emitByte(OP_NIL);   /* `var x;` initializes to nil      */
    consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
    defineVariable(global);
}

static void expressionStatement(void)
{
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
    emitByte(OP_POP);           /* a statement leaves nothing on the stack         */
}

static void printStatement(void)
{
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    emitByte(OP_PRINT);
}

static void returnStatement(void)
{
    if (current->type == TYPE_SCRIPT) error("Can't return from top-level code.");
    if (match(TOKEN_SEMICOLON)) {
        emitReturn();           /* `return;` => return nil                         */
    } else {
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after return value.");
        emitByte(OP_RETURN);
    }
}

static void ifStatement(void)
{
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int thenJump = emitJump(OP_JUMP_IF_FALSE);  /* skip THEN if condition falsey    */
    emitByte(OP_POP);                           /* pop the condition (then branch)  */
    statement();
    int elseJump = emitJump(OP_JUMP);           /* skip ELSE after running THEN      */

    patchJump(thenJump);
    emitByte(OP_POP);                           /* pop the condition (else branch)  */
    if (match(TOKEN_ELSE)) statement();
    patchJump(elseJump);
}

static void whileStatement(void)
{
    int loopStart = currentChunk()->count;      /* jump target for the back-edge    */
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    statement();
    emitLoop(loopStart);                        /* back to re-test the condition    */
    patchJump(exitJump);
    emitByte(OP_POP);
}

/* Desugar `for (init; cond; incr) body` into an equivalent while loop, arranging
 * the increment to run AFTER the body but BEFORE the next condition test — done
 * with two jumps so we never need to buffer the increment's bytecode. */
static void forStatement(void)
{
    beginScope();                               /* a `var` initializer is scoped    */
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");

    /* initializer clause */
    if (match(TOKEN_SEMICOLON))      { /* none */ }
    else if (match(TOKEN_VAR))       { varDeclaration(); }
    else                             { expressionStatement(); }

    int loopStart = currentChunk()->count;
    int exitJump  = -1;

    /* condition clause */
    if (!match(TOKEN_SEMICOLON)) {
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");
        exitJump = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP);
    }

    /* increment clause (compiled now, but jumped OVER until after the body) */
    if (!match(TOKEN_RIGHT_PAREN)) {
        int bodyJump       = emitJump(OP_JUMP);
        int incrementStart = currentChunk()->count;
        expression();
        emitByte(OP_POP);
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");
        emitLoop(loopStart);        /* after increment, re-test condition           */
        loopStart = incrementStart; /* body's back-edge now targets the increment   */
        patchJump(bodyJump);
    }

    statement();                    /* the loop body                                */
    emitLoop(loopStart);
    if (exitJump != -1) { patchJump(exitJump); emitByte(OP_POP); }
    endScope();
}

/* Error recovery: after an error, skip tokens until a likely statement boundary
 * so we can report more than one error per compile without cascading. */
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
        default: ;   /* keep skipping */
        }
        advance();
    }
}

static void statement(void)
{
    if      (match(TOKEN_PRINT))      printStatement();
    else if (match(TOKEN_FOR))        forStatement();
    else if (match(TOKEN_IF))         ifStatement();
    else if (match(TOKEN_RETURN))     returnStatement();
    else if (match(TOKEN_WHILE))      whileStatement();
    else if (match(TOKEN_LEFT_BRACE)) { beginScope(); block(); endScope(); }
    else                              expressionStatement();
}

static void declaration(void)
{
    if      (match(TOKEN_FUN)) funDeclaration();
    else if (match(TOKEN_VAR)) varDeclaration();
    else                       statement();
    if (parser.panicMode) synchronize();
}

/* Entry point: compile a whole program to the top-level <script> function. */
ObjFunction *compile(const char *source)
{
    initScanner(source);
    parser.hadError  = false;
    parser.panicMode = false;

    Compiler compiler;
    initCompiler(&compiler, TYPE_SCRIPT);

    advance();                                  /* prime the lookahead              */
    while (!match(TOKEN_EOF)) declaration();

    ObjFunction *function = endCompiler();
    return parser.hadError ? NULL : function;
}
