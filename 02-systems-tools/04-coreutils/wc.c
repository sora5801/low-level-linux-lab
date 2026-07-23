/* ===========================================================================
 * wc.c — count lines, words, bytes, and (UTF-8) characters.
 * ===========================================================================
 *
 * The counting itself is a tiny state machine, but two details trip people up:
 *
 *   1. WORDS span buffer boundaries. If you read in 64 KiB chunks, a word that
 *      straddles two reads must not be counted twice (or zero times). The fix
 *      is to carry the "are we currently inside a word?" bit ACROSS chunks.
 *      `struct wc_state.in_word` is that carry.
 *
 *   2. CHARS != BYTES for multibyte text. In UTF-8 a code point is 1–4 bytes;
 *      the trailing bytes are "continuation bytes" of the form 0b10xxxxxx. So
 *      the character count is "bytes that are NOT continuation bytes." (True
 *      locale-aware `wc -m` decodes with mbrtowc; we do UTF-8 directly, which
 *      is correct for the ~universal UTF-8 locale and is the instructive case.)
 *
 * The pure counting routine `wc_count` is extracted verbatim into asm/demo.c
 * and annotated — it is this project's "core logic" for the assembly deliverable.
 *
 * Flags: -l lines, -w words, -c bytes, -m chars. Default = -l -w -c. With
 * multiple files a trailing "total" line is printed, like GNU wc.
 * =========================================================================== */
#include "coreutils.h"

#include <fcntl.h>    /* open, O_RDONLY                                        */
#include <stdint.h>   /* uint64_t                                             */
#include <stdio.h>    /* printf                                               */
#include <string.h>   /* strcmp                                               */
#include <unistd.h>   /* read, close, STDIN_FILENO                            */

/* Running totals for one stream. All 64-bit: a 4 GiB log has >4e9 bytes, so
 * 32-bit counters would wrap. */
struct wc_state {
    uint64_t lines;   /* count of '\n'                                          */
    uint64_t words;   /* maximal runs of non-whitespace                         */
    uint64_t chars;   /* UTF-8 code points                                      */
    uint64_t bytes;   /* raw bytes                                              */
    int      in_word; /* CARRY: were we inside a word at the last byte?         */
};

/* ASCII whitespace, matching GNU wc's word delimiters in the C/UTF-8 locale.
 * Kept as a separate function so the extracted asm/demo.c is identical. */
static int wc_is_space(unsigned char c)
{
    return c == ' '  || c == '\t' || c == '\n' ||
           c == '\r' || c == '\v' || c == '\f';
}

/* ---------------------------------------------------------------------------
 * wc_count — fold one chunk of `n` bytes into the running state. THIS is the
 * routine mirrored in asm/demo.c. It is branch-heavy on purpose: reading its
 * assembly shows how the compiler lays out the word-boundary test and the
 * continuation-byte mask.
 *
 * Word logic: a word STARTS on the first non-space after a space (or at the
 * start of input). We only ++words on that transition, driven by `in_word`,
 * which persists in *st between calls so cross-chunk words count once.
 *
 * Char logic: (c & 0xC0) != 0x80 is true for every byte EXCEPT a UTF-8
 * continuation byte (10xxxxxx), i.e. exactly once per code point.
 * ------------------------------------------------------------------------- */
static void wc_count(struct wc_state *st, const unsigned char *buf, uint64_t n)
{
    uint64_t lines = 0, words = 0, chars = 0;   /* locals: let the compiler keep */
    int in_word = st->in_word;                  /* the hot state in registers    */

    for (uint64_t i = 0; i < n; i++) {
        unsigned char c = buf[i];
        if (c == '\n')
            lines++;
        if ((c & 0xC0) != 0x80)                 /* not a continuation byte       */
            chars++;
        if (wc_is_space(c)) {
            in_word = 0;                        /* leaving / staying out of word */
        } else if (!in_word) {
            in_word = 1;                        /* rising edge: a new word starts*/
            words++;
        }
    }

    st->lines += lines;
    st->words += words;
    st->chars += chars;
    st->bytes += n;
    st->in_word = in_word;                      /* persist the carry             */
}

/* Which columns to print, in wc's fixed order: lines, words, chars, bytes. */
struct wcopts { int l, w, c, m; };

/* Count an open fd into `st`. Uses the robust read loop; returns 0 or -1. */
static int wc_fd(int fd, struct wc_state *st)
{
    unsigned char buf[64 * 1024];
    for (;;) {
        ssize_t r = read_retry(fd, buf, sizeof buf);
        if (r < 0) return -1;
        if (r == 0) return 0;                   /* EOF                          */
        wc_count(st, buf, (uint64_t)r);
    }
}

/* Print the selected columns for one line of output. GNU wc right-justifies in
 * a width that grows with the numbers; we use a fixed, readable 7-wide field. */
static void wc_print(const struct wc_state *s, struct wcopts o, const char *name)
{
    if (o.l) printf(" %7ju", (uintmax_t)s->lines);
    if (o.w) printf(" %7ju", (uintmax_t)s->words);
    if (o.m) printf(" %7ju", (uintmax_t)s->chars);
    if (o.c) printf(" %7ju", (uintmax_t)s->bytes);
    if (name) printf(" %s", name);
    putchar('\n');
}

int wc_main(int argc, char **argv)
{
    struct wcopts o = {0, 0, 0, 0};
    int any_opt = 0;

    /* Parse flags. -l -w -c -m may be bundled (-lwc). */
    int argi = 1;
    for (; argi < argc; argi++) {
        const char *a = argv[argi];
        if (a[0] != '-' || a[1] == '\0') break;      /* operand or "-" (stdin)   */
        if (strcmp(a, "--") == 0) { argi++; break; }
        for (const char *p = a + 1; *p; p++) {
            switch (*p) {
            case 'l': o.l = 1; any_opt = 1; break;
            case 'w': o.w = 1; any_opt = 1; break;
            case 'c': o.c = 1; any_opt = 1; break;
            case 'm': o.m = 1; any_opt = 1; break;
            default: diex("unknown option -- '%c'", *p);
            }
        }
    }
    /* Default selection when no count flag was given: lines, words, bytes. */
    if (!any_opt) { o.l = o.w = o.c = 1; }

    struct wc_state total = {0, 0, 0, 0, 0};
    int nfiles = argc - argi;
    int status = 0;

    /* No file operands: count stdin, no name column. */
    if (nfiles <= 0) {
        struct wc_state st = {0, 0, 0, 0, 0};
        if (wc_fd(STDIN_FILENO, &st) < 0) { warn("stdin"); return 1; }
        wc_print(&st, o, NULL);
        return 0;
    }

    for (; argi < argc; argi++) {
        const char *name = argv[argi];
        int fd;
        if (name[0] == '-' && name[1] == '\0') { fd = STDIN_FILENO; name = "-"; }
        else {
            fd = open(name, O_RDONLY);
            if (fd < 0) { warn("%s", name); status = 1; continue; }
        }

        /* Fresh per-file state: crucially, in_word resets to 0 for every file,
         * so a word cannot "bleed" from the end of one file into the start of
         * the next — each file's counts stand alone, exactly like GNU wc. */
        struct wc_state st = {0, 0, 0, 0, 0};
        if (wc_fd(fd, &st) < 0) { warn("%s", name); status = 1; }
        else                     wc_print(&st, o, name);

        /* Fold this file's counts into the grand total. We deliberately do NOT
         * touch total.in_word: the "total" row is a pure sum of the per-file
         * numbers, not a re-run of the state machine over the concatenation. */
        total.lines += st.lines;
        total.words += st.words;
        total.chars += st.chars;
        total.bytes += st.bytes;

        if (fd != STDIN_FILENO) close(fd);   /* never close the shared stdin    */
    }

    /* GNU wc prints a total line only when more than one file was named. */
    if (nfiles > 1)
        wc_print(&total, o, "total");
    return status;
}
