/* ===========================================================================
 * parser.c — the TARGET: a tiny record parser with a deliberately planted bug.
 * ===========================================================================
 *
 * >>> This is a DELIBERATELY VULNERABLE program. Compile and fuzz it ONLY on
 * >>> your own machine. It exists so the fuzzer has something to find. <<<
 *
 * The parser reads a made-up "FZR1" record format:
 *
 *      offset  field       meaning
 *      ------  -----       -------
 *        0..3  magic       must be the 4 bytes  'F' 'Z' 'R' '1'
 *           4  type        1 = "name" record, 2 = "blob" record
 *           5  length      how many body bytes follow (attacker-controlled)
 *        6..   body        `length` bytes
 *
 * THE PLANTED BUG (type 1, "name"): the body is copied into a fixed 16-byte
 * stack buffer using the attacker's `length` with NO bounds check — a classic
 * stack buffer overflow (CWE-121). Any input whose length byte is > 16 smashes
 * the buffer; a large enough length walks off the stack frame and corrupts the
 * saved return address / stack canary region.
 *
 * WHY THE MAGIC-BYTE LADDER MATTERS FOR THE DEMO
 * ----------------------------------------------
 * The bug sits behind four exact magic bytes and a type check. A dumb random
 * fuzzer would need to guess 'F','Z','R','1' simultaneously — about 1 in 2^32.
 * A COVERAGE-GUIDED fuzzer instead gets a reward signal for each byte it gets
 * right: matching 'F' opens a NEW edge, so that input is saved and mutated
 * further; then 'Z' opens another, and so on. The fuzzer climbs the ladder one
 * rung at a time and reaches the vulnerable copy in seconds. Watching it do
 * that is the entire point of this project — feedback turns an exponential
 * search into a linear one.
 *
 * DEFENSE / WHAT SAVES YOU (see the Makefile's two build modes):
 *   - STACK CANARY (-fstack-protector-strong): the overflow overwrites a random
 *     guard word; on return the canary check fails and the process aborts via
 *     __stack_chk_fail -> SIGABRT. The fuzzer still reports a crash, but it is a
 *     CONTROLLED abort, not an attacker-controlled hijack.
 *   - ASan (-fsanitize=address): catches the very first out-of-bounds byte with
 *     a precise report ("stack-buffer-overflow ... in parse_name") — the ideal
 *     triage output. Build `make asan` and point the fuzzer at that binary.
 *   - The deliberately WEAK build (-fno-stack-protector) exists so you can see
 *     the raw SIGSEGV with no mitigation. That is the "vuln" build; the README
 *     is explicit that it is for your own analysis only.
 * ===========================================================================
 */

#include <stddef.h>     /* size_t                                             */
#include <stdint.h>     /* uint8_t                                            */
#include <unistd.h>     /* read, close                                        */
#include <fcntl.h>      /* open, O_RDONLY                                     */

#include "forkserver.h"

/* Largest input we will read from the test-case file. The fuzzer never needs to
 * feed more than this to exercise the parser; capping the read keeps the child
 * cheap. */
#define MAX_INPUT 4096

/* ---------------------------------------------------------------------------
 * A tiny "sink" so the optimizer cannot delete our buffer as dead code.
 *
 * Without a use, an aggressive compiler could prove `buf` is never read and
 * elide the whole copy — deleting the bug and the coverage with it. Writing the
 * buffer through a volatile-ish global forces the store to be observable, so
 * the vulnerable path (and its instrumentation) survives at any -O level.
 * `volatile` tells the compiler another agent may observe this memory, so it
 * must not optimise the writes away. */
volatile uint8_t g_sink;

/* ---------------------------------------------------------------------------
 * parse_name — the vulnerable routine. type-1 records land here.
 *
 * `len` is the attacker-controlled length byte (0..255). We copy that many body
 * bytes into a 16-byte stack buffer. The MISSING check `if (len > sizeof buf)`
 * is the bug. Each iteration and the branch below are separate basic blocks, so
 * the fuzzer earns fresh coverage as it drives `len` upward — another rung of
 * the ladder that leads it straight to the overflow.
 * --------------------------------------------------------------------------- */
static void parse_name(const uint8_t *body, uint8_t len)
{
    char buf[16];                         /* 16-byte stack buffer — the victim */

    /* BUG: no `len <= sizeof(buf)` guard. When len > 16 this writes past `buf`,
     * over the compiler's canary slot and then the saved frame — CWE-121. */
    for (uint8_t i = 0; i < len; i++) {
        buf[i] = (char)body[i];           /* out-of-bounds store once i >= 16  */
    }

    /* Force the buffer to be "used" so the copy is not optimised out. Reading
     * buf[len-1] also nudges the compiler to keep the whole write live. */
    g_sink = (uint8_t)buf[len ? len - 1 : 0];
}

/* ---------------------------------------------------------------------------
 * parse_blob — a benign second record type, present so the parser has real
 * branching (more edges = a more interesting coverage surface to explore).
 * It just XOR-checksums the body; no bug here. This is the "keep going, this
 * path is safe" arm the fuzzer will also map out.
 * --------------------------------------------------------------------------- */
static void parse_blob(const uint8_t *body, uint8_t len)
{
    uint8_t sum = 0;
    for (uint8_t i = 0; i < len; i++) sum ^= body[i];
    g_sink = sum;
}

/* ---------------------------------------------------------------------------
 * parse — the record dispatcher. Every early `return` below is a distinct edge,
 * so the coverage map literally shows the fuzzer how far into the format it got.
 * --------------------------------------------------------------------------- */
static void parse(const uint8_t *data, size_t n)
{
    if (n < 6) return;                    /* too short for header+len: edge #1 */

    /* The magic-byte ladder. Each mismatch is its own edge, so matching one
     * more byte than any prior input == "new coverage" == input worth keeping. */
    if (data[0] != 'F') return;           /* rung 1                            */
    if (data[1] != 'Z') return;           /* rung 2                            */
    if (data[2] != 'R') return;           /* rung 3                            */
    if (data[3] != '1') return;           /* rung 4 — magic fully matched      */

    uint8_t type = data[4];
    uint8_t len  = data[5];
    const uint8_t *body = data + 6;

    /* Do not read past what we actually loaded. (This bounds the *read*; the
     * planted bug is on the *write* side inside parse_name.) */
    if ((size_t)len > n - 6) len = (uint8_t)(n - 6);

    switch (type) {
        case 1: parse_name(body, len); break;   /* -> the vulnerable path      */
        case 2: parse_blob(body, len); break;   /* -> the safe path            */
        default: return;                        /* unknown type: another edge  */
    }
}

/* ---------------------------------------------------------------------------
 * read_all — slurp up to MAX_INPUT bytes from a file descriptor into `buf`.
 * read(2) can return short (esp. from pipes), so we loop until EOF or full.
 * Returns the number of bytes read. Every syscall's result is checked — an
 * unchecked read is how fuzzers "find" phantom bugs that are really I/O races.
 * --------------------------------------------------------------------------- */
static size_t read_all(int fd, uint8_t *buf, size_t cap)
{
    size_t total = 0;
    while (total < cap) {
        ssize_t r = read(fd, buf + total, cap - total);
        if (r < 0)  return total;         /* error: stop, report what we got   */
        if (r == 0) break;                /* EOF                               */
        total += (size_t)r;
    }
    return total;
}

/* ---------------------------------------------------------------------------
 * main — target entry point.
 *
 * The ORDER here is the fork-server contract in miniature:
 *   1. __afl_start_forkserver(): the parent parks and forks per test case;
 *      only a freshly-forked child returns from this call.
 *   2. (child only) open the test-case file the fuzzer just wrote, read it,
 *      parse it, exit. A crash in parse() dies via signal, which the fork
 *      server reaps and reports to the fuzzer as `status`.
 *
 * argv[1] is the input path the fuzzer passes (its `.cur_input`). If run by
 * hand with a filename, it works standalone (forkserver returns immediately
 * because there is no fork-server pipe). With no arg, it reads stdin.
 * --------------------------------------------------------------------------- */
int main(int argc, char **argv)
{
    /* Everything above the fork point runs ONCE for all test cases. There is no
     * expensive init in this toy target, but in a real one (loading a grammar,
     * mmapping a font file, building a DFA) this is where the savings live. */
    __afl_start_forkserver();

    /* --- from here on we are the per-test-case child (or a standalone run) --- */

    static uint8_t input[MAX_INPUT];      /* static: not on the tiny stack     */
    size_t n;

    if (argc > 1) {
        int fd = open(argv[1], O_RDONLY);
        if (fd < 0) return 0;             /* no file this round: nothing to do */
        n = read_all(fd, input, sizeof input);
        close(fd);
    } else {
        n = read_all(0, input, sizeof input);   /* fd 0 = stdin               */
    }

    parse(input, n);
    return 0;                             /* clean exit: status = WIFEXITED,0  */
}
