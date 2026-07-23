/* ===========================================================================
 * demo.c — the pure-logic CORE of `wc`, extracted for the assembly deliverable.
 * ===========================================================================
 *
 * WHY THIS FILE EXISTS
 * --------------------
 * The real applets (cat.c, cp.c, ls.c, wc.c, xargs.c, tailf.c) all pull in
 * Linux system headers (<sys/inotify.h>, <sys/syscall.h>, ...) that are NOT
 * present when we cross-compile to the Linux target from another host. So they
 * cannot be compiled to assembly here. This file is the standard substitution:
 * a SELF-CONTAINED extraction of the project's single most instructive routine —
 * the `wc` counting state machine — with NO #includes and its own typedefs.
 *
 * It is byte-for-byte the same algorithm as wc.c's wc_count(): the assembly you
 * read in demo.s / demo.annotated.s is genuinely the code that counts your text.
 *
 * WHAT THE ALGORITHM DEMONSTRATES (the "everyone gets it wrong" details)
 * ---------------------------------------------------------------------
 *   - WORDS across buffer boundaries: `in_word` is carried in *st between calls
 *     so a word split by a chunk boundary is counted exactly once.
 *   - CHARS vs BYTES: a UTF-8 code point is 1–4 bytes; the continuation bytes
 *     match 0b10xxxxxx. "chars" counts bytes that are NOT continuation bytes.
 *   - LINES: just the count of '\n'.
 *
 * Compile to teaching assembly with the exact commands in the project README /
 * Makefile `asm` target (clang --target=x86_64-pc-linux-gnu ...).
 * =========================================================================== */

/* Our own fixed-width types — no <stdint.h>, so the file is freestanding. On
 * the LP64 model Linux uses for x86-64, `unsigned long` is 64 bits. */
typedef unsigned long  u64;
typedef unsigned char  u8;

/* Running totals plus the cross-chunk carry. Mirrors wc.c's struct wc_state. */
struct wc_state {
    u64 lines;    /* number of '\n' bytes                                       */
    u64 words;    /* number of maximal non-whitespace runs                      */
    u64 chars;    /* number of UTF-8 code points                                */
    u64 bytes;    /* number of raw bytes                                        */
    int in_word;  /* CARRY: were we inside a word at the previous byte?         */
};

/* ASCII whitespace = wc's word delimiters in the C/UTF-8 locale. Marked static
 * so at -O1/-O2 the optimizer can (and does) inline it into wc_count, which the
 * annotated assembly points out. */
static int wc_is_space(u8 c)
{
    return c == ' '  || c == '\t' || c == '\n' ||
           c == '\r' || c == '\v' || c == '\f';
}

/* ---------------------------------------------------------------------------
 * wc_count — fold `n` bytes of `buf` into *st. Pure computation: no syscalls,
 * no allocation, no globals. Perfect for reading as assembly.
 *
 * The locals (lines/words/chars/in_word) let the compiler keep the hot state in
 * registers across the loop and write back to memory only once at the end —
 * watch that happen in demo.annotated.s.
 * ------------------------------------------------------------------------- */
void wc_count(struct wc_state *st, const u8 *buf, u64 n)
{
    u64 lines = 0, words = 0, chars = 0;
    int in_word = st->in_word;

    for (u64 i = 0; i < n; i++) {
        u8 c = buf[i];

        if (c == '\n')                 /* newline => one more line              */
            lines++;

        if ((c & 0xC0) != 0x80)        /* NOT a 10xxxxxx continuation byte      */
            chars++;                   /*   => the first byte of a code point   */

        if (wc_is_space(c)) {
            in_word = 0;               /* whitespace: we are outside a word     */
        } else if (!in_word) {
            in_word = 1;               /* rising edge: a fresh word begins      */
            words++;
        }
        /* else: still inside the same word — count nothing. */
    }

    st->lines += lines;
    st->words += words;
    st->chars += chars;
    st->bytes += n;
    st->in_word = in_word;             /* hand the carry to the next chunk      */
}
