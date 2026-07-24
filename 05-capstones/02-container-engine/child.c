/* ===========================================================================
 * child.c — everything that runs INSIDE the new namespaces, as PID 1.
 * ===========================================================================
 *
 * clone() drops us here. At this instant:
 *   - We are PID 1 in a brand-new PID namespace.
 *   - We are (about to be) uid 0 in a new user namespace, but the parent has not
 *     yet written our uid_map, so we FIRST block on the sync pipe.
 *   - Our network namespace exists but the parent is still wiring the veth into
 *     it — another reason to wait for the pipe before we touch anything.
 *   - Once mapped, we hold a full capability set SCOPED TO THIS user namespace:
 *     power over our own namespaces, not over the host.
 *
 * The setup order is not arbitrary; each step depends on the last:
 *
 *   wait for maps+cgroup+net  ->  set ids  ->  set hostname
 *     ->  build the mount tree (overlay + pivot_root + /proc)
 *     ->  DROP capabilities to the keep set  ->  install seccomp  ->  execve
 *
 * Capability trimming and seccomp come LAST, immediately before execve, because
 * the overlay mount and pivot_root genuinely need CAP_SYS_ADMIN. Lock the door
 * on the way out, not on the way in.
 *
 * WHAT SHOULD REALLY RUN AS PID 1?  We execve the user's command directly. In a
 * production container the ENTRYPOINT should be a correct init that reaps
 * zombies, forwards signals, and handles shutdown — exactly the program built in
 * ../../02-systems-tools/03-init-supervisor. Point --exec at it (or bind it into
 * the image) to get proper PID-1 semantics; this engine deliberately leaves that
 * choice to the payload.
 * ===========================================================================
 */
#define _GNU_SOURCE
#include "engine.h"
#include "util.h"

#include <errno.h>
#include <limits.h>          /* PATH_MAX                                       */
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>       /* mount, umount2, MS_*, MNT_DETACH               */
#include <sys/stat.h>        /* mkdir                                          */
#include <sys/syscall.h>     /* syscall, SYS_pivot_root                        */
#include <unistd.h>

/* ---------------------------------------------------------------------------
 * setup_mounts — give the container its overlay root filesystem and /proc.
 *
 * This is the CLONE_NEWNS payload. Because we are in a private mount namespace,
 * nothing here is visible on the host — but only AFTER we mark the tree private,
 * because a fresh mount namespace inherits the host's mounts and, on most
 * distros, their "shared" propagation. Skip step (1) and our mounts would
 * propagate back to the host and pivot_root would refuse to run (EINVAL).
 * --------------------------------------------------------------------------- */
static void setup_mounts(const struct engine_cfg *cfg)
{
    /* (1) Recursively make every mount private. Source/target/type are ignored;
     *     MS_REC|MS_PRIVATE just clears shared/slave propagation under "/". */
    if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL) == -1)
        die("mount --make-rprivate /");

    /* (2) Assemble the copy-on-write root via overlayfs (lower=image, upper+work
     *     = writable). The mount itself makes cfg->merged a mount point, which is
     *     exactly the precondition pivot_root needs (new_root must be a mount
     *     distinct from its parent) — so unlike a plain-directory rootfs we need
     *     no self-bind trick here. */
    if (overlay_mount(cfg) == -1)
        die("overlay mount (need kernel >= 5.11 for unprivileged overlay)");

    /* (3) pivot_root stacks the OLD root under a directory inside the NEW root;
     *     that directory must already exist. Create <merged>/.oldroot. It lands
     *     in the overlay's upper layer, which is fine. */
    char put_old[PATH_MAX];
    snprintf(put_old, sizeof put_old, "%s/.oldroot", cfg->merged);
    if (mkdir(put_old, 0700) == -1 && errno != EEXIST)
        die("mkdir .oldroot");

    /* (4) pivot_root(new_root, put_old) — syscall 155, no glibc wrapper. The
     *     kernel makes new_root our "/" and moves the previous root under
     *     put_old. Fails EINVAL if step (1)/(2) left new_root shared/non-mount. */
    if (syscall(SYS_pivot_root, cfg->merged, put_old) == -1)
        die("pivot_root");

    /* (5) Our cwd still refers to the old root's inode; move it into the new
     *     root so nothing keeps the old tree busy. */
    if (chdir("/") == -1)
        die("chdir / after pivot_root");

    /* (6) Detach the old root. MNT_DETACH is a LAZY umount: it disconnects the
     *     subtree now and frees it once unreferenced, avoiding EBUSY. Then remove
     *     the now-empty mountpoint. */
    if (umount2("/.oldroot", MNT_DETACH) == -1)
        die("umount2 old root");
    if (rmdir("/.oldroot") == -1)
        warn("rmdir /.oldroot");         /* cosmetic; container still fine        */

    /* (7) Mount a FRESH proc for THIS PID namespace, so `ps` and /proc/net see
     *     only our processes and our (new) network namespace. Hardening flags:
     *     NOSUID (ignore set-uid bits), NODEV (no device nodes), NOEXEC (cannot
     *     exec out of /proc). Needs CAP_SYS_ADMIN — we still hold it. */
    if (mkdir("/proc", 0555) == -1 && errno != EEXIST)
        warn("mkdir /proc");
    if (mount("proc", "/proc", "proc",
              MS_NOSUID | MS_NODEV | MS_NOEXEC, NULL) == -1)
        die("mount /proc");

    /* (8) A read-only sysfs is handy (ip/ifconfig read /sys/class/net) but not
     *     essential, and in some userns configurations it is disallowed — so it
     *     is best-effort: warn and continue on failure. */
    if (mkdir("/sys", 0555) == -1 && errno != EEXIST) {
        /* Image usually already ships /sys; if not, the mount below just fails
         * and we carry on without it. Nothing to do here. */
    }
    if (mount("sysfs", "/sys", "sysfs",
              MS_NOSUID | MS_NODEV | MS_NOEXEC | MS_RDONLY, NULL) == -1)
        warn("mount /sys (non-fatal)");
}

/* ---------------------------------------------------------------------------
 * child_main — the clone() entry point.
 * --------------------------------------------------------------------------- */
int child_main(void *arg)
{
    struct engine_cfg *cfg = arg;

    /* (0) Synchronize with the parent. Close OUR copy of the write end so the
     *     parent is the only writer, then read until EOF — which happens when the
     *     parent closes its write end after installing uid_map/gid_map, the
     *     cgroup, and the network. Reading the byte-stream (rather than a token)
     *     means we also wake up if the parent dies, instead of hanging forever. */
    close(cfg->sync_pipe[1]);
    char b;
    while (read(cfg->sync_pipe[0], &b, 1) > 0)
        ;                                 /* spin until EOF (read returns 0)       */
    close(cfg->sync_pipe[0]);

    /* (1) We are now uid/gid 0 in the user namespace. Set them explicitly so any
     *     supplementary-group state is clean (the parent wrote setgroups=deny, so
     *     we cannot and need not touch the group list). */
    if (setgid(0) == -1) die("setgid(0)");
    if (setuid(0) == -1) die("setuid(0)");

    /* (2) UTS namespace: name the box. sethostname(2) is namespace-local —
     *     invisible to the host and to sibling containers. */
    if (sethostname(cfg->hostname, strlen(cfg->hostname)) == -1)
        die("sethostname");

    /* (3) Mount namespace: overlay root + pivot + /proc (+ best-effort /sys). */
    setup_mounts(cfg);

    /* (4) Shed privilege down to the keep set. After this we can no longer mount
     *     or load modules — which is the point. Composed with the same bitmask
     *     logic asm/demo.c dissects. */
    caps_apply(cfg->cap_keep);

    /* (5) Install the seccomp allowlist. It sets NO_NEW_PRIVS internally (a
     *     precondition) and must run AFTER we no longer need blocked syscalls. */
    if (install_seccomp() == -1)
        die("install_seccomp");

    /* (6) Become the container payload. execve is the FIRST syscall the filter
     *     must allow; from here the process image is the command, still PID 1,
     *     still inside every namespace we built, on the overlay root, networked.
     *     On success this never returns. */
    execvp(cfg->argv[0], cfg->argv);
    die("execvp (is the command present inside the image rootfs?)");
    return 127;                           /* unreachable; keeps the compiler calm  */
}
