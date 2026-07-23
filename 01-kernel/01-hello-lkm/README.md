# Hello-world LKM, done properly 🟩

**What it is.** The smallest Loadable Kernel Module worth reading — a "hello
world" that nonetheless exercises the full scaffolding every real Linux driver
sits on: `module_init`/`module_exit`, the `MODULE_*` metadata contract, tunable
parameters exposed under `/sys/module/`, a **runtime-reactive** parameter driven
by a `kernel_param_ops` callback, and logging at the correct `printk` severity
levels. It is a kernel module, so it runs in ring 0 with no libc and no memory
protection — build and load it **inside a Linux VM**, never on the host.

## What you'll learn

- **`module_init` / `module_exit`** — how code enters the kernel at `insmod` and
  leaves at `rmmod`, and what the `__init` / `__exit` section attributes buy you
  (the `.init.text` section is *freed* after load; `__exit` is dropped for
  built-in modules).
- **`MODULE_LICENSE`** as a load-bearing declaration: it sets (or avoids) the
  kernel *taint* flag and gates access to `EXPORT_SYMBOL_GPL` symbols.
- **`module_param`** (a `charp` string) vs **`module_param_cb`** (an int with a
  custom `.set`/`.get`): how a C variable becomes a file under
  `/sys/module/hello_lkm/parameters/`, and how to **validate and react** to a
  runtime write instead of letting the value change silently.
- **`printk` levels** — why `pr_info` (KERN_INFO, sev 6) and `pr_err`
  (KERN_ERR, sev 3) are different, and how they land in the kernel ring buffer
  you read with `dmesg`.
- Safe kernel string handling: **`kstrtoint`** (not `atoi`) and **`sysfs_emit`**
  (bounds-guaranteed) instead of raw `sprintf`.
- The `insmod` / `rmmod` / `modprobe` / `modinfo` toolchain and the `obj-m`
  Kbuild build.

## Build & run (Linux only — ideally a QEMU VM)

Kernel modules must be built against the running kernel's headers and can crash
the machine on a bug, so do this in a disposable Linux VM. You need the matching
`linux-headers-$(uname -r)` package and `make`, `clang`/`gcc`, `binutils`.

```bash
# 1. Build the module (produces hello_lkm.ko)
make

# 2. Inspect its metadata WITHOUT loading it
make info                 # == modinfo hello_lkm.ko  -> license, params, descs

# 3. Load it with parameters, then read the kernel log
sudo insmod hello_lkm.ko who="lab" count=3
dmesg | tail            # -> "Hello, lab! (1/3)", "(2/3)", "(3/3)"

# 4. Tune it AT RUNTIME through sysfs — this fires the callback
cat  /sys/module/hello_lkm/parameters/count      # -> 3
echo 5 | sudo tee /sys/module/hello_lkm/parameters/count
dmesg | tail            # -> "count updated to 5; greeting lab again:" + 5 lines

# 5. Watch validation reject bad input (the echo itself FAILS)
echo 99 | sudo tee /sys/module/hello_lkm/parameters/count   # -> write error
dmesg | tail            # -> "count=99 out of range [1,10]"

# 6. Unload
sudo rmmod hello_lkm
dmesg | tail            # -> "unloaded. goodbye, lab."
```

If a bad load taints or wedges the VM, just reboot it — that's why we use a VM.

## How it works (file by file)

- **`hello_lkm.c`** — the module. Bottom-up:
  - `clamp_count()` / `fnv1a32()` — two **pure-logic helpers** with no kernel
    calls (the range check and a greeting hash-tag). They are the code extracted
    into `asm/demo.c` for the assembly deliverable.
  - `who` (`charp`) via **`module_param`** — the classic string parameter, set
    at load time (`who="lab"`), exposed read/write at `0644`.
  - `count` (`int`) via **`module_param_cb`** — the "done properly" part. A
    `struct kernel_param_ops` supplies `count_set` (parse with `kstrtoint`,
    validate against `[1,10]`, store, then **re-emit the greeting**) and
    `count_get` (render with `sysfs_emit`). This is what makes a write to the
    sysfs file *react* rather than silently change a variable.
  - `hello_lkm_init()` (`__init`) — validates the load-time config, tags the run
    with `fnv1a32(who)`, and prints the greeting `count` times. Returns `0` for
    success or `-EINVAL` to refuse loading — a driver must never come up
    misconfigured.
  - `hello_lkm_exit()` (`__exit`) — the teardown hook; here it just says goodbye
    (we allocated nothing to free).
- **`Makefile`** — the `obj-m += hello_lkm.o` Kbuild recipe plus a recursive
  `make -C $(KDIR) M=$(PWD) modules` build, and `load`/`unload`/`info`
  convenience targets. `make asm` regenerates the teaching assembly.
- **`asm/`** — the assembly deliverable (see next section).

## Assembly notes

Kernel C is **not standalone-compilable** to assembly: `hello_lkm.c` pulls in
`<linux/module.h>` and a forest of headers that only exist inside a configured
kernel build tree, so `clang -S hello_lkm.c` cannot even parse it. Following the
lab convention, the module's dependency-free core is lifted verbatim into
**`asm/demo.c`** (it declares its own types and includes nothing), which clang
*can* compile to real x86-64 Linux assembly. The in-kernel machine code for the
module's `clamp_count`/`fnv1a32` has the identical shape.

Regenerate with `make asm`. The committed files:

- [`asm/demo.O0.s`](asm/demo.O0.s) — `-O0`, every value spilled to the stack;
  the most literal C-statement-to-instruction mapping.
- [`asm/demo.s`](asm/demo.s) — `-O1`, the annotated baseline.
- [`asm/demo.O2.s`](asm/demo.O2.s) — `-O2`, for comparison.
- [`asm/demo.annotated.s`](asm/demo.annotated.s) — the hand-written, per-line
  walkthrough with the full SysV AMD64 ABI header block.

The annotation's two lessons: **`clamp_count` compiles branchless** — the two
`if`s become `cmp` + conditional-move (`cmovl`/`cmovge`), no jumps to
mispredict, and the signed suffixes matter (at `-O2` clang even switches to an
*unsigned* compare to fold both bounds checks — compare `demo.O2.s`).
**`fnv1a32`** peels its first byte so the empty-string case returns *without ever
setting up a stack frame*, and its `h *= 16777619` stays a single `imul` rather
than a shift/add chain. `main` shows the optimizer constant-folding a hash of a
literal string all the way down to `return 22`.

## Going further (the `Stretch:` from the list)

The stretch goal — **a runtime-tunable parameter via `module_param_cb` whose
callback reacts to the change** — is implemented in the core: writing
`/sys/module/hello_lkm/parameters/count` runs `count_set`, which validates the
value and re-prints the greeting. What a *production* driver adds on top:

- **Locking.** `count_set` can run concurrently with a reader or with module
  exit. A real driver protects shared parameter state with a mutex/spinlock (or
  `READ_ONCE`/`WRITE_ONCE` for lock-free scalars) so a torn read can't happen.
- **`sysfs` attributes / device model.** Beyond `module_param`, drivers expose
  richer state through `struct kobject`/`device_attribute` groups with per-file
  show/store handlers — the same callback idea, integrated with the device tree.
- **Notifier chains / work queues.** Reacting to a change often means scheduling
  deferred work rather than doing it inline in the sysfs write path.

## References

- `Documentation/admin-guide/README` and `Documentation/kbuild/modules.rst` in
  the Linux source — the canonical out-of-tree module build.
- `include/linux/moduleparam.h` — the real `module_param`, `module_param_cb`,
  and `struct kernel_param_ops` definitions.
- `include/linux/printk.h` and `include/linux/kern_levels.h` — the `pr_*`
  helpers and the KERN_* severity constants.
- `man 8 insmod`, `man 8 rmmod`, `man 8 modprobe`, `man 8 modinfo`.
- Corbet, Rubini & Kroah-Hartman, *Linux Device Drivers* (LDD3), ch. 2 — the
  classic walkthrough of exactly this scaffolding.
