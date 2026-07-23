/* ===========================================================================
 * container.c — the PARENT half of `minic`.
 * ===========================================================================
 *
 * Responsibilities that ONLY the parent can perform:
 *
 *   1. clone(2) a child into a fresh set of namespaces.
 *   2. Write the child's UID/GID maps under /proc/<pid>/ — the kernel forbids a
 *      process from writing its OWN map in most cases, so the parent must do it.
 *   3. Create a cgroup and move the child into it (writing to cgroup.procs from
 *      outside the container, where we still have the host's cgroup privileges).
 *   4. Wait for the container's PID 1 to exit, then clean the cgroup up.
 *
 * WHY clone() AND NOT unshare()/fork()?
 *   unshare(CLONE_NEWPID) does not move the *caller* into the new PID namespace;
 *   only its future children land there. You would still need a fork afterwards.
 *   clone() with CLONE_NEWPID makes the child itself PID 1 in one step, which is
 *   what we want: the shell should BE init of its namespace.
 *
 * WHY IS CLONE_NEWUSER LISTED FIRST / WHY DOES ORDER MATTER?
 *   For a *rootless* container the user namespace is the key that unlocks the
 *   rest. An unprivileged process may not create a mount/PID/net/UTS namespace
 *   on its own. But when CLONE_NEWUSER is combined in the SAME clone() call, the
 *   kernel creates the new user namespace FIRST and then creates the other
 *   namespaces as owned by it — and inside a user namespace you hold a full set
 *   of capabilities. So the single combined clone() is what makes an ordinary
 *   user able to build the whole box. (Run as real root, this still works; the
 *   mapping just maps container-root to host-root.)
 * ===========================================================================
 */
#define _GNU_SOURCE          /* clone(), CLONE_NEW* live behind _GNU_SOURCE     */
#include "container.h"
#include "util.h"

#include <errno.h>
#include <sched.h>           /* clone, CLONE_NEW* flags                         */
#include <signal.h>          /* SIGCHLD                                         */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>        /* mmap for the child stack                        */
#include <sys/wait.h>        /* waitpid, W* status macros                       */
#include <unistd.h>          /* geteuid, getegid, close, getpid                 */

/* The child needs a stack because clone() (unlike fork) does not reuse the
 * caller's. 1 MiB is generous for our tiny child; it is lazily backed by the
 * kernel (only touched pages get physical frames) thanks to MAP_ANONYMOUS. */
#define CHILD_STACK_SIZE (1024 * 1024)

/* ---------------------------------------------------------------------------
 * write_maps — establish the identity mapping for the child's user namespace.
 *
 * A user namespace starts with an EMPTY uid/gid map: until we write one, the
 * child's every id shows up as the "overflow" id (65534/nobody) and it cannot
 * do id-checked operations. We install the simplest rootless mapping:
 *
 *     container uid 0  ->  our real euid  (range length 1)
 *     container gid 0  ->  our real egid  (range length 1)
 *
 * so that "root inside the box" is really just us outside it. Mapping a wider
 * range needs entries in /etc/subuid + the setuid helper newuidmap(1); see the
 * README's "Going further".
 *
 * THE setgroups GOTCHA: since Linux 3.19, before an unprivileged process may
 * write gid_map it must first write "deny" to /proc/<pid>/setgroups. This closes
 * a privilege-escalation hole where a mapped process could drop a group to gain
 * access. We therefore write setgroups BEFORE gid_map, always.
 * --------------------------------------------------------------------------- */
static void write_maps(pid_t child)
{
    char path[64];
    char line[64];

    /* /proc/<pid>/setgroups := "deny"  (required before gid_map on modern
     * kernels; on pre-3.19 kernels the file does not exist -> ENOENT, benign). */
    snprintf(path, sizeof path, "/proc/%d/setgroups", (int)child);
    if (write_file(path, "deny") == -1 && errno != ENOENT)
        warn("write setgroups=deny");

    /* /proc/<pid>/uid_map := "0 <euid> 1"  — one line: inside outside length. */
    snprintf(path, sizeof path, "/proc/%d/uid_map", (int)child);
    snprintf(line, sizeof line, "0 %d 1", (int)geteuid());
    if (write_file(path, line) == -1)
        die("write uid_map");            /* fatal: without a map nothing works  */

    /* /proc/<pid>/gid_map := "0 <egid> 1". */
    snprintf(path, sizeof path, "/proc/%d/gid_map", (int)child);
    snprintf(line, sizeof line, "0 %d 1", (int)getegid());
    if (write_file(path, line) == -1)
        die("write gid_map");
}

/* ---------------------------------------------------------------------------
 * usage / arg parsing
 * --------------------------------------------------------------------------- */
static void usage(const char *me)
{
    fprintf(stderr,
        "usage: %s [options] [-- COMMAND [ARGS...]]\n"
        "  --rootfs DIR     root filesystem to pivot into (default ./rootfs)\n"
        "  --hostname NAME  UTS hostname inside the container (default 'container')\n"
        "  --mem BYTES      memory.max cgroup limit (e.g. 67108864 for 64 MiB)\n"
        "  --cpu PERCENT    cpu.max as %% of one CPU (e.g. 50). period=100000us\n"
        "  -h, --help       this help\n"
        "If no COMMAND is given, runs /bin/sh.\n", me);
}

int main(int argc, char **argv)
{
    /* Defaults chosen so `sudo ./minic` demonstrates every feature, while a
     * rootless run still starts a shell (the cgroup step just warns). */
    static char *default_cmd[] = { "/bin/sh", NULL };
    struct container_cfg cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.rootfs     = "./rootfs";
    cfg.hostname   = "container";
    cfg.argv       = default_cmd;
    cfg.mem_max    = 64L * 1024 * 1024;  /* 64 MiB memory.max                   */
    cfg.cpu_quota  = 50000;              /* 50 ms ...                           */
    cfg.cpu_period = 100000;             /* ... per 100 ms period = 50% of 1 CPU */

    /* Hand-rolled parser: small and dependency-free. Everything after "--" is
     * the command line to run inside the container. */
    int i = 1;
    for (; i < argc; i++) {
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        else if (strcmp(argv[i], "--rootfs") == 0 && i + 1 < argc)
            cfg.rootfs = argv[++i];
        else if (strcmp(argv[i], "--hostname") == 0 && i + 1 < argc)
            cfg.hostname = argv[++i];
        else if (strcmp(argv[i], "--mem") == 0 && i + 1 < argc)
            cfg.mem_max = strtol(argv[++i], NULL, 10);
        else if (strcmp(argv[i], "--cpu") == 0 && i + 1 < argc)
            cfg.cpu_quota = strtol(argv[++i], NULL, 10) * cfg.cpu_period / 100;
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage(argv[0]); return 0;
        } else {
            fprintf(stderr, "%s: unknown option '%s'\n", argv[0], argv[i]);
            usage(argv[0]); return 2;
        }
    }
    if (i < argc)                        /* a COMMAND followed "--"             */
        cfg.argv = &argv[i];

    /* The readiness pipe MUST exist before clone() so the child inherits it. */
    if (pipe(cfg.sync_pipe) == -1)
        die("pipe");

    /* Allocate the child's stack. MAP_STACK is a hint; MAP_ANONYMOUS gives us
     * zero-filled, demand-paged memory not backed by any file. clone() wants the
     * HIGHEST address of the region because the x86-64 stack grows downward. */
    char *stack = mmap(NULL, CHILD_STACK_SIZE,
                       PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
    if (stack == MAP_FAILED)
        die("mmap child stack");
    char *stack_top = stack + CHILD_STACK_SIZE;

    /* THE clone() CALL. Flag by flag:
     *   CLONE_NEWUSER — new user namespace (the rootless enabler; see top note).
     *   CLONE_NEWNS   — new mount namespace, so our pivot_root/mounts are private.
     *   CLONE_NEWPID  — new PID namespace; the child becomes PID 1.
     *   CLONE_NEWNET  — new (empty) network namespace: only loopback, no host NICs.
     *   CLONE_NEWUTS  — new UTS namespace, so sethostname() is isolated.
     *   SIGCHLD       — deliver SIGCHLD when the child dies, so waitpid() works
     *                   exactly as it does for a normal fork()ed child.
     * On success the PARENT gets the child's (host-side) pid; the CHILD begins
     * executing child_main(&cfg) on the stack we allocated. */
    int flags = CLONE_NEWUSER | CLONE_NEWNS | CLONE_NEWPID |
                CLONE_NEWNET  | CLONE_NEWUTS | SIGCHLD;
    pid_t child = clone(child_main, stack_top, flags, &cfg);
    if (child == -1)
        die("clone");                    /* EPERM here usually = userns disabled */

    /* --- Parent post-clone setup, in the order the child depends on it. --- */

    /* (1) UID/GID maps first: the child is blocked on the pipe and needs a valid
     *     mapping before it tries anything id-checked (mounts, sethostname). */
    write_maps(child);

    /* (2) cgroup: create the group, set memory.max/cpu.max, and move the child
     *     into it via cgroup.procs — all BEFORE the child runs, so the limits
     *     apply to everything it does. Best-effort: rootless without cgroup
     *     delegation cannot do this, so we only warn and carry on. */
    if (cgroup_create(&cfg, child) == -1)
        warn("cgroup setup skipped (need root or cgroup2 delegation)");

    /* (3) Release the child: closing our write end makes its read() return EOF. */
    close(cfg.sync_pipe[1]);
    close(cfg.sync_pipe[0]);             /* parent never reads; drop it too      */

    /* (4) Reap PID 1 of the container. When it exits, the PID namespace and the
     *     empty network namespace are torn down by the kernel automatically. */
    int status;
    if (waitpid(child, &status, 0) == -1)
        die("waitpid");

    /* Now that no process remains in the cgroup, its directory can be rmdir'd. */
    cgroup_destroy(&cfg);

    /* Mirror the child's exit status to our own so scripts see it in $?. */
    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) {
        fprintf(stderr, "minic: container killed by signal %d\n", WTERMSIG(status));
        return 128 + WTERMSIG(status);
    }
    return 0;
}
