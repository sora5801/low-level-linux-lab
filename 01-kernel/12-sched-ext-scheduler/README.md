# sched-ext scheduler 🟥

**What it is.** A working CPU scheduler for Linux — the code that decides which
task runs on which core, and for how long — written not as kernel C but as a
**sched_ext (SCX) BPF program** that the running kernel loads, verifies, and
puts in charge of the whole machine. It ships two selectable policies: a **strict
global FIFO** and a **weighted virtual-time fair-share** scheduler (a tiny cousin
of EEVDF). `scx_fifo.bpf.c` is the kernel half (the scheduling logic, in BPF);
`scx_fifo.c` is the userspace half that loads and attaches it. This is a 🟥
project because it touches the scheduler core, the BPF verifier, and run-queue
mechanics — but sched_ext makes it *safe* to experiment: a bad policy is ejected
by a watchdog and the kernel falls back to EEVDF without a reboot.

**Teaching-core honesty.** This is a *correct, complete, loadable* scheduler for
the two policies above, demonstrating `select_cpu` / `enqueue` / `dispatch`, the
run queues (DSQs), and the vtime arithmetic end to end. It is **not** a
production scheduler: it uses a single global dispatch queue (a scalability
bottleneck), and has no NUMA-aware load balancing, no per-domain run queues, no
cgroup bandwidth control, no core scheduling, and no CPU-frequency coupling. The
"Going further" section says exactly what the real ones (`scx_rusty`,
`scx_layered`, `scx_lavd`) add.

## What you'll learn

- The **`struct sched_ext_ops` callback model**: how the kernel delegates
  scheduling to `select_cpu`, `enqueue`, `dispatch`, `running`, `stopping`,
  `enable`, `init`, `exit` — and the exact moment each fires in a task's life.
- **Dispatch Queues (DSQs)** — per-CPU *local* run queues vs a *shared* global
  queue — and how `dispatch()` moving a task from shared to local is literally
  "scheduling the next task."
- **The select_cpu idle fast path**: why shoving a waking task straight onto an
  idle CPU's local DSQ is the most important latency optimization there is.
- **Virtual-time / weighted fair-share arithmetic**: charging `used * 100 /
  weight`, the wrap-safe `vtime_before` compare, and the anti-hoarding clamp —
  the machinery behind CFS/EEVDF, in ~10 lines.
- **How BPF becomes a scheduler**: `struct_ops`, the BPF verifier as a load-time
  gate, `const volatile` rodata config, `SEC(".struct_ops.link")`, and libbpf
  skeletons.
- **Measuring a scheduler** against stock EEVDF with `perf sched` and a workload.

## Build & run

> **Platform: Linux only, kernel ≥ 6.12 with `CONFIG_SCHED_CLASS_EXT=y`, run in a
> QEMU VM or a machine you can afford to make unresponsive.** This attaches a
> scheduler to the *whole system*. Kernel work belongs in a throwaway VM. It does
> **not** build or run on this Windows host; the assembly below is what is
> host-portable.

Dependencies: `clang`/`llvm` (with the BPF backend), `bpftool`, `libbpf-dev`,
`libelf-dev`, `zlib`, and the sched_ext helper headers `<scx/common.bpf.h>` /
`<scx/common.h>` (from the kernel tree `tools/sched_ext/include/`, or the
[`sched-ext/scx`](https://github.com/sched-ext/scx) repo).

```bash
# In your VM, kernel >= 6.12:
make                       # clang->BPF, bpftool skeleton, cc loader
                           # (point SCX_INCLUDE at your scx headers if needed:)
# make SCX_INCLUDE=/path/to/linux/tools/sched_ext/include

sudo ./scx_fifo            # default: weighted-vtime fair-share
sudo ./scx_fifo -f         # strict global FIFO
sudo ./scx_fifo -v         # verbose: watch the verifier accept each program
```

While it runs it prints, once a second, how tasks were queued:

```
local=12043 global=337     # local = idle-CPU fast path; global = shared DSQ
```

Confirm it is really the scheduler in charge, then stop it (Ctrl-C restores
EEVDF instantly):

```bash
cat /sys/kernel/sched_ext/root/ops     # -> "fifo"  (our scheduler is live)
dmesg | tail                            # sched_ext load/enable messages
```

### Regenerate the teaching assembly (works on ANY host, no kernel needed)

```bash
make asm      # writes asm/demo.{O0.s,s,O2.s} via cross-targeting clang
```

## How it works

**`scx_fifo.bpf.c` — the kernel half (BPF).** Implements the scheduler as a
table of callbacks. Follow one task through them:

- **`fifo_select_cpu`** — a task woke. Ask the kernel's built-in idle picker
  (`scx_bpf_select_cpu_dfl`) for a good CPU; if it found a truly idle one, insert
  the task straight onto that CPU's **local DSQ** (`SCX_DSQ_LOCAL`) so it runs
  immediately, skipping the shared queue. This is the latency fast path.
- **`fifo_enqueue`** — the task didn't take the fast path, so queue it in the one
  **shared DSQ**. In FIFO mode: tail-insert (`scx_bpf_dsq_insert`). In vtime
  mode: sorted-insert by virtual time (`scx_bpf_dsq_insert_vtime`) after applying
  the anti-hoarding clamp.
- **`fifo_dispatch`** — a CPU went idle and needs work. Move the head of the
  shared DSQ onto this CPU's local DSQ (`scx_bpf_dsq_move_to_local`). Every CPU
  pulling from the same shared queue is what makes this a *global* scheduler.
- **`fifo_running` / `fifo_stopping`** — vtime bookkeeping. On stop, charge
  `(SCX_SLICE_DFL - slice) * 100 / weight` to the task's clock: heavier tasks are
  charged less, so they sort to the front more often → weighted fairness.
- **`fifo_enable` / `fifo_init` / `fifo_exit`** — lifecycle: seed a task's vtime,
  create the shared DSQ at load, record why we unloaded.

The two policies are selected by a `const volatile bool fifo_sched` the loader
sets **before load**, so the verifier constant-folds the unused policy away.

**`scx_fifo.c` — the userspace half.** Opens the libbpf skeleton, sets the FIFO
flag into `.rodata`, then `LOAD` (runs the verifier) and `ATTACH` (the instant
our BPF code becomes the live scheduler). It then loops printing the per-CPU
`stats` map (local vs global queueing) until Ctrl-C or until the kernel ejects
the scheduler. Exiting drops the link, which detaches us and restores EEVDF —
the "unload on exit" safety property.

**`Makefile`** — the BPF toolchain (clang→BPF, `bpftool gen skeleton`, `cc`
loader), *not* a Kbuild `obj-m` module, because an SCX scheduler is a BPF program
the running kernel loads, not a `.ko`. The file explains the distinction.

### Measuring against EEVDF (`perf sched`)

```bash
# Baseline under stock EEVDF (scheduler NOT loaded):
perf sched record -- stress-ng --cpu $(nproc) --timeout 10s
perf sched latency --sort max          # note max/avg wakeup latency

# Now load our scheduler and repeat:
sudo ./scx_fifo &                       # (default weighted mode)
perf sched record -- stress-ng --cpu $(nproc) --timeout 10s
perf sched latency --sort max          # compare

# Also try a latency-sensitive vs throughput mix:
schbench -m 2 -t 4 -r 10               # p99 wakeup latency
```

Expect FIFO mode to show higher tail latency under mixed load (no priority),
and vtime mode to track EEVDF's fairness reasonably while being far simpler.
The single shared DSQ will cap throughput on many-core boxes — that gap *is* the
lesson about why production schedulers shard the run queue.

## Assembly notes

Kernel/BPF C cannot be compiled to standalone x86-64 assembly on this host:
`scx_fifo.bpf.c` needs `vmlinux.h`, the scx headers, and `clang -target bpf`
against a live kernel's BTF — it has no ordinary host form. So, per the lab's
assembly rule, [`asm/demo.c`](asm/demo.c) lifts out the scheduler's most
instructive **pure-logic core** — the virtual-time / weight arithmetic from
`enqueue()` and `stopping()` — into a self-contained file with no headers that
declares its own integer types and computes exactly what the scheduler computes.

[`asm/demo.annotated.s`](asm/demo.annotated.s) is the hand-annotated `-O1`
output. The highlights it walks through:

- **`vtime_before`** — the wrap-safe `(s64)(a-b) < 0` compare compiles to a
  subtract and a single **`shrq $63`** that lifts the sign bit into bit 0. No
  compare, no branch.
- **`vtime_charge`** — a 64-bit multiply then an expensive 64-bit unsigned
  **`divq`** (dividend in `rdx:rax`, so `rdx` must be pre-zeroed). Shows *why*
  the C multiplies by 100 before dividing by weight.
- **`vtime_clamp`** — the `if (before) x = floor` becomes a branchless
  **`cmov`**, with `vtime_before` inlined into the flags of one `cmp`.
- **`vtime_on_stop`** — at `-O1` both helpers are already **inlined** into one
  call-free body. Compare [`asm/demo.O0.s`](asm/demo.O0.s) (naive: real `call`s,
  everything spilled) and [`asm/demo.O2.s`](asm/demo.O2.s), where the optimizer
  adds a `shrq $32; je` divide-width check to use the cheaper 32-bit `divl` when
  the dividend fits — the "why is it doing THAT?" payoff.

Regenerate with `make asm` (cross-targets Linux; runs on any host).

## Going further (the `Stretch:` from the list)

- **Shard the run queue.** Replace the single shared DSQ with per-CPU (or
  per-LLC) DSQs and add a `dispatch()` that steals from a busy neighbour when
  local is empty. That is the step from "global lock" toward a scalable
  scheduler and the core idea behind `scx_rusty`.
- **Add real load balancing** across NUMA nodes with a userspace component that
  reads load stats over a BPF map and rebalances — the split-brain design (BPF
  fast path + userspace planner) that `scx_rusty` and `scx_layered` use.
- **Latency-nice / deadlines.** Move from plain vtime toward EEVDF's virtual
  deadline (eligibility + lag) so latency-sensitive tasks preempt promptly.
- **What production adds:** cgroup CPU bandwidth, core scheduling (SMT security),
  `cpufreq`/uclamp coupling, per-task tuning via BPF maps, and a watchdog-tight
  hot path that never risks the ejection timeout. Read `scx_rusty` and
  `scx_lavd` in the `sched-ext/scx` repo to see all of it assembled.

## References

- Kernel docs: `Documentation/scheduler/sched-ext.rst`; source under
  `kernel/sched/ext.c` and the ops in `include/linux/sched/ext.h`.
- The canonical example this is modeled on: `tools/sched_ext/scx_simple.bpf.c`
  and `scx_simple.c` in the Linux tree.
- [`sched-ext/scx`](https://github.com/sched-ext/scx) — production schedulers
  (`scx_rusty`, `scx_layered`, `scx_lavd`) and the `scx/common*.h` helper headers.
- `man 2 sched_setscheduler`, `man 1 perf-sched`; EEVDF: the LWN "An EEVDF CPU
  scheduler for Linux" articles.
