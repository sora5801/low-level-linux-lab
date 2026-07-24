/* ===========================================================================
 * wal.c — the write-ahead log: the reason a crash can't corrupt this database.
 * ===========================================================================
 *
 * THE ONE RULE (write-ahead logging, aka the "WAL invariant")
 * -----------------------------------------------------------
 *      A change to a page's HOME location on disk must never reach the platter
 *      before a durable copy of that change exists in the log.
 *
 * We enforce it with a strict, un-negotiable ordering at commit time:
 *
 *      1. pwrite every dirty page into the log as a "frame".
 *      2. fdatasync(WAL)          <-- THE BARRIER. Blocks until the log,
 *                                     including the commit frame, is on stable
 *                                     storage. Nothing below happens until it
 *                                     returns.
 *      3. pwrite each dirty page into its HOME block in the data file.
 *      4. fdatasync(data file).
 *      5. reset the log to a fresh, empty generation.
 *
 * Because of step 2, at every possible crash point the database is recoverable:
 *
 *   crash in 1        : the log has no valid commit frame for this txn, so
 *                       recovery discards it. The data file was never touched.
 *                       -> the transaction atomically did-not-happen.
 *   crash in 3 or 4   : the log HAS a valid commit frame, so recovery replays
 *                       every frame into the home blocks — redoing a checkpoint
 *                       that a crash interrupted. Redo is idempotent (we write
 *                       whole page images), so replaying a half-done checkpoint
 *                       simply finishes it. -> the transaction is durable.
 *   crash in 5        : same as above; replay re-applies an already-applied txn
 *                       harmlessly, then we reset again.
 *
 * WHY fdatasync AND NOT fsync
 * ---------------------------
 * fsync flushes file data AND all inode metadata (mtime, atime, ...). fdatasync
 * flushes file data plus ONLY the metadata needed to read that data back — most
 * importantly the file SIZE. Our WAL only ever grows by appending, and our data
 * file grows by pwrite; in both cases the size is the one metadata bit that
 * matters, and fdatasync guarantees it. Skipping the timestamp flush saves an
 * extra metadata write per commit. (On a platform without fdatasync, fsync is a
 * correct — just slightly slower — substitute; see the Makefile note.)
 *
 * CHECKPOINT-ON-COMMIT (a teaching simplification, stated honestly)
 * -----------------------------------------------------------------
 * A production WAL (SQLite, Postgres) lets many transactions accumulate in the
 * log and checkpoints them in the background, using a WAL-index so readers can
 * find the newest version of a page (which may still be in the log). We instead
 * checkpoint at the end of EVERY commit (steps 3-5 above). That keeps the data
 * file always-authoritative — the pager never has to consult the log during
 * normal reads — at the cost of two fdatasyncs per write. The durability
 * reasoning is identical and easier to see; the batching is the optimization we
 * leave for "Going further".
 *
 * WAL FILE FORMAT (all little-endian)
 * -----------------------------------
 *   header (32 bytes, once at offset 0):
 *     0  u32 magic      "WAL1"
 *     4  u32 page_size  must match ours
 *     8  u32 seq        generation counter (bumped on every reset)
 *    12  u32 salt1      generation salt: every frame must carry this exact value
 *    16  u32 reserved x3
 *    28  u32 hdr_crc    crc32 of bytes [0,28)
 *
 *   frame (24-byte header + one PAGE_SIZE image), repeated:
 *     0  u32 page_no    home page this image belongs to
 *     4  u32 db_size    0 for a normal frame; >0 marks a COMMIT and gives the
 *                       database size (in pages) as of this transaction
 *     8  u32 salt1      must equal the header salt1 (else it's a stale frame
 *                       left over from an older generation -> stop replay)
 *    12  u32 reserved
 *    16  u32 crc        crc32 over frame bytes [0,16) followed by the page image
 *    20  u32 reserved
 *    24  ... PAGE_SIZE bytes of page image ...
 *
 * The salt makes an old frame from a previous generation instantly invalid
 * without having to erase the file: after a reset we write a new salt, so any
 * leftover bytes past the new (short) end of the log fail the salt check.
 * ===========================================================================
 */
#include "db.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/* Robust positioned-I/O loops, shared with pager.c. */
extern int db_pread_full(int fd, void *buf, size_t n, uint64_t off);
extern int db_pwrite_full(int fd, const void *buf, size_t n, uint64_t off);

#define WAL_MAGIC   0x57414C31u   /* "WAL1" */
#define WAL_HDR     32u           /* size of the log header                    */
#define FRAME_HDR   24u           /* size of one frame header                  */
#define FRAME_SZ    (FRAME_HDR + PAGE_SIZE)   /* header + page image           */

/* Frame-header field offsets (within a frame). */
#define F_PAGENO    0u
#define F_DBSIZE    4u
#define F_SALT1     8u
#define F_RESV      12u
#define F_CRC       16u
#define F_RESV2     20u

/* ---------------------------------------------------------------------------
 * frame_crc — checksum of a frame: the first 16 bytes of the frame header
 * (page_no, db_size, salt1, reserved — i.e. everything up to but NOT including
 * the CRC field) followed by the whole page image. Computed as one streaming
 * CRC so a torn write anywhere in the 4120-byte frame is caught.
 * --------------------------------------------------------------------------- */
static uint32_t frame_crc(const uint8_t *fhdr, const uint8_t *page)
{
    uint32_t c = crc32_init();
    c = crc32_update(c, fhdr, 16);          /* [0,16): page_no,db_size,salt1,resv */
    c = crc32_update(c, page, PAGE_SIZE);   /* the payload                        */
    return crc32_final(c);
}

/* A small, dependency-free source of a per-generation salt. It only needs to
 * differ from the PREVIOUS generation's salt so leftover frames are rejected;
 * cryptographic quality is irrelevant. We mix wall-clock time, the pid, and the
 * old salt/seq so consecutive resets never collide. */
static uint32_t make_salt(uint32_t prev_salt, uint32_t seq)
{
    uint32_t t = (uint32_t)time(NULL);
    uint32_t p = (uint32_t)getpid();
    uint32_t s = t * 2654435761u + (p << 16) + (seq * 40503u) + (prev_salt + 1u);
    return s ? s : 1u;                       /* never 0 (keeps things obvious)    */
}

/* Write a fresh WAL header describing an empty log of `seq`/`salt1`. Does NOT
 * sync — callers that need durability (wal_reset) sync afterwards. */
static int wal_write_header(DB *db, uint32_t seq, uint32_t salt1)
{
    uint8_t h[WAL_HDR];
    memset(h, 0, sizeof h);
    wr32(h + 0,  WAL_MAGIC);
    wr32(h + 4,  PAGE_SIZE);
    wr32(h + 8,  seq);
    wr32(h + 12, salt1);
    wr32(h + 28, crc32(h, 28));              /* checksum the first 28 bytes       */
    return db_pwrite_full(db->wal_fd, h, WAL_HDR, 0);
}

/* ---------------------------------------------------------------------------
 * wal_reset — begin a brand-new, empty log generation.
 * Bump seq, pick a new salt (invalidating every existing frame), truncate the
 * file back to just its header, then fdatasync so the reset itself is durable.
 * After this the append cursor sits right after the header.
 * --------------------------------------------------------------------------- */
static int wal_reset(DB *db)
{
    uint32_t seq   = db->wal_seq + 1u;
    uint32_t salt1 = make_salt(db->wal_salt1, seq);

    if (wal_write_header(db, seq, salt1) != 0) return -1;
    if (ftruncate(db->wal_fd, WAL_HDR) != 0)   return -1;  /* drop old frames     */
    if (fdatasync(db->wal_fd) != 0)            return -1;  /* make the reset stick */

    db->wal_seq   = seq;
    db->wal_salt1 = salt1;
    db->wal_off   = WAL_HDR;                 /* next frame appends here           */
    return 0;
}

/* ---------------------------------------------------------------------------
 * wal_open — open (or create) the log file and learn the current generation.
 * Called by db_open BEFORE recovery. If the file is empty or its header is
 * bad/foreign, we lay down a fresh header; otherwise we adopt the existing
 * seq/salt so wal_recover can validate that generation's frames.
 * --------------------------------------------------------------------------- */
int wal_open(DB *db)
{
    db->wal_fd = open(db->wal_path, O_RDWR | O_CREAT, 0644);
    if (db->wal_fd < 0) return -1;

    struct stat st;
    if (fstat(db->wal_fd, &st) != 0) return -1;

    if ((uint64_t)st.st_size < WAL_HDR) {
        /* Brand-new (or truncated) log: install generation 0. */
        db->wal_seq = 0;
        db->wal_salt1 = 0;
        if (wal_reset(db) != 0) return -1;   /* writes hdr+truncate+sync, seq->1  */
        return 0;
    }

    /* Existing log: read and validate the header. */
    uint8_t h[WAL_HDR];
    if (db_pread_full(db->wal_fd, h, WAL_HDR, 0) != 0) return -1;
    int ok = rd32(h + 0) == WAL_MAGIC
          && rd32(h + 4) == PAGE_SIZE
          && rd32(h + 28) == crc32(h, 28);
    if (!ok) {
        /* Unreadable/foreign header: we can't trust any frame, so nothing was
         * durably committed through it. Start a clean generation. */
        db->wal_seq = 0;
        db->wal_salt1 = 0;
        return wal_reset(db);
    }
    db->wal_seq   = rd32(h + 8);
    db->wal_salt1 = rd32(h + 12);
    db->wal_off   = (uint64_t)st.st_size;    /* append after existing frames      */
    return 0;
}

/* ---------------------------------------------------------------------------
 * wal_append_frame — append one page image to the log (no sync here).
 * `db_size` is 0 for an ordinary frame and >0 on the transaction's final frame
 * to mark the commit and record the resulting database size.
 * --------------------------------------------------------------------------- */
static int wal_append_frame(DB *db, uint32_t pgno, uint32_t db_size,
                            const uint8_t *page)
{
    uint8_t frame[FRAME_SZ];
    memset(frame, 0, FRAME_HDR);
    wr32(frame + F_PAGENO, pgno);
    wr32(frame + F_DBSIZE, db_size);
    wr32(frame + F_SALT1,  db->wal_salt1);   /* stamp with the current generation */
    memcpy(frame + FRAME_HDR, page, PAGE_SIZE);
    wr32(frame + F_CRC, frame_crc(frame, frame + FRAME_HDR));

    if (db_pwrite_full(db->wal_fd, frame, FRAME_SZ, db->wal_off) != 0) return -1;
    db->wal_off += FRAME_SZ;
    return 0;
}

/* ---------------------------------------------------------------------------
 * wal_commit — make the current transaction (the dirty set) durable.
 * This is the function that implements the five-step ordering at the top of the
 * file. Returns 0 on success, -1 on any I/O error.
 * --------------------------------------------------------------------------- */
int wal_commit(DB *db)
{
    if (db->ndirty == 0) return 0;           /* read-only or no-op txn: nothing   */

    uint32_t db_size = meta_npages(db);      /* the DB size this commit publishes */

    /* --- Step 1: log every dirty page. The LAST one is the commit frame. ----- */
    for (uint32_t i = 0; i < db->ndirty; i++) {
        Page *pg = db->cache[db->dirty[i]];
        page_finalize(pg->data);             /* stamp the page's own CRC now that
                                              *   its contents are final          */
        uint32_t mark = (i + 1 == db->ndirty) ? db_size : 0u;  /* commit on last  */
        if (wal_append_frame(db, pg->pgno, mark, pg->data) != 0) return -1;
    }

    /* --- Step 2: THE BARRIER. Log (incl. commit frame) hits stable storage. -- */
    if (fdatasync(db->wal_fd) != 0) return -1;

    /* Crash-injection hook (testing only): if KVDB_CRASH_AFTER_WAL is set, we
     * die HERE — right after the commit is durable in the log but BEFORE any
     * page reached its home block. This is the most dangerous instant, and the
     * one recovery must handle: reopening replays the WAL and the write is
     * intact. _Exit skips all cleanup, faithfully imitating a power loss. */
    if (getenv("KVDB_CRASH_AFTER_WAL") != NULL) _Exit(99);

    /* --- Step 3: only NOW may pages go to their home blocks (checkpoint). ---- */
    for (uint32_t i = 0; i < db->ndirty; i++) {
        Page *pg = db->cache[db->dirty[i]];
        if (db_pwrite_full(db->db_fd, pg->data, PAGE_SIZE,
                           (uint64_t)pg->pgno * PAGE_SIZE) != 0) return -1;
    }

    /* --- Step 4: make the data file durable. -------------------------------- */
    if (fdatasync(db->db_fd) != 0) return -1;

    /* --- Step 5: retire this log generation; future recovery starts clean. --- */
    if (wal_reset(db) != 0) return -1;

    /* Clear the dirty set: the pages now match their on-disk home blocks. */
    for (uint32_t i = 0; i < db->ndirty; i++)
        db->cache[db->dirty[i]]->dirty = 0;
    db->ndirty = 0;
    return 0;
}

/* ---------------------------------------------------------------------------
 * wal_recover — on open, finish/undo whatever a crash interrupted.
 * Two passes over the log (each keeps memory O(1)):
 *   pass 1 finds the byte just past the LAST valid commit frame;
 *   pass 2 replays every frame up to there into the home blocks, then sizes and
 *          fsyncs the data file. Everything after the last commit (a torn,
 *          uncommitted tail) is discarded. Finally we reset to a fresh log.
 * --------------------------------------------------------------------------- */
int wal_recover(DB *db)
{
    struct stat st;
    if (fstat(db->wal_fd, &st) != 0) return -1;
    uint64_t size = (uint64_t)st.st_size;
    if (size <= WAL_HDR) return 0;           /* empty log: nothing to replay      */

    uint32_t gen_salt = db->wal_salt1;       /* the generation wal_open() adopted */
    uint8_t frame[FRAME_SZ];

    /* ---- Pass 1: locate the end of the last committed transaction. --------- */
    uint64_t off = WAL_HDR;
    uint64_t last_commit_end = 0;
    uint32_t commit_dbsize = 0;
    while (off + FRAME_SZ <= size) {
        if (db_pread_full(db->wal_fd, frame, FRAME_SZ, off) != 0) break;
        if (rd32(frame + F_SALT1) != gen_salt) break;   /* stale generation -> stop */
        if (rd32(frame + F_CRC) != frame_crc(frame, frame + FRAME_HDR))
            break;                                       /* torn frame -> stop       */
        uint32_t dbsz = rd32(frame + F_DBSIZE);
        if (dbsz != 0) {                                 /* a commit boundary        */
            last_commit_end = off + FRAME_SZ;
            commit_dbsize   = dbsz;
        }
        off += FRAME_SZ;
    }

    if (last_commit_end == 0) {
        /* No transaction ever committed durably: discard the whole log. */
        return wal_reset(db);
    }

    /* ---- Pass 2: replay committed frames into their home blocks (redo). ---- */
    off = WAL_HDR;
    while (off < last_commit_end) {
        if (db_pread_full(db->wal_fd, frame, FRAME_SZ, off) != 0) return -1;
        uint32_t pgno = rd32(frame + F_PAGENO);
        /* Idempotent whole-page write: safe to repeat if we crash mid-replay. */
        if (db_pwrite_full(db->db_fd, frame + FRAME_HDR, PAGE_SIZE,
                           (uint64_t)pgno * PAGE_SIZE) != 0) return -1;
        off += FRAME_SZ;
    }

    /* Pin the data file to exactly the committed size, then make it durable. */
    if (ftruncate(db->db_fd, (long)((uint64_t)commit_dbsize * PAGE_SIZE)) != 0)
        return -1;
    if (fdatasync(db->db_fd) != 0) return -1;

    /* The committed state now lives in the data file; retire the log. */
    return wal_reset(db);
}
