/* ===========================================================================
 * scanner.c — the hand-written lexer.
 * ===========================================================================
 *
 * A single global cursor walks the source once, left to right. There is no
 * regex engine and no table: each token shape is recognized by a tiny amount
 * of explicit C, which is both the fastest approach and the clearest to read.
 * The scanner never allocates — tokens borrow pointers into the source buffer.
 * ===========================================================================
 */
#include <stdbool.h>
#include <string.h>

#include "scanner.h"

/* The scanner's entire mutable state. `start` marks the beginning of the token
 * currently being scanned; `current` is the read cursor. The lexeme in flight
 * is always the half-open range [start, current). */
typedef struct {
    const char *start;
    const char *current;
    int         line;
} Scanner;

static Scanner scanner;

void initScanner(const char *source)
{
    scanner.start   = source;
    scanner.current = source;
    scanner.line    = 1;
}

/* --- character classification (no <ctype.h>: it is locale-dependent and we
 * want a fixed ASCII grammar) --------------------------------------------- */
static bool isAlpha(char c)
{
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
            c == '_';                 /* identifiers may start with underscore  */
}
static bool isDigit(char c) { return c >= '0' && c <= '9'; }

/* current points at the NUL that terminates the source when we are done. */
static bool isAtEnd(void)   { return *scanner.current == '\0'; }

/* Consume and return the current character. */
static char advance(void)   { scanner.current++; return scanner.current[-1]; }

/* Look at the current / next character WITHOUT consuming (needed for maximal
 * munch and for deciding when a number/comment ends). */
static char peek(void)      { return *scanner.current; }
static char peekNext(void)
{
    if (isAtEnd()) return '\0';
    return scanner.current[1];
}

/* Conditionally consume: if the current char is `expected`, eat it and return
 * true. This is how "!" vs "!=" and "<" vs "<=" are disambiguated. */
static bool match(char expected)
{
    if (isAtEnd()) return false;
    if (*scanner.current != expected) return false;
    scanner.current++;
    return true;
}

/* Build a token spanning [start, current) with the given type. */
static Token makeToken(TokenType type)
{
    Token token;
    token.type   = type;
    token.start  = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.line   = scanner.line;
    return token;
}

/* An error token carries a message pointer (a string literal, so no allocation)
 * instead of a lexeme. The compiler turns it into a diagnostic. */
static Token errorToken(const char *message)
{
    Token token;
    token.type   = TOKEN_ERROR;
    token.start  = message;
    token.length = (int)strlen(message);
    token.line   = scanner.line;
    return token;
}

/* Skip whitespace and comments between tokens, tracking line numbers so error
 * messages point at the right place. Comments are `// to end of line`. */
static void skipWhitespace(void)
{
    for (;;) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;
            case '\n':
                scanner.line++;   /* the ONLY place line is incremented         */
                advance();
                break;
            case '/':
                if (peekNext() == '/') {
                    /* line comment: consume until the newline (but leave the
                     * newline so the case above counts the line). */
                    while (peek() != '\n' && !isAtEnd()) advance();
                } else {
                    return;       /* a real slash (division) — stop skipping     */
                }
                break;
            default:
                return;
        }
    }
}

/* Keyword recognition via a trie built out of nested switches: cheaper than a
 * hash lookup and it makes the keyword set obvious. We only pay a strcmp-like
 * `checkKeyword` when the prefix already matches, so most identifiers are
 * rejected as keywords after a single character comparison. */
static TokenType checkKeyword(int start, int length,
                              const char *rest, TokenType type)
{
    /* The candidate identifier must be EXACTLY this long and its tail must
     * match `rest`; otherwise it is a user identifier that merely shares a
     * prefix (e.g. "form" vs the keyword "for"). */
    if (scanner.current - scanner.start == start + length &&
        memcmp(scanner.start + start, rest, (size_t)length) == 0) {
        return type;
    }
    return TOKEN_IDENTIFIER;
}

static TokenType identifierType(void)
{
    switch (scanner.start[0]) {
        case 'a': return checkKeyword(1, 2, "nd",    TOKEN_AND);
        case 'e': return checkKeyword(1, 3, "lse",   TOKEN_ELSE);
        case 'f':
            /* two keywords start with 'f' — branch on the second letter */
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
    /* First char was already known alpha; consume the alnum/underscore run. */
    while (isAlpha(peek()) || isDigit(peek())) advance();
    return makeToken(identifierType());
}

static Token number(void)
{
    while (isDigit(peek())) advance();
    /* This VM has integer-only numbers, so we deliberately do NOT scan a
     * fractional part. A '.' after digits is left for the parser (there is no
     * float literal). Keeping numbers integral is the systems-lab choice:
     * overflow and truncating division are the edge cases we care about. */
    return makeToken(TOKEN_NUMBER);
}

static Token string(void)
{
    /* Consume until the closing quote, allowing newlines (multi-line strings).
     * We do NOT process escape sequences here — a teaching simplification noted
     * in the README; the raw bytes between the quotes become the string. */
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') scanner.line++;
        advance();
    }
    if (isAtEnd()) return errorToken("Unterminated string.");
    advance();   /* consume the closing quote                                   */
    return makeToken(TOKEN_STRING);
}

Token scanToken(void)
{
    skipWhitespace();
    scanner.start = scanner.current;   /* the new token begins here             */

    if (isAtEnd()) return makeToken(TOKEN_EOF);

    char c = advance();
    if (isAlpha(c)) return identifier();
    if (isDigit(c)) return number();

    switch (c) {
        case '(': return makeToken(TOKEN_LEFT_PAREN);
        case ')': return makeToken(TOKEN_RIGHT_PAREN);
        case '{': return makeToken(TOKEN_LEFT_BRACE);
        case '}': return makeToken(TOKEN_RIGHT_BRACE);
        case ';': return makeToken(TOKEN_SEMICOLON);
        case ',': return makeToken(TOKEN_COMMA);
        case '.': return makeToken(TOKEN_DOT);
        case '-': return makeToken(TOKEN_MINUS);
        case '+': return makeToken(TOKEN_PLUS);
        case '/': return makeToken(TOKEN_SLASH);
        case '*': return makeToken(TOKEN_STAR);
        /* Maximal munch: prefer the two-char token when the second char fits. */
        case '!': return makeToken(match('=') ? TOKEN_BANG_EQUAL    : TOKEN_BANG);
        case '=': return makeToken(match('=') ? TOKEN_EQUAL_EQUAL   : TOKEN_EQUAL);
        case '<': return makeToken(match('=') ? TOKEN_LESS_EQUAL    : TOKEN_LESS);
        case '>': return makeToken(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
        case '"': return string();
    }

    return errorToken("Unexpected character.");
}
