/* ===========================================================================
 * scanner.c — a hand-written lexer: no regex engine, one forward cursor.
 * ===========================================================================
 *
 * Two classic techniques:
 *   - MAXIMAL MUNCH: when `!` could start `!` or `!=`, we peek one more char and
 *     take the longest valid token. Same for `=`,`<`,`>`.
 *   - A KEYWORD TRIE (identifierType): after scanning an identifier we walk its
 *     first letters through a tiny switch to see if it spells a keyword, instead
 *     of hashing against a keyword table. Branch-cheap and allocation-free.
 */
#include <stdbool.h>
#include <string.h>

#include "scanner.h"

/* Single global scanner: start-of-current-lexeme, current cursor, line. */
typedef struct {
    const char *start;    /* first char of the token being scanned              */
    const char *current;  /* the cursor                                         */
    int         line;
} Scanner;

static Scanner scanner;

void initScanner(const char *source)
{
    scanner.start   = source;
    scanner.current = source;
    scanner.line    = 1;
}

static bool isAlpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}
static bool isDigit(char c) { return c >= '0' && c <= '9'; }

static bool isAtEnd(void)   { return *scanner.current == '\0'; }
static char advance(void)   { return *scanner.current++; }      /* consume one   */
static char peek(void)      { return *scanner.current; }        /* look, no move */
static char peekNext(void)  { return isAtEnd() ? '\0' : scanner.current[1]; }

/* Conditionally consume the next char if it equals `expected` (maximal munch). */
static bool match(char expected)
{
    if (isAtEnd() || *scanner.current != expected) return false;
    scanner.current++;
    return true;
}

/* Build a token spanning [start, current). Points into the source; owns nothing. */
static Token makeToken(TokenType type)
{
    Token token;
    token.type   = type;
    token.start  = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.line   = scanner.line;
    return token;
}

/* An error token carries a message literal in place of a lexeme. */
static Token errorToken(const char *message)
{
    Token token;
    token.type   = TOKEN_ERROR;
    token.start  = message;
    token.length = (int)strlen(message);
    token.line   = scanner.line;
    return token;
}

/* Consume whitespace and `//` line comments so scanToken() always returns a real
 * token. Newlines bump the line counter (for error messages). */
static void skipWhitespace(void)
{
    for (;;) {
        char c = peek();
        switch (c) {
        case ' ':
        case '\r':
        case '\t': advance(); break;
        case '\n': scanner.line++; advance(); break;
        case '/':
            if (peekNext() == '/') {
                while (peek() != '\n' && !isAtEnd()) advance();  /* to EOL       */
            } else {
                return;                     /* a real slash (division)           */
            }
            break;
        default:
            return;
        }
    }
}

/* Tail-matching helper for the keyword trie: does the rest of the current lexeme
 * equal `rest` (of length `length`, starting at offset `start`)? */
static TokenType checkKeyword(int start, int length, const char *rest,
                              TokenType type)
{
    if (scanner.current - scanner.start == start + length &&
        memcmp(scanner.start + start, rest, (size_t)length) == 0) {
        return type;
    }
    return TOKEN_IDENTIFIER;
}

/* Classify a just-scanned identifier: keyword or plain identifier. The switch on
 * the first (sometimes second) letter is a minimal DFA over our keyword set. */
static TokenType identifierType(void)
{
    switch (scanner.start[0]) {
    case 'a': return checkKeyword(1, 2, "nd",   TOKEN_AND);
    case 'e': return checkKeyword(1, 3, "lse",  TOKEN_ELSE);
    case 'f':
        if (scanner.current - scanner.start > 1) {
            switch (scanner.start[1]) {
            case 'a': return checkKeyword(2, 3, "lse", TOKEN_FALSE);
            case 'o': return checkKeyword(2, 1, "r",   TOKEN_FOR);
            case 'u': return checkKeyword(2, 1, "n",   TOKEN_FUN);
            }
        }
        break;
    case 'i': return checkKeyword(1, 1, "f",     TOKEN_IF);
    case 'n': return checkKeyword(1, 2, "il",    TOKEN_NIL);
    case 'o': return checkKeyword(1, 1, "r",     TOKEN_OR);
    case 'p': return checkKeyword(1, 4, "rint",  TOKEN_PRINT);
    case 'r': return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
    case 't': return checkKeyword(1, 3, "rue",   TOKEN_TRUE);
    case 'v': return checkKeyword(1, 2, "ar",    TOKEN_VAR);
    case 'w': return checkKeyword(1, 4, "hile",  TOKEN_WHILE);
    }
    return TOKEN_IDENTIFIER;
}

static Token identifier(void)
{
    while (isAlpha(peek()) || isDigit(peek())) advance();
    return makeToken(identifierType());
}

/* A number is digits, optionally a '.' with more digits. All numbers are doubles
 * at runtime (see value.h), so we don't distinguish int/float here. */
static Token number(void)
{
    while (isDigit(peek())) advance();
    if (peek() == '.' && isDigit(peekNext())) {
        advance();                          /* consume the '.'                    */
        while (isDigit(peek())) advance();
    }
    return makeToken(TOKEN_NUMBER);
}

/* A "..." string. No escape sequences (kept minimal). Multi-line allowed; each
 * embedded newline bumps the line counter. */
static Token string(void)
{
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') scanner.line++;
        advance();
    }
    if (isAtEnd()) return errorToken("Unterminated string.");
    advance();                              /* the closing quote                  */
    return makeToken(TOKEN_STRING);
}

Token scanToken(void)
{
    skipWhitespace();
    scanner.start = scanner.current;        /* the new token starts here          */
    if (isAtEnd()) return makeToken(TOKEN_EOF);

    char c = advance();
    if (isAlpha(c)) return identifier();
    if (isDigit(c)) return number();

    switch (c) {
    case '(': return makeToken(TOKEN_LEFT_PAREN);
    case ')': return makeToken(TOKEN_RIGHT_PAREN);
    case '{': return makeToken(TOKEN_LEFT_BRACE);
    case '}': return makeToken(TOKEN_RIGHT_BRACE);
    case ',': return makeToken(TOKEN_COMMA);
    case '-': return makeToken(TOKEN_MINUS);
    case '+': return makeToken(TOKEN_PLUS);
    case ';': return makeToken(TOKEN_SEMICOLON);
    case '/': return makeToken(TOKEN_SLASH);
    case '*': return makeToken(TOKEN_STAR);
    /* maximal munch on the two-char operators */
    case '!': return makeToken(match('=') ? TOKEN_BANG_EQUAL    : TOKEN_BANG);
    case '=': return makeToken(match('=') ? TOKEN_EQUAL_EQUAL   : TOKEN_EQUAL);
    case '<': return makeToken(match('=') ? TOKEN_LESS_EQUAL    : TOKEN_LESS);
    case '>': return makeToken(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
    case '"': return string();
    }
    return errorToken("Unexpected character.");
}
