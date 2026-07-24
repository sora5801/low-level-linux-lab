/* ===========================================================================
 * main.c — the read/parse/execute loop and the shell's global state.
 * ===========================================================================
 *
 * The loop is deliberately tiny; all the substance lives in the modules it
 * drives:
 *
 *     init_shell()            -> claim our process group and the terminal
 *     do_job_notification()   -> reap & report background jobs before prompting
 *     getline()               -> read one line (Ctrl-D / EOF ends the shell)
 *     parse_line()            -> tokens -> a job (a pipeline of processes)
 *     add_job() + launch_job()-> run it in the fore/background
 *
 * Reading the loop top to bottom is the fastest way to see how a shell's turn
 * actually works.
 * ===========================================================================
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "shell.h"

/* ---- the global shell state declared `extern` in shell.h ------------------ */
/* One definition each, here. Job control needs a handful of process-wide facts;
 * keeping them in one file makes the ownership obvious. */
pid_t          shell_pgid;            /* the shell's own process group id       */
struct termios shell_tmodes;          /* the shell's baseline terminal modes    */
int            shell_terminal;        /* fd of the controlling terminal (0)     */
int            shell_is_interactive;  /* isatty(shell_terminal)?                */
pid_t          shell_pid;             /* getpid(), used to expand $$            */
int            last_status;           /* exit status of the last command ($?)   */
job           *job_list;              /* head of the jobs table                 */

/* prompt — the string printed before each line. Kept minimal; a real shell
 * expands $PS1. We write it to stderr so that a stray redirection of the
 * shell's own stdout would not swallow the prompt. */
static const char *prompt(void)
{
    return "mysh$ ";
}

int main(void)
{
    /* Become master of our terminal (or note that we are non-interactive). */
    init_shell();

    char   *line = NULL;   /* getline() allocates/grows this for us            */
    size_t  cap  = 0;

    for (;;) {
        /* Before prompting, notice any background jobs that finished or stopped
         * while we were idle, print their status, and free the finished ones. */
        do_job_notification();

        if (shell_is_interactive)
            fputs(prompt(), stderr);

        /* getline(3): read a full line including the trailing '\n'. It returns
         * the byte count, or -1 at EOF (Ctrl-D on an empty line) or on error. */
        ssize_t n = getline(&line, &cap, stdin);
        if (n < 0) {
            if (shell_is_interactive)
                fputs("\n", stderr);   /* tidy the terminal after Ctrl-D        */
            break;
        }

        /* Strip the newline so it does not leak into the last token / job text. */
        if (n > 0 && line[n - 1] == '\n')
            line[n - 1] = '\0';

        int  had_error = 0;
        job *j = parse_line(line, &had_error);
        if (!j)
            continue;                  /* empty line or parse error: next prompt */

        /* A lone builtin (cd/exit/export/jobs/fg/bg with no pipe) runs IN the
         * shell and forks nothing, so it must NOT go in the job table — it has no
         * pgid and no child to reap, and tracking it would leak an entry that
         * never completes. launch_job runs it directly; we then free it. */
        process *p0 = j->first_process;
        if (p0 && !p0->next && is_builtin(p0->argv[0])) {
            launch_job(j, 1);          /* sets last_status; may exit() on `exit`  */
            free_job(j);
            continue;
        }

        /* Otherwise it is a real (forking) pipeline: register it so `jobs`/`fg`/
         * `bg` and the reaper can find it, then run it fore- or background. */
        add_job(j);
        launch_job(j, !j->background);
    }

    free(line);
    return last_status;
}
