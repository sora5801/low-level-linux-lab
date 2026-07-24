# A strace clone 🟧

**What it is.** A working `strace`: it runs a program and prints every system
call it makes, decoded — `openat(AT_FDCWD, "/etc/ld.so.cache", O_RDONLY|O_CLOEXEC)
= 3`. It does this **two different ways**, which is the whole point:

1. **`ministrace`** — the classic `ptrace(PTRACE_SYSCALL)` loop. The kernel stops
   the tracee on *every* syscall, twice (entry and exit), and hands us the
   registers each time.
2. **`seccomp-strace`** — a `seccomp`-BPF filter returning `SECCOMP_RET_TRACE`.
   An in-kernel BPF program decides which syscalls trap to the tracer, so you
   pay the context-switch only for the calls you care about. This is the
   mechanism real sandboxes use.

Both share one syscall table and one argument decoder. Difficulty **🟧**: the
concepts (ptrace state machine, the syscall ABI, seccomp-BPF, reading another
process's memory) are individually approachable but interlock in fiddly ways —
the entry/exit pairing, the `r10`-not-`rcx` argument, the negative-errno return
band, and the ordering constraints on installing a filter before `exec`.

## What you'll learn

- **`ptrace(2)` as a state machine**: `PTRACE_TRACEME`, `PTRACE_SETOPTIONS`
  (`PTRACE_O_TRACESYSGOOD`, `PTRACE_O_EXITKILL`, `PTRACE_O_TRACESECCOMP`),
  `PTRACE_SYSCALL` vs `PTRACE_CONT`, `PTRACE_GETREGS`, and the modern
  `PTRACE_GET_SYSCALL_INFO` for classifying entry vs exit.
- **The x86-64 syscall ABI**: number in `orig_rax`; args in `rdi, rsi, rdx,
  r10, r8, r9` (note **`r10`, not `rcx`** — the `syscall` instruction clobbers
  `rcx`); return value in `rax`; errors as the `[-4095, -1]` negative-errno band.
- **Reading another process's memory** with `ptrace(PTRACE_PEEKDATA)` — one word
  at a time, with the `errno`-reset dance that distinguishes a real `-1` word
  from an error — to follow a `char *` argument and quote the string.
- **seccomp-BPF**: `PR_SET_NO_NEW_PRIVS`, `SECCOMP_SET_MODE_FILTER`, a classic
  cBPF program over `struct seccomp_data`, the `AUDIT_ARCH_*` guard, and how
  `SECCOMP_RET_TRACE` turns into `PTRACE_EVENT_SECCOMP` stops.
- **What the optimizer does** to a bitmask decode loop (see Assembly notes).

## Build & run (Linux / WSL2 only)

The tracers use `ptrace(2)`, `seccomp(2)`, and `<sys/user.h>`; they build and
run **only on Linux** (native or WSL2). The *assembly* deliverable regenerates
on any host (clang cross-targets Linux).

```bash
make                     # builds ./ministrace and ./seccomp-strace
make run                 # ./ministrace /bin/echo hello
make test                # runs both tracers, checks the write() line

# Trace anything:
./ministrace /bin/ls -l /
./ministrace cat /etc/hostname
./seccomp-strace /bin/echo hi     # same trace, seccomp-driven

# The trace goes to stderr (like real strace), so the child's stdout is clean:
./ministrace /bin/echo hello > /dev/null   # only the trace remains on screen
```

Example (abridged) output:

```
brk(NULL)                               = 0x561d2c3a1000
arch_prctl(0x3001, 0x7ffe...)           = -1 EINVAL (Invalid argument)
access("/etc/ld.so.preload", 4)         = -1 ENOENT (No such file or directory)
openat(AT_FDCWD, "/etc/ld.so.cache", O_RDONLY|O_CLOEXEC) = 3
mmap(NULL, 8192, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x7f...
write(1, "hello\n", 6)                  = 6
exit_group(0)                           = ?
+++ exited with 0 +++
```

## How it works (file by file)

- **`syscall_table.h` / `syscall_table.c`** — the number→name→arg-shape map. Two
  tables: `detail[]` gives ~45 common syscalls a full argument shape (which arg
  is an fd, a string, an `open` flag bitmask…), and `names[]` covers the whole
  x86-64 set so even undecoded calls print by name. **Self-contained** (no system
  headers) so it compiles to teaching assembly — see `asm/syscall_table.s`.
- **`decode.h` / `decode.c`** — turns a raw register word into text.
  `read_tracee_str()` follows a pointer into the tracee with `PTRACE_PEEKDATA`;
  `decode_flags()` is the bitmask table walk (`O_RDONLY|O_CREAT`);
  `format_arg()` dispatches on the arg type; `format_retval()` decodes the
  negative-errno band into `-1 ENOENT (...)`.
- **`ministrace.c`** — the `PTRACE_SYSCALL` tracer. `fork`, child does
  `PTRACE_TRACEME` + `execvp`, parent supervises: for each syscall-stop it reads
  the registers, classifies entry/exit (via `PTRACE_GET_SYSCALL_INFO` when the
  kernel supports it, else a strict entry/exit toggle), buffers the decoded call
  on entry, and completes it with the return value on exit. Signal-delivery
  stops are reported and re-injected.
- **`seccomp_trace.c`** — the seccomp variant. The child installs a cBPF filter
  returning `SECCOMP_RET_TRACE`, the parent sets `PTRACE_O_TRACESECCOMP` and runs
  the child at full speed with `PTRACE_CONT`, catching `PTRACE_EVENT_SECCOMP`
  stops (syscall entry) and single-stepping once to the exit stop to read the
  result. The filter program is the interesting bit — comment out one branch and
  key on `seccomp_data.nr` and you have a real syscall sandbox.
- **`asm/demo.c`** — a self-contained extraction of the two hottest pure-logic
  routines (`syscall_args`, the register→arg mapping; and `decode_flags`, the
  flag table walk) for the assembly deliverable.

**Honest scope (🟧).** This is a genuinely working teaching tracer, not a drop-in
`strace(1)`. It covers a single traced process (no `-f`/`PTRACE_O_TRACEFORK`
following of children/threads), x86-64 only (one syscall table; no x32/compat or
other arches), decodes a curated ~45 syscalls richly and the rest by name +
raw-hex args, and uses a simplified signal model (it forwards signals but does
not implement full group-stop handling). structs like `struct stat` are shown as
a bare pointer, not expanded. Each of these is a labeled "Going further" below.

## Assembly notes

The committed assembly comes from two **self-contained** units:

- **`asm/demo.annotated.s`** — the hand-annotated `-O1` output of `asm/demo.c`,
  a comment on essentially every instruction plus a full SysV AMD64 ABI header.
  Two things to see:
  - **`syscall_args`** is the register→argument mapping in the clear: six loads
    into `args[0..5]`, and the line that matters reads arg4 from `r->r10`
    (struct offset `+32`) — the kernel's substitute for `rcx`. Eleven
    instructions, no branches; the cleanest possible "C statement → asm" read.
  - **`decode_flags`** shows the optimizer at work at `-O1`: it hoisted `cap-1`
    into `%r9d` and reuses it as the bound everywhere, **inlined `put_hex`
    entirely** (no `call` — the `0x`, the nibble extraction into a stack scratch
    buffer at `-64(%rbp)`, and the reversed emit are all inline), and lowered the
    `while (nm[k])` name copy into a do-while. The `and` / `cmp`-equal /
    `not` / `and` sequence is literally the bitmask "test then consume" that
    makes the leftover-hex correct.
- **`asm/demo.O0.s`** / **`asm/demo.O2.s`** — the same code with everything
  spilled to the stack (`-O0`, `put_hex` a real `call`) and fully optimized
  (`-O2`). Diff `-O0` against the annotated `-O1` to watch the registers appear.
- **`asm/syscall_table.s`** (and `.O0.s` / `.O2.s`) — the assembly of a *real*
  project source file, since it too is header-free. It is mostly `.quad` /
  `.asciz` data with two tiny bounds-checked lookups; a good example of how a
  designated-initializer table becomes a flat `.rodata` array.

Regenerate with `make asm` (works on any host). `asm/demo.annotated.s` is
authored by hand and never overwritten.

## Going further (the `Stretch:` goals)

- **Follow children and threads.** Set `PTRACE_O_TRACEFORK | TRACEVFORK |
  TRACECLONE`, keep a pid→state map, and prefix each line with `[pid N]`. This is
  what `strace -f` does and is the single biggest gap here.
- **Turn the seccomp filter into a sandbox.** In `seccomp_trace.c`, compare
  `seccomp_data.nr` and `RET_TRACE` only chosen syscalls (or `RET_ERRNO` to fail
  them). That is the leap from *observing* to *enforcing* — the basis of
  container runtimes, `systemd`'s `SystemCallFilter=`, and browser sandboxes.
- **Expand structs.** Read `struct stat`, `struct sockaddr`, `struct iovec` out
  of the tracee and print their fields, the way real strace does.
- **Use `process_vm_readv(2)`** instead of the `PEEKDATA` word loop to copy
  strings and buffers in one syscall — a large speedup on I/O-heavy tracees.
- **`SECCOMP_RET_USER_NOTIF`** — the modern user-space notification variant
  (`SECCOMP_IOCTL_NOTIF_RECV`), which lets an unrelated supervisor process handle
  syscalls without being the ptrace parent. This is how sandboxes intercept
  syscalls at scale.

## References

- `man 2 ptrace` (esp. `PTRACE_SYSCALL`, `PTRACE_GET_SYSCALL_INFO`,
  `PTRACE_SETOPTIONS`), `man 2 seccomp`, `man 2 prctl`, `man 2 process_vm_readv`.
- Linux `arch/x86/entry/syscalls/syscall_64.tbl` — the authoritative number→name
  table; and `Documentation/userspace-api/seccomp_filter.rst`.
- The real `strace` source (`src/`), Julia Evans's "strace" zine, and Nelson
  Elhage's `ministrace` — the classic ~200-line teaching tracer this is modeled
  after.
- `linux/seccomp.h`, `linux/filter.h`, `linux/audit.h` — the seccomp/BPF ABI.
