/* ===========================================================================
 * cp.c — copy a regular file, preserving mode and (crucially) sparse holes.
 * ===========================================================================
 *
 * A file is "sparse" when it has HOLES: byte ranges that were never written and
 * that the filesystem stores as metadata ("N zero bytes here") rather than N
 * real blocks on disk. A 10 GiB VM disk image that is 2 GiB full occupies 2 GiB.
 * A naive `read()/write()` cp reads the holes as literal zeros and WRITES them,
 * inflating the copy to the full 10 GiB. Getting this right is the headline
 * correctness bug this file exists to demonstrate.
 *
 * We use three techniques, best-first:
 *
 *   1. SEEK_DATA / SEEK_HOLE (lseek whences): the kernel tells us where data
 *      segments and holes are. We copy only the data segments and leave the
 *      holes as holes in the destination.
 *   2. copy_file_range(2): an in-KERNEL copy. Bytes never cross into user
 *      space, and on reflink-capable filesystems (btrfs/XFS) it can even share
 *      extents (copy-on-write). It also naturally propagates holes.
 *   3. A manual read/write loop that DETECTS all-zero blocks and turns them
 *      back into holes by seeking the destination forward (what GNU
 *      `cp --sparse=always` does). This is the universal fallback.
 *
 * We also preserve the source's permission bits (fchmod). Ownership and
 * timestamps are left as a documented stretch (see README).
 * =========================================================================== */
#include "coreutils.h"

#include <errno.h>    /* errno + the error codes we branch on                  */
#include <fcntl.h>    /* open, O_*                                             */
#include <stdint.h>   /* uint64_t                                             */
#include <stdio.h>    /* snprintf                                             */
#include <string.h>   /* strrchr, memcmp-free zero scan                       */
#include <sys/stat.h> /* fstat, fchmod, struct stat, S_ISDIR                  */
#include <unistd.h>   /* read, write, lseek, ftruncate, close                 */

/* copy_file_range lives behind _GNU_SOURCE; we may be built without the glibc
 * wrapper on an older toolchain, so we also provide a raw syscall path. The
 * number 326 is x86-64's copy_file_range (stable since Linux 4.5). */
#include <sys/syscall.h>
#ifndef SYS_copy_file_range
#define SYS_copy_file_range 326
#endif

#define CP_BUFSZ (256 * 1024)   /* fallback read/write chunk                    */

/* ---------------------------------------------------------------------------
 * cfr — one copy_file_range(2) call via the raw syscall, so the code compiles
 * even without the glibc wrapper. Signature:
 *   ssize_t copy_file_range(int fd_in, off_t *off_in,
 *                           int fd_out, off_t *off_out,
 *                           size_t len, unsigned int flags);
 *   rax=326, rdi=fd_in, rsi=&off_in, rdx=fd_out, r10=&off_out, r8=len, r9=flags.
 * When off pointers are non-NULL the kernel copies from *off_in and ADVANCES
 * both offsets by the number of bytes copied — so the file positions are left
 * untouched, which is exactly what we want when interleaving with lseek. */
static ssize_t cfr(int in, off_t *ioff, int out, off_t *ooff, size_t len)
{
    return (ssize_t)syscall(SYS_copy_file_range, in, ioff, out, ooff, len, 0u);
}

/* Copy exactly [off, off+len) from in->out using copy_file_range, looping over
 * short copies. Returns 0 on success. On the FIRST call failing with a "not
 * supported here" errno we return 1 (caller should fall back); a real I/O error
 * returns -1. The distinction matters: copy_file_range can refuse for benign
 * reasons (cross-filesystem on old kernels: EXDEV; unsupported: ENOSYS/EOPNOTSUPP;
 * some virtual filesystems: EINVAL) and we must not treat that as data loss. */
static int copy_range_segment(int in, int out, off_t off, uint64_t len)
{
    off_t ipos = off, opos = off;
    int   made_progress = 0;
    while (len > 0) {
        /* Cap each request; kernels historically limited to ~2 GiB per call. */
        size_t chunk = len > (uint64_t)0x7ffff000 ? 0x7ffff000 : (size_t)len;
        ssize_t n = cfr(in, &ipos, out, &opos, chunk);
        if (n < 0) {
            if (errno == EINTR) continue;          /* retry, no bytes moved     */
            if (!made_progress &&
                (errno == ENOSYS || errno == EXDEV ||
                 errno == EOPNOTSUPP || errno == EINVAL || errno == EBADF))
                return 1;                          /* benign: signal "fallback" */
            return -1;                             /* genuine I/O error         */
        }
        if (n == 0)                                /* unexpected EOF within len */
            break;
        made_progress = 1;
        len -= (uint64_t)n;                        /* ipos/opos already advanced */
    }
    return 0;
}

/* Is this whole buffer zero? Used by the manual fallback to re-punch holes.
 * We scan as size_t words for speed, then the tail as bytes. */
static int block_is_zero(const char *buf, size_t n)
{
    size_t i = 0;
    /* Word-at-a-time scan: the common case (a real data block) bails on the
     * first non-zero word, so this is cheap. */
    for (; i + sizeof(size_t) <= n; i += sizeof(size_t)) {
        size_t w;
        __builtin_memcpy(&w, buf + i, sizeof(w)); /* alignment-safe load        */
        if (w != 0) return 0;
    }
    for (; i < n; i++)
        if (buf[i] != 0) return 0;
    return 1;
}

/* ---------------------------------------------------------------------------
 * Manual sparse-aware copy: read a block; if it is all zeros, DON'T write it —
 * just lseek the destination forward, leaving a hole. Otherwise write it. A
 * final ftruncate fixes up a trailing hole (a zero block at EOF that we skipped)
 * so the destination ends at exactly `size` bytes.
 *
 * This is the fallback when neither SEEK_HOLE nor copy_file_range is available.
 * ------------------------------------------------------------------------- */
static int copy_manual_sparse(int in, int out, off_t size)
{
    char buf[CP_BUFSZ];
    off_t opos = 0;                       /* our logical position in the dest    */
    for (;;) {
        ssize_t r = read_retry(in, buf, sizeof(buf));
        if (r < 0) return -1;
        if (r == 0) break;                /* EOF                                 */

        if (block_is_zero(buf, (size_t)r)) {
            /* Punch a hole: advance the dest offset without writing. lseek(2)
             * past current EOF is legal and creates a sparse gap that reads as
             * zeros. rax=8, rdi=fd, rsi=offset, rdx=SEEK_CUR. */
            if (lseek(out, r, SEEK_CUR) < 0) return -1;
            opos += r;
        } else {
            if (write_all(out, buf, (size_t)r) < 0) return -1;
            opos += r;
        }
    }
    /* If the file ended with a skipped zero block, the dest is currently short
     * (we lseek'd but never wrote past it). ftruncate sets the size exactly,
     * materialising the trailing hole. Also correct when opos==size already. */
    if (size >= 0 && opos != size)
        opos = size;                      /* trust the stat size at EOF          */
    if (ftruncate(out, opos) < 0) return -1;
    return 0;
}

/* ---------------------------------------------------------------------------
 * SEEK_DATA/SEEK_HOLE-driven copy. Walk the file as an alternating sequence of
 * holes and data segments:
 *
 *   next_data = lseek(in, pos, SEEK_DATA);   // start of next data at/after pos
 *   next_hole = lseek(in, next_data, SEEK_HOLE); // end of that data segment
 *
 * We copy each [next_data, next_hole) segment and skip the holes entirely, so
 * the destination's holes line up with the source's. ftruncate at the end
 * stamps the exact size (covers a trailing hole).
 *
 * Returns 0 on success, 1 if SEEK_DATA is unsupported (caller falls back), -1
 * on a real error.
 * ------------------------------------------------------------------------- */
static int copy_seek_hole(int in, int out, off_t size)
{
    off_t pos    = 0;
    int   copied = 0;    /* have we written any bytes to the dest yet?          */
    while (pos < size) {
        /* Find the next byte of DATA at or after pos. If pos is inside a hole,
         * this jumps to the following data. ENXIO means "no more data" — the
         * rest of the file is a hole up to EOF, so we're done. EINVAL/EOPNOTSUPP
         * means this filesystem lacks SEEK_DATA.
         *
         * We only signal "fall back to the manual copy" (return 1) if we have
         * NOT written anything yet — otherwise restarting a fresh sequential
         * copy over a half-written dest would corrupt it. In practice SEEK_DATA
         * support is per-filesystem, so an unsupported error only ever appears
         * on the very first probe; the guard makes that guarantee explicit. */
        off_t data = lseek(in, pos, SEEK_DATA);
        if (data < 0) {
            if (errno == ENXIO) break;                    /* trailing hole       */
            if ((errno == EINVAL || errno == EOPNOTSUPP) && !copied)
                return 1;                                 /* unsupported: fallback*/
            return -1;
        }
        /* Find where that data segment ends (the next hole, or EOF). On a fully
         * dense file SEEK_HOLE returns `size`. */
        off_t hole = lseek(in, data, SEEK_HOLE);
        if (hole < 0) {
            if ((errno == EINVAL || errno == EOPNOTSUPP) && !copied)
                return 1;
            return -1;
        }
        if (hole <= data)                                  /* defensive: no loop  */
            break;

        /* Copy the dense segment [data, hole). Prefer the in-kernel path; if it
         * declines (return 1) fall back to a positioned read/write for just this
         * segment so holes are still preserved (we simply never touched them). */
        int rc = copy_range_segment(in, out, data, (uint64_t)(hole - data));
        if (rc < 0) return -1;
        copied = 1;                                        /* dest now has data   */
        if (rc == 1) {
            /* Positioned read/write for [data, hole): seek both fds and stream. */
            if (lseek(in, data, SEEK_SET) < 0) return -1;
            if (lseek(out, data, SEEK_SET) < 0) return -1;
            off_t remaining = hole - data;
            char buf[CP_BUFSZ];
            while (remaining > 0) {
                size_t want = remaining > (off_t)sizeof(buf)
                                  ? sizeof(buf) : (size_t)remaining;
                ssize_t r = read_retry(in, buf, want);
                if (r < 0) return -1;
                if (r == 0) break;                         /* short file: stop    */
                if (write_all(out, buf, (size_t)r) < 0) return -1;
                remaining -= r;
            }
        }
        pos = hole;                                        /* advance past segment*/
    }
    /* Stamp the exact final size so a trailing hole exists and the dest isn't
     * one byte short/long relative to the source. */
    if (ftruncate(out, size) < 0) return -1;
    return 0;
}

/* If `path` names an existing directory, cp copies INTO it under the source's
 * basename (POSIX `cp SRC DIR`). Returns a malloc'd path the caller frees, or a
 * copy of `dst` unchanged. We keep it simple: caller passes the buffer. */
static const char *resolve_dest(const char *src, const char *dst,
                                char *out, size_t outsz)
{
    struct stat ds;
    if (stat(dst, &ds) == 0 && S_ISDIR(ds.st_mode)) {
        const char *slash = strrchr(src, '/');
        const char *base  = slash ? slash + 1 : src;
        int n = snprintf(out, outsz, "%s/%s", dst, base);
        if (n > 0 && (size_t)n < outsz)
            return out;
        /* Truncated: fall through and let open() fail with a clear error. */
    }
    return dst;
}

int cp_main(int argc, char **argv)
{
    if (argc != 3)
        diex("usage: cp SOURCE DEST");

    const char *src = argv[1];
    char destbuf[4096];
    const char *dst = resolve_dest(src, argv[2], destbuf, sizeof destbuf);

    /* Open source read-only; capture its metadata with fstat (not stat) so we
     * describe the exact object we opened — no TOCTOU window. */
    int in = open(src, O_RDONLY);
    if (in < 0)
        die("cannot open '%s'", src);

    struct stat st;
    if (fstat(in, &st) < 0) { warn("cannot stat '%s'", src); close(in); return 1; }

    if (S_ISDIR(st.st_mode)) {           /* we copy files, not trees            */
        warnx("-r not supported: '%s' is a directory", src);
        close(in);
        return 1;
    }

    /* Create the destination. O_CREAT|O_TRUNC|O_WRONLY; initial mode 0600 so a
     * would-be-secret file is never momentarily world-readable between create
     * and the fchmod below. We fix the mode to the source's at the end. */
    int out = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (out < 0) { warn("cannot create '%s'", dst); close(in); return 1; }

    /* --- choose the copy strategy, best-first --- */
    int rc = copy_seek_hole(in, out, st.st_size);   /* 1) SEEK_DATA/SEEK_HOLE   */
    if (rc == 1)
        rc = copy_manual_sparse(in, out, st.st_size); /* 3) manual zero-detect  */
    if (rc < 0) {
        warn("copy '%s' -> '%s'", src, dst);
        close(in); close(out);
        return 1;
    }

    /* Preserve permission bits (including setuid/setgid/sticky). fchmod on the
     * fd we already hold avoids a second name lookup and any TOCTOU race with a
     * swapped-in symlink. We mask to the 07777 permission bits; the file-type
     * bits in st_mode are not settable. */
    if (fchmod(out, st.st_mode & 07777) < 0)
        warn("cannot preserve mode of '%s'", dst);   /* non-fatal: data is safe */

    /* Close and CHECK the write side: a deferred write error (ENOSPC, EIO on a
     * network fs) can surface only at close(2). Ignoring it is a classic data-
     * loss bug. */
    if (close(out) < 0) { warn("error closing '%s'", dst); close(in); return 1; }
    close(in);
    return 0;
}
