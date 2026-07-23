/* ===========================================================================
 * ls.c — list a directory using the RAW getdents64 syscall + stat + columns.
 * ===========================================================================
 *
 * Most `ls` clones call readdir(3). We go one layer down to getdents64(2), the
 * syscall readdir is built on, because seeing the raw `struct linux_dirent64`
 * stream is the whole lesson: the kernel hands you a packed, variable-length
 * record buffer and you walk it by each record's d_reclen.
 *
 * Supported (a deliberately small but honest subset of GNU ls):
 *   -a  include dotfiles (names starting with '.')
 *   -l  long format: type+perms, links, owner, group, size, mtime, name
 *   -1  one entry per line (the default when stdout is not a terminal)
 *   default: multi-column, sorted, sized to the terminal width (down-then-across)
 *
 * Syscalls exercised: openat/open(O_DIRECTORY), getdents64, fstatat(lstat),
 * ioctl(TIOCGWINSZ). Plus getpwuid/getgrgid for name resolution.
 * =========================================================================== */
#include "coreutils.h"

#include <errno.h>
#include <fcntl.h>       /* open, O_RDONLY, O_DIRECTORY, AT_SYMLINK_NOFOLLOW    */
#include <grp.h>         /* getgrgid                                           */
#include <pwd.h>         /* getpwuid                                           */
#include <stdint.h>      /* uint64_t, int64_t                                  */
#include <stdio.h>       /* snprintf, printf, putchar                          */
#include <stdlib.h>      /* malloc, realloc, free, qsort                       */
#include <string.h>      /* strcmp, strlen, memcpy                             */
#include <sys/ioctl.h>   /* ioctl, TIOCGWINSZ, struct winsize                  */
#include <sys/stat.h>    /* struct stat, fstatat, S_IS*                        */
#include <sys/syscall.h> /* SYS_getdents64                                     */
#include <time.h>        /* time, localtime_r, strftime                        */
#include <unistd.h>      /* close, isatty, read                                */

/* getdents64 returns a packed stream of these. glibc hides it behind readdir,
 * so we declare it ourselves — this is the kernel's stable layout. The trailing
 * d_name is a flexible array: each record is d_reclen bytes total, so the NEXT
 * record starts at (char*)this + this->d_reclen. */
struct linux_dirent64 {
    uint64_t       d_ino;        /* inode number                                */
    int64_t        d_off;        /* opaque cookie for seeking the dir (unused)  */
    unsigned short d_reclen;     /* THIS record's length -> stride to the next  */
    unsigned char  d_type;       /* DT_DIR/DT_REG/... : type without a stat!    */
    char           d_name[];     /* NUL-terminated filename                     */
};

/* One collected entry. We snapshot the name and d_type from getdents64, and
 * lazily lstat only in long mode (a stat per file is the expensive part). */
struct entry {
    char         *name;          /* owned, malloc'd                             */
    unsigned char d_type;        /* DT_* hint straight from the dirent          */
    struct stat   st;            /* filled in -l mode only                      */
    int           have_st;       /* was st populated?                           */
};

/* Options bitset. */
struct opts { int all, longfmt, one; };

/* ---- name resolution caches would go here in a real ls; we keep it simple ---*/

/* Turn a mode into the classic 10-char string "drwxr-xr-x". */
static void mode_string(mode_t m, char out[11])
{
    /* File-type character from the top bits. */
    char t = '?';
    if      (S_ISREG(m))  t = '-';
    else if (S_ISDIR(m))  t = 'd';
    else if (S_ISLNK(m))  t = 'l';
    else if (S_ISCHR(m))  t = 'c';
    else if (S_ISBLK(m))  t = 'b';
    else if (S_ISFIFO(m)) t = 'p';
    else if (S_ISSOCK(m)) t = 's';
    out[0] = t;

    /* Three rwx triples for owner/group/other. */
    out[1] = (m & S_IRUSR) ? 'r' : '-';
    out[2] = (m & S_IWUSR) ? 'w' : '-';
    out[3] = (m & S_IXUSR) ? 'x' : '-';
    out[4] = (m & S_IRGRP) ? 'r' : '-';
    out[5] = (m & S_IWGRP) ? 'w' : '-';
    out[6] = (m & S_IXGRP) ? 'x' : '-';
    out[7] = (m & S_IROTH) ? 'r' : '-';
    out[8] = (m & S_IWOTH) ? 'w' : '-';
    out[9] = (m & S_IXOTH) ? 'x' : '-';

    /* Overlay the special bits exactly as GNU ls does: setuid shows in the
     * user-exec slot (s/S), setgid in group-exec, sticky in other-exec. Capital
     * letter = the bit is set but the underlying execute bit is NOT. */
    if (m & S_ISUID) out[3] = (m & S_IXUSR) ? 's' : 'S';
    if (m & S_ISGID) out[6] = (m & S_IXGRP) ? 's' : 'S';
    if (m & S_ISVTX) out[9] = (m & S_IXOTH) ? 't' : 'T';
    out[10] = '\0';
}

/* qsort comparator: plain byte-wise name order (C locale), like `LC_ALL=C ls`. */
static int cmp_entry(const void *a, const void *b)
{
    const struct entry *ea = a, *eb = b;
    return strcmp(ea->name, eb->name);
}

/* Query the terminal width for column layout. TIOCGWINSZ via ioctl(2) asks the
 * tty driver for its window size; if stdout is not a tty (a pipe/file) there is
 * no width, so we honour $COLUMNS or default to 80. */
static int term_width(void)
{
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0)
        return ws.ws_col;
    const char *c = getenv("COLUMNS");
    if (c) { int v = atoi(c); if (v > 0) return v; }
    return 80;
}

/* ---------------------------------------------------------------------------
 * read_dir — the getdents64 loop. Open the directory, then repeatedly ask the
 * kernel to fill a byte buffer with as many dirent records as fit. A return of
 * 0 means "end of directory". We grow an array of `struct entry`.
 *
 * getdents64(2): rax=SYS_getdents64, rdi=dirfd, rsi=buf, rdx=buf_size.
 *   Returns bytes written into buf (>0), 0 at end, -1 on error. Each record's
 *   d_reclen is the stride to the next; the last record can be shorter than
 *   sizeof(struct) because d_name is variable length.
 * Returns the (possibly empty) array via out and n, and the open dirfd via dfd
 * so the caller can fstatat relative to it. -1 on error.
 * ------------------------------------------------------------------------- */
static int read_dir(const char *path, const struct opts *o,
                    struct entry **out, size_t *n, int *dfd)
{
    /* O_DIRECTORY makes open fail with ENOTDIR if `path` isn't a directory,
     * closing a race where it changes type under us. O_RDONLY is all getdents
     * needs (no O_WRONLY on a directory). */
    int fd = open(path, O_RDONLY | O_DIRECTORY);
    if (fd < 0)
        return -1;                        /* caller reports with errno           */

    struct entry *arr = NULL;
    size_t cap = 0, cnt = 0;

    /* A page-ish buffer holds many records per syscall. Bigger = fewer
     * syscalls; 32 KiB is a good middle ground. We align it to 8 bytes because
     * the first field of each record (d_ino) is a uint64_t: the kernel packs
     * records back-to-back, so an 8-aligned buffer start keeps every record's
     * 8-byte fields naturally aligned (correctness on strict-alignment arches;
     * a speed detail on x86-64). */
    char buf[32 * 1024] __attribute__((aligned(8)));
    for (;;) {
        long nread = syscall(SYS_getdents64, fd, buf, sizeof buf);
        if (nread < 0) { free(arr); close(fd); return -1; }
        if (nread == 0) break;            /* end of directory                    */

        /* Walk the packed records. bpos advances by each record's d_reclen. */
        for (long bpos = 0; bpos < nread; ) {
            struct linux_dirent64 *d =
                (struct linux_dirent64 *)(buf + bpos);

            /* Filter dotfiles unless -a. Note "." and ".." start with '.', so
             * -a shows them too, matching GNU ls. */
            if (o->all || d->d_name[0] != '.') {
                if (cnt == cap) {
                    size_t ncap = cap ? cap * 2 : 64;
                    struct entry *tmp = realloc(arr, ncap * sizeof *arr);
                    if (!tmp) { free(arr); close(fd); errno = ENOMEM; return -1; }
                    arr = tmp; cap = ncap;
                }
                size_t len = strlen(d->d_name);
                char *nm = malloc(len + 1);
                if (!nm) { free(arr); close(fd); errno = ENOMEM; return -1; }
                memcpy(nm, d->d_name, len + 1);
                arr[cnt].name    = nm;
                arr[cnt].d_type  = d->d_type;
                arr[cnt].have_st = 0;
                cnt++;
            }
            bpos += d->d_reclen;          /* stride to the next record           */
        }
    }

    *out = arr; *n = cnt; *dfd = fd;
    return 0;
}

/* Long-format print of one entry. Needs a completed lstat (via fstatat with
 * AT_SYMLINK_NOFOLLOW = don't follow symlinks, so we describe the link itself).*/
static void print_long(int dfd, struct entry *e)
{
    if (!e->have_st) {
        /* fstatat(2): stat `name` relative to the open dir fd — no path
         * rebuilding, no race with a renamed parent. AT_SYMLINK_NOFOLLOW gives
         * lstat semantics. */
        if (fstatat(dfd, e->name, &e->st, AT_SYMLINK_NOFOLLOW) == 0)
            e->have_st = 1;
    }
    if (!e->have_st) {                    /* stat failed (perms, race): degrade  */
        printf("?????????? ? ? ? ? ? %s\n", e->name);
        return;
    }

    struct stat *s = &e->st;
    char perms[11];
    mode_string(s->st_mode, perms);

    /* Resolve uid/gid to names; fall back to the numeric id (buffer) if the
     * user/group was deleted. getpwuid/getgrgid consult /etc/passwd + nsswitch;
     * they return a pointer into a static buffer we must not free. */
    char ubuf[32], gbuf[32];
    struct passwd *pw = getpwuid(s->st_uid);
    struct group  *gr = getgrgid(s->st_gid);
    const char *user  = pw ? pw->pw_name : (snprintf(ubuf, sizeof ubuf, "%u",
                              (unsigned)s->st_uid), ubuf);
    const char *group = gr ? gr->gr_name : (snprintf(gbuf, sizeof gbuf, "%u",
                              (unsigned)s->st_gid), gbuf);

    /* Time column: GNU ls shows "Mon DD HH:MM" for recent files and
     * "Mon DD  YYYY" for anything older than ~6 months (or in the future),
     * because the year matters more than the minute once it's not recent. */
    char tbuf[32];
    time_t now = time(NULL);
    struct tm tm;
    localtime_r(&s->st_mtime, &tm);
    time_t age = now - s->st_mtime;
    const char *fmt = (age > 15552000 || age < -900) ? "%b %e  %Y" : "%b %e %H:%M";
    strftime(tbuf, sizeof tbuf, fmt, &tm);

    /* One long line. %ju for the off_t size (cast to uintmax_t) is the portable
     * way to print a 64-bit file size. */
    printf("%s %3lu %-8s %-8s %8ju %s %s\n",
           perms,
           (unsigned long)s->st_nlink,
           user, group,
           (uintmax_t)s->st_size,
           tbuf,
           e->name);
}

/* Multi-column layout, down-then-across (the GNU ls default). We compute the
 * widest name, derive how many columns fit in the terminal, then how many rows
 * that implies, and print column-major so reading top-to-bottom is alphabetical.
 */
static void print_columns(struct entry *arr, size_t n, int width)
{
    /* Longest name determines the cell width (+2 spaces of gutter). */
    size_t maxlen = 0;
    for (size_t i = 0; i < n; i++) {
        size_t l = strlen(arr[i].name);
        if (l > maxlen) maxlen = l;
    }
    size_t cellw = maxlen + 2;            /* name + a 2-space gutter             */
    /* How many whole cells fit across the terminal? At least one, never more
     * than the number of entries (no point in empty columns). */
    size_t cols  = cellw ? (size_t)width / cellw : 1;
    if (cols < 1) cols = 1;
    if (cols > n) cols = n;
    size_t rows = (n + cols - 1) / cols;  /* ceil(n/cols) -> rows this implies   */
    /* Recompute cols from rows: with a fixed row count the items may actually
     * pack into fewer columns, and using that tighter value avoids printing a
     * trailing column that is mostly empty (matches GNU ls's appearance). */
    cols = (n + rows - 1) / rows;

    /* GNU ls fills COLUMN-major: reading straight down column 0 is alphabetical,
     * then column 1 continues where it left off. So the entry at (row r, col c)
     * is index c*rows + r — NOT r*cols + c (which would be row-major). */
    for (size_t r = 0; r < rows; r++) {
        for (size_t c = 0; c < cols; c++) {
            size_t idx = c * rows + r;    /* column-major index                  */
            if (idx >= n) continue;       /* last column can be short            */
            const char *nm = arr[idx].name;
            /* Don't pad the final printed cell of a row (either the last column,
             * or the last one that actually has an entry): trailing spaces would
             * be invisible junk at end of line. */
            if (c + 1 == cols || (c + 1) * rows + r >= n)
                printf("%s", nm);
            else
                printf("%-*s", (int)cellw, nm);   /* left-justify in the cell    */
        }
        putchar('\n');
    }
}

/* Print a single non-directory operand (a plain file/symlink named on the
 * command line). In -l mode we lstat it and print the long row; otherwise just
 * the name. Returns 0, or -1 if the file can't be stat'd. */
static int print_file_operand(const char *path, const struct opts *o)
{
    if (!o->longfmt) {
        printf("%s\n", path);
        return 0;
    }
    struct entry e = { .name = (char *)path, .d_type = 0, .have_st = 0 };
    if (lstat(path, &e.st) < 0) {
        warn("cannot access '%s'", path);
        return -1;
    }
    e.have_st = 1;
    /* Reuse the long-format printer; dfd is unused because have_st is already
     * set, so any value (AT_FDCWD) is fine. */
    print_long(AT_FDCWD, &e);
    return 0;
}

/* List one directory path. Returns 0 or -1. */
static int list_one(const char *path, const struct opts *o, int print_header)
{
    struct entry *arr = NULL;
    size_t n = 0;
    int dfd = -1;

    if (read_dir(path, o, &arr, &n, &dfd) < 0) {
        warn("cannot open directory '%s'", path);
        return -1;
    }

    if (print_header)
        printf("%s:\n", path);            /* multi-dir header, like `ls a b`     */

    qsort(arr, n, sizeof *arr, cmp_entry);

    if (o->longfmt) {
        for (size_t i = 0; i < n; i++)
            print_long(dfd, &arr[i]);
    } else if (o->one) {
        for (size_t i = 0; i < n; i++)
            printf("%s\n", arr[i].name);
    } else {
        print_columns(arr, n, term_width());
    }

    /* Free the per-entry names and the array; close the dir fd. Ownership of
     * every malloc'd name ends here. */
    for (size_t i = 0; i < n; i++)
        free(arr[i].name);
    free(arr);
    close(dfd);
    return 0;
}

int ls_main(int argc, char **argv)
{
    struct opts o = {0, 0, 0};

    /* Hand-rolled option scan (no getopt) so the parsing is visible: everything
     * starting with '-' (but not a bare "-") is a bundle of single-char flags. */
    int argi = 1;
    for (; argi < argc; argi++) {
        const char *a = argv[argi];
        if (a[0] != '-' || a[1] == '\0') break;   /* not an option -> operands  */
        if (strcmp(a, "--") == 0) { argi++; break; }
        for (const char *p = a + 1; *p; p++) {
            switch (*p) {
            case 'a': o.all = 1;     break;
            case 'l': o.longfmt = 1; break;
            case '1': o.one = 1;     break;
            default:
                diex("unknown option -- '%c'", *p);
            }
        }
    }

    /* If stdout is not a terminal and no format was chosen, default to one-per-
     * line (so `ls | wc -l` is correct). This mirrors GNU ls. */
    if (!o.longfmt && !o.one && !isatty(STDOUT_FILENO))
        o.one = 1;

    /* No operands => list ".". */
    int noperands = argc - argi;
    if (noperands <= 0) {
        return list_one(".", &o, 0) < 0 ? 1 : 0;
    }

    /* With multiple operands, print a "path:" header before each directory.
     * Decide per-operand up front with lstat so we never rely on errno surviving
     * an intervening warn() (which calls strerror/fprintf and may clobber it). */
    int status = 0;
    int header = noperands > 1;
    for (; argi < argc; argi++) {
        struct stat st;
        if (lstat(argv[argi], &st) < 0) {
            warn("cannot access '%s'", argv[argi]);
            status = 1;
            continue;
        }
        if (S_ISDIR(st.st_mode)) {
            if (list_one(argv[argi], &o, header) < 0)
                status = 1;
        } else {
            if (print_file_operand(argv[argi], &o) < 0)
                status = 1;
        }
    }
    return status;
}
