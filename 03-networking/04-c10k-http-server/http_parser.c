/* ===========================================================================
 * http_parser.c — implementation of the incremental HTTP/1.1 head parser.
 * ===========================================================================
 *
 * Read http_parser.h first for the design rationale. This file is deliberately
 * free of system headers so it compiles "freestanding" and yields clean
 * teaching assembly (asm/http_parser.s). Every helper below operates purely on
 * bytes — no allocation, no syscalls, no libc.
 *
 * THE GRAMMAR WE ACCEPT (RFC 7230, simplified but not sloppy):
 *
 *     request-line = method SP request-target SP "HTTP/1." DIGIT CRLF
 *     header-field = field-name ":" OWS field-value OWS CRLF
 *     head         = request-line *header-field CRLF
 *
 * where SP is one space, CRLF is "\r\n", OWS is optional spaces/tabs, method
 * and field-name are RFC "tokens", and DIGIT is the minor version. We are
 * strict about structure (this is a security boundary) but lenient about a few
 * cosmetic things, each flagged below.
 * ===========================================================================
 */
#include "http_parser.h"

/* --- Character classification -------------------------------------------- *
 * RFC 7230 "token" characters: what a method or a header field-name may
 * contain. Everything outside this set in those positions is a hard error, so
 * "GET\0/etc" or "Foo Bar: x" cannot sneak through. We spell the set out with a
 * branchy predicate rather than a 256-byte table because at -O1 the compiler
 * keeps it inline and the asm reads as exactly this logic (see the .s file);
 * a production parser would use a lookup table for one-cycle classification. */
static int is_tchar(unsigned char c)
{
    /* letters and digits are the common case, tested first */
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9'))
        return 1;
    /* the RFC's "special" token punctuation */
    switch (c) {
    case '!': case '#': case '$': case '%': case '&': case '\'':
    case '*': case '+': case '-': case '.': case '^': case '_':
    case '`': case '|': case '~':
        return 1;
    default:
        return 0;
    }
}

/* ASCII lowercase for one byte. Header names are case-insensitive per the RFC
 * ("Content-Length" == "content-length"), so all header matching folds case.
 * Branchless-ish: only letters are shifted; digits/punctuation pass through. */
static unsigned char lc(unsigned char c)
{
    return (c >= 'A' && c <= 'Z') ? (unsigned char)(c + 32) : c;
}

/* Case-insensitive compare of buf[s.off .. s.off+s.len) against a C string.
 * Returns 1 iff same length and same bytes ignoring ASCII case. */
int hp_slice_ci_eq(const char *buf, struct hp_slice s, const char *lit)
{
    hp_size i = 0;
    for (; i < s.len; i++) {
        unsigned char a = lc((unsigned char)buf[s.off + i]);
        unsigned char b = lc((unsigned char)lit[i]);
        if (b == 0)   return 0;              /* literal ended first: shorter    */
        if (a != b)   return 0;              /* mismatch                        */
    }
    return lit[s.len] == 0;                  /* both ended together => equal    */
}

/* Case-insensitive substring search: does the value slice contain `needle`
 * anywhere, ignoring case? Used for "Connection: keep-alive, close" style
 * lists. This is a simplification — a fully correct parser splits on commas and
 * compares whole tokens — but it is safe here because our needles ("close",
 * "keep-alive") never appear as substrings of other legal connection-options. */
static int slice_ci_contains(const char *buf, struct hp_slice s, const char *needle)
{
    hp_size n = 0;
    while (needle[n]) n++;                    /* strlen(needle), no libc         */
    if (n == 0 || s.len < n) return 0;
    for (hp_size i = 0; i + n <= s.len; i++) {
        hp_size j = 0;
        for (; j < n; j++)
            if (lc((unsigned char)buf[s.off + i + j]) != lc((unsigned char)needle[j]))
                break;
        if (j == n) return 1;                 /* matched all n bytes             */
    }
    return 0;
}

/* Finalize the header we just finished reading: (1) trim trailing OWS from the
 * value, (2) stash the (name,value) slices, (3) extract the two headers the
 * event loop cares about (Connection, Content-Length). Returns 0 on success,
 * -1 if we blew the header count (too many headers => reject). */
static int finish_header(http_request *req, const char *buf)
{
    struct hp_slice name = req->cur_name;
    struct hp_slice val  = req->cur_value;

    /* Trim trailing spaces/tabs from the value. RFC 7230 says field-value has
     * no leading/trailing OWS; we already skipped leading OWS in ST_HEADER_OWS,
     * so only the tail can remain. */
    while (val.len > 0) {
        unsigned char last = (unsigned char)buf[val.off + val.len - 1];
        if (last == ' ' || last == '\t') val.len--;
        else break;
    }

    if (req->num_headers >= HP_MAX_HEADERS)
        return -1;                            /* too many headers: 431 territory */
    req->hdr_name[req->num_headers]  = name;
    req->hdr_value[req->num_headers] = val;
    req->num_headers++;

    /* Connection header overrides the version's default keep-alive policy. */
    if (hp_slice_ci_eq(buf, name, "connection")) {
        if (slice_ci_contains(buf, val, "close"))
            req->keep_alive = 0;
        else if (slice_ci_contains(buf, val, "keep-alive"))
            req->keep_alive = 1;
    }
    /* Content-Length tells us how long the body is (we parse GET/HEAD here, so
     * we mostly use it to know how many body bytes to drain/ignore). Parse a
     * non-negative decimal with an overflow guard: a hostile "99999999999999999
     * 9999" must not wrap into a small positive length. */
    else if (hp_slice_ci_eq(buf, name, "content-length")) {
        long v = 0;
        int ok = val.len > 0;
        for (hp_size i = 0; i < val.len; i++) {
            unsigned char d = (unsigned char)buf[val.off + i];
            if (d < '0' || d > '9') { ok = 0; break; }
            if (v > (long)((~0UL >> 1) - 9) / 10) { ok = 0; break; } /* overflow */
            v = v * 10 + (d - '0');
        }
        req->content_length = ok ? v : -1;
    }
    return 0;
}

/* ---------------------------------------------------------------------------
 * hp_execute — the state machine. One switch, one byte per iteration.
 *
 * The loop invariant: on entry `req->parsed` is the index of the first byte we
 * have NOT yet consumed; we advance until we hit `len` (need more data) or a
 * terminal state (DONE / ERROR). Because we resume from `parsed`, feeding the
 * same growing buffer repeatedly re-processes nothing — each byte transitions
 * the machine exactly once.
 * --------------------------------------------------------------------------- */
int hp_execute(http_request *req, const char *buf, hp_size len)
{
    hp_size i = req->parsed;

    for (; i < len; i++) {
        unsigned char c = (unsigned char)buf[i];

        /* SLOWLORIS CEILING. Every byte of the head is counted; an attacker who
         * trickles bytes to hold the connection open forever still hits this
         * bound and gets cut off. This single check is why a partial-read-
         * tolerant parser is not itself a denial-of-service hole. */
        if (++req->head_bytes > HP_MAX_HEADER_BYTES) {
            req->state = ST_ERROR;
            break;
        }

        switch (req->state) {

        /* ---- request-line: METHOD ------------------------------------- */
        case ST_METHOD:
            if (c == ' ') {                          /* SP ends the method     */
                if (req->method.len == 0) { req->state = ST_ERROR; goto out; }
                req->state = ST_TARGET;
                req->target.off = i + 1;             /* target starts next byte */
            } else if (is_tchar(c)) {
                if (req->method.len == 0) req->method.off = i;
                if (++req->method.len > HP_MAX_METHOD_LEN) {
                    req->state = ST_ERROR; goto out;
                }
            } else {
                req->state = ST_ERROR; goto out;     /* e.g. control char       */
            }
            break;

        /* ---- request-line: TARGET ------------------------------------- */
        case ST_TARGET:
            if (c == ' ') {                          /* SP ends the target     */
                if (req->target.len == 0) { req->state = ST_ERROR; goto out; }
                req->state = ST_VERSION;
                req->ver_idx = 0;
            } else if (c <= 0x20 || c == 0x7f) {
                /* controls and space are illegal inside the target; this also
                 * rejects a bare CR/LF that would truncate the request line. */
                req->state = ST_ERROR; goto out;
            } else {
                if (req->target.len == 0) req->target.off = i;
                req->target.len++;
                /* no explicit target cap here: the head_bytes ceiling already
                 * bounds it, and a real server maps target->file with its own
                 * PATH_MAX check downstream. */
            }
            break;

        /* ---- request-line: "HTTP/1." literal -------------------------- */
        case ST_VERSION: {
            static const char V[] = "HTTP/1.";       /* 7 bytes, then a digit   */
            if (c != (unsigned char)V[req->ver_idx]) { req->state = ST_ERROR; goto out; }
            if (++req->ver_idx == 7) req->state = ST_VERSION_MINOR;
            break;
        }

        /* ---- request-line: minor version digit ------------------------ */
        case ST_VERSION_MINOR:
            if (c < '0' || c > '9') { req->state = ST_ERROR; goto out; }
            req->minor_version = c - '0';
            /* Default keep-alive policy comes from the version: HTTP/1.1
             * defaults to persistent connections, HTTP/1.0 does not. A
             * Connection header (parsed later) can still override this. */
            req->keep_alive = (req->minor_version >= 1) ? 1 : 0;
            req->state = ST_REQLINE_CR;
            break;

        case ST_REQLINE_CR:
            if (c != '\r') { req->state = ST_ERROR; goto out; }
            req->state = ST_REQLINE_LF;
            break;

        case ST_REQLINE_LF:
            if (c != '\n') { req->state = ST_ERROR; goto out; }
            req->state = ST_HEADER_START;
            break;

        /* ---- header block: start of a line ---------------------------- */
        case ST_HEADER_START:
            if (c == '\r') {
                /* a blank line (CRLF on its own) terminates the head */
                req->state = ST_HEADERS_END_LF;
            } else if (is_tchar(c)) {
                req->cur_name.off = i;
                req->cur_name.len = 1;
                req->state = ST_HEADER_NAME;
            } else {
                /* Note: this also rejects obs-fold (a header line starting with
                 * SP/TAB), which RFC 7230 deprecates and servers may reject. */
                req->state = ST_ERROR; goto out;
            }
            break;

        case ST_HEADER_NAME:
            if (c == ':') {
                req->state = ST_HEADER_OWS;
                req->cur_value.off = i + 1;          /* provisional             */
                req->cur_value.len = 0;
            } else if (is_tchar(c)) {
                req->cur_name.len++;
            } else {
                req->state = ST_ERROR; goto out;     /* SP before ':' etc.      */
            }
            break;

        case ST_HEADER_OWS:
            if (c == ' ' || c == '\t') {
                /* skip leading OWS; value has not started yet */
            } else if (c == '\r') {
                req->cur_value.off = i;              /* empty value             */
                req->cur_value.len = 0;
                req->state = ST_HEADER_LF;
            } else if (c == '\n' || c == 0x7f) {
                req->state = ST_ERROR; goto out;
            } else {
                req->cur_value.off = i;
                req->cur_value.len = 1;
                req->state = ST_HEADER_VALUE;
            }
            break;

        case ST_HEADER_VALUE:
            if (c == '\r') {
                req->state = ST_HEADER_LF;
            } else if (c == '\n') {
                req->state = ST_ERROR; goto out;     /* bare LF in value        */
            } else {
                req->cur_value.len++;                /* includes interior OWS   */
            }
            break;

        case ST_HEADER_LF:
            if (c != '\n') { req->state = ST_ERROR; goto out; }
            if (finish_header(req, buf) != 0) { req->state = ST_ERROR; goto out; }
            req->state = ST_HEADER_START;            /* back for the next header */
            break;

        /* ---- terminating blank line ----------------------------------- */
        case ST_HEADERS_END_LF:
            if (c != '\n') { req->state = ST_ERROR; goto out; }
            req->state = ST_DONE;
            i++;                                     /* consume this LF too     */
            goto out;                                /* head complete           */

        default:
            req->state = ST_ERROR; goto out;
        }
    }

out:
    req->parsed = i;
    if (req->state == ST_ERROR) return HP_ERR;
    if (req->state == ST_DONE)  return HP_OK_DONE;
    return HP_OK_MORE;
}
