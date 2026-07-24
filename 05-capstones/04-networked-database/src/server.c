/* ===========================================================================
 * server.c — the epoll reactor: transport, timers, client protocol, wiring.
 * ===========================================================================
 *
 * server.c is the only file that touches file descriptors. It owns:
 *   - the CLIENT listener (line protocol) and PEER listener (Raft RPC),
 *   - the outgoing sockets we dial to peers, and accepted incoming sockets,
 *   - two timerfds (election timeout + leader heartbeat),
 *   - the LENGTH FRAMING + CRC of every peer message,
 *   - the pending-reply table that lets an async commit answer the right client.
 *
 * raft.c and store.c are pure logic this file drives. It hands Raft two
 * callbacks — server_send (transmit a payload to a peer) and server_reset_election
 * (re-arm the election timer) — so the algorithm never has to know about sockets.
 *
 * WHY ONE EVENT LOOP, NO THREADS
 * ------------------------------
 * Everything — clients, peers, timers — is a file descriptor, and epoll lets one
 * thread wait on all of them at once. Single-threaded means NO LOCKS: the Raft
 * state machine is only ever touched from this loop, so there are no data races
 * to reason about. That is the reactor pattern, and it is why this file has zero
 * mutexes despite being a concurrent server. We use LEVEL-triggered epoll (not
 * edge-triggered): it re-notifies while data remains, which is more forgiving and
 * keeps the teaching code short. The edge-triggered variant — and why it needs a
 * drain-to-EAGAIN loop — is the sibling lesson ../../03-networking/04-c10k-http-server.
 *
 * THE PEER FRAME (length framing — the other half of asm/demo.c's subject)
 * -----------------------------------------------------------------------
 *     +----------+--------+========================+
 *     | paylen   | crc32  |        payload         |
 *     |  u32 LE  | u32 LE |     (paylen bytes)     |
 *     +----------+--------+========================+
 * TCP is a byte stream with no message boundaries, so we prefix each Raft message
 * with its length. The reader accumulates bytes until it has the 8-byte header,
 * learns paylen, waits for that many payload bytes, verifies the CRC, and only
 * then delivers one whole message to raft_on_message. A frame is dropped or the
 * link reset on any inconsistency.
 * ===========================================================================
 */
#include "db.h"

#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>
#include <signal.h>

/* ---- tracing + fatal helper (declared in db.h) --------------------------- */

int g_trace_enabled = 0;
static struct timespec g_t0;            /* trace clock origin                    */

void trace_emit(const char *cat, const char *fmt, ...)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    double t = (now.tv_sec - g_t0.tv_sec) + (now.tv_nsec - g_t0.tv_nsec) / 1e9;
    fprintf(stderr, "[+%8.3f] %-6s ", t, cat);
    va_list ap; va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
}

void die(const char *msg) { perror(msg); _exit(1); }

/* ---- connection + server state ------------------------------------------- */

enum { CONN_CLIENT = 1, CONN_PEER_IN, CONN_PEER_OUT };

/* Per-fd connection state. rbuf accumulates inbound bytes (partial lines/frames);
 * wbuf queues outbound bytes not yet accepted by the socket. */
struct conn {
    int      fd;
    int      kind;
    int      peer_id;                    /* CONN_PEER_OUT: which peer            */
    uint8_t *rbuf; size_t rlen, rcap;    /* read accumulator                     */
    uint8_t *wbuf; size_t wlen, wsent, wcap; /* write queue                      */
};

#define FD_MAX 4096                      /* connection table size (fds are small) */

/* A client write awaiting its Raft entry to commit+apply. */
struct pending { int fd; uint64_t index; uint64_t term; };

struct server {
    struct raft   *r;
    int            epfd;
    int            lc_fd;                /* client listener                       */
    int            lp_fd;                /* peer listener                         */
    int            et_fd;               /* election timerfd (one-shot, re-armed) */
    int            hb_fd;               /* heartbeat timerfd (periodic)          */
    struct conn   *conns[FD_MAX];
    struct pending *pend; size_t npend, pend_cap;
};

/* ---- tiny fd/epoll helpers ------------------------------------------------ */

static int set_nonblock(int fd)
{
    int fl = fcntl(fd, F_GETFL, 0);
    return (fl < 0) ? -1 : fcntl(fd, F_SETFL, fl | O_NONBLOCK);
}

static int ep_add(int ep, int fd, uint32_t ev)
{
    struct epoll_event e = { .events = ev, .data = { .fd = fd } };
    return epoll_ctl(ep, EPOLL_CTL_ADD, fd, &e);
}
static int ep_mod(int ep, int fd, uint32_t ev)
{
    struct epoll_event e = { .events = ev, .data = { .fd = fd } };
    return epoll_ctl(ep, EPOLL_CTL_MOD, fd, &e);
}
static void ep_del(int ep, int fd) { epoll_ctl(ep, EPOLL_CTL_DEL, fd, NULL); }

/* ---- connection table ----------------------------------------------------- */

static struct conn *conn_new(struct server *s, int fd, int kind, int peer_id)
{
    if (fd < 0 || fd >= FD_MAX) { close(fd); return NULL; } /* out of table range */
    struct conn *c = calloc(1, sizeof *c);
    if (!c) { close(fd); return NULL; }
    c->fd = fd; c->kind = kind; c->peer_id = peer_id;
    s->conns[fd] = c;
    return c;
}

static void conn_free(struct server *s, int fd)
{
    struct conn *c = s->conns[fd];
    if (!c) return;
    ep_del(s->epfd, fd);
    close(fd);
    free(c->rbuf); free(c->wbuf); free(c);
    s->conns[fd] = NULL;
    /* Drop any pending client replies bound to this now-dead fd. */
    for (size_t i = 0; i < s->npend; ) {
        if (s->pend[i].fd == fd) s->pend[i] = s->pend[--s->npend];
        else i++;
    }
}

/* Grow c->rbuf so it can hold at least `need` bytes; cap enforced by caller. */
static int rbuf_reserve(struct conn *c, size_t need)
{
    if (need <= c->rcap) return 0;
    size_t cap = c->rcap ? c->rcap : 256;
    while (cap < need) cap *= 2;
    uint8_t *n = realloc(c->rbuf, cap);
    if (!n) return -1;
    c->rbuf = n; c->rcap = cap;
    return 0;
}

/* ---- outbound writes (clients) ------------------------------------------- */

/* Queue `data` on the connection and try to flush. On a would-block, the leftover
 * stays in wbuf and we ask epoll to tell us when the socket is writable again. */
static void conn_send(struct server *s, struct conn *c, const void *data, size_t len)
{
    if (c->wlen + len > c->wcap) {
        size_t cap = c->wcap ? c->wcap : 256;
        while (cap < c->wlen + len) cap *= 2;
        uint8_t *n = realloc(c->wbuf, cap);
        if (!n) { conn_free(s, c->fd); return; }        /* OOM ⇒ drop the client  */
        c->wbuf = n; c->wcap = cap;
    }
    memcpy(c->wbuf + c->wlen, data, len);
    c->wlen += len;

    while (c->wsent < c->wlen) {
        ssize_t w = write(c->fd, c->wbuf + c->wsent, c->wlen - c->wsent);
        if (w < 0) {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                ep_mod(s->epfd, c->fd, EPOLLIN | EPOLLOUT); /* resume on writable  */
                return;
            }
            conn_free(s, c->fd);                        /* hard error ⇒ close     */
            return;
        }
        c->wsent += (size_t)w;
    }
    c->wsent = c->wlen = 0;                              /* fully flushed          */
    ep_mod(s->epfd, c->fd, EPOLLIN);                     /* no more EPOLLOUT needed */
}

static void reply_str(struct server *s, struct conn *c, const char *str)
{
    conn_send(s, c, str, strlen(str));
}

/* ---- peer transport ------------------------------------------------------- */

static struct raft_peer *peer_by_id(struct raft *r, int id)
{
    for (int i = 0; i < r->npeers; i++)
        if (r->peers[i].id == id) return &r->peers[i];
    return NULL;
}

/* Tear a peer's outgoing link down (on error or partial write) so it re-dials
 * cleanly next maintenance tick; a half-written frame must never linger. */
static void peer_drop_out(struct server *s, struct raft_peer *p)
{
    if (p->out_fd >= 0) { conn_free(s, p->out_fd); p->out_fd = -1; }
    p->out_connecting = false;
}

/* server_send: Raft's transmit callback. Frame the payload and write it whole.
 * Returns 0 if handed to the kernel, -1 if dropped (blocked/link down/would
 * block). A dropped RPC is a normal, expected event in Raft — heartbeats retry. */
static int server_send(void *net, int peer_id, const uint8_t *payload, size_t len)
{
    struct server *s = net;
    struct raft_peer *p = peer_by_id(s->r, peer_id);
    if (!p) return -1;
    if (p->blocked) return -1;                          /* injected partition     */
    if (p->out_fd < 0 || p->out_connecting) return -1;  /* not connected yet      */
    if (len > DB_PEER_FRAME_MAX) return -1;

    /* Build [paylen][crc][payload] in one buffer so it leaves as one write. */
    static uint8_t frame[8 + DB_PEER_FRAME_MAX];
    put_u32(frame, (uint32_t)len);
    put_u32(frame + 4, crc32_ieee(payload, len));
    memcpy(frame + 8, payload, len);
    size_t total = 8 + len, off = 0;

    while (off < total) {
        ssize_t w = write(p->out_fd, frame + off, total - off);
        if (w < 0) {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                if (off == 0) return -1;                /* nothing sent ⇒ safe drop */
                peer_drop_out(s, p);                    /* torn frame ⇒ reset link  */
                return -1;
            }
            peer_drop_out(s, p);
            return -1;
        }
        off += (size_t)w;
    }
    return 0;
}

/* Parse whole frames out of an incoming peer connection and deliver each to Raft.
 * Handles the partial-read reality of TCP: leftover bytes stay in rbuf. */
static void peer_in_process(struct server *s, struct conn *c)
{
    size_t off = 0;
    while (c->rlen - off >= 8) {                         /* have a full header?    */
        uint32_t paylen = get_u32(c->rbuf + off);
        uint32_t crc    = get_u32(c->rbuf + off + 4);
        if (paylen > DB_PEER_FRAME_MAX) {                /* insane length ⇒ reset  */
            conn_free(s, c->fd);
            return;
        }
        if (c->rlen - off < 8 + paylen) break;           /* payload not all here   */
        const uint8_t *pl = c->rbuf + off + 8;
        if (crc32_ieee(pl, paylen) == crc && paylen >= 13) {
            /* The sender id lives at payload offset 9 ([type][term][from]); honor
             * an injected partition by dropping frames from a blocked peer too. */
            int from = (int)get_u32(pl + 9);
            struct raft_peer *fp = peer_by_id(s->r, from);
            if (!(fp && fp->blocked))
                raft_on_message(s->r, from, pl, paylen);
        }
        /* else: corrupt frame — skip it (length framing keeps us in sync). */
        off += 8 + paylen;
    }
    /* Shift the unconsumed tail to the front so the buffer doesn't grow forever. */
    if (off) { memmove(c->rbuf, c->rbuf + off, c->rlen - off); c->rlen -= off; }
}

/* ---- client line protocol ------------------------------------------------- */

/* Redirect a client to the current leader (Redis-cluster style -MOVED), or say
 * we don't know one yet. Reads and writes both funnel to the leader here. */
static void reply_redirect(struct server *s, struct conn *c)
{
    struct raft *r = s->r;
    struct raft_peer *lp = (r->leader_id >= 0) ? peer_by_id(r, r->leader_id) : NULL;
    if (lp) {
        char buf[128];
        snprintf(buf, sizeof buf, "-MOVED %s:%d\r\n", lp->host, lp->client_port);
        reply_str(s, c, buf);
    } else {
        reply_str(s, c, "-ERR no leader elected yet\r\n");
    }
}

/* Case-insensitive match of a NUL/space-delimited token against a keyword. */
static int tok_is(const char *tok, size_t n, const char *kw)
{
    for (size_t i = 0; i < n; i++) {
        char a = tok[i], b = kw[i];
        if (a >= 'a' && a <= 'z') a -= 32;
        if (b >= 'a' && b <= 'z') b -= 32;
        if (a != b || b == 0) return 0;
    }
    return kw[n] == 0;
}

static void client_line(struct server *s, struct conn *c, char *line, size_t n);

/* Split complete '\n'-terminated lines out of the client's read buffer. */
static void client_process(struct server *s, struct conn *c)
{
    for (;;) {
        uint8_t *nl = memchr(c->rbuf, '\n', c->rlen);
        if (!nl) {
            if (c->rlen > DB_CLIENT_BUF) {               /* a line too long to be sane */
                reply_str(s, c, "-ERR line too long\r\n");
                conn_free(s, c->fd);
            }
            return;                                      /* wait for the rest      */
        }
        size_t linelen = (size_t)(nl - c->rbuf);
        char line[DB_CLIENT_BUF + 1];
        size_t copy = linelen < DB_CLIENT_BUF ? linelen : DB_CLIENT_BUF;
        memcpy(line, c->rbuf, copy);
        if (copy && line[copy - 1] == '\r') copy--;      /* tolerate CRLF          */
        line[copy] = '\0';
        /* Consume the line (and its newline) from the buffer. */
        size_t consumed = linelen + 1;
        memmove(c->rbuf, c->rbuf + consumed, c->rlen - consumed);
        c->rlen -= consumed;

        client_line(s, c, line, copy);
        if (!s->conns[c->fd]) return;                    /* handler closed us      */
    }
}

/* Record a write awaiting commit; flush_pending answers when it applies. */
static void pending_add(struct server *s, int fd, uint64_t index, uint64_t term)
{
    if (s->npend == s->pend_cap) {
        size_t cap = s->pend_cap ? s->pend_cap * 2 : 16;
        struct pending *n = realloc(s->pend, cap * sizeof *n);
        if (!n) return;                                  /* client just times out  */
        s->pend = n; s->pend_cap = cap;
    }
    s->pend[s->npend++] = (struct pending){ fd, index, term };
}

/* Handle one parsed client line. */
static void client_line(struct server *s, struct conn *c, char *line, size_t n)
{
    struct raft *r = s->r;
    if (n == 0) return;                                  /* blank line ⇒ ignore    */

    /* First token = command. */
    char *cmd = line; size_t ci = 0;
    while (ci < n && line[ci] != ' ') ci++;
    size_t cmdlen = ci;
    while (ci < n && line[ci] == ' ') ci++;              /* skip spaces            */
    char *arg = line + ci;                               /* remainder (key [val])  */
    size_t arglen = n - ci;

    if (tok_is(cmd, cmdlen, "PING")) { reply_str(s, c, "+PONG\r\n"); return; }

    if (tok_is(cmd, cmdlen, "STATUS")) {
        const char *role = r->role == ROLE_LEADER ? "leader"
                         : r->role == ROLE_CANDIDATE ? "candidate" : "follower";
        char buf[192];
        snprintf(buf, sizeof buf,
                 "+role=%s term=%llu leader=%d commit=%llu applied=%llu log=%llu\r\n",
                 role, (unsigned long long)r->current_term, r->leader_id,
                 (unsigned long long)r->commit_index,
                 (unsigned long long)r->last_applied,
                 (unsigned long long)r->log_len);
        reply_str(s, c, buf);
        return;
    }

    /* ADMIN partition <peerid> on|off   — inject/heal a simulated partition.
     * ADMIN leader                       — print who we think the leader is.    */
    if (tok_is(cmd, cmdlen, "ADMIN")) {
        char *sub = arg; size_t si = 0;
        while (si < arglen && arg[si] != ' ') si++;
        size_t sublen = si;
        while (si < arglen && arg[si] == ' ') si++;
        char *rest = arg + si; size_t restlen = arglen - si;
        if (tok_is(sub, sublen, "partition")) {
            /* rest = "<peerid> on|off" */
            char *idtok = rest; size_t ii = 0;
            while (ii < restlen && rest[ii] != ' ') ii++;
            int pid = atoi(idtok);
            while (ii < restlen && rest[ii] == ' ') ii++;
            char *mode = rest + ii; size_t modelen = restlen - ii;
            struct raft_peer *p = peer_by_id(r, pid);
            if (!p) { reply_str(s, c, "-ERR no such peer\r\n"); return; }
            p->blocked = tok_is(mode, modelen, "on");
            TR("admin", "node=%d partition peer=%d %s", r->id, pid,
               p->blocked ? "ON" : "OFF");
            reply_str(s, c, "+OK\r\n");
            return;
        }
        reply_redirect(s, c);                            /* ADMIN leader / other   */
        return;
    }

    if (tok_is(cmd, cmdlen, "GET")) {
        if (arglen == 0 || arglen > DB_MAX_KEY) { reply_str(s, c, "-ERR usage: GET key\r\n"); return; }
        /* Reads are served by the leader from applied state (documented: not
         * strictly linearizable without a read-index/lease). Followers redirect. */
        if (r->role != ROLE_LEADER) { reply_redirect(s, c); return; }
        const char *val; uint32_t vlen;
        if (store_get(r->store, arg, (uint32_t)arglen, &val, &vlen)) {
            char hdr[32];
            int h = snprintf(hdr, sizeof hdr, "$%u\r\n", vlen);
            conn_send(s, c, hdr, (size_t)h);
            conn_send(s, c, val, vlen);
            conn_send(s, c, "\r\n", 2);
        } else {
            reply_str(s, c, "$-1\r\n");                  /* nil                    */
        }
        return;
    }

    if (tok_is(cmd, cmdlen, "PUT") || tok_is(cmd, cmdlen, "DEL")) {
        /* PUT: arg = "key value"; DEL: arg = "key". */
        char *key = arg; size_t ki = 0;
        while (ki < arglen && arg[ki] != ' ') ki++;
        size_t klen = ki;
        while (ki < arglen && arg[ki] == ' ') ki++;
        char *val = arg + ki; size_t vlen = arglen - ki;

        bool is_put = tok_is(cmd, cmdlen, "PUT");
        if (klen == 0 || klen > DB_MAX_KEY) { reply_str(s, c, "-ERR bad key\r\n"); return; }
        if (is_put && (vlen == 0 || vlen > DB_MAX_VAL)) { reply_str(s, c, "-ERR bad value\r\n"); return; }

        struct command cmd_rec = {
            .op = is_put ? OP_PUT : OP_DEL,
            .key = key, .klen = (uint32_t)klen,
            .val = is_put ? val : NULL, .vlen = is_put ? (uint32_t)vlen : 0
        };
        int64_t idx = raft_client_propose(r, &cmd_rec);
        if (idx == 0)  { reply_redirect(s, c); return; } /* not the leader         */
        if (idx < 0)   { reply_str(s, c, "-ERR log write failed\r\n"); return; }
        /* Do NOT reply yet: wait until this index commits and applies. */
        pending_add(s, c->fd, (uint64_t)idx, r->current_term);
        return;
    }

    reply_str(s, c, "-ERR unknown command\r\n");
}

/* After any Raft progress, answer clients whose writes have now resolved. */
static void flush_pending(struct server *s)
{
    struct raft *r = s->r;
    for (size_t i = 0; i < s->npend; ) {
        struct pending *p = &s->pend[i];
        struct conn *c = (p->fd >= 0 && p->fd < FD_MAX) ? s->conns[p->fd] : NULL;
        if (!c) { *p = s->pend[--s->npend]; continue; }  /* client vanished        */

        bool applied    = p->index <= r->last_applied;
        bool still_ours  = p->index <= r->log_len && r->log[p->index].term == p->term;

        if (applied && still_ours) {                     /* committed + applied    */
            reply_str(s, c, "+OK\r\n");
            *p = s->pend[--s->npend];
        } else if (!still_ours) {                         /* our entry was overwritten */
            reply_str(s, c, "-ERR leadership lost; retry\r\n");
            *p = s->pend[--s->npend];
        } else if (r->role != ROLE_LEADER) {             /* can't make progress    */
            reply_redirect(s, c);
            *p = s->pend[--s->npend];
        } else {
            i++;                                         /* still replicating      */
        }
    }
}

/* ---- listeners + dialing -------------------------------------------------- */

static int make_listener(int port)
{
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (fd < 0) return -1;
    int one = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one); /* fast restart   */
    struct sockaddr_in a = { 0 };
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_ANY);              /* bind all interfaces     */
    a.sin_port = htons((uint16_t)port);                 /* network byte order      */
    if (bind(fd, (struct sockaddr *)&a, sizeof a) < 0) { close(fd); return -1; }
    if (listen(fd, 128) < 0) { close(fd); return -1; }  /* backlog 128            */
    return fd;
}

/* Non-blocking dial of a peer's Raft port. connect() returns EINPROGRESS; we
 * finish the handshake when epoll reports the socket writable. */
static void peer_connect(struct server *s, struct raft_peer *p)
{
    if (p->out_fd >= 0 || p->out_connecting) return;     /* already up/connecting  */
    char portstr[16];
    snprintf(portstr, sizeof portstr, "%d", p->peer_port);
    struct addrinfo hints = { 0 }, *res = NULL;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(p->host, portstr, &hints, &res) != 0 || !res) return;

    int fd = socket(res->ai_family, res->ai_socktype | SOCK_NONBLOCK | SOCK_CLOEXEC,
                    res->ai_protocol);
    if (fd < 0) { freeaddrinfo(res); return; }

    int rc = connect(fd, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);
    if (rc == 0) {
        /* Rare: connected immediately (peer on localhost). */
        if (!conn_new(s, fd, CONN_PEER_OUT, p->id)) return;
        p->out_fd = fd; p->out_connecting = false;
        ep_add(s->epfd, fd, EPOLLRDHUP);                 /* watch only for death   */
    } else if (errno == EINPROGRESS) {
        if (!conn_new(s, fd, CONN_PEER_OUT, p->id)) return;
        p->out_fd = fd; p->out_connecting = true;
        ep_add(s->epfd, fd, EPOLLOUT);                   /* completion ⇒ writable  */
    } else {
        close(fd);                                       /* dial failed; retry later */
    }
}

/* Finish a non-blocking connect once epoll says the out socket is writable. */
static void peer_connect_done(struct server *s, struct conn *c)
{
    struct raft_peer *p = peer_by_id(s->r, c->peer_id);
    if (!p) { conn_free(s, c->fd); return; }
    int err = 0; socklen_t el = sizeof err;
    getsockopt(c->fd, SOL_SOCKET, SO_ERROR, &err, &el);  /* real connect result    */
    if (err != 0) { peer_drop_out(s, p); return; }       /* failed ⇒ drop, re-dial */
    p->out_connecting = false;
    ep_mod(s->epfd, c->fd, EPOLLRDHUP);                  /* stop watching writable */
    TR("net", "node=%d connected to peer %d", s->r->id, p->id);
}

/* Re-dial any peer whose outgoing link is down. Called each timer tick, so a
 * partitioned/crashed peer is retried roughly every heartbeat. */
static void maintain_links(struct server *s)
{
    for (int i = 0; i < s->r->npeers; i++) {
        struct raft_peer *p = &s->r->peers[i];
        if (p->out_fd < 0 && !p->blocked) peer_connect(s, p);
    }
}

/* ---- timers --------------------------------------------------------------- */

/* Arm the election timerfd with a fresh RANDOM timeout in [MIN,MAX). Randomizing
 * per-arm is what makes split votes rare: two candidates almost never time out
 * at the same instant, so one gets ahead and wins (Raft §5.2). */
static void arm_election(struct server *s)
{
    long span = DB_ELECTION_MAX_MS - DB_ELECTION_MIN_MS;
    long ms = DB_ELECTION_MIN_MS + (random() % span);
    struct itimerspec it = { 0 };
    it.it_value.tv_sec  = ms / 1000;
    it.it_value.tv_nsec = (ms % 1000) * 1000000L;        /* one-shot; re-armed     */
    timerfd_settime(s->et_fd, 0, &it, NULL);
}

/* Raft's reset_election callback: push the election deadline into the future
 * because we just saw legitimate leader/candidate activity. */
static void server_reset_election(void *net)
{
    arm_election((struct server *)net);
}

/* ---- accept loops --------------------------------------------------------- */

static void accept_clients(struct server *s)
{
    for (;;) {
        int fd = accept4(s->lc_fd, NULL, NULL, SOCK_NONBLOCK | SOCK_CLOEXEC);
        if (fd < 0) break;                               /* EAGAIN ⇒ drained       */
        if (!conn_new(s, fd, CONN_CLIENT, -1)) continue;
        ep_add(s->epfd, fd, EPOLLIN);
    }
}

static void accept_peers(struct server *s)
{
    for (;;) {
        int fd = accept4(s->lp_fd, NULL, NULL, SOCK_NONBLOCK | SOCK_CLOEXEC);
        if (fd < 0) break;
        if (!conn_new(s, fd, CONN_PEER_IN, -1)) continue;
        ep_add(s->epfd, fd, EPOLLIN | EPOLLRDHUP);
    }
}

/* Drain a readable connection into its rbuf. Returns 0 on live, -1 if closed. */
static int conn_fill(struct server *s, struct conn *c, size_t cap)
{
    for (;;) {
        if (rbuf_reserve(c, c->rlen + 4096) < 0) return -1;
        if (c->rlen > cap) return -1;                    /* runaway buffer ⇒ reset */
        ssize_t r = read(c->fd, c->rbuf + c->rlen, c->rcap - c->rlen);
        if (r > 0) { c->rlen += (size_t)r; continue; }
        if (r == 0) return -1;                           /* peer closed            */
        if (errno == EINTR) continue;
        if (errno == EAGAIN || errno == EWOULDBLOCK) return 0; /* drained          */
        return -1;                                       /* hard error             */
    }
}

/* ---- the event loop ------------------------------------------------------- */

int server_run(struct raft *r, int client_port, int peer_port)
{
    clock_gettime(CLOCK_MONOTONIC, &g_t0);
    /* Seed the PRNG for election-timeout jitter with something node-distinct so
     * two nodes started in the same second still pick different timeouts. */
    srandom((unsigned)(time(NULL) ^ (getpid() << 16) ^ (r->id << 1)));

    struct server S = { 0 };
    S.r = r;
    /* Install the transport callbacks now that we (server.c) exist: Raft calls
     * these to send RPCs and re-arm its election timer, all against &S. */
    r->send = server_send;
    r->reset_election = server_reset_election;
    r->net = &S;

    /* Writing to a peer that reset its connection would raise SIGPIPE and kill
     * the process; ignore it and handle EPIPE from write() instead. */
    signal(SIGPIPE, SIG_IGN);

    S.epfd = epoll_create1(EPOLL_CLOEXEC);
    if (S.epfd < 0) die("epoll_create1");

    S.lc_fd = make_listener(client_port);
    if (S.lc_fd < 0) die("client listener");
    S.lp_fd = make_listener(peer_port);
    if (S.lp_fd < 0) die("peer listener");

    /* CLOCK_MONOTONIC timers: immune to wall-clock jumps (NTP/settimeofday). */
    S.et_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    S.hb_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (S.et_fd < 0 || S.hb_fd < 0) die("timerfd_create");

    ep_add(S.epfd, S.lc_fd, EPOLLIN);
    ep_add(S.epfd, S.lp_fd, EPOLLIN);
    ep_add(S.epfd, S.et_fd, EPOLLIN);
    ep_add(S.epfd, S.hb_fd, EPOLLIN);

    /* Periodic heartbeat timer (fires every DB_HEARTBEAT_MS forever). */
    struct itimerspec hb = { 0 };
    hb.it_value.tv_nsec = hb.it_interval.tv_nsec = 0;
    hb.it_value.tv_sec = 0;
    hb.it_value.tv_nsec = DB_HEARTBEAT_MS * 1000000L;
    hb.it_interval.tv_nsec = DB_HEARTBEAT_MS * 1000000L;
    timerfd_settime(S.hb_fd, 0, &hb, NULL);

    arm_election(&S);                                    /* start the election clock */
    maintain_links(&S);                                 /* first dial attempt      */

    TR("boot", "node=%d client=:%d peer=:%d peers=%d",
       r->id, client_port, peer_port, r->npeers);

    struct epoll_event evs[64];
    for (;;) {
        int nfd = epoll_wait(S.epfd, evs, 64, -1);       /* block until something  */
        if (nfd < 0) { if (errno == EINTR) continue; die("epoll_wait"); }

        for (int i = 0; i < nfd; i++) {
            int fd = evs[i].data.fd;
            uint32_t ev = evs[i].events;

            if (fd == S.lc_fd) { accept_clients(&S); continue; }
            if (fd == S.lp_fd) { accept_peers(&S); continue; }

            if (fd == S.et_fd) {                          /* election timeout       */
                uint64_t x; (void)!read(S.et_fd, &x, 8);
                arm_election(&S);                         /* schedule the next one  */
                raft_on_election_timeout(r);
                flush_pending(&S);
                continue;
            }
            if (fd == S.hb_fd) {                          /* heartbeat tick         */
                uint64_t x; (void)!read(S.hb_fd, &x, 8);
                maintain_links(&S);                       /* re-dial dead peers     */
                raft_on_heartbeat(r);
                flush_pending(&S);
                continue;
            }

            struct conn *c = S.conns[fd];
            if (!c) continue;                             /* stale event            */

            /* Peer link death (either half-closed or errored). */
            if (ev & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
                if (c->kind == CONN_PEER_OUT) {
                    struct raft_peer *p = peer_by_id(r, c->peer_id);
                    if (p) peer_drop_out(&S, p); else conn_free(&S, fd);
                } else {
                    conn_free(&S, fd);
                }
                continue;
            }

            if (c->kind == CONN_PEER_OUT) {               /* connect completion     */
                if (ev & EPOLLOUT) peer_connect_done(&S, c);
                continue;
            }

            if (ev & EPOLLOUT) {                          /* client writable again  */
                conn_send(&S, c, "", 0);                  /* re-drive the flush     */
                if (!S.conns[fd]) continue;
            }

            if (ev & EPOLLIN) {
                size_t cap = (c->kind == CONN_CLIENT) ? (DB_CLIENT_BUF * 4)
                                                      : (DB_PEER_FRAME_MAX + 16);
                if (conn_fill(&S, c, cap) < 0) { conn_free(&S, fd); continue; }
                if (c->kind == CONN_CLIENT) client_process(&S, c);
                else                        peer_in_process(&S, c);
                if (S.conns[fd]) flush_pending(&S);       /* writes may have applied */
            }
        }
    }
    /* Not reached in normal operation (loop runs until the process is killed). */
}
