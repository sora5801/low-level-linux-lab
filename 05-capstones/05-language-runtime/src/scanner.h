/* ===========================================================================
 * scanner.h — the lexer: source text -> a stream of tokens, on demand.
 * ===========================================================================
 *
 * The scanner is PULL-based: the compiler calls scanToken() when it wants the
 * next token, so there is no token array and no separate lexing pass. A token
 * does not own its characters — `start` points straight into the source buffer
 * and `length` says how many bytes — so lexing allocates nothing.
 */
#ifndef LUMEN_SCANNER_H
#define LUMEN_SCANNER_H

typedef enum {
    /* single-character punctuation */
    TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
    TOKEN_COMMA, TOKEN_MINUS, TOKEN_PLUS,
    TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR,
    /* one- or two-character operators (maximal munch: `!` vs `!=`) */
    TOKEN_BANG, TOKEN_BANG_EQUAL,
    TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
    TOKEN_GREATER, TOKEN_GREATER_EQUAL,
    TOKEN_LESS, TOKEN_LESS_EQUAL,
    /* literals */
    TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,
    /* keywords */
    TOKEN_AND, TOKEN_ELSE, TOKEN_FALSE, TOKEN_FOR, TOKEN_FUN,
    TOKEN_IF, TOKEN_NIL, TOKEN_OR, TOKEN_PRINT, TOKEN_RETURN,
    TOKEN_TRUE, TOKEN_VAR, TOKEN_WHILE,
    /* housekeeping */
    TOKEN_ERROR,   /* lexical error; `start`/`length` point at a message literal */
    TOKEN_EOF      /* end of source                                             */
} TokenType;

typedef struct {
    TokenType   type;
    const char *start;   /* into the source buffer (not owned)                 */
    int         length;  /* byte length of the lexeme                          */
    int         line;    /* 1-based source line, for error messages            */
} Token;

void  initScanner(const char *source);
Token scanToken(void);

#endif /* LUMEN_SCANNER_H */
