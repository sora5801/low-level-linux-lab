# container engine (production-shaped) 🟥 · teaching-core 🧩

**What it is.** `ceng` is a container engine capstone: it takes an unpacked OCI
image and runs a process inside a *full* box — the same seven mechanisms a real
runtime combines, wired together end to end:

```
clone(NEWUSER|NEWNS|NEWPID|NEWNET|NEWUTS|NEWIPC)  +  UID/GID maps
  +  an OVERLAYFS root (lower=image, upper+work = copy-on-write writable layer)
  +  pivot_root into it  +  a private /proc (+ best-effort /sys)
  +  cgroup v2 limits   (memory.max, cpu.max, pids.max)
  +  a veth pair + NAT  (the box gets real, routable egress)
  +  capability drop    (keep only a Docker-like default set)
  +  a seccomp-BPF allowlist
  +  execve the payload as PID 1 of its own namespace.
```

It ships two host-side helpers the spec calls for: a **rootfs builder**
(`scripts/build-rootfs.sh`, busybox or debootstrap) and a **minimal OCI puller**
(`scripts/oci-pull.sh`, token → manifest → layers → extract) so you can feed it
either a hand-built or a real registry image.

**This is an honest teaching core, not `runc`.** It is genuinely runnable on a
Linux host with root and demonstrates every subsystem at small scale on the happy
path. It is deliberately *not* a production runtime — the [honest-scope](#what-this-teaching-core-does-not-do-honest-scope)
list below names every gap. Honesty about scope is part of the lesson: a
container engine is a *composition* of a dozen kernel features, and the value
here is seeing the composition whole, then reading each feature's dedicated
sibling project for depth (see [Architecture](#architecture)).

## What you'll learn

- **`clone(2)` flag composition** — building the namespace flag word bit by bit,
  why `CLONE_NEWUSER` in the *same* call is the rootless enabler, and why the
  `CLONE_NEW*` constants are sparse (dissected in the assembly).
- **overlayfs as a filesystem** — `mount("overlay", …, "lowerdir=…,upperdir=…,
  workdir=…")`, copy-up on first write, why the image stays read-only and shared,
  and the kernel ≥ 5.11 requirement for mounting it inside a user namespace.
- **`pivot_root(2)`** and the mount-propagation dance (`MS_REC|MS_PRIVATE`) that
  must precede it, plus mounting a fresh `/proc` for the PID namespace.
- **cgroup v2 as pure filesystem writes** — `memory.max`, `cpu.max`, **`pids.max`**
  (the fork-bomb guard), the `subtree_control` controller-enable rule, and
  `cgroup.procs` migration.
- **Capability sets** — composing a keep mask, trimming the **bounding set**
  first (it outlives `execve`), then `capset(2)` — and *why that order*.
- **seccomp mode 2** — a classic-BPF allowlist, arch-pinning, `NO_NEW_PRIVS`.
- **rtnetlink by hand** — creating a **veth pair** with a nested-attribute
  `RTM_NEWLINK` (`IFLA_LINKINFO → IFLA_INFO_KIND=veth → IFLA_INFO_DATA →
  VETH_INFO_PEER`) and moving the peer into another netns by pid
  (`IFLA_NET_NS_PID`), then **NAT/MASQUERADE** for egress.
- **The OCI pull protocol** — bearer token, manifest (and multi-arch index),
  content-addressed layer blobs, gzip-tar extraction, `.wh.` whiteouts.

## Build & run

Platform: **Linux or WSL2**, kernel **≥ 5.11** (for unprivileged overlayfs). The
full-feature run needs **root** (overlay mount, cgroup writes, and veth/NAT all
require real host privilege). The `asm` target builds on any host.

```bash
make                       # builds ./ceng with -Wall -Wextra
make rootfs                # ./image from a static busybox   (needs busybox)
#   or:
make pull IMAGE=alpine:3.19# ./image pulled from Docker Hub  (needs curl/jq/tar)

sudo ./ceng --image ./image --state ./state -- /bin/sh   # interactive shell
make run                   # same thing
make test                  # non-interactive: prints hostname, PID 1, veth, /proc
```

Useful flags (see `./ceng --help`):

```
--mem 33554432   # 32 MiB memory.max
--cpu 25         # 25% of one CPU (cpu.max quota/period)
--pids 64        # cap the task count
--no-net         # skip veth/NAT; leave the netns with only loopback
--hostname box   # UTS hostname inside
```

Prove the isolation from inside the shell:

```sh
hostname                 # -> container         (UTS namespace)
echo $$                  # -> 1                 (PID namespace: the shell is init)
id -u                    # -> 0                 (user namespace: root in-box only)
ip addr                  # -> lo + cec<pid> with 10.0.42.2   (veth moved in)
ping -c1 1.1.1.1         # -> replies           (NAT egress works)
cat > /etc/hostname      # -> writes land in ./state/upper   (overlay copy-up)
mount -t proc p /mnt     # -> Operation not permitted        (caps dropped)
```

Rootless (no `sudo`): the container still starts and isolates, but the overlay
needs kernel ≥ 5.11, and cgroup limits + veth/NAT are **skipped with a clear
warning** (they need privilege) — the box comes up with only loopback.

## Architecture

`ceng` is the wiring; each subsystem has a sibling project in this lab that
dissects it in isolation. Read `ceng` for *how they compose*, and the siblings
for *how each one works*.

```
                      ┌───────────────────────────────────────────────┐
   host / parent      │                   ceng (main.c)               │
   (needs root)       │   parse args → clone() → maps → cgroup → net  │
                      └───────┬───────────────┬──────────────┬────────┘
                              │ clone()        │ /proc/<pid>/ │ rtnetlink
                              │ (compose_      │ uid_map      │ + ip/iptables
                              │  clone_flags)  │ cgroup.procs │
                              ▼                ▼              ▼
   ┌──────────────────────────────────────────────────────────────────────┐
   │  CHILD = PID 1 in new namespaces (child.c)                            │
   │                                                                      │
   │   overlay.c   lower=image  upper+work=writable  → merged  → pivot_root│
   │   caps.c      trim bounding set → capset() to the keep mask           │
   │   seccomp.c   classic-BPF allowlist over every syscall                │
   │   execve(payload)  ── runs on overlay root, networked, resource-capped│
   └──────────────────────────────────────────────────────────────────────┘
                              ▲                ▲              ▲
        veth pair + NAT ──────┘   cgroup v2 ───┘   namespaces ┘
        (network.c)               (cgroup.c)       (main/child)
```

### Subsystem → sibling project map

| Subsystem in `ceng` | File(s) here | Sibling project that dissects it |
|---|---|---|
| Namespaces, user-ns UID/GID maps, `pivot_root`, capability drop, seccomp — the core isolation mechanisms | `main.c`, `child.c`, `caps.c`, `seccomp.c` | [`../../02-systems-tools/02-container-runtime`](../../02-systems-tools/02-container-runtime) — the same mechanisms built standalone, with per-file deep dives and the seccomp-interpreter assembly |
| The NAT / packet path the veth egress rides on (`POSTROUTING` MASQUERADE is one of Netfilter's five hook points) | `network.c` | [`../../01-kernel/09-netfilter-hook`](../../01-kernel/09-netfilter-hook) — a kernel module that registers `nf_hook_ops` at those same hook points and parses `sk_buff`s |
| The PID-1 init a real container should run as its ENTRYPOINT (reaping, signals, shutdown) | (payload; not built in) | [`../../02-systems-tools/03-init-supervisor`](../../02-systems-tools/03-init-supervisor) — a correct PID 1: `SIGCHLD` reaping, `signalfd`, dependency-ordered start/restart |

New to this capstone (not in the siblings): the **overlayfs** copy-on-write root
(`overlay.c`), the **cgroup v2 `pids.max`** guard (`cgroup.c`), the **raw
rtnetlink veth** creation and netns-move (`network.c`), and the **OCI puller**
(`scripts/oci-pull.sh`). The assembly reference standard is
[`../../04-security-asm/01-nolibc-programs/asm/hello.annotated.s`](../../04-security-asm/01-nolibc-programs/asm/hello.annotated.s).

## How it works (file by file)

The code is split so the container "story" reads across files:

- **`engine.h`** — the shared `struct engine_cfg` (every tunable) and the module
  boundary. Explains why `clone`'s single `void *arg` can safely point at parent
  memory (no `CLONE_VM` ⇒ copy-on-write address space, like `fork`).
- **`main.c`** — the **parent**. Parses args, composes the clone flag word
  (`compose_clone_flags`, mirrored in `asm/demo.c`), allocates the child stack
  and makes the one `clone()` call. Then does what only the parent can, in
  dependency order: `write_maps()` (setgroups=deny → uid_map → gid_map),
  `cgroup_create()`, `network_setup()` — all **before** releasing the child
  through the sync pipe. Finally `waitpid`s PID 1 and tears everything down.
- **`child.c`** — everything **inside** the namespaces, as PID 1. Waits on the
  pipe, sets ids + hostname, `setup_mounts()` (make-private → overlay mount →
  `pivot_root` → detach old root → `/proc` → best-effort `/sys`), then
  `caps_apply()` and `install_seccomp()`, then `execvp`. Privileged mounts
  first, lock-down last — that ordering is the whole lesson.
- **`overlay.c`** — the copy-on-write root: `overlay_prepare()` (parent makes
  `upper/ work/ merged/`) and `overlay_mount()` (child's single `mount(2)` with
  the `lowerdir=,upperdir=,workdir=` option string). The writable layer is
  preserved in `./state/upper` so you can inspect what the container changed.
- **`cgroup.c`** — cgroup v2 as filesystem writes: enable `+memory +cpu +pids`
  in `subtree_control`, `mkdir` the leaf, write the three limit files, then write
  the child PID to `cgroup.procs`. Best-effort with honest warnings.
- **`caps.c`** — compose the 64-bit keep mask (Docker's 14 defaults), trim the
  **bounding set** to it, then `capset(2)`. The bitmask composition here is what
  the assembly deliverable isolates.
- **`seccomp.c`** — the classic-BPF allowlist: arch-pin (kill on mismatch),
  then a linear allow-scan of `nr`, default `EPERM` so you can *see* what is
  blocked. Broadened over the sibling's set to include the socket syscalls, so
  the payload can actually use the network the veth gives it.
- **`network.c`** — the flagship new piece: **raw rtnetlink** builds the veth
  pair (nested `IFLA_LINKINFO/INFO_KIND/INFO_DATA/VETH_INFO_PEER`) and moves the
  peer into the child's netns by pid (`IFLA_NET_NS_PID`); then `ip`/`iptables`
  (via `run_cmd`) assign addresses/routes and install the `MASQUERADE` rule.
  The header comment is explicit and honest about the netlink-vs-shell split.
- **`util.c`** — `die`/`warn`, the `write_file()` full-write helper every
  `/proc` and cgroup write goes through, and `run_cmd()` (fork/exec/wait).

### Two orderings this capstone is about

1. **User namespace first, in one `clone()`.** An unprivileged process cannot
   create mount/PID/net namespaces alone; combining `CLONE_NEWUSER` in the same
   call makes the kernel build the user namespace first and grant the rest to it.
2. **Privileged setup first, drop last.** Overlay mount, `pivot_root`, `/proc`,
   and the veth all need `CAP_SYS_ADMIN`; capabilities are trimmed and seccomp
   installed **after** that work and immediately **before** `execve`.

## Assembly notes

The engine sources `#include` Linux headers, so they only compile on Linux; the
committed teaching assembly instead comes from **`asm/demo.c`**, a self-contained
(no-headers) extraction of the capstone's most instructive pure-logic core: the
**clone-flags and capability bitmask composition** (`compose_clone_flags`,
`cap_keep_mask`, `cap_drop_mask`). `demo.c` composes a full container's flag word
and cap masks and probes three bits; a correct build exits **7** (`make demo`).

- [`asm/demo.annotated.s`](asm/demo.annotated.s) — the hand-annotated `-O1`
  output: a full SysV AMD64 ABI header and a comment on essentially every
  instruction. It walks the table-driven `testl`/`orl` flag loop, shows the
  optimizer **constant-folding the entire `cap_keep_mask` loop** into a single
  `movl $0xA80425FB`, and shows **why the masks are `u64`** — `cap_drop_mask`
  needs `movabsq $0x1FFFFFFFFFF` and 64-bit `notq`/`andq` because the "all
  defined caps 0..40" value overflows 32 bits. `main` shows single-bit probes
  via shift-and-mask and callee-saved caching across calls.
- [`asm/demo.O0.s`](asm/demo.O0.s) — naive `-O0`: every value spilled, both
  loops written out; easiest to map line-for-line to the C.
- [`asm/demo.s`](asm/demo.s) — `-O1`, the annotated baseline.
- [`asm/demo.O2.s`](asm/demo.O2.s) — `-O2`, tighter still, for comparison.

Regenerate with `make asm` (works on any host — clang cross-targets Linux).

## Going further

- **Parse an OCI `config.json` / runtime bundle.** Instead of CLI flags, read the
  OCI runtime-spec `config.json` (`process`, `root`, `linux.namespaces`,
  `linux.resources`, `linux.seccomp`) and implement the create/start/kill/delete
  state machine over a `state.json` — that is essentially what `runc` wraps
  around this same core.
- **A real init as PID 1.** Bind [`../../02-systems-tools/03-init-supervisor`](../../02-systems-tools/03-init-supervisor)
  into the image and `--exec` it so zombies are reaped and `SIGTERM` shuts the
  box down cleanly — the correct behavior for a container's process 1.
- **Netlink all the way.** Replace the `ip`/`iptables` shell-outs with
  `RTM_NEWADDR`/`RTM_NEWROUTE` (same nested-attribute pattern as the veth create)
  and program the NAT rule over the nftables netlink API instead of forking.
- **A subuid range** via `/etc/subuid` + `newuidmap(1)` so the container can
  `useradd`, instead of the single `0 → euid` map used here.
- **Layer store + GC.** Cache pulled layers by digest and share unchanged lowers
  across images (the `lowerdir=a:b:c` multi-layer form), instead of flattening.

### What this teaching core does **not** do (honest scope)

- **Single-UID map** (`0 → your uid`), not a subuid range.
- **Networking L3 is shelled out.** veth creation + netns-move are raw rtnetlink;
  addresses, routes, and the `MASQUERADE` rule are done via `ip`/`iptables`
  (documented in `network.c`). A fixed `10.0.42.0/24` `/24`, one container at a
  time on that subnet.
- **overlayfs needs kernel ≥ 5.11** for the unprivileged-userns mount; `/dev` is
  not populated (no `devtmpfs`/device cgroup) and `/sys` is best-effort.
- **cgroup limits are best-effort** and need root or explicit v2 delegation.
- **seccomp default is `EPERM`** (so you can *see* what is blocked); a production
  sandbox flips `DEFAULT_ACTION` in `seccomp.c` to `SECCOMP_RET_KILL_PROCESS`.
- **The OCI puller** handles gzip layers + basic whiteouts only — no zstd, no
  signature/digest verification, no manifest caching.
- **No image GC, no `exec` into a running container, no checkpoint/restore, no
  cgroup namespace** (left out to keep the cgroup-migration story simple).

## References

- `man 2 clone`, `man 7 user_namespaces`, `man 7 pid_namespaces`,
  `man 7 mount_namespaces`, `man 2 pivot_root`, `man 8 mount` (overlay).
- `man 7 cgroups` and the kernel's `Documentation/admin-guide/cgroup-v2.rst`.
- `man 2 capset`, `man 7 capabilities`; `man 2 seccomp`, `man 2 prctl`.
- `man 7 rtnetlink`, `man 7 netlink`, `ip-link(8)`, `iptables(8)`.
- The **OCI image-spec** and **distribution-spec** (registry API v2).
- Read the real thing: **`runc`** (`libcontainer/`), **`crun`** (a C runtime —
  the closest large-scale cousin of this code), **`containerd`**, and the CNI
  **bridge**/**host-local** plugins for the networking side.
