# shellcode: mechanics & defense 🟧

**What it is.** A didactic look at *shellcode* — small position-independent
machine code that makes a system call directly — and, more importantly, at the
**defenses** that make classic shellcode injection a solved problem on modern
systems. This project is deliberately scoped to **teach the concepts and the
blue-team side**, not to hand you a turnkey weapon.

> **Scope & ethics.** This is for understanding your own systems and for
> authorized learning (CTF/coursework) on machines you own. In keeping with that
> framing — and with the repo's rule that *the detector is the more instructive
> half* — this directory ships the **mechanics** (how a `syscall` is issued, why
> null bytes matter) and a **defensive analyzer** (a bad-char/null-byte
> scanner), but intentionally does **not** ship a ready-to-inject payload or a
> polymorphic encoder/decoder. Building a full payload is left as a guided
> exercise against your own deliberately-vulnerable target (see
> [`../10-memory-corruption-ladder`](../10-memory-corruption-ladder)); packaged
> offensive tooling and detection-evasion encoders are out of scope for a public
> teaching repo.

## What you'll learn

- **How a syscall is issued** on x86-64 Linux: number in `rax`, arguments in
  `rdi, rsi, rdx, r10, r8, r9`, then `syscall`. `execve` is number 59; it takes
  `(const char *path, char *const argv[], char *const envp[])`. Making that call
  is exactly what a shell does when it launches a program — the syscall itself is
  not the "attack", the *unauthorized injection* is.
- **Position independence**: why injected code can't rely on absolute addresses,
  and how the "string on the stack" / RIP-relative techniques get an address for
  a literal without a load-time relocation.
- **The null-byte problem**: many injection vectors are C string copies
  (`strcpy`), which stop at the first `0x00`. So injected code historically had
  to be *null-free*, which is why you see `xor %rax,%rax` (2 bytes, no zero) used
  to zero a register instead of `mov $0,%rax` (which encodes zero bytes). This is
  a property of the *delivery channel*, not of the code's behavior.
- **Why this era is largely over — the defenses (the real lesson):**
  - **NX / DEP** (`W^X`): the stack and heap are mapped no-execute, so injected
    bytes can't run as code at all. This is the single change that killed classic
    shellcode injection; it's why exploitation moved to **ROP** (reusing existing
    executable code — see [`../10-memory-corruption-ladder`](../10-memory-corruption-ladder)).
  - **ASLR**: randomized mappings mean the attacker can't predict where anything
    is, defeating hardcoded addresses.
  - **Stack canaries**, **RELRO**, **PIE**, **CFI/shadow stacks**: each raises the
    bar further.
  - **`seccomp`** (see [`../12-syscall-sandbox`](../12-syscall-sandbox)): even if
    code runs, an allowlist can forbid `execve` outright.
  - **Detection**: IDS/AV and memory scanners flag the tell-tale signatures —
    the `execve("/bin/sh")` byte pattern, NOP sleds, and the absence of null
    bytes in a data buffer that "should" have them. **That scanner is what this
    project ships.**

## Build & run (Linux / WSL)

```bash
make            # builds ./nullscan, the defensive bad-char/null-byte analyzer
make test       # runs it over sample buffers
make asm        # regenerates the annotated teaching assembly
```

`nullscan` reads a binary buffer (file or stdin) and reports whether it is
null-free and which "bad characters" (configurable, e.g. `0x00 0x0a 0x0d`) it
contains and at what offsets — the exact analysis a defender or a shellcode
*author* runs to reason about a delivery channel. It is a pure, safe utility.

## How it works

- `nullscan.c` — the defensive analyzer: reads bytes, scans for a configurable
  bad-char set, prints offsets, and summarizes null-freeness. Heavily commented
  on *why* each bad char matters for a given delivery channel (`0x00` for
  `strcpy`, `0x0a`/`0x0d` for line-based input, etc.).
- `execve_syscall.S` — a **commented walkthrough** of the register setup for an
  `execve` syscall (rax/rdi/rsi/rdx + `syscall`), presented purely as *"this is
  how the ABI issues a syscall"*, the same mechanics used by
  [`../01-nolibc-programs`](../01-nolibc-programs) and
  [`../04-mini-libc`](../04-mini-libc) to call `write`/`exit`. It is annotated to
  show which bytes of each instruction would or would not contain a zero, which
  is the didactic point about null-freeness — without being assembled into a
  packaged, injectable payload.

## Assembly notes

The committed teaching assembly (`asm/demo.annotated.s`) is generated from
`asm/demo.c`, the **null-byte / bad-char scanner core** — a self-contained
`contains_badchar()` routine. It's a great asm read: at `-O2` clang turns the
byte-scan into a tight loop (and, for the plain null scan, effectively a
`memchr`), and the annotation walks the SysV ABI, the loop induction, and the
branchless flag accumulation. This is deliberately the *defensive* primitive:
the same code an IDS or a memory scanner uses.

## Going further

- Read the classic treatment (education): *Smashing the Stack for Fun and
  Profit* (Aleph One), *Hacking: The Art of Exploitation* (Erickson). Then study
  how **NX + ASLR** forced the shift to **ROP** — build that in
  [`../10-memory-corruption-ladder`](../10-memory-corruption-ladder) on a target
  you compile yourself with mitigations toggled.
- Blue team: extend `nullscan` into a signature scanner (NOP-sled detection,
  `execve("/bin/sh")` pattern matching, entropy analysis for encoded payloads),
  and wire it to `seccomp` ([`../12-syscall-sandbox`](../12-syscall-sandbox)) so a
  process that somehow runs injected code still can't reach `execve`.

## References

- System V AMD64 ABI; `man 2 execve`, `man 2 syscall`.
- `man 7 mprotect` / `W^X`, `Documentation/admin-guide/hw-vuln/`, and the kernel
  NX/ASLR docs for the defenses.
