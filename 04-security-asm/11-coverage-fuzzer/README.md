# Coverage-guided fuzzer (mini-AFL) 🟥

**What it is.** A working, teaching-core coverage-guided fuzzer in the shape of
[AFL](https://github.com/google/AFL) / [AFL++](https://github.com/AFLplusplus/AFLplusplus):
compile-time **edge-coverage instrumentation** feeds a **shared-memory bitmap**,
a **fork server** in the target re-executes test cases without paying `execve`
each time, a **mutation engine** (bit/byte flips, arithmetic, interesting
values, block ops, splicing, dictionary) churns an input corpus, a **feedback
loop** keeps only the inputs that reach *new* edges, and **crash triage** via
`waitpid`/signal (optionally ASan) saves the faults. Pointed at a tiny record
parser with a planted stack overflow, it climbs the parser's magic-byte ladder
and finds the crash in seconds. That "keep what reaches new code" loop is the
one idea behind every modern fuzzer.

> ### ⚠️ On your own machines only
> The default `parser` build is **deliberately vulnerable** (a real
> stack-buffer overflow, mitigations turned off). Compile and fuzz it **only on
> hardware you own or are explicitly authorized to test.** Nothing here reaches
> the network or touches another process. This project is framed **defensively**:
> the lesson is that *you* run the fuzzer, in CI, on your own code, and find the
> bug before an attacker does — with a stack trace, on your terms. See
> **[Defense](#defense-the-blue-team-half)** below.

## What you'll learn

- **Compile-time instrumentation.** How `-fsanitize-coverage=trace-pc-guard`
  makes clang insert a callback at every control-flow edge, and how to implement
  `__sanitizer_cov_trace_pc_guard{,_init}` yourself (`rt.c`).
- **The AFL edge hash** — the "heart": `idx = (cur_loc ^ prev_loc) & (MAP_SIZE-1)`,
  `map[idx]++`, `prev_loc = cur_loc >> 1`. *Why* the XOR, *why* the shift, *why*
  the map is a power of two. This is the annotated `asm/demo.c`.
- **SysV shared memory** (`shmget`/`shmat`/`shmctl`) as a zero-copy channel: the
  target writes coverage, the fuzzer reads it in another address space for free.
- **The fork-server protocol** (`fork`/`execv`/`dup2`, fds 198/199, `waitpid`):
  exec the target once, then fork a fresh COW child per case — the single biggest
  speed win in fuzzing.
- **Coverage feedback & hit-count bucketing**: `classify_counts` + `has_new_bits`
  — turning raw edge counts into "did we see genuinely new behaviour?".
- **Crash triage**: decoding `waitpid` status (`WIFSIGNALED`/`WTERMSIG`), telling
  SIGSEGV from a stack-canary SIGABRT from an ASan abort; hang detection via `poll`.

## Build & run (Linux / WSL)

Building and running need Linux (fork, SysV shm, `waitpid`, `poll`). Assembly
regeneration works on any host.

```bash
make            # builds ./fuzzer and the vulnerable ./parser
make run        # fuzz it; prints stats and announces the first crash
# [+] crash found after N execs — saved in out/crashes/
# [+] reproduce with:  ./parser out/crashes/<id>

./parser out/crashes/<id>          # reproduce standalone -> Segmentation fault
```

See each mitigation change the result:

```bash
make run-asan   # builds ./parser_asan and fuzzes it; crashes become precise,
                # symbolized "stack-buffer-overflow in parse_name" reports
make parser_hardened && ./parser_hardened out/crashes/<id>
                # canary trips: "*** stack smashing detected ***" -> SIGABRT
```

One-shot smoke test (bounded, asserts a crash was found):

```bash
make test       # timeout 30 ./fuzzer ...; PASS iff out/crashes/ is non-empty
```

Regenerate the committed teaching assembly (any OS):

```bash
make asm        # clang -S at -O0/-O1/-O2 from asm/demo.c
```

## How it works

A coverage-guided fuzzer is two processes sharing three kernel objects (a shm
segment + two pipes). `forkserver.h` pins the contract between them.

| File | Role |
|------|------|
| `forkserver.h` | The wire protocol: `MAP_SIZE` (64 KiB), the shm env-var name, control/status fds **198/199**, and the one function the target calls to become a fork server. Heavily commented — read it first. |
| `rt.c` | The **target-side runtime** (AFL's `afl-compiler-rt` analogue). Implements the two `trace_pc_guard` callbacks (the edge hash), attaches the shm bitmap, and runs the fork server. **Built without coverage** so its own edges never pollute the map. |
| `parser.c` | The **target**: a tiny `FZR1` record parser with a planted 16-byte stack overflow (`parse_name`) hidden behind a 4-byte magic + type check — the "ladder" coverage feedback climbs. |
| `fuzzer.c` | The **driver** (`afl-fuzz` analogue): SysV shm setup, fork-server launch, the seedable PRNG, `classify_counts`/`has_new_bits` feedback, the full havoc + splice + dictionary mutation engine, crash/hang saving, and the main loop. |
| `asm/demo.c` | Self-contained extraction of the **coverage-bitmap update + feedback** for the assembly walkthrough. |
| `seeds/` | Starting inputs that deliberately **lack** the magic, so you watch the fuzzer discover it. |
| `dict.txt` | Optional AFL-style dictionary (includes the `FZR1` token). |

**The execution cycle** (`fuzzer.c` → `run_target`): zero the shm bitmap → write
the mutated bytes to `out/.cur_input` → send 4 bytes down the control pipe →
the fork server `fork()`s a child that reads the file and parses it → read the
child pid, then its `waitpid` status back up the status pipe. Then
`classify_counts` buckets the raw edge counts and `has_new_bits` asks whether any
bucket is new; if so the input joins the corpus, if the child died by signal it
is saved as a crash.

**Why it finds the bug fast.** The overflow sits behind `data[0..3] == "FZR1"`.
Blind random search would need ~2³² tries to guess the magic. Coverage-guided
search gets a *reward for each byte it gets right* — matching `'F'` opens a new
edge, so that input is saved and mutated further; then `'Z'`, then `'R'`, then
`'1'`; then a large length byte drives the unbounded copy past `char buf[16]`.
An exponential search becomes linear. Watching the `corpus=` counter tick up as
it climbs the ladder is the entire point.

**Scope — what this teaching core omits** (honestly, per convention):
- **No scheduler.** Real AFL weights the queue (favored/small/fast entries,
  energy assignment); we round-robin. This is the main simplification.
- **No deterministic stage / effector map / trimming / calibration**; we go
  straight to havoc. No CmpLog/RedQueen, no persistent-mode loop (one fork per
  case), no shared-memory test-case delivery (we use a file), no `-M/-S` parallel
  sync. The instrumentation, shm, fork server, feedback, mutation, and triage —
  the load-bearing ideas — are all real.

## Defense: the blue-team half

Coverage-guided fuzzing *is* a defensive practice, and this project shows both
sides of each mitigation so the lesson is concrete:

- **Stack canary** (`-fstack-protector-strong`, `parser_hardened`). The overflow
  overwrites a random guard word; the epilogue's check fails and the process
  aborts via `__stack_chk_fail` → **SIGABRT**. The bug is still *found* (good —
  that's your fuzzer catching it), but at runtime an attacker gets a clean abort,
  not a hijack. Cost to the attacker: they must now leak/guess the canary.
- **ASan** (`-fsanitize=address`, `parser_asan`). Redzones + shadow memory catch
  the **first** out-of-bounds byte and print exactly where (`parse_name`, the
  buffer, the write). This is the ideal triage signal and belongs in CI — fuzz
  the ASan build and every crash comes with a diagnosis.
- **NX, ASLR, PIE, RELRO** (the mitigation vocabulary from project 10). This
  fuzzer's job stops at *finding* the crash; those mitigations decide whether a
  found overflow is *exploitable*. `-no-pie` here just makes crash addresses
  easier to read while learning.

The takeaway: instrument and fuzz your own parsers, in CI, with ASan on. You
become the one who finds the crash.

## Assembly notes

`asm/demo.annotated.s` walks the `-O1` output of `asm/demo.c` line by line. The
highlights:

- **The edge hash is five instructions.** `cov_update` compiles to
  `xor` (fold the edge) → `movzwl %si` (mask to 16 bits) → `movzbl` (load the
  counter) → saturating `incb` → `shrl` (the `>>1`). Note the mask `& (MAP_SIZE-1)`
  is *free*: with `MAP_SIZE = 2¹⁶` it is just reading the low 16-bit sub-register
  `%si`, not an `and`/`div`. That is the entire reason the map is a power of two.
- **The compiler runs the algorithm for you.** In `demo_selftest` the block ids
  are constants, so clang folded the whole edge-hash chain at *compile time* into
  four fixed-offset `map[k]++` stores at offsets **10, 17, 30, 20** — exactly the
  buckets the hash `(cur ^ prev) & 0xFFFF` produces for the walk `10→20→20→30`.
  You can verify the AFL math by reading the disassembly.
- **Branchless class split.** `classify_count`'s final 64-vs-128 decision is a
  `cmovns` — "is the byte ≥ 128?" *is* "is the sign bit set?".
- Compare `asm/demo.O0.s` (every call real, every value spilled) with
  `asm/demo.O2.s` (aggressive) to see the optimizer's range.

## Going further (the `Stretch:` from the list)

- **A real LLVM pass.** Replace `-fsanitize-coverage=trace-pc-guard` with a
  custom `ModulePass` that inserts the `map[hash]++` inline (no call per edge) —
  this is what `afl-clang-fast` does and it is a large speedup. Add context
  sensitivity (hash in the call stack) or n-gram edges.
- **Persistent mode.** Loop inside one child over many inputs (`__AFL_LOOP`)
  to amortize even the `fork`. Deliver test cases through the shm instead of a
  file to drop the write/open/read per case.
- **Smarter mutation.** CmpLog/RedQueen (learn comparison operands to defeat
  magic bytes without a dictionary), input-to-state, structure-aware (grammar)
  mutators, and a real queue scheduler with energy assignment.
- **Better triage.** De-duplicate crashes by hashing the faulting call stack
  (via `ptrace`/`PTRACE_GETREGS` or a core dump), minimize the crashing input,
  and auto-bucket by crash type.

## References

- AFL technical whitepaper — the edge-coverage + fork-server design this mirrors.
- AFL++ source: `instrumentation/afl-compiler-rt.o.c` (the runtime), `src/afl-fuzz-*.c`
  (the driver), `docs/` (persistent mode, CmpLog).
- libFuzzer docs on `-fsanitize-coverage=trace-pc-guard` and the
  `__sanitizer_cov_trace_pc_guard` callback ABI.
- `man 2 shmget`, `man 2 shmat`, `man 2 fork`, `man 2 waitpid`, `man 2 poll`.
- Project `10-memory-corruption-ladder` (what an overflow like this becomes) and
  `12-syscall-sandbox` (confining a target you fuzz).
