/* ===========================================================================
 * child.c — everything that runs INSIDE the new namespaces, as PID 1.
 * ===========================================================================
 *
 * clone() drops us here. At this instant:
 *   - We are PID 1 in a brand-new PID namespace.
 *   - We are (about to be) uid 0 in a new user namespace, but the parent has not
 *     yet written our uid_map, so we FIRST block on the sync pipe.
 *   - We hold, once mapped, a full capability set *scoped to this user ns* — it
 *     grants power over our own namespaces, not over the host.
 *
 * The setup order below is not arbitrary; each step depends on the last:
 *
 *   wait for maps  ->  set ids  ->  set hostname  ->  build the mount tree
 *   (pivot_root + /proc)  ->  DROP capabilities  ->  install seccomp  ->  execve
 *
 * Capability dropping and seccomp come LAST, right before execve, because the
 * mount/pivot_root work genuinely needs CAP_SYS_ADMIN. Lock the door on the way
 * out, not on the way in.
 * ===========================================================================
 */
#define _GNU_SOURCE
#include "container.h"
#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>          /* PATH_MAX                                       */
#include <linux/capability.h>/* _LINUX_CAPABILITY_VERSION_3, cap data structs  */
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>       /* mount, umount2, MS_*, MNT_DETACH               */
#include <sys/prctl.h>       /* prctl, PR_CAPBSET_DROP                         */
#include <sys/stat.h>        /* mkdir                                          */
#include <sys/syscall.h>     /* syscall, SYS_pivot_root, SYS_capset            */
#include <unistd.h>

/* ---------------------------------------------------------------------------
 * setup_mounts — give the container its own root filesystem and /proc.
 *
 * This is the CLONE_NEWNS payload. Because we are in a private mount namespace,
 * nothing we do here is visible on the host — but only AFTER we mark the tree
 * private, because a fresh mount namespace inherits the host's mounts and, on
 * most distros, their "shared" propagation. If we skipped step (1), our mounts
 * would propagate back to the host and pivot_root would refuse to run.
 * --------------------------------------------------------------------------- */
static void setup_mounts(const struct container_cfg *cfg)
{
    /* (1) Recursively make every mount private. mount(2) with source/target/type
     *     all ignored and MS_REC|MS_PRIVATE just changes propagation flags.
     *     Kernel: walk the mount tree under "/" and clear shared/slave state. */
    if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL) == -1)
        die("mount --make-rprivate /");

    /* (2) pivot_root requires new_root to be a MOUNT POINT distinct from its
     *     parent mount. A plain directory is not. Bind-mounting rootfs onto
     *     itself turns it into its own mount. MS_BIND replays an existing subtree
     *     at a new location; MS_REC includes any nested mounts. */
    if (mount(cfg->rootfs, cfg->rootfs, NULL, MS_BIND | MS_REC, NULL) == -1)
        die("bind-mount rootfs onto itself");

    /* (3) pivot_root stacks the OLD root somewhere inside the NEW root; that
     *     "somewhere" must already exist. Create <rootfs>/.oldroot. */
    char put_old[PATH_MAX];
    snprintf(put_old, sizeof put_old, "%s/.oldroot", cfg->rootfs);
    if (mkdir(put_old, 0700) == -1 && errno != EEXIST)
        die("mkdir .oldroot");

    /* (4) pivot_root(new_root, put_old) — syscall 155. Args in rdi, rsi. The
     *     kernel makes new_root the process's root ("/") and moves the previous
     *     root's mount under put_old. There is no glibc wrapper, so we call it
     *     raw. Fails EINVAL if step (1)/(2) left new_root shared or non-mount. */
    if (syscall(SYS_pivot_root, cfg->rootfs, put_old) == -1)
        die("pivot_root");

    /* (5) Our working directory still refers to the old root's inode; move it to
     *     the new root so nothing keeps the old tree busy. */
    if (chdir("/") == -1)
        die("chdir / after pivot_root");

    /* (6) Detach the old root. MNT_DETACH is a LAZY umount: it disconnects the
     *     subtree now and frees it once nobody references it, which avoids EBUSY
     *     from mounts we may not have fully released. Then remove the mountpoint. */
    if (umount2("/.oldroot", MNT_DETACH) == -1)
        die("umount2 old root");
    if (rmdir("/.oldroot") == -1)
        warn("rmdir /.oldroot");         /* cosmetic; container still fine       */

    /* (7) Mount a FRESH proc for this PID namespace. Without this, /proc would
     *     still show the host's processes (or be absent). A new procfs instance
     *     reflects only our namespace's PIDs, so `ps` inside sees PID 1 = shell.
     *     Flags harden it: NOSUID (ignore set-uid bits), NODEV (no device nodes),
     *     NOEXEC (can't exec from /proc). Needs CAP_SYS_ADMIN — we still have it. */
    if (mkdir("/proc", 0555) == -1 && errno != EEXIST)
        warn("mkdir /proc");
    if (mount("proc", "/proc", "proc",
              MS_NOSUID | MS_NODEV | MS_NOEXEC, NULL) == -1)
        die("mount /proc");
}

/* ---------------------------------------------------------------------------
 * drop_capabilities — strip every capability before we hand control to /bin/sh.
 *
 * In a user namespace our capabilities do not confer host power, but dropping
 * them is real defense in depth: it shrinks what a compromise of the payload can
 * even attempt (no CAP_SYS_ADMIN to try more mounts, no CAP_NET_RAW, etc.), and
 * it is exactly what a production runtime does.
 *
 * Two distinct things must happen, IN THIS ORDER:
 *   (a) Empty the BOUNDING set. The bounding set is the ceiling on what any
 *       future execve can ever put in the permitted set. Dropping a bounding bit
 *       is itself privileged (needs CAP_SETPCAP), so we must do it while we still
 *       hold that capability — i.e. BEFORE step (b) zeroes everything.
 *   (b) Zero the effective/permitted/inheritable sets of THIS process via
 *       capset(2). Combined with an empty ambient set (never populated here),
 *       execve then yields a process with no capabilities at all.
 * --------------------------------------------------------------------------- */
static void drop_capabilities(void)
{
    /* (a) Drop the whole bounding set. We loop 0..63 to cover every currently
     *     defined capability plus headroom; prctl returns EINVAL for numbers the
     *     kernel doesn't define, which we ignore. PR_CAPBSET_DROP is one-way. */
    for (int cap = 0; cap <= 63; cap++) {
        if (prctl(PR_CAPBSET_DROP, cap, 0, 0, 0) == -1 && errno != EINVAL)
            warn("PR_CAPBSET_DROP");
    }

    /* (b) capset(2) — syscall 126. The v3 ABI splits 64 capabilities across two
     *     32-bit "data" words, so we pass a 2-element array, all zero = drop all.
     *     header.pid = 0 means "operate on the calling thread". */
    struct __user_cap_header_struct hdr;
    hdr.version = _LINUX_CAPABILITY_VERSION_3;   /* 0x20080522: the 64-bit ABI  */
    hdr.pid     = 0;

    struct __user_cap_data_struct data[2];
    memset(data, 0, sizeof data);                /* effective=permitted=inh.=0  */

    if (syscall(SYS_capset, &hdr, data) == -1)
        die("capset");
}

/* ---------------------------------------------------------------------------
 * child_main — the clone() entry point.
 * --------------------------------------------------------------------------- */
int child_main(void *arg)
{
    struct container_cfg *cfg = arg;

    /* (0) Synchronize with the parent. Close OUR copy of the write end so the
     *     parent is the only writer; then read until EOF, which happens when the
     *     parent closes its write end after installing uid_map/gid_map + cgroup.
     *     Reading the byte-stream (rather than expecting a token) means we also
     *     wake up if the parent dies, instead of hanging forever. */
    close(cfg->sync_pipe[1]);
    char b;
    while (read(cfg->sync_pipe[0], &b, 1) > 0)
        ; /* spin until EOF (return 0) */
    close(cfg->sync_pipe[0]);

    /* (1) We are now uid/gid 0 in the user namespace. Set them explicitly so any
     *     supplementary-group state is clean (setgroups was denied by the
     *     parent, so we cannot and need not touch the group list). */
    if (setgid(0) == -1) die("setgid(0)");
    if (setuid(0) == -1) die("setuid(0)");

    /* (2) UTS namespace: name the box. sethostname(2) writes the namespace-local
     *     hostname; it is invisible to the host and to sibling containers. */
    if (sethostname(cfg->hostname, strlen(cfg->hostname)) == -1)
        die("sethostname");

    /* (3) Mount namespace: pivot into the rootfs and mount a private /proc. */
    setup_mounts(cfg);

    /* (4) Shed privilege. After this we cannot mount again — which is the point. */
    drop_capabilities();

    /* (5) Install the seccomp allowlist. It sets NO_NEW_PRIVS internally (a
     *     precondition) and must run AFTER we no longer need blocked syscalls. */
    if (install_seccomp() == -1)
        die("install_seccomp");

    /* (6) Replace ourselves with the container payload. execve is the FIRST
     *     syscall the filter must allow; from here on the process image is the
     *     shell, still PID 1, still inside every namespace we built. On success
     *     this never returns; on failure (bad path, missing binary in rootfs) we
     *     report and exit non-zero. */
    execvp(cfg->argv[0], cfg->argv);
    die("execvp (is the command present inside the rootfs?)");
    return 127;                          /* unreachable; keeps the compiler calm */
}
