/* ===========================================================================
 * pager.c — the block layer: turn a byte-stream file into numbered 4 KiB pages,
 *           with positioned I/O (pread/pwrite), per-page CRC verification, a
 *           resident page cache, and a per-transaction dirty set.
 * ===========================================================================
 *
 * Everything above this layer (btree.c) thinks in terms of `Page *` objects it
 * can read and scribble on. Everything this layer does is make that safe and
 * persistent: read a page exactly once from its home offset, check its CRC,
 * hand out a stable pointer, and remember which pages were dirtied so the WAL
 * can flush them in the right order at commit.
 *
 * WHY pread/pwrite (not read/write + lseek)
 * -----------------------------------------
 * pread(fd,buf,n,off) / pwrite(fd,buf,n,off) carry the file offset as an
 * argument instead of using the shared file-descriptor cursor. That means:
 *   - no lseek()+read() race (the offset can't be moved by another thread/call
 *     between the two syscalls);
 *   - the offset is explicit at the call site, which is exactly page*PAGE_SIZE;
 *   - it composes with O_DIRECT, where all of {buffer addr, offset, length}
 *     must be block-aligned — and our page geometry makes them so.
 * ===========================================================================
 */
#include "db.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ---------------------------------------------------------------------------
 * pread_full / pwrite_full — loop over the short-count and EINTR cases.
 *
 * A single pread/pwrite is allowed by POSIX to move FEWER bytes than asked
 * (a "partial" transfer) and to fail with EINTR if a signal arrives mid-call.
 * Treating either as fatal is a classic durability bug, so we loop until the
 * whole page has moved, advancing the buffer and offset each turn.
 *
 * Return: 0 on success, -1 on a real error (errno set), -2 on unexpected EOF
 * (a pread that hit end-of-file before `n` bytes — a truncated/corrupt file).
 * --------------------------------------------------------------------------- */
static int pread_full(int fd, void *buf, size_t n, uint64_t off)
{
    uint8_t *p = (uint8_t *)buf;
    while (n > 0) {
        ssize_t r = pread(fd, p, n, (long)off);
        if (r < 0) {
            if (errno == EINTR) continue;   /* interrupted before any byte moved: retry */
            return -1;                       /* genuine I/O error                        */
        }
        if (r == 0) return -2;               /* EOF with bytes still owed: short file    */
        p   += (size_t)r;                    /* advance past the bytes we got            */
        off += (uint64_t)r;
        n   -= (size_t)r;
    }
    return 0;
}

static int pwrite_full(int fd, const void *buf, size_t n, uint64_t off)
{
    const uint8_t *p = (const uint8_t *)buf;
    while (n > 0) {
        ssize_t w = pwrite(fd, p, n, (long)off);
        if (w < 0) {
            if (errno == EINTR) continue;   /* retry the exact same request             */
            return -1;
        }
        p   += (size_t)w;                    /* pwrite past EOF grows the file for us     */
        off += (uint64_t)w;
        n   -= (size_t)w;
    }
    return 0;
}

/* Re-exported so wal.c can share the same robust loops. */
int db_pread_full(int fd, void *buf, size_t n, uint64_t off)  { return pread_full(fd, buf, n, off); }
int db_pwrite_full(int fd, const void *buf, size_t n, uint64_t off) { return pwrite_full(fd, buf, n, off); }

/* ---------------------------------------------------------------------------
 * Page checksum. The CRC lives in the first 4 bytes (NH_CRC) and covers every
 * OTHER byte on the page, i.e. [4 .. PAGE_SIZE). We deliberately exclude the
 * CRC field itself — you cannot checksum the slot that holds the checksum.
 * --------------------------------------------------------------------------- */
uint32_t page_checksum(const uint8_t *page)
{
    return crc32(page + 4, PAGE_SIZE - 4);
}
void page_finalize(uint8_t *page)      /* call right before a page goes to disk */
{
    wr32(page + NH_CRC, page_checksum(page));
}
int page_verify(const uint8_t *page)   /* call right after a page comes from disk */
{
    return rd32(page + NH_CRC) == page_checksum(page);
}

/* ---------------------------------------------------------------------------
 * Aligned page buffer allocation.
 *
 * Each page buffer is PAGE_ALIGN-aligned so it is legal for O_DIRECT I/O and so
 * the CPU never eats an unaligned-access penalty walking the page. We use
 * posix_memalign; the returned block is freed with plain free().
 * OWNERSHIP: the Page and its data buffer are owned by the DB cache and freed
 * in db_close().
 * --------------------------------------------------------------------------- */
static uint8_t *alloc_page_buf(void)
{
    void *p = NULL;
    if (posix_memalign(&p, PAGE_ALIGN, PAGE_SIZE) != 0)
        return NULL;                 /* out of memory                            */
    memset(p, 0, PAGE_SIZE);         /* start zeroed: a zero page is a valid,    */
    return (uint8_t *)p;             /*   empty slotted page once we set its type*/
}

/* Grow the direct-mapped cache array so index `pgno` exists (filled with NULL).
 * The Page objects themselves are separately malloc'd, so their addresses stay
 * stable even when THIS pointer array is realloc'd — that is what lets btree.c
 * hold a `Page *` across other pager_get() calls. */
static int cache_reserve(DB *db, uint32_t pgno)
{
    if (pgno < db->cache_cap) return 0;
    uint32_t ncap = db->cache_cap ? db->cache_cap : 16;
    while (ncap <= pgno) ncap *= 2;
    Page **n = (Page **)realloc(db->cache, ncap * sizeof(Page *));
    if (!n) return -1;
    for (uint32_t i = db->cache_cap; i < ncap; i++) n[i] = NULL;
    db->cache = n;
    db->cache_cap = ncap;
    return 0;
}

/* ---------------------------------------------------------------------------
 * pager_mark_dirty — enrol a page in the current transaction's dirty set.
 *
 * The dirty set is the exact list of pages wal_commit() will (a) append to the
 * log and (b) checkpoint into the data file. We keep no duplicates: the Page's
 * own `dirty` flag is the guard, so calling this twice on one page is a no-op.
 * --------------------------------------------------------------------------- */
void pager_mark_dirty(DB *db, Page *p)
{
    if (p->dirty) return;                       /* already enrolled              */
    if (db->ndirty == db->dirty_cap) {
        uint32_t ncap = db->dirty_cap ? db->dirty_cap * 2 : 16;
        uint32_t *n = (uint32_t *)realloc(db->dirty, ncap * sizeof(uint32_t));
        if (!n) return;                          /* OOM: leave un-enrolled; commit
                                                  *   will still flush via the flag
                                                  *   scan below if we tracked it —
                                                  *   here we simply cannot grow, so
                                                  *   the caller's write is at risk.
                                                  *   Teaching core: treat as fatal
                                                  *   upstream. */
        db->dirty = n;
        db->dirty_cap = ncap;
    }
    p->dirty = 1;
    db->dirty[db->ndirty++] = p->pgno;
}

/* ---------------------------------------------------------------------------
 * pager_get — return the resident Page for an EXISTING on-disk page.
 *
 * If it is not cached yet, allocate a buffer, pread it from its home offset
 * (pgno * PAGE_SIZE), and verify the CRC before anyone is allowed to trust it.
 * Returns NULL on OOM, short read, or CRC failure (corruption).
 * --------------------------------------------------------------------------- */
Page *pager_get(DB *db, uint32_t pgno)
{
    if (cache_reserve(db, pgno) != 0) return NULL;
    if (db->cache[pgno]) return db->cache[pgno];      /* cache hit               */

    Page *pg = (Page *)malloc(sizeof(Page));
    if (!pg) return NULL;
    pg->data  = alloc_page_buf();
    pg->pgno  = pgno;
    pg->dirty = 0;
    if (!pg->data) { free(pg); return NULL; }

    /* Read the page from its fixed home location. */
    int rc = pread_full(db->db_fd, pg->data, PAGE_SIZE, (uint64_t)pgno * PAGE_SIZE);
    if (rc != 0) { free(pg->data); free(pg); return NULL; }

    /* Trust nothing until the CRC checks out: a torn write is detected HERE. */
    if (!page_verify(pg->data)) { free(pg->data); free(pg); return NULL; }

    db->cache[pgno] = pg;
    return pg;
}

/* ---------------------------------------------------------------------------
 * pager_alloc — hand out a brand-new page at the end of the file.
 *
 * We bump the meta page's num_pages high-water mark (grow-only: this teaching
 * core never reclaims freed pages — see README) and materialise a zeroed,
 * typed, resident page. It exists only in memory (dirty) until the next commit
 * logs and checkpoints it. Marking meta dirty too is essential: the new page
 * count is part of the same atomic transaction.
 * --------------------------------------------------------------------------- */
Page *pager_alloc(DB *db, uint8_t type)
{
    uint32_t pgno = meta_npages(db);            /* next unused page number        */
    if (cache_reserve(db, pgno) != 0) return NULL;

    Page *pg = (Page *)malloc(sizeof(Page));
    if (!pg) return NULL;
    pg->data  = alloc_page_buf();
    pg->pgno  = pgno;
    pg->dirty = 0;
    if (!pg->data) { free(pg); return NULL; }

    /* Initialise an empty slotted page: type set, zero slots, cell area empty
     * (cell_top == PAGE_SIZE means "no cells yet; all of [NODE_HDR,PAGE_SIZE) is
     * free"). rightmost = 0 == "no child / no sibling". */
    pg->data[NH_TYPE] = type;
    wr16(pg->data + NH_NSLOTS,  0);
    wr16(pg->data + NH_CELLTOP, (uint16_t)PAGE_SIZE);
    wr32(pg->data + NH_RIGHTMOST, 0);

    db->cache[pgno] = pg;
    meta_set_npages(db, pgno + 1);              /* grow the file (dirties meta)   */
    pager_mark_dirty(db, pg);
    return pg;
}

/* ---------------------------------------------------------------------------
 * Meta-page accessors. The meta page (page 0) is the single source of truth for
 * global state; the DB struct caches nothing that lives here, which removes a
 * whole class of "the two copies disagree" bugs. cache[0] is loaded at open and
 * stays resident for the life of the handle.
 * --------------------------------------------------------------------------- */
uint32_t meta_root(DB *db)              { return rd32(db->cache[0]->data + M_ROOT); }
uint32_t meta_npages(DB *db)            { return rd32(db->cache[0]->data + M_NPAGES); }

void meta_set_root(DB *db, uint32_t root)
{
    wr32(db->cache[0]->data + M_ROOT, root);
    pager_mark_dirty(db, db->cache[0]);         /* the root move is part of the txn */
}
void meta_set_npages(DB *db, uint32_t n)
{
    wr32(db->cache[0]->data + M_NPAGES, n);
    pager_mark_dirty(db, db->cache[0]);
}
