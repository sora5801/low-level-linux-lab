/* ===========================================================================
 * db.c — open/close and the public get/put/del/scan, each wrapped as a durable
 *        transaction: mutate the page cache, then commit through the WAL.
 * ===========================================================================
 *
 * TRANSACTION MODEL (deliberately minimal)
 * ----------------------------------------
 * Every db_put / db_del is its own auto-committing transaction:
 *      bt_*()          modify pages in the cache and record them in the dirty set
 *      wal_commit()    log -> fdatasync -> checkpoint -> fdatasync -> reset log
 * If the B-tree operation fails partway (e.g. out of memory during a split),
 * tx_rollback() throws away the half-modified in-memory pages so the on-disk
 * database — which was never touched — remains the single source of truth.
 * db_get / db_scan are read-only and never commit.
 *
 * A real engine exposes explicit BEGIN/COMMIT so many operations share one
 * fsync; batching is the obvious next step (see README "Going further").
 * ===========================================================================
 */
#include "db.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/* Robust positioned writes, shared from pager.c (used to lay down a new file). */
extern int db_pwrite_full(int fd, const void *buf, size_t n, uint64_t off);

/* ---------------------------------------------------------------------------
 * init_new_db — lay down a brand-new database: a meta page (0) plus one empty
 * leaf root (1), each CRC-stamped, then fdatasync so the file exists durably
 * before we start caching pages out of it.
 * --------------------------------------------------------------------------- */
static int init_new_db(DB *db)
{
    uint8_t meta[PAGE_SIZE];
    memset(meta, 0, PAGE_SIZE);
    meta[NH_TYPE] = PT_META;
    wr16(meta + NH_NSLOTS, 0);
    wr16(meta + NH_CELLTOP, (uint16_t)PAGE_SIZE);
    wr32(meta + NH_RIGHTMOST, 0);
    wr32(meta + M_MAGIC,   DB_MAGIC);
    wr32(meta + M_VERSION, DB_VERSION);
    wr32(meta + M_PGSIZE,  PAGE_SIZE);
    wr32(meta + M_ROOT,    1);            /* root lives at page 1               */
    wr32(meta + M_NPAGES,  2);            /* meta(0) + root(1)                  */
    wr32(meta + M_RESV,    0);
    page_finalize(meta);                  /* stamp CRC last                     */

    uint8_t root[PAGE_SIZE];
    memset(root, 0, PAGE_SIZE);
    root[NH_TYPE] = PT_LEAF;              /* an empty leaf: 0 slots             */
    wr16(root + NH_NSLOTS, 0);
    wr16(root + NH_CELLTOP, (uint16_t)PAGE_SIZE);
    wr32(root + NH_RIGHTMOST, 0);         /* no sibling                         */
    page_finalize(root);

    if (db_pwrite_full(db->db_fd, meta, PAGE_SIZE, 0) != 0) return -1;
    if (db_pwrite_full(db->db_fd, root, PAGE_SIZE, PAGE_SIZE) != 0) return -1;
    if (fdatasync(db->db_fd) != 0) return -1;
    return 0;
}

/* ---------------------------------------------------------------------------
 * tx_rollback — discard the current transaction's in-memory changes.
 * Drop every dirty page from the cache (freeing it) so the next access re-reads
 * the untouched on-disk copy, then reload the meta page so the accessors work
 * again. Newly-allocated pages simply vanish (their page numbers, held only in
 * the in-memory meta we just dropped, are forgotten).
 * --------------------------------------------------------------------------- */
static void tx_rollback(DB *db)
{
    for (uint32_t i = 0; i < db->ndirty; i++) {
        uint32_t p = db->dirty[i];
        if (p < db->cache_cap && db->cache[p]) {
            free(db->cache[p]->data);
            free(db->cache[p]);
            db->cache[p] = NULL;
        }
    }
    db->ndirty = 0;
    (void)pager_get(db, 0);               /* reload meta (page 0) from disk     */
}

/* ---------------------------------------------------------------------------
 * db_open — open or create the database and recover from any prior crash.
 * Ordering matters: we recover the WAL into the data file BEFORE we cache any
 * page, so everything the pager later reads is already the committed state.
 * --------------------------------------------------------------------------- */
DB *db_open(const char *path)
{
    DB *db = (DB *)calloc(1, sizeof(DB));
    if (!db) return NULL;
    db->db_fd = -1;
    db->wal_fd = -1;

    db->path = strdup(path);
    size_t plen = strlen(path);
    db->wal_path = (char *)malloc(plen + 5);          /* "<path>-wal" + NUL      */
    if (!db->path || !db->wal_path) goto fail;
    memcpy(db->wal_path, path, plen);
    memcpy(db->wal_path + plen, "-wal", 5);           /* copies the trailing NUL */

    db->db_fd = open(path, O_RDWR | O_CREAT, 0644);
    if (db->db_fd < 0) goto fail;

    struct stat st;
    if (fstat(db->db_fd, &st) != 0) goto fail;
    int fresh = (st.st_size == 0);

    if (wal_open(db) != 0) goto fail;                 /* learn/reset WAL geometry */
    if (!fresh) {
        if (wal_recover(db) != 0) goto fail;          /* replay committed frames  */
    }
    if (fresh) {
        if (init_new_db(db) != 0) goto fail;          /* fabricate an empty DB    */
    }

    /* Pin the meta page (page 0) resident for the life of the handle — the
     * meta_* accessors assume cache[0] is present. */
    Page *meta = pager_get(db, 0);
    if (!meta) goto fail;
    if (rd32(meta->data + M_MAGIC) != DB_MAGIC ||
        rd32(meta->data + M_PGSIZE) != PAGE_SIZE) goto fail;   /* wrong format    */

    return db;

fail:
    db_close(db);
    return NULL;
}

/* Free every resident page, close both files, and free the handle. No flush is
 * needed: each mutation already committed durably. */
void db_close(DB *db)
{
    if (!db) return;
    if (db->cache) {
        for (uint32_t i = 0; i < db->cache_cap; i++) {
            if (db->cache[i]) { free(db->cache[i]->data); free(db->cache[i]); }
        }
        free(db->cache);
    }
    free(db->dirty);
    if (db->db_fd  >= 0) close(db->db_fd);
    if (db->wal_fd >= 0) close(db->wal_fd);
    free(db->path);
    free(db->wal_path);
    free(db);
}

/* ---- durable mutations ---------------------------------------------------- */
int db_put(DB *db, const void *key, uint16_t klen, const void *val, uint32_t vlen)
{
    int rc = bt_put(db, key, klen, val, vlen);
    if (rc < 0) { tx_rollback(db); return rc; }       /* leave disk untouched     */
    return wal_commit(db);                            /* make it durable          */
}

int db_del(DB *db, const void *key, uint16_t klen)
{
    int rc = bt_del(db, key, klen);                   /* 1 deleted, 0 absent      */
    if (rc < 0) { tx_rollback(db); return rc; }
    int c = wal_commit(db);                           /* no-op if nothing dirtied */
    return c < 0 ? c : rc;
}

/* ---- read paths (no WAL, no fsync) ---------------------------------------- */
int db_get(DB *db, const void *key, uint16_t klen,
           void *valbuf, uint32_t valcap, uint32_t *vallen_out)
{
    return bt_get(db, key, klen, valbuf, valcap, vallen_out);
}

int db_scan(DB *db, bt_scan_fn fn, void *ctx)
{
    return bt_scan(db, fn, ctx);
}
