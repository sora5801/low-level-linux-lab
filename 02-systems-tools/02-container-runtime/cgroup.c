/* ===========================================================================
 * cgroup.c — resource limits via the cgroup v2 unified hierarchy.
 * ===========================================================================
 *
 * cgroup v2 is "just a filesystem": you create a directory under
 * /sys/fs/cgroup/, write limits into its controller files, and write a PID into
 * its `cgroup.procs` to move that process (and its future descendants) under the
 * limit. No syscalls of its own — the VFS *is* the API. We use it for two knobs:
 *
 *     memory.max   hard cap on the group's memory; hitting it triggers the OOM
 *                  killer scoped to the group (the host stays healthy).
 *     cpu.max      "<quota> <period>" in microseconds: the group may run for
 *                  <quota> us of CPU time out of every <period> us. 50000/100000
 *                  = 50% of one core.
 *
 * THE CONTROLLER-ENABLE RULE. A child cgroup only exposes memory.max / cpu.max
 * if its PARENT lists those controllers in `cgroup.subtree_control`. So before
 * creating our leaf we try to write "+memory +cpu" to the root's
 * subtree_control. On a systemd host the root is usually already delegated this
 * way; where it is not (or where we are rootless with no delegation) the writes
 * fail with EACCES/EROFS/ENOENT and we degrade gracefully — the container still
 * runs, just uncapped. That honesty matters: silently pretending to limit is
 * worse than a clear warning.
 * ===========================================================================
 */
#define _GNU_SOURCE
#include "container.h"
#include "util.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>        /* mkdir                                          */
#include <unistd.h>          /* rmdir, getpid                                  */

#define CGROUP_ROOT "/sys/fs/cgroup"

/* Build "<CGROUP_ROOT>/<leaf>/<file>" into buf. Centralized so every path is
 * constructed the same way and length-checked by snprintf's truncation. */
static void cg_path(char *buf, size_t n, const char *leaf, const char *file)
{
    if (file)
        snprintf(buf, n, "%s/%s/%s", CGROUP_ROOT, leaf, file);
    else
        snprintf(buf, n, "%s/%s", CGROUP_ROOT, leaf);
}

int cgroup_create(struct container_cfg *cfg, pid_t child)
{
    char path[512];
    char val[64];

    /* Unique leaf name per launcher process so concurrent runs don't collide. */
    char leaf[64];
    snprintf(leaf, sizeof leaf, "minic-%d", (int)getpid());

    /* (1) Ask the root to expose the memory + cpu controllers to its children.
     *     Best-effort: if we lack permission the later limit writes will fail
     *     too, and we handle that below. */
    if (write_file(CGROUP_ROOT "/cgroup.subtree_control", "+memory +cpu") == -1)
        warn("enable +memory +cpu (may already be set or need delegation)");

    /* (2) Create the leaf cgroup directory. mkdir on cgroupfs is special: the
     *     kernel materializes all the controller files inside it. EEXIST is fine
     *     (a stale dir from a crash); anything else is a real failure. */
    cg_path(path, sizeof path, leaf, NULL);
    if (mkdir(path, 0755) == -1 && errno != EEXIST)
        return -1;                       /* no cgroup -> caller warns and skips  */

    /* Remember the path so cgroup_destroy() can remove exactly this directory. */
    cg_path(cfg->cgroup_path, sizeof cfg->cgroup_path, leaf, NULL);

    /* (3) memory.max := bytes. Writing the ASCII integer sets the hard limit. */
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

    /* (5) Attach the child. Writing its PID to cgroup.procs migrates it (and,
     *     because it is PID 1 of its namespace, every process it later spawns)
     *     into the group, so the limits bind before the shell does anything.
     *     This is the one write that must succeed for the limits to mean
     *     something, so its failure is what makes the whole step "fail". */
    cg_path(path, sizeof path, leaf, "cgroup.procs");
    snprintf(val, sizeof val, "%d", (int)child);
    if (write_file(path, val) == -1)
        return -1;

    return 0;
}

void cgroup_destroy(struct container_cfg *cfg)
{
    /* Nothing to do if we never created a group. */
    if (cfg->cgroup_path[0] == '\0')
        return;

    /* rmdir(2) on a cgroup only succeeds when the group is EMPTY (no procs, no
     * sub-cgroups). We call this AFTER waitpid() reaped PID 1, so the group is
     * drained and the removal should succeed; if a stray descendant lingers the
     * kernel returns EBUSY and we just leave the (now-idle) directory behind. */
    if (rmdir(cfg->cgroup_path) == -1)
        warn("rmdir cgroup (non-fatal)");
}
