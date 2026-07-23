# LKM rootkit & detector 🟥

**What it is.** A matched pair of Linux kernel modules, built for *learning to
detect* kernel rootkits. The **red** module (`rootkit.c`) hides files and a PID
by hooking the `getdents64` syscall with **ftrace**, and can optionally unlink
itself from the module list (a **DKOM** self-hide). The **blue** module
(`detector.c`) is the one this project is really about: it snapshots a
known-good baseline of kernel state at load and, on demand, diffs the live
kernel against it to flag syscall-table tampering and ftrace hooks. This is a
🟥 (hard) topic shipped as an honest **teaching core** — see *Going further* for
what a production tool adds.

> **Safety.** Offensive code here is educational and belongs on a **throwaway
> QEMU/KVM VM you can reset** — never a machine or data you care about. The lab's
> rule is that every red-team tool ships with its blue-team detector, because
> knowing the *exact artifact* a technique leaves behind is what lets you catch
> it. Load the **detector first**.

## What you'll learn

- **ftrace function hooking**: how the compiler's `__fentry__`/`-pg` NOP at the
  top of every kernel function becomes a hook via `ftrace_ops` +
  `FTRACE_OPS_FL_IPMODIFY`, and why rewriting `regs->ip` redirects execution.
- **The post-5.7 kallsyms bootstrap**: `kallsyms_lookup_name` is no longer
  exported, so both modules recover it with a one-shot **kprobe** — the same
  trick used offensively and defensively.
- **Syscall-table integrity**: what `sys_call_table` is, why it lives in
  read-only `.rodata`, and how to fingerprint and bounds-check it.
- **DKOM** (Direct Kernel Object Manipulation): hiding by unlinking an object
  (here, the module) from a kernel list, and the **cross-view** idea that
  defeats it — compare a list DKOM edited against a source it didn't.
- **The user/kernel boundary**: `copy_from_user`/`copy_to_user` invariants while
  editing a `linux_dirent64` buffer, and why you never walk a user pointer
  directly (TOCTOU, faults).
- **Reading x86 by hand**: decoding a `call rel32` (opcode `0xe8`) to compute an
  ftrace trampoline target and decide whether it left kernel `.text`.

## Build & run (Linux, in a throwaway VM)

These are **kernel modules**: they must be built against real kernel headers on
Linux and cannot be built or loaded on this Windows host. Use a disposable VM
(kernel 5.11+ recommended; the ftrace callback ABI differs on older kernels and
is handled with a `#if`).

```bash
# Prereqs on the guest (Debian/Ubuntu example):
sudo apt install build-essential linux-headers-$(uname -r)

# Build both modules:
make                       # -> rootkit.ko and detector.ko

# 1) Load the DETECTOR first, on the clean kernel, and read its report:
sudo insmod detector.ko
cat /proc/rkdetect         # every check should say [ OK ]

# 2) Now load the ROOTKIT and watch the detector catch the ftrace hook:
sudo insmod rootkit.ko hide_prefix=rk_ hide_pid=$$
touch rk_secret            # a file whose name starts with rk_
ls                         # rk_secret is INVISIBLE to ls (getdents64 filtered)
cat /proc/rkdetect         # [C1]/[C2] now WARN about __x64_sys_getdents64

# 3) Clean up (unload rootkit first, then detector):
sudo rmmod rootkit
sudo rmmod detector
```

Module parameters for `rootkit.ko`:

| param         | default | meaning                                                   |
|---------------|---------|-----------------------------------------------------------|
| `hide_prefix` | `rk_`   | hide directory entries whose name starts with this        |
| `hide_pid`    | (none)  | hide `/proc/<pid>` so the process vanishes from `ps`/`ls` |
| `hide_self`   | `0`     | DKOM: unlink from `lsmod`. **You cannot `rmmod` after** — reboot the VM. |

Regenerate the teaching assembly on any host (no kernel needed):

```bash
make asm                   # writes asm/demo.{O0.s,s,O2.s} from asm/demo.c
```

## How it works (file by file)

- **`ftrace_helper.h`** — a heavily-commented, reusable ftrace-hook installer.
  It resolves a symbol's address, points an `ftrace_ops` at it with
  `SAVE_REGS | IPMODIFY`, and in the callback rewrites `regs->ip` to the
  replacement — *only* when the call did not originate inside our module (the
  anti-recursion invariant). Also holds the kprobe-based
  `kallsyms_lookup_name` bootstrap shared in spirit by both modules.

- **`rootkit.c`** (red) — installs one hook on `__x64_sys_getdents64`. The
  replacement calls the real syscall, copies the returned `linux_dirent64`
  buffer into the kernel, splices out records whose name matches `hide_prefix`
  or equals `hide_pid` (by growing the previous record's `d_reclen` so
  `readdir()` steps over them), and copies the edited buffer back. Optionally
  performs a DKOM self-hide via `list_del(&THIS_MODULE->list)`.

- **`detector.c`** (blue) — captures a baseline at load, then on each read of
  `/proc/rkdetect` runs four checks:
  - **[A]** re-hash `sys_call_table` (FNV-1a) and compare to the baseline; on a
    mismatch, `region_first_diff` reports the first changed `__NR_*` entry.
  - **[B]** verify every table entry points into `[_stext, _etext)`; flag any
    that points into a module (a classic table hook, catchable even if it
    predates us).
  - **[C]** for a watch-list of commonly hooked functions, compare each
    prologue's fingerprint to load time **(C1)** and decode the fentry site to
    see if it was redirected to an out-of-`.text` trampoline **(C2)** — this is
    what fires when `rootkit.ko` is present.
  - **[D]** print the scheduler's authoritative PID list (`for_each_process`)
    for a cross-view diff against `ls /proc`.

- **`asm/demo.c`** — the detector's pure-logic core (`fnv1a64`,
  `region_first_diff`, `table_fingerprint`) extracted so it can be compiled to
  standalone assembly. See below.

**Which check catches which attack.** The shipped rootkit uses *ftrace*, which
patches the function body, **not** the syscall table — so on this demo you'll
see checks **[C1]/[C2]** fire while **[A]/[B]** stay green. That contrast is the
lesson: table-integrity and text-integrity catch *different* techniques, and a
detector needs both.

## Assembly notes

Kernel C is **not** standalone-compilable (it needs `<linux/*>`, the kernel
config, and Kbuild), so there is no honest way to run `clang -S detector.c`.
Following the lab convention, the detector's most instructive **pure-logic
helper** — the memory-region **checksum + diff** used by check [A] — is lifted
into a self-contained `asm/demo.c` that declares its own types and includes no
headers. The exact commands in the spec were run to produce:

- [`asm/demo.O0.s`](asm/demo.O0.s) — `-O0`, the literal statement-by-statement
  mapping (every value spilled to the stack).
- [`asm/demo.s`](asm/demo.s) — `-O1`, the annotated baseline.
- [`asm/demo.O2.s`](asm/demo.O2.s) — `-O2`, unrolling/strength-reduction.
- [`asm/demo.annotated.s`](asm/demo.annotated.s) — the `-O1` output with a
  comment on essentially every instruction plus the SysV AMD64 ABI header.

What the annotation highlights: the FNV-1a loop is a textbook
load-byte → `xor` → `imul` **multiply-accumulate** with the prime hoisted out of
the loop and the hash never spilling; `region_first_diff` **preloads `-1`** so
its common "identical" answer is free and its early `jne` makes cost
`O(first difference)`, not `O(len)` — the property the detector relies on to
localize a single hooked entry fast; and `table_fingerprint` shows the optimizer
turn `count * sizeof(u64)` into `shlq $3` and **inline** `fnv1a64` so no `call`
survives. This same hash+diff, in kernel form, is what `detector.c` runs over
`sys_call_table`.

## Going further (the `Stretch:` goal)

- **Detect pre-existing ftrace hooks robustly.** Checks [A] and [C1] are
  baseline-relative — they only see tampering that happens *after* the detector
  loads. A production detector walks ftrace's own registered `ftrace_ops` list
  and flags any `IPMODIFY` op on a sensitive function, and validates function
  bytes against **on-disk** symbol hashes so a rootkit loaded first can't poison
  the baseline. Add that and the "load detector first" caveat disappears.
- **Automate the process cross-view.** Instead of asking the human to compare
  `ls /proc`, open `/proc` in-kernel, drive the *hooked* `getdents64` path, and
  diff its numeric entries against `for_each_process` to name hidden PIDs
  directly.
- **Hook `getdents` (legacy) too**, and hide network connections by hooking
  `tcp4_seq_show` — then extend the detector's watch-list to match.
- **What production does.** Tools like *Tripwire*/*AIDE* (offline integrity),
  kernel **lockdown**, `CONFIG_STRICT_KERNEL_RWX`, IMA/EVM measured boot, and
  eBPF-based runtime monitors (Falco, Tetragon) attack this from prevention and
  telemetry angles, not just point-in-time diffing.

## References

- Linux `kernel/trace/ftrace.c`; `Documentation/trace/ftrace-uses.rst` —
  `ftrace_ops`, `register_ftrace_function`, `FTRACE_OPS_FL_IPMODIFY`.
- `kernel/kprobes.c`, `man 2 kprobe` — the `kallsyms_lookup_name` bootstrap.
- `fs/readdir.c` — the real `getdents64` and `struct linux_dirent64` layout.
- `arch/x86/entry/` and `arch/x86/include/asm/syscall_wrapper.h` — the
  `__x64_sys_*(struct pt_regs *)` calling convention this hook depends on.
- The FNV hash: Fowler–Noll–Vo, `isthe.com/chongo/tech/comp/fnv/`.
- Background reading (the technique this distills): Harvey Phillips'
  "Linux Rootkits" ftrace-hooking series and TheXcellerator's `ftrace_helper.h`.
