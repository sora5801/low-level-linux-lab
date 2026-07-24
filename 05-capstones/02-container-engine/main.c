/* ===========================================================================
 * main.c — the PARENT half of `ceng`, the container-engine capstone.
 * ===========================================================================
 *
 * Responsibilities only the parent can perform:
 *
 *   1. clone(2) a child into a fresh set of namespaces.
 *   2. Write the child's UID/GID maps under /proc/<pid>/ — the kernel forbids a
 *      process from writing its OWN map in the common case, so the parent must.
 *   3. Create a cgroup v2 leaf and migrate the child into it (from outside the
 *      container, where we still hold the host's cgroup privileges).
 *   4. Wire up networking: a veth pair whose peer we push into the child's
 *      netns, addressed and NAT'd, all reaching in by the child's pid.
 *   5. Release the child (close the sync pipe), wait for PID 1, then tear down
 *      the network and cgroup.
 *
 * WHY clone() AND NOT unshare()+fork()?
 *   unshare(CLONE_NEWPID) does not move the CALLER into the new PID namespace;
 *   only its future children land there. clone() with CLONE_NEWPID makes the
 *   child itself PID 1 in one step — the payload should BE init of its namespace.
 *
 * WHY IS CLONE_NEWUSER THE KEY / WHY DOES ORDER MATTER?
 *   For a rootless container the user namespace unlocks the rest: an
 *   unprivileged process may not create mount/PID/net/UTS/IPC namespaces alone,
 *   but combined with CLONE_NEWUSER in the SAME clone() the kernel creates the
 *   user namespace FIRST and the others as owned by it, where we hold a full
 *   capability set. (As real root this still works; the map just becomes 0->0.)
 *   Note: the OVERLAY mount and the veth/NAT plumbing DO need real host
 *   privilege, so the full-feature run wants root — see the README.
 * ===========================================================================
 */
#define _GNU_SOURCE          /* clone(), CLONE_NEW* live behind _GNU_SOURCE     */
#include "engine.h"
#include "util.h"

#include <errno.h>
#include <sched.h>           /* clone, CLONE_NEW* flags                         */
#include <signal.h>          /* SIGCHLD                                         */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>        /* mmap for the child stack                        */
#include <sys/wait.h>        /* waitpid, W* status macros                       */
#include <unistd.h>          /* geteuid, getegid, close                        */

/* clone() (unlike fork) does not reuse the caller's stack, so we hand the child
 * its own. 1 MiB is generous for our tiny child; MAP_ANONYMOUS makes it lazily
 * backed (only touched pages get physical frames). */
#define CHILD_STACK_SIZE (1024 * 1024)

/* Compose the clone(2) flag word from the namespaces we want. Writing it as an
 * explicit OR of independently-toggleable bits (rather than one constant) is the
 * exact shape asm/demo.c extracts and annotates — the "clone-flags composition"
 * logic. Every flag creates one kind of isolation:
 *   NEWUSER — the rootless enabler (see the header note).
 *   NEWNS   — private mount table (our overlay + pivot_root stay contained).
 *   NEWPID  — the child becomes PID 1; its children are invisible to the host.
 *   NEWNET  — empty network namespace we then populate with the veth.
 *   NEWUTS  — private hostname/domainname.
 *   NEWIPC  — private System V IPC / POSIX message queues.
 * SIGCHLD is OR'd in as the termination signal so waitpid() behaves as for a
 * normal fork()ed child. */
static int compose_clone_flags(void)
{
    int flags = 0;
    flags |= CLONE_NEWUSER;
    flags |= CLONE_NEWNS;
    flags |= CLONE_NEWPID;
    flags |= CLONE_NEWNET;
    flags |= CLONE_NEWUTS;
    flags |= CLONE_NEWIPC;
    return flags;
}

/* ---------------------------------------------------------------------------
 * write_maps — establish the identity mapping for the child's user namespace.
 *
 * A user namespace starts with an EMPTY uid/gid map: until we write one, every
 * id shows up as the overflow id (65534/nobody) and id-checked operations fail.
 * We install the simplest rootless mapping:
 *     container uid 0 -> our real euid   (length 1)
 *     container gid 0 -> our real egid   (length 1)
 * so "root inside the box" is really just us outside it. A wider range needs
 * /etc/subuid + the setuid helper newuidmap(1); see the README "Going further".
 *
 * THE setgroups GOTCHA: since Linux 3.19, before an unprivileged process may
 * write gid_map it must first write "deny" to /proc/<pid>/setgroups (closing a
 * privilege-escalation hole). So we write setgroups BEFORE gid_map, always.
 * --------------------------------------------------------------------------- */
static void write_maps(pid_t child)
{
    char path[64];
    char line[64];

    snprintf(path, sizeof path, "/proc/%d/setgroups", (int)child);
    if (write_file(path, "deny") == -1 && errno != ENOENT)
        warn("write setgroups=deny");    /* ENOENT on pre-3.19 kernels: benign   */

    snprintf(path, sizeof path, "/proc/%d/uid_map", (int)child);
    snprintf(line, sizeof line, "0 %d 1", (int)geteuid());
    if (write_file(path, line) == -1)
        die("write uid_map");            /* fatal: without a map nothing works   */

    snprintf(path, sizeof path, "/proc/%d/gid_map", (int)child);
    snprintf(line, sizeof line, "0 %d 1", (int)getegid());
    if (write_file(path, line) == -1)
        die("write gid_map");
}

static void usage(const char *me)
{
    fprintf(stderr,
        "usage: %s [options] [-- COMMAND [ARGS...]]\n"
        "  --image DIR      read-only image rootfs (overlay lowerdir; default ./image)\n"
        "  --state DIR      where upper/work/merged live (default ./state)\n"
        "  --hostname NAME  UTS hostname inside the container (default 'container')\n"
        "  --mem BYTES      memory.max cgroup limit (e.g. 67108864 = 64 MiB)\n"
        "  --cpu PERCENT    cpu.max as %% of one CPU (e.g. 50). period=100000us\n"
        "  --pids N         pids.max (fork-bomb guard; default 128)\n"
        "  --net            set up a veth pair + NAT (default; needs root)\n"
        "  --no-net         leave the netns with only loopback\n"
        "  -h, --help       this help\n"
        "If no COMMAND is given, runs /bin/sh.\n", me);
}

int main(int argc, char **argv)
{
    static char *default_cmd[] = { "/bin/sh", NULL };
    struct engine_cfg cfg;
    memset(&cfg, 0, sizeof cfg);

    /* Defaults chosen so `sudo ./ceng` demonstrates every feature. */
    cfg.image_dir  = "./image";
    cfg.state_dir  = "./state";
    cfg.hostname   = "container";
    cfg.argv       = default_cmd;
    cfg.mem_max    = 64L * 1024 * 1024;   /* 64 MiB memory.max                    */
    cfg.cpu_quota  = 50000;               /* 50 ms ...                            */
    cfg.cpu_period = 100000;              /* ... per 100 ms = 50% of one CPU      */
    cfg.pids_max   = 128;                 /* generous, but stops a fork bomb      */
    cfg.cap_keep   = caps_default_keep(); /* the Docker-like default cap set      */
    cfg.net_enable = 1;                   /* veth + NAT by default (needs root)   */
    cfg.net_subnet = "10.0.42.0/24";
    cfg.host_ip    = "10.0.42.1";
    cfg.cont_ip    = "10.0.42.2";

    /* Hand-rolled parser: small and dependency-free. Everything after "--" is
     * the command line to run inside the container. */
    int i = 1;
    for (; i < argc; i++) {
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        else if (strcmp(argv[i], "--image") == 0 && i + 1 < argc)
            cfg.image_dir = argv[++i];
        else if (strcmp(argv[i], "--state") == 0 && i + 1 < argc)
            cfg.state_dir = argv[++i];
        else if (strcmp(argv[i], "--hostname") == 0 && i + 1 < argc)
            cfg.hostname = argv[++i];
        else if (strcmp(argv[i], "--mem") == 0 && i + 1 < argc)
            cfg.mem_max = strtol(argv[++i], NULL, 10);
        else if (strcmp(argv[i], "--cpu") == 0 && i + 1 < argc)
            cfg.cpu_quota = strtol(argv[++i], NULL, 10) * cfg.cpu_period / 100;
        else if (strcmp(argv[i], "--pids") == 0 && i + 1 < argc)
            cfg.pids_max = strtol(argv[++i], NULL, 10);
        else if (strcmp(argv[i], "--net") == 0)
            cfg.net_enable = 1;
        else if (strcmp(argv[i], "--no-net") == 0)
            cfg.net_enable = 0;
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage(argv[0]); return 0;
        } else {
            fprintf(stderr, "%s: unknown option '%s'\n", argv[0], argv[i]);
            usage(argv[0]); return 2;
        }
    }
    if (i < argc)                         /* a COMMAND followed "--"              */
        cfg.argv = &argv[i];

    /* Build the writable overlay layout on the host BEFORE clone(), so the paths
     * are ready when the child mounts the overlay. Fatal on failure: no rootfs,
     * no container. */
    if (overlay_prepare(&cfg) == -1)
        die("overlay_prepare (create state dirs)");

    /* The readiness pipe MUST exist before clone() so the child inherits it. */
    if (pipe(cfg.sync_pipe) == -1)
        die("pipe");

    /* Allocate the child's stack. clone() wants the HIGHEST address of the
     * region because the x86-64 stack grows downward. */
    char *stack = mmap(NULL, CHILD_STACK_SIZE,
                       PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
    if (stack == MAP_FAILED)
        die("mmap child stack");
    char *stack_top = stack + CHILD_STACK_SIZE;

    /* THE clone() CALL. flags = the composed namespace set + SIGCHLD. On success
     * the PARENT gets the child's (host-side) pid; the CHILD begins executing
     * child_main(&cfg) on the stack we allocated. */
    cfg.clone_flags = compose_clone_flags();
    pid_t child = clone(child_main, stack_top, cfg.clone_flags | SIGCHLD, &cfg);
    if (child == -1)
        die("clone");                     /* EPERM here usually = userns disabled */

    /* --- Parent post-clone setup, in the order the child depends on it. --- */

    /* (1) UID/GID maps first: the child is blocked on the pipe and needs a valid
     *     mapping before anything id-checked (mounts, sethostname). */
    write_maps(child);

    /* (2) cgroup: create the leaf, set limits, migrate the child — all BEFORE it
     *     runs, so the limits bind to everything it does. Best-effort. */
    if (cgroup_create(&cfg, child) == -1)
        warn("cgroup setup skipped (need root or cgroup2 delegation)");

    /* (3) network: veth pair + NAT, reaching into the child's netns by pid.
     *     Best-effort: without root this warns and the box runs with only lo. */
    if (network_setup(&cfg, child) == -1)
        warn("network setup skipped (need root; container has only loopback)");

    /* (4) Release the child: closing our write end makes its read() return EOF.
     *     From here the child does its in-namespace setup and execs the payload. */
    close(cfg.sync_pipe[1]);
    close(cfg.sync_pipe[0]);              /* parent never reads; drop it too       */

    /* (5) Reap PID 1 of the container. When it exits, the kernel tears down the
     *     PID/net/IPC/UTS/mount namespaces automatically. */
    int status;
    if (waitpid(child, &status, 0) == -1)
        die("waitpid");

    /* (6) Teardown, in reverse: remove our NAT rules + host veth, then the now-
     *     empty cgroup, then note where the writable layer was preserved. */
    network_cleanup(&cfg);
    cgroup_destroy(&cfg);
    overlay_cleanup(&cfg);

    /* Mirror the child's exit status to our own so scripts see it in $?. */
    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) {
        fprintf(stderr, "ceng: container killed by signal %d\n", WTERMSIG(status));
        return 128 + WTERMSIG(status);
    }
    return 0;
}
