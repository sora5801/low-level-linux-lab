/* ===========================================================================
 * shell.h — the data model and module contracts for the teaching shell.
 * ===========================================================================
 *
 * The shell is split into small single-responsibility modules; this header is
 * the seam between them. Read it first — the struct definitions below ARE the
 * mental model:
 *
 *   a LINE the user types  ->  one PIPELINE  ->  a `job`
 *   a job is a list of     ->  `process`es joined by '|'
 *   each process has       ->  an argv[] and a list of `redir`ections
 *
 * Job control (the hard, interesting part) hangs off `job`: a process-group id
 * (pgid), saved terminal modes, and per-process completed/stopped flags that the
 * SIGCHLD-free waitpid loop in jobs.c updates. The layout deliberately mirrors
 * the GNU libc manual's "Implementing a Job Control Shell", because that is the
 * canonical reference and lining up with it makes the code checkable.
 * ===========================================================================
 */
#ifndef SHELL_H
#define SHELL_H

#include <stddef.h>      /* size_t                                            */
#include <sys/types.h>   /* pid_t                                             */
#include <termios.h>     /* struct termios (saved per-job terminal state)     */

/* Maximum length of a single lexer WORD after quote/'$' expansion. Fixed so the
 * lexer needs no allocation; a longer word is a hard error, not a buffer
 * overflow. 64 KiB is far beyond any real command word. */
#define TOK_MAX 65536

/* ------------------------------------------------------------------ tokens -- */
/* The lexer (lexer.c) turns raw input into a stream of these. Only T_WORD
 * carries text; the operator tokens are pure punctuation. */
typedef enum {
    T_EOF = 0,   /* end of the input line                                     */
    T_WORD,      /* a command word or filename (text[] is filled)             */
    T_PIPE,      /* |                                                         */
    T_LT,        /* <   redirect stdin from a file                            */
    T_GT,        /* >   redirect stdout to a file (truncate)                  */
    T_GTGT,      /* >>  redirect stdout to a file (append)                    */
    T_2GT,       /* 2>  redirect stderr to a file (truncate)                  */
    T_AMP        /* &   run the pipeline in the background                    */
} ttype;

typedef struct {
    ttype type;
    char  text[TOK_MAX]; /* WORD payload, NUL-terminated (post-expansion)     */
    int   can_glob;      /* 1 iff the word had an UNQUOTED * ? or [ and no    */
                         /*   quoting anywhere — see lexer.c for the rule.    */
} token;

/* -------------------------------------------------------------- redirection -- */
typedef enum {
    REDIR_IN,      /* <   : open O_RDONLY, dup onto fd 0                       */
    REDIR_OUT,     /* >   : open O_WRONLY|O_CREAT|O_TRUNC,  dup onto fd 1      */
    REDIR_APPEND,  /* >>  : open O_WRONLY|O_CREAT|O_APPEND, dup onto fd 1      */
    REDIR_ERR      /* 2>  : open O_WRONLY|O_CREAT|O_TRUNC,  dup onto fd 2      */
} redir_type;

typedef struct redir {
    redir_type    type;
    char         *file;   /* target filename, owned (freed by free_job)        */
    struct redir *next;   /* redirections form a small singly-linked list      */
} redir;

/* ------------------------------------------------------------------ process -- */
/* One simple command within a pipeline. `pid`, `completed`, `stopped`, `status`
 * are runtime state filled in after fork()/waitpid(); the parser leaves them 0. */
typedef struct process {
    struct process *next;      /* next stage in the pipeline (or NULL)         */
    char          **argv;      /* NULL-terminated argument vector, owned       */
    redir          *redirs;    /* this stage's redirections, owned             */
    pid_t           pid;       /* child pid (0 until forked)                    */
    int             completed; /* set when the child has exited                */
    int             stopped;   /* set when the child is stopped (SIGTSTP etc.) */
    int             status;    /* the raw wait(2) status word                  */
} process;

/* ---------------------------------------------------------------------- job -- */
/* A whole pipeline plus its job-control state. Jobs live in a singly-linked
 * list (job_list in main.c) so `jobs`/`fg`/`bg` can find them by id. */
typedef struct job {
    struct job    *next;
    int            id;             /* small integer shown as [id] by `jobs`    */
    char          *command;        /* original text, for messages, owned       */
    process       *first_process;  /* head of the pipeline's process list      */
    pid_t          pgid;           /* process-group id (== first child's pid)  */
    int            notified;       /* have we told the user this job stopped?   */
    int            background;      /* launched with a trailing '&' ?           */
    struct termios tmodes;         /* terminal modes saved when the job stops  */
    int            stdin_fd;       /* the job's standard fds (usually 0/1/2);  */
    int            stdout_fd;      /*   named *_fd (not stdin/…) to avoid the   */
    int            stderr_fd;      /*   <stdio.h> macro clash.                  */
} job;

/* ---------------------------------------------------- small growable vector -- */
/* Used to accumulate argv words (which glob expansion may multiply). */
typedef struct {
    char **items;   /* array of owned char* (argv-style)                        */
    int    len;     /* number of items in use                                   */
    int    cap;     /* allocated capacity                                       */
} strvec;

/* ============================ global shell state (defined in main.c) ======== */
extern pid_t          shell_pgid;          /* the shell's own process group     */
extern struct termios shell_tmodes;        /* the shell's default terminal modes*/
extern int            shell_terminal;      /* fd of the controlling terminal    */
extern int            shell_is_interactive;/* isatty(shell_terminal)            */
extern pid_t          shell_pid;           /* getpid(); used for $$             */
extern int            last_status;         /* exit status of the last command=$?*/
extern job           *job_list;            /* head of the jobs table            */

/* ================================= module contracts ======================== */

/* util.c — allocation that never returns NULL, and the strvec helpers. */
void  *xmalloc(size_t n);
void  *xrealloc(void *p, size_t n);
char  *xstrdup(const char *s);
void   strvec_init(strvec *v);
void   strvec_push(strvec *v, char *s);   /* takes ownership of s              */
void   strvec_free(strvec *v);            /* frees items and the array         */

/* match.c — glob matching (declared in match.h too; repeated for expand.c). */
int    wildcard_match(const char *pat, const char *str);

/* expand.c — filesystem glob expansion using wildcard_match. Appends every
 * matching path to `out` and returns the count; 0 means "no file matched",
 * and the caller keeps the literal pattern (bash's default "nullglob off"). */
int    glob_expand(const char *pattern, strvec *out);

/* lexer.c — pull ONE token from *cursor, advancing it. Returns 0 on success
 * (including a T_EOF at end of line) or -1 on a lexical error (message printed
 * to stderr), e.g. an unterminated quote or an over-long word. */
int    lex_next(const char **cursor, token *t);

/* parser.c — parse an entire line into a heap-allocated job (caller owns it),
 * or NULL for an empty line / parse error (*had_error distinguishes them). */
job   *parse_line(const char *line, int *had_error);
void   free_job(job *j);

/* exec.c — fork/pipe/dup2/exec the pipeline. `foreground` decides whether the
 * shell hands the terminal to the job and waits, or backgrounds it. */
void   launch_job(job *j, int foreground);

/* builtins.c — cd/exit/export/jobs/fg/bg. run_builtin executes in the CURRENT
 * process (so cd/export can change the shell) and returns an exit status. */
int    is_builtin(const char *name);
int    run_builtin(process *p);

/* jobs.c — the job table and the terminal/signal dance. */
void   init_shell(void);                 /* claim our pgrp + the terminal       */
job   *add_job(job *j);                   /* link into job_list, assign an id    */
void   do_job_notification(void);        /* reap bg jobs, print Done/Stopped     */
void   put_job_in_foreground(job *j, int cont);
void   put_job_in_background(job *j, int cont);
void   wait_for_job(job *j);             /* block until job stops or completes  */
void   mark_job_running(job *j);         /* clear stopped flags (for fg/bg)     */
void   format_job_info(job *j, const char *status);
int    job_is_stopped(job *j);
int    job_is_completed(job *j);
int    job_status_code(job *j);          /* -> shell exit status of the job     */
job   *find_job_by_id(int id);

#endif /* SHELL_H */
