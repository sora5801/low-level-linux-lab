/* ===========================================================================
 * persist.c — durability: RDB point-in-time snapshots and the AOF command log.
 * ===========================================================================
 *
 * Two complementary strategies, exactly as in Redis:
 *
 * RDB (Redis DataBase): a compact binary SNAPSHOT of the whole keyspace. We take
 * it without blocking the server by fork(2)ing: the child inherits a
 * copy-on-write view of the parent's memory, serializes the dataset as it
 * existed at the instant of the fork, and exits. The parent keeps serving; only
 * pages the parent MODIFIES during the save are physically copied (that is the
 * "CoW" — copy on write — and it is why a save costs memory proportional to the
 * write rate, not to the dataset size).
 *
 * AOF (Append Only File): a REDO LOG. Every write command is appended, in RESP,
 * to a file; on restart we replay the file to reconstruct the dataset. The
 * fsync policy trades durability for throughput:
 *     always   — fsync after every command (lose nothing, slowest)
 *     everysec — fsync at most once per second (lose <=1s, the default)
 *     no       — never fsync explicitly (fastest, lose whatever the OS buffers)
 *
 * OUR RDB FORMAT is custom (not byte-compatible with real Redis) but real in
 * spirit: a magic header, a record per key (optional expire, a type byte, the
 * length-prefixed key, then the value), an EOF marker, and a trailing CRC-64 for
 * integrity. Integers are stored little-endian to match the x86-64 host.
 * =========================================================================== */
#include "server.h"
#include "zmalloc.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     /* fork, write, read, fsync, close, _exit, unlink       */
#include <fcntl.h>      /* open, O_*                                            */
#include <errno.h>      /* errno, EINTR                                         */
#include <sys/stat.h>   /* fstat, mode bits                                     */
#include <sys/types.h>

/* Our snapshot magic. Bumping the version invalidates old files on load. */
static const char RDB_MAGIC[] = "REDIS-TEACH0001";   /* 15 bytes, no NUL saved   */
#define RDB_MAGIC_LEN 15

/* Record opcodes. */
#define RDB_OP_EOF     0xFF     /* end of file; a CRC-64 follows                 */
#define RDB_OP_EXPIRE  0xFD     /* an 8-byte ms deadline precedes the next record*/
#define RDB_TYPE_STRING 0x00
#define RDB_TYPE_LIST   0x01
#define RDB_TYPE_HASH   0x02

/* ---------------------------------------------------------------------------
 * CRC-64 (reflected, poly 0xC96C5795D7870F42 == CRC-64/XZ). Used only for our
 * own integrity check, so it need not match Redis's Jones-poly CRC — it only
 * has to be self-consistent between save and load. Bitwise (no table) to stay
 * self-contained and obviously correct; the file is written once, so speed is
 * irrelevant here.
 * ------------------------------------------------------------------------- */
static uint64_t crc64(uint64_t crc, const void *data, size_t len)
{
    const uint8_t *p = (const uint8_t *)data;
    const uint64_t poly = 0xC96C5795D7870F42ULL;    /* reflected polynomial      */
    for (size_t i = 0; i < len; i++) {
        crc ^= p[i];                                /* fold in the next byte     */
        for (int b = 0; b < 8; b++)                 /* process 8 bits, LSB first  */
            crc = (crc & 1) ? (crc >> 1) ^ poly : (crc >> 1);
    }
    return crc;
}

/* ---------------------------------------------------------------------------
 * Little-endian append helpers (payload is a growable sds).
 * ------------------------------------------------------------------------- */
static sds appendU32(sds s, uint32_t v)
{
    uint8_t b[4] = { (uint8_t)v, (uint8_t)(v >> 8),
                     (uint8_t)(v >> 16), (uint8_t)(v >> 24) };
    return sdscatlen(s, b, 4);
}
static sds appendU64(sds s, uint64_t v)
{
    uint8_t b[8];
    for (int i = 0; i < 8; i++) b[i] = (uint8_t)(v >> (8 * i));  /* LE byte order */
    return sdscatlen(s, b, 8);
}
/* A length-prefixed byte string: uint32 length, then the bytes. */
static sds appendString(sds s, const void *p, size_t len)
{
    s = appendU32(s, (uint32_t)len);
    return sdscatlen(s, p, len);
}

/* ---------------------------------------------------------------------------
 * Serialization: turn the in-memory dataset into an RDB payload.
 * ------------------------------------------------------------------------- */

/* Append one hash field/value pair. Used as a dictForEach callback; `priv` is
 * the address of the payload sds so we can reassign it after each (reallocating)
 * append. */
static void rdbSaveHashField(void *priv, const void *key, void *val)
{
    sds *pp = (sds *)priv;
    *pp = appendString(*pp, key, sdslen((sds)key));
    *pp = appendString(*pp, val, sdslen((sds)val));
}

/* Append a value (the part after the type byte) for object `o`. */
static sds rdbSaveValue(sds s, robj *o)
{
    switch (o->type) {
    case OBJ_STRING:
        s = appendString(s, o->as.str, sdslen(o->as.str));
        break;
    case OBJ_LIST: {
        list *l = o->as.list;
        s = appendU32(s, (uint32_t)l->len);          /* element count           */
        for (listNode *n = l->head; n; n = n->next)  /* head..tail order         */
            s = appendString(s, n->value, sdslen((sds)n->value));
        break;
    }
    case OBJ_HASH:
        s = appendU32(s, (uint32_t)dictSize(o->as.hash)); /* field count         */
        dictForEach(o->as.hash, rdbSaveHashField, &s);    /* &s: may reallocate  */
        break;
    }
    return s;
}

/* dictForEach callback over the keyspace: append one full key record. */
struct saveCtx { sds *payload; };
static void rdbSaveEntry(void *priv, const void *key, void *val)
{
    sds *pp = ((struct saveCtx *)priv)->payload;
    robj *o = (robj *)val;

    long long expire = getExpire((sds)key);
    if (expire >= 0) {                               /* optional expire record   */
        uint8_t op = RDB_OP_EXPIRE;
        *pp = sdscatlen(*pp, &op, 1);
        *pp = appendU64(*pp, (uint64_t)expire);
    }
    uint8_t type = (uint8_t)o->type;                 /* RDB_TYPE_* == OBJ_*       */
    *pp = sdscatlen(*pp, &type, 1);
    *pp = appendString(*pp, key, sdslen((sds)key));  /* the key                  */
    *pp = rdbSaveValue(*pp, o);                       /* the value               */
}

/* Robustly write `len` bytes from `buf` to fd, retrying short writes/EINTR.
 * Returns 0 on success, -1 on a hard error. */
static int writeAll(int fd, const void *buf, size_t len)
{
    const uint8_t *p = (const uint8_t *)buf;
    while (len > 0) {
        ssize_t n = write(fd, p, len);
        if (n < 0) {
            if (errno == EINTR) continue;            /* a signal interrupted us   */
            return -1;
        }
        p   += n;                                    /* advance past bytes written*/
        len -= (size_t)n;                            /* partial writes are normal */
    }
    return 0;
}

/* Synchronous snapshot: build the whole payload in memory, write it to a temp
 * file, fsync, and atomically rename into place so a crash mid-save can never
 * leave a half-written snapshot at the real path. Returns 0 on success. */
int rdbSave(const char *filename)
{
    sds payload = sdsnewlen(RDB_MAGIC, RDB_MAGIC_LEN);
    struct saveCtx ctx = { &payload };
    dictForEach(server.db.dict, rdbSaveEntry, &ctx);

    uint8_t eof = RDB_OP_EOF;
    payload = sdscatlen(payload, &eof, 1);
    uint64_t crc = crc64(0, payload, sdslen(payload));  /* checksum all-but-CRC   */
    payload = appendU64(payload, crc);

    /* Write to "<filename>.tmp", fsync, then rename. */
    char tmp[4096];
    snprintf(tmp, sizeof(tmp), "%s.tmp", filename);
    int fd = open(tmp, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) { sdsfree(payload); return -1; }

    int ok = 0;
    if (writeAll(fd, payload, sdslen(payload)) != 0) ok = -1;
    if (ok == 0 && fsync(fd) != 0) ok = -1;          /* force bytes to the disk   */
    if (close(fd) != 0) ok = -1;
    if (ok == 0 && rename(tmp, filename) != 0) ok = -1; /* atomic replace         */
    if (ok != 0) unlink(tmp);                         /* clean up on failure       */

    sdsfree(payload);
    return ok;
}

/* Fork a child to snapshot without blocking the event loop. The child sees a
 * frozen copy-on-write image of the dataset; the parent returns immediately and
 * keeps serving clients. serverCron later reaps the child (waitpid) and calls
 * backgroundSaveDoneHandler. Returns 0 if the fork started. */
int rdbSaveBackground(void)
{
    if (server.rdb_child_pid != -1) return -1;       /* one save at a time        */

    pid_t pid = fork();
    if (pid < 0) return -1;                          /* fork failed (ENOMEM/EAGAIN)*/

    if (pid == 0) {
        /* ---- CHILD ----
         * We share the parent's address space copy-on-write. Reading the dataset
         * touches shared pages; the parent's subsequent WRITES are what trigger
         * per-page copies in the kernel. We must NOT run the parent's atexit
         * handlers or flush its stdio (that could double-flush buffers and
         * corrupt shared state), so we terminate with _exit, not exit. */
        int ret = rdbSave(server.rdb_filename);
        _exit(ret == 0 ? 0 : 1);                     /* status read by the parent */
    }

    /* ---- PARENT ---- */
    server.rdb_child_pid = pid;
    serverLog("Background saving started by pid %d", (int)pid);
    return 0;
}

/* Called from serverCron once the BGSAVE child has been reaped. */
void backgroundSaveDoneHandler(int ok)
{
    server.rdb_child_pid = -1;
    serverLog("Background saving %s", ok ? "terminated with success"
                                         : "error");
}

/* ---------------------------------------------------------------------------
 * Deserialization: rebuild the dataset from an RDB payload at startup.
 * ------------------------------------------------------------------------- */

/* A cursor over the file bytes with an error flag. Every read checks bounds and
 * sets `err` rather than reading past the end of a truncated/corrupt file. */
struct rdbReader { const uint8_t *buf; size_t len; size_t pos; int err; };

static uint8_t readU8(struct rdbReader *r)
{
    if (r->pos + 1 > r->len) { r->err = 1; return 0; }
    return r->buf[r->pos++];
}
static uint32_t readU32(struct rdbReader *r)
{
    if (r->pos + 4 > r->len) { r->err = 1; return 0; }
    uint32_t v = (uint32_t)r->buf[r->pos] | ((uint32_t)r->buf[r->pos + 1] << 8) |
                 ((uint32_t)r->buf[r->pos + 2] << 16) |
                 ((uint32_t)r->buf[r->pos + 3] << 24);
    r->pos += 4;
    return v;
}
static uint64_t readU64(struct rdbReader *r)
{
    if (r->pos + 8 > r->len) { r->err = 1; return 0; }
    uint64_t v = 0;
    for (int i = 0; i < 8; i++) v |= (uint64_t)r->buf[r->pos + i] << (8 * i);
    r->pos += 8;
    return v;
}
/* Read a length-prefixed string into a fresh sds (caller owns it). */
static sds readString(struct rdbReader *r)
{
    uint32_t len = readU32(r);
    if (r->err || r->pos + len > r->len) { r->err = 1; return NULL; }
    sds s = sdsnewlen(r->buf + r->pos, len);
    r->pos += len;
    return s;
}

/* Load the whole file into memory (returns an sds, or NULL if missing/error). */
static sds slurpFile(const char *filename, int *missing)
{
    *missing = 0;
    int fd = open(filename, O_RDONLY);
    if (fd < 0) { *missing = 1; return NULL; }       /* no file == nothing to load */

    struct stat st;
    if (fstat(fd, &st) != 0) { close(fd); return NULL; }
    size_t size = (size_t)st.st_size;

    sds data = sdsnewlen(NULL, size);                /* preallocate exact size    */
    size_t off = 0;
    while (off < size) {
        ssize_t n = read(fd, data + off, size - off);
        if (n < 0) { if (errno == EINTR) continue; close(fd); sdsfree(data); return NULL; }
        if (n == 0) break;                           /* unexpected EOF            */
        off += (size_t)n;
    }
    close(fd);
    return data;
}

/* Restore the dataset from `filename`. Returns 0 on success (or if the file is
 * simply absent), -1 on a corrupt/unreadable file. */
int rdbLoad(const char *filename)
{
    int missing;
    sds data = slurpFile(filename, &missing);
    if (missing) return 0;                           /* first run: nothing to load */
    if (data == NULL) return -1;

    size_t total = sdslen(data);
    /* Minimum viable file: magic + EOF opcode + 8-byte CRC. */
    if (total < RDB_MAGIC_LEN + 1 + 8 ||
        memcmp(data, RDB_MAGIC, RDB_MAGIC_LEN) != 0) {
        serverLog("RDB: bad magic or truncated file");
        sdsfree(data);
        return -1;
    }
    /* Verify the trailing CRC covers everything before it. */
    uint64_t stored = 0;
    for (int i = 0; i < 8; i++)
        stored |= (uint64_t)(uint8_t)data[total - 8 + i] << (8 * i);
    if (crc64(0, data, total - 8) != stored) {
        serverLog("RDB: CRC mismatch (corrupt snapshot)");
        sdsfree(data);
        return -1;
    }

    struct rdbReader r = { (const uint8_t *)data, total - 8, RDB_MAGIC_LEN, 0 };
    int rc = 0;
    for (;;) {
        long long expire = -1;
        uint8_t op = readU8(&r);
        if (r.err) { rc = -1; break; }
        if (op == RDB_OP_EOF) break;                 /* clean end                 */
        if (op == RDB_OP_EXPIRE) {                   /* an expire precedes a record*/
            expire = (long long)readU64(&r);
            op = readU8(&r);                         /* now the real type byte    */
        }

        sds key = readString(&r);
        if (r.err) { rc = -1; break; }

        robj *o = NULL;
        if (op == RDB_TYPE_STRING) {
            sds v = readString(&r);
            if (r.err) { sdsfree(key); rc = -1; break; }
            o = createStringObjectFromSds(v);
        } else if (op == RDB_TYPE_LIST) {
            o = createListObject();
            uint32_t n = readU32(&r);
            for (uint32_t i = 0; i < n && !r.err; i++) {
                sds v = readString(&r);
                if (r.err) break;
                listAddNodeTail(o->as.list, v);      /* preserves head..tail order*/
            }
            if (r.err) { freeObject(o); sdsfree(key); rc = -1; break; }
        } else if (op == RDB_TYPE_HASH) {
            o = createHashObject();
            uint32_t n = readU32(&r);
            for (uint32_t i = 0; i < n && !r.err; i++) {
                sds field = readString(&r);
                sds value = r.err ? NULL : readString(&r);
                if (r.err) { sdsfree(field); break; }
                dictReplace(o->as.hash, field, value);
            }
            if (r.err) { freeObject(o); sdsfree(key); rc = -1; break; }
        } else {
            serverLog("RDB: unknown type byte 0x%02x", (unsigned)op);
            sdsfree(key); rc = -1; break;
        }

        dbAdd(key, o);                               /* dict takes both           */
        if (expire >= 0) setExpire(key, expire);
    }

    sdsfree(data);
    return rc;
}

/* ---------------------------------------------------------------------------
 * AOF: append-only command logging.
 * ------------------------------------------------------------------------- */
void aofInit(void)
{
    server.aof_buf        = sdsempty();
    server.aof_last_fsync = 0;
    server.aof_fd         = -1;
    if (!server.aof_enabled) return;

    /* O_APPEND makes every write atomically land at end-of-file, so even if the
     * process is racing with itself the log never interleaves partial records. */
    server.aof_fd = open(server.aof_filename,
                         O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (server.aof_fd < 0) {
        serverLog("AOF: cannot open %s; disabling AOF", server.aof_filename);
        server.aof_enabled = 0;
    }
}

/* Encode a command as a RESP multibulk array and queue it in aof_buf. The bytes
 * are flushed to the file (and fsynced per policy) later by
 * flushAppendOnlyFile, so many commands within one event-loop tick coalesce
 * into a single write()/fsync(). */
void aofFeed(sds *argv, int argc)
{
    if (!server.aof_enabled) return;                 /* no-op when AOF is off /
                                                        during load (temporarily) */
    server.aof_buf = sdscatprintf(server.aof_buf, "*%d\r\n", argc);
    for (int i = 0; i < argc; i++) {
        server.aof_buf = sdscatprintf(server.aof_buf, "$%zu\r\n", sdslen(argv[i]));
        server.aof_buf = sdscatlen(server.aof_buf, argv[i], sdslen(argv[i]));
        server.aof_buf = sdscatlen(server.aof_buf, "\r\n", 2);
    }
}

/* Write any buffered AOF bytes to the file and fsync according to policy.
 * `force` (used at shutdown) fsyncs regardless of the everysec timer. */
void flushAppendOnlyFile(int force)
{
    if (server.aof_fd < 0) return;
    if (sdslen(server.aof_buf) == 0) {
        if (!force) return;
    } else {
        if (writeAll(server.aof_fd, server.aof_buf, sdslen(server.aof_buf)) != 0) {
            serverLog("AOF: write failed: %s", strerror(errno));
            return;                                  /* keep buffer for a retry   */
        }
        /* Reset the buffer in place (shrink; no reallocation). */
        SDS_HDR(server.aof_buf)->len = 0;
        server.aof_buf[0] = '\0';
    }

    time_t now = time(NULL);
    int do_fsync = 0;
    if (force) do_fsync = 1;
    else if (server.aof_fsync == AOF_FSYNC_ALWAYS) do_fsync = 1;
    else if (server.aof_fsync == AOF_FSYNC_EVERYSEC &&
             now > server.aof_last_fsync) do_fsync = 1;

    if (do_fsync) {
        /* fsync forces the kernel page cache for this file out to stable media.
         * Without it, a power loss can lose "durable" writes still sitting in
         * RAM — the whole reason the fsync policy exists. */
        if (fsync(server.aof_fd) == 0) server.aof_last_fsync = now;
    }
}

/* Replay the AOF at startup by feeding its bytes through the ordinary command
 * parser attached to a throwaway "fake" client whose replies we discard. We
 * temporarily disable AOF logging so replaying commands does not re-log them. */
int loadAppendOnlyFile(const char *filename)
{
    int missing;
    sds data = slurpFile(filename, &missing);
    if (missing) return 0;                           /* no AOF yet: nothing to do  */
    if (data == NULL) return -1;

    /* A minimal client just for parsing/execution. fd = -1: it is never read
     * from or written to; command handlers append replies into buf, which we
     * throw away after loading. */
    client fake;
    memset(&fake, 0, sizeof(fake));
    fake.fd              = -1;
    fake.querybuf        = data;                     /* the whole file as input   */
    fake.bulklen         = -1;
    fake.pubsub_channels = dictCreate(&keyptrDictType);

    int saved_aof = server.aof_enabled;
    server.aof_enabled = 0;                          /* suppress re-logging       */
    processInputBuffer(&fake);                       /* execute every command     */
    server.aof_enabled = saved_aof;

    /* Tear down the fake client's transient state. */
    resetClientCommand(&fake);
    sdsfree(fake.querybuf);
    zfree(fake.buf);
    dictRelease(fake.pubsub_channels);

    serverLog("AOF loaded from %s", filename);
    return 0;
}
