# Syscall / function tracer with kprobes & ftrace 🟧

**What it is.** Two small kernel modules that hook a live kernel function
(`do_sys_openat2`, the worker behind `open`/`openat`/`openat2`) **without
recompiling or rebooting the kernel**, and observe it two different ways:

- `kprobe_tracer.ko` plants a **kprobe** to log each call's arguments on entry
  and a **kretprobe** to measure entry→return **latency**, bucketed into a
  lock-free **log2 histogram** and dumped to `dmesg` on unload.
- `ftrace_tracer.ko` does the same entry hook the cheaper way — via an
  **`ftrace_ops`** attached to the function's **fentry** call site (the same
  mechanism BPF `fentry/` programs and live-patching use).

This is a **teaching core**, and an honest one: it demonstrates the four hook
mechanisms (`register_kprobe`, `register_kretprobe`, `ftrace_ops`, fentry) end
to end with correct concurrency and context discipline. It is not a full tracing
framework — there is no per-PID filtering, no ring buffer to userspace, and no
stack traces. The "going further" section says exactly what a production tracer
(perf/BCC/bpftrace) adds.

> **Platform: Linux only, and run it in a throwaway VM.** These are kernel
> modules; they build against real kernel headers and load into a running
> kernel. A bug here panics the machine. Build and load them inside a **QEMU/KVM
> guest**, never on your host, and never on this Windows box (which cannot build
> them at all). The assembly deliverable *is* host-portable — see below.

## What you'll learn

- **kprobes**: how `register_kprobe()` patches an `int3` (0xCC) breakpoint into
  live kernel text, and how the `.pre_handler` runs in atomic, non-preemptible
  context — so it must never sleep.
- **kretprobes**: how the return address is hijacked with a trampoline, and how
  a **per-instance scratch buffer** (`.data_size`, `ri->data`) matches an entry
  timestamp to the correct return under recursion and SMP concurrency. What
  `.maxactive` and `krp.nmissed` mean.
- **ftrace_ops / fentry**: how `-mfentry` + `CONFIG_DYNAMIC_FTRACE` turn every
  function prologue into a patchable `nop`, and how `register_ftrace_function()`
  swaps it for a call to your callback — a direct call, not a trap. The
  **recursion guard** (`ftrace_test_recursion_trylock`) every real callback needs.
- **Reading userspace safely from probe context**: why `strncpy_from_user_nofault`
  (not `strncpy_from_user`) is mandatory when you can't afford to fault/sleep.
- **Lock-free counting**: why the histogram uses `atomic64_t` (lowered to
  `lock xadd`) instead of a spinlock on the hot path.
- **The log2 bucketing math** and, in the assembly, how a bit-counting loop
  becomes a single `bsr` instruction (the kernel's `fls64()`).

## Build & run (Linux, inside a VM)

```bash
# In the guest, with kernel headers installed (linux-headers-$(uname -r)):
make                     # -> kprobe_tracer.ko, ftrace_tracer.ko
                         # runs: make -C /lib/modules/$(uname -r)/build M=$(PWD) modules

sudo insmod kprobe_tracer.ko          # optional: symbol=vfs_read to retarget
sudo insmod ftrace_tracer.ko
cat /etc/hostname > /dev/null          # trigger some open()s
ls -l /usr >/dev/null                  # ...and some more

sudo dmesg | tail -n 30                # see the arg logs + any SLOW lines
sudo rmmod ftrace_tracer kprobe_tracer # unload
sudo dmesg | tail -n 30                # kprobe_tracer prints the latency histogram here
```

`make load` / `make unload` wrap the insmod/rmmod pair. Example histogram (the
counts are illustrative):

```
kprobe_tracer latency: 128 samples
      nsec range            : count
  [           256,            511] : 5
  [           512,           1023] : 61
  [          1024,           2047] : 55
  [       65536,          131071] : 7          <- cold-cache / disk-backed opens
```

Regenerate the teaching assembly on **any** OS (no kernel tree needed):

```bash
make asm                 # writes asm/demo.{O0.s,s,O2.s}
```

## How it works (file by file)

- **`latency_hist.h`** — the shared, lock-free log2 histogram. `lat_bucket(v)`
  returns the number of significant bits of `v` (= `floor(log2 v)+1`), so each
  bucket covers a power-of-two-wide band and ~64 slots span the whole latency
  range. Counters are `atomic64_t`; `hist_record()` does one atomic increment
  (safe from any CPU in atomic context), and `hist_dump()` pretty-prints on
  teardown. This header holds the routine that the assembly deliverable extracts.

- **`kprobe_tracer.c`** — registers **both** a `struct kprobe` (whose
  `.pre_handler` reads the args out of `pt_regs` and logs the filename via a
  *nofault* user copy) **and** a `struct kretprobe` (whose `.entry_handler`
  stamps `ktime_get()` into per-call scratch and whose `.handler` computes the
  ns delta at return and feeds the histogram). Registration is unwound in
  reverse on any failure — a half-installed probe left in kernel text is an
  instant crash. `symbol=` is a module parameter, so you can retarget it without
  recompiling.

- **`ftrace_tracer.c`** — attaches a `struct ftrace_ops` (with
  `FTRACE_OPS_FL_SAVE_REGS` so the callback gets a full `pt_regs`) to the same
  symbol via `ftrace_set_filter()` + `register_ftrace_function()`. The callback
  takes the mandatory recursion guard, logs the caller (`%pS` on `parent_ip`)
  and the filename, and counts hits. It is **entry-only** by design — a bare
  `ftrace_ops` has no return hook — which is exactly what makes the contrast
  with the kretprobe instructive.

- **`Makefile`** — the standard out-of-tree Kbuild wrapper: `obj-m` names the
  two modules, and the default rule re-invokes make inside
  `/lib/modules/$(uname -r)/build`. A separate `make asm` target regenerates the
  didactic assembly with clang's Linux cross-target.

## Assembly notes

Kernel C can't be compiled standalone on this (or any) host — it needs the
kernel's headers and config — so it produces no meaningful standalone assembly.
Per the repo convention, the project's most instructive **pure-logic** routine
is extracted into **`asm/demo.c`** (self-contained, its own `u32`/`u64`
typedefs, zero includes): the log2 histogram bucketing. The exact commands in
`CONVENTIONS.md` generate `asm/demo.{O0.s,s,O2.s}`, and **`asm/demo.annotated.s`**
annotates the `-O1` output line by line.

The headline lesson is the loop-to-instruction collapse:

- **`asm/demo.O0.s`** shows `lat_bucket` as the real C loop (`.LBB0_1:` … `shr`
  … `jmp`), and `hist_record` making an actual `call lat_bucket`.
- **`asm/demo.s` (-O1)** — already gone: clang recognizes "count significant
  bits" and emits a single **`bsr`** (bit-scan-reverse, exactly what `fls64()`
  compiles to), turns every `if` into a branchless **`cmov`**, and **inlines**
  `lat_bucket` into `hist_record`. It even **deletes** the `if (b==0)` branch in
  `bucket_high` after proving the general formula `(1<<b)-1` already yields 0
  there. The annotation also flags the two hardware corner cases the code steps
  around: `bsr` is undefined on input 0, and `shl`'s count is masked mod 64.
- **`asm/demo.O2.s`** is the same, minus the frame-pointer prologue.

## Going further (the `Stretch:` from the list)

**Rewrite it as an eBPF / libbpf CO-RE program.** The kprobe+kretprobe pattern
here maps almost one-to-one onto BPF: `SEC("kprobe/do_sys_openat2")` and
`SEC("kretprobe/...")` (or the faster `SEC("fentry/")` / `SEC("fexit/")`), a
`BPF_MAP_TYPE_HASH` keyed by pid_tgid to carry the entry timestamp instead of
`ri->data`, and `bpf_map_lookup_elem` on a per-bucket array — or just
`bpf_printk`/a histogram map read from userspace. **CO-RE** (Compile Once, Run
Everywhere) uses BTF + `bpf_core_read` relocations so one compiled object loads
across kernels whose struct layouts differ — solving the portability problem
these modules have (they must be rebuilt per kernel). That is precisely how
`bcc`/`libbpf-tools` `opensnoop` and `funclatency` work; read those next.

Other extensions a production tracer adds: per-PID/uid filtering, a
`BPF_MAP_TYPE_RINGBUF` (or perf buffer) to stream events to userspace instead of
`dmesg`, stack traces (`bpf_get_stackid`), and function-graph-style latency via
`fexit` rather than a trampoline.

## References

- `Documentation/trace/kprobes.rst` and `samples/kprobes/` in the kernel tree —
  the canonical `register_kprobe`/`register_kretprobe` examples.
- `Documentation/trace/ftrace.rst`, `Documentation/trace/fprobe.rst`, and
  `samples/ftrace/ftrace-ops.c` / `ftrace-direct*.c`.
- `kernel/kprobes.c`, `kernel/trace/ftrace.c` — the implementations.
- `man 2 openat2`, `fs/open.c` (`do_sys_openat2`).
- libbpf-tools: `opensnoop.bpf.c`, `funclatency.bpf.c` (the eBPF version of this).
