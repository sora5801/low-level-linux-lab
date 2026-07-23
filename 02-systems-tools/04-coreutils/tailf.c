/* ===========================================================================
 * tailf.c — print the tail of a file and FOLLOW it (tail -f), correctly.
 * ===========================================================================
 *
 * `tail -f` looks like "sleep, check size, print new bytes." The naive polling
 * version wastes CPU and still gets three cases wrong. This version uses
 * inotify(7) — the kernel filesystem-event interface — and handles all three:
 *
 *   - APPEND: the common case. IN_MODIFY fires; we read from our saved offset
 *     to the new EOF and print the delta.
 *   - TRUNCATION: `> logfile` shrinks the file. If we kept our old (now past-
 *     EOF) offset we'd read nothing forever. We detect size < offset via fstat
 *     and reset the offset to the new (smaller) size.
 *   - ROTATION: logrotate renames logfile -> logfile.1 and creates a fresh
 *     logfile. Our open fd still points at the OLD inode (now logfile.1), so we
 *     must reopen the PATH. IN_MOVE_SELF / IN_DELETE_SELF tell us this happened;
 *     we close, reopen by name (retrying until it reappears), and re-watch.
 *
 * inotify API used:
 *   inotify_init1(IN_CLOEXEC)             -> an fd you read events from
 *   inotify_add_watch(ifd, path, mask)    -> a watch descriptor (wd)
 *   read(ifd, buf, ...)                   -> a stream of struct inotify_event
 *
 * Flags: -n N (start by showing the last N lines; default 10).
 * =========================================================================== */
#include "coreutils.h"

#include <errno.h>
#include <fcntl.h>        /* open, O_RDONLY                                     */
#include <limits.h>       /* NAME_MAX                                          */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>  /* inotify_init1, inotify_add_watch, struct inotify_event */
#include <sys/stat.h>     /* fstat                                             */
#include <time.h>         /* nanosleep, struct timespec                        */
#include <unistd.h>       /* read, lseek, close                                */

/* ---------------------------------------------------------------------------
 * print_last_lines — show the final `want` lines of an already-open regular
 * file, then leave the fd's offset at EOF ready to follow.
 *
 * We read the file BACKWARD in blocks, counting '\n' bytes, until we've seen
 * enough newlines (or hit the start). This avoids scanning a multi-GiB file
 * from the front just to show its last 10 lines. Offsets are off_t (64-bit).
 * ------------------------------------------------------------------------- */
static off_t print_last_lines(int fd, off_t size, long want)
{
    if (size == 0) return 0;

    char buf[8192];
    off_t pos = size;                    /* we scan [pos-block, pos)            */
    long  newlines = 0;
    off_t start = 0;                     /* byte offset where our output begins  */

    /* A trailing '\n' at EOF ends the last line but does not begin a new one,
     * so to show N lines we look for N newlines BEFORE the final byte. We count
     * every '\n' and stop once we've found `want` of them strictly before EOF. */
    while (pos > 0) {
        off_t chunk = pos > (off_t)sizeof(buf) ? (off_t)sizeof(buf) : pos;
        off_t rstart = pos - chunk;
        /* pread(2): read at an explicit offset without moving the fd position —
         * ideal here because we still want the position at EOF afterwards. */
        ssize_t r = pread(fd, buf, (size_t)chunk, rstart);
        if (r < 0) { if (errno == EINTR) continue; return -1; }
        if (r == 0) break;

        for (off_t k = r - 1; k >= 0; k--) {
            if (buf[k] == '\n') {
                /* Ignore a newline that is the very last byte of the file. */
                if (rstart + k == size - 1) continue;
                newlines++;
                if (newlines >= want) {
                    start = rstart + k + 1;   /* line begins after this '\n'     */
                    goto found;
                }
            }
        }
        pos = rstart;
    }
    start = 0;                            /* fewer than `want` lines: show all   */
found:
    /* Print [start, size) with the robust copy loop, then position at EOF. */
    if (lseek(fd, start, SEEK_SET) < 0) return -1;
    {
        char cbuf[65536];
        off_t remaining = size - start;
        while (remaining > 0) {
            size_t w = remaining > (off_t)sizeof(cbuf)
                           ? sizeof(cbuf) : (size_t)remaining;
            ssize_t r = read_retry(fd, cbuf, w);
            if (r < 0) return -1;
            if (r == 0) break;
            if (write_all(STDOUT_FILENO, cbuf, (size_t)r) < 0) return -1;
            remaining -= r;
        }
    }
    return size;                         /* new follow offset = current EOF     */
}

/* Drain from `off` to EOF on fd, printing everything. Returns the new offset,
 * or -1 on error. Handles the case where new data arrived. */
static off_t drain_to_eof(int fd, off_t off)
{
    char buf[65536];
    if (lseek(fd, off, SEEK_SET) < 0) return -1;
    for (;;) {
        ssize_t r = read_retry(fd, buf, sizeof buf);
        if (r < 0) return -1;
        if (r == 0) break;               /* reached current EOF                 */
        if (write_all(STDOUT_FILENO, buf, (size_t)r) < 0) return -1;
        off += r;
    }
    return off;
}

/* Open the path and add an inotify watch for the events we care about. Returns
 * the fd (>=0) and stores the watch descriptor via *wd, or -1 on failure. */
static int open_and_watch(int ifd, const char *path, int *wd)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0) return -1;

    /* Watch mask:
     *   IN_MODIFY      -> data appended (or written): read the delta.
     *   IN_MOVE_SELF   -> the file was renamed (logrotate step 1): reopen.
     *   IN_DELETE_SELF -> the file was unlinked: reopen when it returns.
     *   IN_ATTRIB      -> metadata change; also fires when nlink drops to 0. */
    *wd = inotify_add_watch(ifd, path,
                            IN_MODIFY | IN_MOVE_SELF | IN_DELETE_SELF | IN_ATTRIB);
    if (*wd < 0) { close(fd); return -1; }
    return fd;
}

int tailf_main(int argc, char **argv)
{
    long want = 10;                      /* default last-N lines                */
    int  argi = 1;

    for (; argi < argc; argi++) {
        const char *a = argv[argi];
        if (a[0] != '-' || a[1] == '\0') break;
        if (strcmp(a, "--") == 0) { argi++; break; }
        if (strcmp(a, "-n") == 0) {
            if (++argi >= argc) diex("option requires an argument -- 'n'");
            want = atol(argv[argi]);
            if (want < 0) want = 0;
            continue;
        }
        if (a[1] == 'n') { want = atol(a + 2); if (want < 0) want = 0; continue; }
        diex("unknown option '%s'", a);
    }

    if (argi >= argc)
        diex("usage: tailf [-n N] FILE   (follows a single file)");
    const char *path = argv[argi];

    /* inotify_init1(2): create the event fd. IN_CLOEXEC so a future exec (we
     * don't do one, but it's the safe default) won't leak the fd. */
    int ifd = inotify_init1(IN_CLOEXEC);
    if (ifd < 0)
        die("inotify_init1");

    int wd = -1;
    int fd = open_and_watch(ifd, path, &wd);
    if (fd < 0) { warn("%s", path); close(ifd); return 1; }

    /* Print the initial tail and set our follow offset to EOF. */
    struct stat st;
    if (fstat(fd, &st) < 0) { warn("%s", path); goto fail; }
    off_t off = print_last_lines(fd, st.st_size, want);
    if (off < 0) { warn("%s", path); goto fail; }

    /* Event loop. inotify events are variable length (a fixed header + a
     * possibly-empty NUL-padded name). We read a batch and walk it like the
     * getdents buffer in ls.c. `struct inotify_event` + NAME_MAX + 1 is the
     * max size of one event, so this buffer holds at least one. */
    char evbuf[4096]
        __attribute__((aligned(__alignof__(struct inotify_event))));

    for (;;) {
        ssize_t n = read(ifd, evbuf, sizeof evbuf);
        if (n < 0) {
            if (errno == EINTR) continue;    /* signal: reissue the blocking read*/
            warn("inotify read"); goto fail;
        }

        int need_reopen = 0;
        int data_changed = 0;

        for (char *p = evbuf; p < evbuf + n; ) {
            struct inotify_event *ev = (struct inotify_event *)p;

            if (ev->mask & (IN_MOVE_SELF | IN_DELETE_SELF))
                need_reopen = 1;             /* rotation/unlink: switch inodes  */
            if (ev->mask & (IN_MODIFY | IN_ATTRIB))
                data_changed = 1;            /* appended or truncated           */
            if (ev->mask & IN_IGNORED)
                need_reopen = 1;             /* watch auto-removed (file gone)  */

            p += sizeof(struct inotify_event) + ev->len;  /* stride to next event*/
        }

        /* Handle an in-place change first: append OR truncate. fstat gives the
         * current size; if it shrank below our offset, the file was truncated,
         * so reset to the new size (start following from there). */
        if (data_changed && !need_reopen) {
            if (fstat(fd, &st) == 0) {
                if (st.st_size < off) {
                    warnx("%s: file truncated", path);
                    off = st.st_size;        /* follow from the new, smaller EOF*/
                }
                off_t no = drain_to_eof(fd, off);
                if (no < 0) { warn("%s", path); goto fail; }
                off = no;
            }
        }

        /* Handle rotation: the name now points at a NEW inode (or none yet).
         * Drain any last bytes from the OLD fd, then close it and reopen the
         * path, retrying until it exists again (logrotate's create step may lag
         * the rename). Re-add the watch to the new inode. */
        if (need_reopen) {
            /* Flush trailing bytes still readable on the old inode. */
            off_t no = drain_to_eof(fd, off);
            if (no >= 0) off = no;

            close(fd);
            /* The old watch is gone (IN_IGNORED); rm it defensively if valid. */
            if (wd >= 0) inotify_rm_watch(ifd, wd);
            wd = -1;

            /* Reopen with a bounded retry so we don't spin the CPU if the file
             * stays gone. In a real tool this would use a short nanosleep; here
             * we retry on the next inotify wakeup by re-opening opportunistically.
             */
            int nfd = open_and_watch(ifd, path, &wd);
            if (nfd < 0) {
                /* Not back yet. Sleep briefly and retry a few times; give up to
                 * the outer read() loop otherwise (it will block until the
                 * directory changes are irrelevant — so we poll a little). */
                for (int tries = 0; tries < 100 && nfd < 0; tries++) {
                    struct timespec ts = { 0, 50 * 1000 * 1000 }; /* 50 ms      */
                    nanosleep(&ts, NULL);
                    nfd = open_and_watch(ifd, path, &wd);
                }
                if (nfd < 0) { warn("%s: cannot reopen", path); goto fail; }
                warnx("%s: reopened (rotated)", path);
            } else {
                warnx("%s: reopened (rotated)", path);
            }
            fd = nfd;
            /* New file: start from its beginning so we don't miss early bytes. */
            off = drain_to_eof(fd, 0);
            if (off < 0) { warn("%s", path); goto fail; }
        }
    }

    /* Not reached (the loop only exits via goto fail), but keep the cleanup
     * path explicit and correct. */
fail:
    if (fd >= 0)  close(fd);
    if (ifd >= 0) close(ifd);
    return 1;
}
