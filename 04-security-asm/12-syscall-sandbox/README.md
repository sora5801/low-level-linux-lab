# A syscall sandbox 🟧

**What it is.** A small program launcher, `./sandbox`, that confines a target
program **three complementary ways** and then hands it control:

1. **seccomp-bpf allowlist** — a hand-built classic-BPF program installed in the
   kernel before `execve`. It runs on every syscall and returns
   `ALLOW` / `ERRNO` / `TRACE` / `KILL` based on the syscall **number**. Fast,
   unbypassable, but **pointer-blind** (it cannot read the path an `openat`
   points at).
2. **ptrace interposer** — a supervising parent that the `SECCOMP_RET_TRACE`
   action wakes on selected syscalls. It **can** read the tracee's memory, so it
   inspects `openat` paths — but that read is a classic **TOCTOU** race.
3. **Landlock** — a kernel LSM that enforces filesystem access **by path,
   in-kernel, race-free**. It does exactly the job seccomp cannot and ptrace
   does unsafely.

Each layer closes a hole the others leave open. The project then **tries to
escape its own sandbox** (`target.c`) and the README explains, honestly, where
each layer's guarantees end.

> ### On your own box only
> This is a **defensive**, educational tool: a sandbox you build, run against a
> program you compiled (`./target`), on a machine you own. The "escape attempts"
> are calls `target.c` makes against **your** sandbox to demonstrate its limits.
> Nothing here attacks a system you do not control, and there is no reason to run
> it anywhere but your own Linux/WSL box. Mitigations are toggled by you, in the
> Makefile and on the command line.

---

## What you'll learn

- **seccomp mode 2 / classic BPF**: the `struct seccomp_data` layout, writing a
  filter by hand with `BPF_STMT`/`BPF_JUMP`, the `SECCOMP_RET_*` actions and how
  the low 16 bits carry an errno or a tracer tag, `PR_SET_NO_NEW_PRIVS`, and
  installing via `seccomp(2)` (with the `prctl(PR_SET_SECCOMP)` fallback).
- **The pointer-blind rule**: why a seccomp filter sees `args[]` as *register
  values* and can never dereference a userspace pointer — the single fact that
  forces layers 2 and 3 to exist.
- **ptrace supervision**: `PTRACE_TRACEME`, `PTRACE_O_TRACESECCOMP`,
  `PTRACE_EVENT_SECCOMP`, reading tracee registers (`PTRACE_GETREGS`) and memory
  (`process_vm_readv`), and cancelling a syscall by rewriting `orig_rax`.
- **TOCTOU**: why validating a path in a tracer and then letting the kernel
  re-read it is racy on a multithreaded tracee, and why that is unfixable in a
  tracer.
- **Landlock**: `landlock_create_ruleset` / `landlock_add_rule` /
  `landlock_restrict_self` (syscalls 444–446), the access-right bitset, ABI
  version negotiation for forward/backward compatibility, and `O_PATH` handles.
- **The x86-64 syscall ABI** under a filter: `nr` in `rax`/`orig_rax`, args in
  `rdi, rsi, rdx, r10, r8, r9`, and the x32-alias / arch-tag pitfalls a filter
  must guard against.

---

## Build & run (Linux / WSL2, kernel ≥ 5.13 for Landlock)

```bash
make                 # builds ./sandbox (glibc) and ./target (static, no-libc)

make run             # seccomp + Landlock(/etc) + ptrace supervisor  (full demo)
make run-notrace     # seccomp + Landlock only (no tracer)
make run-strict      # seccomp + Landlock deny-ALL-filesystem

make asm             # regenerate asm/demo.{s,O0.s,O2.s} (works on ANY host)
```

Direct usage:

```bash
./sandbox [--trace] [--allow DIR]... -- PROGRAM [ARGS...]
  --trace      inspect openat() paths in a ptrace supervisor
  --allow DIR  permit read/exec beneath DIR via Landlock (repeatable)
```

**Expected finale of every run:** `target` prints its progress, survives the
neutralized `ptrace` probe, gets its `openat` calls ruled on by Landlock/the
supervisor, and is finally **killed by SIGSYS** when it calls `socket()` — a
syscall that is simply not on the allowlist. A non-zero exit of **159**
(`128 + 31`, where 31 is `SIGSYS`) is the sandbox working, not a bug.

On a non-Linux host only `make asm` works; the syscalls the binaries need do not
exist elsewhere. That is expected (see CONVENTIONS.md §6).

---

## How it works

Install order is fixed and load-bearing (`sandbox.c`, in the child, just before
`execve`):

```
PR_SET_NO_NEW_PRIVS  ->  Landlock ruleset  ->  seccomp filter  ->  execve(target)
```

`NO_NEW_PRIVS` first, because it is what makes unprivileged Landlock **and**
seccomp legal (and it stops the target regaining privilege through a setuid
`execve`). Landlock **before** seccomp, because installing Landlock itself needs
syscalls (`landlock_*`, `open`); if seccomp went first with those off the
allowlist we could not build the filesystem jail. So we lock the filesystem,
then slam the syscall door behind us. Both restrictions are **inherited across
`execve`** by design and can never be removed — which is the whole point.

File by file:

- **`sandbox.h`** — the policy types and the four public entry points, one per
  layer. The `policy_rule` table reads as intent (`ACT_ALLOW`/`ERRNO`/`TRACE`/
  `KILL`), not as raw constants.
- **`seccomp_filter.c`** — the blue-team core. `SANDBOX_POLICY[]` is the
  human-readable allowlist; `build_program()` compiles it into a flat
  `struct sock_filter[]`: an **arch guard** (pin `AUDIT_ARCH_X86_64`), an **x32
  guard** (reject the `0x40000000` alias space), then `if (nr==X) return ACTION`
  per row, then a **default `KILL`**. Every jump is a short local hop, so the
  array is correct for any policy length. Installed with `seccomp(2)`.
- **`landlock_fs.c`** — declares the Landlock ABI locally (so it builds even
  without `<linux/landlock.h>`), **negotiates the ABI version** to stay
  compatible across kernels, *handles* (denies) every filesystem right, then
  grants back only **read + execute** beneath each `--allow` path via
  `O_PATH` handles, and `restrict_self`.
- **`supervisor.c`** — the ptrace loop. On each `PTRACE_EVENT_SECCOMP` it reads
  the registers, pulls the `openat` path out of tracee memory with
  `process_vm_readv`, applies a prefix policy, and either lets the syscall run or
  **cancels** it (set `orig_rax = -1`, `rax = -EACCES`). Heavily commented with
  the TOCTOU caveat at the exact line the race lives.
- **`sandbox.c`** — argument parsing, `fork`, the child that jails itself and
  `execve`s, and the parent that either supervises or just `waitpid`s and reports
  how the target died.
- **`target.c`** — a static, no-libc program (its own `_start`, raw `syscall`s)
  so its trust surface is exactly the syscalls you can read. It walks one example
  of **each** seccomp action and ends with the deliberate `socket()` escape.
- **`asm/demo.c`** — the self-contained extraction (see below).

---

## Trying to escape — and where each layer stops

The point of a sandbox is knowing precisely what it does **not** stop.

### 1. Why an allowlist beats a denylist
`target` calls `socket()`. We never listed `socket`, and it dies anyway, because
the default action is `KILL`. A **denylist** ("block `socket`, `connect`,
`ptrace`, …") fails open: the ~400 syscalls you forgot, plus every syscall added
in a future kernel (`io_uring`, `openat2`, `pidfd_open`, …), sail straight
through — and several of them re-implement the very capability you were trying to
remove. `io_uring`, in particular, can perform reads/writes/opens *without the
corresponding syscalls ever appearing*, defeating a naive denylist entirely. An
**allowlist fails closed**: the syscall you never thought of is denied for free.
That is why every serious sandbox (systemd `SystemCallFilter=`, Docker's default
profile, Chrome, OpenSSH's `sandbox-seccomp`) is allowlist-shaped.

### 2. Why seccomp cannot make the `openat` decision (pointer-blind)
`target` opens `/etc/hostname` and `/etc/shadow`. The seccomp filter sees, for
both, an identical shape: `nr = openat`, `args[1] = <some 64-bit pointer>`. The
filter **cannot follow that pointer** — classic BPF has no instruction to
dereference user memory, and `seccomp_data` deliberately contains only the
register values. So seccomp can gate "may you call `openat` at all?" but never
"may you open *this path*?". Path decisions must come from a layer that runs
*after* the kernel resolves the pointer: Landlock (layer 3), or the ptrace tracer
(layer 2, unsafely — next point).

### 3. Why the ptrace path check is TOCTOU-racy
With `--trace`, the supervisor reads the `openat` path from tracee memory,
validates it, and allows the syscall. But the read (**time of check**) and the
kernel's own copy of the path during the real syscall (**time of use**) are on
opposite sides of a context switch, and the buffer stays writable by the tracee
the whole time. On a **multithreaded** target, thread B can overwrite the buffer
between the two:

```
   supervisor reads  "/home/me/ok.txt"   -> looks fine, allow
   thread B writes   "/etc/shadow"        (into the same buffer)
   kernel copies in  "/etc/shadow"        -> opens the wrong file
```

You cannot close this in a tracer: you do not control when the kernel re-reads,
and you cannot atomically freeze the tracee's memory across the boundary. (You
can *narrow* it — copy the arg into memory the tracee cannot reach, or use
`SECCOMP_RET_TRAP` with an in-process handler — but userspace argument
inspection is fundamentally the wrong tool.) **Landlock has no such window**: it
checks inside the kernel at the moment of use, after the path is resolved, so
`/etc/shadow` is denied atomically no matter what any thread does. That contrast
— run `make run` (tracer denies `/etc/shadow`) vs. the same policy in Landlock —
is the core defensive lesson: **enforce in the kernel, on the resolved object,
not in userspace on a copy of a pointer.**

### Defense summary (the blue-team takeaway)
- Prefer **allowlists** everywhere; deny by default; treat a new syscall as
  hostile until reviewed.
- Put **capability** decisions (which syscalls) in seccomp; put **object**
  decisions (which files/paths) in an in-kernel enforcer (Landlock, LSM), never
  in a userspace argument-scraper.
- Set `NO_NEW_PRIVS`, install before `execve`, and **fail closed** if any layer
  cannot be installed — a half-built sandbox is a false sense of safety.
- Pin the **arch** and reject the **x32 alias**; a filter that ignores
  `seccomp_data.arch` can be bypassed by presenting numbers under another ABI.

---

## Assembly notes

The full launcher needs Linux headers and a live kernel, so the committed
teaching assembly comes from **`asm/demo.c`**, a self-contained extraction (no
system headers, own fixed-width types) of the two genuinely instructive,
host-portable pieces:

- `build_allowlist()` — emits the classic-BPF program.
- `seccomp_run()` — **the classic-BPF virtual machine the kernel runs**: a
  one-register accumulator interpreter that loads a word from `seccomp_data`,
  compares, and returns an action. This is *"the classic-BPF check of the syscall
  nr against the allowlist"* in its purest form.

`asm/demo.annotated.s` (hand-written from the `-O1` output) walks all of it. The
highlights it draws out:

- A `struct bpf_insn` is **exactly 8 bytes** (`code:2 jt:1 jf:1 k:4`), so the
  compiler folds each fixed instruction into one 64-bit immediate — the packed
  constants like `0xC000003E00010015` you see *are* the BPF program's machine
  code (`code 0x15 = JMP|JEQ|K`, `jt 1`, `k = x86_64 arch`).
- `seccomp_run` shows two **branchless `jt`/`jf` selects** worth stealing:
  `sete`+`xorq $3` (pick the `jt` or `jf` byte on equality) and `adcq` (add the
  carry from the compare to pick the offset for `JGE`).
- Every error path in the interpreter loads `0x80000000` (`KILL`) — the machine
  **fails closed** even on a malformed program.
- In `demo_selftest`, `-O1` **constant-folded the entire filter** (splatting it
  into the stack with SSE `movaps`) and **inlined the VM four times**. That
  collapse is the lesson: an allowlist is small enough that the compiler can
  evaluate the whole thing at build time — there is nothing magic in seccomp.

Compare the three levels: `asm/demo.O0.s` (naive, every value spilled),
`asm/demo.s` (`-O1`, the annotated baseline), `asm/demo.O2.s` (`-O2`, vectorized
setup). Regenerate with `make asm`.

---

## Going further

- **`SECCOMP_RET_USER_NOTIF`** (the modern successor to `RET_TRACE`): the kernel
  hands the supervisor a file descriptor and, crucially, lets it fetch arguments
  from a stable kernel-side snapshot and supply the result — closing much of the
  ptrace TOCTOU gap. Re-implement layer 2 on top of it and compare.
- **`SCMP_ACT_LOG` / audit**: derive a real allowlist empirically by running a
  glibc program under a log-only filter and collecting the syscall set (this is
  how production profiles are built, e.g. for a static-target vs. the dynamic
  loader's startup calls).
- **Landlock networking** (ABI 4+) and **`TCP_BIND`/`TCP_CONNECT`** rules, to
  confine the `socket`/`connect` path we currently just `KILL`.
- **Namespaces + seccomp + Landlock together** — how real container runtimes
  (`runc`, `crun`, gVisor, `bubblewrap`) stack user/mount/pid namespaces under
  these same primitives.
- What production does: systemd's `SystemCallFilter=`/`RestrictAddressFamilies=`,
  Docker/containerd's default seccomp JSON, `libseccomp` (which generates exactly
  the kind of BPF `build_program()` writes by hand), and Chrome's
  `sandbox/linux/seccomp-bpf`.

---

## References

- `man 2 seccomp`, `man 2 prctl` (`PR_SET_NO_NEW_PRIVS`, `PR_SET_SECCOMP`),
  `man 2 ptrace`, `man 7 landlock`, `man 2 landlock_create_ruleset`.
- Kernel docs: `Documentation/userspace-api/seccomp_filter.rst` and
  `Documentation/userspace-api/landlock.rst`.
- `struct seccomp_data`, `SECCOMP_RET_*`: `include/uapi/linux/seccomp.h`.
- Classic BPF: `Documentation/networking/filter.rst`; the `BPF_*` macros in
  `include/uapi/linux/bpf_common.h` and `include/uapi/linux/filter.h`.
- Real allowlists to read: systemd `src/shared/seccomp-util.c`, `libseccomp`,
  Docker `profiles/seccomp/default.json`, OpenSSH `sandbox-seccomp-filter.c`.
