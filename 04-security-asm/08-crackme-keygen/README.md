# crackme + keygen 🟧

**What it is.** A tiny Linux binary (`crackme`) that checks a serial against a
username, wrapped in two classic anti-debugging tricks — plus the full
**solution**: a `keygen` that mints valid serials, an `LD_PRELOAD` shim that
neutralizes the anti-debug, and a step-by-step reverse-engineering
[writeup](docs/writeup.md). The serial check is a real, non-trivial transform
(FNV-style hash: xor + multiply-mod-2⁶⁴ + rotate, then a MurmurHash3 avalanche),
so recovering it is a genuine exercise rather than a `strings` one-liner.

> **On your own box only.** This is *your* binary — you compile it, you own it,
> you attack it. Everything here (self-ptrace, timing gates, `LD_PRELOAD`
> hooking, byte-patching) is standard, legal reverse-engineering practice on
> software you own and on deliberately-vulnerable targets you build yourself.
> The mitigation knobs (PIE/ASLR, stack canary) are toggled **by you** in the
> Makefile, and the README says exactly what each one costs. Do not point these
> techniques at software you do not own or are not authorized to test.

## What you'll learn

- **Static RE**: find a validation routine in a disassembly by its *constants*
  (the FNV prime `0x100000001B3`, the fmix64 constants) and its *shape* (a
  `rolq $7`, three `imulq`/`shr`/`xor` avalanche steps).
- **Dynamic RE**: drive the binary under `gdb`, read/patch registers and memory,
  break past a check.
- **Anti-debug, and why it's weak**:
  - `ptrace(PTRACE_TRACEME)` self-attach — a process may have only one tracer,
    so a second attach (gdb) fails with `EPERM`. That failure is the tell.
  - an `rdtsc` **timing gate** — cycles across a tiny loop balloon when
    single-stepped.
- **Three bypasses**, weakest-target-first: `LD_PRELOAD` hooking of `ptrace`,
  byte-patching the branch, and forcing the return value live in `gdb`.
- **A defensive crypto detail**: the serial compare is **constant-time** to
  avoid a timing oracle, and we show that the optimizer preserved that property
  (it lowered the idiom to a branch-free `sete`).

## Build & run (Linux / WSL)

```bash
make            # crackme, keygen, libfakeptrace.so
make test       # keygen a serial for several names, prove crackme accepts it

# do it by hand:
./keygen alice                       # -> 40A0-E72C-6088-C79A
./crackme alice 40A0-E72C-6088-C79A  # -> Correct! Serial valid for user 'alice'.
./crackme alice 0000-0000-0000-0000  # -> Wrong serial for user 'alice'.
```

Real, reproducible serials from this exact transform:

| username       | serial                |
|----------------|-----------------------|
| `alice`        | `40A0-E72C-6088-C79A` |
| `bob`          | `166F-4461-6977-7494` |
| `Ada Lovelace` | `1883-0FC1-F754-0B00` |
| `root`         | `E371-0677-9810-03F5` |

Try to break the anti-debug:

```bash
gdb ./crackme                        # ptrace gate fires: "nice try — I'm being traced."
LD_PRELOAD=./libfakeptrace.so ./crackme alice 40A0-E72C-6088-C79A   # gate defeated
make bypass                          # scripted demo of the LD_PRELOAD hook
```

`make asm` regenerates the teaching assembly on **any** host (it doesn't run the
binary — clang cross-targets Linux).

## How it works — a tour of the files

- **`serial.h`** — the single source of truth for the username→serial transform,
  included by both the crackme and the keygen so they *cannot* disagree. Holds
  `key_from_name` (the hash), `format_serial` (64-bit key → `GGGG-GGGG-GGGG-GGGG`),
  and `ct_equal` (the constant-time compare, with the timing-oracle lesson).
- **`crackme.c`** — the target. `main` runs gate 1 (`anti_debug_ptrace`), gate 2
  (`anti_debug_timing`, built on an inline-asm `rdtsc`), then the real check:
  `key_from_name(name)` → `format_serial` → length gate → `ct_equal`. Distinct
  exit codes (0/1/2/3) tell you which gate fired.
- **`keygen.c`** — the solution. Runs the transform *forward* and prints the
  serial. Short on purpose: reversing is hard, re-running is trivial — the whole
  economics of a keygen-me.
- **`libfakeptrace.c`** — an `LD_PRELOAD` shim exporting its own `ptrace` that
  always returns 0, so the crackme believes its self-attach succeeded even under
  gdb. Works *because* the crackme calls the libc `ptrace` wrapper (a PLT
  symbol); a raw-`syscall` version would defeat this hook — a hardening step the
  writeup discusses.
- **`asm/demo.c`** — a header-free mirror of the transform, so the committed
  assembly is exactly the codegen you'd reverse. See below.
- **`docs/writeup.md`** — the full static + dynamic walkthrough and all three
  bypasses.

## Assembly notes

`asm/demo.annotated.s` is the hand-annotated `-O1` codegen for the transform.
The three landmarks that let you locate the keygen math in *any* build:

```
49 b8 25 23 22 84 e4 9c f2 cb   movabs $0xcbf29ce484222325,%r8   ; FNV offset basis
48 b8 b3 01 00 00 00 01 00 00   movabs $0x100000001b3,%rax       ; FNV prime (odd)
48 c1 c2 07                     rol    $0x7,%rdx                 ; the rotate
48 b9 cd 8c 55 ed d7 af 51 ff   movabs $0xff51afd7ed558ccd,%rcx  ; fmix64 constant 1
```

Two teaching payoffs in the annotated file:

1. In `validate`, clang **inlined four functions** and reshaped the group-format
   loop into a *shift-amount countdown* (`%cl` = 48 → 32 → 16 → 0) — a good
   example of "why is it doing THAT?" optimizer output.
2. The hand-rolled constant-time compare `1u ^ ((diff|(0-diff))>>31)` was
   **proved equal to `diff == 0`** and lowered to a single branch-free `sete`.
   The compare stays constant-time — but you only *know* that by reading the asm,
   which is the point.

Compare `asm/demo.O0.s` (naive, one call per routine), `asm/demo.s` (`-O1`, the
annotated baseline), and `asm/demo.O2.s` (more aggressive). Regenerate with
`make asm`.

## Going further (the `Stretch:`)

- **Harden it, then re-break it.** Replace the libc `ptrace()` call with a raw
  inline `syscall` (rax=101). `LD_PRELOAD` now does nothing — feel the hook die —
  and fall back to byte-patching or a seccomp/`PTRACE`-based syscall rewriter.
- **Add a checksum/anti-patch.** Have the code `read()` its own `.text` and hash
  it, so NOP-ing the anti-debug branch changes the hash and trips a second gate.
  Then defeat *that* by hooking the read or patching the expected hash — the
  client-side arms race, which nobody wins (see Defense).
- **Invert the transform.** The per-byte multiply is a bijection mod 2⁶⁴; with
  the modular inverse of the FNV prime you can walk the hash *backwards*. It
  won't give you a printable username (the fmix and rotates fold information),
  but it's the right way to learn why "reversible" and "invertible" differ.

## Defense — the actual lesson

Client-side anti-tamper is **speed-bump, not wall**. Every gate here falls to a
few minutes of `LD_PRELOAD` or a two-byte patch, because the code *and* the
check both run on the attacker's machine — they control the whole world the
binary sees. The takeaways a defender should carry away:

- **Do not gate value on client-side checks.** Real licensing binds to a
  server, or verifies a **signature** the client cannot forge (the secret —
  the signing key — never ships). A hash-of-username is a *tell-me-the-answer*
  scheme; a signature is a *prove-you-were-told* scheme.
- **Anti-debug is for raising analysis cost and for detection, not prevention.**
  The blue-team value of knowing these tricks is recognizing them in *malware*:
  self-ptrace and `rdtsc` timing gates are textbook sandbox-evasion, and EDR/
  sandboxes watch for exactly the `PTRACE_TRACEME` and `rdtsc`-delta patterns
  shown here. You learn the offense to build the detector.
- **Constant-time comparison is a real defense** (unlike the anti-debug theater).
  A short-circuiting `memcmp` on a secret leaks its length-of-correct-prefix
  through timing and has broken real MAC checks. `ct_equal` here folds every
  byte before deciding — and we verified in the asm that the optimizer kept it
  branch-free.

## References

- `man 2 ptrace` — `PTRACE_TRACEME`, the one-tracer rule, `EPERM`.
- Intel SDM Vol. 2 — `RDTSC` (and `RDTSCP`, the serializing variant).
- `man 8 ld.so` — `LD_PRELOAD` and symbol interposition order.
- FNV hash (Fowler–Noll–Vo) and MurmurHash3 `fmix64` — the transform's lineage.
- "Remote Timing Attacks are Practical" (Brumley & Boneh) — why constant-time.
- The classic *crackme* corpus (crackmes.one) — practice targets you own.
