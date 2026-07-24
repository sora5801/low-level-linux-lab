/* ===========================================================================
 * nullscan.c — a DEFENSIVE bad-char / null-byte analyzer.
 * ===========================================================================
 *
 * Reads a byte buffer (from a file argument or stdin) and reports:
 *   - whether it is NUL-free (the classic strcpy-channel constraint),
 *   - which "bad characters" it contains and at what offsets,
 *   - a short hex/ascii dump of the first bad hit for context.
 *
 * This is the analysis a DEFENDER runs when inspecting a suspicious buffer, and
 * equally the analysis anyone reasoning about a delivery channel runs. It ships
 * no payload and executes nothing it reads — it only *scans*. That is the whole
 * point: the instructive, safe, blue-team half of the shellcode topic.
 *
 * Build:  make          (cc -Wall -Wextra -O2 nullscan.c -o nullscan)
 * Use:    ./nullscan [-b 00,0a,0d] [file]      # default bad set: 0x00
 *         printf '\x48\x31\xc0...' | ./nullscan # scan bytes from stdin
 * =========================================================================== */

#include <stdio.h>      /* fprintf, printf                                    */
#include <stdlib.h>     /* strtoul, malloc, realloc, free                     */
#include <string.h>     /* memset, strtok                                     */
#include <unistd.h>     /* read, STDIN_FILENO                                 */
#include <fcntl.h>      /* open, O_RDONLY                                     */
#include <errno.h>      /* errno                                              */

/* A 256-bit set of "bad" byte values: one bit per possible byte value.
 * Membership test is O(1). See asm/demo.c for the extracted, annotated core. */
typedef struct { unsigned char bits[32]; } badset;

static void badset_add(badset *s, unsigned c) { s->bits[c >> 3] |= 1u << (c & 7); }
static int  badset_has(const badset *s, unsigned c) { return (s->bits[c >> 3] >> (c & 7)) & 1; }

/* Read an entire stream into a heap buffer, growing as needed. Returns the
 * buffer (caller frees) and writes the length via *out_len, or NULL on error.
 * We loop on read() because a single read() may return fewer bytes than asked
 * (short read) or be interrupted by a signal (EINTR) — the kind of correctness
 * detail the coreutils project harps on too. */
static unsigned char *slurp(int fd, size_t *out_len)
{
    size_t cap = 4096, len = 0;
    unsigned char *buf = malloc(cap);
    if (!buf) return NULL;
    for (;;) {
        if (len == cap) {                       /* grow geometrically         */
            size_t ncap = cap * 2;
            unsigned char *nb = realloc(buf, ncap);
            if (!nb) { free(buf); return NULL; }
            buf = nb; cap = ncap;
        }
        ssize_t n = read(fd, buf + len, cap - len);
        if (n < 0) {
            if (errno == EINTR) continue;       /* interrupted: retry         */
            free(buf); return NULL;             /* real I/O error             */
        }
        if (n == 0) break;                      /* EOF                        */
        len += (size_t)n;
    }
    *out_len = len;
    return buf;
}

/* Parse a comma-separated hex list like "00,0a,0d" into the bad set. */
static void parse_badset(badset *s, char *spec)
{
    for (char *tok = strtok(spec, ","); tok; tok = strtok(NULL, ",")) {
        unsigned long v = strtoul(tok, NULL, 16);
        if (v <= 0xff) badset_add(s, (unsigned)v);
    }
}

int main(int argc, char **argv)
{
    badset set;
    memset(&set, 0, sizeof set);
    const char *path = NULL;
    int have_custom = 0;

    /* Minimal arg parse: -b <hexlist> sets the bad-char set; a bare arg is a
     * file to scan (otherwise read stdin). */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-b") == 0 && i + 1 < argc) {
            parse_badset(&set, argv[++i]);
            have_custom = 1;
        } else {
            path = argv[i];
        }
    }
    if (!have_custom) badset_add(&set, 0x00);   /* default: NUL is the bad char */

    int fd = STDIN_FILENO;
    if (path) {
        fd = open(path, O_RDONLY);
        if (fd < 0) { fprintf(stderr, "nullscan: cannot open %s\n", path); return 2; }
    }

    size_t len = 0;
    unsigned char *buf = slurp(fd, &len);
    if (path) close(fd);
    if (!buf) { fprintf(stderr, "nullscan: read error\n"); return 2; }

    /* The scan: count bad bytes, remember the first offset, and whether any
     * NUL appears (the headline result). */
    long first_bad = -1;
    size_t bad_count = 0;
    int nul_present = 0;
    for (size_t i = 0; i < len; i++) {
        if (buf[i] == 0x00) nul_present = 1;
        if (badset_has(&set, buf[i])) {
            if (first_bad < 0) first_bad = (long)i;
            bad_count++;
        }
    }

    printf("scanned %zu bytes\n", len);
    printf("NUL-free:   %s\n", nul_present ? "NO  (contains 0x00)" : "yes");
    printf("bad bytes:  %zu", bad_count);
    if (first_bad >= 0) printf("  (first at offset %ld = 0x%lx)", first_bad, first_bad);
    printf("\n");

    /* Context dump around the first hit, so a defender sees what/where. */
    if (first_bad >= 0) {
        size_t start = (first_bad > 8) ? (size_t)first_bad - 8 : 0;
        size_t end = (size_t)first_bad + 8;
        if (end > len) end = len;
        printf("context: ");
        for (size_t i = start; i < end; i++)
            printf("%s%02x", i == (size_t)first_bad ? ">" : " ", buf[i]);
        printf("\n");
    }

    free(buf);
    /* Exit code: 0 = clean (no bad chars), 1 = bad chars present. Useful in
     * scripts:  ./nullscan payload.bin && echo "channel-safe". */
    return (bad_count > 0) ? 1 : 0;
}
