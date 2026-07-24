/* ===========================================================================
 * overlay.c — the container's copy-on-write root filesystem, via overlayfs.
 * ===========================================================================
 *
 * A container image is READ-ONLY and SHARED: ten containers off the same image
 * must not see each other's writes, and none may corrupt the image. overlayfs
 * gives us exactly that, for free, in the kernel. It stacks directories:
 *
 *     lowerdir  = the image rootfs        (read-only, shared by all containers)
 *     upperdir  = a per-container dir      (every write lands here)
 *     workdir   = overlayfs private scratch(same fs as upper; kernel-internal)
 *     merged    = the union mountpoint     (what the container sees as "/")
 *
 * A read falls through upper -> lower. A write is COPIED UP: the file is copied
 * from lower into upper on first modification, then edited there. Deleting a
 * lower file leaves a "whiteout" in upper. The image is never touched. This is
 * precisely how Docker/containerd layer images (each image layer is one more
 * lowerdir); we model the single-image case.
 *
 * KERNEL REQUIREMENT (honest): mounting overlayfs from inside a NON-INITIAL user
 * namespace needs Linux >= 5.11. This engine ALWAYS creates CLONE_NEWUSER, so
 * the child mounts overlay in a new userns — on older kernels that mount returns
 * EPERM. Run on a >= 5.11 kernel (any current distro/WSL2). The README says so.
 *
 * SPLIT ACROSS THE clone() BOUNDARY:
 *   overlay_prepare()  runs in the PARENT: it only makes the host directories
 *                      (upper/work/merged). Cheap, and keeps the child's
 *                      in-namespace work to the single privileged mount(2).
 *   overlay_mount()    runs in the CHILD, inside the new mount+user namespace,
 *                      where it holds CAP_SYS_ADMIN over its own mounts.
 * ===========================================================================
 */
#define _GNU_SOURCE
#include "engine.h"
#include "util.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/mount.h>       /* mount, MS_*                                    */
#include <sys/stat.h>        /* mkdir                                          */
#include <unistd.h>

/* Recursive mkdir (like `mkdir -p`). overlayfs needs upper/ and work/ to exist
 * and be EMPTY; merged/ is just a mountpoint. We walk the path component by
 * component, creating each and tolerating EEXIST. */
static int mkdir_p(const char *path)
{
    char tmp[PATH_MAX];
    size_t len = strlen(path);
    if (len == 0 || len >= sizeof tmp)
        return -1;
    memcpy(tmp, path, len + 1);

    /* Strip any trailing slash so the final component is created too. */
    if (tmp[len - 1] == '/')
        tmp[len - 1] = '\0';

    /* For each interior '/', temporarily terminate the string there and mkdir
     * the prefix. p starts at tmp+1 so we never try to mkdir "" (the root). */
    for (char *p = tmp + 1; *p; p++) {
        if (*p != '/')
            continue;
        *p = '\0';                                   /* cut here                 */
        if (mkdir(tmp, 0755) == -1 && errno != EEXIST)
            return -1;
        *p = '/';                                    /* restore and continue     */
    }
    if (mkdir(tmp, 0755) == -1 && errno != EEXIST)   /* the full path            */
        return -1;
    return 0;
}

int overlay_prepare(struct engine_cfg *cfg)
{
    /* Derive the three working paths under state_dir. upper holds the container's
     * writes; work is overlayfs's private scratch (MUST be on the same
     * filesystem as upper, so we place them as siblings); merged is the
     * mountpoint we will pivot_root into. */
    snprintf(cfg->upper,  sizeof cfg->upper,  "%s/upper",  cfg->state_dir);
    snprintf(cfg->work,   sizeof cfg->work,   "%s/work",   cfg->state_dir);
    snprintf(cfg->merged, sizeof cfg->merged, "%s/merged", cfg->state_dir);

    if (mkdir_p(cfg->state_dir) == -1) return -1;
    if (mkdir_p(cfg->upper)     == -1) return -1;
    if (mkdir_p(cfg->work)      == -1) return -1;
    if (mkdir_p(cfg->merged)    == -1) return -1;
    return 0;
}

int overlay_mount(const struct engine_cfg *cfg)
{
    /* Build the overlayfs options string the kernel parses:
     *   "lowerdir=<image>,upperdir=<upper>,workdir=<work>"
     * There is no dedicated overlay syscall — it is an ordinary filesystem type
     * passed to mount(2), with everything in the comma-separated 5th argument.
     * (Note: a ':' would separate MULTIPLE lower layers, deepest last; we use a
     * single image layer here.) */
    char opts[3 * PATH_MAX + 64];
    int n = snprintf(opts, sizeof opts,
                     "lowerdir=%s,upperdir=%s,workdir=%s",
                     cfg->image_dir, cfg->upper, cfg->work);
    if (n < 0 || (size_t)n >= sizeof opts)
        return -1;                       /* paths too long to encode safely      */

    /* mount(2): source="overlay" (cosmetic label), target=merged, type="overlay",
     * flags=0, data=opts. The kernel builds the union and mounts it at `merged`.
     * Needs CAP_SYS_ADMIN in this user namespace (we have it) and kernel >= 5.11
     * for the unprivileged-userns case. Common errors:
     *   EPERM  -> kernel < 5.11 (no unprivileged overlay) — see README.
     *   EINVAL -> upper/work not on the same fs, or work not empty. */
    if (mount("overlay", cfg->merged, "overlay", 0, opts) == -1)
        return -1;

    return 0;
}

void overlay_cleanup(const struct engine_cfg *cfg)
{
    /* The overlay mount lives in the CHILD's mount namespace, which the kernel
     * tears down automatically when PID 1 of that namespace exits — so there is
     * nothing to umount from the parent here. We deliberately LEAVE upper/ in
     * place: it is the container's writable layer, and being able to inspect it
     * after the run ("what did the container change?") is part of the lesson.
     * We only note where it is. */
    fprintf(stderr, "ceng: writable layer preserved at %s\n", cfg->upper);
}
