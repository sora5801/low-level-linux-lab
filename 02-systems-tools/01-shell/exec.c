/* ===========================================================================
 * exec.c — run a pipeline: fork, wire pipes with dup2, apply redirections, exec.
 * ===========================================================================
 *
 * A pipeline `a | b | c` becomes three children whose stdout/stdin are chained
 * by pipes, all placed in ONE process group so the terminal and signals treat
 * them as a single job. The tricky, must-get-right details:
 *
 *   - PROCESS GROUP RACE: both the parent and each child call setpgid() for that
 *     child. Whichever runs first wins; doing it in BOTH removes the race where
 *     the parent tries to tcsetpgrp() a group the child hasn't joined yet.
 *   - PIPE FD HYGIENE: every fd of every pipe must be closed in every process
 *     that does not need it, or readers never see EOF and the pipeline hangs.
 *   - SIGNALS: children restore SIG_DFL so Ctrl-C/Ctrl-Z reach the job, not the
 *     (signal-ignoring) shell.
 *   - execvp only returns on FAILURE; the child must _exit, never return into
 *     the shell's code with a duplicated job table.
 * ===========================================================================
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>       /* open, O_* flags                                    */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>      /* strerror                                          */
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>      /* fork, execvp, dup2, pipe, close, setpgid, _exit    */

#include "shell.h"

/* ---------------------------------------------------------------------------
 * apply_redirections — open each redirect target and dup2 it onto the right fd.
 * Runs in the CHILD (after pipe wiring), so an error here must _exit the child.
 * Returns 0 on success, -1 on the first failure (message already printed).
 *
 * dup2(oldfd, newfd): make newfd refer to the same open-file description as
 * oldfd, atomically closing whatever newfd was. That is how "> file" makes fd 1
 * point at the file. We close the temporary `fd` afterward since it has served
 * its purpose. Redirections apply LEFT TO RIGHT and OVERRIDE pipe wiring, so
 * `cmd > f | c` sends cmd's stdout to the file, not the pipe — same as bash.
 * --------------------------------------------------------------------------- */
static int apply_redirections(process *p)
{
    for (redir *r = p->redirs; r; r = r->next) {
        int fd, target;
        switch (r->type) {
        case REDIR_IN:
            /* open(2) O_RDONLY: fd 0 reads from the file. ENOENT if it is
             * missing — a common, expected error we report cleanly. */
            fd = open(r->file, O_RDONLY);
            target = STDIN_FILENO;
            break;
        case REDIR_OUT:
            /* O_WRONLY|O_CREAT|O_TRUNC, mode 0644: create-or-truncate for '>'. */
            fd = open(r->file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            target = STDOUT_FILENO;
            break;
        case REDIR_APPEND:
            /* O_APPEND instead of O_TRUNC for '>>': every write seeks to end. */
            fd = open(r->file, O_WRONLY | O_CREAT | O_APPEND, 0644);
            target = STDOUT_FILENO;
            break;
        case REDIR_ERR:
        default:
            fd = open(r->file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            target = STDERR_FILENO;
            break;
        }
        if (fd < 0) {
            fprintf(stderr, "%s: %s\n", r->file, strerror(errno));
            return -1;
        }
        if (dup2(fd, target) < 0) {
            perror("dup2");
            close(fd);
            return -1;
        }
        close(fd);                 /* the target fd now holds the description   */
    }
    return 0;
}

/* ---------------------------------------------------------------------------
 * launch_process — the CHILD side. Never returns: it either execs the program
 * or _exit()s. Runs after fork() in the child, so it must undo the shell's
 * signal posture and wire up its fds before handing off to the new program.
 *
 * infile/outfile/errfile are the fds this stage should use for 0/1/2 (they come
 * from the pipe wiring in launch_job); pgid is the group to join; foreground
 * says whether this job should grab the terminal.
 * --------------------------------------------------------------------------- */
static void launch_process(process *p, pid_t pgid,
                           int infile, int outfile, int errfile,
                           int foreground)
{
    if (shell_is_interactive) {
        /* Join (or create) the job's process group. Doing this in the child as
         * well as the parent closes the fork/exec race. If pgid is 0 we are the
         * first process, so the group id becomes our own pid. */
        pid_t pid = getpid();
        if (pgid == 0)
            pgid = pid;
        setpgid(pid, pgid);

        /* If this is a foreground job, grab the terminal. The parent also does
         * this; again, both is intentional and harmless. */
        if (foreground)
            tcsetpgrp(shell_terminal, pgid);

        /* Restore default signal handling so Ctrl-C/Ctrl-Z affect THIS job.
         * The shell set these to SIG_IGN; a child that kept ignoring them would
         * be unkillable from the keyboard. */
        signal(SIGINT,  SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);
    }

    /* Move the pipe endpoints onto the standard fds, then close the originals.
     * The guard (infile != STDIN_FILENO) avoids a pointless dup2(fd,fd) and,
     * more importantly, avoids closing fd 0/1/2 when this stage already uses it. */
    if (infile != STDIN_FILENO)  { dup2(infile,  STDIN_FILENO);  close(infile);  }
    if (outfile != STDOUT_FILENO){ dup2(outfile, STDOUT_FILENO); close(outfile); }
    if (errfile != STDERR_FILENO){ dup2(errfile, STDERR_FILENO); close(errfile); }

    /* Explicit redirections come AFTER pipe wiring so they win (bash semantics). */
    if (apply_redirections(p) < 0)
        _exit(1);

    /* A builtin inside a pipeline runs here, in the child, then exits — its
     * side effects cannot touch the shell, which is what we want. */
    if (is_builtin(p->argv[0]))
        _exit(run_builtin(p));

    /* execvp(2): replace this process image with the program named argv[0],
     * searching $PATH for a bare name. On success it NEVER returns. On failure
     * (ENOENT = not found, EACCES = not executable) it returns -1 and we exit
     * 127, the conventional "command not found" status. */
    execvp(p->argv[0], p->argv);
    fprintf(stderr, "%s: %s\n", p->argv[0], strerror(errno));
    _exit(127);
}

/* ---------------------------------------------------------------------------
 * run_single_builtin_in_shell — a lone builtin (not in a pipeline) must run in
 * the shell process so cd/export/exit take effect. We still honor redirections
 * by saving fds 0/1/2, applying the redirects, running, and restoring. Returns
 * the builtin's exit status.
 * --------------------------------------------------------------------------- */
static int run_single_builtin_in_shell(process *p)
{
    /* No redirections: the common, fast path — just run it. */
    if (!p->redirs)
        return run_builtin(p);

    /* Save the shell's standard fds so we can restore them afterward. dup(fd)
     * returns a new fd referring to the same description; we stash 0/1/2. */
    int saved_in  = dup(STDIN_FILENO);
    int saved_out = dup(STDOUT_FILENO);
    int saved_err = dup(STDERR_FILENO);

    int rc;
    if (apply_redirections(p) < 0) {
        rc = 1;
    } else {
        rc = run_builtin(p);
    }

    /* Restore and drop the saved copies. Order does not matter; each dup2 puts
     * the original description back onto its standard fd. */
    if (saved_in  >= 0) { dup2(saved_in,  STDIN_FILENO);  close(saved_in);  }
    if (saved_out >= 0) { dup2(saved_out, STDOUT_FILENO); close(saved_out); }
    if (saved_err >= 0) { dup2(saved_err, STDERR_FILENO); close(saved_err); }
    return rc;
}

/* ---------------------------------------------------------------------------
 * launch_job — fork every stage, wire the pipes, then either wait (foreground)
 * or return (background). See the file header for the invariants.
 * --------------------------------------------------------------------------- */
void launch_job(job *j, int foreground)
{
    /* Special case: a single builtin runs in the shell itself. */
    if (!j->first_process->next && is_builtin(j->first_process->argv[0])) {
        last_status = run_single_builtin_in_shell(j->first_process);
        return;
    }

    int mypipe[2];
    int infile  = j->stdin_fd;     /* what THIS stage reads from (starts at 0)  */
    int outfile;                   /* what THIS stage writes to                 */

    for (process *p = j->first_process; p; p = p->next) {
        /* Create the pipe to the NEXT stage, unless we are the last stage. */
        if (p->next) {
            if (pipe(mypipe) < 0) {
                /* pipe(2) fills mypipe = {read end, write end}. EMFILE/ENFILE on
                 * fd exhaustion. Fatal for this job; report and stop launching. */
                perror("pipe");
                break;
            }
            outfile = mypipe[1];   /* this stage writes into the pipe           */
        } else {
            outfile = j->stdout_fd;/* last stage writes to the job's stdout     */
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            /* Close this stage's pipe ends so we do not leak fds, then bail. */
            if (p->next) { close(mypipe[0]); close(mypipe[1]); }
            break;
        } else if (pid == 0) {
            /* CHILD: never returns. */
            launch_process(p, j->pgid, infile, outfile, j->stderr_fd, foreground);
        } else {
            /* PARENT: record the pid and (racily, deliberately) set the group. */
            p->pid = pid;
            if (shell_is_interactive) {
                if (!j->pgid)
                    j->pgid = pid;          /* first child defines the pgid      */
                setpgid(pid, j->pgid);      /* mirror the child's setpgid        */
            }
        }

        /* PARENT fd hygiene: close the ends we handed to the child. If we keep
         * the write end open, the reader downstream never gets EOF. */
        if (infile != j->stdin_fd)
            close(infile);
        if (outfile != j->stdout_fd)
            close(outfile);

        /* The next stage reads from this pipe's read end. Only valid when we
         * actually made a pipe (i.e. this was not the last stage). */
        if (p->next)
            infile = mypipe[0];
    }

    /* Now decide how to wait. */
    if (!shell_is_interactive) {
        /* No job control (e.g. reading a script): just reap synchronously. */
        wait_for_job(j);
        last_status = job_status_code(j);
    } else if (foreground) {
        put_job_in_foreground(j, 0);        /* cont=0: not resuming, just wait   */
        last_status = job_status_code(j);
    } else {
        put_job_in_background(j, 0);
        format_job_info(j, "running &");    /* announce "[id] pgid running & …"  */
        last_status = 0;                    /* backgrounding "succeeds" now       */
    }
}
