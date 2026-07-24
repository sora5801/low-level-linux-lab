/* ===========================================================================
 * server.c — a non-blocking, edge-triggered, keep-alive HTTP/1.1 file server
 *            built to scale toward the "C10k" regime (10k concurrent sockets).
 * ===========================================================================
 *
 * THE C10k IDEA
 * -------------
 * A thread-per-connection server dies at a few thousand clients: each thread
 * costs a stack (~8 MiB of address space) and the scheduler thrashes context-
 * switching between them. The alternative — one thread, thousands of sockets,
 * multiplexed by the kernel — is what this file demonstrates. The machinery:
 *
 *   epoll (edge-triggered)  the kernel tells us WHICH fds are ready in O(ready),
 *                           not O(total). Unlike select/poll it does not re-scan
 *                           every fd on every wait.
 *   O_NONBLOCK + accept4    no syscall ever blocks the single thread; if there
 *                           is nothing to do on one fd we move to the next.
 *   a resumable parser      recv() returns partial data, so the HTTP parser is a
 *                           byte-at-a-time state machine (see http_parser.c).
 *   sendfile()              static bodies go from page cache to socket with zero
 *                           copies through user space.
 *   SO_REUSEPORT            N worker processes each bind the SAME port; the
 *                           kernel load-balances new connections across them, so
 *                           we scale across cores with no shared accept lock.
 *
 * EDGE-TRIGGERED (EPOLLET) vs LEVEL-TRIGGERED — the rule you must internalize:
 *   Level-triggered: epoll keeps reporting "readable" as long as ANY data is
 *     buffered. Easy, but a busy fd can be reported every loop.
 *   Edge-triggered: epoll reports readiness only on the *transition* from
 *     not-ready to ready (a new packet arrives). If you do not read until recv()
 *     returns EAGAIN, the bytes still sitting in the socket buffer will NOT
 *     wake you again — you hang. Therefore: on every ET wakeup you MUST drain
 *     the fd to EAGAIN (reads) and write until EAGAIN (writes). Every loop in
 *     this file that ends on EAGAIN exists for exactly this reason.
 *
 * SCOPE (this is a teaching CORE, honestly bounded — see README "Scope"):
 *   - serves GET and HEAD for static files under a document root;
 *   - HTTP/1.1 keep-alive, including back-to-back (pipelined) bodyless requests;
 *   - correct partial-read / slowloris handling via the incremental parser;
 *   - it does NOT implement request bodies (POST/PUT), chunked transfer,
 *     TLS, or HTTP/2/3. A request that carries a body is answered then closed.
 * ===========================================================================
 */
#define _GNU_SOURCE          /* accept4, SO_REUSEPORT, EPOLLRDHUP, MSG_NOSIGNAL,
                              * sendfile, gmtime_r all live behind this feature
                              * macro in glibc; it must precede every #include. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "http_parser.h"

/* --- Tunables -------------------------------------------------------------- */
#define READ_BUF      16384   /* per-connection request buffer (head must fit)  */
#define MAX_RESP_HEAD  4096   /* per-connection response-head buffer            */
#define MAX_EVENTS     1024   /* epoll_wait batch size                          */
#define DEFAULT_PORT   8080
#define DEFAULT_ROOT   "./www"

/* epoll interest masks. EPOLLET makes them edge-triggered; EPOLLRDHUP fires
 * when the peer closes its writing half (a half-close we want to notice). We
 * watch reads OR writes for a given connection, never both at once, which is
 * what makes it safe to free a connection inside its handler (see worker_loop).*/
#define EV_READ   (EPOLLIN  | EPOLLRDHUP | EPOLLET)
#define EV_WRITE  (EPOLLOUT | EPOLLRDHUP | EPOLLET)

/* A connection can be waiting to READ a request or WRITE its response. */
enum conn_phase { PH_READING, PH_WRITING };

/* Per-connection state. One malloc'd instance per client socket; the pointer
 * rides in epoll_event.data.ptr so a wakeup hands us the state in O(1) with no
 * lookup table. The listening socket reuses this struct with is_listener=1. */
struct conn {
    int      is_listener;          /* 1 => this is the accept()ing socket        */
    int      fd;                   /* the socket file descriptor                 */
    enum conn_phase phase;
    uint32_t cur_events;           /* what we currently asked epoll to watch      */

    /* ---- read side: the request head accumulates here ---- */
    char     rbuf[READ_BUF];
    size_t   rlen;                 /* valid bytes in rbuf                         */
    http_request req;              /* incremental parser state                   */

    /* ---- write side: the response head (status line + headers) ---- */
    char     whead[MAX_RESP_HEAD];
    size_t   wlen;                 /* total head bytes to send                   */
    size_t   wsent;                /* head bytes already sent (partial-send safe)*/

    /* ---- body: streamed with sendfile(), no user-space copy ---- */
    int      file_fd;              /* open file, or -1 (HEAD / error responses)  */
    off_t    file_off;            /* sendfile updates this in place             */
    off_t    file_left;           /* body bytes still to send                    */

    int      close_after;          /* close the socket once this reply flushes   */
};

/* flush() outcomes. */
enum { FLUSH_DONE = 0, FLUSH_BLOCKED = 1, FLUSH_ERROR = -1 };

/* ===========================================================================
 * Small helpers
 * ===========================================================================
 */

/* Change what epoll watches for a connection, but only if it actually changed
 * (an epoll_ctl syscall per loop iteration is pure overhead otherwise). */
static int epoll_watch(int epfd, struct conn *c, uint32_t events)
{
    if (c->cur_events == events)
        return 0;
    struct epoll_event ev;
    ev.events   = events;
    ev.data.ptr = c;
    /* EPOLL_CTL_MOD (op=3): change the interest set for an fd already in the
     * epoll set. Args: epfd, op, fd, &event. Returns 0 or -1/errno. */
    if (epoll_ctl(epfd, EPOLL_CTL_MOD, c->fd, &ev) < 0)
        return -1;
    c->cur_events = events;
    return 0;
}

/* Allocate + zero a connection. Zeroing matters: http_request must start clean
 * (state ST_METHOD == 0, all offsets 0), and file_fd must start at -1. */
static struct conn *conn_new(int fd)
{
    struct conn *c = malloc(sizeof *c);
    if (!c)
        return NULL;
    memset(c, 0, sizeof *c);
    c->fd      = fd;
    c->phase   = PH_READING;
    c->file_fd = -1;
    c->req.content_length = -1;    /* -1 == "no Content-Length seen"            */
    c->req.minor_version  = -1;
    return c;
}

/* Tear a connection down: remove it from epoll, close both fds, free memory.
 * After this returns the caller MUST NOT touch `c` again (use-after-free). */
static void conn_close(int epfd, struct conn *c)
{
    /* EPOLL_CTL_DEL (op=2): closing the fd also auto-removes it from every
     * epoll set, but we delete explicitly so the intent is clear and so a
     * dup'd fd elsewhere would not linger in our set. Errors are ignored: the
     * fd may already be gone. */
    epoll_ctl(epfd, EPOLL_CTL_DEL, c->fd, NULL);
    if (c->file_fd >= 0)
        close(c->file_fd);
    close(c->fd);
    free(c);
}

/* Format the current time as an RFC 1123 HTTP-date, e.g.
 * "Sun, 06 Nov 1994 08:49:37 GMT". Uses gmtime_r (thread-safe) + strftime with
 * the C locale so the English day/month abbreviations the RFC mandates come
 * out regardless of the host's locale. */
static void http_date(char *out, size_t n)
{
    time_t now = time(NULL);
    struct tm tm;
    gmtime_r(&now, &tm);
    /* strftime returns 0 on overflow; guarantee a valid C string either way. */
    if (strftime(out, n, "%a, %d %b %Y %H:%M:%S GMT", &tm) == 0 && n > 0)
        out[0] = '\0';
}

/* Guess a Content-Type from the path's extension. A real server consults a
 * mime.types database; this covers the handful a demo actually serves. The
 * default, application/octet-stream, tells the browser "unknown bytes: download
 * rather than render," which is the safe fallback. */
static const char *mime_type(const char *path)
{
    const char *dot = NULL;
    for (const char *p = path; *p; p++)
        if (*p == '.') dot = p;      /* remember the LAST dot (real extension)  */
    if (!dot) return "application/octet-stream";
    dot++;
    if (!strcmp(dot, "html") || !strcmp(dot, "htm")) return "text/html; charset=utf-8";
    if (!strcmp(dot, "css"))  return "text/css";
    if (!strcmp(dot, "js"))   return "text/javascript";
    if (!strcmp(dot, "json")) return "application/json";
    if (!strcmp(dot, "txt"))  return "text/plain; charset=utf-8";
    if (!strcmp(dot, "png"))  return "image/png";
    if (!strcmp(dot, "jpg") || !strcmp(dot, "jpeg")) return "image/jpeg";
    if (!strcmp(dot, "gif"))  return "image/gif";
    if (!strcmp(dot, "svg"))  return "image/svg+xml";
    if (!strcmp(dot, "ico"))  return "image/x-icon";
    return "application/octet-stream";
}

/* ===========================================================================
 * Response construction — fills c->whead (and, for GET, c->file_*).
 * ===========================================================================
 */

/* Build a small self-contained error response. `extra` is optional extra header
 * lines (e.g. "Allow: GET, HEAD\r\n") or NULL. Errors always close the
 * connection: after a protocol error we can no longer trust the byte stream. */
static void build_error(struct conn *c, int code, const char *reason,
                        const char *extra)
{
    char body[256];
    int bn = snprintf(body, sizeof body,
        "<!doctype html><title>%d %s</title>"
        "<h1>%d %s</h1>\n", code, reason, code, reason);
    if (bn < 0) bn = 0;

    char date[64];
    http_date(date, sizeof date);

    int hn = snprintf(c->whead, sizeof c->whead,
        "HTTP/1.1 %d %s\r\n"
        "Server: c10k-teach\r\n"
        "Date: %s\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "%s"                       /* extra header lines, already CRLF-terminated */
        "\r\n%s",
        code, reason, date, bn, extra ? extra : "", body);
    /* If the head somehow overflowed the buffer, fall back to a minimal fixed
     * response so we never send a truncated (and thus malformed) header block. */
    if (hn < 0 || hn >= (int)sizeof c->whead) {
        static const char fallback[] =
            "HTTP/1.1 500 Internal Server Error\r\n"
            "Content-Length: 0\r\nConnection: close\r\n\r\n";
        memcpy(c->whead, fallback, sizeof fallback - 1);
        hn = (int)(sizeof fallback - 1);
    }
    c->wlen        = (size_t)hn;
    c->wsent       = 0;
    c->file_fd     = -1;
    c->file_left   = 0;
    c->close_after = 1;
}

/* Turn a parsed request into a response. On success fills the 200 head and, for
 * GET, arms sendfile; on any failure fills an error head. */
static void build_response(struct conn *c, const char *docroot)
{
    http_request *r = &c->req;

    /* --- method routing: we implement GET (with body) and HEAD (head only). */
    int is_get  = hp_slice_ci_eq(c->rbuf, r->method, "GET");
    int is_head = hp_slice_ci_eq(c->rbuf, r->method, "HEAD");
    if (!is_get && !is_head) {
        build_error(c, 405, "Method Not Allowed", "Allow: GET, HEAD\r\n");
        return;
    }

    /* --- copy the request-target out of rbuf and cut any "?query". We work on
     * a bounded local copy so nothing downstream trusts attacker-length data. */
    char tpath[2048];
    hp_size tlen = r->target.len;
    if (tlen >= sizeof tpath) {            /* absurdly long URI                 */
        build_error(c, 414, "URI Too Long", NULL);
        return;
    }
    size_t k = 0;
    for (hp_size i = 0; i < tlen; i++) {
        char ch = c->rbuf[r->target.off + i];
        if (ch == '?') break;              /* query string is not part of path  */
        tpath[k++] = ch;
    }
    tpath[k] = '\0';

    /* --- security: reject path traversal. Any ".." anywhere is refused. This
     * is deliberately blunt (it also rejects a legitimate file literally named
     * "..foo"), which is the correct trade-off for a static file server: better
     * to 403 a weird name than to serve /etc/passwd via "/../../etc/passwd".
     * We also require an absolute-style target beginning with '/'. */
    if (tpath[0] != '/') {
        build_error(c, 400, "Bad Request", NULL);
        return;
    }
    for (size_t i = 0; i + 1 < k; i++) {
        if (tpath[i] == '.' && tpath[i + 1] == '.') {
            build_error(c, 403, "Forbidden", NULL);
            return;
        }
    }

    /* --- map "/" (or any trailing-slash dir) to the index file. */
    const char *suffix = "";
    if (k == 0 || tpath[k - 1] == '/')
        suffix = "index.html";

    /* --- compose the filesystem path: docroot + target + optional index. */
    char fpath[4096];
    int flen = snprintf(fpath, sizeof fpath, "%s%s%s", docroot, tpath, suffix);
    if (flen < 0 || flen >= (int)sizeof fpath) {
        build_error(c, 414, "URI Too Long", NULL);
        return;
    }

    /* --- open the file. O_NONBLOCK so a slow filesystem cannot stall the loop;
     * O_CLOEXEC so a forked worker never inherits a stray fd (fd-leak hygiene).
     * open() returns the lowest free fd, or -1 with errno (ENOENT => 404). */
    int fd = open(fpath, O_RDONLY | O_CLOEXEC | O_NONBLOCK);
    if (fd < 0) {
        build_error(c, 404, "Not Found", NULL);
        return;
    }

    /* --- stat it: reject anything that is not a regular file (a directory, a
     * device, a fifo — none of which we can meaningfully sendfile). */
    struct stat st;
    if (fstat(fd, &st) < 0 || !S_ISREG(st.st_mode)) {
        close(fd);
        build_error(c, 404, "Not Found", NULL);
        return;
    }
    off_t size = st.st_size;

    /* --- keep-alive decision: honor the parsed intent, but if the request
     * carried a body we did not read (unusual for GET/HEAD), the byte stream is
     * now out of sync, so we must close to avoid mis-parsing the leftover body
     * as the next request. */
    int keep = r->keep_alive == 1;
    if (r->content_length > 0)
        keep = 0;

    char date[64];
    http_date(date, sizeof date);

    int hn = snprintf(c->whead, sizeof c->whead,
        "HTTP/1.1 200 OK\r\n"
        "Server: c10k-teach\r\n"
        "Date: %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %lld\r\n"
        "Connection: %s\r\n"
        "\r\n",
        date, mime_type(fpath), (long long)size, keep ? "keep-alive" : "close");
    if (hn < 0 || hn >= (int)sizeof c->whead) {
        close(fd);
        build_error(c, 500, "Internal Server Error", NULL);
        return;
    }
    c->wlen        = (size_t)hn;
    c->wsent       = 0;
    c->close_after = !keep;

    if (is_head) {
        /* HEAD: send the identical headers (including the real Content-Length)
         * but NO body. We opened the file only to stat its size; release it. */
        close(fd);
        c->file_fd   = -1;
        c->file_left = 0;
    } else {
        c->file_fd   = fd;
        c->file_off  = 0;
        c->file_left = size;
    }
}

/* ===========================================================================
 * The write path — drain to EAGAIN, then defer to EPOLLOUT if still not done.
 * ===========================================================================
 */
static int flush(int epfd, struct conn *c)
{
    /* (1) push the response head. send() may accept fewer bytes than offered
     * (the socket send buffer is finite), so we track wsent and loop. */
    while (c->wsent < c->wlen) {
        ssize_t n = send(c->fd, c->whead + c->wsent, c->wlen - c->wsent,
                         MSG_NOSIGNAL);   /* MSG_NOSIGNAL: get EPIPE, not a signal */
        if (n > 0) {
            c->wsent += (size_t)n;
            continue;
        }
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            /* Socket buffer full: stop, ask epoll to wake us when writable. */
            if (epoll_watch(epfd, c, EV_WRITE) < 0)
                return FLUSH_ERROR;
            return FLUSH_BLOCKED;
        }
        if (n < 0 && errno == EINTR)
            continue;                     /* interrupted before any byte: retry  */
        return FLUSH_ERROR;               /* EPIPE / ECONNRESET: peer gone       */
    }

    /* (2) stream the body with sendfile(out=socket, in=file, &off, count).
     * The kernel copies straight from the file's page cache into the socket —
     * the bytes never enter this process's address space (zero-copy), which is
     * the single biggest win for static file serving. sendfile updates *off in
     * place and returns the count sent, which may be short on a full socket. */
    while (c->file_left > 0) {
        ssize_t n = sendfile(c->fd, c->file_fd, &c->file_off, (size_t)c->file_left);
        if (n > 0) {
            c->file_left -= n;            /* c->file_off already advanced by kernel */
            continue;
        }
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            if (epoll_watch(epfd, c, EV_WRITE) < 0)
                return FLUSH_ERROR;
            return FLUSH_BLOCKED;
        }
        if (n < 0 && errno == EINTR)
            continue;
        return FLUSH_ERROR;
    }

    /* Fully sent. Release the file fd promptly (fds are a scarce resource at
     * C10k scale — the whole point is to not run out). */
    if (c->file_fd >= 0) {
        close(c->file_fd);
        c->file_fd = -1;
    }
    return FLUSH_DONE;
}

/* Prepare a kept-alive connection to read the NEXT request. Any bytes already
 * received past the current request head (HTTP pipelining) are shifted to the
 * front of rbuf so the parser sees them first; the parser state is zeroed. */
static void keepalive_reset(struct conn *c)
{
    size_t consumed = c->req.parsed;          /* bytes the parser used for head  */
    if (consumed > c->rlen) consumed = c->rlen;
    size_t leftover = c->rlen - consumed;
    if (leftover > 0)
        memmove(c->rbuf, c->rbuf + consumed, leftover);
    c->rlen = leftover;

    memset(&c->req, 0, sizeof c->req);        /* fresh parser: state ST_METHOD=0 */
    c->req.content_length = -1;
    c->req.minor_version  = -1;

    c->wlen = c->wsent = 0;
    c->file_fd   = -1;
    c->file_left = 0;
    c->close_after = 0;
    c->phase = PH_READING;
}

/* ===========================================================================
 * The connection engine: parse -> build -> flush, looping over any pipelined
 * requests. Entered both after a read (new bytes) and after EPOLLOUT (write
 * unblocked). On return `c` is either freed (do not touch) or parked in epoll.
 * ===========================================================================
 */
static void process(int epfd, struct conn *c, const char *docroot)
{
    for (;;) {
        if (c->phase == PH_READING) {
            int r = hp_execute(&c->req, c->rbuf, c->rlen);
            if (r == HP_OK_MORE)
                return;                    /* incomplete head: wait for EPOLLIN  */
            if (r == HP_ERR)
                build_error(c, 400, "Bad Request", NULL);
            else /* HP_OK_DONE */
                build_response(c, docroot);
            c->phase = PH_WRITING;
        }

        /* PH_WRITING: try to push the whole response now. */
        int f = flush(epfd, c);
        if (f == FLUSH_BLOCKED)
            return;                        /* EPOLLOUT will re-enter process()   */
        if (f == FLUSH_ERROR) {
            conn_close(epfd, c);           /* c is freed */
            return;
        }

        /* FLUSH_DONE: the response is fully on the wire. */
        if (c->close_after) {
            conn_close(epfd, c);           /* c is freed */
            return;
        }

        /* keep-alive: recycle for the next request. */
        keepalive_reset(c);
        if (c->rlen == 0) {
            /* Nothing pipelined. Make sure we are back to watching reads (we may
             * have switched to EV_WRITE while flushing) and park. */
            if (epoll_watch(epfd, c, EV_READ) < 0)
                conn_close(epfd, c);
            return;
        }
        /* Else: a pipelined request is already buffered — loop and serve it. */
    }
}

/* Edge-triggered READ handler: drain the socket to EAGAIN (mandatory under ET),
 * appending to rbuf, then run the engine. May free `c`. */
static void on_readable(int epfd, struct conn *c, const char *docroot)
{
    for (;;) {
        if (c->rlen == sizeof c->rbuf) {
            /* rbuf is full. Two reasons this guard exists:
             *  (1) recv(fd, ptr, 0, ...) returns 0, which our n==0 branch below
             *      would misread as "peer closed" — so we must NOT call recv
             *      with a zero-length window.
             *  (2) A single request head cannot legitimately reach here: the
             *      parser's HP_MAX_HEADER_BYTES ceiling (8 KiB) is half of
             *      READ_BUF (16 KiB), so an over-long head returns HP_ERR first.
             * CAVEAT (honest scope): if a client PIPELINES more than 16 KiB of
             * requests in one burst without reading our replies, we stop
             * reading here; because we are edge-triggered, the bytes already in
             * the socket buffer will not wake us again until *new* data arrives.
             * A production server uses a growable read buffer or applies read
             * back-pressure. Normal keep-alive (one outstanding request at a
             * time) never fills rbuf, so it is unaffected. */
            break;
        }
        ssize_t n = recv(c->fd, c->rbuf + c->rlen, sizeof c->rbuf - c->rlen, 0);
        if (n > 0) {
            c->rlen += (size_t)n;
            continue;                      /* keep draining: ET gives one wakeup */
        }
        if (n == 0) {                      /* orderly peer shutdown (FIN)        */
            conn_close(epfd, c);
            return;
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            break;                         /* socket drained — the ET contract   */
        if (errno == EINTR)
            continue;                      /* signal interrupted us: retry       */
        conn_close(epfd, c);               /* ECONNRESET or similar              */
        return;
    }
    process(epfd, c, docroot);             /* may free c */
}

/* ===========================================================================
 * Accepting connections
 * ===========================================================================
 */

/* Edge-triggered ACCEPT handler: one wakeup can mean many pending connections,
 * so we accept4() until EAGAIN — otherwise the backlog would sit unnoticed
 * until the next new SYN (the ET trap again, on the listening socket). */
static void accept_loop(int epfd, struct conn *lc)
{
    for (;;) {
        struct sockaddr_in peer;
        socklen_t plen = sizeof peer;
        /* accept4() = accept() + atomically set flags on the NEW fd:
         *   SOCK_NONBLOCK  the client fd is non-blocking from birth;
         *   SOCK_CLOEXEC   it won't leak across an exec.
         * Doing it in one syscall avoids the accept()+fcntl() race where a fork
         * in another thread could inherit a blocking, non-cloexec fd. */
        int fd = accept4(lc->fd, (struct sockaddr *)&peer, &plen,
                         SOCK_NONBLOCK | SOCK_CLOEXEC);
        if (fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;                     /* backlog drained                    */
            if (errno == EINTR)
                continue;
            if (errno == ECONNABORTED)
                continue;                  /* client vanished between SYN/accept */
            if (errno == EMFILE || errno == ENFILE) {
                /* Out of file descriptors: we cannot serve this client now.
                 * Stop accepting; established connections keep working, and the
                 * kernel holds the backlog until an fd frees up. A production
                 * server keeps a spare "idle" fd to accept-and-close so the
                 * backlog cannot wedge; we simply back off. */
                break;
            }
            break;                         /* unexpected: give up this batch     */
        }

        struct conn *c = conn_new(fd);
        if (!c) {
            close(fd);                     /* OOM: drop the connection cleanly   */
            continue;
        }
        struct epoll_event ev;
        ev.events   = EV_READ;
        ev.data.ptr = c;
        c->cur_events = EV_READ;
        /* EPOLL_CTL_ADD (op=1): start watching the new fd. */
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) < 0) {
            close(fd);
            free(c);
            continue;
        }
    }
}

/* ===========================================================================
 * Listening socket with SO_REUSEPORT
 * ===========================================================================
 */
static int make_listener(uint16_t port)
{
    /* socket(): AF_INET + SOCK_STREAM = TCP/IPv4. OR-in the flags so the
     * listening fd itself is non-blocking (accept4 gives us that anyway) and
     * close-on-exec. Returns an fd or -1/errno. */
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (fd < 0) {
        perror("socket");
        return -1;
    }

    int one = 1;
    /* SO_REUSEADDR: let us re-bind while a previous socket's connections linger
     * in TIME_WAIT, so a restart is instant instead of "Address already in use".*/
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one) < 0) {
        perror("setsockopt SO_REUSEADDR");
        close(fd);
        return -1;
    }
    /* SO_REUSEPORT: the star of multi-core scaling. Several processes may each
     * bind THIS exact ip:port; the kernel then hashes each incoming connection
     * to one of them, spreading load across workers with no shared accept lock
     * and no thundering herd. Each worker calls make_listener() for its own fd. */
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &one, sizeof one) < 0) {
        perror("setsockopt SO_REUSEPORT");
        close(fd);
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family      = AF_INET;
    /* htonl/htons convert HOST byte order to NETWORK byte order (big-endian).
     * x86-64 is little-endian, so these actually swap bytes; on a big-endian
     * host they'd be no-ops. The wire format is defined to be big-endian so
     * that every machine agrees on which byte of the port/address comes first —
     * without this, a little-endian client and big-endian server would read
     * port 8080 as 36895. INADDR_ANY (0.0.0.0) binds all local interfaces. */
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port        = htons(port);

    /* bind(): attach the socket to the local ip:port. */
    if (bind(fd, (struct sockaddr *)&addr, sizeof addr) < 0) {
        perror("bind");
        close(fd);
        return -1;
    }
    /* listen(): mark it passive and size the accept backlog. SOMAXCONN is the
     * kernel's cap on half/fully established connections queued for accept(). */
    if (listen(fd, SOMAXCONN) < 0) {
        perror("listen");
        close(fd);
        return -1;
    }
    return fd;
}

/* ===========================================================================
 * The event loop — one per worker process.
 * ===========================================================================
 */
static int worker_loop(uint16_t port, const char *docroot)
{
    /* epoll_create1(EPOLL_CLOEXEC): make an epoll instance (a kernel object
     * holding our interest set + ready list). The flag sets close-on-exec. */
    int epfd = epoll_create1(EPOLL_CLOEXEC);
    if (epfd < 0) {
        perror("epoll_create1");
        return 1;
    }

    /* Each worker owns a distinct listening socket, all bound to the same port
     * via SO_REUSEPORT. This is what lets the kernel balance accepts for us. */
    int lfd = make_listener(port);
    if (lfd < 0) {
        close(epfd);
        return 1;
    }

    /* The listener is modeled as a conn with is_listener=1 so the same
     * data.ptr dispatch works for it. It is LEVEL vs EDGE? We use EDGE here too
     * (EV_READ carries EPOLLET) and drain with accept_loop, matching the rule.*/
    struct conn listener;
    memset(&listener, 0, sizeof listener);
    listener.is_listener = 1;
    listener.fd          = lfd;

    struct epoll_event lev;
    lev.events   = EV_READ;
    lev.data.ptr = &listener;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &lev) < 0) {
        perror("epoll_ctl ADD listener");
        close(lfd);
        close(epfd);
        return 1;
    }

    struct epoll_event events[MAX_EVENTS];
    for (;;) {
        /* epoll_wait(): block until >=1 fd is ready (timeout -1 = forever) and
         * fill `events` with up to MAX_EVENTS ready fds. Returns the count. */
        int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (n < 0) {
            if (errno == EINTR)
                continue;              /* a signal (e.g. SIGCHLD) woke us: retry */
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < n; i++) {
            struct conn *c = events[i].data.ptr;
            uint32_t     e = events[i].events;

            if (c->is_listener) {
                accept_loop(epfd, c);
                continue;
            }

            /* Error/hangup first: nothing more to do on this socket. Note we
             * must decide the connection's fate exactly once per event and then
             * `continue`, because any handler below may free `c`. */
            if (e & (EPOLLERR | EPOLLHUP)) {
                conn_close(epfd, c);
                continue;
            }
            /* Because we watch reads XOR writes for a given conn, at most one of
             * the following fires, so there is no use-after-free from handling
             * both a read and a write on a just-freed connection. */
            if (c->phase == PH_READING && (e & (EPOLLIN | EPOLLRDHUP))) {
                on_readable(epfd, c, docroot);      /* may free c */
                continue;
            }
            if (c->phase == PH_WRITING && (e & (EPOLLOUT | EPOLLRDHUP))) {
                process(epfd, c, docroot);          /* may free c */
                continue;
            }
        }
    }

    close(lfd);
    close(epfd);
    return 0;
}

/* ===========================================================================
 * main — parse args, fork the worker pool, run the loop.
 * ===========================================================================
 */
int main(int argc, char **argv)
{
    uint16_t    port    = DEFAULT_PORT;
    const char *docroot = DEFAULT_ROOT;
    int         workers = 1;

    /* usage: server [port] [docroot] [workers] */
    if (argc > 1) {
        int p = atoi(argv[1]);
        if (p <= 0 || p > 65535) {
            fprintf(stderr, "bad port: %s\n", argv[1]);
            return 2;
        }
        port = (uint16_t)p;
    }
    if (argc > 2)
        docroot = argv[2];
    if (argc > 3) {
        workers = atoi(argv[3]);
        if (workers < 1) workers = 1;
        if (workers > 256) workers = 256;
    }

    /* SIGPIPE would kill us the first time we write to a socket the peer has
     * closed. We ignore it process-wide and instead handle the EPIPE errno from
     * send()/sendfile(). (send() also uses MSG_NOSIGNAL, but sendfile() has no
     * such flag, so ignoring the signal is the reliable belt-and-braces fix.) */
    signal(SIGPIPE, SIG_IGN);

    /* Reap workers that die so they don't linger as zombies. SIG_IGN on
     * SIGCHLD tells the kernel to auto-reap exited children. */
    signal(SIGCHLD, SIG_IGN);

    fprintf(stderr, "c10k-teach: http://0.0.0.0:%u  root=%s  workers=%d\n",
            (unsigned)port, docroot, workers);

    /* Fork the worker pool. Each child runs an INDEPENDENT event loop with its
     * own epoll instance and its own SO_REUSEPORT listening socket; the kernel
     * load-balances new connections across them. The parent becomes worker 0 so
     * we don't waste a core just supervising. */
    for (int w = 1; w < workers; w++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            break;                     /* run with however many we managed to spawn */
        }
        if (pid == 0)
            return worker_loop(port, docroot);   /* child: never returns normally */
        /* parent: keep forking */
    }

    return worker_loop(port, docroot); /* parent doubles as a worker */
}
