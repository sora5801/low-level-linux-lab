/* ===========================================================================
 * job.c — the build scheduler: run stale targets' recipes in dependency order,
 * up to -jN at a time, via fork(2)/execve(2), with correct child reaping and
 * signal handling.
 * ===========================================================================
 *
 * THE SCHEDULING MODEL is Kahn's algorithm run LIVE against the DAG:
 *   - A node is READY when its `unbuilt_deps` counter (its in-degree) is 0.
 *   - We launch ready nodes as long as we can acquire a jobserver token.
 *   - When a child finishes we return its token, mark the node DONE, and
 *     decrement the in-degree of each of its dependents; any that reach 0 join
 *     the ready queue. This ordering is exactly a topological sort, produced on
 *     the fly, and it naturally exposes all currently-independent work for
 *     parallel execution.
 *
 * PROCESS STRUCTURE (two fork levels, mirroring how real make shells out):
 *
 *   make (this process)
 *     └─ fork ─► JOB CHILD  (one per target; owns its own process group)
 *                  └─ for each recipe line:
 *                       fork ─► /bin/sh -c "<expanded line>"   (a GRANDCHILD)
 *
 * The job child runs a target's recipe lines sequentially, honoring the '@'
 * (silent), '-' (ignore-error), and '+' (always-run) prefixes, and _exit()s
 * non-zero on the first failing, non-ignored line. Putting each job child in its
 * OWN process group lets an interrupt kill the whole subtree with one killpg.
 *
 * SIGNALS. We catch SIGINT/SIGTERM (no SA_RESTART, so a blocking waitpid is
 * interrupted promptly), set a flag, and on the next loop turn tear the build
 * down: killpg every running job group, reap the children, and exit non-zero.
 * Recipe children reset the handlers to default, so they die normally.
 * ===========================================================================
 */
#include "mk.h"

#include <unistd.h>     /* fork, execl, _exit, setpgid                        */
#include <signal.h>     /* sigaction, kill, killpg, SIGINT, SIGTERM           */
#include <sys/wait.h>   /* waitpid, WIFEXITED, WEXITSTATUS, WIFSIGNALED       */
#include <string.h>     /* memset, strlen                                     */
#include <errno.h>      /* errno, EINTR, ECHILD                               */
#include <stdio.h>      /* fprintf, fflush                                    */
#include <stdlib.h>     /* free                                              */

/* ---------------------------------------------------------------------------
 * SIGNAL PLUMBING
 * ---------------------------------------------------------------------------
 * `g_interrupted` is written only by the handler and read only by the loop, so
 * it must be `volatile sig_atomic_t`: `volatile` stops the compiler caching it
 * in a register across the wait, and `sig_atomic_t` is the one type the C
 * standard promises can be read/written atomically with respect to a signal.
 * We store the signal NUMBER so the exit status can be the conventional
 * 128 + signo. */
static volatile sig_atomic_t g_interrupted = 0;

static void on_signal(int signo)
{
    g_interrupted = signo;             /* the ONLY thing that is async-safe here */
}

void mk_install_signal_handlers(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = on_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;                   /* deliberately NO SA_RESTART: we want a  */
                                       /*   blocked waitpid to return EINTR so    */
                                       /*   the loop can notice the interrupt.    */
    if (sigaction(SIGINT,  &sa, NULL) < 0) die("sigaction SIGINT");
    if (sigaction(SIGTERM, &sa, NULL) < 0) die("sigaction SIGTERM");
}

/* ---------------------------------------------------------------------------
 * RECIPE LINE PREPARATION
 * ---------------------------------------------------------------------------
 * Strip the leading modifier prefixes ('@','-','+' in any order), then expand
 * the remaining command against the node's automatic variables. Returns a
 * freshly-allocated, fully-expanded command string (caller frees).
 */
static char *prep_line(mk *m, const char *raw, const autoctx *ctx,
                       int *silent, int *ignore, int *always)
{
    *silent = *ignore = *always = 0;
    const char *p = raw;
    for (;;) {
        if      (*p == '@') { *silent = 1; p++; }
        else if (*p == '-') { *ignore = 1; p++; }
        else if (*p == '+') { *always = 1; p++; }
        else break;
    }
    return mk_expand(m, p, ctx);       /* $(CC) $< ... -> concrete command       */
}

/* Build the "$^" string: every prerequisite name, space-separated. Caller frees.*/
static char *join_prereqs(node *n)
{
    strbuf sb;
    sb_init(&sb);
    for (size_t i = 0; i < n->ndep; i++) {
        if (i) sb_addch(&sb, ' ');
        sb_addstr(&sb, n->deps[i]->name);
    }
    return sb_detach(&sb);
}

/* Fill an autoctx for a node. `all` must be freed by the caller after use. */
static autoctx make_ctx(node *n, char **all_out)
{
    char *all = join_prereqs(n);
    autoctx ctx;
    ctx.target = n->name;
    ctx.first  = (n->ndep > 0) ? n->deps[0]->name : "";
    ctx.all    = all;
    *all_out   = all;
    return ctx;
}

/* ---------------------------------------------------------------------------
 * run_one_command — fork a shell for a single recipe line and wait for it.
 * ---------------------------------------------------------------------------
 * Syscalls:
 *   fork(2)  : number 57. Duplicates the calling process. Returns 0 in the
 *              child, the child's pid in the parent, or -1 on failure.
 *   execve(2): number 59 (via execl). Replaces the child's image with /bin/sh;
 *              on success it NEVER returns, so any code after it ran only
 *              because exec FAILED — hence the _exit(127) ("command not found").
 *   waitpid(2): number 61. Reap the specific child; EINTR is retried.
 * Returns the shell's exit code (or 128+signal if it was killed).
 */
static int run_one_command(const char *cmd)
{
    pid_t pid = fork();
    if (pid < 0)
        die("fork");

    if (pid == 0) {
        /* CHILD (grandchild of make). It inherits the job child's process group,
         * so a group-kill reaches it. Exec the shell to interpret one line. */
        execl("/bin/sh", "sh", "-c", cmd, (char *)NULL);
        /* Only reached if execl failed (e.g. no /bin/sh). Use _exit, not exit,
         * so we do not flush the parent's stdio buffers in this doomed copy. */
        _exit(127);
    }

    int status;
    for (;;) {
        pid_t w = waitpid(pid, &status, 0);
        if (w >= 0) break;
        if (errno == EINTR) continue;  /* a signal poked us; keep waiting        */
        die("waitpid (recipe shell)");
    }
    if (WIFEXITED(status))  return WEXITSTATUS(status);      /* normal exit code */
    if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);  /* killed by signal */
    return 1;                                               /* stopped/other     */
}

/* ---------------------------------------------------------------------------
 * run_job_child — the body of a per-target job child. NEVER returns; it _exit()s
 * with 0 on success or the first failing line's status.
 * ---------------------------------------------------------------------------
 */
__attribute__((noreturn))
static void run_job_child(mk *m, node *n)
{
    /* Restore default signal handling so this subtree responds normally to
     * Ctrl-C, and start our OWN process group so make can group-kill us. */
    signal(SIGINT,  SIG_DFL);
    signal(SIGTERM, SIG_DFL);
    if (setpgid(0, 0) < 0)
        _exit(126);                    /* extremely unlikely; bail clearly        */

    char   *all;
    autoctx ctx = make_ctx(n, &all);

    for (size_t i = 0; i < n->nrecipe; i++) {
        int silent, ignore, always;
        char *cmd = prep_line(m, n->recipe[i], &ctx, &silent, &ignore, &always);
        (void)always;                  /* '+' only affects -n, handled in parent */

        if (!(m->silent || silent)) {  /* echo the command unless silenced       */
            fprintf(stdout, "%s\n", cmd);
            fflush(stdout);            /* flush before fork so output is ordered  */
        }

        int rc = run_one_command(cmd);
        free(cmd);

        if (rc != 0 && !ignore) {      /* a real failure stops this target        */
            fprintf(stderr, "mmake: *** [%s] Error %d\n", n->name, rc);
            free(all);
            _exit(rc);
        }
    }
    free(all);
    _exit(0);                          /* every line succeeded (or was ignored)   */
}

/* Parent-side dry-run (-n): print what WOULD run, without forking anything. */
static void print_recipe(mk *m, node *n)
{
    char   *all;
    autoctx ctx = make_ctx(n, &all);
    for (size_t i = 0; i < n->nrecipe; i++) {
        int silent, ignore, always;
        char *cmd = prep_line(m, n->recipe[i], &ctx, &silent, &ignore, &always);
        (void)silent; (void)ignore; (void)always;
        fprintf(stdout, "%s\n", cmd);  /* -n shows even @-silenced lines          */
        free(cmd);
    }
    free(all);
}

/* ===========================================================================
 * THE SCHEDULER
 * ===========================================================================
 */

/* One slot in the table of currently-running job children. */
typedef struct {
    pid_t      pid;   /* job child's pid (also its process-group id)           */
    node      *n;     /* which target it is building                          */
    token_kind tok;   /* the jobserver token to return when it finishes        */
} running_job;

/* A simple growable FIFO of ready nodes. */
typedef struct { node **v; size_t head, len, cap; } readyq;

static void rq_push(readyq *q, node *n)
{
    if (q->len == q->cap) {
        q->cap = q->cap ? q->cap * 2 : 16;
        q->v   = xrealloc(q->v, q->cap * sizeof *q->v);
    }
    q->v[q->len++] = n;
}

/* DFS from the goal, marking the reachable sub-DAG and initializing each node's
 * scheduler state. Only reachable nodes are ever built. */
static void collect_reachable(node *n, readyq *q)
{
    if (n->reachable) return;          /* visit each node once                   */
    n->reachable    = 1;
    n->unbuilt_deps = n->ndep;         /* fresh in-degree for this build          */
    for (size_t i = 0; i < n->ndep; i++)
        collect_reachable(n->deps[i], q);

    n->state = (n->ndep == 0) ? BUILD_READY : BUILD_WAITING;
    if (n->state == BUILD_READY)
        rq_push(q, n);                 /* leaves (in-degree 0) start ready        */
}

/* A node finished successfully (or was already up to date / recipe-less): mark
 * it DONE and wake any dependent whose last prerequisite this was. */
static void complete_success(node *n, readyq *q)
{
    n->state = BUILD_DONE;
    for (size_t i = 0; i < n->ndependent; i++) {
        node *dep = n->dependents[i];
        if (!dep->reachable) continue;         /* outside the goal subtree: skip  */
        if (dep->state != BUILD_WAITING) continue;
        if (dep->unbuilt_deps > 0 && --dep->unbuilt_deps == 0) {
            dep->state = BUILD_READY;
            rq_push(q, dep);                   /* its last prereq just landed      */
        }
    }
}

/* Kill and reap every still-running job group (used on failure/interrupt). */
static void teardown(running_job *run, size_t nrun, int signo)
{
    for (size_t i = 0; i < nrun; i++)
        if (run[i].pid > 0)
            killpg(run[i].pid, signo); /* signal the whole job process group      */
    for (size_t i = 0; i < nrun; i++) {
        if (run[i].pid <= 0) continue;
        int status;
        while (waitpid(run[i].pid, &status, 0) < 0 && errno == EINTR)
            ;                          /* reap; ignore EINTR                      */
    }
}

/* ---------------------------------------------------------------------------
 * mk_build — build `goal`. Returns 0 on success, non-zero on any failure.
 * ---------------------------------------------------------------------------
 */
int mk_build(mk *m, node *goal)
{
    /* Set up the jobserver token pool for -jN and advertise it to sub-makes. */
    jobserver js;
    jobserver_init(&js, m->jobs);
    jobserver_export(&js);

    readyq q;
    memset(&q, 0, sizeof q);
    collect_reachable(goal, &q);

    /* Running-job table: concurrency is capped by the token budget, so `jobs`
     * slots always suffice. */
    size_t       cap  = (m->jobs > 0) ? (size_t)m->jobs : 1;
    running_job *run  = xcalloc(cap, sizeof *run);
    size_t       nrun = 0;

    int failures = 0;                  /* count of targets whose recipe failed    */
    int aborting = 0;                  /* set once we decide to stop launching    */
    int ran      = 0;                  /* recipes actually started (for the       */
                                       /*   "Nothing to be done" message)         */

    for (;;) {
        /* ---- Handle an interrupt as early as possible each turn ---- */
        if (g_interrupted) {
            fprintf(stderr, "\nmmake: *** interrupted; killing %zu job(s)\n", nrun);
            teardown(run, nrun, SIGTERM);
            nrun = 0;
            free(run);
            free(q.v);
            jobserver_destroy(&js);
            return 128 + (int)g_interrupted;
        }

        /* ---- LAUNCH: start ready nodes while tokens and work remain ---- */
        while (!aborting && q.head < q.len) {
            node *r = q.v[q.head];

            /* Nothing to run for this node? (up to date, or has no recipe.)
             * Complete it instantly — it consumes no token and no process. */
            if (!r->needs_rebuild || r->nrecipe == 0) {
                q.head++;
                complete_success(r, &q);
                continue;
            }

            /* Try to get a slot. TOKEN_NONE => every slot busy: stop launching
             * and go reap a running child (which returns its token). */
            token_kind tok = jobserver_acquire(&js);
            if (tok == TOKEN_NONE)
                break;

            q.head++;
            ran++;                     /* a real (or dry-run) recipe is happening  */

            if (m->dry_run) {          /* -n: show the recipe, run nothing        */
                print_recipe(m, r);
                jobserver_release(&js, tok);
                complete_success(r, &q);
                continue;
            }

            fflush(stdout);            /* order parent output before child output */
            pid_t pid = fork();
            if (pid < 0)
                die("fork");
            if (pid == 0)
                run_job_child(m, r);   /* never returns                          */

            /* PARENT: also set the child's pgid here to avoid a race — whichever
             * of parent/child wins, the group id ends up == pid. */
            setpgid(pid, pid);
            r->state = BUILD_RUNNING;
            run[nrun].pid = pid;
            run[nrun].n   = r;
            run[nrun].tok = tok;
            nrun++;
        }

        /* ---- Termination: nothing running and nothing left to launch ---- */
        if (nrun == 0) {
            if (aborting || q.head >= q.len)
                break;                 /* done (or nothing more can be built)     */
        }

        /* ---- WAIT: reap exactly one finished job, free its token ---- */
        int   status;
        pid_t w;
        for (;;) {
            w = waitpid(-1, &status, 0);
            if (w >= 0) break;
            if (errno == EINTR) {
                if (g_interrupted) break;   /* jump back to the interrupt handler */
                continue;
            }
            if (errno == ECHILD) { w = -1; break; }  /* no children (shouldn't)   */
            die("waitpid");
        }
        if (w < 0)
            continue;                  /* interrupted or ECHILD: re-loop           */

        /* Find the table slot for the pid that just exited. */
        size_t idx;
        for (idx = 0; idx < nrun; idx++)
            if (run[idx].pid == w) break;
        if (idx == nrun)
            continue;                  /* an unknown pid (e.g. a stray) — ignore   */

        node      *done = run[idx].n;
        token_kind tok  = run[idx].tok;
        run[idx] = run[--nrun];        /* compact the table (swap-with-last)       */

        jobserver_release(&js, tok);   /* give the slot back to the pool           */

        int ok = WIFEXITED(status) && WEXITSTATUS(status) == 0;
        if (ok) {
            complete_success(done, &q); /* DONE: may unblock dependents            */
        } else {
            done->state = BUILD_FAILED;
            failures++;
            fprintf(stderr, "mmake: target '%s' failed\n", done->name);
            if (!m->keep_going) {
                /* Stop launching new work; drain what is already running, then
                 * exit. Dependents of the failed node are simply never awakened.*/
                aborting = 1;
            }
            /* With -k we keep going: other independent targets still build; the
             * failed node's dependents stay WAITING forever (correctly skipped).*/
        }
    }

    /* A phony goal whose subtree was entirely up to date runs no recipes; say so
     * the way make does, instead of exiting silently. */
    if (ran == 0 && failures == 0)
        printf("mmake: Nothing to be done for '%s'.\n", goal->name);

    free(run);
    free(q.v);
    jobserver_destroy(&js);
    return failures ? 1 : 0;
}
