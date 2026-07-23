# A regex engine 🟧

**What it is.** A small but genuine **linear-time** regular-expression engine: it
parses a pattern into an AST, compiles the AST to a **Thompson NFA** (a tiny
bytecode with epsilon transitions), and matches by simulating the NFA over the
input while tracking the **set of active states as a bitset**. Because it
advances *all* possible states one input byte at a time instead of trying paths
and backing up, it runs in **O(n · m)** (n = input length, m = pattern size) and
is **immune to catastrophic backtracking (ReDoS)** — the class of bug that makes
Perl/PCRE/Python/JavaScript regexes hang for seconds on a 30-character string.
The engine ships as a grep-like CLI and is checked for correctness against
`grep -E`. Difficulty: 🟧 (intermediate).

This is a teaching *core*, honestly scoped: it implements concatenation,
alternation `|`, the quantifiers `* + ?`, grouping `( )`, the wildcard `.`,
bracket classes `[...]`/`[^...]` with ranges, and the escapes `\d \w \s`
(and `\D \W \S`, `\n \t` …). It reports **match / no-match** and, for the CLI,
which *lines* match. It does **not** implement capture groups, backreferences
(which are what force backtracking and are provably not regular), anchors
`^ $`, or POSIX named classes `[[:alpha:]]`. See **Going further** for how the
production engines (RE2) add captures while keeping the linear-time guarantee.

## What you'll learn

- **Automata theory made concrete**: Thompson's construction (regex → NFA), the
  role of **epsilon transitions**, and NFA **simulation by subset tracking** —
  the same idea as on-the-fly DFA construction, minus the state cache.
- **Why backtracking explodes and this does not.** A backtracker explores a tree
  of choices whose size is exponential in the input; a state-set simulation
  keeps at most `m` states, so one input byte is O(m) work, full stop.
- A compact **NFA bytecode VM** (`OP_CONSUME / OP_SPLIT / OP_JMP / OP_MATCH`) —
  the representation RE2 and PCRE use internally.
- **Bit-parallel systems technique**: representing a state set as a bitset and
  iterating only the set bits with `TZCNT` + `BLSR` (`bits & (bits-1)`).
- **Memory discipline**: an *arena* for the throwaway AST (one `free()` reclaims
  all nodes), Structure-of-Arrays layout for cache-friendly scanning, and a
  matcher that performs **zero allocation per input** (all scratch preallocated).

## Build & run

Portable ISO C11 — builds on **Linux, WSL, and the msys2 clang on Windows**.
`make asm` works on any host because clang cross-targets Linux.

```bash
make            # build ./regex
make test       # run the built-in correctness self-test (37 match cases + error cases)
make bench      # show linear scaling and no-ReDoS behavior
make run        # a sample grep over the Makefile

# Use it like grep:
./regex '[0-9]+' file.txt            # print lines containing a number
./regex -n '(foo|bar)' file.txt      # with line numbers
./regex -c '[A-Za-z_]\w*' file.txt   # count matching lines
./regex -x '[a-z]+' file.txt         # whole-line (anchored) match
./regex -v error log.txt             # invert: lines WITHOUT a match
printf 'a\nbb\nccc\n' | ./regex '.{0,0}' # reads stdin when no file is given

./regex --selftest                   # correctness checks
./regex --bench                      # timing demonstration
```

Cross-check correctness against the system engine (they should agree on every
pattern this engine supports):

```bash
diff <(./regex '[0-9]+' file.txt) <(grep -E '[0-9]+' file.txt)
```

## How it works

The pipeline is four files, one stage each, so every stage is legible alone:

| file | role | key idea |
|------|------|----------|
| `regex.h` | public API + the NFA bytecode (`Prog`) and scratch types | four opcodes encode any regex; SoA layout |
| `ast.h` / `parser.c` | pattern string → **AST** (recursive descent) | one function per grammar rule; **arena**-allocated nodes |
| `nfa.c` | AST → **Prog** (Thompson construction) + lifecycle | fragments that "fall out the bottom", so a CONSUME's success edge is always `pc+1` |
| `sim.c` | `Prog` + text → match? (**bitset simulation**) | epsilon-closure + the pure `nfa_step` transition; **no libc, no allocation** |
| `main.c` | grep-like CLI, self-test, benchmark | the only file that does I/O |

**Parsing (`parser.c`).** Standard recursive descent with the usual precedence:
`alternation → concatenation → repetition → atom`. Every byte-consuming atom — a
literal, `.`, an escape, or a bracket class — collapses to one node kind,
`N_SET`, carrying a **256-bit membership bitmap**. Unifying them means the
compiler and simulator each have exactly one consuming case to handle. Nodes come
from a **bounded arena** sized `6·len+16`; a parse error just frees the whole
pool, so no error path can leak.

**Compilation (`nfa.c`).** Each AST node emits a Thompson *fragment*. The layout
invariant is that a fragment occupies a contiguous instruction range and control
"falls out the bottom", so:
- concatenation `a·b` is literally *emit a, emit b* — no glue;
- alternation, `*`, `+`, `?` introduce **`OP_SPLIT`** (an epsilon fork into two
  states) and **`OP_JMP`** to close loops;
- a consuming instruction's success edge is always the next instruction (`pc+1`),
  so we never store it.

**Simulation (`sim.c`).** The active state set is a **bitset** (one bit per
instruction). One step is two halves:
1. **transition** (`nfa_step`, the star of the show): for each active `OP_CONSUME`
   state whose 256-bit set contains the input byte, set the successor bit in a
   fresh "seed" set — pure bitset arithmetic;
2. **epsilon-closure** (`add_state`): expand the seeds through `OP_SPLIT`/`OP_JMP`
   edges, using the bitset itself as the visited-marker so cycles from `a*`
   terminate. An explicit stack (no recursion) bounds memory to `nstate`.

`re_fullmatch` is anchored (accept iff `OP_MATCH` is active after the *whole*
input is consumed); `re_contains` is grep-style (re-seed the start state at each
position, accept as soon as `OP_MATCH` appears). Both are O(n·m) and never
allocate — the `Scratch` is sized once and reused across every line and file.

### Why backtracking blows up and this doesn't

Consider `(a+)+b` against `"aaaa…aaac"`. A backtracker must decide how to split
the run of `a`s between the inner `a+` and the outer `+`; there are **2^(n-1)**
such splittings, and because the trailing `c` makes the match fail, it tries
*all* of them before giving up — exponential time. This engine never "decides":
at each `SPLIT` it keeps **both** successors in the state set. The set has at
most `m` members, so the whole scan is `n · m`. `make bench` matches `(a+)+b`
against up to 512 000 `a`s in ~10 ms and the time doubles when `n` doubles —
linear, exactly as the theory predicts, on an input that would out-live the
universe under a backtracker.

## Assembly notes

The annotated deliverable is [`asm/demo.annotated.s`](asm/demo.annotated.s): the
`-O1` compilation of `nfa_step` — the state-set transition inner loop — with a
comment on essentially every instruction, plus a header block on the System V
AMD64 ABI, the prologue/epilogue, and the seventh argument arriving on the stack.
`asm/demo.c` is a **self-contained** extraction of that loop (its own types, no
headers) so a bare cross-compiler can build it; the logic is identical to the
real `nfa_step` in `sim.c`.

What the annotation highlights on this project's hot loop:

- **Bit iteration** compiles to `rep bsf` (**TZCNT**, lowest set-bit index) plus
  `lea -1; and` (**BLSR**, `bits & (bits-1)` to clear it) — so the loop visits
  only *active* states, not all `m`.
- **Per-byte constant hoisting**: the membership mask `1 << (c & 63)` and the
  word index `c >> 6` depend only on the input byte, so clang computes them
  **once** before the loops and turns the 256-bit set test into a single
  `test class_word, mask`.
- **Shrink-wrapping**: the `nwords <= 0` early-out is emitted *before* the
  prologue, so a no-op call saves/restores nothing.
- In `demo_run`, clang **constant-folds** the entire hand-built match at compile
  time (four `movaps` to fill the class table, then `mov $2` for the answer) — a
  vivid reminder that with all inputs known the optimizer runs your algorithm at
  build time.

Because the real matcher `sim.c` uses only *freestanding* headers and no libc
calls in the hot path, its assembly is generated too — compare
[`asm/sim.s`](asm/sim.s) (the real `nfa_step`, `add_state`, `re_fullmatch`) with
the extracted `asm/demo.s`. One teaching wrinkle: clang's loop-idiom recognition
rewrites the tiny zeroing loop `for(i)w[i]=0` into a `call memset@PLT`, so `sim.s`
is not *link*-freestanding even though it *compiles* with no system headers.

Regenerate everything with `make asm` (works on any host):
- [`asm/demo.O0.s`](asm/demo.O0.s) — naive mapping, both hoisted constants
  recomputed every iteration, every value spilled to the stack.
- [`asm/demo.s`](asm/demo.s) — `-O1`, the annotated baseline.
- [`asm/demo.O2.s`](asm/demo.O2.s) — `-O2`: BMI `blsr`, aggressive scheduling.

## Going further (the `Stretch:` from the list)

- **On-the-fly DFA (cached subset construction).** The simulation already *is* a
  subset construction; memoize each distinct active-set → next-set transition in
  a hash-keyed cache and you have RE2's lazy DFA, which amortizes the epsilon
  closure to near one comparison per byte. Bound the cache and evict under memory
  pressure — that bound is the whole engineering problem.
- **Leftmost-longest submatch (captures) without backtracking.** Add `OP_SAVE`
  slots and carry a small array of positions per thread (the *Pike VM*), choosing
  thread priority to realize greedy/lazy semantics. This is how RE2 reports
  capture groups while staying linear.
- **Anchors, counted repetition `{m,n}`, Unicode.** `^`/`$` are cheap epsilon
  assertions; `{m,n}` desugars to copies + `?`; UTF-8 means classes over
  codepoints, not bytes.
- **Read the real thing.** Russ Cox's series *"Regular Expression Matching Can Be
  Simple And Fast"* and the **RE2** source are the canonical references; this
  engine is a deliberately minimal cousin of them.

## References

- Ken Thompson, *"Regular Expression Search Algorithm"*, CACM 11(6), 1968 — the
  construction implemented in `nfa.c`.
- Russ Cox, *"Regular Expression Matching Can Be Simple And Fast"*
  (swtch.com/~rsc/regexp) — the modern write-up; the `Prog`/bytecode design here
  follows its `re1`.
- Navarro & Raffinot, *Flexible Pattern Matching in Strings* — bit-parallel NFA
  simulation (the bitset step).
- **RE2** (github.com/google/re2) — a production linear-time engine with the DFA
  cache and captures described in *Going further*.
- `man 7 regex`, `man 1 grep`; the Intel SDM entries for `TZCNT` / `BLSR`.
