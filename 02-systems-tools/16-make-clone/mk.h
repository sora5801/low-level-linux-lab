/* ===========================================================================
 * mk.h — shared types and prototypes for `mmake`, a teaching make(1) clone.
 * ===========================================================================
 *
 * This is the one header every translation unit includes. It defines the data
 * model (variables, rules, the dependency-graph node) and declares the API each
 * .c file exports. Read it top-to-bottom before the .c files: it is the map.
 *
 * THE PIPELINE, end to end:
 *
 *     read_file ─► parse.c ─► rules + variables
 *                     │
 *                     ▼
 *                  graph.c  ─► resolve names to nodes, build the DAG,
 *                     │         stat(2) files, decide staleness (mtime),
 *                     │         detect cycles (DFS coloring)
 *                     ▼
 *                  job.c    ─► topological/parallel schedule; fork(2)+execve(2)
 *                     │         each recipe under /bin/sh -c; bound concurrency
 *                     │         with the POSIX jobserver pipe (jobserver.c)
 *                     ▼
 *                  exit status
 *
 * PLATFORM: Linux / WSL. The build engine uses fork, execve, pipe2, stat,
 * waitpid, sigaction and kill — all POSIX; some flags (pipe2, O_CLOEXEC) are
 * Linux-friendly. It does not build on native Windows. The committed teaching
 * assembly in asm/ is host-portable (clang cross-targets Linux).
 * ===========================================================================
 */
#ifndef MK_H
#define MK_H

/* _GNU_SOURCE exposes pipe2(2), the SA_RESTART/SA_NOCLDSTOP sigaction flags,
 * and struct stat's nanosecond st_mtim member from glibc's headers. We also set
 * it via -D_GNU_SOURCE in the Makefile; the guard keeps both paths happy. */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stddef.h>   /* size_t                                              */
#include <sys/types.h>/* pid_t                                               */

/* ---------------------------------------------------------------------------
 * mk_time — a whole file modification time squashed into ONE comparable 64-bit
 * integer of nanoseconds since the epoch.
 *
 * stat(2) reports mtime as a struct timespec { time_t tv_sec; long tv_nsec; }.
 * Comparing two timespecs means "compare tv_sec, then tv_nsec on a tie" — easy
 * to get subtly wrong. Folding both fields into a single `long long` of ns lets
 * the staleness test be one plain `>` comparison. A signed 64-bit ns counter
 * does not overflow until the year ~2262, so this is safe for any real file.
 *
 * MK_TIME_MISSING (-1) is a sentinel meaning "the file does not exist"; because
 * every real mtime is >= 0, a missing target always compares older than any
 * prerequisite, which is exactly the "must build it" semantics we want.
 * --------------------------------------------------------------------------- */
typedef long long mk_time;
#define MK_TIME_MISSING ((mk_time)-1)

/* ===========================================================================
 * VARIABLES
 * ===========================================================================
 * make has two variable "flavors":
 *   RECURSIVE (`VAR = text`)  — the right-hand side is stored VERBATIM and
 *       re-expanded every time the variable is referenced. `A = $(B)` tracks
 *       later changes to B. This is the deferred/lazy flavor.
 *   SIMPLE    (`VAR := text`) — the RHS is expanded ONCE, at definition time,
 *       and the result stored. Snapshots B's current value.
 * `?=` assigns only if the variable is not already set; `+=` appends.
 */
typedef enum { VAR_RECURSIVE, VAR_SIMPLE } var_flavor;

typedef struct {
    char       *name;    /* owns: variable name, e.g. "CFLAGS"               */
    char       *value;   /* owns: RHS text (raw for RECURSIVE, expanded for  */
                         /*       SIMPLE). Expanded again at use if RECURSIVE.*/
    var_flavor  flavor;  /* how to treat `value` when the var is referenced   */
} variable;

/* ===========================================================================
 * RULES  (as parsed, before graph resolution)
 * ===========================================================================
 * One `target ... : prereq ...` block plus its tab-indented recipe lines. A
 * single logical target may be described by several rule blocks; graph.c merges
 * them into one node.
 */
typedef struct {
    char  **targets;   /* owns each string: names left of the ':'            */
    size_t  ntarget;
    char  **prereqs;   /* owns each string: names right of the ':'           */
    size_t  nprereq;
    char  **recipe;    /* owns each string: recipe command lines (raw, so the */
    size_t  nrecipe;   /*   $(VAR)/$@/$< in them expand at execution time)    */
} rule;

/* ===========================================================================
 * GRAPH NODES  (the DAG the scheduler walks)
 * ===========================================================================
 * After parsing, every distinct target/prerequisite name becomes exactly one
 * node. Edges point target -> prerequisite ("I depend on you"); we also keep
 * the reverse edges ("who depends on me") so the parallel scheduler can, when a
 * node finishes, cheaply wake the nodes that were waiting on it.
 */

/* DFS coloring for cycle detection (the classic three-color scheme):
 *   WHITE = not yet visited, GRAY = on the current recursion stack,
 *   BLACK = fully processed. An edge to a GRAY node is a back edge == a cycle. */
typedef enum { NODE_WHITE, NODE_GRAY, NODE_BLACK } node_color;

/* Build lifecycle of a node, driven by the scheduler in job.c. */
typedef enum {
    BUILD_WAITING,   /* still has unbuilt prerequisites                       */
    BUILD_READY,     /* all prerequisites done; eligible to run              */
    BUILD_RUNNING,   /* a recipe process has been forked for it              */
    BUILD_DONE,      /* finished successfully (or was already up to date)    */
    BUILD_FAILED     /* its recipe exited non-zero                           */
} build_state;

typedef struct node {
    char         *name;         /* owns: the target/file name                */

    struct node **deps;         /* prerequisites, resolved to node pointers  */
    size_t        ndep;         /*   (array borrows the nodes, owns the array)*/
    struct node **dependents;   /* reverse edges: nodes that list me as a dep */
    size_t        ndependent;

    char        **recipe;       /* borrowed from the owning rule (not freed   */
    size_t        nrecipe;      /*   here); the command lines to run          */

    int           is_phony;     /* declared in .PHONY, or has a recipe but is */
                                /*   not a real file: always considered stale */

    /* Filled by graph.c's stat pass: */
    mk_time       mtime;        /* file mtime in ns, or MK_TIME_MISSING       */
    int           needs_rebuild;/* staleness verdict (1 = must run recipe)    */

    /* Scratch used by the algorithms: */
    node_color    color;        /* cycle-detection DFS state                  */
    int           staleness_done;/* memo flag: staleness already computed      */
    int           reachable;    /* 1 if in the sub-DAG rooted at the goal;    */
                                /*   the scheduler ignores unreachable nodes  */
    build_state   state;        /* scheduler lifecycle                        */
    size_t        unbuilt_deps; /* prerequisites not yet BUILD_DONE; when this */
                                /*   hits 0 the node becomes BUILD_READY. This */
                                /*   is Kahn's in-degree, decremented live.    */
} node;

/* ===========================================================================
 * mk — the whole loaded makefile: variables, rules, and the resolved graph.
 * ===========================================================================
 */
typedef struct {
    variable *vars;    size_t nvar,   varcap;
    rule     *rules;   size_t nrule,  rulecap;
    node    **nodes;   size_t nnode,  nodecap;   /* owns each node + the array */

    /* Command-line-ish options that affect the engine (set by main.c): */
    int   jobs;        /* -jN: max concurrent recipe processes (>=1)          */
    int   dry_run;     /* -n : print recipes, do not execute                  */
    int   silent;      /* -s : never echo recipe lines                        */
    int   keep_going;  /* -k : on failure, build what still can be built      */
    int   always_make; /* -B : treat every target as stale                    */
    int   question;    /* -q : question mode; exit 1 iff something is stale    */
} mk;

/* ---------------------------------------------------------------------------
 * autoctx — the context needed to expand a recipe's AUTOMATIC variables.
 * Each recipe line is expanded against the node it belongs to:
 *     $@  -> target       (this node's name)
 *     $<  -> first        (first prerequisite)
 *     $^  -> all          (all prerequisites, space-separated)
 * For ordinary (non-recipe) expansion the caller passes NULL and $@/$</$^
 * expand to the empty string.
 * --------------------------------------------------------------------------- */
typedef struct {
    const char *target;
    const char *first;
    const char *all;
} autoctx;

/* ===========================================================================
 * util.c — checked allocation, a growable string buffer, and file slurping.
 * ===========================================================================
 * Every allocator here aborts on OOM (via die), so callers never null-check.
 * That is a deliberate teaching choice: it keeps the algorithmic code free of
 * error-path clutter while still never ignoring a failure. */
void  *xmalloc(size_t n);                 /* malloc or die                     */
void  *xcalloc(size_t n, size_t sz);      /* calloc or die                     */
void  *xrealloc(void *p, size_t n);       /* realloc or die                    */
char  *xstrdup(const char *s);            /* strdup or die                     */
char  *xstrndup(const char *s, size_t n); /* bounded strdup or die             */
void   die(const char *fmt, ...);         /* perror-ish; prints, then exit(2)  */

/* strbuf — an amortized-O(1)-append byte buffer used to build expanded strings
 * and command lines. `data` is always NUL-terminated so it can be used as a C
 * string; `len` excludes the terminator. */
typedef struct { char *data; size_t len, cap; } strbuf;
void   sb_init(strbuf *sb);
void   sb_addch(strbuf *sb, char c);
void   sb_addstr(strbuf *sb, const char *s);
void   sb_addbytes(strbuf *sb, const char *p, size_t n);
char  *sb_detach(strbuf *sb);             /* hand ownership of data to caller  */
void   sb_free(strbuf *sb);

/* read_file — slurp an entire file into a fresh, NUL-terminated buffer.
 * Returns the malloc'd bytes and writes the length (excluding the added NUL)
 * through *out_len. Dies on I/O error. Handles short reads and EINTR. */
char  *read_file(const char *path, size_t *out_len);

/* ===========================================================================
 * parse.c — turn makefile text into `vars` and `rules`.
 * ===========================================================================
 */
void   mk_parse(mk *m, const char *text);              /* populate m->rules/vars */
char  *mk_expand(mk *m, const char *text, const autoctx *ctx); /* $() expansion  */
variable *mk_var_find(mk *m, const char *name);        /* lookup, or NULL        */
void   mk_var_set(mk *m, const char *name, const char *value, var_flavor fl);

/* ===========================================================================
 * graph.c — build and analyze the dependency DAG.
 * ===========================================================================
 */
node  *mk_node_get(mk *m, const char *name);   /* find-or-create by name        */
void   mk_graph_build(mk *m);                  /* rules -> nodes + edges        */
/* Returns 0 if the sub-DAG rooted at `goal` is acyclic; on a cycle, returns -1
 * after printing "circular <a> <- <b> dependency dropped". */
int    mk_graph_check_cycles(mk *m, node *goal);
void   mk_graph_stat(mk *m);                   /* stat(2) every file node       */
/* Post-order staleness: sets needs_rebuild on `goal` and everything it reaches.
 * A node is stale if phony, missing, newer-prereq, or a stale prerequisite. */
void   mk_compute_staleness(mk *m, node *goal);

/* ===========================================================================
 * jobserver.c — the POSIX jobserver: a pipe whose bytes are "job tokens".
 * ===========================================================================
 * The concurrency budget is `jobs` slots. The top make owns ONE implicitly (it
 * is itself allowed to run one job); the remaining jobs-1 tokens live as bytes
 * in a pipe. To start another parallel job a process must hold a token: it
 * either takes the free implicit slot or reads one byte from the pipe. When the
 * job ends it returns the token (frees the implicit slot, or writes the byte
 * back). A sub-make inherits the pipe fds via MAKEFLAGS and shares the same
 * budget — that is how `-jN` stays global across a recursive build.
 */
typedef enum { TOKEN_NONE, TOKEN_IMPLICIT, TOKEN_PIPE } token_kind;

typedef struct {
    int  rfd, wfd;        /* pipe read/write fds (may be inherited)           */
    int  jobs;            /* total token budget = -jN                         */
    int  own_pipe;        /* 1 if WE created the pipe (must fill+close it)     */
    int  implicit_free;   /* is our own always-available slot unused?         */
    int  pipe_held;       /* how many pipe tokens we currently hold           */
} jobserver;

/* Create (or attach to an inherited) jobserver for a budget of `jobs`. */
void        jobserver_init(jobserver *js, int jobs);
/* Non-blocking acquire: returns TOKEN_IMPLICIT or TOKEN_PIPE on success, or
 * TOKEN_NONE if every slot is currently in use (caller should wait for a
 * running job to finish and return its token). */
token_kind  jobserver_acquire(jobserver *js);
void        jobserver_release(jobserver *js, token_kind t);
/* Export "--jobserver-auth=R,W -jN" in MAKEFLAGS for recursive sub-makes. */
void        jobserver_export(jobserver *js);
void        jobserver_destroy(jobserver *js);

/* ===========================================================================
 * job.c — the scheduler: run stale nodes' recipes, in dependency order,
 * up to `jobs` at a time, with correct child and signal handling.
 * ===========================================================================
 * Returns 0 if everything that needed building built successfully, else 1.
 */
int    mk_build(mk *m, node *goal);

/* Install SIGINT/SIGTERM handlers so an interrupted build tears down its
 * children instead of orphaning them. Call once from main. */
void   mk_install_signal_handlers(void);

/* ===========================================================================
 * top-level teardown
 * ===========================================================================
 */
void   mk_free(mk *m);

#endif /* MK_H */
