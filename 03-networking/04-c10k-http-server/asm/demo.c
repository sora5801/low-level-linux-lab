/* ===========================================================================
 * asm/demo.c — the HTTP request-parser STATE MACHINE STEP, distilled.
 * ===========================================================================
 *
 * This is a self-contained extraction of the single most instructive routine in
 * the C10k server: the incremental, partial-read-tolerant parser that turns a
 * stream of bytes into a parsed request head (see ../http_parser.c for the full
 * version with header semantics). It is written with:
 *
 *   - NO #include and NO system headers   -> "freestanding"; clang emits clean
 *                                            teaching assembly with zero libc.
 *   - its own integer types                -> depends on nothing.
 *
 * It exists to be turned into assembly:
 *
 *   clang --target=x86_64-pc-linux-gnu -S -O0 -fno-asynchronous-unwind-tables \
 *         -fno-jump-tables -fno-omit-frame-pointer demo.c -o demo.O0.s
 *   clang --target=x86_64-pc-linux-gnu -S -O1 -fno-asynchronous-unwind-tables \
 *         -fno-jump-tables -fno-omit-frame-pointer demo.c -o demo.s
 *   clang --target=x86_64-pc-linux-gnu -S -O2 -fno-asynchronous-unwind-tables \
 *         -fno-jump-tables demo.c -o demo.O2.s
 *
 * demo.annotated.s is hand-written from demo.s (-O1). The lesson to watch for:
 * a `switch` over a state enum, compiled WITHOUT a jump table (we pass
 * -fno-jump-tables), becomes a ladder of compare-and-branch — which is exactly
 * how a state machine "feels" at the instruction level.
 * ===========================================================================
 */

/* Freestanding integer types (LP64: unsigned long is 64-bit on x86-64 Linux). */
typedef unsigned char  u8;
typedef unsigned long  usize;

/* Parser states, one per position in the request-line/header grammar. Value 0
 * is the start state so a zeroed struct is a fresh parse. */
enum {
    S_METHOD = 0,   /* reading the method token, e.g. "GET"                     */
    S_TARGET,       /* reading the request-target, e.g. "/index.html"          */
    S_VERSION,      /* matching the literal "HTTP/1."                           */
    S_MINOR,        /* the single minor-version digit                          */
    S_CR,           /* expecting CR at end of the request line                 */
    S_LF,           /* expecting LF at end of the request line                 */
    S_H_START,      /* start of a header line (or the blank line that ends it) */
    S_H_NAME,       /* header field-name up to ':'                             */
    S_H_OWS,        /* optional whitespace after ':'                           */
    S_H_VALUE,      /* header field-value up to CR                             */
    S_H_CR,         /* expecting LF after a header's CR                        */
    S_END_LF,       /* expecting the final LF of the terminating blank line    */
    S_DONE,         /* the whole head is parsed                                */
    S_ERR           /* malformed input or a limit was exceeded                 */
};

/* Result codes: >= 0 means "no error". */
enum { R_MORE = 0, R_DONE = 1, R_ERR = -1 };

/* The parser's persistent state. Field boundaries are (offset,length) INTO the
 * caller's buffer — zero copy. Small enough that the whole thing lives happily
 * in registers/stack; watch the asm spill only what it must. */
struct req {
    int   state;         /* current state (enum above)                         */
    usize parsed;        /* bytes consumed so far (resume point)               */
    usize method_off, method_len;
    usize target_off, target_len;
    int   minor;         /* HTTP minor version, or -1 if unknown               */
    unsigned num_headers;/* count of headers seen                              */
    unsigned head_bytes; /* running byte count -> the slowloris ceiling        */
    u8    ver_idx;       /* progress while matching "HTTP/1."                   */
};

#define MAX_METHOD_LEN   16
#define MAX_HEADERS      64
#define MAX_HEAD_BYTES 8192

/* RFC 7230 "token" test: the characters legal in a method or header name.
 * Kept as branchy logic (not a table) so the emitted asm mirrors the source;
 * at -O1 clang inlines this into the state machine. */
static int is_tchar(u8 c)
{
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9'))
        return 1;
    switch (c) {
    case '!': case '#': case '$': case '%': case '&': case '\'':
    case '*': case '+': case '-': case '.': case '^': case '_':
    case '`': case '|': case '~':
        return 1;
    default:
        return 0;
    }
}

/* ---------------------------------------------------------------------------
 * hp_parse — advance the state machine over buf[req->parsed .. len).
 *
 * SysV AMD64 ABI: req in %rdi, buf in %rsi, len in %rdx; result in %eax.
 * Returns R_DONE / R_MORE / R_ERR. Resumable: call again with more bytes when
 * it returns R_MORE. Every byte drives exactly one transition (O(n) total).
 * --------------------------------------------------------------------------- */
int hp_parse(struct req *req, const char *buf, usize len)
{
    usize i = req->parsed;

    for (; i < len; i++) {
        u8 c = (u8)buf[i];

        /* slowloris ceiling: every byte of the head counts */
        if (++req->head_bytes > MAX_HEAD_BYTES) { req->state = S_ERR; break; }

        switch (req->state) {
        case S_METHOD:
            if (c == ' ') {
                if (req->method_len == 0) { req->state = S_ERR; goto out; }
                req->target_off = i + 1;
                req->state = S_TARGET;
            } else if (is_tchar(c)) {
                if (req->method_len == 0) req->method_off = i;
                if (++req->method_len > MAX_METHOD_LEN) { req->state = S_ERR; goto out; }
            } else { req->state = S_ERR; goto out; }
            break;

        case S_TARGET:
            if (c == ' ') {
                if (req->target_len == 0) { req->state = S_ERR; goto out; }
                req->ver_idx = 0;
                req->state = S_VERSION;
            } else if (c <= 0x20 || c == 0x7f) {
                req->state = S_ERR; goto out;
            } else {
                if (req->target_len == 0) req->target_off = i;
                req->target_len++;
            }
            break;

        case S_VERSION: {
            static const char V[] = "HTTP/1.";
            if (c != (u8)V[req->ver_idx]) { req->state = S_ERR; goto out; }
            if (++req->ver_idx == 7) req->state = S_MINOR;
            break;
        }

        case S_MINOR:
            if (c < '0' || c > '9') { req->state = S_ERR; goto out; }
            req->minor = c - '0';
            req->state = S_CR;
            break;

        case S_CR:
            if (c != '\r') { req->state = S_ERR; goto out; }
            req->state = S_LF;
            break;

        case S_LF:
            if (c != '\n') { req->state = S_ERR; goto out; }
            req->state = S_H_START;
            break;

        case S_H_START:
            if (c == '\r') {
                req->state = S_END_LF;
            } else if (is_tchar(c)) {
                if (req->num_headers >= MAX_HEADERS) { req->state = S_ERR; goto out; }
                req->state = S_H_NAME;
            } else { req->state = S_ERR; goto out; }
            break;

        case S_H_NAME:
            if (c == ':') req->state = S_H_OWS;
            else if (!is_tchar(c)) { req->state = S_ERR; goto out; }
            break;

        case S_H_OWS:
            if (c == ' ' || c == '\t') { /* skip leading OWS */ }
            else if (c == '\r') { req->num_headers++; req->state = S_H_CR; }
            else if (c == '\n' || c == 0x7f) { req->state = S_ERR; goto out; }
            else req->state = S_H_VALUE;
            break;

        case S_H_VALUE:
            if (c == '\r') { req->num_headers++; req->state = S_H_CR; }
            else if (c == '\n') { req->state = S_ERR; goto out; }
            break;

        case S_H_CR:
            if (c != '\n') { req->state = S_ERR; goto out; }
            req->state = S_H_START;
            break;

        case S_END_LF:
            if (c != '\n') { req->state = S_ERR; goto out; }
            req->state = S_DONE;
            i++;
            goto out;

        default:
            req->state = S_ERR; goto out;
        }
    }

out:
    req->parsed = i;
    if (req->state == S_ERR)  return R_ERR;
    if (req->state == S_DONE) return R_DONE;
    return R_MORE;
}
