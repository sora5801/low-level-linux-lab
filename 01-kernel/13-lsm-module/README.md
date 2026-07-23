# A Linux Security Module (LSM) 🟥

**What it is.** `pathguard` is a tiny **Mandatory Access Control** module written
in the modern *stackable* LSM style. It registers two security hooks and
enforces a path-prefix policy:

- **`file_open`** — an unprivileged process may not *open* files under a
  protected directory (`/root`, a secrets dir). A read/write **denylist**.
- **`bprm_check_security`** — an unprivileged process may not *execute* a binary
  that lives **outside an allowlist** of trusted dirs (`/usr`, `/bin`, …). The
  "no exec from `/tmp`" pattern.

This is an honest **teaching core**: two hooks, a hand-written policy table, a
root bypass. It shows how an LSM *plugs in* and where hooks *fire* — not a
production MAC system (no labels, no per-inode policy, no userspace tooling).

> **This is 🟥 largely because of *how it runs*, not how much code it is.** A
> real LSM is compiled **into the kernel image**, not loaded with `insmod`. The
> "Build & run" section explains both the quick compile-check and the real
> in-tree build, and *why* the module can't just be `insmod`-ed.

## What you'll learn

- The **LSM framework**: `security_hook_list`, `LSM_HOOK_INIT`,
  `security_add_hooks()`, and `DEFINE_LSM()` — how a module attaches callbacks
  to the kernel's ~250 hook points.
- **LSM stacking**: how SELinux/AppArmor/Yama/Landlock/pathguard coexist, the
  `CONFIG_LSM=` ordering, and the *restrictive AND* composition rule (any LSM
  can deny; all must allow).
- **Where hooks fire**: `file_open` inside `do_dentry_open()` (fs/open.c),
  `bprm_check_security` inside `security_bprm_check()` on the exec path
  (fs/exec.c).
- **Credentials & `current`**: reading `current_cred()->fsuid`, `uid_eq()`,
  `GLOBAL_ROOT_UID`, and why fsuid (not uid) governs file access.
- **Turning a `struct path` into text** with `d_path()` — the backward-building
  buffer, the pointer-into-buffer return, `IS_ERR()`, and page-sized scratch.
- Why `security_add_hooks()` being `__init` **proves** an LSM cannot be a
  loadable module.

## Build & run

> **Platform: Linux only, and do it in a QEMU VM.** A wrong exec allowlist can
> lock *you* out of running programs. Never debug an enforcing LSM on a machine
> you can't reboot freely.

### 1. Compile-check against your kernel headers (any Linux box)

This type-checks the code against the *real* hook signatures and structs. It
produces `pathguard.ko`, but **that .ko will not `insmod`** — see the note.

```bash
sudo apt-get install -y linux-headers-$(uname -r)   # or your distro's equivalent
cd 01-kernel/13-lsm-module
make                       # -> make -C /lib/modules/$(uname -r)/build M=$(PWD) modules
```

Expect a successful compile and, at the `modpost` stage, a warning that
`security_add_hooks` is an undefined symbol. **That warning is the lesson**:
`security_add_hooks()` is `__init` and unexported, so it is not available to
modules. `insmod pathguard.ko` would fail with *"Unknown symbol
security_add_hooks"* even if the warning were suppressed.

### 2. Build it *into* a kernel and actually run it (QEMU)

```bash
# In a Linux kernel source tree (e.g. linux-6.8+):
mkdir -p security/pathguard
cp /path/to/pathguard.c security/pathguard/

# Wire it into the security subsystem's Kconfig and Makefile:
#   security/pathguard/Kconfig:
#       config SECURITY_PATHGUARD
#           bool "PathGuard path-prefix MAC (teaching)"
#           depends on SECURITY
#           default n
#   security/pathguard/Makefile:
#       obj-$(CONFIG_SECURITY_PATHGUARD) += pathguard.o
#   security/Kconfig:   add:  source "security/pathguard/Kconfig"
#   security/Makefile:  add:  subdir-$(CONFIG_SECURITY_PATHGUARD) += pathguard
#                             obj-$(CONFIG_SECURITY_PATHGUARD)    += pathguard/

make menuconfig            # enable CONFIG_SECURITY and CONFIG_SECURITY_PATHGUARD
# Ensure pathguard is in the active LSM list. Either set it in the config's
# CONFIG_LSM="landlock,lockdown,yama,...,pathguard" or pass on the kernel
# command line:  lsm=...,pathguard   (and pathguard.enforce=0 to start permissive)

make -j$(nproc) bzImage
qemu-system-x86_64 -kernel arch/x86/boot/bzImage \
    -append "console=ttyS0 root=/dev/... lsm=capability,pathguard pathguard.enforce=0" \
    -nographic ...        # plus your rootfs/initramfs
```

### 3. Watch it work (inside the VM)

```bash
dmesg | grep PathGuard                 # "initialising (enforce=0, ...)"

# file_open denylist — as a NON-root user:
sudo mkdir -p /etc/pathguard-secret && echo hi | sudo tee /etc/pathguard-secret/k
cat /etc/pathguard-secret/k            # permissive: works + logs; enforcing: EACCES

# exec allowlist — as a NON-root user:
cp /bin/true /tmp/payload && /tmp/payload   # enforcing: "Permission denied"
dmesg | tail                                 # PathGuard: deny exec ... path=/tmp/payload

# flip to enforcing at runtime (built-in module exposes the param via sysfs):
echo 1 | sudo tee /sys/module/pathguard/parameters/enforce
```

## How it works (file by file)

- **`pathguard.c`** — the module, in four parts:
  1. *Pure policy core.* `pg_path_has_prefix()` (component-boundary-aware prefix
     match) and `pg_policy_lookup()` (ordered first-match over a rule table),
     plus the two policy tables. This is the code extracted to `asm/demo.c`.
  2. *`pathguard_check_path()`* — renders a `struct path` to text with
     `d_path()` into a page-sized buffer, runs the table, logs, and returns
     `-EACCES` (enforcing) or `0`. Documents the `d_path` return-pointer and
     fail-open-on-OOM tradeoff.
  3. *The two hooks* — `pathguard_file_open()` and
     `pathguard_bprm_check_security()`, each with a `current_cred()->fsuid` root
     bypass and a note on **where in the kernel the hook fires**.
  4. *Registration* — the `pathguard_hooks[] __ro_after_init` table via
     `LSM_HOOK_INIT`, the 6.8+ `struct lsm_id`, the `__init` init function
     calling `security_add_hooks()`, and `DEFINE_LSM()`.
- **`Makefile`** — Kbuild `obj-m` build (compile-check) + `make asm` to
  regenerate the teaching assembly.
- **`asm/`** — the assembly deliverable (below).

## Assembly notes

Kernel C is **not standalone-compilable** on this host (it needs a configured
kernel tree), so — per the repo convention — the project's most instructive
pure-logic routine is extracted into a self-contained **`asm/demo.c`**: the
`pg_path_has_prefix()` / `pg_policy_lookup()` pair, byte-for-byte identical to
the module's, declaring its own types and including nothing.

- [`asm/demo.annotated.s`](asm/demo.annotated.s) — the `-O1` output with a
  comment on essentially every instruction, an ABI header, and the struct-layout
  walkthrough. Highlights:
  - The **security boundary rule is one `orb %cl, %al`**: a prefix matches only
    at a component boundary (`'\0'` or `'/'`). That single instruction is what
    separates `/root` from `/rootkit`.
  - `sizeof(struct pg_rule) == 16` and `.verdict` at offset 8 are *readable off
    the code*: indexing `rules[i]` is `shlq $4` (×16), reading the verdict is
    `movl 8(...)`. **Struct layout is the addressing mode.**
  - The optimizer **inlined** `pg_path_has_prefix` into `pg_policy_lookup`,
    producing the nested loop you can trace at `.LBB1_*`.
- [`asm/demo.O0.s`](asm/demo.O0.s) — naive `-O0`: every variable spilled to the
  stack and reloaded. Easiest to map statement-by-statement.
- [`asm/demo.s`](asm/demo.s) — `-O1`, the annotated baseline.
- [`asm/demo.O2.s`](asm/demo.O2.s) — `-O2`, the aggressive form.

Regenerate with `make asm` (works on any host; clang cross-targets Linux).

## Going further (the `Stretch:`)

- **A third hook + real state.** Add `task_alloc`/`cred_prepare` and allocate a
  per-task or per-cred **security blob** (declare `lsm_blob_sizes`, use
  `current->security` offsets) so the policy can depend on process lineage, not
  just uid — this is how SELinux/AppArmor carry a label.
- **Move the policy to userspace.** Replace the compile-time tables with a
  `securityfs` file the policy is written to at boot (see Landlock and how it
  builds rulesets), so posture is data, not code.
- **What production does.** Real MAC (SELinux, AppArmor, Smack) labels every
  inode and task, checks a compiled policy on *many* hooks (sockets, IPC,
  signals, mounts, ptrace), and integrates with the audit subsystem. **Landlock**
  is the closest modern analog to this project's spirit: an *unprivileged*,
  path-based, stackable LSM — read its source next.

## References

- `security/security.c` — `security_add_hooks()` (note the `__init`),
  `call_int_hook()`, and the stacking/ordering logic.
- `include/linux/lsm_hooks.h` — `LSM_HOOK_INIT`, `security_hook_list`,
  `DEFINE_LSM`, `struct lsm_id`; `include/uapi/linux/lsm.h` — the `LSM_ID_*` list.
- `security/landlock/` — a small, modern, stackable, path-oriented LSM to read
  in full; `security/yama/yama_lsm.c` — an even smaller single-purpose LSM.
- `Documentation/security/lsm.rst` and `lsm-development.rst`.
- `fs/open.c` (`do_dentry_open`) and `fs/exec.c` (`security_bprm_check`) — the
  exact call sites of the two hooks used here. `fs/d_path.c` — `d_path()`.
