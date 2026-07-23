/* ===========================================================================
 * service.h — the in-memory model of one supervised service, plus the tunables.
 * ===========================================================================
 *
 * A "service" is a program the supervisor keeps alive: its command line, its
 * restart policy, the services it must start after, and its live runtime state
 * (child pid, how many times it has crashed, when it is next allowed to
 * restart). Everything here is plain data — the behaviour lives in config.c
 * (parsing) and supervisor.c (the event loop).
 *
 * NO DYNAMIC ALLOCATION. Every service lives in a fixed-size array and every
 * string in a fixed-size buffer. PID 1 is the one process that must never die;
 * an out-of-memory failure in malloc() at the wrong moment could wedge the whole
 * machine, so we simply never call it. The cost is hard caps (below); the win is
 * that the supervisor's memory footprint is known at compile time and no error
 * path can leak or double-free.
 * ===========================================================================
 */
#ifndef SUP_SERVICE_H
#define SUP_SERVICE_H

#include <sys/types.h>   /* pid_t — a signed integer wide enough for any PID    */

/* --- Hard limits (see "NO DYNAMIC ALLOCATION" above) ---------------------- */
#define SUP_MAX_SERVICES  64    /* must equal SUP_MAX_NODES in order.h         */
#define SUP_MAX_ARGS      31    /* argv tokens per service (+1 for the NULL)   */
#define SUP_MAX_NAME      64    /* bytes for a service name (incl. NUL)        */
#define SUP_MAX_DEPS      16    /* `after` prerequisites per service           */
#define SUP_MAX_EXEC     512    /* bytes backing one service's argv strings    */
#define SUP_MAX_LINE     1024   /* longest config line we accept               */

/* --- Backoff tunables (fed to sup_backoff_delay_ms in order.c) ------------ */
#define SUP_BACKOFF_BASE_MS   100u    /* first restart waits ~100ms            */
#define SUP_BACKOFF_CAP_MS  30000u    /* ...doubling up to a 30s ceiling       */

/* --- Shutdown tunable ----------------------------------------------------- */
#define SUP_SHUTDOWN_GRACE_MS 5000    /* SIGTERM grace before we escalate SIGKILL */

/* What to do when a service's process exits. */
enum sup_restart {
    SUP_RESTART_NO = 0,     /* one-shot: run once, never restart               */
    SUP_RESTART_ON_FAILURE, /* restart only on non-zero exit or fatal signal   */
    SUP_RESTART_ALWAYS      /* restart on any exit, success or failure         */
};

/* Where a service is in its lifecycle right now. */
enum sup_state {
    SUP_STOPPED = 0,   /* not running and not scheduled to run                 */
    SUP_RUNNING,       /* we have a live child pid for it                      */
    SUP_BACKOFF        /* died recently; waiting until `next_start_ms` to retry */
};

/* One supervised service. Instances live only inside the supervisor's global
 * array and are never copied (the `argv` pointers below alias into this same
 * struct — copying it would leave them dangling). */
struct sup_service {
    /* ---- static config (filled by the parser) ---- */
    char  name[SUP_MAX_NAME];               /* unique service name             */

    /* argv[] is what execvp() consumes: a NULL-terminated vector of C strings.
     * The strings themselves live in `exec_buf`; argv[k] points INTO exec_buf,
     * which the parser split in place by overwriting spaces with '\0'. Invariant:
     * argv is valid only as long as this struct is alive and unmoved. */
    char *argv[SUP_MAX_ARGS + 1];
    char  exec_buf[SUP_MAX_EXEC];           /* backing storage for argv tokens */
    int   argc;                             /* number of non-NULL argv entries */

    enum sup_restart restart;               /* restart policy                  */

    /* Dependencies: names as written in the config, then resolved to indices
     * into the service array after the whole file is parsed. */
    char  after_names[SUP_MAX_DEPS][SUP_MAX_NAME];
    int   after_idx[SUP_MAX_DEPS];          /* resolved prerequisite indices   */
    int   n_after;

    /* ---- live runtime state (maintained by the event loop) ---- */
    enum sup_state state;
    pid_t   pid;                            /* valid iff state == SUP_RUNNING   */
    unsigned restart_count;                 /* feeds the backoff calculation    */
    long    started_ms;                     /* CLOCK_MONOTONIC when last spawned */
    long    next_start_ms;                  /* CLOCK_MONOTONIC deadline in BACKOFF */
    int     last_status;                    /* raw status from waitpid()        */
};

#endif /* SUP_SERVICE_H */
