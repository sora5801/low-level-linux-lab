# container runtime (mini-Docker) 🟧

**What it is.** `minic` is a small, **rootless-by-default** container runtime — a
Docker/`runc` in miniature. It puts a single process (a shell) inside a fresh
set of Linux namespaces, gives it a private root filesystem via `pivot_root`,
mounts it a private `/proc`, caps its RAM and CPU with **cgroup v2**, strips its
**capabilities**, and clamps its syscall surface with a **seccomp-BPF allowlist**
— then `execve`s the shell as PID 1 of its own PID namespace. It is a *teaching
core*: it demonstrates every mechanism a real runtime uses, end to end, on the
happy path, and is honest (below) about what it leaves out.

This project was **built and run**: on WSL2 (kernel 6.6, cgroup v2, rootless) it
starts a shell reporting `hostname=container`, `pid=1`, `uid=0` (mapped from the
real uid 1000), an isolated `/proc`, a network namespace with only `lo`, and a
seccomp filter that returns `EPERM` for any syscall outside the allowlist.

## What you'll learn

- **`clone(2)`** with `CLONE_NEWUSER | CLONE_NEWNS | CLONE_NEWPID | CLONE_NEWNET
  | CLONE_NEWUTS` — creating five namespaces atomically, and *why the user
  namespace has to be in the same call* for a rootless container.
- **UID/GID mapping**: writing `/proc/<pid>/uid_map`, `gid_map`, and the
  mandatory `setgroups=deny` step (the Linux 3.19 hardening).
- **`pivot_root(2)`** (syscall 155) and the mount-propagation dance
  (`MS_REC|MS_PRIVATE`, the self-bind trick) that has to precede it.
- **cgroup v2 as a filesystem**: `memory.max`, `cpu.max`, `cgroup.procs`, and the
  `cgroup.subtree_control` controller-enable rule.
- **Capabilities**: dropping the bounding set with `prctl(PR_CAPBSET_DROP)` and
  clearing all sets with **`capset(2)`** — in the right order.
- **seccomp mode 2**: building a classic-BPF allowlist, arch-pinning it, and
  installing it with `seccomp(2)` after `PR_SET_NO_NEW_PRIVS`.

## Build & run (Linux / WSL only)

The runtime uses Linux-only syscalls, so it builds and runs on **Linux or WSL2**.
You need a kernel with cgroup v2 and (for rootless) unprivileged user namespaces
enabled — check with `cat /proc/sys/user/max_user_namespaces` (nonzero = ok).

```bash
make                       # builds ./minic with -Wall -Wextra
make rootfs                # builds ./rootfs from a static busybox (needs busybox)
make run                   # ./minic --rootfs ./rootfs -- /bin/sh   (interactive)
make test                  # non-interactive: prints hostname, PID 1, isolated /proc
```

No busybox? Any directory containing a `/bin/sh` and its shared libraries works
as a rootfs (copy them with `ldd`), or `docker export $(docker create alpine) |
tar -x -C rootfs`.

**cgroup limits** need root or a delegated cgroup (rootless without delegation
can't write the controller files — `minic` prints a clear warning and runs on,
uncapped):

```bash
sudo ./minic --mem 33554432 --cpu 25 --rootfs ./rootfs -- /bin/sh
#            32 MiB RAM      25% of one CPU
```

Prove the isolation from inside the shell:

```sh
hostname            # -> container          (UTS namespace)
echo $$             # -> 1                  (PID namespace: the shell is init)
id -u               # -> 0                  (user namespace: root in-box only)
cat /proc/net/dev   # -> only 'lo'          (empty network namespace)
chmod 777 /         # -> Operation not permitted  (seccomp: chmod not allowlisted)
```

## How it works (file by file)

The code is split so the container "story" reads across files:

- **`container.h`** — the shared `struct container_cfg` (all tunables) and the
  module boundary. Explains why `clone`'s single `void *arg` can safely point at
  parent memory (no `CLONE_VM` ⇒ copy-on-write address space, like `fork`).
- **`container.c`** — the **parent**. Parses args, allocates the child stack
  (`mmap`), and makes the one `clone()` call. Then does the two things only the
  parent can: `write_maps()` installs the UID/GID identity map, and
  `cgroup_create()` puts the child in a cgroup — both **before** releasing the
  child through the sync pipe. Finally `waitpid`s PID 1 and cleans up.
- **`child.c`** — everything **inside** the namespaces, running as PID 1. Waits
  on the pipe, sets its ids and hostname, calls `setup_mounts()`
  (`MS_PRIVATE` → self-bind → `pivot_root` → detach old root → mount `/proc`),
  then `drop_capabilities()` (bounding set first, then `capset`), installs
  seccomp, and `execve`s the payload. The order — privileged mounts first,
  lock-down last — is the whole lesson.
- **`cgroup.c`** — cgroup v2 as pure filesystem writes: enable controllers in
  `subtree_control`, `mkdir` the leaf, write `memory.max` / `cpu.max`, then write
  the child PID to `cgroup.procs`. Best-effort with honest warnings.
- **`seccomp.c`** — builds the classic-BPF program: validate `arch` (kill on
  mismatch to stop the x32/ia32 number-reuse bypass), then a linear allow-scan of
  `nr`, defaulting to `EPERM`. Installs via `seccomp(2)`, falling back to
  `prctl(PR_SET_SECCOMP)` on old kernels.
- **`util.c`** — `die`/`warn` and the `write_file()` full-write helper (handles
  `EINTR`/short writes) that every `/proc` and cgroup write goes through.

### Why the ordering matters (the two orderings this project is about)

1. **User namespace first.** An unprivileged process cannot create mount/PID/net
   namespaces alone. Putting `CLONE_NEWUSER` in the *same* `clone()` makes the
   kernel build the user namespace first and grant the others to it, where we
   hold full in-namespace capabilities. That is what makes rootless possible.
2. **Drop privilege last.** `pivot_root` and mounting `/proc` need
   `CAP_SYS_ADMIN`, so capabilities are dropped and the seccomp filter installed
   **after** the mount work and immediately **before** `execve`.

## Assembly notes

The real sources `#include` Linux headers, so they only compile on Linux; the
committed teaching assembly instead comes from **`asm/demo.c`**, a self-contained
(no-headers) extraction of the project's most instructive pure-logic routine: the
**classic-BPF interpreter** that is the heart of seccomp evaluation. The kernel
runs exactly this kind of loop — load a field of `seccomp_data`, compare, branch,
return a verdict — on *every syscall* a container makes. `demo.c` compiles a tiny
allowlist (the same shape `seccomp.c` emits) and runs `read`, `write`, and
`ptrace` through it; a correct build exits `7`.

- [`asm/demo.annotated.s`](asm/demo.annotated.s) — the hand-annotated `-O1`
  output: a full SysV AMD64 ABI header (arg/return/callee-saved registers, the
  red zone, 16-byte call alignment) and a comment on essentially every
  instruction. It walks the interpreter's register allocation (`ecx`=pc,
  `r8d`=accumulator), shows how `-fno-jump-tables` turns the opcode `switch` into
  an explicit compare-chain, and how the compiler builds an 8-byte `seccomp_data`
  struct with a **single `movabsq`** store (a neat little-endian layout lesson).
- [`asm/demo.O0.s`](asm/demo.O0.s) — naive `-O0`: every value spilled to the
  stack; easiest to map line-for-line to the C.
- [`asm/demo.s`](asm/demo.s) — `-O1`, the annotated baseline.
- [`asm/demo.O2.s`](asm/demo.O2.s) — `-O2`, tighter register use, for comparison.

Regenerate them with `make asm` (works on any host — clang cross-targets Linux).

## Going further (the `Stretch:` from the list)

- **An OCI-compatible `runc`-lite.** Parse an OCI `config.json` (the `process`,
  `root`, `linux.namespaces`, `linux.resources`, `linux.seccomp` blocks) instead
  of CLI flags, and implement the create/start/kill/delete state machine over a
  `state.json`. That is essentially what `runc` is around this same core.
- **`veth` pair + NAT networking.** Today the network namespace has only a
  down `lo`. Real networking means: create a `veth` pair, move one end into the
  container's netns (via its netns fd), assign addresses, bring both ends up, add
  a default route, and `iptables`/`nftables` MASQUERADE on the host for egress —
  all over `AF_NETLINK` (rtnetlink) `RTM_NEWLINK`/`RTM_NEWADDR`/`RTM_NEWROUTE`.
- **Wider UID mapping.** For a container that can `useradd`, map a whole range
  from `/etc/subuid` + `/etc/subgid` using the setuid helpers `newuidmap(1)` /
  `newgidmap(1)`, instead of the single `0 → euid` mapping used here.

### What this teaching core does **not** do (honest scope)

- Only a **single-UID** map (`0 → your uid`), not a subuid range.
- **No networking setup** beyond the empty netns (no veth/NAT).
- **No device management** (`/dev` is not populated; add a `devtmpfs`/bind and a
  device cgroup for real use), no user/OCI mounts, no pivot to an overlayfs.
- cgroup limits are **best-effort** and need root or explicit v2 delegation.
- The seccomp default is `EPERM` (so you can *see* what's blocked); a production
  sandbox flips `DEFAULT_ACTION` in `seccomp.c` to `SECCOMP_RET_KILL_PROCESS`.

## References

- `man 2 clone`, `man 7 user_namespaces`, `man 7 pid_namespaces`,
  `man 7 mount_namespaces`, `man 2 pivot_root`.
- `man 7 cgroups` and the kernel's `Documentation/admin-guide/cgroup-v2.rst`.
- `man 2 capset`, `man 7 capabilities`; `man 2 seccomp`, `man 2 prctl`, and the
  kernel's `Documentation/userspace-api/seccomp_filter.rst`.
- Read the real thing: **`runc`** (`libcontainer/`), **`crun`** (a C runtime — the
  closest large-scale cousin of this code), and the **OCI runtime-spec**.
