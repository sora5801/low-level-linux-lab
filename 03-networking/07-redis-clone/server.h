/* ===========================================================================
 * server.h — central type definitions and cross-module prototypes.
 * ===========================================================================
 *
 * This is the "spine" header: the client, the value objects (robj), the single
 * database, and the one global `server` singleton all live here, plus the
 * prototypes every .c file needs from its neighbours. Read it top to bottom to
 * get the shape of the whole program before diving into any one module.
 *
 * PLATFORM: Linux (or WSL2). The event loop is built on epoll(7) and the
 * snapshot path on fork(2)'s copy-on-write — both Linux facilities. See README.
 * =========================================================================== */
#ifndef SERVER_H
#define SERVER_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>   /* pid_t                 */
#include <time.h>        /* time_t                */
#include <signal.h>      /* sig_atomic_t          */

#include "sds.h"
#include "dict.h"

/* ---------------------------------------------------------------------------
 * Value objects (robj). Redis keys map to typed values. Our teaching core
 * supports three types; each variant owns its payload.
 * ------------------------------------------------------------------------- */
#define OBJ_STRING 0   /* an sds byte string (GET/SET/INCR)                     */
#define OBJ_LIST   1   /* a doubly linked list of sds (LPUSH/LRANGE)           */
#define OBJ_HASH   2   /* a nested dict of sds->sds (HSET/HGET)                */

typedef struct robj {
    int type;                  /* one of OBJ_*                                  */
    union {
        sds          str;      /* OBJ_STRING                                    */
        struct list *list;     /* OBJ_LIST                                      */
        dict        *hash;     /* OBJ_HASH                                      */
    } as;
} robj;

/* ---------------------------------------------------------------------------
 * A generic doubly linked list (used for OBJ_LIST values and for the set of
 * subscribers on a pub/sub channel). Elements are `void *`; an optional
 * destructor frees them on release (sdsfree for value lists; NULL for the
 * subscriber list, whose client pointers are owned elsewhere).
 * ------------------------------------------------------------------------- */
typedef struct listNode {
    struct listNode *prev, *next;
    void *value;               /* sds (value list) or client* (subscriber list) */
} listNode;

typedef struct list {
    listNode *head, *tail;
    unsigned long len;
    void (*free)(void *value); /* element destructor, or NULL                   */
} list;

list     *listCreate(void);
void      listRelease(list *l);
listNode *listAddNodeHead(list *l, void *value);  /* LPUSH: prepend             */
listNode *listAddNodeTail(list *l, void *value);  /* RPUSH: append              */
void      listDelNode(list *l, listNode *node);   /* unlink + free one node     */
#define   listLength(l) ((l)->len)

/* ---------------------------------------------------------------------------
 * The database: the keyspace plus a parallel table of expiration deadlines.
 * `expires` is keyed by the same key strings (its own owned copies) and stores
 * an absolute millisecond deadline encoded straight into the value pointer.
 * ------------------------------------------------------------------------- */
typedef struct redisDb {
    dict *dict;      /* key (sds)  -> robj*                                     */
    dict *expires;   /* key (sds)  -> deadline ms, as (void*)(intptr_t)         */
} redisDb;

/* ---------------------------------------------------------------------------
 * A connected client and everything we track for it.
 * ------------------------------------------------------------------------- */
#define CLIENT_CLOSE_AFTER_REPLY (1 << 0)  /* drain reply, then hang up         */
#define CLIENT_PENDING_WRITE     (1 << 1)  /* fd is armed for EPOLLOUT          */

/* Request framing types discovered on the first byte of a command. */
#define REQ_INLINE     1                   /* a bare line: PING\r\n             */
#define REQ_MULTIBULK  2                   /* RESP array: *3\r\n$3\r\nSET...    */

typedef struct client {
    int fd;                    /* the connected socket                          */

    /* ---- input side --------------------------------------------------- */
    sds    querybuf;           /* accumulated, not-yet-parsed input bytes       */
    size_t qpos;               /* parse cursor: bytes of querybuf consumed      */
    int    reqtype;            /* REQ_INLINE / REQ_MULTIBULK / 0 (undetermined) */
    int    multibulklen;       /* multibulk: bulk args still to read            */
    long   bulklen;            /* multibulk: length of the in-progress bulk, -1 */

    /* ---- the currently parsed command --------------------------------- */
    int  argc;                 /* number of arguments                           */
    sds *argv;                 /* argv[0]=command name, ... (each owned)        */

    /* ---- output side (a single growable reply buffer) ----------------- */
    char  *buf;                /* queued reply bytes                            */
    size_t bufpos;             /* total bytes queued                            */
    size_t sentlen;            /* bytes already written to the socket           */
    size_t bufcap;             /* capacity of buf                               */

    int flags;                 /* CLIENT_* bitmask                              */

    /* ---- pub/sub ------------------------------------------------------- */
    dict *pubsub_channels;     /* set: channel(sds) -> NULL                     */

    struct client *prev, *next;/* intrusive server-wide client list             */
} client;

/* ---------------------------------------------------------------------------
 * The command table entry and dispatcher contract.
 * ------------------------------------------------------------------------- */
typedef void redisCommandProc(client *c);

#define CMD_WRITE  (1 << 0)    /* mutates the dataset -> may be logged to AOF    */
#define CMD_PUBSUB (1 << 1)    /* allowed while a client is in subscribe mode    */

struct redisCommand {
    const char       *name;    /* lowercase command name                        */
    redisCommandProc *proc;    /* the handler                                   */
    int               arity;   /* argc: exact if >0, minimum if <0 (incl. name) */
    int               flags;   /* CMD_* bitmask                                 */
};

struct redisCommand *lookupCommand(sds name);

/* ---------------------------------------------------------------------------
 * AOF fsync policies (durability vs throughput trade-off).
 * ------------------------------------------------------------------------- */
#define AOF_FSYNC_NO       0   /* never fsync: fastest, lose up to OS buffer     */
#define AOF_FSYNC_EVERYSEC 1   /* fsync once per second (Redis default)          */
#define AOF_FSYNC_ALWAYS   2   /* fsync every command: safest, slowest           */

/* ---------------------------------------------------------------------------
 * The server singleton. Redis is architected around one global server state;
 * we follow that (a per-call context pointer would be pure ceremony here). The
 * event loop, the database, the client list and both persistence engines hang
 * off this struct.
 * ------------------------------------------------------------------------- */
struct redisServer {
    int   port;                /* TCP port to listen on                         */
    int   listen_fd;           /* the listening socket                          */
    int   epoll_fd;            /* epoll instance driving the whole loop         */
    struct epoll_event *events;/* scratch array filled by epoll_wait            */
    int   maxevents;           /* size of the events array                      */

    redisDb db;                /* the single database (db0)                     */

    client       *clients;     /* head of the intrusive client list             */
    unsigned int  numclients;

    dict *pubsub_channels;     /* channel(sds) -> list* of subscriber clients    */

    long long dirty;           /* dataset changes since the last save (AOF gate) */

    /* ---- persistence: RDB ---------------------------------------------- */
    char *rdb_filename;        /* snapshot path                                 */
    pid_t rdb_child_pid;       /* pid of the forked BGSAVE child, or -1          */

    /* ---- persistence: AOF ---------------------------------------------- */
    int    aof_enabled;        /* is append-only logging on?                    */
    int    aof_fd;             /* the open AOF file, or -1                       */
    sds    aof_buf;            /* pending bytes to write+fsync in the cron       */
    int    aof_fsync;          /* one of AOF_FSYNC_*                             */
    time_t aof_last_fsync;     /* last time we fsynced (for EVERYSEC)            */
    char  *aof_filename;

    /* ---- time / control ------------------------------------------------ */
    long long mstime;          /* cached wall-clock time in ms (per loop tick)   */
    long long cron_last_ms;    /* last serverCron run (ms)                       */
    volatile sig_atomic_t shutdown_asap; /* set by SIGTERM/SIGINT handler        */
    uint64_t  hashseed;        /* per-run random seed for the dict hash          */
};

extern struct redisServer server;   /* defined once in server.c                 */

/* ---------------------------------------------------------------------------
 * object.c — value construction / destruction.
 * ------------------------------------------------------------------------- */
robj *createStringObject(const char *ptr, size_t len); /* copies the bytes      */
robj *createStringObjectFromSds(sds s);   /* TAKES OWNERSHIP of s               */
robj *createListObject(void);
robj *createHashObject(void);
void  freeObject(robj *o);                /* frees the object and its payload    */

extern dictType dbDictType;     /* keyspace:      sds  -> robj*                  */
extern dictType hashDictType;   /* hash value:    sds  -> sds                    */
extern dictType keyptrDictType; /* expires/sets:  sds  -> borrowed value / NULL  */
extern dictType pubsubDictType; /* channels:      sds  -> list* of clients       */

/* ---------------------------------------------------------------------------
 * db.c — keyspace access, expiration.
 * ------------------------------------------------------------------------- */
robj *lookupKeyRead(sds key);   /* NULL if absent or (lazily) expired            */
robj *lookupKeyWrite(sds key);  /* like read but for write commands              */
void  dbAdd(sds key, robj *val);              /* key must not already exist      */
void  dbOverwrite(sds key, robj *val);        /* key must exist; frees old val   */
void  setKey(sds key, robj *val);             /* add or overwrite                */
int   dbDelete(sds key);                      /* remove key + any expire; 0/1    */
int   dbExists(sds key);

long long getExpire(sds key);                 /* deadline ms, or -1 if none      */
void  setExpire(sds key, long long when_ms);  /* set/replace the deadline        */
int   removeExpire(sds key);
int   expireIfNeeded(sds key);                /* lazy expiry; 1 if it was expired */
void  activeExpireCycle(void);                /* sampled background expiry        */

/* ---------------------------------------------------------------------------
 * resp.c — protocol parsing and reply construction.
 * ------------------------------------------------------------------------- */
void processInputBuffer(client *c);           /* parse+run all complete commands */
void resetClientCommand(client *c);           /* free argv + per-command state    */
int  string2ll(const char *s, size_t len, long long *out); /* strict int parse    */
void processCommand(client *c);               /* arity check + dispatch (server.c)*/

/* Low-level: append raw bytes to the client's reply buffer (grows as needed). */
void addReplyRaw(client *c, const void *s, size_t len);

/* Typed RESP reply helpers (RESP2). */
void addReplyError(client *c, const char *err);       /* "-ERR ...\r\n"          */
void addReplyStatus(client *c, const char *status);   /* "+OK\r\n"               */
void addReplyLongLong(client *c, long long ll);       /* ":123\r\n"              */
void addReplyBulkCBuffer(client *c, const void *p, size_t len); /* "$n\r\n..."    */
void addReplyBulkCString(client *c, const char *s);
void addReplyBulkSds(client *c, sds s);               /* bulk from an sds (copy) */
void addReplyNull(client *c);                         /* "$-1\r\n"               */
void addReplyArrayLen(client *c, long n);             /* "*n\r\n"                */

/* ---------------------------------------------------------------------------
 * server.c — networking, event loop, time.
 * ------------------------------------------------------------------------- */
long long mstime(void);                       /* wall-clock milliseconds         */
void freeClient(client *c);                   /* unlink, close fd, free buffers  */
void serverLog(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
/* Arm a client's fd for EPOLLOUT when it has unsent reply bytes. Used by PUBLISH
 * to schedule writes to OTHER clients from outside their read handler. */
void clientInstallWriteHandler(client *c);

/* ---------------------------------------------------------------------------
 * persist.c — RDB snapshotting and AOF logging.
 * ------------------------------------------------------------------------- */
int  rdbSave(const char *filename);           /* synchronous snapshot            */
int  rdbSaveBackground(void);                 /* fork + CoW snapshot             */
int  rdbLoad(const char *filename);           /* restore at startup              */
void backgroundSaveDoneHandler(int ok);       /* reap child, log result          */

void aofInit(void);
void aofFeed(sds *argv, int argc);            /* encode+queue a write command    */
void flushAppendOnlyFile(int force);          /* write buffered AOF, fsync/policy */
int  loadAppendOnlyFile(const char *filename);/* replay the AOF at startup       */

/* ---------------------------------------------------------------------------
 * pubsub.c
 * ------------------------------------------------------------------------- */
void pubsubInit(void);                        /* create server.pubsub_channels    */
void subscribeCommand(client *c);
void unsubscribeCommand(client *c);
void publishCommand(client *c);
int  pubsubUnsubscribeAll(client *c);         /* on disconnect; returns count     */

#endif /* SERVER_H */
