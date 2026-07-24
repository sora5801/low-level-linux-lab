/* ===========================================================================
 * jobs.c — the job table and the terminal/signal "dance" of job control.
 * ===========================================================================
 *
 * This is THE file to read. Everything else (parsing, globbing, exec) is table
 * stakes; JOB CONTROL is the part real tutorials skip. The problem it solves:
 * a Unix terminal can only be "owned" by ONE process group at a time (its
 * foreground group). Whoever owns it receives keyboard-generated signals —
 * Ctrl-C -> SIGINT, Ctrl-Z -> SIGTSTP — and may read from it without being
 * stopped. So to run `vim` in the foreground and later `Ctrl-Z` it back to the
 * shell, the shell must choreograph terminal ownership precisely.
 *
 * THE CAST OF SYSCALLS (each explained at its use site below)
 *   setpgid(pid, pgid)          put a process into a process group
 *   tcsetpgrp(fd, pgid)         make `pgid` the terminal's FOREGROUND group
 *   tcgetpgrp(fd)               ask who the foreground group currently is
 *   waitpid(pid, &st, WUNTRACED)  reap children AND learn about stopped ones
 *   kill(-pgid, SIGCONT)        resume an entire stopped group
 *   tcgetattr/tcsetattr         save & restore terminal line-discipline modes
 *
 * THE SIGNAL POSTURE
 *   The interactive shell IGNORES SIGINT/SIGQUIT/SIGTSTP/SIGTTIN/SIGTTOU. It has
 *   to: if it owns the terminal between jobs, an accidental Ctrl-C must not kill
 *   the shell, and a background write must not stop it. Children RESET these to
 *   SIG_DFL just before exec (see exec.c) so that Ctrl-C reaches THEM. We use NO
 *   SIGCHLD handler; instead we reap synchronously with waitpid — simpler to
 *   reason about and free of async-signal-safety hazards.
 *
 * The structure follows the GNU libc manual's job-control chapter closely, on
 * purpose: it is the canonical, correct reference, and matching it makes this
 * code auditable line by line.
 * ===========================================================================
 */
#define _GNU_SOURCE
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>    /* waitpid, WUNTRACED, WIF* macros                    */
#include <termios.h>     /* tcsetpgrp, tcgetattr, tcsetattr                    */
#include <unistd.h>      /* getpid, getpgrp, setpgid, isatty, STDIN_FILENO     */

#include "shell.h"

/* ---------------------------------------------------------------------------
 * init_shell — run once at startup. Make the shell a well-behaved job-control
 * "session leader-ish" master of its terminal. Skipped when not interactive
 * (e.g. `mysh < script`), where there is no terminal to arbitrate.
 * --------------------------------------------------------------------------- */
void init_shell(void)
{
    shell_terminal       = STDIN_FILENO;
    shell_is_interactive = isatty(shell_terminal);
    shell_pid            = getpid();

    if (shell_is_interactive) {
        /* We can only drive job control if WE are the terminal's foreground
         * group. If the shell was itself started in the background, reading or
         * calling tcsetpgrp would earn us a SIGTTIN/SIGTTOU. So we loop: while
         * we are NOT the foreground group, stop ourselves by sending our own
         * group a SIGTTIN. When a human eventually `fg`s us, tcgetpgrp will
         * finally equal our pgrp and we fall through. */
        while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
            kill(-shell_pgid, SIGTTIN);

        /* Ignore the interactive/job-control signals in the shell itself. The
         * terminal driver sends these to whichever group is in the foreground;
         * by ignoring them we ensure that while the SHELL holds the terminal
         * (at the prompt), Ctrl-C/Ctrl-Z do nothing destructive. Children undo
         * this with SIG_DFL before exec so the signals reach the running job. */
        signal(SIGINT,  SIG_IGN);   /* Ctrl-C  */
        signal(SIGQUIT, SIG_IGN);   /* Ctrl-\  */
        signal(SIGTSTP, SIG_IGN);   /* Ctrl-Z  */
        signal(SIGTTIN, SIG_IGN);   /* bg read from terminal */
        signal(SIGTTOU, SIG_IGN);   /* bg write to terminal / tcsetpgrp */

        /* Put the shell into its OWN process group, with pgid == our pid, so we
         * are a group of one that the kernel will never confuse with a job.
         *
         * EPERM here is expected and harmless in ONE case: if we are already a
         * session leader (our pgid already equals our pid — e.g. we were started
         * under a fresh setsid(), as a pty test harness does), the kernel forbids
         * moving a session leader, but the postcondition we wanted already holds.
         * Any OTHER error is fatal. A normal shell launched from a login/terminal
         * is not the session leader, so the call simply succeeds. */
        shell_pgid = getpid();
        if (setpgid(shell_pgid, shell_pgid) < 0 && errno != EPERM) {
            perror("shell: couldn't put the shell in its own process group");
            exit(1);
        }

        /* Take ownership of the terminal: become its foreground group. */
        tcsetpgrp(shell_terminal, shell_pgid);

        /* Snapshot the default terminal modes. When a foreground job stops or
         * exits, we restore THESE so a program that mangled the line discipline
         * (raw mode, no echo, ...) cannot leave the shell's terminal broken. */
        tcgetattr(shell_terminal, &shell_tmodes);
    }
}

/* ---------------------------------------------------------------------------
 * add_job — link a freshly-parsed job into the table and give it the next id.
 * ids are "smallest unused-ish": max existing id + 1, which is fine for a
 * teaching shell (bash reuses freed low numbers; not worth the bookkeeping).
 * --------------------------------------------------------------------------- */
job *add_job(job *j)
{
    int    maxid = 0;
    job  **pp    = &job_list;
    while (*pp) {
        if ((*pp)->id > maxid)
            maxid = (*pp)->id;
        pp = &(*pp)->next;
    }
    j->id   = maxid + 1;
    j->next = NULL;
    *pp     = j;          /* append at the tail                                */
    return j;
}

job *find_job_by_id(int id)
{
    for (job *j = job_list; j; j = j->next)
        if (j->id == id)
            return j;
    return NULL;
}

/* job_is_stopped/completed — a job's aggregate state is derived from its
 * processes. A pipeline is "stopped" once none of its members are still running,
 * and "completed" once every member has exited. */
int job_is_stopped(job *j)
{
    for (process *p = j->first_process; p; p = p->next)
        if (!p->completed && !p->stopped)
            return 0;      /* found a still-running member                     */
    return 1;
}

int job_is_completed(job *j)
{
    for (process *p = j->first_process; p; p = p->next)
        if (!p->completed)
            return 0;
    return 1;
}

/* job_status_code — collapse the pipeline's outcome into a single $?-style code.
 * Convention: the LAST stage decides the status (bash without `pipefail`).
 * 0..255 for a normal exit; 128+signal for a kill; 128+signal for a stop. */
int job_status_code(job *j)
{
    process *p = j->first_process;
    if (!p)
        return 0;
    while (p->next)                    /* walk to the last stage               */
        p = p->next;
    int s = p->status;
    if (WIFEXITED(s))   return WEXITSTATUS(s);
    if (WIFSIGNALED(s)) return 128 + WTERMSIG(s);
    if (WIFSTOPPED(s))  return 128 + WSTOPSIG(s);
    return 0;
}

/* mark_job_running — used by fg/bg: clear the stopped flags so the job counts as
 * running again, and reset the "already told the user it stopped" latch. */
void mark_job_running(job *j)
{
    for (process *p = j->first_process; p; p = p->next)
        p->stopped = 0;
    j->notified = 0;
}

/* ---------------------------------------------------------------------------
 * mark_process_status — record one waitpid result against the process it names.
 *
 * Returns 0 if `pid` was found and updated, -1 otherwise (invalid pid, no
 * children, or a pid we do not track). The odd-looking return convention exists
 * so the wait loops below can spin with `while (!mark_process_status(...))`.
 * --------------------------------------------------------------------------- */
static int mark_process_status(pid_t pid, int status)
{
    if (pid > 0) {
        /* Find the process across ALL jobs — a single waitpid(-1) may hand us a
         * background job's status while we are blocked on a foreground one. */
        for (job *j = job_list; j; j = j->next) {
            for (process *p = j->first_process; p; p = p->next) {
                if (p->pid == pid) {
                    p->status = status;
                    if (WIFSTOPPED(status)) {
                        p->stopped = 1;         /* Ctrl-Z / SIGSTOP etc.        */
                    } else {
                        p->completed = 1;       /* exited or was killed          */
                        if (WIFSIGNALED(status))
                            fprintf(stderr, "shell: pid %d terminated by signal %d\n",
                                    (int)pid, WTERMSIG(status));
                    }
                    return 0;
                }
            }
        }
        fprintf(stderr, "shell: no child process %d\n", (int)pid);
        return -1;
    } else if (pid == 0 || errno == ECHILD) {
        /* pid==0: WNOHANG poll with nothing ready. ECHILD: no children at all.
         * Either way there is nothing more to report right now. */
        return -1;
    } else {
        perror("waitpid");
        return -1;
    }
}

/* update_status — NON-blocking: drain any pending child status changes without
 * waiting. Called before printing a prompt so background completions/stops are
 * noticed. WNOHANG makes each waitpid return immediately. */
static void update_status(void)
{
    int   status;
    pid_t pid;
    do {
        pid = waitpid(-1, &status, WUNTRACED | WNOHANG);
    } while (!mark_process_status(pid, status));
}

/* ---------------------------------------------------------------------------
 * wait_for_job — BLOCK until the given foreground job either stops or finishes.
 *
 * We keep calling waitpid(-1, WUNTRACED): WUNTRACED is the crucial flag — it
 * makes waitpid return for a child that STOPPED (Ctrl-Z), not just for one that
 * exited. Without it, Ctrl-Z would hang the shell forever. The loop ends as soon
 * as mark_process_status fails (no more children to reap) OR the job as a whole
 * is stopped/completed.
 * --------------------------------------------------------------------------- */
void wait_for_job(job *j)
{
    int   status;
    pid_t pid;
    do {
        pid = waitpid(-1, &status, WUNTRACED);
    } while (!mark_process_status(pid, status) &&
             !job_is_stopped(j) &&
             !job_is_completed(j));
}

/* ---------------------------------------------------------------------------
 * put_job_in_foreground — hand the terminal to the job, wait, then take it back.
 *
 * `cont` is true when we are RESUMING a stopped job (from `fg`), in which case we
 * must restore the job's saved terminal modes and send SIGCONT before waiting.
 * The ordering here is a well-known minefield; each step is commented.
 * --------------------------------------------------------------------------- */
void put_job_in_foreground(job *j, int cont)
{
    /* (1) Give the job the terminal. From now until we take it back, Ctrl-C and
     * Ctrl-Z are delivered to the JOB's group, not the shell. */
    tcsetpgrp(shell_terminal, j->pgid);

    /* (2) If resuming, restore the terminal modes the job was using when it
     * stopped, then wake the whole group. SIGCONT is sent to -pgid (the group). */
    if (cont) {
        tcsetattr(shell_terminal, TCSADRAIN, &j->tmodes);
        if (kill(-j->pgid, SIGCONT) < 0)
            perror("kill (SIGCONT)");
    }

    /* (3) Wait for the job to stop or complete. */
    wait_for_job(j);

    /* (4) Put the SHELL back in the foreground — reclaim the terminal. */
    tcsetpgrp(shell_terminal, shell_pgid);

    /* (5) Save whatever modes the job left behind (so a later `fg` resumes with
     * them), then restore the shell's own sane modes. This is what rescues your
     * terminal after a program that switched to raw mode and then stopped. */
    tcgetattr(shell_terminal, &j->tmodes);
    tcsetattr(shell_terminal, TCSADRAIN, &shell_tmodes);
}

/* ---------------------------------------------------------------------------
 * put_job_in_background — the easy sibling: just (optionally) continue it. We do
 * NOT touch tcsetpgrp, so the shell keeps the terminal and the job runs blind;
 * if it tries to read the terminal it will get SIGTTIN and stop, which is
 * exactly the desired behavior for a background job.
 * --------------------------------------------------------------------------- */
void put_job_in_background(job *j, int cont)
{
    if (cont)
        if (kill(-j->pgid, SIGCONT) < 0)
            perror("kill (SIGCONT)");
}

/* format_job_info — one-line status report, e.g. "[1] 4213 stopped  sleep 100". */
void format_job_info(job *j, const char *status)
{
    fprintf(stderr, "[%d] %ld %s\t%s\n",
            j->id, (long)j->pgid, status, j->command ? j->command : "");
}

/* ---------------------------------------------------------------------------
 * do_job_notification — housekeeping run before each prompt. Poll for status
 * changes, announce finished/stopped BACKGROUND jobs, and unlink+free jobs that
 * are entirely done. Foreground jobs that completed are removed silently (the
 * user already watched them run). Stopped jobs are announced exactly once.
 * --------------------------------------------------------------------------- */
void do_job_notification(void)
{
    update_status();                   /* absorb any pending waitpid results    */

    job *j    = job_list;
    job *last = NULL;
    while (j) {
        job *next = j->next;

        if (job_is_completed(j)) {
            /* Announce only background completions; foreground ones are noise. */
            if (j->background)
                format_job_info(j, "done");
            /* Unlink from the list, then free everything the job owns. */
            if (last) last->next = next;
            else      job_list   = next;
            free_job(j);
        } else if (job_is_stopped(j) && !j->notified) {
            format_job_info(j, "stopped");
            j->notified = 1;           /* latch so we do not repeat it          */
            last = j;
        } else {
            last = j;                  /* still running: leave it be            */
        }
        j = next;
    }
}
