/* ===========================================================================
 * util.c — error handling, checked allocation, raw file I/O, and hex.
 * ===========================================================================
 *
 * The interesting part of this file is the file I/O: it is built directly on the
 * open(2)/read(2)/write(2)/close(2) syscalls rather than stdio, because the
 * error paths ARE the lesson. A read can return fewer bytes than you asked for
 * without any error; a write can too; both can be interrupted by a signal and
 * return -1/EINTR before doing anything. Production code that ignores this
 * corrupts data under load. We handle all three cases explicitly.
 * ===========================================================================
 */
#include "mygit.h"

#include <stdio.h>      /* fprintf, vfprintf, snprintf                        */
#include <stdlib.h>     /* malloc, realloc, free, exit                        */
#include <stdarg.h>     /* va_list for die()                                  */
#include <string.h>     /* strlen, memcpy                                     */
#include <errno.h>      /* errno, EINTR                                       */
#include <fcntl.h>      /* open, O_* flags                                    */
#include <unistd.h>     /* read, write, close                                 */
#include <sys/stat.h>   /* stat, mkdir, S_ISDIR                               */

/* ---------------------------------------------------------------------------
 * die / die_errno — print a message and abort. Marked noreturn so the compiler
 * knows control never continues past a call (no "use of uninitialized var"
 * warnings after a die(), and dead-code elimination downstream).
 * --------------------------------------------------------------------------- */
void die(const char *fmt, ...)
{
    va_list ap;
    fputs("mygit: fatal: ", stderr);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    exit(1);
}

/* Like die(), but also append the current errno's message — use this right
 * after a failed syscall so the reader learns *why* (ENOENT, EACCES, ...). */
void die_errno(const char *fmt, ...)
{
    int e = errno;                  /* capture errno before anything can change it */
    va_list ap;
    fputs("mygit: fatal: ", stderr);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, ": %s\n", strerror(e));
    exit(1);
}

/* ---------------------------------------------------------------------------
 * Checked allocation. Every heap block in this program comes through here, so a
 * NULL from the allocator becomes an immediate, uniform failure instead of a
 * scattered crash. Ownership convention: the CALLER frees what these return.
 * --------------------------------------------------------------------------- */
void *xmalloc(size_t n)
{
    /* malloc(0) may legally return NULL; bump to 1 so "success" is unambiguous. */
    void *p = malloc(n ? n : 1);
    if (!p) die("out of memory (malloc %zu bytes)", n);
    return p;
}

void *xrealloc(void *p, size_t n)
{
    void *q = realloc(p, n ? n : 1);
    if (!q) die("out of memory (realloc %zu bytes)", n);
    return q;
}

char *xstrdup(const char *s)
{
    size_t n = strlen(s) + 1;
    char  *d = xmalloc(n);
    memcpy(d, s, n);
    return d;
}

/* ---------------------------------------------------------------------------
 * xread — read up to `count` bytes, retrying only the EINTR case.
 *
 * read(2): ssize_t read(int fd, void *buf, size_t count).  Syscall number 0 on
 * x86-64; args in rdi=fd, rsi=buf, rdx=count; returns the count read in rax, 0
 * at EOF, or -1 with errno set. The kernel is allowed to return a SHORT read
 * (fewer than count) for any reason — a pipe with less data ready, a signal, a
 * slow device — so a single read() is never a guarantee. Here we return whatever
 * this one call produced and let read_all() loop; we only transparently retry
 * EINTR (a signal interrupted us before any bytes moved). */
static ssize_t xread(int fd, void *buf, size_t count)
{
    ssize_t n;
    do {
        n = read(fd, buf, count);
    } while (n < 0 && errno == EINTR);   /* interrupted before any data: retry */
    return n;
}

/* xwrite — write up to `count` bytes, retrying only EINTR. write(2) is syscall
 * number 1 (rdi=fd, rsi=buf, rdx=count -> rax=bytes written or -1). Like read,
 * it may write FEWER bytes than requested with no error at all, which is why
 * write_all() below loops until everything is flushed. */
static ssize_t xwrite(int fd, const void *buf, size_t count)
{
    ssize_t n;
    do {
        n = write(fd, buf, count);
    } while (n < 0 && errno == EINTR);
    return n;
}

/* Loop until exactly `count` bytes are written, advancing past short writes. A
 * short write (n < remaining) is normal, not an error; we simply continue from
 * where the kernel stopped. Any real -1 (after EINTR handling) is fatal. */
static void write_all(int fd, const void *buf, size_t count, const char *path)
{
    const unsigned char *p = buf;
    while (count > 0) {
        ssize_t n = xwrite(fd, p, count);
        if (n < 0) die_errno("write %s", path);
        p     += (size_t)n;      /* advance past the bytes that DID land       */
        count -= (size_t)n;
    }
}

/* ---------------------------------------------------------------------------
 * read_file — slurp an entire file into a heap buffer.
 *
 * We fstat(2) to get a size hint for a single right-sized allocation, then loop
 * read() until EOF (n == 0), growing if the file turned out larger than the
 * stat said (it can, for special files). The buffer is NUL-terminated as a
 * convenience for text callers, but *len is the authoritative byte count —
 * blobs contain NULs and you must not treat them as C strings.
 * --------------------------------------------------------------------------- */
unsigned char *read_file(const char *path, size_t *len)
{
    /* open(2): syscall 2 on x86-64 (rdi=path, rsi=flags, rdx=mode). O_RDONLY.
     * Returns the lowest free file descriptor, or -1/errno (ENOENT, EACCES). */
    int fd = open(path, O_RDONLY);
    if (fd < 0) die_errno("open %s", path);

    struct stat st;
    size_t cap = 0;
    /* fstat(2): fill `st` for an open fd. st_size is our capacity hint; for a
     * regular file it is exact, so most reads take one pass. */
    if (fstat(fd, &st) == 0 && st.st_size > 0)
        cap = (size_t)st.st_size;
    if (cap == 0) cap = 4096;              /* pipes/empty stat: start somewhere */

    unsigned char *buf = xmalloc(cap + 1); /* +1 so we can NUL-terminate       */
    size_t total = 0;

    for (;;) {
        if (total == cap) {                /* grew past the hint: double it     */
            cap *= 2;
            buf = xrealloc(buf, cap + 1);
        }
        ssize_t n = xread(fd, buf + total, cap - total);
        if (n < 0) die_errno("read %s", path);
        if (n == 0) break;                 /* clean EOF                         */
        total += (size_t)n;
    }

    /* close(2): release the descriptor. We check its return too — a failing
     * close can surface a deferred write error on some filesystems, and leaking
     * descriptors in a long-lived process is a real bug. */
    if (close(fd) < 0) die_errno("close %s", path);

    buf[total] = '\0';
    if (len) *len = total;
    return buf;
}

/* write_file — create/truncate `path` and write `len` bytes in full. Mode 0644
 * (rw-r--r--, subject to umask) is git's default for loose files. */
void write_file(const char *path, const void *data, size_t len)
{
    /* O_WRONLY|O_CREAT|O_TRUNC: create if absent, empty it if present. The third
     * open() arg (0644) is the mode used only when O_CREAT actually creates. */
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) die_errno("open %s for writing", path);
    write_all(fd, data, len, path);
    if (close(fd) < 0) die_errno("close %s", path);
}

/* ---------------------------------------------------------------------------
 * mkdir_p — create `path` and any missing parents. We walk the string, and at
 * each '/' temporarily NUL-terminate and mkdir the prefix. EEXIST is success
 * (the directory is already there), which is what makes this idempotent — safe
 * to call before every object write.
 * --------------------------------------------------------------------------- */
void mkdir_p(const char *path)
{
    char tmp[4096];
    size_t n = strlen(path);
    if (n >= sizeof tmp) die("path too long: %s", path);
    memcpy(tmp, path, n + 1);

    for (size_t i = 1; i < n; i++) {
        if (tmp[i] == '/') {
            tmp[i] = '\0';                 /* isolate the parent prefix         */
            /* mkdir(2): rdi=path, rsi=mode. EEXIST means "already there" = ok. */
            if (mkdir(tmp, 0777) < 0 && errno != EEXIST)
                die_errno("mkdir %s", tmp);
            tmp[i] = '/';                  /* restore and continue              */
        }
    }
    if (mkdir(tmp, 0777) < 0 && errno != EEXIST)
        die_errno("mkdir %s", tmp);
}

/* stat wrappers used by the porcelain. path_exists tolerates any stat failure
 * as "no"; is_dir checks the S_IFDIR bit via the S_ISDIR macro. */
int path_exists(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0;
}

int is_dir(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

/* ---------------------------------------------------------------------------
 * hex — the human<->machine boundary for object ids.
 *
 * An object id is 20 bytes internally but is always SHOWN as 40 lowercase hex
 * characters (two per byte, high nibble first). hex_encode is the formatter you
 * see in every `cat-file`/`log` line; hex_decode parses an id typed on the
 * command line. The core of both is nibble<->char, isolated in asm/demo.c so its
 * branch-free table lookup can be read in assembly.
 * --------------------------------------------------------------------------- */
static const char HEXDIGITS[16] = "0123456789abcdef";

void hex_encode(const unsigned char *raw, size_t n, char *out)
{
    for (size_t i = 0; i < n; i++) {
        out[2 * i]     = HEXDIGITS[raw[i] >> 4];    /* high nibble (bits 7..4)  */
        out[2 * i + 1] = HEXDIGITS[raw[i] & 0x0f];  /* low nibble  (bits 3..0)  */
    }
    out[2 * n] = '\0';
}

/* Parse one hex digit to 0..15, or -1 if it is not a hex character. */
static int unhex_nibble(int ch)
{
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return -1;
}

int hex_decode(const char *hex, unsigned char *raw, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        int hi = unhex_nibble((unsigned char)hex[2 * i]);
        int lo = unhex_nibble((unsigned char)hex[2 * i + 1]);
        if (hi < 0 || lo < 0) return -1;            /* bad digit or short input */
        raw[i] = (unsigned char)((hi << 4) | lo);
    }
    return 0;
}

void oid_to_hex(const unsigned char oid[OID_RAWSZ], char out[OID_HEXSZ + 1])
{
    hex_encode(oid, OID_RAWSZ, out);
}

/* need_repo — most subcommands require an initialized store. We check for the
 * objects directory specifically (its presence is what init guarantees). */
void need_repo(void)
{
    if (!is_dir(GIT_DIR "/objects"))
        die("not a mygit repository (run 'mygit init' first)");
}
