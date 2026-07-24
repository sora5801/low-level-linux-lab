/* ===========================================================================
 * lb.h — shared types and prototypes for the reverse-proxy / L4 load balancer.
 * ===========================================================================
 *
 * This header defines the whole data model in one place so the three .c files
 * agree on it:
 *   - hashring.c  builds and queries the consistent-hash ring.
 *   - lb.c        runs the epoll event loop, splice pumps, health checks, the
 *                 warm connection pool, the control socket, and graceful drain.
 *   - (asm/demo.c is a standalone extraction of the selection math for teaching
 *     assembly; it deliberately does NOT include this header.)
 *
 * The proxy is single-threaded and event-driven: ONE epoll instance multiplexes
 * every file descriptor (the listener, every client<->backend splice, the health
 * timer, the signal fd, the admin control socket). There is no locking because
 * there is no second thread — the "concurrency" here is I/O concurrency, not
 * thread concurrency, which is exactly what epoll buys you.
 * ===========================================================================
 */
#ifndef LB_H
#define LB_H

#include <stdint.h>          /* uint32_t                                      */
#include <stdbool.h>         /* bool                                          */
#include <stddef.h>          /* size_t                                        */
#include <sys/socket.h>      /* struct sockaddr_storage, socklen_t            */

/* ---- compile-time limits (kept small and static so there is no per-config
 * allocation to leak; a production LB would size these dynamically) --------- */
#define MAX_BACKENDS   64        /* upper bound on -b backends                 */
#define RING_VNODES    160       /* virtual nodes per backend on the hash ring */
#define POOL_MAX       64        /* warm idle connections kept per backend     */
#define PIPE_LEN       (1 << 16) /* 64 KiB: the default kernel pipe capacity,  */
                                 /*   and the most a single splice() can move  */
                                 /*   between a socket and a pipe here.        */
#define MAX_EVENTS     256       /* epoll_wait() batch size                    */
#define NAME_MAX_LEN   128       /* "host:port" label length                   */

/* Health-check hysteresis: how many consecutive probe results flip a backend.
 * Hysteresis prevents flapping — one unlucky timeout should not eject a backend,
 * and one lucky success should not re-admit a still-shaky one. */
#define HEALTH_RISE    2         /* consecutive OK probes to bring a DOWN be UP */
#define HEALTH_FALL    3         /* consecutive failed probes to eject an UP be */

/* Backend-selection policy chosen at startup with -p. */
enum policy {
    POL_RR,      /* round-robin: hand out backends in rotation                 */
    POL_LC,      /* least-connections: fewest in-flight conns wins             */
    POL_HASH     /* consistent hash on the client IP (sticky routing)          */
};

/* Liveness state of a backend, driven by the health checker. */
enum be_state {
    BE_UP,       /* passing health checks; eligible for new connections        */
    BE_DOWN      /* failing health checks; ejected until it recovers           */
};

/* Which side of a proxied connection an epoll event refers to. */
enum side {
    SIDE_CLIENT  = 0,
    SIDE_BACKEND = 1
};

/* The kind of thing an epoll event's data.ptr points at. Every fd we register
 * carries a `struct io_handle*` in data.ptr so the dispatcher can recover what
 * to do without a fd->object table. */
enum ev_kind {
    EV_LISTEN,   /* the accept() listener socket                              */
    EV_CONN,     /* a client or backend fd belonging to a struct conn          */
    EV_HEALTH,   /* an in-flight health-probe socket for a backend             */
    EV_POOL,     /* an in-flight warm-pool connect() for a backend             */
    EV_TIMER,    /* the timerfd that drives periodic health checks             */
    EV_SIGNAL,   /* the signalfd delivering SIGINT/SIGTERM                     */
    EV_CONTROL   /* the AF_UNIX admin control-socket listener                  */
};

/* A tag stored in epoll_event.data.ptr. `obj` is a struct conn* (EV_CONN) or a
 * struct backend* (EV_HEALTH/EV_POOL) or NULL (singletons); `side` disambiguates
 * the two fds of a conn. These live INSIDE the object they describe (conn or
 * backend), whose addresses are stable, so the pointer epoll holds stays valid. */
struct io_handle {
    enum ev_kind kind;
    void        *obj;
    enum side    side;
};

/* ---------------------------------------------------------------------------
 * struct pump — one unidirectional byte pipe (client->backend OR backend->client).
 *
 * Zero-copy splice() moves bytes between an fd and a PIPE without ever mapping
 * them into user space. To move socket->socket we therefore stage through a pipe:
 *     splice(src_socket -> pipe_wr)   then   splice(pipe_rd -> dst_socket)
 * The bytes live only in kernel pipe buffers (struct pipe_buffer page refs) the
 * whole time; the CPU never memcpy's them and they never enter this process's
 * address space. `inpipe` tracks how many bytes are currently parked in the pipe
 * so we know when to stop reading src (pipe full -> backpressure) and when the
 * pipe has drained enough to propagate EOF.
 * --------------------------------------------------------------------------- */
struct pump {
    int    src_fd;      /* read from here (a socket)                           */
    int    dst_fd;      /* write to here (a socket)                            */
    int    pipe_rd;     /* read end of the staging pipe                        */
    int    pipe_wr;     /* write end of the staging pipe                       */
    size_t inpipe;      /* bytes currently buffered in the pipe (0..PIPE_LEN)  */
    bool   src_closed;  /* src reached EOF (or errored) — no more bytes coming */
    bool   shut_done;   /* we already did shutdown(dst, SHUT_WR) to relay EOF  */
    bool   broken;      /* hard error on this direction — tear the conn down   */
};

/* ---------------------------------------------------------------------------
 * struct conn — one proxied client<->backend session.
 *
 * A raw L4 (TCP) proxy maps each accepted client to exactly one backend socket
 * for the life of the connection and splices bytes both ways. The two pumps are
 * independent half-duplex flows, so we can half-close one direction (client sent
 * FIN) while the other keeps flowing (backend still streaming a response).
 * --------------------------------------------------------------------------- */
struct conn {
    int  client_fd;
    int  backend_fd;
    int  backend_idx;           /* index into ctx->be[]; for stats & drain     */
    bool connecting;            /* true until the backend connect() completes  */

    struct io_handle h_client;  /* epoll tag for client_fd  (side=CLIENT)      */
    struct io_handle h_backend; /* epoll tag for backend_fd (side=BACKEND)     */

    struct pump c2b;            /* client -> backend                           */
    struct pump b2c;            /* backend -> client                           */

    /* A conn is freed LAZILY. epoll_wait() can hand us a batch containing an
     * event for BOTH this conn's fds; if the first event tears the conn down and
     * frees it, the second event would dereference freed memory. So teardown sets
     * `closed`, unlinks from the live list, closes the fds, and parks the conn on
     * ctx->dead; the actual free happens after the whole batch is processed. Any
     * later event in the same batch sees `closed` and bails. */
    bool         closed;
    struct conn *dead_next;     /* single-linked chain of conns awaiting free   */

    struct conn *prev, *next;   /* intrusive list of all live conns (for drain)*/
};

/* ---------------------------------------------------------------------------
 * struct backend — one upstream server.
 * --------------------------------------------------------------------------- */
struct backend {
    char                    name[NAME_MAX_LEN]; /* "host:port": hash label + log id */
    struct sockaddr_storage addr;               /* resolved upstream address        */
    socklen_t               addrlen;

    enum be_state           state;      /* UP / DOWN (health checker owns this) */
    bool                    draining;   /* admin drain: no NEW conns, existing  */
                                        /*   ones finish naturally              */
    int                     active;     /* live conns using this backend        */

    /* --- health probe state (one probe in flight at a time) --- */
    int                     probe_fd;   /* in-flight probe socket, or -1        */
    int                     ok_count;   /* consecutive successful probes        */
    int                     fail_count; /* consecutive failed probes            */
    struct io_handle        probe_h;    /* epoll tag for probe_fd               */

    /* --- warm connection pool (pre-established idle upstream sockets) --- */
    int                     idle[POOL_MAX]; /* ready-to-use connected fds       */
    int                     idle_n;         /* how many are stacked in idle[]   */
    int                     pool_fd;        /* one in-flight pool connect, or -1 */
    struct io_handle        pool_h;         /* epoll tag for pool_fd            */
};

/* One consistent-hash ring virtual node (kept sorted by hash in ring.nodes). */
struct rnode {
    uint32_t hash;      /* position on the 2^32 ring                           */
    int      idx;       /* backend index this vnode routes to                  */
};

struct ring {
    struct rnode *nodes;    /* sorted-by-hash array of virtual nodes           */
    int           n;        /* number of live vnodes                           */
    int           cap;      /* allocated capacity of nodes[]                    */
};

/* ---------------------------------------------------------------------------
 * struct lb_ctx — the whole server state, threaded through by pointer so we
 * avoid mutable globals (per repo convention). Singletons (listener, timerfd,
 * signalfd, control socket) each carry their own io_handle so epoll can tag them.
 * --------------------------------------------------------------------------- */
struct lb_ctx {
    int              epfd;              /* the one epoll instance              */
    int              listen_fd;         /* client-facing listener              */
    int              timer_fd;          /* timerfd_create for health ticks     */
    int              sig_fd;            /* signalfd for SIGINT/SIGTERM         */
    int              ctl_fd;            /* AF_UNIX control-socket listener     */

    struct io_handle h_listen, h_timer, h_signal, h_control;

    struct backend  *be;                /* backend array [0, nbe)              */
    int              nbe;

    enum policy      policy;
    int              rr_cursor;         /* round-robin rotation position       */

    struct ring      ring;              /* consistent-hash ring (policy=hash)  */

    int              warm;              /* warm-pool target per backend (-w)   */
    int              health_ms;         /* health-check interval (-i)          */
    const char      *ctl_path;          /* control socket filesystem path      */

    struct conn     *conns;             /* head of the live-conn list          */
    struct conn     *dead;              /* conns closed this batch, awaiting free*/
    int              nconns;            /* count of live conns                 */
    bool             shutting_down;     /* SIGINT/SIGTERM received; draining    */
};

/* ---- hashring.c ---------------------------------------------------------- */

/* FNV-1a 32-bit hash — shared by the ring builder and the client-IP lookup so
 * both live in the same coordinate space. Defined in hashring.c. */
uint32_t lb_hash(const void *data, size_t len);

/* A backend is eligible for NEW connections iff it is UP and not draining. This
 * is the single source of truth used by both selection and ring building. */
static inline bool be_eligible(const struct backend *b)
{
    return b->state == BE_UP && !b->draining;
}

/* (Re)build the ring from the currently eligible backends. Call whenever
 * eligibility changes (a backend goes UP/DOWN or is drained/undrained). */
void ring_build(struct ring *r, const struct backend *be, int nbe, int vnodes);

/* Map a key hash to a backend index by walking clockwise to the next vnode,
 * wrapping around the ring. Returns -1 if the ring is empty. */
int  ring_lookup(const struct ring *r, uint32_t key);

/* Free the ring's node array. */
void ring_free(struct ring *r);

#endif /* LB_H */
