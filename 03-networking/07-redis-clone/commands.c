/* ===========================================================================
 * commands.c — the command table and the handlers behind each Redis verb.
 * ===========================================================================
 * Each handler reads its arguments from c->argv[0..argc-1] (argv[0] is the
 * command name) and writes a RESP reply with the addReply* helpers. Handlers
 * that mutate the dataset bump server.dirty; the dispatcher (processCommand in
 * server.c) uses that delta to decide whether to append the command to the AOF.
 *
 * Arity in the table: a POSITIVE arity is exact; a NEGATIVE arity is a minimum
 * (its absolute value), for variadic commands like DEL/LPUSH/HSET. Arity counts
 * the command name itself, matching Redis.
 * =========================================================================== */
#include "server.h"
#include "zmalloc.h"

#include <string.h>
#include <strings.h>   /* strcasecmp */
#include <stdio.h>     /* snprintf   */
#include <limits.h>    /* LLONG_MAX/MIN */

/* ---------------------------------------------------------------------------
 * Small shared helpers.
 * ------------------------------------------------------------------------- */

/* Standard WRONGTYPE error, e.g. LPUSH on a key that holds a string. */
static void addReplyWrongType(client *c)
{
    addReplyError(c,
        "WRONGTYPE Operation against a key holding the wrong kind of value");
}

/* Return 1 (and reply WRONGTYPE) if `o` is not of `type`. */
static int checkType(client *c, robj *o, int type)
{
    if (o->type != type) { addReplyWrongType(c); return 1; }
    return 0;
}

/* Parse an sds as a long long, replying with `err` and returning 0 on failure. */
static int getLongLongOrReply(client *c, sds s, long long *out, const char *err)
{
    if (!string2ll(s, sdslen(s), out)) {
        addReplyError(c, err ? err : "ERR value is not an integer or out of range");
        return 0;
    }
    return 1;
}

/* Build a fresh sds holding the decimal text of `v`. Caller owns it. */
static sds sdsFromLongLong(long long v)
{
    char buf[32];
    int n = snprintf(buf, sizeof(buf), "%lld", v);
    return sdsnewlen(buf, (size_t)n);
}

/* ---------------------------------------------------------------------------
 * Connection / misc commands.
 * ------------------------------------------------------------------------- */
static void pingCommand(client *c)
{
    /* PING            -> +PONG
     * PING <message>  -> bulk echo of the message (Redis behaviour). */
    if (c->argc == 1) addReplyStatus(c, "PONG");
    else              addReplyBulkSds(c, c->argv[1]);
}

static void echoCommand(client *c)
{
    addReplyBulkSds(c, c->argv[1]);
}

static void quitCommand(client *c)
{
    addReplyStatus(c, "OK");
    c->flags |= CLIENT_CLOSE_AFTER_REPLY;   /* hang up once the +OK is flushed  */
}

/* redis-cli issues COMMAND DOCS / COMMAND COUNT on connect. We do not maintain
 * command metadata, so we answer with an empty array — enough to keep the CLI
 * happy without pretending to introspect. */
static void commandCommand(client *c)
{
    addReplyArrayLen(c, 0);
}

/* Minimal CONFIG: GET returns an empty map, anything else returns +OK. Real
 * config is out of scope; this exists so redis-cli's startup probes succeed. */
static void configCommand(client *c)
{
    if (c->argc >= 2 && strcasecmp(c->argv[1], "GET") == 0)
        addReplyArrayLen(c, 0);
    else
        addReplyStatus(c, "OK");
}

/* ---------------------------------------------------------------------------
 * String commands: GET / SET / INCR / DECR / INCRBY.
 * ------------------------------------------------------------------------- */
static void getCommand(client *c)
{
    robj *o = lookupKeyRead(c->argv[1]);
    if (o == NULL) { addReplyNull(c); return; }
    if (checkType(c, o, OBJ_STRING)) return;
    addReplyBulkSds(c, o->as.str);
}

static void setCommand(client *c)
{
    /* SET key value [EX seconds | PX milliseconds] */
    long long expire_ms = -1;   /* -1 = no expiry                              */
    for (int j = 3; j < c->argc; j++) {
        sds opt = c->argv[j];
        int is_ex = (strcasecmp(opt, "EX") == 0);
        int is_px = (strcasecmp(opt, "PX") == 0);
        if ((is_ex || is_px) && j + 1 < c->argc) {
            long long n;
            if (!getLongLongOrReply(c, c->argv[j + 1], &n, NULL)) return;
            if (n <= 0) { addReplyError(c, "ERR invalid expire time in 'set'"); return; }
            expire_ms = is_ex ? n * 1000 : n;   /* EX is seconds, PX is ms      */
            j++;                                 /* also consumed the value      */
        } else {
            addReplyError(c, "ERR syntax error");
            return;
        }
    }

    /* Store an owned copy of the value bytes under an owned copy of the key. */
    robj *o = createStringObject(c->argv[2], sdslen(c->argv[2]));
    setKey(sdsdup(c->argv[1]), o);
    if (expire_ms >= 0) setExpire(c->argv[1], mstime() + expire_ms);
    else                removeExpire(c->argv[1]);   /* SET clears any old TTL   */
    server.dirty++;
    addReplyStatus(c, "OK");
}

/* Shared INCR/DECR/INCRBY core: read the key as an integer (missing == 0), add
 * `incr` with overflow protection, store the new value back as a string. */
static void incrDecr(client *c, long long incr)
{
    long long value = 0;
    robj *o = lookupKeyWrite(c->argv[1]);
    if (o != NULL) {
        if (checkType(c, o, OBJ_STRING)) return;
        if (!getLongLongOrReply(c, o->as.str, &value,
                                "ERR value is not an integer or out of range"))
            return;
    }
    /* Detect signed overflow BEFORE performing the addition (adding past the
     * limit would be undefined behaviour). */
    if ((incr > 0 && value > LLONG_MAX - incr) ||
        (incr < 0 && value < LLONG_MIN - incr)) {
        addReplyError(c, "ERR increment or decrement would overflow");
        return;
    }
    value += incr;

    /* Replace the value with the freshly-formatted number. */
    robj *newo = createStringObjectFromSds(sdsFromLongLong(value));
    setKey(sdsdup(c->argv[1]), newo);
    server.dirty++;
    addReplyLongLong(c, value);
}

static void incrCommand(client *c) { incrDecr(c, 1); }
static void decrCommand(client *c) { incrDecr(c, -1); }
static void incrbyCommand(client *c)
{
    long long incr;
    if (!getLongLongOrReply(c, c->argv[2], &incr, NULL)) return;
    incrDecr(c, incr);
}

/* ---------------------------------------------------------------------------
 * Generic key commands: DEL / EXISTS / EXPIRE / TTL / TYPE / KEYS scope.
 * ------------------------------------------------------------------------- */
static void delCommand(client *c)
{
    long deleted = 0;
    for (int j = 1; j < c->argc; j++) {
        expireIfNeeded(c->argv[j]);           /* an already-expired key is a miss */
        if (dbDelete(c->argv[j])) { deleted++; server.dirty++; }
    }
    addReplyLongLong(c, deleted);
}

static void existsCommand(client *c)
{
    long count = 0;
    for (int j = 1; j < c->argc; j++)
        if (dbExists(c->argv[j])) count++;    /* counts multiplicity, like Redis */
    addReplyLongLong(c, count);
}

static void expireCommand(client *c)
{
    long long seconds;
    if (!getLongLongOrReply(c, c->argv[2], &seconds, NULL)) return;
    if (lookupKeyWrite(c->argv[1]) == NULL) { addReplyLongLong(c, 0); return; }

    long long when = mstime() + seconds * 1000;
    if (when <= mstime()) {
        /* A non-positive TTL means "delete now". */
        dbDelete(c->argv[1]);
    } else {
        setExpire(c->argv[1], when);
    }
    server.dirty++;
    addReplyLongLong(c, 1);
}

static void ttlCommand(client *c)
{
    /* -2 key missing, -1 key exists but has no TTL, else remaining seconds. */
    if (lookupKeyRead(c->argv[1]) == NULL) { addReplyLongLong(c, -2); return; }
    long long when = getExpire(c->argv[1]);
    if (when < 0) { addReplyLongLong(c, -1); return; }
    long long ttl = when - mstime();
    if (ttl < 0) ttl = 0;
    addReplyLongLong(c, (ttl + 500) / 1000);   /* round to nearest second        */
}

static void typeCommand(client *c)
{
    robj *o = lookupKeyRead(c->argv[1]);
    const char *t = "none";
    if (o) {
        switch (o->type) {
        case OBJ_STRING: t = "string"; break;
        case OBJ_LIST:   t = "list";   break;
        case OBJ_HASH:   t = "hash";   break;
        }
    }
    addReplyStatus(c, t);                        /* +string / +none / ...          */
}

static void dbsizeCommand(client *c)
{
    addReplyLongLong(c, (long long)dictSize(server.db.dict));
}

static void flushdbCommand(client *c)
{
    /* Drop the whole keyspace by releasing and recreating both dicts. */
    dictRelease(server.db.dict);
    dictRelease(server.db.expires);
    server.db.dict    = dictCreate(&dbDictType);
    server.db.expires = dictCreate(&keyptrDictType);
    server.dirty++;
    addReplyStatus(c, "OK");
}

/* ---------------------------------------------------------------------------
 * List commands: LPUSH / RPUSH / LRANGE / LLEN / LPOP / RPOP.
 * ------------------------------------------------------------------------- */

/* Shared LPUSH/RPUSH: `head` selects the insertion end. */
static void pushGeneric(client *c, int head)
{
    robj *o = lookupKeyWrite(c->argv[1]);
    if (o != NULL && checkType(c, o, OBJ_LIST)) return;
    if (o == NULL) {                            /* first push creates the list    */
        o = createListObject();
        dbAdd(sdsdup(c->argv[1]), o);
    }
    for (int j = 2; j < c->argc; j++) {
        sds val = sdsdup(c->argv[j]);           /* the list owns its own copy     */
        if (head) listAddNodeHead(o->as.list, val);
        else      listAddNodeTail(o->as.list, val);
    }
    server.dirty += (c->argc - 2);
    addReplyLongLong(c, (long long)listLength(o->as.list));
}

static void lpushCommand(client *c) { pushGeneric(c, 1); }
static void rpushCommand(client *c) { pushGeneric(c, 0); }

static void llenCommand(client *c)
{
    robj *o = lookupKeyRead(c->argv[1]);
    if (o == NULL) { addReplyLongLong(c, 0); return; }
    if (checkType(c, o, OBJ_LIST)) return;
    addReplyLongLong(c, (long long)listLength(o->as.list));
}

static void lrangeCommand(client *c)
{
    long long start, stop;
    if (!getLongLongOrReply(c, c->argv[2], &start, NULL)) return;
    if (!getLongLongOrReply(c, c->argv[3], &stop, NULL)) return;

    robj *o = lookupKeyRead(c->argv[1]);
    if (o == NULL) { addReplyArrayLen(c, 0); return; }
    if (checkType(c, o, OBJ_LIST)) return;

    long long llen = (long long)listLength(o->as.list);
    /* Negative indices count from the end (-1 == last element). */
    if (start < 0) start = llen + start;
    if (stop  < 0) stop  = llen + stop;
    if (start < 0) start = 0;
    if (stop >= llen) stop = llen - 1;
    if (start > stop || llen == 0) { addReplyArrayLen(c, 0); return; }

    long long rangelen = stop - start + 1;
    addReplyArrayLen(c, (long)rangelen);
    /* Walk to the start node, then emit rangelen elements. */
    listNode *node = o->as.list->head;
    for (long long i = 0; i < start; i++) node = node->next;
    for (long long i = 0; i < rangelen; i++) {
        addReplyBulkSds(c, (sds)node->value);
        node = node->next;
    }
}

/* Shared LPOP/RPOP: `head` selects which end to remove from. */
static void popGeneric(client *c, int head)
{
    robj *o = lookupKeyWrite(c->argv[1]);
    if (o == NULL) { addReplyNull(c); return; }
    if (checkType(c, o, OBJ_LIST)) return;

    listNode *node = head ? o->as.list->head : o->as.list->tail;
    if (node == NULL) { addReplyNull(c); return; }

    addReplyBulkSds(c, (sds)node->value);       /* reply with the popped element  */
    listDelNode(o->as.list, node);              /* frees the node and its sds     */
    if (listLength(o->as.list) == 0)            /* empty list -> delete the key   */
        dbDelete(c->argv[1]);
    server.dirty++;
}

static void lpopCommand(client *c) { popGeneric(c, 1); }
static void rpopCommand(client *c) { popGeneric(c, 0); }

/* ---------------------------------------------------------------------------
 * Hash commands: HSET / HGET / HDEL / HGETALL / HLEN.
 * ------------------------------------------------------------------------- */
static void hsetCommand(client *c)
{
    /* HSET key field value [field value ...] — an even number of trailing args. */
    if ((c->argc - 2) % 2 != 0) {
        addReplyError(c, "ERR wrong number of arguments for 'hset'");
        return;
    }
    robj *o = lookupKeyWrite(c->argv[1]);
    if (o != NULL && checkType(c, o, OBJ_HASH)) return;
    if (o == NULL) {
        o = createHashObject();
        dbAdd(sdsdup(c->argv[1]), o);
    }
    long added = 0;
    for (int j = 2; j < c->argc; j += 2) {
        sds field = sdsdup(c->argv[j]);
        sds value = sdsdup(c->argv[j + 1]);
        /* dictReplace returns 1 if the field was newly created (a counted add),
         * 0 if it overwrote an existing field. It frees the duplicate key/old
         * value as required. */
        if (dictReplace(o->as.hash, field, value)) added++;
    }
    server.dirty++;
    addReplyLongLong(c, added);
}

static void hgetCommand(client *c)
{
    robj *o = lookupKeyRead(c->argv[1]);
    if (o == NULL) { addReplyNull(c); return; }
    if (checkType(c, o, OBJ_HASH)) return;
    sds val = dictFetchValue(o->as.hash, c->argv[2]);
    if (val == NULL) addReplyNull(c);
    else             addReplyBulkSds(c, val);
}

static void hdelCommand(client *c)
{
    robj *o = lookupKeyWrite(c->argv[1]);
    if (o == NULL) { addReplyLongLong(c, 0); return; }
    if (checkType(c, o, OBJ_HASH)) return;
    long deleted = 0;
    for (int j = 2; j < c->argc; j++)
        if (dictDelete(o->as.hash, c->argv[j]) == DICT_OK) deleted++;
    if (dictSize(o->as.hash) == 0) dbDelete(c->argv[1]); /* empty hash -> del key */
    if (deleted) server.dirty++;
    addReplyLongLong(c, deleted);
}

static void hlenCommand(client *c)
{
    robj *o = lookupKeyRead(c->argv[1]);
    if (o == NULL) { addReplyLongLong(c, 0); return; }
    if (checkType(c, o, OBJ_HASH)) return;
    addReplyLongLong(c, (long long)dictSize(o->as.hash));
}

/* HGETALL emits a flat array [field1, value1, field2, value2, ...]. We use the
 * dict iterator, which is safe to walk mid-rehash. */
struct hgetallCtx { client *c; };
static void hgetallEmit(void *priv, const void *key, void *val)
{
    client *c = ((struct hgetallCtx *)priv)->c;
    addReplyBulkSds(c, (sds)key);
    addReplyBulkSds(c, (sds)val);
}
static void hgetallCommand(client *c)
{
    robj *o = lookupKeyRead(c->argv[1]);
    if (o == NULL) { addReplyArrayLen(c, 0); return; }
    if (checkType(c, o, OBJ_HASH)) return;
    addReplyArrayLen(c, (long)(dictSize(o->as.hash) * 2)); /* field+value pairs   */
    struct hgetallCtx ctx = { c };
    dictForEach(o->as.hash, hgetallEmit, &ctx);
}

/* ---------------------------------------------------------------------------
 * Persistence commands: SAVE / BGSAVE.
 * ------------------------------------------------------------------------- */
static void saveCommand(client *c)
{
    if (server.rdb_child_pid != -1) {
        addReplyError(c, "ERR Background save already in progress");
        return;
    }
    if (rdbSave(server.rdb_filename) == 0) addReplyStatus(c, "OK");
    else                                   addReplyError(c, "ERR save failed");
}

static void bgsaveCommand(client *c)
{
    if (server.rdb_child_pid != -1) {
        addReplyStatus(c, "Background saving already in progress");
        return;
    }
    if (rdbSaveBackground() == 0) addReplyStatus(c, "Background saving started");
    else                          addReplyError(c, "ERR bgsave failed");
}

/* ---------------------------------------------------------------------------
 * The command table. lookupCommand does a case-insensitive linear scan — with
 * ~30 commands that is a handful of comparisons, cheaper than the machinery of
 * a hash lookup and easy to read. Real Redis stores these in a dict.
 * ------------------------------------------------------------------------- */
static struct redisCommand commandTable[] = {
    /* name        proc              arity  flags        */
    { "ping",      pingCommand,       -1,   0 },
    { "echo",      echoCommand,        2,   0 },
    { "quit",      quitCommand,        1,   0 },
    { "command",   commandCommand,    -1,   0 },
    { "config",    configCommand,     -2,   0 },

    { "get",       getCommand,         2,   0 },
    { "set",       setCommand,        -3,   CMD_WRITE },
    { "incr",      incrCommand,        2,   CMD_WRITE },
    { "decr",      decrCommand,        2,   CMD_WRITE },
    { "incrby",    incrbyCommand,      3,   CMD_WRITE },

    { "del",       delCommand,        -2,   CMD_WRITE },
    { "exists",    existsCommand,     -2,   0 },
    { "expire",    expireCommand,      3,   CMD_WRITE },
    { "ttl",       ttlCommand,         2,   0 },
    { "type",      typeCommand,        2,   0 },
    { "dbsize",    dbsizeCommand,      1,   0 },
    { "flushdb",   flushdbCommand,     1,   CMD_WRITE },

    { "lpush",     lpushCommand,      -3,   CMD_WRITE },
    { "rpush",     rpushCommand,      -3,   CMD_WRITE },
    { "lpop",      lpopCommand,        2,   CMD_WRITE },
    { "rpop",      rpopCommand,        2,   CMD_WRITE },
    { "llen",      llenCommand,        2,   0 },
    { "lrange",    lrangeCommand,      4,   0 },

    { "hset",      hsetCommand,       -4,   CMD_WRITE },
    { "hget",      hgetCommand,        3,   0 },
    { "hdel",      hdelCommand,       -3,   CMD_WRITE },
    { "hlen",      hlenCommand,        2,   0 },
    { "hgetall",   hgetallCommand,     2,   0 },

    { "subscribe", subscribeCommand,  -2,   CMD_PUBSUB },
    { "unsubscribe",unsubscribeCommand,-1,  CMD_PUBSUB },
    { "publish",   publishCommand,     3,   0 },

    { "save",      saveCommand,        1,   0 },
    { "bgsave",    bgsaveCommand,      1,   0 },
};

struct redisCommand *lookupCommand(sds name)
{
    size_t n = sizeof(commandTable) / sizeof(commandTable[0]);
    for (size_t i = 0; i < n; i++)
        if (strcasecmp(name, commandTable[i].name) == 0)
            return &commandTable[i];
    return NULL;
}
