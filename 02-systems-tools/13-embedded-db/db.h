/* ===========================================================================
 * db.h — on-disk format, public API, and the little-endian byte codecs for a
 *        crash-safe embedded key/value store (a B+-tree behind a write-ahead log).
 * ===========================================================================
 *
 * THE WHOLE STORE IN ONE PARAGRAPH
 * --------------------------------
 * Keys and values live in a B+-tree of fixed-size 4 KiB *pages* held in one
 * file (`<name>`). Every mutation is made durable by first appending the changed
 * pages to a *write-ahead log* (`<name>-wal`), fdatasync()-ing the log, and only
 * THEN writing the pages back into the main file. That ordering — "log hits the
 * platter before the home location it protects" — is the entire reason a power
 * cut can't corrupt the database. On open we replay the log to finish (or undo)
 * whatever a crash interrupted. Every page carries a CRC32 so a torn write is
 * *detected* rather than silently trusted.
 *
 * WHY THESE TYPES / THIS LAYOUT
 * -----------------------------
 *  - Fixed 4096-byte pages: matches the common filesystem block and x86 page
 *    size, so a single page write is the kernel's natural atomic-ish unit and
 *    an O_DIRECT buffer would be correctly aligned (see PAGE_ALIGN below).
 *  - Everything on disk is LITTLE-ENDIAN, encoded by hand through rd16/wr32/...
 *    We never memcpy a struct to disk: struct layout is compiler/ABI dependent
 *    (padding, endianness), and a database file must be byte-for-byte portable.
 *    The hand codecs make every byte offset explicit — which is exactly the part
 *    a reader needs to see.
 *
 * PLATFORM: Linux / WSL. It uses pread/pwrite/fdatasync/ftruncate. It builds and
 * runs anywhere those POSIX calls exist (Linux, WSL2, macOS with fsync in place
 * of fdatasync). The generated teaching assembly is host-independent.
 * ===========================================================================
 */
#ifndef EMBEDDED_DB_H
#define EMBEDDED_DB_H

#include <stddef.h>   /* size_t                                              */
#include <stdint.h>   /* uintN_t: we want exact widths on the wire           */

/* ---------------------------------------------------------------------------
 * Page geometry
 * ---------------------------------------------------------------------------
 * PAGE_SIZE is fixed at compile time. A production engine stores it in the meta
 * page and honours it at runtime; we hard-code 4096 to keep the arithmetic
 * legible. PAGE_ALIGN is the alignment we give every in-memory page buffer so
 * that the buffers are ready for O_DIRECT (which requires the buffer address,
 * the file offset, and the length to all be multiples of the device block
 * size — 512 is the classic floor; 4096 covers "advanced format" drives too).
 */
#define PAGE_SIZE   4096u
#define PAGE_ALIGN  4096u

/* ---------------------------------------------------------------------------
 * The node header — the first 16 bytes of EVERY page (meta, internal, leaf).
 * Byte offsets are load-bearing; the accessors below read exactly these.
 *
 *   off  size  field
 *    0    4    crc32     CRC32 of bytes [4 .. PAGE_SIZE); detects torn writes.
 *    4    1    type      PT_META / PT_INTERNAL / PT_LEAF
 *    5    1    flags     reserved (0)
 *    6    2    nslots    number of live cells = length of the slot array
 *    8    2    cell_top  offset where the cell content area begins (grows DOWN
 *                        from PAGE_SIZE); free space is [slots_end, cell_top).
 *   10    4    rightmost internal: page # of the child right of the last key;
 *                        leaf:     page # of the next leaf (sibling) for scans;
 *                        (0 means "none").
 *   14    2    reserved  (0)
 *
 * Immediately after the header (offset 16) comes the SLOT ARRAY: `nslots`
 * little-endian u16s, each the byte offset of a cell within this page, kept
 * sorted by key. Binary search runs over this array — see btree.c / asm/demo.c.
 * The cells themselves live in the content area and are stored in arbitrary
 * (allocation) order; only the slot array is sorted. This is the classic
 * "slotted page" used by SQLite, PostgreSQL heap pages, InnoDB, etc.
 */
#define NH_CRC        0u   /* u32 */
#define NH_TYPE       4u   /* u8  */
#define NH_FLAGS      5u   /* u8  */
#define NH_NSLOTS     6u   /* u16 */
#define NH_CELLTOP    8u   /* u16 */
#define NH_RIGHTMOST 10u   /* u32 */
#define NH_RESV      14u   /* u16 */
#define NODE_HDR     16u   /* header size == start of the slot array          */
#define SLOT_SZ       2u   /* each slot is one little-endian u16              */

/* Page type tags (stored at NH_TYPE). */
#define PT_META      1u
#define PT_INTERNAL  2u
#define PT_LEAF      3u

/* Usable bytes on a page for (slots + cells): everything after the header. */
#define PAGE_USABLE  (PAGE_SIZE - NODE_HDR)

/* ---------------------------------------------------------------------------
 * Meta page (page 0). It reuses the 16-byte node header (type = PT_META, so it
 * is CRC-protected like any other page) and stores the database's global state
 * in fixed fields right after the header:
 *
 *   off 16  u32 magic     identifies the file format ("MYDB")
 *   off 20  u32 version    format version
 *   off 24  u32 page_size  sanity-check against our compiled PAGE_SIZE
 *   off 28  u32 root       page # of the B-tree root
 *   off 32  u32 num_pages  total pages in the file (the high-water mark)
 *   off 36  u32 reserved   (was a freelist head; unused — see README "omits")
 */
#define M_MAGIC     16u
#define M_VERSION   20u
#define M_PGSIZE    24u
#define M_ROOT      28u
#define M_NPAGES    32u
#define M_RESV      36u

#define DB_MAGIC    0x4259444Du   /* 'M','Y','D','B' as a LE u32               */
#define DB_VERSION  1u

/* ---------------------------------------------------------------------------
 * Size limits. A cell must fit on one page (this teaching core has no overflow
 * pages), so we bound keys and values well under a page. A single leaf cell is
 * 6 bytes of header (u16 key_len + u32 val_len) plus the bytes, and needs one
 * 2-byte slot; the runtime check in btree.c is the real guard, these are the
 * advertised ceilings.
 */
#define MAX_KEY   1024u
#define MAX_VAL   3000u

/* ===========================================================================
 * Little-endian byte codecs.
 * ---------------------------------------------------------------------------
 * These are the ONLY way bytes cross the memory<->disk boundary. Doing it by
 * hand (rather than casting a pointer to uint32_t*) is deliberate: it is
 * endian-correct on any host and has no alignment requirement — a cell can
 * start at any byte offset in the page, and an unaligned `*(uint32_t*)p` is
 * undefined behaviour in C even on x86. Shifting bytes is always defined.
 * ===========================================================================
 */
static inline uint16_t rd16(const uint8_t *p) {
    /* byte 0 is the least-significant (little-endian) */
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}
static inline uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0]        | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static inline void wr16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)((v >> 8) & 0xFF);
}
static inline void wr32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)((v >> 8) & 0xFF);
    p[2] = (uint8_t)((v >> 16) & 0xFF);
    p[3] = (uint8_t)((v >> 24) & 0xFF);
}

/* ===========================================================================
 * CRC32 (crc32.c). Reflected IEEE 802.3 polynomial (0xEDB88320), the same
 * variant zlib/gzip/PNG use, so `crc32` of a page could be cross-checked with
 * standard tools. Exposed in streaming form so the WAL can checksum a frame
 * header and a page image in one running computation.
 * ===========================================================================
 */
uint32_t crc32_init(void);                                  /* seed = 0xFFFFFFFF */
uint32_t crc32_update(uint32_t crc, const void *buf, size_t n);
uint32_t crc32_final(uint32_t crc);                         /* final XOR         */
uint32_t crc32(const void *buf, size_t n);                  /* one-shot          */

/* ===========================================================================
 * In-memory objects.
 * ===========================================================================
 */

/* One resident page. `data` is a PAGE_ALIGN-aligned PAGE_SIZE buffer (see
 * pager.c) so it is O_DIRECT-ready. `dirty` means "modified since the last
 * commit; must be journaled before it may reach its home block". */
typedef struct Page {
    uint8_t  *data;   /* PAGE_SIZE bytes, PAGE_ALIGN-aligned                   */
    uint32_t  pgno;   /* this page's number (== file offset / PAGE_SIZE)       */
    int       dirty;  /* 1 if changed since last commit                        */
} Page;

/* The database handle. See the struct-field comments in db.c/pager.c for the
 * ownership and lifetime of each member. */
typedef struct DB {
    int        db_fd;      /* main data file                                   */
    int        wal_fd;     /* write-ahead log file                             */
    char      *path;       /* strdup'd data-file path (we own it)              */
    char      *wal_path;   /* strdup'd "<path>-wal" (we own it)                */

    /* Resident page cache: a direct map pgno -> Page*. This teaching pager keeps
     * every touched page resident (no eviction); a production pager bounds this
     * with an LRU and must never evict a dirty page ahead of the WAL. */
    Page     **cache;      /* cache[pgno]; NULL until first touched            */
    uint32_t   cache_cap;  /* length of the cache array                        */

    /* Dirty set for the current (implicit) transaction: the pages that must be
     * logged, then checkpointed, on the next commit. */
    uint32_t  *dirty;      /* page numbers, no duplicates (Page.dirty guards)  */
    uint32_t   ndirty;
    uint32_t   dirty_cap;

    /* WAL append state. salt1 tags the current log "generation"; bumping it on
     * reset makes every already-written frame instantly stale (see wal.c). */
    uint32_t   wal_salt1;
    uint32_t   wal_seq;
    uint64_t   wal_off;    /* current append offset (== WAL file size)         */
} DB;

/* ---- page-level checksum helpers (implemented in pager.c) ----------------- */
uint32_t page_checksum(const uint8_t *page);  /* CRC of [4 .. PAGE_SIZE)       */
void     page_finalize(uint8_t *page);        /* recompute & store the CRC     */
int      page_verify(const uint8_t *page);    /* 1 == CRC matches              */

/* ---- pager (pager.c) ------------------------------------------------------ */
Page *pager_get(DB *db, uint32_t pgno);   /* fetch (read+verify) an EXISTING page */
Page *pager_alloc(DB *db, uint8_t type);  /* grow the file by one fresh page      */
void  pager_mark_dirty(DB *db, Page *p);  /* enrol p in the current txn's dirty set*/

/* meta-page accessors (single source of truth lives in cache[0]->data) */
uint32_t meta_root(DB *db);
void     meta_set_root(DB *db, uint32_t root);
uint32_t meta_npages(DB *db);
void     meta_set_npages(DB *db, uint32_t n);

/* ---- write-ahead log (wal.c) ---------------------------------------------- */
int  wal_open(DB *db);                 /* open/create the log, learn its geometry  */
int  wal_recover(DB *db);              /* replay committed frames into the data file*/
int  wal_commit(DB *db);               /* THE durability path: log -> fsync -> data */

/* ---- B-tree (btree.c) ----------------------------------------------------- */
int  bt_get(DB *db, const void *key, uint16_t klen,
            void *valbuf, uint32_t valcap, uint32_t *vallen_out); /* 1 found,0 miss,<0 err */
int  bt_put(DB *db, const void *key, uint16_t klen,
            const void *val, uint32_t vlen);        /* insert or overwrite       */
int  bt_del(DB *db, const void *key, uint16_t klen);/* 1 deleted, 0 absent, <0 err*/

/* In-order scan: calls fn(key,klen,val,vlen,ctx) for every pair. */
typedef int (*bt_scan_fn)(const void *key, uint16_t klen,
                          const void *val, uint32_t vlen, void *ctx);
int  bt_scan(DB *db, bt_scan_fn fn, void *ctx);

/* ---- top-level API (db.c) ------------------------------------------------- */
DB  *db_open(const char *path);
void db_close(DB *db);
int  db_put(DB *db, const void *key, uint16_t klen, const void *val, uint32_t vlen);
int  db_get(DB *db, const void *key, uint16_t klen,
            void *valbuf, uint32_t valcap, uint32_t *vallen_out);
int  db_del(DB *db, const void *key, uint16_t klen);
int  db_scan(DB *db, bt_scan_fn fn, void *ctx);

#endif /* EMBEDDED_DB_H */
