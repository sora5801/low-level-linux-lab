/* ===========================================================================
 * supervisor.c — a process supervisor that can run as PID 1.
 * ===========================================================================
 *
 * WHY PID 1 IS SPECIAL (read this first — it justifies everything below)
 * ---------------------------------------------------------------------
 * The very first user-space process the kernel starts gets PID 1, and the kernel
 * treats it unlike any other process:
 *
 *   1. IT IS THE UNIVERSAL REAPER. When any process dies, its not-yet-waited-on
 *      exit status becomes a "zombie" until its parent calls wait(). If a
 *      process's parent has already died, the orphan is RE-PARENTED to PID 1.
 *      So every orphaned zombie in the system (or in a PID namespace/container)
 *      eventually lands on PID 1. If PID 1 does not wait() them, they pile up as
 *      un-killable zombies and leak the process table. Reaping is therefore not
 *      optional for an init — it is its defining job. (Outside a container we
 *      opt into the same duty with PR_SET_CHILD_SUBREAPER; see main().)
 *
 *   2. ITS SIGNAL DEFAULTS ARE DIFFERENT. For a normal process, signals like
 *      SIGTERM have a default disposition of "terminate the process". For PID 1
 *      the kernel DISABLES the default actions of signals that would kill it:
 *      unless PID 1 has explicitly installed a handler (or unblocked+caught the
 *      signal), SIGTERM/SIGINT/etc. are silently dropped. This prevents a stray
 *      `kill` from taking down the whole machine — but it also means a naive
 *      init that relies on the default SIGTERM behaviour will simply IGNORE
 *      shutdown requests. To receive them we must arrange to catch them; here we
 *      block them and drain them from a signalfd (see below).
 *
 *   3. IF PID 1 EXITS, EVERYTHING DIES. In the root namespace the kernel panics
 *      ("Attempted to kill init!"); in a PID namespace the kernel sends SIGKILL
 *      to every other process in that namespace and tears it down. So PID 1 must
 *      not exit until it has deliberately shut its children down, and must never
 *      crash. That is why this program uses NO dynamic allocation (nothing to
 *      fail under OOM) and checks every syscall.
 *
 * HOW WE HANDLE SIGNALS: signalfd, not a handler
 * ----------------------------------------------
 * The tempting design is a SIGCHLD handler that calls waitpid(). But async
 * signal handlers are a minefield: only async-signal-safe functions are legal
 * inside them, signals coalesce, and they race with the main loop. The robust
 * alternative used here is to BLOCK SIGCHLD/SIGTERM/SIGINT process-wide and read
 * them synchronously as data from a signalfd(2) inside our poll() loop. Now the
 * "signal handler" is just ordinary code running at a well-defined point.
 *
 * WHAT THIS TEACHING CORE DOES
 * ----------------------------
 *   - becomes a subreaper (or is PID 1) and reaps every child, tracked or not;
 *   - parses a small service config (see config.c);
 *   - orders services by their `after` dependencies (topological sort, order.c);
 *   - starts them, and restarts them on exit with exponential backoff (order.c);
 *   - on SIGTERM/SIGINT, forwards SIGTERM to all children, reaps them within a
 *     grace period, escalates to SIGKILL if needed, then exits cleanly.
 *
 * WHAT IT DELIBERATELY DOES NOT DO (honest scope; see README "Going further")
 *   - no readiness protocol: `after` orders *starts*, it does not wait for a
 *     dependency to become "ready" (that needs sd_notify / socket activation);
 *   - no cgroup/namespace setup, no mounting /proc, no getty/respawn-on-tty;
 *   - no shell quoting in `exec` (wrap in `/bin/sh -c "..."`).
 *
 * Platform: Linux only (signalfd, prctl subreaper are Linux-specific). Build on
 * Linux or WSL2. See README.md for how to exercise it as PID 1 with a container.
 * ===========================================================================
 */

#include "service.h"
#include "config.h"
#include "order.h"

#include <errno.h>          /* errno, EINTR, EAGAIN, ECHILD                    */
#include <poll.h>           /* poll                                            */
#include <signal.h>         /* sigemptyset, sigaddset, sigprocmask, kill       */
#include <stdarg.h>         /* va_list for the tiny logger                     */
#include <stdio.h>          /* snprintf, vsnprintf                             */
#include <string.h>         /* strerror, strlen, memset                        */
#include <time.h>           /* clock_gettime, CLOCK_MONOTONIC                  */
#include <unistd.h>         /* fork, execvp, getpid, read, write, close, _exit */

#include <sys/prctl.h>      /* prctl, PR_SET_CHILD_SUBREAPER                   */
#include <sys/signalfd.h>   /* signalfd, struct signalfd_siginfo, SFD_*       */
#include <sys/wait.h>       /* waitpid, WNOHANG, WIFEXITED, WEXITSTATUS, ...   */
#include <sys/types.h>      /* pid_t                                           */
#include <limits.h>         /* INT_MAX                                         */

/* --- Global supervisor state ---------------------------------------------
 * Fixed-size, statically allocated (see service.h for WHY no heap). `g_svc` is
 * the service table; `g_order` is the dependency-sorted start order. These are
 * file-scope because the small helpers below all operate on the one supervisor
 * instance; there is exactly one init per system. */
static struct sup_service g_svc[SUP_MAX_SERVICES];
static int   g_order[SUP_MAX_SERVICES];
static int   g_nsvc = 0;

/* Set once a shutdown signal arrives; the event loop then stops restarting
 * services and drives everything to exit. `volatile` is NOT needed here (we are
 * not touching this from an async signal handler — signalfd delivers signals as
 * ordinary reads), but we keep the state explicit. */
static int  g_shutting_down = 0;
static long g_kill_deadline_ms = 0;   /* when to escalate SIGTERM -> SIGKILL   */
static int  g_sent_sigkill = 0;

/* ---------------------------------------------------------------------------
 * now_ms — a monotonic millisecond clock for scheduling backoff and shutdown.
 *
 * We use CLOCK_MONOTONIC, not CLOCK_REALTIME: monotonic time never jumps
 * backwards when the admin sets the wall clock or NTP steps it, so a "restart in
 * 2 seconds" deadline stays 2 seconds. clock_gettime(2) is syscall #228 on
 * x86-64; it fills a `struct timespec {tv_sec, tv_nsec}`. On failure (which for
 * CLOCK_MONOTONIC essentially cannot happen) we fall back to 0 so arithmetic
 * stays well-defined.
 * --------------------------------------------------------------------------- */
static long now_ms(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
        return 0;
    /* seconds*1000 + nanoseconds/1e6. Both fit in a long (64-bit) for any
     * realistic uptime, so there is no overflow to guard here. */
    return (long)ts.tv_sec * 1000L + ts.tv_nsec / 1000000L;
}

/* Tiny logger. We write to stderr (fd 2) which an init typically has wired to
 * the console. Prefixed with our pid so you can see "1" when running as init. */
static void logmsg(const char *fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    int hdr = snprintf(buf, sizeof buf, "[init %ld] ", (long)getpid());
    if (hdr < 0) hdr = 0;
    if ((size_t)hdr > sizeof buf - 1) hdr = (int)(sizeof buf - 1); /* clamp (snprintf may report untruncated len) */
    int n = vsnprintf(buf + hdr, sizeof buf - (size_t)hdr, fmt, ap);
    va_end(ap);
    if (n < 0) n = 0;
    size_t total = (size_t)hdr + (size_t)n;
    if (total > sizeof buf - 1) total = sizeof buf - 1;
    buf[total] = '\n';
    /* write(2) can return short; loop until the whole line is out or we error.
     * We ignore EINTR-driven partial writes by simply retrying from where we
     * stopped. Logging failures are non-fatal — we must not let them kill init. */
    size_t off = 0, len = total + 1;
    while (off < len) {
        ssize_t w = write(2, buf + off, len - off);
        if (w < 0) { if (errno == EINTR) continue; break; }
        off += (size_t)w;
    }
}

/* Find which service owns a reaped pid, or -1 if it is an orphan we inherited
 * (a grandchild whose parent died, reparented to us because we are the reaper).
 * Linear scan over <=64 entries — trivial. */
static int service_of_pid(pid_t pid)
{
    for (int i = 0; i < g_nsvc; i++)
        if (g_svc[i].state == SUP_RUNNING && g_svc[i].pid == pid)
            return i;
    return -1;
}

/* ---------------------------------------------------------------------------
 * start_service — fork(2) + execvp(3) one service.
 *
 * fork() returns TWICE: 0 in the freshly-created child, the child's pid in the
 * parent, or -1 on failure. The child is a near-exact copy of us, so we must
 * undo the things we set up for the PARENT before exec:
 *
 *   - UNBLOCK signals. We blocked SIGCHLD/SIGTERM/SIGINT for our signalfd; a
 *     child inheriting that mask would start life unable to receive SIGTERM,
 *     which would break normal programs. We reset the mask to empty.
 *   - Give the child its OWN process group (setpgid). Then on shutdown we can
 *     signal the whole group with kill(-pgid, ...) to catch the service *and*
 *     any subprocesses it spawned.
 *
 * execvp replaces the child image with the target program; it only returns if
 * it FAILS (e.g. ENOENT). On failure the child must _exit() (not exit()) so it
 * does not run the parent's atexit handlers or flush shared stdio buffers.
 * Returns 0 on success (parent has a running child), -1 if fork itself failed.
 * --------------------------------------------------------------------------- */
static int start_service(int i)
{
    struct sup_service *s = &g_svc[i];

    pid_t pid = fork();
    if (pid < 0) {
        /* EAGAIN (RLIMIT_NPROC / kernel pid exhaustion) or ENOMEM. We leave the
         * service STOPPED and let the caller decide; init keeps running. */
        logmsg("fork failed for '%s': %s", s->name, strerror(errno));
        return -1;
    }

    if (pid == 0) {
        /* ------------------- CHILD ------------------- *
         * Everything here runs in the child. Keep it minimal and async-safe:
         * we are between fork and exec, so we avoid anything that touches
         * locks the parent might have held. */

        /* Restore the default signal mask: unblock everything we blocked. */
        sigset_t empty;
        sigemptyset(&empty);
        sigprocmask(SIG_SETMASK, &empty, 0);   /* best-effort; child is doomed if it fails */

        /* Own process group == our pid. Doing this in the child (as well as the
         * parent, below) closes the classic race where we try to signal the
         * group before setpgid has run. */
        setpgid(0, 0);

        execvp(s->argv[0], s->argv);

        /* Only reached if exec FAILED. Report and leave with 127, the shell's
         * "command not found" convention, so the parent's restart logic sees a
         * failure. _exit avoids flushing the parent's stdio copy twice. */
        const char *m1 = "exec failed: ";
        /* (void) — we are exiting anyway; there is nowhere to report a failed
         * write to, and we must not pull in error handling on the doomed path. */
        (void)write(2, m1, 13);
        (void)write(2, s->argv[0], strlen(s->argv[0]));
        (void)write(2, "\n", 1);
        _exit(127);
    }

    /* ------------------- PARENT ------------------- */
    setpgid(pid, pid);                 /* mirror of the child's setpgid (race-free) */
    s->pid = pid;
    s->state = SUP_RUNNING;
    s->started_ms = now_ms();
    logmsg("started '%s' pid=%d", s->name, (int)pid);
    return 0;
}

/* ---------------------------------------------------------------------------
 * reap_children — drain ALL exited children, then decide restarts.
 *
 * THE KEY SUBTLETY: standard signals do not queue. If three children die at
 * nearly the same instant, the kernel may deliver just ONE SIGCHLD (or, via
 * signalfd, one readable record). Therefore on every SIGCHLD we must loop
 * waitpid(-1, WNOHANG) until it reports "no more" — otherwise zombies leak.
 *
 * waitpid(2) return values:
 *   > 0            : reaped that pid; `status` describes how it died.
 *   0  (WNOHANG)   : children exist but none has changed state — stop.
 *   -1 & ECHILD    : we have no children at all — stop.
 *   -1 & EINTR     : interrupted; retry (rare here since signals are blocked).
 * --------------------------------------------------------------------------- */
static void reap_children(void)
{
    for (;;) {
        int status;
        pid_t pid = waitpid(-1, &status, WNOHANG);

        if (pid == 0)
            break;                     /* live children, none exited right now  */
        if (pid < 0) {
            if (errno == EINTR)
                continue;              /* retry the wait                        */
            /* ECHILD: nothing left to reap. Any other errno is unexpected. */
            break;
        }

        int idx = service_of_pid(pid);
        if (idx < 0) {
            /* An orphan that reparented to us (we are PID 1 / a subreaper).
             * There is nothing to restart — its parent service, if any, is
             * managed separately — but we MUST reap it or it stays a zombie.
             * This single line is the whole reason a container needs a real
             * init. */
            logmsg("reaped orphan pid=%d (subreaper duty)", (int)pid);
            continue;
        }

        struct sup_service *s = &g_svc[idx];
        s->last_status = status;
        s->pid = 0;
        s->state = SUP_STOPPED;

        /* Describe the exit for the log and to decide "did it fail?". */
        int failed;
        if (WIFEXITED(status)) {
            int code = WEXITSTATUS(status);
            failed = (code != 0);
            logmsg("'%s' exited code=%d", s->name, code);
        } else if (WIFSIGNALED(status)) {
            failed = 1;                /* killed by a signal counts as failure  */
            logmsg("'%s' killed by signal %d", s->name, WTERMSIG(status));
        } else {
            failed = 1;                /* WIFSTOPPED/CONTINUED shouldn't reach here */
            continue;
        }

        if (g_shutting_down)
            continue;                  /* during shutdown we never restart      */

        /* If the service had been up for longer than the backoff ceiling, treat
         * it as "stable" and forget its crash history — mirrors systemd's
         * StartLimitInterval so a service that runs fine for an hour then dies
         * once retries immediately instead of at the capped 30s. */
        long uptime = now_ms() - s->started_ms;
        if (uptime > (long)SUP_BACKOFF_CAP_MS)
            s->restart_count = 0;

        int want_restart =
            (s->restart == SUP_RESTART_ALWAYS) ||
            (s->restart == SUP_RESTART_ON_FAILURE && failed);

        if (!want_restart) {
            logmsg("'%s' will not be restarted (policy)", s->name);
            continue;
        }

        /* Schedule the restart. sup_backoff_delay_ms is the pure-logic routine
         * from order.c (and the star of asm/demo.annotated.s). */
        sup_u64 delay = sup_backoff_delay_ms(s->restart_count,
                                             SUP_BACKOFF_BASE_MS,
                                             SUP_BACKOFF_CAP_MS);
        s->restart_count++;
        s->next_start_ms = now_ms() + (long)delay;
        s->state = SUP_BACKOFF;
        logmsg("'%s' restarting in %lums (attempt #%u)",
               s->name, (unsigned long)delay, s->restart_count);
    }
}

/* Start any BACKOFF service whose retry deadline has arrived. Called at the top
 * of every loop turn, after poll wakes us (whether by signal or by timeout). */
static void start_due_services(long now)
{
    for (int i = 0; i < g_nsvc; i++) {
        if (g_svc[i].state == SUP_BACKOFF && now >= g_svc[i].next_start_ms)
            start_service(i);
    }
}

/* ---------------------------------------------------------------------------
 * begin_shutdown — a shutdown signal arrived; forward SIGTERM to every child.
 *
 * We latch g_shutting_down so reap_children stops restarting, record a grace
 * deadline, and signal each running service's WHOLE process group with
 * kill(-pgid, SIGTERM). Negative pid == "the process group |pid|", so a service
 * that itself spawned helpers takes them all down. SIGTERM is the polite "please
 * exit" request; well-behaved programs flush and quit. If they don't within the
 * grace window, the loop escalates to SIGKILL (see main()).
 * --------------------------------------------------------------------------- */
static void begin_shutdown(int signo)
{
    if (g_shutting_down)
        return;                        /* already shutting down; ignore repeats */
    g_shutting_down = 1;
    g_kill_deadline_ms = now_ms() + SUP_SHUTDOWN_GRACE_MS;
    logmsg("shutdown requested (signal %d); forwarding SIGTERM to children", signo);

    for (int i = 0; i < g_nsvc; i++) {
        if (g_svc[i].state == SUP_RUNNING && g_svc[i].pid > 0) {
            /* kill(2): send SIGTERM to the process group. ESRCH (group already
             * gone) is fine and ignored — the child may have died between our
             * check and here; the subsequent reap will notice. */
            if (kill(-g_svc[i].pid, SIGTERM) != 0 && errno != ESRCH)
                logmsg("kill(%d, SIGTERM) failed: %s", (int)g_svc[i].pid, strerror(errno));
        }
        /* A service waiting in BACKOFF should not come back mid-shutdown. */
        if (g_svc[i].state == SUP_BACKOFF)
            g_svc[i].state = SUP_STOPPED;
    }
}

/* True once no service is RUNNING (or waiting to run). */
static int all_stopped(void)
{
    for (int i = 0; i < g_nsvc; i++)
        if (g_svc[i].state == SUP_RUNNING || g_svc[i].state == SUP_BACKOFF)
            return 0;
    return 1;
}

/* Compute the poll() timeout in milliseconds: the time until the nearest future
 * event (a due backoff restart, or the shutdown SIGKILL escalation). Returns -1
 * for "block forever" when nothing is scheduled, or 0 when something is already
 * due (poll returns immediately and the top-of-loop handlers fire). */
static int compute_timeout(long now)
{
    long best = -1;                    /* -1 == no deadline == infinite         */

    for (int i = 0; i < g_nsvc; i++) {
        if (g_svc[i].state == SUP_BACKOFF) {
            long d = g_svc[i].next_start_ms - now;
            if (d < 0) d = 0;
            if (best < 0 || d < best) best = d;
        }
    }
    if (g_shutting_down && !g_sent_sigkill) {
        long d = g_kill_deadline_ms - now;
        if (d < 0) d = 0;
        if (best < 0 || d < best) best = d;
    }

    if (best < 0)
        return -1;
    if (best > INT_MAX)
        best = INT_MAX;                /* poll's timeout is an int (ms)         */
    return (int)best;
}

/* ---------------------------------------------------------------------------
 * main — set up PID-1 duties, start services, run the event loop.
 * --------------------------------------------------------------------------- */
int main(int argc, char **argv)
{
    const char *cfg_path = "services.conf";
    if (argc >= 2)
        cfg_path = argv[1];            /* optional: path to the service file    */

    pid_t self = getpid();
    if (self == 1)
        logmsg("running as PID 1 (init)");
    else
        logmsg("running as pid %d (not init; using subreaper mode)", (int)self);

    /* --- Become the reaper for orphans -----------------------------------
     * PR_SET_CHILD_SUBREAPER makes THIS process the "nearest reaper": when any
     * of our descendants' parents die, the orphan reparents to us instead of to
     * PID 1. That lets us reap grandchildren even when we are NOT PID 1, so you
     * can test the supervisor as an ordinary user. When we really are PID 1 the
     * kernel already reparents all orphans to us, so this is a harmless no-op.
     * prctl(2): rax has the syscall, rdi=PR_SET_CHILD_SUBREAPER(=36), rsi=1. */
    if (prctl(PR_SET_CHILD_SUBREAPER, 1, 0, 0, 0) != 0)
        logmsg("warning: PR_SET_CHILD_SUBREAPER failed: %s", strerror(errno));

    /* --- Block the signals we will read via signalfd ---------------------
     * We must block them BEFORE creating the signalfd (and before forking any
     * child), otherwise a signal delivered in the gap would take the default
     * action. sigprocmask(SIG_BLOCK) adds our set to the process's blocked mask;
     * blocked signals stay pending and are delivered through the fd instead. */
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);          /* a child changed state (died)          */
    sigaddset(&mask, SIGTERM);          /* polite shutdown request               */
    sigaddset(&mask, SIGINT);           /* Ctrl-C when run interactively         */
    if (sigprocmask(SIG_BLOCK, &mask, 0) != 0) {
        logmsg("fatal: sigprocmask failed: %s", strerror(errno));
        return 1;
    }

    /* --- Create the signalfd ---------------------------------------------
     * signalfd(2): -1 means "new fd"; `mask` is the set of signals to report;
     * SFD_CLOEXEC closes it across exec (children must not inherit it);
     * SFD_NONBLOCK makes read() return EAGAIN instead of blocking, so we can
     * drain all queued records in a loop without stalling. Each readable record
     * is a `struct signalfd_siginfo`; ssi_signo is the signal number. */
    int sfd = signalfd(-1, &mask, SFD_CLOEXEC | SFD_NONBLOCK);
    if (sfd < 0) {
        logmsg("fatal: signalfd failed: %s", strerror(errno));
        return 1;
    }

    /* --- Parse the config ------------------------------------------------- */
    const char *err = 0;
    g_nsvc = sup_config_parse(cfg_path, g_svc, &err);
    if (g_nsvc < 0) {
        logmsg("fatal: config '%s': %s", cfg_path, err ? err : "parse error");
        close(sfd);
        return 1;
    }
    if (g_nsvc == 0) {
        logmsg("fatal: no services defined in '%s'", cfg_path);
        close(sfd);
        return 1;
    }
    logmsg("parsed %d service(s) from %s", g_nsvc, cfg_path);

    /* --- Order the services by dependency --------------------------------
     * Build a dense adjacency matrix `edges[i*n + j] = 1` meaning "i depends on
     * j" (from each service's resolved after_idx[]), then topologically sort it
     * with order.c's sup_toposort. A cycle (A after B, B after A) is fatal. */
    static unsigned char edges[SUP_MAX_SERVICES * SUP_MAX_SERVICES];
    memset(edges, 0, sizeof edges);
    for (int i = 0; i < g_nsvc; i++) {
        for (int d = 0; d < g_svc[i].n_after; d++) {
            int j = g_svc[i].after_idx[d];
            edges[(size_t)i * (size_t)g_nsvc + (size_t)j] = 1;
        }
    }
    int placed = sup_toposort(g_nsvc, edges, g_order);
    if (placed < g_nsvc) {
        logmsg("fatal: dependency cycle in config (ordered %d of %d)", placed, g_nsvc);
        close(sfd);
        return 1;
    }

    /* --- Start every service in dependency order -------------------------
     * NOTE: this starts them in order but does not wait for one to be "ready"
     * before starting the next; `after` gates ORDER, not readiness. Real init
     * systems add a readiness protocol (sd_notify, socket activation). */
    for (int k = 0; k < g_nsvc; k++)
        start_service(g_order[k]);

    /* --- The event loop --------------------------------------------------
     * One poll() on the signalfd. We wake on (a) a signal record, or (b) a
     * timeout we computed for the next scheduled action (a backoff restart or
     * the shutdown SIGKILL escalation). This single loop is the whole runtime. */
    struct pollfd pfd;
    pfd.fd = sfd;
    pfd.events = POLLIN;               /* wake when the signalfd has a record   */

    for (;;) {
        long now = now_ms();

        /* (1) Fire any backoff restarts that are due. */
        if (!g_shutting_down)
            start_due_services(now);

        /* (2) Shutdown bookkeeping. */
        if (g_shutting_down) {
            if (all_stopped()) {
                logmsg("all services stopped; init exiting cleanly");
                break;                 /* the ONLY normal exit from the loop    */
            }
            if (!g_sent_sigkill && now >= g_kill_deadline_ms) {
                /* Grace expired: some child ignored SIGTERM. Escalate to
                 * SIGKILL, which cannot be caught or ignored. */
                logmsg("grace period elapsed; sending SIGKILL to survivors");
                for (int i = 0; i < g_nsvc; i++)
                    if (g_svc[i].state == SUP_RUNNING && g_svc[i].pid > 0)
                        kill(-g_svc[i].pid, SIGKILL);
                g_sent_sigkill = 1;
            }
        }

        /* (3) Wait for the next event. */
        int timeout = compute_timeout(now);
        int r = poll(&pfd, 1, timeout);
        if (r < 0) {
            if (errno == EINTR)
                continue;              /* a non-blocked signal interrupted us   */
            logmsg("fatal: poll failed: %s", strerror(errno));
            break;
        }
        if (r == 0)
            continue;                  /* timeout: loop to handle timed events  */

        /* (4) The signalfd is readable. Drain EVERY queued record — recall that
         * signals coalesce, so we cannot assume one record per real signal. */
        if (pfd.revents & POLLIN) {
            for (;;) {
                struct signalfd_siginfo si;
                ssize_t n = read(sfd, &si, sizeof si);
                if (n < 0) {
                    if (errno == EAGAIN)
                        break;         /* no more records queued right now      */
                    if (errno == EINTR)
                        continue;
                    logmsg("read(signalfd) failed: %s", strerror(errno));
                    break;
                }
                if (n != (ssize_t)sizeof si)
                    break;             /* short read: shouldn't happen, be safe */

                switch (si.ssi_signo) {
                case SIGCHLD:
                    /* One or more children changed state. Reap them ALL. */
                    reap_children();
                    break;
                case SIGTERM:
                case SIGINT:
                    begin_shutdown((int)si.ssi_signo);
                    break;
                default:
                    /* We only registered three signals; anything else is a
                     * surprise we simply log. */
                    logmsg("ignoring unexpected signal %u", si.ssi_signo);
                    break;
                }
            }
        }
    }

    close(sfd);
    return 0;
}
