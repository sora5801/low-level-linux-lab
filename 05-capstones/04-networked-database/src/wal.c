/* ===========================================================================
 * wal.c — the write-ahead log record format, and crash-recovery replay.
 * ===========================================================================
 *
 * This file owns the on-disk (and, via the shared command encoder, on-wire)
 * *record format* and the recovery algorithm. It is the storage-engine WAL — the
 * embedded-db lesson (../../02-systems-tools/13-embedded-db) — but it is written
 * to be format-agnostic: it takes an fd and an apply() callback, so store.c uses
 * it for state and the same primitives serialize Raft entries.
 *
 * THE RECORD FORMAT (little-endian, self-describing, checksummed)
 * --------------------------------------------------------------
 *
 *     +--------+--------+============================+
 *     | reclen |  crc32 |        command body        |
 *     | u32 LE | u32 LE |     (reclen bytes)         |
 *     +--------+--------+============================+
 *      \_____ 8-byte header _____/ \___ payload ___/
 *
 *   reclen = byte length of the command body that follows.
 *   crc32  = CRC-32/IEEE over the command body (NOT the header) — see crc32.c.
 *   body   = [u8 op][u32 klen][key bytes][u32 vlen][val bytes].
 *
 * WHY THIS SHAPE
 * --------------
 * - LENGTH FRAMING FIRST. A log/stream has no natural record boundaries; the
 *   reader must be *told* how many bytes the next record occupies before it can
 *   read it. The u32 length prefix is that frame. (This is the exact problem the
 *   peer transport also solves — one length prefix per message — which is why
 *   asm/demo.c distills the framing+CRC math.)
 * - CRC AFTER LENGTH, BEFORE BODY. On replay we read the 8-byte header, learn
 *   reclen, read exactly that many body bytes, then verify. A torn write shows up
 *   as either a short read (fewer than reclen body bytes remain) or a CRC
 *   mismatch — both mean "the log ends here; truncate."
 * - APPEND-ONLY. Writes only ever extend the file; we never overwrite committed
 *   bytes. That is what makes a crash recoverable: prior records are immutable.
 * ===========================================================================
 */
#include "db.h"

#include <unistd.h>    /* read, write, ftruncate, lseek, fsync                  */
#include <string.h>    /* memcpy                                                */
#include <errno.h>

/* --------------------------------------------------------------------------
 * Command body (de)serialization. Shared by the WAL and by Raft entries, so the
 * bytes a leader replicates are byte-for-byte the bytes a follower will later
 * write to its own store.wal — encode once, learn once.
 * ------------------------------------------------------------------------ */

/* Body length: op(1) + klen(4) + key + vlen(4) + val. For OP_DEL, vlen is 0. */
size_t cmd_body_size(const struct command *c)
{
    uint32_t vlen = (c->op == OP_DEL) ? 0 : c->vlen;
    return 1u + 4u + c->klen + 4u + vlen;
}

/* Serialize `c` into `out`; returns the number of bytes written. Caller
 * guarantees out has cmd_body_size(c) bytes. */
size_t cmd_encode_body(uint8_t *out, const struct command *c)
{
    uint32_t vlen = (c->op == OP_DEL) ? 0 : c->vlen;
    uint8_t *p = out;
    *p++ = c->op;                              /* 1: the opcode                 */
    put_u32(p, c->klen); p += 4;               /* 4: key length                 */
    memcpy(p, c->key, c->klen); p += c->klen;  /*    key bytes                  */
    put_u32(p, vlen); p += 4;                  /* 4: value length (0 for DEL)   */
    if (vlen) { memcpy(p, c->val, vlen); p += vlen; } /* value bytes (if PUT)   */
    return (size_t)(p - out);
}

/* Parse a command body out of `in[0..len)`. On success returns 0 and fills `c`
 * with slices that BORROW into `in` (no allocation). On any inconsistency
 * (truncated field, length overrun, bad op, oversized key/val) returns -1 — the
 * caller treats that as corruption and stops. Bounds are checked at every step
 * because this parser eats attacker/crash-controlled bytes. */
int cmd_decode_body(const uint8_t *in, size_t len, struct command *c)
{
    size_t off = 0;
    if (len < 1) return -1;                    /* need at least the opcode      */
    c->op = in[off++];
    if (c->op != OP_PUT && c->op != OP_DEL) return -1;

    if (off + 4 > len) return -1;              /* klen field present?           */
    c->klen = get_u32(in + off); off += 4;
    if (c->klen == 0 || c->klen > DB_MAX_KEY) return -1; /* sane key length     */
    if (off + c->klen > len) return -1;        /* key bytes present?            */
    c->key = (const char *)(in + off); off += c->klen;

    if (off + 4 > len) return -1;              /* vlen field present?           */
    c->vlen = get_u32(in + off); off += 4;
    if (c->vlen > DB_MAX_VAL) return -1;       /* sane value length             */
    if (off + c->vlen > len) return -1;        /* value bytes present?          */
    c->val = (const char *)(in + off); off += c->vlen;

    /* Exactly one command per body: trailing bytes mean a framing bug. */
    return (off == len) ? 0 : -1;
}

/* --------------------------------------------------------------------------
 * WAL record = 8-byte header + command body.
 * ------------------------------------------------------------------------ */

size_t wal_record_size(const struct command *c)
{
    return 8u + cmd_body_size(c);              /* reclen(4) + crc(4) + body      */
}

/* Encode a full record into `out` (>= wal_record_size(c) bytes). Layout above.
 * Returns total bytes written. */
size_t wal_encode_record(uint8_t *out, const struct command *c)
{
    /* Encode the body first, starting 8 bytes in, so the header can describe it. */
    size_t body = cmd_encode_body(out + 8, c);
    put_u32(out, (uint32_t)body);              /* reclen = body length          */
    /* CRC covers the body only; a bad reclen surfaces as a short/failed read. */
    put_u32(out + 4, crc32_ieee(out + 8, body));
    return 8u + body;
}

/* Robust full-buffer read: loop over read() until `n` bytes are in, EOF, or a
 * hard error. Returns bytes read (< n only at clean EOF), or -1 on error.
 * read() can return fewer bytes than requested for any reason — a short read is
 * NORMAL, not an error — so every log reader must loop. EINTR (a signal
 * interrupted the syscall) is retried, not treated as failure. */
static ssize_t read_full(int fd, void *buf, size_t n)
{
    uint8_t *p = (uint8_t *)buf;
    size_t got = 0;
    while (got < n) {
        ssize_t r = read(fd, p + got, n - got);
        if (r < 0) { if (errno == EINTR) continue; return -1; }
        if (r == 0) break;                     /* EOF                           */
        got += (size_t)r;
    }
    return (ssize_t)got;
}

/* --------------------------------------------------------------------------
 * wal_replay — the recovery algorithm.
 *
 * Read records from the current file offset until EOF or the first damaged
 * record. For every intact record, call apply(ctx, &cmd). At the first sign of a
 * torn tail (short header, insane length, short body, or CRC mismatch) we STOP
 * and ftruncate() the file back to the offset where that bad record began, so the
 * log is left clean and the next append lands contiguously after the last good
 * record. Returns the count of records replayed, or -1 on a fatal I/O error.
 * ------------------------------------------------------------------------ */
long wal_replay(int fd, void (*apply)(void *ctx, const struct command *c),
                void *ctx)
{
    if (lseek(fd, 0, SEEK_SET) < 0) return -1; /* replay from the start         */

    /* One reusable body buffer sized to the largest legal record. A record can
     * never legitimately exceed one command, so this bound also rejects a
     * corrupt reclen that claims a gigantic body. */
    static uint8_t body[1 + 4 + DB_MAX_KEY + 4 + DB_MAX_VAL];
    const uint32_t max_body = (uint32_t)sizeof body;

    long replayed = 0;
    off_t good_off = 0;                        /* offset of the last clean point */

    for (;;) {
        uint8_t hdr[8];
        ssize_t got = read_full(fd, hdr, 8);
        if (got < 0) return -1;                /* real I/O error                */
        if (got == 0) break;                   /* clean EOF at a record boundary */
        if (got < 8) break;                    /* torn header ⇒ end of good log  */

        uint32_t reclen = get_u32(hdr);
        uint32_t crc    = get_u32(hdr + 4);
        if (reclen == 0 || reclen > max_body) break;  /* insane length ⇒ torn   */

        got = read_full(fd, body, reclen);
        if (got < 0) return -1;
        if ((uint32_t)got < reclen) break;     /* torn body ⇒ end of good log    */

        if (crc32_ieee(body, reclen) != crc) break;   /* CRC mismatch ⇒ torn    */

        struct command c;
        if (cmd_decode_body(body, reclen, &c) != 0) break; /* garbage ⇒ torn    */

        apply(ctx, &c);                        /* replay this mutation           */
        replayed++;
        good_off += 8 + reclen;                /* advance the clean watermark    */
    }

    /* Chop off any torn tail so future appends are contiguous and the next
     * recovery sees a clean file. Then position the fd at the end for appends. */
    if (ftruncate(fd, good_off) < 0) return -1;
    if (lseek(fd, good_off, SEEK_SET) < 0) return -1;
    return replayed;
}
