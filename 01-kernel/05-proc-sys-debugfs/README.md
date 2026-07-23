# procfs / sysfs / debugfs interfaces 🟧

**What it is.** One tiny Linux kernel module that exposes the *same* piece of
driver state through **all three** kernel-to-userspace filesystems at once, so
you can compare them side by side and learn *when each is the right choice*:

| Interface | Path | Mechanism | Social contract |
|-----------|------|-----------|-----------------|
| **procfs** | `/proc/psd_demo` | `proc_create` + a `seq_file` iterator | Legacy, human-oriented, historically process-centric. A `seq_file` makes reads of any size safe. |
| **sysfs** | `/sys/kernel/psd_demo/` | `kobject` + `sysfs_create_group` show/store attrs | **Stable ABI.** One value per file. Tools depend on it; you may not break it. |
| **debugfs** | `/sys/kernel/debug/psd_demo/` | `debugfs_create_u32` / `_atomic_t` / `_file` | **No promises.** Raw internals for kernel devs; may change or vanish any release. |

This is a teaching module: it is written to be *read*. Roughly half of the C is
comments explaining the *why* — every locking decision, every ABI rule, every
`copy`/`store` invariant. Difficulty **🟧** because it touches four subsystems
(proc, seq_file, kobject/sysfs, debugfs) and the correctness lives in the error
unwinding and the locking, not in the line count.

> **Platform:** this is real kernel code. It builds and runs **only on Linux**
> (do it in a **QEMU VM** or WSL2 with kernel headers — never on the Windows
> host you may be editing from). The `asm/` deliverable is host-portable.

## What you'll learn

- **`seq_file`** the right way: the `start`/`next`/`stop`/`show` iterator
  contract, the `SEQ_START_TOKEN` header idiom (as `/proc/slabinfo` uses), and
  *why* it makes arbitrarily large reads and partial reads correct where a naive
  `->read()` is a bug factory.
- **`proc_create` + `struct proc_ops`** — and why proc moved off
  `file_operations` in kernel 5.6.
- **kobjects and sysfs**: `kobject_create_and_add`, `__ATTR`/`__ATTR_RO`/
  `__ATTR_WO`, attribute *groups*, the `show`/`store` signatures, `sysfs_emit`,
  and the one-value-per-file / PAGE_SIZE / stable-ABI rules.
- **debugfs**: how `debugfs_create_u32`/`_atomic_t` wire a variable straight to
  a file with zero boilerplate, why you deliberately **don't** check their return
  values, and why that convenience is exactly the "no ABI" trade-off.
- **Kernel concurrency in miniature**: an `atomic_t` counter vs. a `spinlock`
  guarding multi-word state, `spin_lock_irqsave`, and the rule that a `seq_file`
  holding a spinlock across `->start..->stop` must never sleep in `->show`.
- **Lifecycle discipline**: register in order, tear down in *reverse* order, and
  unwind correctly on every error path so `rmmod` can never leave a dangling
  file pointing at freed module text.

## Build & run (Linux / QEMU VM only)

```bash
# Prereqs: matching kernel headers, e.g. on Debian/Ubuntu:
#   sudo apt install linux-headers-$(uname -r) build-essential

make                       # builds proc_sys_debugfs.ko via Kbuild
sudo insmod proc_sys_debugfs.ko    # (or: make load)
dmesg | tail -1            # "psd_demo: loaded — see /proc/... /sys/kernel/... debug/..."
```

Now watch all three views of the same state:

```bash
# --- procfs: the formatted, possibly-large human report (seq_file) ---
cat /proc/psd_demo

# --- sysfs: one value per file, the stable ABI ---
ls -l /sys/kernel/psd_demo/
cat  /sys/kernel/psd_demo/hits          # read-only counter
echo 250 | sudo tee /sys/kernel/psd_demo/threshold   # rw knob
echo hello | sudo tee /sys/kernel/psd_demo/label     # rw string
echo 1 | sudo tee /sys/kernel/psd_demo/trigger       # write-only: records an event

# --- debugfs: raw internals, no ABI (root only) ---
sudo cat  /sys/kernel/debug/psd_demo/state           # the raw dump
sudo cat  /sys/kernel/debug/psd_demo/threshold       # SAME u32 as sysfs above
```

Trigger a few events, then `cat /proc/psd_demo` again to see the event table and
the `hits`/`dropped` counters move together across all three interfaces.

```bash
sudo rmmod proc_sys_debugfs        # (or: make unload)
```

**Regenerate the teaching assembly** (works on any host, including Windows,
because clang cross-targets Linux):

```bash
make asm       # regenerates asm/demo.{O0.s,s,O2.s}; annotated.s is hand-authored
```

## How it works (file by file)

- **`proc_sys_debugfs.c`** — the whole module, one file, built in three clearly
  marked sections:
  - *Driver state* (`struct psd_state`, single instance `psd`): an
    `atomic_t hits`, a `u32 threshold` shared by sysfs **and** debugfs, a
    saturating event ring (`ring`/`ring_count`/`dropped`), and a user-settable
    `label`. A `spinlock_t` guards exactly the multi-word fields; the counter is
    lock-free.
  - *Interface 1 — procfs.* A full `seq_operations` iterator over the event ring.
    `->start` takes the lock and returns `SEQ_START_TOKEN` to print a header,
    then hands out `&ring[i]`; `->show` formats one row; `->stop` drops the lock.
    Wired to `/proc/psd_demo` via `proc_create` + `struct proc_ops`
    (`seq_read`/`seq_lseek`/`seq_release` do the heavy lifting).
  - *Interface 2 — sysfs.* Four `kobj_attribute`s — `hits` (ro), `threshold`
    (rw), `label` (rw), `trigger` (wo) — collected into an `attribute_group` and
    attached to a kobject under `/sys/kernel`. Reads use `sysfs_emit`; writes
    parse with `kstrtou32` and return the byte count.
  - *Interface 3 — debugfs.* `threshold`, `hits`, `dropped` wired straight to
    their variables by address, plus a `state` file using `single_open` for the
    raw dump. Return values intentionally unchecked (that's the debugfs idiom).
  - *Lifecycle.* `psd_init` creates the three interfaces in order with a
    reverse-order error unwind; `psd_exit` removes them in strict reverse order.
- **`Makefile`** — Kbuild: `obj-m := proc_sys_debugfs.o`, plus a `make asm`
  target and `load`/`unload` helpers.
- **`asm/demo.c`** — the standalone, host-compilable extract (see below).
- **`asm/demo.{O0.s,s,O2.s}`** — genuine clang output at three optimization
  levels. **`asm/demo.annotated.s`** — the hand-commented walkthrough.

## Assembly notes

Kernel C **cannot** be compiled to assembly on this host — it needs the kernel
headers and build system that only exist inside a Linux tree. So, per the repo
convention, `asm/demo.c` extracts the module's most instructive **pure-logic**
core: the **number-to-string formatting** that sits underneath every
`seq_printf("%llu", …)` and `sysfs_emit("%u", …)` the module performs. It has no
system headers and declares its own types, so any `clang` turns it into real
assembly.

The three files are genuine clang output (`make asm` regenerates them). The star
lesson is **division by 10**:

- `asm/demo.O0.s` — literal, easy to trace: `v / 10` is a real `divq`.
- `asm/demo.s` (**-O1**, the annotated baseline) — the compiler replaces the
  division with a **multiply by the magic reciprocal `0xCCCCCCCCCCCCCCCD`** plus
  `shrq $3` (the classic "division by an invariant integer via multiplication"
  trick), and computes `v % 10` as `v - (v/10)*10` using two `lea`s. No `div`
  survives.
- `asm/demo.O2.s` — keeps the magic multiply **and** vectorizes the digit-reversal
  loop with SSE (`punpcklbw`/`pshufd`/`packuswb`).

`asm/demo.annotated.s` comments essentially every instruction, normalizes clang's
`.LBB0_*` labels to meaningful names, and opens with the SysV AMD64 ABI contract
(arg regs `rdi, rsi, rdx, rcx, r8, r9`; return in `rax`; callee-saved
`rbx, rbp, r12–r15`) plus the prologue/epilogue and the inlining that folds
`u64_to_dec` and `format_field` into `render_line`.

## Going further (the `Stretch:` from the list)

- **Make the ring truly circular** (keep the *most recent* N events with a head
  index) and update the `seq_file` iterator to walk oldest→newest with modular
  arithmetic — the realistic form this teaching module deliberately simplified.
- **Per-instance state.** Turn the single global `psd` into a kmalloc'd object
  per device, reached via `seq_file->private`, the kobject `container_of`, and
  debugfs's `data` pointer — the pattern every real driver uses.
- **Notify on change.** Call `sysfs_notify(psd_kobj, NULL, "hits")` from the
  trigger path so a userspace `poll()` on the sysfs file wakes up — how `udev`
  and monitoring tools react without busy-looping.
- **Document the ABI.** Write a `Documentation/ABI/testing/sysfs-kernel-psd_demo`
  stanza for the sysfs files. That file *is* the promise that makes sysfs stable.
- **What production does:** real drivers rarely touch `/proc` for new state
  (it's discouraged); they expose stable knobs via sysfs (often through the
  driver core's `DEVICE_ATTR` and `ATTRIBUTE_GROUPS` macros under `struct
  device`), and dump firehose diagnostics via debugfs. Tracing-heavy subsystems
  add `tracefs`/`ftrace` events on top.

## References

- `Documentation/filesystems/seq_file.rst` — the iterator contract, verbatim.
- `Documentation/filesystems/sysfs.rst` and `Documentation/kobject.txt` — the
  one-value-per-file rule and kobject lifetime/refcounting.
- `Documentation/filesystems/debugfs.rst` — the "no stable ABI" statement.
- Kernel source: `fs/proc/`, `fs/seq_file.c`, `fs/sysfs/`, `fs/debugfs/`, and
  `lib/vsprintf.c` (`number()` / `put_dec()` — the real number-formatting core
  that `asm/demo.c` mirrors).
- `man 5 proc`, `man 7 sysfs`.
