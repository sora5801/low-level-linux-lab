/* ===========================================================================
 * http_parser.h — an INCREMENTAL, restartable HTTP/1.1 request-head parser.
 * ===========================================================================
 *
 * WHY THIS FILE HAS NO #include
 * -----------------------------
 * A C10k server lives and dies by its request parser, and this one is written
 * to be understood *and* to be self-contained: it pulls in ZERO system headers
 * and defines its own integer types. Two payoffs:
 *
 *   1. It is "freestanding" — you can drop it into a kernel, a unikernel, or a
 *      sandbox with no libc, and it just works. Parsing bytes needs no syscalls.
 *   2. Because it is self-contained, `clang -S` on http_parser.c produces clean
 *      teaching assembly with no libc noise (see asm/http_parser.s). That is
 *      the whole didactic point of this lab.
 *
 * THE DESIGN: a byte-at-a-time state machine, tolerant of PARTIAL reads.
 * ---------------------------------------------------------------------
 * An edge-triggered, non-blocking server never gets to "read the whole
 * request" in one call — recv() hands you whatever bytes happen to have
 * arrived: maybe "GE", maybe "GET / HTTP/1.1\r\nHo", maybe three requests at
 * once. The parser must therefore be *resumable*: you feed it every byte
 * exactly once, it advances a finite-state machine, and it remembers where it
 * was between calls via `parsed`. This is precisely what defeats (and detects)
 * a slowloris attacker who dribbles one byte per second: we never block waiting
 * for a full line, and every byte counts against a hard ceiling (head_bytes).
 *
 * We record field boundaries as (offset,length) pairs INTO THE CALLER'S BUFFER
 * rather than copying strings out. Zero allocation, zero copy: the connection
 * owns one growing read buffer, and the parser just points into it. The caller
 * must keep that buffer stable (append-only) for the lifetime of the request.
 * ===========================================================================
 */
#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

/* Our own fixed-width-ish types so we depend on nothing. On the LP64 model
 * Linux uses for x86-64, `unsigned long` is 64 bits and holds any buffer
 * offset or size without truncation; `unsigned int` is 32 bits. */
typedef unsigned char  hp_u8;
typedef unsigned int   hp_u32;
typedef unsigned long  hp_size;

/* -------- Hard limits: these ARE the slowloris / resource-exhaustion defense.
 * A well-behaved request head is a few hundred bytes with a handful of headers.
 * Anything past these ceilings is either broken or hostile, and we reject it
 * (a real server answers 400 or 431). Bounding every array here means a single
 * connection can never make us allocate or scan without limit. */
#define HP_MAX_HEADERS       64      /* most-headers a request may carry       */
#define HP_MAX_METHOD_LEN    16      /* "OPTIONS" is 7; 16 is generous         */
#define HP_MAX_HEADER_BYTES  8192    /* total bytes of the request HEAD        */

/* Parser states. One per "where the cursor is" in the grammar:
 *
 *   request-line = method SP request-target SP HTTP-version CRLF
 *   header-field = field-name ":" OWS field-value OWS CRLF
 *   head         = request-line *( header-field CRLF ) CRLF
 *
 * The machine walks left to right, one byte per transition, never backing up.
 * Keep this enum in sync with the switch in hp_execute(). */
enum hp_state {
    ST_METHOD = 0,      /* reading the method token ("GET", "POST", ...)       */
    ST_TARGET,          /* reading the request-target ("/index.html")          */
    ST_VERSION,         /* matching the literal "HTTP/1."                       */
    ST_VERSION_MINOR,   /* the single minor-version digit (0 or 1)             */
    ST_REQLINE_CR,      /* expecting the CR that ends the request line          */
    ST_REQLINE_LF,      /* expecting the LF that ends the request line          */
    ST_HEADER_START,    /* at column 0 of a header line: header, or blank->end  */
    ST_HEADER_NAME,     /* reading a field-name up to the ':'                   */
    ST_HEADER_OWS,      /* skipping optional whitespace after the ':'           */
    ST_HEADER_VALUE,    /* reading the field-value up to CR                     */
    ST_HEADER_LF,       /* expecting the LF that ends a header line             */
    ST_HEADERS_END_LF,  /* expecting the final LF of the terminating blank line */
    ST_DONE,            /* the full head is parsed — hand off to the app        */
    ST_ERROR            /* malformed input or a limit exceeded — send 400/431   */
};

/* Return codes from hp_execute(). Chosen so `>= 0` means "no error". */
enum hp_result {
    HP_OK_MORE =  0,    /* consumed all input, head incomplete: recv() more     */
    HP_OK_DONE =  1,    /* the request head is fully parsed                     */
    HP_ERR     = -1     /* malformed request or limit exceeded                  */
};

/* One (offset,length) slice into the caller's read buffer. `off` is the byte
 * index where the field starts; `len` its length. Nothing is NUL-terminated —
 * you compare with an explicit length, which is also how you stay safe against
 * embedded NULs an attacker might inject. */
struct hp_slice {
    hp_size off;
    hp_size len;
};

/* The whole parser state. Zero-initialize it (all fields 0) before the first
 * byte of a new request; state ST_METHOD == 0, so a memset(0) is a valid
 * "fresh request" and keep-alive reuse just memsets again. */
typedef struct {
    int      state;              /* current enum hp_state                       */
    hp_size  parsed;             /* bytes of the buffer already CONSUMED        */

    struct hp_slice method;      /* e.g. "GET"                                  */
    struct hp_slice target;      /* e.g. "/index.html?q=1"                      */
    int      minor_version;      /* 0 => HTTP/1.0, 1 => HTTP/1.1, -1 unknown    */

    /* Scratch for the header currently being read. */
    struct hp_slice cur_name;
    struct hp_slice cur_value;

    /* Parsed headers, stored as parallel slices (SoA: better cache behavior
     * than an array of {name,value} structs when you scan just the names). */
    struct hp_slice hdr_name[HP_MAX_HEADERS];
    struct hp_slice hdr_value[HP_MAX_HEADERS];
    hp_u32   num_headers;

    /* Derived request semantics the event loop needs. */
    int      keep_alive;         /* 1 keep the connection, 0 close after reply  */
    long     content_length;     /* body length, or -1 if no Content-Length     */

    hp_u32   head_bytes;         /* running total: the slowloris ceiling check  */
    hp_u8    ver_idx;            /* progress index while matching "HTTP/1."     */
} http_request;

/* ---------------------------------------------------------------------------
 * hp_execute — feed newly-arrived bytes to the parser.
 *
 *   req  : parser state (must survive across calls for one request).
 *   buf  : the connection's ENTIRE read buffer, base pointer. Must be stable
 *          and append-only: earlier bytes must not move, because `req` stores
 *          offsets into it.
 *   len  : total valid bytes currently in `buf` (old + newly recv()'d).
 *
 * The parser resumes at buf[req->parsed] and walks to buf[len), updating
 * req->parsed as it consumes. It NEVER reads past `len`, so a request split
 * across many recv() calls is parsed correctly with no re-scanning of old
 * bytes (each byte drives exactly one transition, total work is O(bytes)).
 *
 * Returns HP_OK_DONE / HP_OK_MORE / HP_ERR (see enum hp_result).
 * --------------------------------------------------------------------------- */
int hp_execute(http_request *req, const char *buf, hp_size len);

/* Case-insensitive compare of a buffer slice against a NUL-terminated ASCII
 * literal. Exposed because the event loop reuses it to route on the method and
 * to read headers the parser did not special-case. Returns 1 on match. */
int hp_slice_ci_eq(const char *buf, struct hp_slice s, const char *lit);

#endif /* HTTP_PARSER_H */
