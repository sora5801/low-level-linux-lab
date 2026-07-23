# init / PID 1 / service supervisor 🟧

**What it is.** A small process supervisor written to run as **PID 1** inside a
container or VM. It reaps zombies correctly, adopts orphaned processes, parses a
tiny service config, starts services in dependency order, restarts them on exit
with exponential backoff, and shuts everything down cleanly on `SIGTERM`. It is
the program that would be your container's `ENTRYPOINT` — the thing `docker run`
starts as process 1. This is a genuinely working **teaching core**: it does the
hard, easy-to-get-wrong parts (reaping, signal handling, ordering, backoff,
shutdown) and deliberately omits the production sprawl (readiness protocols,
cgroups, socket activation) — see [Going further](#going-further).

## What you'll learn

- **Why PID 1 is special**, and why a naive program makes a broken init:
  - it is the **universal reaper** — every orphaned process in the system (or PID
    namespace) reparents to it, so it *must* `wait()` on them or leak zombies;
  - its **signal defaults are disabled** — the kernel drops `SIGTERM`/`SIGINT`
    unless PID 1 explicitly catches them, so a naive init silently ignores
    shutdown;
  - **if PID 1 exits, everything dies** — kernel panic in the root namespace,
    or `SIGKILL` to the whole PID namespace in a container.
- **`SIGCHLD` + `waitpid(WNOHANG)` in a loop** — and the crucial subtlety that
  standard signals *coalesce*, so one `SIGCHLD` can stand for several dead
  children.
- **`signalfd(2)`** — turning asynchronous signals into ordinary readable file
  descriptors so signal handling happens at a well-defined point in a `poll()`
  loop, instead of in a treacherous async signal handler.
- **`PR_SET_CHILD_SUBREAPER`** (`prctl(2)`) — becoming the reaper for orphans
  even when you are *not* PID 1, which is what lets you test this as a normal user.
- **`fork`/`execvp`, process groups (`setpgid`), and signal forwarding** —
  including resetting the child's signal mask before exec.
- **Dependency ordering** via a topological sort, and **exponential backoff**
  with overflow-safe integer math — the two pure-logic routines dissected in the
  assembly.

## Build & run (Linux / WSL only)

The binary uses `signalfd(2)` and `PR_SET_CHILD_SUBREAPER`, which are
Linux-specific, so it builds and runs on **Linux or WSL2** only. (The assembly
regenerates on any host — see [Assembly notes](#assembly-notes).)

```bash
make                 # builds ./supervisor with -Wall -Wextra -O2 -D_GNU_SOURCE
make run             # runs it against services.conf in "subreaper mode"
```

Running `make run` starts it as an ordinary process (not PID 1). Thanks to
`PR_SET_CHILD_SUBREAPER` it still reaps orphans and shows the full lifecycle.
You will see `logger` start before `web` (dependency ordering), `flapper`
crash-loop with a doubling restart delay (backoff), and `oneshot` run once and
stay down. Press **Ctrl-C** to trigger a clean shutdown:

```
[init 34211] running as pid 34211 (not init; using subreaper mode)
[init 34211] parsed 4 service(s) from services.conf
[init 34211] started 'logger' pid=34212
[init 34211] started 'web' pid=34213
[init 34211] started 'flapper' pid=34214
[init 34211] started 'oneshot' pid=34215
[init 34211] 'oneshot' exited code=0
[init 34211] 'oneshot' will not be restarted (policy)
[init 34211] 'flapper' exited code=1
[init 34211] 'flapper' restarting in 100ms (attempt #1)
[init 34211] 'flapper' restarting in 200ms (attempt #2)
...                               # ^C
[init 34211] shutdown requested (signal 2); forwarding SIGTERM to children
[init 34211] all services stopped; init exiting cleanly
```

**Run it as real PID 1** (the point of the project) with an unshared PID
namespace or a container:

```bash
# Quickest: a throwaway PID namespace. The supervisor is pid 1 inside it.
sudo unshare --pid --fork --mount-proc ./supervisor services.conf

# Or in a container, as the entrypoint:
#   COPY supervisor services.conf /
#   ENTRYPOINT ["/supervisor", "/services.conf"]
docker run --rm your-image
```

Verify the reaper duty: inside the namespace, start something that orphans a
child, watch the child reparent to pid 1 and get reaped (`[init 1] reaped orphan
pid=…`). Send `docker stop` (a `SIGTERM`) and watch the clean shutdown.

## How it works

The code is split so the **pure logic** (which compiles to legible assembly) is
isolated from the **syscall-heavy runtime**.

- **`supervisor.c`** — the heart. In order: announce whether we are PID 1; call
  `prctl(PR_SET_CHILD_SUBREAPER, 1)`; **block** `SIGCHLD`/`SIGTERM`/`SIGINT` with
  `sigprocmask` and open a `signalfd` for them; parse the config; build the
  dependency matrix and topologically sort it; `fork`+`execvp` every service in
  order; then run one `poll()` loop that (a) reaps children on `SIGCHLD` with a
  `waitpid(-1, WNOHANG)` drain loop, (b) reschedules crashed services with
  backoff, and (c) on `SIGTERM`/`SIGINT` forwards `SIGTERM` to every child's
  process group, reaps them within a grace window, escalates to `SIGKILL`, and
  exits cleanly. Every syscall's return is checked; there is **no `malloc`** in
  the whole program (PID 1 must never fail an allocation).
- **`order.c` / `order.h`** — the pure-logic core, deliberately **header-free** so
  it compiles straight to assembly: `sup_toposort` (Kahn's algorithm for start
  ordering) and `sup_backoff_delay_ms` (exponential backoff with an overflow-safe
  clamp). Used for real by `supervisor.c` *and* shipped as teaching asm.
- **`config.c` / `config.h`** — the INI-style config parser. Fixed buffers,
  every bound checked, `after` names resolved to indices with unknown-dependency
  and self-dependency errors caught up front.
- **`service.h`** — the `struct sup_service` model (command line, restart policy,
  dependencies, live runtime state) and all the fixed-size limits. Explains why
  there is no dynamic allocation.
- **`services.conf`** — a sample that exercises every behaviour with just
  coreutils: ordering, a stable daemon, a crash-loop, and a one-shot task.

### The signal design, in one paragraph

The naive init installs a `SIGCHLD` handler that calls `waitpid`. That is a trap:
signal handlers may only call async-signal-safe functions, they race the main
loop, and signals coalesce. Instead we **block** the signals we care about and
read them as bytes from a `signalfd` inside `poll()`. Now "handling a signal" is
just ordinary code at a known point. The one rule we must still respect: because
signals coalesce, each `SIGCHLD` triggers a `waitpid(-1, WNOHANG)` loop that
reaps *every* currently-dead child, not just one.

## Assembly notes

The self-contained routines in `order.c` are extracted verbatim (minus the
header) into [`asm/demo.c`](asm/demo.c) and compiled with the lab's exact flags.
Because `order.c` itself includes no system headers, its **real** object code is
committed too (`asm/order.*.s`) — the assembly reflects actual project source.

Generated (genuine `clang --target=x86_64-pc-linux-gnu` output, regenerate with
`make asm`):

- [`asm/demo.O0.s`](asm/demo.O0.s) — naive mapping: every variable spilled to
  the stack, every loop a literal compare-and-jump. Easiest to read line-by-line.
- [`asm/demo.s`](asm/demo.s) — `-O1`, the annotated baseline.
- [`asm/demo.O2.s`](asm/demo.O2.s) — `-O2` for comparison.
- [`asm/order.{O0.s,s,O2.s}`](asm/) — the same three levels for the *real*
  `order.c` (identical logic, `sup_`-prefixed symbols).

Hand-written: [`asm/demo.annotated.s`](asm/demo.annotated.s) comments essentially
every instruction and opens with the SysV AMD64 ABI cheat-sheet. The highlights
it draws out:

- **`backoff_delay_ms` is a leaf function**: no `call`, so nothing beyond the
  frame-pointer prologue, and the clamp/saturate is done **branchlessly with
  `cmov`** — no data-dependent branch for the CPU to mispredict.
- **`toposort` is *not* a leaf** (it calls `memset`): so it (1) parks the values
  that must survive the call in **callee-saved** `rbx`/`r14`/`r15`, exactly as
  the ABI requires, and (2) allocates real stack with `subq` instead of using the
  **128-byte red zone** (which a non-leaf cannot touch), padding `128→136` so
  `rsp` is **16-byte aligned** at the `call`.
- The optimizer turning `deg += (row[j] != 0)` into the branchless carry-flag
  trick **`cmpb $1, mem ; sbb $-1, %ecx`** — visible only by reading the asm.

## Going further

The `Stretch:` direction is to grow this core toward a real init:

- **Readiness, not just ordering.** `after` here gates *start order*, not
  *readiness*. Add a readiness protocol (systemd's `sd_notify`, or socket
  activation where PID 1 opens the listening socket and hands it to the service)
  so a dependent starts only once its dependency is actually serving.
- **Shell quoting in `exec`.** The parser splits on spaces; add proper quote
  handling (or keep wrapping in `/bin/sh -c "…"`, which is what most inits do).
- **`StartLimitBurst`.** We already reset the backoff counter after a service
  stays up longer than the cap; add a hard "give up after N failures in a window"
  so a permanently-broken service stops thrashing.
- **Signal fan-out and `reload`.** Forward `SIGHUP`/`SIGUSR1` to services, and
  re-read the config on `SIGHUP` to add/remove services without restarting init.
- **cgroup/namespace setup, `/proc` mount, tty/getty respawn** — the parts a
  container init (tini, dumb-init) or a full init (systemd, runit, s6) adds.

What production does: **tini**/**dumb-init** are ~this program's reaping +
signal-forwarding half, meant to sit at PID 1 under your real app.
**runit**/**s6**/**systemd** add supervision trees, readiness, sockets, logging,
and dependency graphs an order of magnitude larger than `order.c`.

## References

- `man 2 signalfd`, `man 2 prctl` (`PR_SET_CHILD_SUBREAPER`), `man 2 waitpid`,
  `man 2 sigprocmask`, `man 2 poll`, `man 2 clock_gettime`, `man 7 signal`.
- The Linux kernel on PID 1 signal semantics: `kernel/signal.c`
  (`sig_task_ignored` / the special-casing of the init task).
- **tini** (`krallin/tini`) and **dumb-init** (`Yelp/dumb-init`) — minimal PID-1
  reapers; read `tini.c`, it is the same idea as `supervisor.c`.
- **runit** (`runsv`), **s6** (`s6-supervise`), and systemd (`src/core/`) — the
  real supervision trees to graduate to.
- The "PID 1 zombie reaping problem" write-up (Phusion) — the canonical
  explanation of why containers need a real init.
```
