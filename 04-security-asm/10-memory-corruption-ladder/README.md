# The classic memory-corruption ladder 🟥

**What it is.** Four rungs of the classic x86-64 memory-corruption progression,
each shipped as a matched set: a **vulnerable target you compile yourself**, an
**exploit** that defeats it, and — the point of the whole project — the exact
**defense** that would have stopped it and what that defense costs the attacker.
The rungs are ordered so each one's *defense* motivates the next one's *attack*:

| Rung | Attack | The bug | Defeated by → forces next rung |
|----:|--------|---------|--------------------------------|
| 1 | **Stack smash → shellcode** | overflow a stack buffer, jump to injected code on an executable stack | **NX/DEP** (can't run stack data) → reuse existing code |
| 2 | **ret2libc** | overwrite the return address with `system("/bin/sh")` | **ASLR** (libc base unknown) → need a leak; and finer-grained reuse |
| 3 | **ROP** | chain `ret`-terminated gadgets into `execve("/bin/sh")` | **ASLR/PIE, canary, CET shadow stack, CFI, seccomp** |
| 4 | **Format string `%n`** | `printf(user)` → arbitrary write, no overflow at all | **`-Werror=format-security`, FORTIFY, RELRO, ASLR** |

> ### ⚠️ For your own machines and authorized targets only
> Every binary here is **deliberately vulnerable**. The exploits only work
> because *you* compiled the target with mitigations turned **off** and (for
> rungs 1–2) disabled ASLR on *your* box. This is legal, on-your-own-hardware
> learning. Do not point any of this at software or systems you do not own and
> have permission to test. The most important half of each rung is the
> **defense** section — that is the transferable, real-world lesson.

This is an honest **teaching core**, not a weaponized framework: the targets
hand out a convenience leak or a planted gadget set where doing so isolates the
concept being taught. Each such simplification is called out in the code and
below, along with what a real attack would have to do instead.

## What you'll learn

- **Stack frame layout**: where a local buffer sits relative to the saved `rbp`
  and the **saved return address**, and why the overflow offset is *measured*,
  not guessed (the reference build's naive "72" is really **88** for `demo.c`,
  and **72** for the targets — same source shape, different frames).
- **NX / DEP**: why an executable stack is required for rung 1 and how marking
  it non-executable kills shellcode injection outright.
- **The GOT/PLT and dynamic linking**: how `system` and `"/bin/sh"` are found in
  a live libc mapping, and why **ASLR** turns a one-shot exploit into a two-stage
  leak-then-exploit.
- **Return-Oriented Programming**: what a "gadget" is, how a tiny finder locates
  them by scanning for `ret` (`0xC3`) bytes, and how a chain becomes a program
  that NX cannot stop.
- **Format-string exploitation**: the varargs ABI, how `%n` becomes a
  write-what-where, and the two-`%hn` trick for writing a 32-bit value.
- **The mitigation stack**: NX, ASLR, stack canaries, RELRO, PIE, CFI/CET,
  seccomp — what each one defends, and its cost to attacker and defender.

## Build & run (Linux / WSL required to run; asm builds anywhere)

```bash
make                 # build all four targets with their exact mitigation flags
make aslr-off        # echo 0 > /proc/sys/kernel/randomize_va_space  (needs sudo)

python3 exploits/exploit_rung1_stack.py       # shellcode on the stack
python3 exploits/exploit_rung2_ret2libc.py    # system("/bin/sh")
python3 exploits/exploit_rung3_rop.py         # execve via a ROP chain
python3 exploits/exploit_rung4_fmt.py         # %n arbitrary write

make run             # shortcut: build rung 3 and pop a shell via ROP
make test            # non-interactive smoke test of the ROP chain
make harden          # rebuild rung 1 with mitigations ON — watch the exploit FAIL
make aslr-on         # restore full ASLR when you're done
```

No pwntools required — `exploits/common.py` is a from-scratch mini-pwntools so
every primitive (packing, the cyclic offset finder, ELF/`/proc` introspection)
is visible. It shells out to standard binutils (`nm`, `readelf`, `ldd`).

**Measuring an offset yourself** (the skill the whole ladder rests on):

```bash
python3 exploits/common.py cyclic 200 | ./targets/vuln_stack   # crash it
# read the 8 bytes it tried to `ret` into (gdb: x/gx $rsp-8), then:
python3 exploits/common.py cyclic-find 0x616161616161616a     # -> 72
```

## How it works (file by file)

```
targets/     the deliberately-vulnerable programs (heavy WHY-comments)
  vuln_stack.c     rung 1: read() overflow, leaks &buf, executable stack
  vuln_ret2libc.c  rung 2: same overflow, NX on, dynamic libc
  vuln_rop.c       rung 3: same overflow + a planted gadget farm & "/bin/sh"
  vuln_fmt.c       rung 4: printf(user) with a global to overwrite via %n
exploits/    plain-Python exploits + the shared toolkit
  common.py        p64/u64, De Bruijn cyclic(+find), nm/readelf/ldd, /proc maps
  ropgadget.py     a tiny gadget finder (scan for ret, decode backward)
  exploit_rung1_stack.py … exploit_rung4_fmt.py
asm/         the teaching assembly for the vulnerable function
  demo.c  demo.O0.s  demo.s  demo.O2.s  demo.annotated.s
```

- **Rung 1 — stack smash.** `vuln()` `read()`s 1024 bytes into a 64-byte buffer.
  The exploit sends `shellcode + NOP pad + &buf`; the overwritten return address
  is `&buf`, so `ret` jumps into the 24-byte `execve("/bin//sh")` shellcode. The
  target leaks `&buf` as a stand-in for a real info leak (a separate bug, or gdb
  with ASLR off). Because the leak makes the address exact, this rung isolates
  the **NX** lesson: turn the stack non-executable and it dies.
- **Rung 2 — ret2libc.** NX is on, so we don't inject code — we return into
  libc's `system` with `rdi = &"/bin/sh"`. The exploit reads the target's own
  `/proc/<pid>/maps` for the libc base, gets `&system` from `nm -D`, finds
  `"/bin/sh"` inside the live libc memory, and finds a `pop rdi ; ret` gadget in
  libc. A bare `ret` is inserted for 16-byte stack alignment (glibc's `movaps`).
- **Rung 3 — ROP.** `ropgadget.py` scans the no-PIE binary for `pop rdi/rsi/rdx/
  rax ; ret` and `syscall ; ret`; `nm` locates the planted `binsh_str`. The chain
  loads the four `execve` argument registers and fires `syscall`. Everything is
  at fixed addresses (no-PIE `.text` is never randomized), so no leak is needed —
  which is exactly why the **defense** here is to *reintroduce* randomness and
  return-address integrity.
- **Rung 4 — format string.** `printf(line)` lets `%n` write "chars printed so
  far" through a pointer we plant in `line`. `build_write()` emits an
  all-positional format string that stores `0xDEADBEEF` as two `%hn` halves
  (`0xBEEF` then `0xDEAD`), with the destination addresses parked at the end so
  their NUL bytes don't truncate the directives. The positional offset is
  auto-detected by leaking a marker. Flipping the global trips `win()`.

### The DEFENSE for each rung (the real lesson)

**Rung 1 — stack shellcode**
- **NX / DEP** (default today): the stack page is non-executable; the CPU faults
  on the first shellcode byte. *Cost:* the attacker must reuse existing
  executable code (→ rung 2/3). This single bit ended an entire exploit class.
- **Stack canary** (`-fstack-protector-strong`): a random word between the buffer
  and the saved return address is checked before `ret`; the overflow corrupts it
  → `__stack_chk_fail` aborts. *Cost:* the attacker must leak or avoid the canary.
- **ASLR** (`randomize_va_space`): `&buf` is unpredictable. *Cost:* an info leak
  is now a prerequisite (this target hands one out; remove it and ASLR bites).

**Rung 2 — ret2libc**
- **ASLR**: randomizing libc's base makes `&system` / `&"/bin/sh"` unknown — *the*
  mitigation for this rung. *Cost:* a libc-address leak first (one-shot → two-stage).
- **Full RELRO + BIND_NOW**: GOT resolved at startup and made read-only, killing
  the related GOT-overwrite technique. *Cost:* fewer writable, useful anchors.
- **CET-IBT / CFI**: an indirect transfer into the middle of `system` (not its
  `ENDBR64` entry) is rejected. *Cost:* need entry-valid / call-oriented gadgets.

**Rung 3 — ROP**
- **PIE + ASLR**: gadget addresses randomize per run. *Cost:* a leak first.
- **Stack canary**: the overflow is caught before the first `ret` — the chain
  never starts. *Cost:* leak/avoid the canary.
- **Intel CET shadow stack**: a write-protected second copy of every return
  address; each `ret` is checked against it, so a `ret` into a gadget faults
  (`#CP`). *Cost:* ret-based ROP is dead; pivot to harder JOP/COP if possible.
- **seccomp** (see `../12-syscall-sandbox`): even a perfect `execve` chain is
  denied by a syscall filter. *Cost:* must reach the goal within allowed syscalls.

**Rung 4 — format string**
- **`-Werror=format-security` / `-Wformat-security`**: the compiler *refuses to
  build* `printf(line)`. The single most effective fix, at zero runtime cost —
  always write `printf("%s", line)`. This is the blue-team takeaway.
- **`_FORTIFY_SOURCE=2`**: glibc aborts at runtime on a `%n` in a writable-memory
  format string. *Cost:* the `%n` write primitive is gone.
- **Full RELRO**: read-only GOT defeats the classic "aim `%n` at a GOT entry"
  variant. *Cost:* another writable target must be found.
- **ASLR / PIE**: the address to aim at is unknown without a leak.

`make harden` rebuilds rung 1 with canary + NX + PIE + RELRO + FORTIFY so you can
run the rung-1 exploit against it and see the abort/fault for yourself.

## Assembly notes

`asm/demo.c` is the vulnerable function distilled to pure logic — an unbounded
`my_strcpy` (== libc `strcpy`, bug and all) into a fixed 64-byte buffer, with no
system headers so the frame is the only thing on screen. The generated files:

- [`asm/demo.O0.s`](asm/demo.O0.s) — `-O0`, the most literal frame: `subq $80,%rsp`,
  `leaq -80(%rbp),%rdi` puts `buf` at `rbp-80` (the incoming `attacker` pointer is
  spilled to `rbp-8`, *above* `buf`, which is what pushes `buf` down).
- [`asm/demo.s`](asm/demo.s) — `-O1`, the annotation baseline: here `pushq %rbx`
  + `subq $72,%rsp` also places `buf` at `rbp-80`.
- [`asm/demo.O2.s`](asm/demo.O2.s) — `-O2`, for comparison.
- [`asm/demo.annotated.s`](asm/demo.annotated.s) — **the deliverable**: every
  instruction commented, with a frame map that points at the **saved return
  address** and derives the overflow offset directly from the encoded
  instructions (`88` for `demo.c`, because an incoming pointer arg is spilled
  above `buf`). The key lesson, verified against `objdump`, is spelled out there:
  the offset is **not** "buffer size + 8" — alignment slack and saved registers
  move it, so you *measure* it with the cyclic pattern.

Regenerate the compiler outputs with `make asm` (any host — clang cross-targets
Linux); the hand-annotated file is authored and never overwritten.

## Going further (the `Stretch:` goals)

- **A ROP-chain compiler.** `exploits/ropgadget.py` finds gadgets; the natural
  next step is a small *compiler* that, given a goal like `execve("/bin/sh")`,
  searches the gadget set and emits the chain automatically (register-allocation
  over gadgets, handling multi-pop gadgets and stack-alignment constraints). This
  is what `ropper --chain` / angrop do.
- **Heap exploitation (tcache / fastbin).** This project is stack- and
  format-string-centric. The heap ladder is its own world: use-after-free and
  double-free into glibc's **tcache** (poison the singly-linked free list to
  return an arbitrary chunk) or **fastbin** attacks, then tcache/fastbin
  "dup" to allocate over a chosen address. A follow-on project would ship a
  vulnerable allocator harness plus a tcache-poisoning walkthrough.
- **What production does.** Real defense stacks *combine* these: PIE + full ASLR
  + `-fstack-protector-strong` + full RELRO + FORTIFY + CET (IBT + shadow stack)
  + a seccomp allowlist. Each is cheap alone; together they force an attacker to
  chain multiple independent bugs (a leak *and* a write *and* a way around CFI).

## References

- Aleph One, *Smashing the Stack for Fun and Profit* (Phrack 49) — rung 1.
- Solar Designer, return-into-libc (Bugtraq, 1997) — rung 2.
- H. Shacham, *The Geometry of Innocent Flesh on the Bone* (CCS 2007) — ROP.
- scut/team-teso, *Exploiting Format String Vulnerabilities* — rung 4.
- `man 3 printf` (positional args, `%n`), `man 2 execve`, `man 2 mprotect`.
- Intel CET (shadow stack + IBT); the System V AMD64 psABI for register/stack rules.
- This repo: `../09-shellcode` (the shellcode), `../12-syscall-sandbox` (seccomp
  as the last-line defense against rung 3).
