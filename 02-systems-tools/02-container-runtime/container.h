/* ===========================================================================
 * container.h — shared types and the module boundary for `minic`.
 * ===========================================================================
 *
 * `minic` is a *rootless-by-default* mini-Docker: it puts a single process
 * (a shell) inside its own set of Linux namespaces, gives it a private root
 * filesystem, caps its memory and CPU with cgroup v2, strips its capabilities,
 * and locks its syscall surface with a seccomp-BPF allowlist.
 *
 * The code is split so the story reads top-to-bottom across three files:
 *
 *   container.c  — the PARENT side. Parses args, calls clone(2), and does the
 *                  two things only the parent can do for a user namespace:
 *                  write the UID/GID maps, and move the child into a cgroup.
 *   child.c      — the INSIDE-the-namespaces side. Runs as PID 1 of the new
 *                  PID namespace: sets the hostname, pivots into the rootfs,
 *                  mounts a fresh /proc, drops caps, installs seccomp, execs.
 *   seccomp.c    — builds and installs the classic-BPF syscall allowlist.
 *   cgroup.c     — writes the cgroup v2 controller files (memory.max/cpu.max).
 *
 * WHY A SHARED CONFIG STRUCT?  clone(2) hands the child ONE `void *arg`. We
 * point it at this struct. Because we deliberately do NOT pass CLONE_VM, the
 * child gets a copy-on-write duplicate of the parent's address space (exactly
 * like fork), so the pointer is valid in the child and refers to the child's
 * own copy of the struct. The inherited pipe file descriptors inside it share
 * the same fd numbers in both processes.
 * ===========================================================================
 */
#ifndef MINIC_CONTAINER_H
#define MINIC_CONTAINER_H

#include <sys/types.h>   /* pid_t */

/* All tunables for one container, filled in by main() and read by the child. */
struct container_cfg {
    const char *rootfs;     /* directory we pivot_root() into (the new "/")     */
    const char *hostname;   /* CLONE_NEWUTS hostname set with sethostname(2)    */
    char      **argv;       /* command to exec; argv[0] is the program path     */

    /* cgroup v2 limits. A value < 0 means "leave this controller alone". */
    long        mem_max;    /* memory.max, in bytes                             */
    long        cpu_quota;  /* cpu.max quota, in microseconds per period        */
    long        cpu_period; /* cpu.max period, in microseconds (usually 100000) */

    /* Parent->child readiness pipe. The child blocks reading sync_pipe[0] until
     * the parent has written the UID/GID maps and set up the cgroup, then closes
     * sync_pipe[1] to signal "go". Using EOF (not a byte) means a crashing parent
     * still releases the child instead of deadlocking it forever. */
    int         sync_pipe[2];

    /* Filled in by cgroup_create() so cgroup_destroy() can rmdir the same path. */
    char        cgroup_path[256];
};

/* child.c — the function clone(2) jumps to; runs as PID 1 in the new namespaces. */
int  child_main(void *arg);

/* seccomp.c — install the classic-BPF allowlist over the current thread. */
int  install_seccomp(void);

/* cgroup.c — best-effort cgroup v2 setup/teardown (needs root or delegation). */
int  cgroup_create(struct container_cfg *cfg, pid_t child);
void cgroup_destroy(struct container_cfg *cfg);

#endif /* MINIC_CONTAINER_H */
