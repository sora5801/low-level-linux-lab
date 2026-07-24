/* ===========================================================================
 * store.c — the state machine: a durable in-memory KV hash table.
 * ===========================================================================
 *
 * This is the "storage engine" Raft replicates onto. It is deliberately simple —
 * separate-chaining hash table in RAM, backed by an append-only write-ahead log
 * (store.wal) — because the *durability discipline*, not the data structure, is
 * the lesson. A production engine swaps the hash table for a B-tree or LSM tree
 * (../../02-systems-tools/13-embedded-db) and adds snapshots so the WAL doesn't
 * grow forever; those are additive and called out in the README's Scope section.
 *
 * THE DURABILITY CONTRACT (write-ahead logging)
 * ---------------------------------------------
 * "Write-ahead" means: the log record hits stable storage BEFORE the in-memory
 * state is considered committed. So store_apply() always does, in this order:
 *
 *     1. append the record to store.wal        (write)
 *     2. fsync(store.wal)                        (make it durable)
 *     3. mutate the hash table                   (now safe to reflect it)
 *
 * If we crash between (2) and (3), recovery replays the record and redoes the
 * mutation — no data lost. If we crash during (1)/(2), the torn tail is detected
 * by its CRC on replay and discarded — no corruption. Doing (3) before (2) would
 * be the classic bug: acknowledge a write the disk never got, then lose it on
 * power failure.
 *
 * WHY fsync AND NOT JUST write
 * ----------------------------
 * write() only copies into the kernel page cache; the bytes can sit in RAM for
 * seconds before the block layer flushes them. A power cut in that window loses
 * an "acknowledged" write. fsync(fd) blocks until the device reports the data
 * (and the file size metadata) durable. It is expensive — a real fsync is often
 * the dominant cost of a write — which is exactly why databases batch many
 * logical writes into one fsync (group commit). We fsync per apply for clarity.
 * ===========================================================================
 */
#include "db.h"

#include <unistd.h>    /* write, fsync, close, ftruncate                        */
#include <fcntl.h>     /* open, O_* flags                                       */
#include <string.h>    /* memcpy, memcmp, strlen                                */
#include <stdlib.h>    /* malloc, free                                          */
#include <errno.h>
#include <stdio.h>     /* snprintf                                              */

/* --------------------------------------------------------------------------
 * Hashing. FNV-1a: a fast, well-dispersing non-cryptographic hash. We cache the
 * full 32-bit hash in each entry so a bucket walk compares cheap integers before
 * ever touching memcmp on the key bytes.
 * ------------------------------------------------------------------------ */
static uint32_t fnv1a(const char *k, uint32_t n)
{
    uint32_t h = 2166136261u;                  /* FNV offset basis (32-bit)      */
    for (uint32_t i = 0; i < n; i++) {
        h ^= (uint8_t)k[i];                    /* xor the byte in first (1a)     */
        h *= 16777619u;                        /* then multiply by the FNV prime */
    }
    return h;
}

/* Bucket index from a hash. DB_HASH_BUCKETS is a power of two, so masking with
 * (buckets-1) is the same as % buckets but is a single AND instruction. */
static inline uint32_t bucket_of(uint32_t hash)
{
    return hash & (DB_HASH_BUCKETS - 1);
}

/* Find the entry for (key,klen) in its bucket, or NULL. Also outputs the address
 * of the pointer that links to it (**prevp), so callers can unlink in O(1). */
static struct kv_entry *find(struct store *s, const char *key, uint32_t klen,
                             uint32_t hash, struct kv_entry ***prevp)
{
    struct kv_entry **pp = &s->buckets[bucket_of(hash)];
    for (struct kv_entry *e = *pp; e; pp = &e->next, e = e->next) {
        /* Compare the cheap cached hash and length before the byte compare. */
        if (e->hash == hash && e->klen == klen && memcmp(e->key, key, klen) == 0) {
            if (prevp) *prevp = pp;
            return e;
        }
    }
    if (prevp) *prevp = pp;                     /* points at the tail NULL slot   */
    return NULL;
}

/* --------------------------------------------------------------------------
 * store_apply_mem — apply a command to the IN-MEMORY table only (no logging).
 * Used both by the durable path (after the log write) and by recovery replay
 * (where the record is already on disk). Returns 0, or -1 on OOM.
 * ------------------------------------------------------------------------ */
static int store_apply_mem(struct store *s, const struct command *c)
{
    uint32_t hash = fnv1a(c->key, c->klen);
    struct kv_entry **prev;
    struct kv_entry  *e = find(s, c->key, c->klen, hash, &prev);

    if (c->op == OP_DEL) {
        if (e) {                               /* delete: unlink + free          */
            *prev = e->next;                   /* prev now skips e                */
            free(e->key); free(e->val); free(e);
            s->count--;
        }
        return 0;                              /* deleting an absent key is a no-op */
    }

    /* OP_PUT. If the key exists, replace its value in place (reuse the node). */
    if (e) {
        char *nv = malloc(c->vlen ? c->vlen : 1); /* malloc(0) is impl-defined   */
        if (!nv) return -1;
        memcpy(nv, c->val, c->vlen);
        free(e->val);                          /* release the old value          */
        e->val = nv; e->vlen = c->vlen;
        return 0;
    }

    /* New key: allocate the node + owned copies of key and value. Ownership: the
     * entry owns both heap blocks until it is deleted or the store is closed. */
    e = malloc(sizeof *e);
    if (!e) return -1;
    e->key = malloc(c->klen);
    e->val = malloc(c->vlen ? c->vlen : 1);
    if (!e->key || !e->val) { free(e->key); free(e->val); free(e); return -1; }
    memcpy(e->key, c->key, c->klen);
    memcpy(e->val, c->val, c->vlen);
    e->klen = c->klen; e->vlen = c->vlen; e->hash = hash;

    /* Insert at the head of the bucket (O(1); order within a bucket is
     * irrelevant to correctness). */
    uint32_t b = bucket_of(hash);
    e->next = s->buckets[b];
    s->buckets[b] = e;
    s->count++;
    return 0;
}

/* wal_replay() calls this for every intact record during recovery. It mutates
 * only memory — the record is already durably on disk, so re-logging it would
 * duplicate it. The signature matches wal_replay's apply callback. */
static void replay_apply(void *ctx, const struct command *c)
{
    (void)store_apply_mem((struct store *)ctx, c);
}

/* --------------------------------------------------------------------------
 * store_open — open (or create) the WAL and rebuild the table by replaying it.
 * ------------------------------------------------------------------------ */
int store_open(struct store *s, const char *data_dir)
{
    memset(s, 0, sizeof *s);
    snprintf(s->wal_path, sizeof s->wal_path, "%s/store.wal", data_dir);

    /* O_RDWR: we both replay (read) and append (write). O_CREAT: first boot.
     * 0644: owner rw, others r. No O_APPEND — we manage the offset ourselves so
     * replay can seek to 0 then leave the fd at the end. O_CLOEXEC so a future
     * exec() (none here, but hygiene) doesn't leak the fd. */
    s->wal_fd = open(s->wal_path, O_RDWR | O_CREAT | O_CLOEXEC, 0644);
    if (s->wal_fd < 0) return -1;

    /* Recovery: replay every good record into the table, truncating a torn tail.
     * After this, wal_fd is positioned at end-of-good-log for appends. */
    long n = wal_replay(s->wal_fd, replay_apply, s);
    if (n < 0) { close(s->wal_fd); s->wal_fd = -1; return -1; }
    TR("store", "recovered %ld records from %s (%zu live keys)",
       n, s->wal_path, s->count);
    return 0;
}

/* --------------------------------------------------------------------------
 * store_apply — the durable write path (log-ahead, then mutate). See the file
 * header for the ordering contract. `sync` lets a bulk caller defer fsync (not
 * used by the current wiring, but the hook is where group-commit would go).
 * ------------------------------------------------------------------------ */
int store_apply(struct store *s, const struct command *c, bool sync)
{
    /* Encode the full record on the stack. Max size is bounded by DB_MAX_*. */
    uint8_t rec[8 + 1 + 4 + DB_MAX_KEY + 4 + DB_MAX_VAL];
    size_t n = wal_encode_record(rec, c);

    /* (1) Append the record. write() may be partial; loop until all bytes land.
     * Because no other writer shares this fd (single thread), the appends are
     * naturally ordered — record N is fully on disk before record N+1 starts. */
    size_t off = 0;
    while (off < n) {
        ssize_t w = write(s->wal_fd, rec + off, n - off);
        if (w < 0) { if (errno == EINTR) continue; return -1; }
        off += (size_t)w;
    }

    /* (2) Make it durable BEFORE we reflect it in memory. */
    if (sync && fsync(s->wal_fd) < 0) return -1;

    /* (3) Now it is safe to mutate the visible state. */
    return store_apply_mem(s, c);
}

/* Read path: borrow the current value. The returned pointer is valid only until
 * the next mutation of this key — callers copy it out (server.c does, before it
 * touches the buffer again). Returns 1 if found, 0 if not. */
int store_get(struct store *s, const char *key, uint32_t klen,
              const char **val, uint32_t *vlen)
{
    uint32_t hash = fnv1a(key, klen);
    struct kv_entry *e = find(s, key, klen, hash, NULL);
    if (!e) return 0;
    *val = e->val; *vlen = e->vlen;
    return 1;
}

/* Free every entry and close the WAL. After this the struct is inert. */
void store_close(struct store *s)
{
    for (uint32_t b = 0; b < DB_HASH_BUCKETS; b++) {
        struct kv_entry *e = s->buckets[b];
        while (e) {
            struct kv_entry *nx = e->next;
            free(e->key); free(e->val); free(e);
            e = nx;
        }
        s->buckets[b] = NULL;
    }
    if (s->wal_fd >= 0) { close(s->wal_fd); s->wal_fd = -1; }
    s->count = 0;
}
