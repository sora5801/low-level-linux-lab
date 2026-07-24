/* ===========================================================================
 * cgroup.c — resource limits via the cgroup v2 unified hierarchy.
 * ===========================================================================
 *
 * cgroup v2 is "just a filesystem": create a directory under /sys/fs/cgroup/,
 * write limits into its controller files, and write a PID into `cgroup.procs` to
 * move that process (and all its future descendants) under the limit. There is
 * no cgroup syscall — the VFS *is* the API. We drive three controllers:
 *
 *     memory.max   hard cap on the group's memory; exceeding it invokes the OOM
 *                  killer scoped to the GROUP (the host stays healthy).
 *     cpu.max      "<quota> <period>" microseconds: the group may run <quota> us
 *                  of CPU per <period> us. 50000/100000 = 50% of one core.
 *     pids.max     max number of tasks in the group — a fork-bomb guard. This is
 *                  the controller a shell-in-a-box most wants and the sibling
 *                  runtime omits; a container without it can `:(){ :|:& };:` the
 *                  host into the ground.
 *
 * THE CONTROLLER-ENABLE RULE. A child cgroup only exposes memory.max/cpu.max/
 * pids.max if its PARENT lists those controllers in `cgroup.subtree_control`. So
 * before creating our leaf we try to write "+memory +cpu +pids" to the root's
 * subtree_control. On a systemd host the root is usually delegated this way;
 * where it is not (or we are rootless with no delegation) the writes fail with
 * EACCES/EROFS/ENOENT and we degrade gracefully — the container still runs, just
 * uncapped. Silently pretending to limit is worse than a clear warning.
 * ===========================================================================
 */
#define _GNU_SOURCE
#include "engine.h"
#include "util.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>        /* mkdir                                          */
#include <unistd.h>          /* rmdir, getpid                                  */

#define CGROUP_ROOT "/sys/fs/cgroup"

/* Build "<CGROUP_ROOT>/<leaf>/<file>" (or ".../<leaf>" when file==NULL) into
 * buf. Centralized so every path is constructed and length-checked identically. */
static void cg_path(char *buf, size_t n, const char *leaf, const char *file)
{
    if (file)
        snprintf(buf, n, "%s/%s/%s", CGROUP_ROOT, leaf, file);
    else
        snprintf(buf, n, "%s/%s", CGROUP_ROOT, leaf);
}

int cgroup_create(struct engine_cfg *cfg, pid_t child)
{
    char path[512];
    char val[64];

    /* Unique leaf per launcher process so concurrent runs never collide. */
    char leaf[64];
    snprintf(leaf, sizeof leaf, "ceng-%d", (int)getpid());

    /* (1) Ask the root to expose the controllers to its children. Best-effort:
     *     if we lack permission the later limit writes fail too, handled below. */
    if (write_file(CGROUP_ROOT "/cgroup.subtree_control", "+memory +cpu +pids") == -1)
        warn("enable +memory +cpu +pids (may already be set or need delegation)");

    /* (2) Create the leaf directory. mkdir on cgroupfs is special: the kernel
     *     materializes the controller files inside it. EEXIST (a stale dir from
     *     a crash) is fine; anything else is a real failure. */
    cg_path(path, sizeof path, leaf, NULL);
    if (mkdir(path, 0755) == -1 && errno != EEXIST)
        return -1;                       /* no cgroup -> caller warns and skips  */

    /* Memo the path so cgroup_destroy() removes exactly this directory. */
    cg_path(cfg->cgroup_path, sizeof cfg->cgroup_path, leaf, NULL);

    /* (3) memory.max := bytes. */
    if (cfg->mem_max >= 0) {
        cg_path(path, sizeof path, leaf, "memory.max");
        snprintf(val, sizeof val, "%ld", cfg->mem_max);
        if (write_file(path, val) == -1)
            warn("set memory.max");      /* controller maybe not delegated       */
    }

    /* (4) cpu.max := "<quota> <period>". */
    if (cfg->cpu_quota >= 0) {
        cg_path(path, sizeof path, leaf, "cpu.max");
        snprintf(val, sizeof val, "%ld %ld", cfg->cpu_quota, cfg->cpu_period);
        if (write_file(path, val) == -1)
            warn("set cpu.max");
    }

    /* (5) pids.max := count. Caps the number of tasks; a fork bomb hits EAGAIN
     *     on clone once the group is full, instead of exhausting the host. */
    if (cfg->pids_max >= 0) {
        cg_path(path, sizeof path, leaf, "pids.max");
        snprintf(val, sizeof val, "%ld", cfg->pids_max);
        if (write_file(path, val) == -1)
            warn("set pids.max");
    }

    /* (6) Attach the child. Writing its PID to cgroup.procs migrates it and,
     *     because it is PID 1 of its namespace, every process it later spawns.
     *     This is the one write that MUST succeed for the limits to bind, so its
     *     failure is what makes the whole step "fail" and the caller warn. */
    cg_path(path, sizeof path, leaf, "cgroup.procs");
    snprintf(val, sizeof val, "%d", (int)child);
    if (write_file(path, val) == -1)
        return -1;

    return 0;
}

void cgroup_destroy(struct engine_cfg *cfg)
{
    if (cfg->cgroup_path[0] == '\0')
        return;                          /* never created a group                */

    /* rmdir(2) on a cgroup succeeds only when the group is EMPTY (no procs, no
     * sub-cgroups). We call this AFTER waitpid() reaped PID 1, so it is drained;
     * a stray lingering descendant yields EBUSY and we just leave the idle dir. */
    if (rmdir(cfg->cgroup_path) == -1)
        warn("rmdir cgroup (non-fatal)");
}
