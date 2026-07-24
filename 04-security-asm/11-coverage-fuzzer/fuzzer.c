/* ===========================================================================
 * fuzzer.c — the fuzzer engine (this repo's tiny "afl-fuzz").
 * ===========================================================================
 *
 * This is the driver process. It owns the coverage bitmap, launches the target
 * as a fork server, and then loops forever: pick an input from the corpus,
 * MUTATE it, run it through the fork server, look at the coverage the run
 * produced, and KEEP the mutation if it reached an edge no earlier input did.
 * Crashes get saved for triage. That feedback loop — "keep what reaches new
 * code" — is the entire idea of coverage-guided fuzzing.
 *
 * The pieces, in reading order below:
 *   - PRNG           : a fast, seedable xorshift so runs are reproducible.
 *   - shared memory  : shmget/shmat the 64 KiB coverage bitmap.
 *   - fork server    : fork+exec the target once, speak the 198/199 protocol.
 *   - feedback       : classify_counts() + has_new_bits() decide "is this new?".
 *   - mutation       : bit/byte flips, arithmetic, interesting values, block
 *                      ops, splicing, and dictionary injection.
 *   - main loop      : the fuzz_one() cycle + crash saving + a stats line.
 *
 * PLATFORM: Linux (fork, SysV shm, waitpid, poll). Build with `make`; see the
 * README for a WSL/Linux run. It is intentionally single-process/single-target
 * — a teaching core, not AFL's full scheduler.
 * ===========================================================================
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <dirent.h>
#include <poll.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "forkserver.h"

/* Largest single test case the fuzzer will build or hold in a scratch buffer.
 * Bounds every insert/splice so mutations can never overrun a stack buffer. */
#define MAX_INPUT_CAP 8192

/* =====================================================================
 * 0. Small typedefs and a deterministic PRNG.
 * ===================================================================== */
typedef uint8_t  u8;
typedef uint32_t u32;
typedef uint64_t u64;

/* xorshift64*: tiny, fast, and — crucially for a fuzzer — SEEDABLE. A fixed
 * seed makes an entire campaign replayable, which is how you re-derive the
 * exact mutation that produced a crash. `state` must never be 0. */
static u64 rng_state = 0x2545F4914F6CDD1DULL;
static inline u64 rng_next(void)
{
    u64 x = rng_state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    rng_state = x;
    return x * 0x2545F4914F6CDD1DULL;
}
/* Uniform-ish integer in [0, n). Fine for fuzzing (we do not need crypto-grade
 * uniformity — we need speed and reproducibility). */
static inline u32 rng_below(u32 n) { return n ? (u32)(rng_next() % n) : 0; }

/* =====================================================================
 * 1. Global fuzzer state.
 * ===================================================================== */

/* trace_bits: the shared coverage bitmap, attached from the SAME SysV segment
 * the target writes. After each run we read the target's edge hits straight out
 * of here — no copy, no IPC message, just shared pages. */
static u8 *trace_bits = NULL;
static int shm_id = -1;

/* virgin_bits: the fuzzer's running memory of "every hit-count class ever seen,
 * per edge". Initialised to 0xFF (all bits still virgin/unseen). A run is
 * interesting iff it clears a bit here that was still set — see has_new_bits().
 * This is how the fuzzer accumulates coverage across the whole campaign. */
static u8 virgin_bits[MAP_SIZE];

/* Fork-server plumbing: the two pipe ends we keep, plus the server's pid. */
static int fsrv_ctl_fd = -1;    /* we WRITE "go" here          -> target 198  */
static int fsrv_st_fd  = -1;    /* we READ pid/status here     <- target 199  */
static pid_t fsrv_pid  = -1;

/* Where we write the current test case; passed to the target as argv[1]. */
static char cur_input_path[512];

/* Output dirs and counters for the stats line. */
static char out_dir[400];
static u64  total_execs = 0;
static u32  queued      = 0;    /* corpus entries (inputs kept for new cov)   */
static u32  crashes     = 0;
static u32  hangs       = 0;

/* Execution timeout in milliseconds — a target that runs longer is a "hang". */
static int exec_tmout_ms = 1000;

/* =====================================================================
 * 2. The corpus (a.k.a. the queue).
 *
 * Each entry is one input we decided to keep because it reached new coverage.
 * Kept simple: a growable array of {bytes, len}. AFL layers a scheduler on top
 * (favored entries, energy assignment); we just round-robin, which is enough to
 * demonstrate the feedback loop.
 * ===================================================================== */
typedef struct { u8 *data; size_t len; } Testcase;
static Testcase *corpus = NULL;
static size_t    corpus_n = 0, corpus_cap = 0;

static void corpus_add(const u8 *data, size_t len)
{
    if (corpus_n == corpus_cap) {
        corpus_cap = corpus_cap ? corpus_cap * 2 : 64;
        corpus = realloc(corpus, corpus_cap * sizeof *corpus);
        if (!corpus) { perror("realloc"); exit(1); }
    }
    u8 *copy = malloc(len ? len : 1);   /* own a private copy; caller's buffer
                                         * is reused by the mutator next tick  */
    if (!copy) { perror("malloc"); exit(1); }
    memcpy(copy, data, len);
    corpus[corpus_n].data = copy;
    corpus[corpus_n].len  = len;
    corpus_n++;
    queued++;
}

/* =====================================================================
 * 3. Dictionary (optional): user-supplied tokens the mutator can splice in.
 *
 * Dictionaries are how you feed a fuzzer domain knowledge it cannot discover by
 * chance — magic values, keywords, checksummed headers. For THIS target the
 * token "FZR1" lets the fuzzer paste the whole magic in one move; but note the
 * coverage feedback finds it even WITHOUT the dictionary, one byte at a time.
 * ===================================================================== */
#define MAX_DICT 256
static u8    *dict_tok[MAX_DICT];
static size_t dict_len[MAX_DICT];
static size_t dict_n = 0;

static void dict_add(const u8 *tok, size_t len)
{
    if (dict_n >= MAX_DICT || len == 0) return;
    u8 *c = malloc(len);
    if (!c) return;
    memcpy(c, tok, len);
    dict_tok[dict_n] = c;
    dict_len[dict_n] = len;
    dict_n++;
}

/* Load a dictionary file: one token per line, either raw text or the AFL-style
 * name="..." with \xHH escapes. We accept a permissive subset: everything after
 * the first '"' up to the last '"', with \xHH and \\ decoded. Lines without a
 * quote are taken verbatim (trimmed of trailing newline). */
static void dict_load(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) return;
    char line[1024];
    while (fgets(line, sizeof line, f)) {
        size_t L = strlen(line);
        while (L && (line[L-1] == '\n' || line[L-1] == '\r')) line[--L] = 0;
        if (L == 0 || line[0] == '#') continue;

        char *q1 = strchr(line, '"');
        u8 buf[512]; size_t bn = 0;
        if (q1) {
            for (char *p = q1 + 1; *p && bn < sizeof buf; p++) {
                if (*p == '"') break;
                if (p[0] == '\\' && p[1] == 'x' && p[2] && p[3]) { /* \xHH     */
                    int hi = p[2], lo = p[3];
                    #define HEX(c) ((c)>='0'&&(c)<='9'?(c)-'0': \
                                     (c)>='a'&&(c)<='f'?(c)-'a'+10: \
                                     (c)>='A'&&(c)<='F'?(c)-'A'+10:0)
                    buf[bn++] = (u8)((HEX(hi) << 4) | HEX(lo));
                    #undef HEX
                    p += 3;
                } else if (p[0] == '\\') {
                    buf[bn++] = (u8)p[1]; p++;
                } else {
                    buf[bn++] = (u8)*p;
                }
            }
        } else {
            for (size_t i = 0; i < L && bn < sizeof buf; i++) buf[bn++] = (u8)line[i];
        }
        dict_add(buf, bn);
    }
    fclose(f);
    printf("[*] loaded %zu dictionary tokens from %s\n", dict_n, path);
}

/* =====================================================================
 * 4. Coverage feedback: classify_counts + has_new_bits.
 *
 * The target stores RAW per-edge hit counts (0..255). Raw counts are too noisy
 * to compare directly: an edge hit 19 times vs 20 times is not a meaningfully
 * "new" behaviour. AFL buckets counts into hit-count CLASSES — 1, 2, 3, 4-7,
 * 8-15, 16-31, 32-127, 128+ — each a distinct bit. Two runs differ meaningfully
 * only if they land in different classes. This is the SAME logic hand-annotated
 * in asm/demo.c; it lives there as the self-contained teaching extraction.
 * ===================================================================== */
static u8 count_class_lookup8[256];
static void init_count_class_lookup(void)
{
    /* Map each raw count to a single-bit class. One-hot bits so that OR-ing a
     * run's classes into virgin_bits accumulates "which classes we've seen". */
    count_class_lookup8[0] = 0;
    count_class_lookup8[1] = 1;          /* class: hit once                    */
    count_class_lookup8[2] = 2;          /* class: hit twice                   */
    count_class_lookup8[3] = 4;          /* class: 3                           */
    for (int i = 4;   i <= 7;   i++) count_class_lookup8[i] = 8;    /* 4-7      */
    for (int i = 8;   i <= 15;  i++) count_class_lookup8[i] = 16;   /* 8-15     */
    for (int i = 16;  i <= 31;  i++) count_class_lookup8[i] = 32;   /* 16-31    */
    for (int i = 32;  i <= 127; i++) count_class_lookup8[i] = 64;   /* 32-127   */
    for (int i = 128; i <= 255; i++) count_class_lookup8[i] = 128;  /* 128+     */
}
static void classify_counts(u8 *map)
{
    for (u32 i = 0; i < MAP_SIZE; i++) map[i] = count_class_lookup8[map[i]];
}

/* has_new_bits: after classify, does `trace` set any class bit that virgin
 * still has as 1 (unseen)? If so this run reached genuinely new behaviour; we
 * clear those bits in virgin (now "seen") and return 1 so the caller keeps the
 * input. This mirrors AFL's has_new_bits exactly. */
static int has_new_bits(u8 *trace)
{
    int is_new = 0;
    for (u32 i = 0; i < MAP_SIZE; i++) {
        u8 t = trace[i];
        if (t && (virgin_bits[i] & t)) {   /* a class bit set here, unseen     */
            virgin_bits[i] &= (u8)~t;       /* mark it seen                     */
            is_new = 1;
        }
    }
    return is_new;
}

/* =====================================================================
 * 5. Shared memory + fork server setup.
 * ===================================================================== */

/* Create the coverage bitmap as a SysV shared-memory segment and attach it.
 * We pass its id to the target via the environment (SHM_ENV_VAR). IPC_PRIVATE
 * makes a fresh anonymous segment; 0600 keeps it to our uid. We mark it for
 * deletion after attach paths are set up in cleanup. */
static void setup_shm(void)
{
    shm_id = shmget(IPC_PRIVATE, MAP_SIZE, IPC_CREAT | IPC_EXCL | 0600);
    if (shm_id < 0) { perror("shmget"); exit(1); }

    trace_bits = shmat(shm_id, NULL, 0);
    if (trace_bits == (void *)-1) { perror("shmat"); exit(1); }

    /* Export the id so the target's runtime (rt.c) can attach the same segment. */
    char buf[32];
    snprintf(buf, sizeof buf, "%d", shm_id);
    setenv(SHM_ENV_VAR, buf, 1);

    memset(virgin_bits, 0xFF, sizeof virgin_bits);   /* nothing seen yet       */
}

/* Remove the shm segment on exit so we do not leak kernel IPC objects (they
 * outlive the process otherwise — check `ipcs -m`). Registered with atexit()
 * AND wired to SIGINT/SIGTERM below, because the normal way to stop a fuzzer is
 * Ctrl-C, which by default would skip atexit handlers and leak the segment. */
static void remove_shm(void)
{
    if (shm_id >= 0) shmctl(shm_id, IPC_RMID, NULL);
}

/* Signal handler: shmctl(IPC_RMID) and _exit are async-signal-safe, so it is
 * legal to tear the segment down from here. We also SIGKILL the fork server so
 * it does not linger as an orphan after we go. */
static void on_signal(int sig)
{
    (void)sig;
    remove_shm();
    if (fsrv_pid > 0) kill(fsrv_pid, SIGKILL);
    _exit(0);
}

/* Launch the target as a fork server:
 *   - make two pipes (control fuzzer->target, status target->fuzzer),
 *   - fork; in the child dup2 the pipe ends onto the agreed fds 198/199 and
 *     execv the target, which will call __afl_start_forkserver(),
 *   - in the parent, read the 4-byte HELLO to confirm the server is up.
 */
static void init_forkserver(char *target_path)
{
    int ctl_pipe[2], st_pipe[2];
    if (pipe(ctl_pipe) || pipe(st_pipe)) { perror("pipe"); exit(1); }

    fsrv_pid = fork();
    if (fsrv_pid < 0) { perror("fork"); exit(1); }

    if (fsrv_pid == 0) {
        /* ---- CHILD: become the target with the pipes on the fixed fds. ---- */
        /* dup2(oldfd, newfd): make newfd refer to the same open file as oldfd.
         * The target reads its "go" from FORKSRV_CTL_FD and writes status to
         * FORKSRV_ST_FD, so we bind the correct pipe ENDS there:
         *   - target reads control -> it needs the READ end of ctl_pipe,
         *   - target writes status -> it needs the WRITE end of st_pipe. */
        if (dup2(ctl_pipe[0], FORKSRV_CTL_FD) < 0) { perror("dup2"); _exit(1); }
        if (dup2(st_pipe[1],  FORKSRV_ST_FD)  < 0) { perror("dup2"); _exit(1); }

        /* Close every original pipe fd; the dups above are what survive. Leaving
         * the wrong end open would make EOF detection (which tells the server to
         * exit) never fire. */
        close(ctl_pipe[0]); close(ctl_pipe[1]);
        close(st_pipe[0]);  close(st_pipe[1]);

        char *argv[] = { target_path, cur_input_path, NULL };
        execv(target_path, argv);
        perror("execv");                 /* only reached if exec failed        */
        _exit(1);
    }

    /* ---- PARENT: keep the ends WE use, close the target's ends. ---- */
    fsrv_ctl_fd = ctl_pipe[1];           /* we write "go" into the control pipe */
    fsrv_st_fd  = st_pipe[0];            /* we read status out of the st pipe   */
    close(ctl_pipe[0]);
    close(st_pipe[1]);

    /* Block until the server announces itself. Receiving the 4-byte HELLO proves
     * the target mapped the shm and parked in the fork loop. If we instead get
     * EOF/short read, the target died during init (missing shm? bad binary?). */
    u32 hello = 0;
    ssize_t r = read(fsrv_st_fd, &hello, 4);
    if (r != 4) {
        fprintf(stderr, "[!] fork server did not come up (read %zd bytes). "
                        "Is the target instrumented and linked with rt.c?\n", r);
        exit(1);
    }
    printf("[+] fork server is up (pid %d)\n", (int)fsrv_pid);
}

/* =====================================================================
 * 6. Running one test case through the fork server.
 * ===================================================================== */

/* Result codes from a single run. */
enum { R_OK = 0, R_CRASH = 1, R_HANG = 2, R_ERROR = 3 };

/* Write the candidate bytes to the test-case file the target will open. We
 * truncate first so a shorter input this round does not leave stale trailing
 * bytes from a longer one. */
static void write_testcase(const u8 *data, size_t len)
{
    int fd = open(cur_input_path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0) { perror("open cur_input"); exit(1); }
    if (write(fd, data, len) != (ssize_t)len) { perror("write cur_input"); }
    close(fd);
}

/* run_target: the core of the loop. Zero the coverage map, tell the server to
 * run one case, wait for the child's status (with a hang timeout), and classify
 * whether it crashed. The map is left populated for the caller to inspect. */
static int run_target(const u8 *data, size_t len)
{
    write_testcase(data, len);

    /* Zero the bitmap so it reflects ONLY this run. memset of 64 KiB is a few
     * microseconds — negligible next to a fork+exec, which is exactly why the
     * shared-map design wins. */
    memset(trace_bits, 0, MAP_SIZE);

    /* Tell the fork server to run one case: any 4 bytes; arrival is the signal. */
    u32 go = 1;
    if (write(fsrv_ctl_fd, &go, 4) != 4) { fprintf(stderr, "[!] ctl write failed\n"); return R_ERROR; }

    /* The server replies with the child's pid first. */
    pid_t child = 0;
    if (read(fsrv_st_fd, &child, 4) != 4) { fprintf(stderr, "[!] no child pid\n"); return R_ERROR; }

    /* Wait for the status word, but not forever: poll() with a timeout turns a
     * hung/infinite-loop target into a detectable "hang" instead of freezing the
     * whole campaign. If it times out we SIGKILL the child; the server's blocked
     * waitpid() then reaps it and hands us the (killed) status. */
    struct pollfd pfd = { .fd = fsrv_st_fd, .events = POLLIN };
    int pr = poll(&pfd, 1, exec_tmout_ms);
    int timed_out = 0;
    if (pr == 0) {
        kill(child, SIGKILL);            /* break the hang                     */
        timed_out = 1;
    }

    int status = 0;
    if (read(fsrv_st_fd, &status, 4) != 4) { fprintf(stderr, "[!] no status\n"); return R_ERROR; }

    total_execs++;

    if (timed_out) return R_HANG;

    /* Decode the wait status. WIFSIGNALED == died from a signal: SIGSEGV (raw
     * memory fault), SIGABRT (ASan / stack-canary trip), SIGBUS, SIGFPE, etc.
     * THAT is a crash — the whole reason we are here. WIFEXITED means a clean
     * (or handled) exit; not a crash however nonzero the code. */
    if (WIFSIGNALED(status)) {
        int sig = WTERMSIG(status);
        if (sig == SIGKILL) return R_HANG;   /* we killed it above             */
        return R_CRASH;
    }
    return R_OK;
}

/* =====================================================================
 * 7. Saving interesting inputs and crashes.
 * ===================================================================== */
static void save_to_dir(const char *sub, const char *tag, const u8 *data, size_t len)
{
    char path[600];
    snprintf(path, sizeof path, "%s/%s/%s_%llu", out_dir, sub, tag,
             (unsigned long long)total_execs);
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0) return;
    if (write(fd, data, len) != (ssize_t)len) { /* best effort */ }
    close(fd);
}

/* After a run, decide what to do with the input:
 *   - classify the raw counts into hit-count classes,
 *   - if it crashed, save it under crashes/ and count it,
 *   - if it reached NEW coverage, add it to the corpus and save under queue/.
 * Returns 1 if the input was added to the corpus (so the caller can note it). */
static int save_if_interesting(const u8 *data, size_t len, int res)
{
    classify_counts(trace_bits);          /* raw counts -> class bits          */

    if (res == R_CRASH) {
        /* De-dup by coverage: only save a crash that also reached new edges, so
         * we do not save ten thousand copies of the same overflow. (A fuller
         * fuzzer hashes the crashing stack; coverage is a decent proxy here.) */
        int newcov = has_new_bits(trace_bits);
        crashes++;
        save_to_dir("crashes", "id", data, len);
        if (newcov) return 0;             /* already recorded coverage-wise     */
        return 0;
    }
    if (res == R_HANG) { hangs++; save_to_dir("hangs", "id", data, len); return 0; }

    /* Non-crashing run: keep it iff it reached an edge/class we had not seen. */
    if (has_new_bits(trace_bits)) {
        corpus_add(data, len);
        save_to_dir("queue", "id", data, len);
        return 1;
    }
    return 0;
}

/* =====================================================================
 * 8. The mutation engine.
 *
 * Given a scratch buffer holding a copy of some corpus entry, apply a random
 * pile of "havoc" mutations in place and return the new length. These are the
 * classic AFL havoc operators. None of them understands the input format — the
 * COVERAGE FEEDBACK is what turns this blind noise into directed search.
 * ===================================================================== */

/* AFL's "interesting" values: boundary numbers that disproportionately trigger
 * bugs (signed/unsigned overflow edges, off-by-ones, NUL, max lengths). */
static const int8_t  interesting_8[]  = { -128, -1, 0, 1, 16, 32, 64, 100, 127 };
static const int16_t interesting_16[] = { -32768, -129, 128, 255, 256, 512,
                                          1000, 1024, 4096, 32767 };
static const int32_t interesting_32[] = { -2147483647-1, -100663046, -32769,
                                          32768, 65535, 65536, 100663045,
                                          2147483647 };

/* mutate_havoc: apply `rounds` random edits to buf[0..len), returning new len.
 * `cap` is the buffer capacity so insert/splice never overrun it. */
static size_t mutate_havoc(u8 *buf, size_t len, size_t cap, u32 rounds)
{
    for (u32 r = 0; r < rounds; r++) {
        /* 15 operator classes; pick one uniformly. Each is a documented AFL
         * havoc stage. Guards keep every index in-bounds. */
        switch (rng_below(15)) {

        case 0: /* single BIT flip: the finest-grained mutation. Flipping one
                 * bit of a length or flag byte is often all it takes. */
            if (len) buf[rng_below((u32)len)] ^= (u8)(1u << rng_below(8));
            break;

        case 1: /* set a byte to a random value */
            if (len) buf[rng_below((u32)len)] = (u8)rng_next();
            break;

        case 2: /* set a byte to an "interesting" 8-bit value */
            if (len) buf[rng_below((u32)len)] =
                        (u8)interesting_8[rng_below(sizeof interesting_8)];
            break;

        case 3: /* write an interesting 16-bit value (little-endian) */
            if (len >= 2) {
                u32 p = rng_below((u32)len - 1);
                int16_t v = interesting_16[rng_below(sizeof interesting_16/2)];
                memcpy(buf + p, &v, 2);
            }
            break;

        case 4: /* write an interesting 32-bit value */
            if (len >= 4) {
                u32 p = rng_below((u32)len - 3);
                int32_t v = interesting_32[rng_below(sizeof interesting_32/4)];
                memcpy(buf + p, &v, 4);
            }
            break;

        case 5: /* ARITHMETIC: add/subtract a small delta from a byte. Catches
                 * off-by-one and boundary logic that pure bit-flipping misses. */
            if (len) {
                u32 p = rng_below((u32)len);
                buf[p] = (u8)(buf[p] + (int)rng_below(35) - 17);
            }
            break;

        case 6: /* 16-bit little-endian arithmetic */
            if (len >= 2) {
                u32 p = rng_below((u32)len - 1);
                uint16_t v; memcpy(&v, buf + p, 2);
                v = (uint16_t)(v + (int)rng_below(35) - 17);
                memcpy(buf + p, &v, 2);
            }
            break;

        case 7: /* 32-bit little-endian arithmetic */
            if (len >= 4) {
                u32 p = rng_below((u32)len - 3);
                uint32_t v; memcpy(&v, buf + p, 4);
                v = v + (u32)((int)rng_below(35) - 17);
                memcpy(buf + p, &v, 4);
            }
            break;

        case 8: case 9: /* overwrite a run of bytes with one repeated value
                         * (weight 2/15 — block overwrites are productive). */
            if (len) {
                u32 p = rng_below((u32)len);
                u32 n = 1 + rng_below((u32)(len - p));
                u8  v = (u8)rng_next();
                memset(buf + p, v, n);
            }
            break;

        case 10: /* delete a block: shrink the input. Smaller inputs run faster
                  * and can expose different paths (missing optional fields). */
            if (len > 1) {
                u32 n = 1 + rng_below((u32)len - 1);
                u32 p = rng_below((u32)len - n + 1);
                memmove(buf + p, buf + p + n, len - p - n);
                len -= n;
            }
            break;

        case 11: /* insert a block of random bytes (grow, if capacity allows) */
            if (len < cap) {
                u32 n = 1 + rng_below(16);
                if (n > cap - len) n = (u32)(cap - len);
                u32 p = rng_below((u32)len + 1);
                memmove(buf + p + n, buf + p, len - p);
                for (u32 i = 0; i < n; i++) buf[p + i] = (u8)rng_next();
                len += n;
            }
            break;

        case 12: /* clone an existing block elsewhere (self-splice within one
                  * input) — cheaply builds repeated structures. */
            if (len && len < cap) {
                u32 n = 1 + rng_below((u32)len);
                if (n > cap - len) n = (u32)(cap - len);
                u32 src = rng_below((u32)len);
                if (src + n > len) n = (u32)(len - src);
                u32 dst = rng_below((u32)len + 1);
                memmove(buf + dst + n, buf + dst, len - dst);
                memmove(buf + dst, buf + src + (dst <= src ? n : 0), n);
                len += n;
            }
            break;

        case 13: /* DICTIONARY injection: overwrite at a random offset with a
                  * user token. This is how domain knowledge (magic values,
                  * keywords) enters the search. No-op if no dictionary. */
            if (dict_n && len) {
                size_t d = rng_below((u32)dict_n);
                u32 p = rng_below((u32)len);
                size_t n = dict_len[d];
                if (p + n > cap) n = cap - p;
                memcpy(buf + p, dict_tok[d], n);
                if (p + n > len) len = p + n;
            }
            break;

        case 14: /* dictionary INSERT (grow): splice a token in, shifting the
                  * tail right. Lets the fuzzer *add* a magic header it lacks. */
            if (dict_n && len < cap) {
                size_t d = rng_below((u32)dict_n);
                size_t n = dict_len[d];
                if (n > cap - len) n = cap - len;
                u32 p = rng_below((u32)len + 1);
                memmove(buf + p + n, buf + p, len - p);
                memcpy(buf + p, dict_tok[d], n);
                len += n;
            }
            break;
        }
    }
    return len;
}

/* splice: cross-over two corpus inputs — take the head of `a` and the tail of
 * `b`, joined at a random point. Splicing escapes local minima the way genetic
 * crossover does: it recombines partial progress from two different lineages
 * (e.g. one input that matched the magic with one that has a big length byte). */
static size_t splice(const Testcase *a, const Testcase *b, u8 *out, size_t cap)
{
    if (a->len < 2 || b->len < 2) { size_t n = a->len < cap ? a->len : cap;
                                    memcpy(out, a->data, n); return n; }
    u32 split = 1 + rng_below((u32)(a->len < b->len ? a->len : b->len) - 1);
    size_t head = split;
    size_t tail = b->len - split;
    if (head + tail > cap) tail = cap - head;
    memcpy(out, a->data, head);
    memcpy(out + head, b->data + split, tail);
    return head + tail;
}

/* =====================================================================
 * 9. One fuzzing cycle: pick a seed, mutate, run, learn.
 * ===================================================================== */
static void fuzz_one(void)
{
    /* Round-robin seed selection: simple and fair. AFL weights toward small,
     * fast, coverage-rich entries; that scheduler is the main thing this
     * teaching core omits (README says so). */
    static size_t cursor = 0;
    if (corpus_n == 0) return;
    Testcase *seed = &corpus[cursor % corpus_n];
    cursor++;

    u8 scratch[MAX_INPUT_CAP];
    size_t len = seed->len;
    if (len > sizeof scratch) len = sizeof scratch;
    memcpy(scratch, seed->data, len);

    /* Occasionally splice with another random entry before havoc (crossover). */
    if (corpus_n >= 2 && rng_below(4) == 0) {
        Testcase *other = &corpus[rng_below((u32)corpus_n)];
        len = splice(seed, other, scratch, sizeof scratch);
    }

    /* Apply a random burst of havoc edits, then run the mutant. The number of
     * edits per cycle is randomized so we get both gentle and aggressive
     * mutations from the same seed. */
    u32 rounds = 1u << (1 + rng_below(6));           /* 2..64 edits            */
    len = mutate_havoc(scratch, len, sizeof scratch, rounds);

    int res = run_target(scratch, len);
    save_if_interesting(scratch, len, res);
}

/* =====================================================================
 * 10. Seed loading + main.
 * ===================================================================== */

/* Read a whole file into a malloc'd buffer. Caller frees. */
static u8 *read_file(const char *path, size_t *out_len)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0) return NULL;
    struct stat st;
    if (fstat(fd, &st) < 0 || st.st_size <= 0) { close(fd); return NULL; }
    size_t n = (size_t)st.st_size;
    if (n > MAX_INPUT_CAP) n = MAX_INPUT_CAP;
    u8 *buf = malloc(n);
    if (!buf) { close(fd); return NULL; }
    ssize_t r = read(fd, buf, n);
    close(fd);
    if (r <= 0) { free(buf); return NULL; }
    *out_len = (size_t)r;
    return buf;
}

/* Load every file in the seed directory into the corpus. If none exist we plant
 * a single trivial seed so the mutator has something to chew on — the fuzzer can
 * bootstrap from a single byte because coverage feedback grows the corpus. */
static void load_seeds(const char *seed_dir)
{
    DIR *d = opendir(seed_dir);
    if (d) {
        struct dirent *e;
        while ((e = readdir(d))) {
            if (e->d_name[0] == '.') continue;
            char path[600];
            snprintf(path, sizeof path, "%s/%s", seed_dir, e->d_name);
            size_t n; u8 *buf = read_file(path, &n);
            if (buf) { corpus_add(buf, n); free(buf); }
        }
        closedir(d);
    }
    if (corpus_n == 0) {
        u8 seed[1] = { '\n' };
        corpus_add(seed, 1);
        printf("[*] no seeds found; bootstrapping from a 1-byte input\n");
    }
    printf("[*] loaded %zu seed(s)\n", corpus_n);
}

/* mkdir -p one level (out_dir and its subdirs). Ignores EEXIST. */
static void ensure_dir(const char *p)
{
    if (mkdir(p, 0700) < 0 && errno != EEXIST) { perror("mkdir"); exit(1); }
}

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr,
            "coverage-guided fuzzer (teaching core)\n"
            "usage: %s <target-binary> <seed-dir> [out-dir] [dict-file]\n"
            "  <target-binary>  instrumented target linked with rt.c (e.g. ./parser)\n"
            "  <seed-dir>       directory of starting inputs (may be empty)\n"
            "  [out-dir]        where queue/, crashes/, hangs/ are written (default ./out)\n"
            "  [dict-file]      optional AFL-style dictionary\n", argv[0]);
        return 2;
    }
    char *target    = argv[1];
    char *seed_dir  = argv[2];
    const char *od  = (argc > 3) ? argv[3] : "out";
    snprintf(out_dir, sizeof out_dir, "%s", od);

    /* Seed the PRNG from the clock unless COVFUZZ_SEED is set, so campaigns are
     * reproducible on demand (set the env var to replay an exact run). */
    char *seed_env = getenv("COVFUZZ_SEED");
    rng_state = seed_env ? strtoull(seed_env, NULL, 0)
                         : (u64)time(NULL) * 6364136223846793005ULL + 1;
    if (rng_state == 0) rng_state = 1;

    /* Output tree. */
    ensure_dir(out_dir);
    char sub[520];
    snprintf(sub, sizeof sub, "%s/queue",   out_dir); ensure_dir(sub);
    snprintf(sub, sizeof sub, "%s/crashes", out_dir); ensure_dir(sub);
    snprintf(sub, sizeof sub, "%s/hangs",   out_dir); ensure_dir(sub);
    snprintf(cur_input_path, sizeof cur_input_path, "%s/.cur_input", out_dir);

    if (argc > 4) dict_load(argv[4]);

    init_count_class_lookup();
    setup_shm();
    atexit(remove_shm);                  /* never leak the IPC segment         */
    signal(SIGINT,  on_signal);          /* Ctrl-C: clean up, then exit        */
    signal(SIGTERM, on_signal);
    load_seeds(seed_dir);
    init_forkserver(target);

    /* Prime coverage from the seeds themselves so their edges count as "seen"
     * and only genuinely new mutations get queued. */
    for (size_t i = 0; i < corpus_n; i++) {
        run_target(corpus[i].data, corpus[i].len);
        classify_counts(trace_bits);
        has_new_bits(trace_bits);
    }

    printf("[*] fuzzing... (Ctrl-C to stop)\n");
    time_t last = 0;
    int announced = 0;
    for (;;) {
        fuzz_one();

        /* The first crash proves the point of the demo; announce it once with a
         * reproduce command, then keep fuzzing to collect more. */
        if (crashes > 0 && !announced) {
            announced = 1;
            printf("\n[+] crash found after %llu execs — saved in %s/crashes/\n",
                   (unsigned long long)total_execs, out_dir);
            printf("[+] reproduce with:  %s %s/crashes/<id>\n", target, out_dir);
        }

        /* Print a stats line about once a second — cheap, human-friendly. */
        time_t now = time(NULL);
        if (now != last) {
            last = now;
            printf("\r[ execs=%llu  corpus=%u  crashes=%u  hangs=%u ]   ",
                   (unsigned long long)total_execs, queued, crashes, hangs);
            fflush(stdout);
        }
    }
    return 0;   /* unreachable; loop exits via signal/Ctrl-C */
}
