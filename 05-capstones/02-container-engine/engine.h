/* ===========================================================================
 * engine.h — shared types and the module boundary for `ceng`, the container
 *            engine capstone.
 * ===========================================================================
 *
 * WHAT THIS IS.  `ceng` is a *production-shaped* container engine, built as a
 * teaching core. It takes an image directory (an unpacked OCI rootfs) and runs
 * a process inside a full box:
 *
 *     clone(CLONE_NEWUSER|NEWNS|NEWPID|NEWNET|NEWUTS|NEWIPC|NEWCGROUP)
 *        + UID/GID maps                          (rootless identity)
 *        + an OVERLAYFS root  (lower=image, upper+work=writable copy-on-write)
 *        + pivot_root into it + a private /proc
 *        + cgroup v2 limits   (memory.max, cpu.max, pids.max)
 *        + a veth pair + NAT  (the box gets real, routable network egress)
 *        + capability drop    (keep only a Docker-like default set)
 *        + a seccomp-BPF allowlist
 *        + execve the payload as PID 1 of its namespace.
 *
 * HOW IT RELATES TO THE REST OF THE LAB.  Every mechanism here has a dedicated
 * sibling project that dissects it in isolation; this capstone is the wiring
 * that makes them a single system. The README's Architecture table links each
 * subsystem to its sibling:
 *
 *   namespaces / userns / pivot_root / cgroup / seccomp / caps
 *        -> ../../02-systems-tools/02-container-runtime   (the mechanisms)
 *   the NAT / packet path the veth egress rides on
 *        -> ../../01-kernel/09-netfilter-hook             (the kernel hook)
 *   the PID-1 payload you would really run inside
 *        -> ../../02-systems-tools/03-init-supervisor     (a correct init)
 *
 * HONEST SCOPE (repeated in the README, because honesty is the lesson): this is
 * the happy path at small scale. It is a real, runnable engine on a Linux host
 * with root; it is NOT runc. The named gaps (single-UID map, no image GC, no
 * userns-in-cgroup delegation dance, iproute2/iptables shelled out for the L3
 * plumbing) are documented, not hidden.
 *
 * WHY ONE SHARED CONFIG STRUCT?  clone(2) hands the child exactly one `void *`.
 * We point it at a `struct engine_cfg`. Because we deliberately do NOT pass
 * CLONE_VM, the child receives a copy-on-write duplicate of our address space
 * (exactly like fork), so the pointer stays valid and refers to the child's own
 * copy. The inherited pipe fds inside the struct keep the same fd numbers in
 * both processes, which is how the parent and child rendezvous.
 * ===========================================================================
 */
#ifndef CENG_ENGINE_H
#define CENG_ENGINE_H

#include <sys/types.h>   /* pid_t                                               */
#include <limits.h>      /* PATH_MAX                                            */

/* All tunables for one container, filled in by main() and read across modules.
 * Kept POD (plain-old-data) so a byte-for-byte CoW copy in the child is valid. */
struct engine_cfg {
    /* ---- payload + filesystem ------------------------------------------- */
    const char *image_dir;   /* overlay LOWER dir: the read-only image rootfs   */
    const char *state_dir;   /* where we create upper/, work/, merged/          */
    const char *hostname;    /* CLONE_NEWUTS hostname (sethostname(2))          */
    char      **argv;        /* command to exec inside; argv[0] is the program  */

    /* Overlay working paths, derived from state_dir by overlay_prepare().
     * merged is the mountpoint we pivot_root() into; upper holds all writes the
     * container makes; work is overlayfs's private scratch (must be an empty dir
     * on the SAME filesystem as upper). */
    char merged[PATH_MAX];
    char upper[PATH_MAX];
    char work[PATH_MAX];

    /* ---- namespaces ------------------------------------------------------ */
    /* The composed clone(2) flag word. main() builds it with the same bitmask
     * logic that asm/demo.c extracts and annotates. Kept in the struct so the
     * child can log which namespaces it was born into. */
    int clone_flags;

    /* ---- cgroup v2 limits (a value < 0 means "leave this controller alone") */
    long mem_max;    /* memory.max, bytes                                       */
    long cpu_quota;  /* cpu.max quota, microseconds per period                  */
    long cpu_period; /* cpu.max period, microseconds (typically 100000)         */
    long pids_max;   /* pids.max, max tasks in the cgroup (fork-bomb guard)     */
    char cgroup_path[256]; /* memo of the leaf dir so cgroup_destroy() rmdirs it */

    /* ---- capabilities ---------------------------------------------------- */
    /* A 64-bit KEEP mask: bit N set => keep capability N in the payload. The
     * bounding set is trimmed to exactly this mask and capset() installs it.
     * caps.c composes it with the same routine asm/demo.c isolates. */
    unsigned long long cap_keep;

    /* ---- network (veth pair + NAT) -------------------------------------- */
    int net_enable;          /* 0 => leave the netns with only loopback         */
    /* Fixed teaching subnet 10.0.42.0/24: host end .1, container end .2. Kept
     * as strings because we hand them to `ip`/`iptables` (see network.c note). */
    const char *net_subnet;  /* "10.0.42.0/24"                                  */
    const char *host_ip;     /* "10.0.42.1"  (default gateway seen by the box)  */
    const char *cont_ip;     /* "10.0.42.2"                                     */
    char veth_host[16];      /* host-side veth name, e.g. "ceh12345" (< IFNAMSIZ)*/
    char veth_cont[16];      /* container-side veth name, e.g. "cec12345"       */
    int  nat_added;          /* set once we install the MASQUERADE rule, so we   */
                             /*   only delete a rule we actually created         */

    /* ---- parent<->child rendezvous -------------------------------------- */
    /* The child blocks reading sync_pipe[0] until the parent has written the
     * UID/GID maps, joined it to the cgroup, and wired up the network, then
     * closes sync_pipe[1]. Using EOF (not a token byte) means a crashing parent
     * still releases the child instead of deadlocking it forever. */
    int sync_pipe[2];
};

/* --- module boundary ------------------------------------------------------ */

/* child.c — the clone(2) entry point; runs as PID 1 inside the namespaces. */
int  child_main(void *arg);

/* overlay.c — assemble the copy-on-write root filesystem. */
int  overlay_prepare(struct engine_cfg *cfg);   /* parent: make the dirs        */
int  overlay_mount(const struct engine_cfg *cfg);/* child: mount(2) the overlay */
void overlay_cleanup(const struct engine_cfg *cfg);/* parent: lazy-umount/rmdir  */

/* cgroup.c — best-effort cgroup v2 setup/teardown (needs root or delegation). */
int  cgroup_create(struct engine_cfg *cfg, pid_t child);
void cgroup_destroy(struct engine_cfg *cfg);

/* caps.c — compose the keep-mask and drop everything else (bounding + capset). */
unsigned long long caps_default_keep(void); /* the Docker-like default set      */
void caps_apply(unsigned long long keep);   /* child: trim bounding + capset    */

/* seccomp.c — install the classic-BPF allowlist over the current thread. */
int  install_seccomp(void);

/* network.c — veth pair + NAT. All parent-side (operates on the child's netns
 * by pid, while the child is still blocked on the sync pipe). Best-effort. */
int  network_setup(struct engine_cfg *cfg, pid_t child);
void network_cleanup(struct engine_cfg *cfg);

#endif /* CENG_ENGINE_H */
