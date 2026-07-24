/* ===========================================================================
 * resp.c — the RESP wire protocol: parse requests, serialize replies.
 * ===========================================================================
 *
 * RESP (REdis Serialization Protocol) is line-oriented and length-prefixed.
 * A client sends a command in one of two framings:
 *
 *   INLINE     a bare, space-separated line ended by \n (or \r\n):
 *                  PING\r\n
 *                  SET foo bar\r\n
 *              Handy for telnet/debugging; no length prefixes.
 *
 *   MULTIBULK  a RESP array of bulk strings — how real clients send everything:
 *                  *3\r\n           (an array of 3 elements)
 *                  $3\r\nSET\r\n     ($3 = a 3-byte bulk string, then the bytes)
 *                  $3\r\nfoo\r\n
 *                  $3\r\nbar\r\n
 *              Binary safe: the $<len> tells us exactly how many bytes follow,
 *              so a value may contain \r, \n, or NUL.
 *
 * THE HARD PART is that TCP is a byte STREAM: a single read() may deliver half a
 * command, or three-and-a-half commands. So both parsers are RESUMABLE. They
 * consume whole tokens from c->querybuf, advance c->qpos, and stash any partial
 * multibulk state on the client so the next read picks up exactly where we left
 * off. A parser that returns 0 means "need more bytes"; nothing is lost.
 *
 * Replies are built in the other direction: addReply* append RESP-encoded bytes
 * to the client's output buffer, which the event loop later writes to the fd.
 * =========================================================================== */
#include "server.h"
#include "zmalloc.h"

#include <string.h>   /* memchr, memmove, memcpy, strlen */
#include <stdio.h>    /* snprintf                        */
#include <stdlib.h>   /* (none directly; kept minimal)   */

/* Protocol limits, mirroring Redis's defaults, that bound memory an adversary
 * can force us to buffer before a command completes. */
#define PROTO_INLINE_MAX     (64 * 1024)          /* longest inline line         */
#define PROTO_MAX_MULTIBULK  (1024 * 1024)        /* max array elements          */
#define PROTO_MAX_BULK       (512L * 1024 * 1024) /* max single bulk string      */

/* ---------------------------------------------------------------------------
 * string2ll — decimal (optionally signed) string -> long long, with strict
 * validation and overflow detection. This is the pure-logic routine the asm/
 * demo extracts, so the arithmetic is written to be transparent in assembly.
 * Returns 1 on success (and writes *out), 0 on any malformed/overflowing input.
 * ------------------------------------------------------------------------- */
int string2ll(const char *s, size_t len, long long *out)
{
    if (len == 0) return 0;

    size_t i = 0;
    int neg = 0;
    if (s[0] == '-') { neg = 1; i = 1; if (len == 1) return 0; }
    else if (s[0] == '+') { i = 1; if (len == 1) return 0; }

    /* Accumulate into an UNSIGNED value so we can detect overflow precisely
     * against the signed limits without invoking signed-overflow UB. */
    unsigned long long acc = 0;
    /* The largest magnitude we may reach: LLONG_MAX (9223372036854775807) for a
     * positive number, or LLONG_MAX+1 (…808) for the most-negative long long. */
    const unsigned long long lim = neg ? 9223372036854775808ULL
                                        : 9223372036854775807ULL;
    for (; i < len; i++) {
        unsigned c = (unsigned char)s[i] - '0';   /* map '0'..'9' -> 0..9        */
        if (c > 9) return 0;                       /* any non-digit is a failure  */
        /* Would acc*10 + c exceed the limit? Check BEFORE multiplying so we
         * never overflow the accumulator itself. */
        if (acc > (lim - c) / 10) return 0;
        acc = acc * 10 + c;                        /* shift left one decimal digit*/
    }
    *out = neg ? -(long long)acc : (long long)acc;
    return 1;
}

/* ---------------------------------------------------------------------------
 * Output side: append raw bytes / typed RESP replies to the client's buffer.
 * The buffer is a single growable char[]; the event loop drains it to the fd.
 * ------------------------------------------------------------------------- */
void addReplyRaw(client *c, const void *s, size_t len)
{
    if (c->bufpos + len > c->bufcap) {
        size_t need = c->bufpos + len;
        size_t cap  = c->bufcap ? c->bufcap : 1024;
        while (cap < need) cap <<= 1;             /* double until it fits        */
        c->buf    = zrealloc(c->buf, cap);
        c->bufcap = cap;
    }
    memcpy(c->buf + c->bufpos, s, len);
    c->bufpos += len;
}

/* ":<n>\r\n" and "$<n>\r\n" prefixes both need a decimal number then CRLF; this
 * helper writes `prefix` + the number + CRLF. */
static void addReplyLongLongWithPrefix(client *c, char prefix, long long n)
{
    char buf[32];
    /* buf[0]=prefix, then the number, then \r\n. snprintf returns the digit
     * count; +3 accounts for the prefix char handled separately below. */
    int len = snprintf(buf, sizeof(buf), "%c%lld\r\n", prefix, n);
    addReplyRaw(c, buf, (size_t)len);
}

void addReplyError(client *c, const char *err)
{
    /* RESP error: '-' <message> CRLF. Callers pass the message WITH its code,
     * e.g. "ERR unknown command". */
    addReplyRaw(c, "-", 1);
    addReplyRaw(c, err, strlen(err));
    addReplyRaw(c, "\r\n", 2);
}

void addReplyStatus(client *c, const char *status)
{
    /* Simple string: '+' <status> CRLF, e.g. "+OK\r\n". */
    addReplyRaw(c, "+", 1);
    addReplyRaw(c, status, strlen(status));
    addReplyRaw(c, "\r\n", 2);
}

void addReplyLongLong(client *c, long long ll)
{
    addReplyLongLongWithPrefix(c, ':', ll);       /* integer reply ":123\r\n"    */
}

void addReplyBulkCBuffer(client *c, const void *p, size_t len)
{
    /* Bulk string: '$' <len> CRLF <len bytes> CRLF. Binary safe. */
    addReplyLongLongWithPrefix(c, '$', (long long)len);
    addReplyRaw(c, p, len);
    addReplyRaw(c, "\r\n", 2);
}

void addReplyBulkCString(client *c, const char *s)
{
    addReplyBulkCBuffer(c, s, strlen(s));
}

void addReplyBulkSds(client *c, sds s)
{
    /* Copies the sds bytes into the reply; does NOT take ownership of `s`. */
    addReplyBulkCBuffer(c, s, sdslen(s));
}

void addReplyNull(client *c)
{
    /* RESP2 null bulk string: "$-1\r\n" (GET on a missing key returns this). */
    addReplyRaw(c, "$-1\r\n", 5);
}

void addReplyArrayLen(client *c, long n)
{
    addReplyLongLongWithPrefix(c, '*', (long long)n);  /* array header "*n\r\n"  */
}

/* ---------------------------------------------------------------------------
 * resetClientCommand — release the parsed argv and per-command parse state so
 * the client is ready to parse the NEXT command. querybuf/qpos are left alone
 * (the caller trims consumed bytes). Also used by freeClient on disconnect.
 * ------------------------------------------------------------------------- */
void resetClientCommand(client *c)
{
    for (int i = 0; i < c->argc; i++) sdsfree(c->argv[i]);
    zfree(c->argv);
    c->argv         = NULL;
    c->argc         = 0;
    c->reqtype      = 0;
    c->multibulklen = 0;
    c->bulklen      = -1;
}

/* Emit a fatal protocol error: reply, then flag the client to be closed once
 * the reply has drained. Returns -1 so the caller stops parsing. */
static int protocolError(client *c, const char *msg)
{
    addReplyError(c, msg);
    c->flags |= CLIENT_CLOSE_AFTER_REPLY;
    return -1;
}

/* ---------------------------------------------------------------------------
 * Inline command parser: split one \n-terminated line on whitespace.
 * Returns 1 (command parsed), 0 (need more bytes), -1 (protocol error).
 * ------------------------------------------------------------------------- */
static int processInlineBuffer(client *c)
{
    size_t avail = sdslen(c->querybuf) - c->qpos;
    char  *base  = c->querybuf + c->qpos;

    /* Find the end of the line. Without a newline yet, wait for more — unless
     * the pending line is already absurdly long (a slow-loris style abuse). */
    char *nl = memchr(base, '\n', avail);
    if (nl == NULL) {
        if (avail > PROTO_INLINE_MAX)
            return protocolError(c, "ERR Protocol error: too big inline request");
        return 0;                                  /* need more data            */
    }

    size_t linelen = (size_t)(nl - base);          /* bytes before the '\n'     */
    size_t consumed = linelen + 1;                 /* include the '\n'          */
    if (linelen > 0 && base[linelen - 1] == '\r')  /* trim an optional '\r'     */
        linelen--;

    /* Split [base, base+linelen) on runs of spaces/tabs into argv. First pass:
     * count tokens; then allocate; then a second pass fills them. */
    int argc = 0;
    for (size_t i = 0; i < linelen; ) {
        while (i < linelen && (base[i] == ' ' || base[i] == '\t')) i++;
        if (i >= linelen) break;
        argc++;
        while (i < linelen && base[i] != ' ' && base[i] != '\t') i++;
    }

    c->argv = zmalloc(sizeof(sds) * (argc ? argc : 1));
    c->argc = 0;
    for (size_t i = 0; i < linelen && c->argc < argc; ) {
        while (i < linelen && (base[i] == ' ' || base[i] == '\t')) i++;
        if (i >= linelen) break;
        size_t start = i;
        while (i < linelen && base[i] != ' ' && base[i] != '\t') i++;
        c->argv[c->argc++] = sdsnewlen(base + start, i - start);
    }

    c->qpos += consumed;                           /* advance past the line     */
    /* NOTE: this simple splitter does not honour quotes/escapes the way Redis's
     * sdssplitargs does — inline mode here is for hand-typed debugging only. */
    return 1;
}

/* ---------------------------------------------------------------------------
 * Multibulk command parser: the real client framing. Resumable — the client
 * carries multibulklen (args still to read), bulklen (current bulk length or
 * -1), and the partially-filled argv across calls.
 * Returns 1 (command complete), 0 (need more), -1 (protocol error).
 * ------------------------------------------------------------------------- */
static int processMultibulkBuffer(client *c)
{
    /* --- Phase 1: parse the "*<count>\r\n" array header, once per command. --- */
    if (c->multibulklen == 0) {
        char  *base  = c->querybuf + c->qpos;
        size_t avail = sdslen(c->querybuf) - c->qpos;
        char  *nl    = memchr(base, '\n', avail);
        if (nl == NULL) {
            if (avail > PROTO_INLINE_MAX)
                return protocolError(c, "ERR Protocol error: too big mbulk count");
            return 0;
        }
        /* base[0] is '*' (that is how we chose REQ_MULTIBULK). Parse the count
         * between '*' and the '\r'. */
        size_t hdrlen = (size_t)(nl - base);
        size_t numlen = (hdrlen > 0 && base[hdrlen - 1] == '\r') ? hdrlen - 1 : hdrlen;
        long long count;
        if (!string2ll(base + 1, numlen - 1, &count))
            return protocolError(c, "ERR Protocol error: invalid multibulk length");

        c->qpos += hdrlen + 1;                     /* consume "*<count>\r\n"    */

        if (count <= 0) return 1;                  /* empty array: no command    */
        if (count > PROTO_MAX_MULTIBULK)
            return protocolError(c, "ERR Protocol error: invalid multibulk length");

        c->multibulklen = (int)count;              /* args still to read         */
        c->argv = zmalloc(sizeof(sds) * count);
        c->argc = 0;
        c->bulklen = -1;                           /* no bulk length read yet    */
    }

    /* --- Phase 2: read each "$<len>\r\n<bytes>\r\n" bulk argument. --- */
    while (c->multibulklen > 0) {
        /* 2a: if we don't yet know this bulk's length, parse its "$<len>\r\n". */
        if (c->bulklen == -1) {
            char  *base  = c->querybuf + c->qpos;
            size_t avail = sdslen(c->querybuf) - c->qpos;
            char  *nl    = memchr(base, '\n', avail);
            if (nl == NULL) {
                if (avail > PROTO_INLINE_MAX)
                    return protocolError(c, "ERR Protocol error: too big bulk count");
                return 0;                          /* wait for the length line   */
            }
            if (base[0] != '$')
                return protocolError(c, "ERR Protocol error: expected '$'");

            size_t hdrlen = (size_t)(nl - base);
            size_t numlen = (hdrlen > 0 && base[hdrlen - 1] == '\r')
                            ? hdrlen - 1 : hdrlen;
            long long len;
            if (!string2ll(base + 1, numlen - 1, &len) ||
                len < 0 || len > PROTO_MAX_BULK)
                return protocolError(c, "ERR Protocol error: invalid bulk length");

            c->qpos   += hdrlen + 1;               /* consume "$<len>\r\n"       */
            c->bulklen = (long)len;
        }

        /* 2b: do we have the whole bulk payload plus its trailing CRLF yet? */
        size_t avail = sdslen(c->querybuf) - c->qpos;
        if (avail < (size_t)c->bulklen + 2)
            return 0;                              /* not all bytes arrived      */

        /* Copy out exactly bulklen bytes (binary safe), skip the CRLF. */
        c->argv[c->argc++] = sdsnewlen(c->querybuf + c->qpos, (size_t)c->bulklen);
        c->qpos     += (size_t)c->bulklen + 2;     /* payload + CRLF             */
        c->bulklen   = -1;                         /* next arg needs a length    */
        c->multibulklen--;
    }

    return 1;   /* all args read: a full command is in argv/argc */
}

/* ---------------------------------------------------------------------------
 * processInputBuffer — the driver: parse and execute every complete command
 * currently buffered, then trim the consumed prefix out of querybuf.
 * ------------------------------------------------------------------------- */
void processInputBuffer(client *c)
{
    while (c->qpos < sdslen(c->querybuf)) {
        /* Decide framing from the first byte of a fresh command. */
        if (c->reqtype == 0)
            c->reqtype = (c->querybuf[c->qpos] == '*') ? REQ_MULTIBULK : REQ_INLINE;

        int rc = (c->reqtype == REQ_MULTIBULK) ? processMultibulkBuffer(c)
                                               : processInlineBuffer(c);
        if (rc == 0) break;                        /* incomplete: await more     */
        if (rc < 0) break;                         /* protocol error: stop       */

        /* A command is ready. Empty inputs (blank inline line / "*0") have no
         * args and are simply skipped. */
        if (c->argc > 0) {
            processCommand(c);
            if (c->flags & CLIENT_CLOSE_AFTER_REPLY) { resetClientCommand(c); break; }
        }
        resetClientCommand(c);
    }

    /* Drop the bytes we consumed so querybuf does not grow without bound. This
     * is an O(remaining) memmove; real Redis is cleverer (it repositions less
     * often) but the effect is the same: unbounded pipelines stay bounded. */
    if (c->qpos > 0) {
        size_t remaining = sdslen(c->querybuf) - c->qpos;
        if (remaining) memmove(c->querybuf, c->querybuf + c->qpos, remaining);
        SDS_HDR(c->querybuf)->len = remaining;     /* shrink in place (no move)  */
        c->querybuf[remaining] = '\0';
        c->qpos = 0;
    }
}
