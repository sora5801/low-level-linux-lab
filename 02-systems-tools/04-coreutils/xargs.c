/* ===========================================================================
 * xargs.c — read items from stdin, batch them under ARG_MAX, fork/exec.
 * ===========================================================================
 *
 * `xargs` exists because of one kernel limit: execve(2) rejects an argument +
 * environment block larger than ARG_MAX with E2BIG. `find | xargs rm` works
 * where `rm $(find ...)` would explode. So the ENTIRE job of xargs is:
 *
 *   1. Read whitespace-separated items from stdin (or NUL-separated with -0).
 *   2. Pack as many as possible into one command line WITHOUT exceeding the
 *      exec limit — and this is the part everyone gets wrong. The limit is on
 *      argv + envp TOGETHER, counting: every string's bytes + its NUL, PLUS a
 *      pointer per string in the argv/envp arrays. You must subtract the size
 *      of the inherited environment and leave headroom, or you get intermittent
 *      E2BIG only on machines with big environments.
 *   3. fork(2) + execvp(3) the command with that batch; wait; repeat.
 *
 * Flags: -n MAX (at most MAX items per command), -0 (NUL-delimited input, pairs
 * with `find -print0`). With no command, the default is `echo`.
 * =========================================================================== */
#include "coreutils.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>   /* malloc/realloc/free, exit, _exit                     */
#include <string.h>
#include <sys/wait.h> /* fork, waitpid, WIFEXITED, WEXITSTATUS                */
#include <unistd.h>   /* read, fork, execvp, sysconf, environ                 */

extern char **environ;   /* the inherited environment, needed for budgeting     */

/* Reserve this many bytes below the true ARG_MAX. The kernel also stores the
 * argv/envp pointer arrays and some accounting inside the same limit, and glibc
 * execvp copies the path; GNU xargs reserves 2048 by default. We match it. */
#define ARG_HEADROOM 2048

/* A growable byte buffer we read stdin into, plus a growable pointer list into
 * it for the parsed items. Splitting parse-from-storage keeps the pointers
 * stable only if the storage never reallocates mid-parse, so we read the WHOLE
 * input first, then parse — simple and correct for xargs's typical inputs. */
struct inbuf { char *data; size_t len, cap; };

/* Slurp all of fd into buf (robust loop). Returns 0 or -1. */
static int slurp(int fd, struct inbuf *b)
{
    for (;;) {
        if (b->len + 65536 > b->cap) {
            size_t ncap = b->cap ? b->cap * 2 : 131072;
            while (ncap < b->len + 65536) ncap *= 2;
            char *t = realloc(b->data, ncap);
            if (!t) { errno = ENOMEM; return -1; }
            b->data = t; b->cap = ncap;
        }
        ssize_t r = read_retry(fd, b->data + b->len, 65536);
        if (r < 0) return -1;
        if (r == 0) return 0;
        b->len += (size_t)r;
    }
}

/* Compute the byte budget for one exec: ARG_MAX minus the current environment's
 * footprint minus headroom. Counting the environment is the detail that makes
 * xargs robust across machines with fat environments. */
static size_t exec_budget(void)
{
    long am = sysconf(_SC_ARG_MAX);      /* the kernel's execve size ceiling    */
    if (am <= 0) am = 128 * 1024;        /* POSIX floor is 4096; be generous    */

    size_t env = 0;
    for (char **e = environ; *e; e++)
        env += strlen(*e) + 1            /* the string + its NUL                */
             + sizeof(char *);           /* its slot in the envp pointer array  */
    env += sizeof(char *);               /* the NULL terminator slot            */

    size_t budget = (size_t)am;
    if (budget > env + ARG_HEADROOM)
        budget -= env + ARG_HEADROOM;
    else
        budget = 4096;                   /* pathological: keep making progress  */
    return budget;
}

/* Per-item cost against the budget: the bytes of the string + NUL + one argv
 * pointer slot. */
static size_t item_cost(const char *s) { return strlen(s) + 1 + sizeof(char *); }

/* fork + execvp one argv (NULL-terminated). Returns the child's exit status
 * (0..255), or 127 if the command was not found / could not be exec'd. */
static int run(char **argv)
{
    pid_t pid = fork();                  /* fork(2): clone; child gets pid 0    */
    if (pid < 0) { warn("fork"); return 1; }

    if (pid == 0) {
        /* CHILD. execvp searches $PATH for argv[0] and replaces this image. It
         * only returns on failure, in which case we MUST _exit (not return, not
         * exit): returning would run the parent's cleanup twice, and exit(3)
         * would flush the shared stdio buffers we inherited — double output. */
        execvp(argv[0], argv);
        /* Convention: 127 = not found, 126 = found but not executable. */
        int code = (errno == ENOENT) ? 127 : 126;
        warn("%s", argv[0]);
        _exit(code);
    }

    /* PARENT. Reap the child so it doesn't become a zombie; retry EINTR. */
    int wstatus;
    while (waitpid(pid, &wstatus, 0) < 0) {
        if (errno == EINTR) continue;
        warn("waitpid");
        return 1;
    }
    if (WIFEXITED(wstatus))   return WEXITSTATUS(wstatus);
    if (WIFSIGNALED(wstatus)) return 128 + WTERMSIG(wstatus);
    return 1;
}

int xargs_main(int argc, char **argv)
{
    int   nmax = 0;                      /* -n: max items per command, 0=unlimited*/
    int   nul  = 0;                      /* -0: NUL-delimited input             */
    int   argi = 1;

    for (; argi < argc; argi++) {
        const char *a = argv[argi];
        if (a[0] != '-' || a[1] == '\0') break;
        if (strcmp(a, "--") == 0) { argi++; break; }
        if (strcmp(a, "-0") == 0) { nul = 1; continue; }
        if (strcmp(a, "-n") == 0) {
            if (++argi >= argc) diex("option requires an argument -- 'n'");
            nmax = atoi(argv[argi]);
            if (nmax < 1) diex("-n: argument must be >= 1");
            continue;
        }
        /* Unbundled -nN form (e.g. -n3). */
        if (a[1] == 'n') { nmax = atoi(a + 2); if (nmax < 1) diex("bad -n"); continue; }
        diex("unknown option '%s'", a);
    }

    /* The fixed command prefix = the remaining CLI args, default "echo". These
     * lead every batch. We measure their cost so per-batch budgeting includes
     * them. */
    char *default_cmd[] = { (char *)"echo", NULL };
    char **cmd    = (argi < argc) ? &argv[argi] : default_cmd;
    int    ncmd   = (argi < argc) ? (argc - argi) : 1;

    size_t base_cost = sizeof(char *);   /* the argv NULL terminator slot       */
    for (int i = 0; i < ncmd; i++)
        base_cost += item_cost(cmd[i]);

    /* Read all of stdin, then parse into items. */
    struct inbuf in = {0, 0, 0};
    if (slurp(STDIN_FILENO, &in) < 0) { warn("stdin"); free(in.data); return 1; }

    size_t budget = exec_budget();
    /* If even the command prefix doesn't fit, nothing we do will help. */
    if (base_cost >= budget)
        { diex("command line prefix already exceeds ARG_MAX budget"); }

    /* Build the argv for one batch: [cmd..., item, item, ..., NULL]. We grow a
     * char* array; the strings point INTO in.data (no copies), valid until we
     * free in.data at the very end. */
    size_t argcap = ncmd + 16;
    char **batch = malloc(argcap * sizeof *batch);
    if (!batch) { free(in.data); diex("out of memory"); }

    int    status   = 0;
    size_t bi;                            /* count of slots used in `batch`       */
    size_t cost;                          /* running byte cost of current batch   */
    int    items;                         /* items in current batch (for -n)      */

    /* (re)initialise an empty batch with just the command prefix. */
    #define RESET_BATCH() do {                                   \
        for (int _i = 0; _i < ncmd; _i++) batch[_i] = cmd[_i];   \
        bi = (size_t)ncmd; cost = base_cost; items = 0;          \
    } while (0)

    RESET_BATCH();

    /* Walk the input, carving out items. An item ends at a delimiter: NUL when
     * -0, otherwise any run of ASCII whitespace. We NUL-terminate each item in
     * place (overwriting the delimiter) so it becomes a C string in situ. */
    size_t i = 0;
    while (i < in.len) {
        /* Skip leading delimiters. */
        if (nul) {
            if (in.data[i] == '\0') { i++; continue; }
        } else {
            unsigned char c = (unsigned char)in.data[i];
            if (c==' '||c=='\t'||c=='\n'||c=='\r'||c=='\v'||c=='\f') { i++; continue; }
        }

        /* Mark the item start, then scan to the next delimiter. */
        size_t start = i;
        while (i < in.len) {
            unsigned char c = (unsigned char)in.data[i];
            int delim = nul ? (c == '\0')
                            : (c==' '||c=='\t'||c=='\n'||c=='\r'||c=='\v'||c=='\f');
            if (delim) break;
            i++;
        }
        /* Terminate the item in place. If we're at end-of-buffer with no
         * trailing delimiter, there is guaranteed room only if len < cap; we
         * grew cap generously in slurp, but be safe and append if needed. */
        if (i < in.len) {
            in.data[i] = '\0';           /* overwrite the delimiter             */
            i++;
        } else {
            /* No trailing delimiter: ensure a NUL terminator exists. */
            if (in.len == in.cap) {
                char *t = realloc(in.data, in.cap + 1);
                if (!t) { free(batch); free(in.data); diex("out of memory"); }
                in.data = t; in.cap += 1;
            }
            in.data[in.len] = '\0';
        }
        char *item = &in.data[start];

        /* Would adding this item overflow the byte budget OR the -n cap? If the
         * batch already has at least one item, flush it first, then start fresh
         * and add the item to the new batch. */
        size_t ic = item_cost(item);
        int over_bytes = (cost + ic > budget);
        int over_count = (nmax > 0 && items >= nmax);
        if ((over_bytes || over_count) && items > 0) {
            batch[bi] = NULL;
            int rc = run(batch);
            if (rc != 0) status = 1;
            RESET_BATCH();
        }

        /* A single item bigger than the whole budget can never be placed. */
        if (base_cost + ic > budget) {
            warnx("argument too long, skipping: %.40s...", item);
            status = 1;
            continue;
        }

        /* Grow the batch array if needed, then append. */
        if (bi + 2 > argcap) {           /* +1 for item, +1 for the NULL slot   */
            size_t ncap = argcap * 2;
            char **t = realloc(batch, ncap * sizeof *batch);
            if (!t) { free(batch); free(in.data); diex("out of memory"); }
            batch = t; argcap = ncap;
        }
        batch[bi++] = item;
        cost += ic;
        items++;
    }

    /* Flush the final partial batch. GNU xargs runs the command once even with
     * NO input UNLESS -r/--no-run-if-empty is given; we take the simpler,
     * safer default: only run if we actually collected items. */
    if (items > 0) {
        batch[bi] = NULL;
        int rc = run(batch);
        if (rc != 0) status = 1;
    }

    free(batch);
    free(in.data);
    return status;
    #undef RESET_BATCH
}
