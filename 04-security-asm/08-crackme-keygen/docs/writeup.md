# Writeup: reversing the crackme, writing the keygen, killing the anti-debug

> Legal note: this is *your* binary — you built it from the source in this
> directory. Everything below is standard RE practice on software you own.

The goal is to make `./crackme <user> <serial>` print `Correct!` for a user of
our choosing, three ways:

1. **Understand the check** and write a **keygen** (the "right" solution).
2. **Neutralize the anti-debug** so you can watch it run in `gdb`.
3. **Patch** the binary so it never checks at all.

Throughout, build with symbols first (`make crackme`) to learn the layout, then
graduate to `make crackme-stripped` to practice without the training wheels.

---

## 0. Recon

```bash
$ file crackme
crackme: ELF 64-bit LSB executable, x86-64, ... not stripped   # symbols present
$ ./crackme alice AAAA-BBBB-CCCC-DDDD
Wrong serial for user 'alice'.
$ echo $?
1                                   # exit 1 = wrong serial (see enum in crackme.c)
```

`strings crackme | grep -i serial` shows the format hint `GGGG-GGGG-GGGG-GGGG`
and the messages, but **not** any serial — because no serial is stored. That
absence is the first real clue: the answer is *computed*, so we must recover the
computation.

---

## 1. Static analysis — recover the transform

Disassemble and jump to `main`:

```bash
objdump -d -Mintel crackme | sed -n '/<main>:/,/ret/p'
```

You will see, in order: a `call ptrace@plt`, a couple of `rdtsc` instructions
around a small loop, then a `call` into the validation. Follow the validation.
Because the transform's constants are unusual 64-bit immediates, the fastest way
to find the math is to **grep for the constants**:

```bash
objdump -d crackme | grep -iE 'movabs|rol|imul'
```

Three landmarks pin the algorithm (bytes are from `asm/demo.annotated.s`, the
mirror of this exact code — your `crackme` will match):

```
movabs $0xcbf29ce484222325,%r8    ; 49 b8 25 23 22 84 e4 9c f2 cb   FNV-1a offset basis
movabs $0x100000001b3,%rax        ; 48 b8 b3 01 00 00 00 01 00 00   FNV-1a prime (odd)
rol    $0x7,%rdx                  ; 48 c1 c2 07                     rotate-left 7
movabs $0xff51afd7ed558ccd,%rcx   ;                                 MurmurHash3 fmix64 #1
movabs $0xc4ceb9fe1a85ec53,%rcx   ;                                 MurmurHash3 fmix64 #2
```

Read the mixing loop (see `asm/demo.annotated.s` for the fully-commented copy):

```
loop:  movzbl %sil, %edx      ; edx = current byte
       xor    %r8, %rdx        ; h ^ byte
       imul   %rax, %rdx       ; * 0x100000001B3     (mult mod 2^64)
       rol    $7, %rdx         ; rotl(_, 7)
       xor    %rcx, %rdx       ; ^ 0x5DEECE66D
       ...advance pointer, loop until NUL...
```

and the avalanche after it:

```
h ^= h >> 33 ; h *= 0xFF51AFD7ED558CCD ; h ^= h >> 29 ;
h *= 0xC4CEB9FE1A85EC53 ; h ^= h >> 33
```

Then `format_serial` splits the 64-bit `h` into four 16-bit groups and prints
uppercase hex with dashes (`GGGG-GGGG-GGGG-GGGG`), and `ct_equal` compares 19
bytes. That is the whole algorithm; you have reversed it.

### The keygen

Re-implement the forward transform. In Python (no dependencies):

```python
def key_from_name(name: str) -> int:
    h = 0xCBF29CE484222325
    M = (1 << 64) - 1
    for b in name.encode():
        h = ((h ^ b) * 0x100000001B3) & M          # xor, then mult mod 2^64
        h = ((h << 7) | (h >> 57)) & M             # rotl 7
        h = (h ^ 0x5DEECE66D) & M                  # xor const
    h ^= h >> 33; h = (h * 0xFF51AFD7ED558CCD) & M  # fmix64
    h ^= h >> 29; h = (h * 0xC4CEB9FE1A85EC53) & M
    h ^= h >> 33
    return h

def serial(name: str) -> str:
    k = key_from_name(name)
    g = [(k >> s) & 0xFFFF for s in (48, 32, 16, 0)]
    return "-".join(f"{x:04X}" for x in g)

print(serial("alice"))   # 40A0-E72C-6088-C79A
```

Cross-check against the shipped `keygen` (which shares `serial.h` with the
crackme): `./keygen alice` → `40A0-E72C-6088-C79A`. They agree, because they are
the same function — one recovered, one authored. That match *is* the solve.

---

## 2. Dynamic analysis — and the anti-debug that stops it

Try to trace it:

```bash
$ gdb -q ./crackme
(gdb) run alice 40A0-E72C-6088-C79A
nice try — I'm being traced. Bailing.
[Inferior 1 exited with code 02]
```

Exit code 2 = the **ptrace gate**. Here is why it fires:

- A Linux process can have **exactly one** tracer. `gdb` attached, so it holds
  the slot.
- `crackme` calls `ptrace(PTRACE_TRACEME, 0, 0, 0)` asking "let my parent trace
  me." The slot is taken, so the kernel returns **-1 / EPERM**.
- `anti_debug_ptrace()` returns `(ret == -1)` → true → `main` prints the message
  and exits with code 2.

`strace` trips the same wire (it, too, is a ptrace tracer). We need to disarm it.

---

## 3. Bypass A — `LD_PRELOAD` hook (no patching)

The crackme calls the **libc `ptrace` wrapper**, which is resolved through the
PLT/GOT at load time. The dynamic loader searches `LD_PRELOAD` libraries first,
so we can shadow `ptrace` with our own that always says "success":

```c
// libfakeptrace.c (already in this dir)
long ptrace(long request, long pid, long addr, long data) { return 0; }
```

```bash
$ make libfakeptrace.so
$ LD_PRELOAD=./libfakeptrace.so gdb -q ./crackme
(gdb) run alice 40A0-E72C-6088-C79A
[libfakeptrace] intercepted ptrace(request=0) -> returning 0 (anti-debug neutralized)
Correct! Serial valid for user 'alice'.
```

The self-attach now "succeeds" even though gdb owns the real trace slot, so the
gate passes. This is the cleanest bypass and it needs zero edits to the target.

**Why it works / when it won't.** It only works because `ptrace` is an
*interposable dynamic symbol*. If the crackme issued the syscall directly —

```c
long r; register long a7 __asm__("rax") = 101 /*SYS_ptrace*/;
__asm__ volatile("syscall" : "+r"(a7) : "D"(0) : "rcx","r11","memory");
```

— there is no symbol to shadow and `LD_PRELOAD` does nothing. Then you fall back
to §4 (patching) or a seccomp/`PTRACE_SYSCALL` filter that rewrites the return
value. Recognizing which bypass a target *admits* is the skill.

---

## 4. Bypass B — byte-patch the branch (permanent)

Find the decision right after the ptrace call:

```bash
objdump -d -Mintel crackme | grep -A8 'call.*ptrace'
```

You will see something like (addresses vary by build):

```
  ... call   <ptrace@plt>
  ... cmp    rax, -1          ; ret == -1 ?  (or test/inc + je/jne)
  ... jne    <ok>             ; not traced -> continue        <-- the gate
  ... <fall through to the "Bailing" path>
```

Two equivalent patches:

- **Invert or NOP the conditional jump.** Change the `jne <ok>` so control always
  reaches `<ok>`. The simplest reliable edit is to turn the two-byte conditional
  jump into an unconditional `jmp` (opcode `0xEB`) with the same displacement, or
  overwrite it with two `0x90` (NOP) bytes if falling through already lands on the
  good path. (Whether you NOP or flip depends on which side is the "bail" path —
  read the target of the jump first.)
- **Force the compare.** Overwrite the `cmp rax, -1` so the "traced" test can
  never be true.

Do it in place without an editor using `gdb`'s patching, then let it run:

```bash
gdb -q -write ./crackme
(gdb) # example: at the gate's conditional jump address 0xNNNN, make it unconditional
(gdb) set {unsigned char}0xNNNN = 0xeb
(gdb) # save is automatic in -write mode after `set`; or use `dump`/objcopy for a file copy
```

Or patch a *copy* on disk with a one-liner once you know the file offset `OFF`
and the byte `B` (from `objdump`'s left-hand column mapped via the section
headers):

```bash
printf "\x${B}" | dd of=crackme.patched bs=1 seek=$((OFF)) conv=notrunc
```

After patching, `./crackme.patched alice 40A0-E72C-6088-C79A` prints `Correct!`
even under gdb. This is the most durable bypass and the one that survives being
copied to another machine.

---

## 5. Bypass C — beat it live in gdb (no files touched)

If you just want to step through once, override the ptrace result at runtime.
Break right after the call and set the return register to 0:

```bash
gdb -q ./crackme
(gdb) break *main                    # then disassemble to find the call site
(gdb) run alice 40A0-E72C-6088-C79A
(gdb) # step to just after `call ptrace@plt`, then:
(gdb) set $rax = 0                    # pretend the self-attach succeeded
(gdb) # ...or skip the whole gate by jumping past its branch:
(gdb) # set $pc = <address of the 'ok' path>
(gdb) continue
```

### The timing gate, too

Single-stepping (`si`) through the `rdtsc … rdtsc` region will blow the cycle
budget and trip gate 2 (`too slow — am I being single-stepped?`, exit 3). Three
easy answers:

- Don't single-step it: set a breakpoint *after* the timing check and `continue`
  to it at full speed (the generous budget tolerates normal breakpoints).
- `set var` / `set $pc` past the timing branch, exactly as in §4/§5 for ptrace.
- Patch the timing branch out permanently (same technique as §4).

The timing check is deliberately fragile — a context switch can even false-
positive it on a native run — which is precisely why real tools do not lean on
it.

---

## 6. What this proves (defense)

All three bypasses took minutes because **the check runs on the attacker's
machine**. That is unfixable client-side: you can raise the cost (raw syscalls,
self-checksums, obfuscation, packing) but never the *floor* — a determined
reverser owns the CPU, the loader, and the debugger.

- A real licensing scheme verifies a **signature** the client cannot forge (the
  signing key never ships) or binds entitlement to a **server**. Our
  hash-of-username hands the client everything it needs to answer itself.
- The anti-debug tricks are worth knowing for the **blue team**: `PTRACE_TRACEME`
  self-attach and `rdtsc` timing gates are classic malware sandbox-evasion, and
  detection tooling watches for exactly these patterns. You learn the offense to
  build the detector.
- The one genuinely defensive line of code here is `ct_equal`: a **constant-time**
  comparison, so the accept/reject decision leaks nothing through timing. We even
  confirmed in `asm/demo.annotated.s` that the optimizer preserved the property
  (it emitted a branch-free `sete`, not an early-out) — a reminder that
  constant-time code must be checked *in the assembly*, because the compiler is
  free to reintroduce a branch.
```
