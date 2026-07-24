/* ===========================================================================
 * builtins.c — commands the shell must run in its OWN process.
 * ===========================================================================
 *
 * Why builtins exist at all: some effects are impossible from a child. `cd` must
 * change the SHELL's working directory (a child's chdir dies with the child);
 * `export` must alter the SHELL's environment; `fg`/`bg`/`jobs` manipulate the
 * SHELL's job table; `exit` must terminate the SHELL. So these run in-process.
 *
 * The one wrinkle (handled in exec.c): a builtin used inside a PIPELINE, e.g.
 * `echo hi | cat`, runs in a forked child like any pipeline stage, so its
 * side effects (if any) are correctly confined to that child — matching bash.
 * ===========================================================================
 */
#define _GNU_SOURCE
#include <errno.h>
#include <limits.h>      /* PATH_MAX                                          */
#include <stdio.h>
#include <stdlib.h>      /* getenv, setenv, atoi, exit                        */
#include <string.h>
#include <unistd.h>      /* chdir, getcwd                                     */

#include "shell.h"

/* The environment array, for `export` with no arguments. Declared by <unistd.h>
 * under _GNU_SOURCE, but we declare it explicitly to be unambiguous. */
extern char **environ;

/* is_builtin — is this command name handled in-process? Cheap string compares;
 * the set is small and fixed. */
int is_builtin(const char *name)
{
    return  !strcmp(name, "cd")     || !strcmp(name, "exit")   ||
            !strcmp(name, "export") || !strcmp(name, "jobs")   ||
            !strcmp(name, "fg")     || !strcmp(name, "bg");
}

/* ------------------------------------------------------------------ cd ----- */
/* cd [dir] — change directory. No arg -> $HOME; "cd -" -> $OLDPWD (and echo it).
 * Keeps $PWD and $OLDPWD in sync so scripts and prompts can read them. */
static int builtin_cd(char **argv)
{
    const char *dir = argv[1];

    if (!dir) {
        dir = getenv("HOME");
        if (!dir) { fprintf(stderr, "cd: HOME not set\n"); return 1; }
    } else if (!strcmp(dir, "-")) {
        dir = getenv("OLDPWD");
        if (!dir) { fprintf(stderr, "cd: OLDPWD not set\n"); return 1; }
        printf("%s\n", dir);           /* bash prints the target for "cd -"     */
    }

    /* Remember where we are so we can set $OLDPWD after a successful chdir. */
    char oldpwd[PATH_MAX];
    if (!getcwd(oldpwd, sizeof oldpwd))
        oldpwd[0] = '\0';

    /* chdir(2): change the calling process's working directory. Errors we care
     * about: ENOENT (no such dir), ENOTDIR (a path component isn't a dir),
     * EACCES (search permission denied). We surface the errno text either way. */
    if (chdir(dir) < 0) {
        fprintf(stderr, "cd: %s: %s\n", dir, strerror(errno));
        return 1;
    }

    if (oldpwd[0])
        setenv("OLDPWD", oldpwd, 1);
    char newpwd[PATH_MAX];
    if (getcwd(newpwd, sizeof newpwd))
        setenv("PWD", newpwd, 1);
    return 0;
}

/* ----------------------------------------------------------------- exit ---- */
/* exit [n] — leave the shell with status n (default: last command's status). */
static int builtin_exit(char **argv)
{
    int code = last_status;
    if (argv[1])
        code = atoi(argv[1]);
    /* A production shell would warn if there are stopped jobs ("There are
     * stopped jobs."). We keep it simple and just leave. */
    exit(code);
    return 0;   /* not reached */
}

/* --------------------------------------------------------------- export ---- */
/* export [NAME[=VALUE] ...] — put variables into the environment so children
 * inherit them. With no args, list the environment in `export NAME=VALUE` form. */
static int builtin_export(char **argv)
{
    if (!argv[1]) {
        for (char **e = environ; *e; e++)
            printf("export %s\n", *e);
        return 0;
    }
    for (int i = 1; argv[i]; i++) {
        char *eq = strchr(argv[i], '=');
        if (eq) {
            /* Split "NAME=VALUE" at the '=' by temporarily NUL-ing it. We put
             * it back afterward so we never mutate the caller's argv for good. */
            *eq = '\0';
            if (setenv(argv[i], eq + 1, 1) < 0)   /* 1 = overwrite existing     */
                perror("export");
            *eq = '=';
        } else if (!getenv(argv[i])) {
            /* `export NAME` with NAME unset: create it empty, so it is exported. */
            setenv(argv[i], "", 1);
        }
    }
    return 0;
}

/* ----------------------------------------------------------------- jobs ---- */
/* jobs — list the job table with each job's aggregate state. */
static int builtin_jobs(char **argv)
{
    (void)argv;
    for (job *j = job_list; j; j = j->next) {
        const char *st = job_is_completed(j) ? "Done"
                       : job_is_stopped(j)   ? "Stopped"
                                             : "Running";
        printf("[%d] %ld %s\t%s\n", j->id, (long)j->pgid, st,
               j->command ? j->command : "");
    }
    return 0;
}

/* pick_job — resolve an fg/bg argument to a job. "%n" or "n" selects job n;
 * with no argument, the most recently added job (the list tail). */
static job *pick_job(const char *spec)
{
    if (spec) {
        if (spec[0] == '%')
            spec++;
        int id = atoi(spec);
        return find_job_by_id(id);
    }
    /* No spec: return the last job in the table (the "current" job). */
    job *last = NULL;
    for (job *j = job_list; j; j = j->next)
        last = j;
    return last;
}

/* ------------------------------------------------------------------ fg ----- */
/* fg [%n] — resume a job in the FOREGROUND: hand it the terminal, SIGCONT it,
 * and block until it stops or finishes. put_job_in_foreground does the dance. */
static int builtin_fg(char **argv)
{
    job *j = pick_job(argv[1]);
    if (!j) { fprintf(stderr, "fg: no such job\n"); return 1; }

    fprintf(stderr, "%s\n", j->command ? j->command : "");  /* echo, like bash */
    mark_job_running(j);
    j->background = 0;
    put_job_in_foreground(j, 1);       /* cont=1: send SIGCONT before waiting   */
    return job_status_code(j);
}

/* ------------------------------------------------------------------ bg ----- */
/* bg [%n] — resume a stopped job in the BACKGROUND: SIGCONT it but do NOT give
 * it the terminal, so control returns immediately to the prompt. */
static int builtin_bg(char **argv)
{
    job *j = pick_job(argv[1]);
    if (!j) { fprintf(stderr, "bg: no such job\n"); return 1; }

    mark_job_running(j);
    j->background = 1;
    format_job_info(j, "running &");
    put_job_in_background(j, 1);        /* cont=1: send SIGCONT                  */
    return 0;
}

/* run_builtin — dispatch to the handler for p->argv[0]. Precondition: the caller
 * already checked is_builtin(p->argv[0]). Returns the builtin's exit status. */
int run_builtin(process *p)
{
    char **argv = p->argv;
    if      (!strcmp(argv[0], "cd"))     return builtin_cd(argv);
    else if (!strcmp(argv[0], "exit"))   return builtin_exit(argv);
    else if (!strcmp(argv[0], "export")) return builtin_export(argv);
    else if (!strcmp(argv[0], "jobs"))   return builtin_jobs(argv);
    else if (!strcmp(argv[0], "fg"))     return builtin_fg(argv);
    else if (!strcmp(argv[0], "bg"))     return builtin_bg(argv);
    return 1;                            /* unreachable given the precondition   */
}
