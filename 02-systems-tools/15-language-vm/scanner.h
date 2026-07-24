/* ===========================================================================
 * scanner.h — the lexer: source text -> a stream of Tokens.
 * ===========================================================================
 *
 * The scanner is PULL-BASED and allocation-free: the compiler asks for one
 * token at a time via scanToken(), and each Token merely POINTS INTO the
 * original source string (a `start` pointer + a `length`) instead of copying
 * the lexeme. The source buffer outlives compilation, so these pointers stay
 * valid; only when a lexeme must become a runtime value (a string/identifier)
 * does the compiler copy it into a GC-managed ObjString. Zero heap traffic in
 * the hot lexing loop is the payoff.
 * ===========================================================================
 */
#ifndef CLOXI_SCANNER_H
#define CLOXI_SCANNER_H

/* Token categories. One entry per keyword and per punctuation shape, plus
 * TOKEN_ERROR (the scanner reports lexical errors as a token carrying the
 * message) and TOKEN_EOF (a sentinel so the parser never reads past the end). */
typedef enum {
    /* single-character punctuation */
    TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
    TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS,
    TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR,
    /* one- or two-character punctuation (maximal-munch handled in scanner.c) */
    TOKEN_BANG, TOKEN_BANG_EQUAL,
    TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
    TOKEN_GREATER, TOKEN_GREATER_EQUAL,
    TOKEN_LESS, TOKEN_LESS_EQUAL,
    /* literals */
    TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,
    /* keywords */
    TOKEN_AND, TOKEN_ELSE, TOKEN_FALSE, TOKEN_FOR, TOKEN_FUN, TOKEN_IF,
    TOKEN_NIL, TOKEN_OR, TOKEN_PRINT, TOKEN_RETURN, TOKEN_TRUE,
    TOKEN_VAR, TOKEN_WHILE,
    /* housekeeping */
    TOKEN_ERROR, TOKEN_EOF,
} TokenType;

typedef struct {
    TokenType   type;
    const char *start;   /* points INTO the source; NOT NUL-terminated here    */
    int         length;  /* lexeme byte length                                 */
    int         line;    /* 1-based source line, for error messages            */
} Token;

/* Point the scanner at a fresh source string (NUL-terminated). */
void initScanner(const char *source);

/* Produce the next token. At end of input it returns TOKEN_EOF forever. */
Token scanToken(void);

#endif /* CLOXI_SCANNER_H */
