/* ===========================================================================
 * persist.c — durable state with crash-safe fsync ordering.
 * ===========================================================================
 *
 * WHY THIS FILE IS THE HEART OF RAFT'S SAFETY
 * -------------------------------------------
 * Raft's paper marks three pieces of state "persistent" and requires them to be
 * "updated on stable storage BEFORE responding to RPCs": currentTerm, votedFor,
 * and log[]. That word "before" is a happens-before edge across a power failure,
 * and honoring it is entirely a story about the ORDER of write(2), fsync(2), and
 * rename(2). Get the order wrong and a single crash can violate Raft's core
 * invariants:
 *
 *   - votedFor not durable  -> a node reboots, "forgets" it already voted this
 *     term, and votes AGAIN for a different candidate. Two leaders in one term.
 *   - a log entry counted as replicated but not durable -> the leader commits it
 *     on a majority, a follower in that majority crashes and comes back missing
 *     the entry, and a later leader can overwrite a committed index. Applied
 *     state diverges — the exact failure Raft exists to prevent.
 *
 * THE DURABLE-RENAME DANCE (used for both the state and snapshot files):
 *
 *     1. write the ENTIRE new file contents to a sibling temp file  (state.tmp)
 *     2. fsync(tmp_fd)          <- the bytes are now physically on the medium
 *     3. rename(tmp, final)     <- atomic swap: a reader sees old XOR new file,
 *                                  never a half-written one (POSIX guarantees
 *                                  rename over an existing name is atomic)
 *     4. fsync(dir_fd)          <- make the DIRECTORY ENTRY durable, so the
 *                                  rename itself survives a crash
 *
 * Each fsync closes a specific crash window; the inline comments below name the
 * window each one closes. We rewrite the whole file every time rather than
 * appending to a WAL — O(log size) per persist. That is deliberately simple:
 * snapshotting keeps the log bounded, so the cost stays small, and the reader
 * gets one obviously-correct code path instead of an append+truncate+recovery
 * state machine. The README's "Going further" says what production does instead.
 *
 * INTEGRITY: every file ends with a CRC-32 over its own bytes. A rename is
 * atomic, so the live file is never torn — but a disk can still rot a sector, so
 * we verify on load and refuse corrupt input rather than feeding garbage into
 * the state machine. The CRC covers the classic "did the bytes I read back match
 * the bytes I wrote" check that all durable formats need.
 *
 * ENDIANNESS: fields are encoded LITTLE-ENDIAN explicitly (put_u64/get_u64), not
 * by memcpy'ing a struct, so the on-disk format is defined independent of the
 * host and byte offsets are exact and inspectable with `xxd`.
 *
 * Platform: Linux / WSL2 (POSIX open/write/fsync/rename, and fsync on a
 * directory fd). Not portable to native Windows; that is expected for this lab.
 * =========================================================================== */

#include "raft.h"

#include <fcntl.h>     /* open, O_* flags                                        */
#include <unistd.h>    /* write, read, fsync, close, unlink                      */
#include <string.h>    /* memcpy, memcmp                                         */
#include <stdlib.h>    /* malloc, free, realloc                                  */
#include <stdio.h>     /* snprintf                                               */
#include <errno.h>     /* errno, EINTR, ENOENT                                   */

/* ===========================================================================
 * CRC-32 (IEEE 802.3 / zlib variant): reflected input & output, polynomial
 * 0xEDB88820 (the bit-reversed form of 0x04C11DB7), initial 0xFFFFFFFF, final
 * XOR 0xFFFFFFFF. Computed bitwise (no 256-entry table) because these files are
 * tiny and the loop is more instructive than a precomputed table: for each byte
 * we XOR it into the low bits of the register and, for each of its 8 bits, shift
 * right and conditionally XOR the polynomial — polynomial long division in GF(2).
 * =========================================================================== */
uint32_t crc32_ieee(const void *data, size_t len)
{
    const unsigned char *p = (const unsigned char *)data;
    uint32_t crc = 0xFFFFFFFFu;                 /* preset all-ones (detects leading zeros) */
    for (size_t i = 0; i < len; i++) {
        crc ^= p[i];                            /* fold the next message byte in  */
        for (int b = 0; b < 8; b++) {
            /* If the low bit is set, subtract (== XOR) the reflected polynomial
             * after shifting; else just shift. The -(crc & 1) turns the low bit
             * into an all-ones/all-zeros mask, making this branch-free. */
            crc = (crc >> 1) ^ (0xEDB88820u & (uint32_t)(-(int32_t)(crc & 1)));
        }
    }
    return crc ^ 0xFFFFFFFFu;                    /* final inversion                */
}

/* ---- Little-endian fixed-width encode/decode. The cursor is advanced by the
 * caller; each helper writes/reads exactly its width at *pp and bumps it. Byte
 * order is pinned so the format is host-independent and offsets are exact. ---*/
static void put_u16(unsigned char **pp, uint16_t v)
{
    (*pp)[0] = (unsigned char)(v      );
    (*pp)[1] = (unsigned char)(v >>  8);
    *pp += 2;
}
static void put_u32(unsigned char **pp, uint32_t v)
{
    for (int i = 0; i < 4; i++) (*pp)[i] = (unsigned char)(v >> (8 * i));
    *pp += 4;
}
static void put_u64(unsigned char **pp, uint64_t v)
{
    for (int i = 0; i < 8; i++) (*pp)[i] = (unsigned char)(v >> (8 * i));
    *pp += 8;
}
static uint16_t get_u16(const unsigned char **pp)
{
    uint16_t v = (uint16_t)((*pp)[0] | ((*pp)[1] << 8));
    *pp += 2; return v;
}
static uint32_t get_u32(const unsigned char **pp)
{
    uint32_t v = 0;
    for (int i = 0; i < 4; i++) v |= (uint32_t)(*pp)[i] << (8 * i);
    *pp += 4; return v;
}
static uint64_t get_u64(const unsigned char **pp)
{
    uint64_t v = 0;
    for (int i = 0; i < 8; i++) v |= (uint64_t)(*pp)[i] << (8 * i);
    *pp += 8; return v;
}

/* ---------------------------------------------------------------------------
 * write_all — write exactly `len` bytes, coping with the two ways write(2) can
 * return "less than everything": a short write (the kernel accepted only part)
 * and EINTR (a signal interrupted the call before any byte moved). Both are
 * normal, not errors; we simply advance and retry. Any other errno is a real
 * failure. Every durable format needs this loop — a bare write() that ignores
 * the return value is a latent truncation bug.
 * --------------------------------------------------------------------------- */
static int write_all(int fd, const void *buf, size_t len)
{
    const unsigned char *p = (const unsigned char *)buf;
    size_t off = 0;
    while (off < len) {
        ssize_t w = write(fd, p + off, len - off);
        if (w < 0) {
            if (errno == EINTR) continue;   /* interrupted before writing: retry  */
            return -1;                       /* genuine I/O error                  */
        }
        off += (size_t)w;                    /* short write: advance past what took*/
    }
    return 0;
}

/* read_all — read up to `len` bytes; returns the count actually read (may be
 * short at EOF) or -1. Same EINTR/partial handling as write_all. */
static ssize_t read_all(int fd, void *buf, size_t len)
{
    unsigned char *p = (unsigned char *)buf;
    size_t off = 0;
    while (off < len) {
        ssize_t r = read(fd, p + off, len - off);
        if (r < 0) { if (errno == EINTR) continue; return -1; }
        if (r == 0) break;                   /* EOF                               */
        off += (size_t)r;
    }
    return (ssize_t)off;
}

/* ---------------------------------------------------------------------------
 * durable_write — the crash-safe file replace used for BOTH files. Writes `buf`
 * to `<dir>/<name>.tmp`, fsyncs it, atomically renames it over `<dir>/<name>`,
 * then fsyncs the directory so the rename itself is durable. Returns 0 / -1.
 * --------------------------------------------------------------------------- */
static int durable_write(const char *dir, const char *name,
                         const void *buf, size_t len)
{
    char final_path[512], tmp_path[512];
    snprintf(final_path, sizeof(final_path), "%s/%s", dir, name);
    snprintf(tmp_path,   sizeof(tmp_path),   "%s/%s.tmp", dir, name);

    /* Create/truncate the temp file. 0644: owner rw, others r — ordinary data. */
    int fd = open(tmp_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return -1;

    if (write_all(fd, buf, len) != 0) { close(fd); return -1; }

    /* fsync #1: flush the temp file's DATA to the physical medium. Without this,
     * the rename below could publish a filename that points at a file whose
     * contents are still in the page cache and vanish on power loss. */
    if (fsync(fd) != 0) { close(fd); return -1; }
    if (close(fd) != 0) return -1;

    /* Atomic swap: after this returns, any reader opening `final_path` sees
     * either the complete old file or the complete new one — never a mix. This
     * is why we never write the live file in place: an in-place write could be
     * caught half-done by a crash, and there is no CRC recovery from that. */
    if (rename(tmp_path, final_path) != 0) return -1;

    /* fsync #2: the rename modified the DIRECTORY (it changed which inode the
     * name points to). File data being durable does not make the directory
     * entry durable — on many filesystems a crash here could roll the rename
     * back to the old file even though the new data is safely on disk. fsync on
     * the directory fd forces the metadata out and closes that window. */
    int dfd = open(dir, O_RDONLY);              /* a directory opened read-only    */
    if (dfd < 0) return -1;
    if (fsync(dfd) != 0) { close(dfd); return -1; }
    if (close(dfd) != 0) return -1;
    return 0;
}

/* File format tags. A magic + version at the head lets the loader reject a file
 * from an incompatible build instead of misparsing it. */
#define STATE_MAGIC  0x54464152u   /* "RAFT" little-endian                       */
#define SNAP_MAGIC   0x504E5352u   /* "RSNP" little-endian                       */
#define FMT_VERSION  1u

/* ===========================================================================
 * persist_save_state — encode {currentTerm, votedFor, snapshot base, log[]} and
 * durably replace the `state` file. This is called on EVERY change to persistent
 * state, and always completes (fsyncs and returns) BEFORE the caller sends the
 * RPC reply that depended on the change — that synchronous ordering is what
 * makes "persist before responding" true.
 * =========================================================================== */
int persist_save_state(struct raft_node *n)
{
    /* Upper bound on the encoded size: fixed header + per-entry worst case. */
    size_t max = 4 + 4                 /* magic + version                        */
               + 8 + 4                 /* current_term + voted_for               */
               + 8 + 8                 /* snap_last_index + snap_last_term        */
               + 8                     /* log count                              */
               + n->log.count * (8 + 8 + 2 + RAFT_CMD_MAX)
               + 4;                    /* trailing CRC                           */
    unsigned char *buf = (unsigned char *)malloc(max);
    if (!buf) return -1;

    unsigned char *p = buf;
    put_u32(&p, STATE_MAGIC);
    put_u32(&p, FMT_VERSION);
    put_u64(&p, n->current_term);
    put_u32(&p, (uint32_t)n->voted_for);        /* -1 encodes as 0xFFFFFFFF       */
    put_u64(&p, n->log.snap_last_index);
    put_u64(&p, n->log.snap_last_term);
    put_u64(&p, (uint64_t)n->log.count);
    for (size_t i = 0; i < n->log.count; i++) {
        struct log_entry *e = &n->log.entries[i];
        put_u64(&p, e->term);
        put_u64(&p, e->index);
        put_u16(&p, e->cmd_len);
        memcpy(p, e->cmd, e->cmd_len);          /* only the meaningful bytes      */
        p += e->cmd_len;
    }
    /* CRC over everything written so far, appended last so the loader can peel
     * it off, recompute over the head, and compare. */
    uint32_t crc = crc32_ieee(buf, (size_t)(p - buf));
    put_u32(&p, crc);

    int rc = durable_write(n->dir, "state", buf, (size_t)(p - buf));
    free(buf);
    return rc;
}

/* ===========================================================================
 * persist_save_snapshot — durably write the `snapshot` file (its own
 * lastIncluded{Index,Term} + the serialized state machine bytes). MUST be called
 * BEFORE the persist_save_state that records the compacted (shorter) log: the
 * state file will claim "the log starts after index X because a snapshot covers
 * everything up to X," so if that state became durable while the snapshot did
 * not, recovery would find a truncated log with no snapshot to fill the gap —
 * silent data loss. Ordering the two fsyncs (snapshot, THEN state) is the same
 * write-ahead discipline as a database's WAL-before-page rule.
 * =========================================================================== */
int persist_save_snapshot(struct raft_node *n, const char *data, uint32_t len)
{
    size_t max = 4 + 4 + 8 + 8 + 4 + len + 4;
    unsigned char *buf = (unsigned char *)malloc(max);
    if (!buf) return -1;

    unsigned char *p = buf;
    put_u32(&p, SNAP_MAGIC);
    put_u32(&p, FMT_VERSION);
    put_u64(&p, n->log.snap_last_index);
    put_u64(&p, n->log.snap_last_term);
    put_u32(&p, len);
    memcpy(p, data, len); p += len;
    uint32_t crc = crc32_ieee(buf, (size_t)(p - buf));
    put_u32(&p, crc);

    int rc = durable_write(n->dir, "snapshot", buf, (size_t)(p - buf));
    free(buf);
    return rc;
}

/* Read an entire file into a malloc'd buffer. Returns bytes read via *out_len
 * and the buffer via *out_buf (caller frees), or 0 with *out_buf==NULL if the
 * file does not exist (a fresh node), or -1 on a real error. */
static int slurp(const char *path, unsigned char **out_buf, size_t *out_len)
{
    *out_buf = NULL; *out_len = 0;
    int fd = open(path, O_RDONLY);
    if (fd < 0) return (errno == ENOENT) ? 0 : -1;   /* absent == fresh, not error */

    /* Read in growing chunks; we do not stat() first because the file is small
     * and this avoids a TOCTOU size race. read_all returns fewer bytes than we
     * asked for exactly when it reached EOF, which is our stop condition; if it
     * filled the buffer completely, there may be more, so we double and retry. */
    size_t cap = 4096, len = 0;
    unsigned char *buf = (unsigned char *)malloc(cap);
    if (!buf) { close(fd); return -1; }
    for (;;) {
        ssize_t r = read_all(fd, buf + len, cap - len);
        if (r < 0) { free(buf); close(fd); return -1; }
        len += (size_t)r;
        if (len < cap) break;                    /* short read => hit EOF, done    */
        size_t ncap = cap * 2;                    /* buffer full: grow and continue */
        unsigned char *nb = (unsigned char *)realloc(buf, ncap);
        if (!nb) { free(buf); close(fd); return -1; }
        buf = nb; cap = ncap;
    }
    close(fd);
    *out_buf = buf; *out_len = len;
    return 1;
}

/* Verify magic/version and the trailing CRC of a slurped file. Returns 0 if the
 * bytes are a valid frame we can parse, -1 otherwise (corruption / wrong file). */
static int verify_frame(const unsigned char *buf, size_t len, uint32_t magic)
{
    if (len < 8 + 4) return -1;                  /* too short for header + CRC     */
    const unsigned char *p = buf;
    if (get_u32(&p) != magic)      return -1;
    if (get_u32(&p) != FMT_VERSION) return -1;
    uint32_t stored = 0;                         /* last 4 bytes are the CRC       */
    const unsigned char *cp = buf + len - 4;
    stored = get_u32(&cp);
    uint32_t calc = crc32_ieee(buf, len - 4);
    return (stored == calc) ? 0 : -1;
}

/* ===========================================================================
 * persist_load — reconstruct a node's persistent state from disk on startup or
 * after a simulated crash. This is the ONLY path that populates term/votedFor/
 * log and the state machine on a restart: the whole point of a "reboot" test is
 * that in-memory state is thrown away and rebuilt from what was fsync'd.
 *
 * Order: load `state` (authoritative for term/votedFor/log and the snapshot
 * base index), then, if a snapshot base exists, load `snapshot` to rebuild the
 * state machine bytes. Volatile progress (commit_index/last_applied) is NOT on
 * disk; the caller resets it to the snapshot base and re-learns the rest from
 * the leader. Returns 0 on success (including "fresh node, nothing on disk").
 * =========================================================================== */
int persist_load(struct raft_node *n)
{
    char state_path[512], snap_path[512];
    snprintf(state_path, sizeof(state_path), "%s/state", n->dir);
    snprintf(snap_path,  sizeof(snap_path),  "%s/snapshot", n->dir);

    unsigned char *buf = NULL; size_t len = 0;
    int got = slurp(state_path, &buf, &len);
    if (got < 0) return -1;
    if (got == 0) return 0;                       /* no prior state: fresh node    */
    if (verify_frame(buf, len, STATE_MAGIC) != 0) { free(buf); return -1; }

    const unsigned char *p = buf + 8;             /* skip magic+version            */
    n->current_term        = get_u64(&p);
    n->voted_for           = (int)get_u32(&p);    /* 0xFFFFFFFF -> -1 (int)         */
    n->log.snap_last_index = get_u64(&p);
    n->log.snap_last_term  = get_u64(&p);
    uint64_t count         = get_u64(&p);

    /* Replace any in-memory log with the persisted one. */
    free(n->log.entries);
    n->log.entries = NULL; n->log.count = 0; n->log.cap = 0;
    if (count > 0) {
        n->log.entries = (struct log_entry *)malloc(count * sizeof(struct log_entry));
        if (!n->log.entries) { free(buf); return -1; }
        n->log.cap = (size_t)count;
    }
    for (uint64_t i = 0; i < count; i++) {
        struct log_entry *e = &n->log.entries[i];
        memset(e, 0, sizeof(*e));
        e->term    = get_u64(&p);
        e->index   = get_u64(&p);
        e->cmd_len = get_u16(&p);
        if (e->cmd_len > RAFT_CMD_MAX) { free(buf); return -1; } /* corrupt length */
        memcpy(e->cmd, p, e->cmd_len);
        p += e->cmd_len;
    }
    n->log.count = (size_t)count;
    free(buf);

    /* If the log has a snapshot base, the state machine content lives in the
     * snapshot file — load and replay it into kv. If there is no base, kv starts
     * empty and every command will be re-applied from the log by the caller. */
    if (n->log.snap_last_index > 0) {
        unsigned char *sbuf = NULL; size_t slen = 0;
        int sgot = slurp(snap_path, &sbuf, &slen);
        if (sgot <= 0) return -1;                 /* base claimed but missing: bad */
        if (verify_frame(sbuf, slen, SNAP_MAGIC) != 0) { free(sbuf); return -1; }
        const unsigned char *sp = sbuf + 8;
        uint64_t sidx = get_u64(&sp);
        (void)get_u64(&sp);                       /* last_included_term (checked via state) */
        uint32_t dlen = get_u32(&sp);
        /* Cross-check the two files agree on the snapshot base index. */
        if (sidx != n->log.snap_last_index) { free(sbuf); return -1; }
        kv_deserialize(&n->kv, (const char *)sp, dlen);
        free(sbuf);
    }
    return 0;
}
