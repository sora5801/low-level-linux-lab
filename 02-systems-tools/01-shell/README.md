# A Unix shell 🟧

**What it is.** A real, interactive Unix shell with the piece most tutorials skip:
**job control**. It tokenizes and parses a command line; runs programs with
`fork` + `execvp` + `waitpid`; builds pipelines with `pipe`/`dup2`; does `<`,
`>`, `>>`, `2>` redirection; expands `$VAR`, `~`, and `* ? []` globs; ships the
builtins `cd`/`exit`/`export`/`jobs`/`fg`/`bg`; and — the interesting part —
manages **process groups** (`setpgid`), hands the terminal to the foreground job
(`tcsetpgrp`), and choreographs `SIGTSTP`/`SIGINT`/`SIGCONT` so that Ctrl-Z
stops a job, `bg` resumes it detached, and `fg` brings it back. It is a genuine
working shell, not a toy REPL; the honest scope limits are listed under
[Going further](#going-further-the-stretch).

Difficulty: 🟧 (the job-control terminal/signal dance is the hard part).

## What you'll learn

- **Process creation & replacement:** `fork(2)`, `execvp(3)`, `_exit(2)`, and why
  a failed `exec` must `_exit` rather than `return`.
- **Pipes:** `pipe(2)` + `dup2(2)` to chain stdout→stdin, and the fd-hygiene rule
  that makes readers actually see EOF.
- **Redirection:** `open(2)` flag sets (`O_CREAT|O_TRUNC` vs `O_APPEND`) and
  `dup2` onto fds 0/1/2, including save/restore so a builtin can be redirected.
- **Job control:** `setpgid(2)` (and the fork/exec race that makes you call it in
  *both* parent and child), `tcsetpgrp(3)`/`tcgetpgrp(3)` to move terminal
  ownership, and `waitpid(2)` with **`WUNTRACED`** so a *stopped* child is
  reported, not just an exited one.
- **The terminal signal posture:** why an interactive shell sets
  `SIGINT`/`SIGQUIT`/`SIGTSTP`/`SIGTTIN`/`SIGTTOU` to `SIG_IGN`, and why children
  reset them to `SIG_DFL` before `exec`.
- **Terminal modes:** `tcgetattr`/`tcsetattr` to save and restore the line
  discipline, so a program that stopped in raw mode can't wreck your prompt.
- **A hand-rolled lexer:** quoting (`'…'`, `"…"`), backslash escapes, and inline
  `$` expansion done *during* scanning because they depend on character context.
- **Glob matching as pure logic:** the two-pointer `*` backtracking that is
  linear, not exponential — the routine we lift into the assembly deliverable.

## Build & run (Linux / WSL)

The shell is **Linux-only** — it relies on POSIX process groups, `tcsetpgrp`, and
the job-control signals. Build and run it on Linux or WSL.

```bash
make            # cc -Wall -Wextra -O2 -std=c11 … -o mysh   (builds clean, no warnings)
./mysh          # start an interactive session
```

Try it out:

```text
mysh$ ls | wc -l
mysh$ echo hello > out.txt ; cat < out.txt
mysh$ echo $HOME and $USER, status was $?
mysh$ ls *.c
mysh$ sleep 30 &          # background: prints "[1] <pgid> running &"
mysh$ jobs                # list jobs
mysh$ sleep 30            # then press Ctrl-Z ...
^Z[1] <pgid> stopped   sleep 30
mysh$ bg                  # ... resume it in the background
mysh$ fg                  # ... or bring it back to the foreground
mysh$ exit
```

Non-interactive smoke test (pipes/redirection/globbing/builtins, no terminal):

```bash
make test
```

Regenerate the committed teaching assembly (needs `clang`; works on any host):

```bash
make asm
```

## How it works

The code is split into small, single-responsibility modules. Read them in this
order:

| File | Role |
|------|------|
| `shell.h`   | The data model: `token`, `redir`, `process`, `job`, `strvec`, and the global shell state. **Start here** — the structs *are* the mental model (`line → pipeline → job → processes`). |
| `main.c`    | The read/parse/execute loop and the definitions of the global state. Tiny on purpose. |
| `lexer.c`   | `lex_next` — the tokenizer. Handles `'…'`/`"…"` quoting, `\` escapes, and **inline `$VAR`/`${VAR}`/`$?`/`$$` and `~` expansion**, because those depend on quote context. Records `can_glob` per word. |
| `parser.c`  | `parse_line` — turns the token stream into one `job` (a pipeline of `process`es with redirections). Also `free_job`, the single owner of the teardown. |
| `expand.c`  | `glob_expand` — the filesystem half of globbing: walk a directory with `opendir`/`readdir`, test each name with `wildcard_match`, sort the hits. "Nullglob off": no match ⇒ keep the literal word. |
| `match.c`   | `wildcard_match` — pure glob matching with `* ? [] \`, iterative with `*` backtracking. **No system headers**, so its assembly is committed (`asm/match.s`). |
| `exec.c`    | `launch_job` — `fork`/`pipe`/`dup2`/`execvp` the pipeline, plus the child-side `setpgid`/signal-reset and redirection application. |
| `jobs.c`    | **The heart.** `init_shell` (claim our process group + the terminal), the `waitpid(WUNTRACED)` reaping loops, `put_job_in_foreground`/`background`, and `do_job_notification`. |
| `builtins.c`| `cd`/`exit`/`export`/`jobs`/`fg`/`bg`, run in the shell process so they can change it. |
| `util.c`    | `xmalloc`/`xstrdup` (abort-on-OOM) and the `strvec` growable vector. |

**The job-control dance, concretely.** When you run `vim`:
1. The shell `fork`s; both parent and child `setpgid(child, child)` (racing on
   purpose so neither order breaks).
2. The parent `tcsetpgrp(term, pgid)` — now `vim`'s group owns the terminal, so
   Ctrl-C/Ctrl-Z go to `vim`, not the shell.
3. The child resets the job-control signals to `SIG_DFL` and `execvp`s `vim`.
4. The shell `waitpid(-1, …, WUNTRACED)` — it wakes when `vim` **exits** *or*
   **stops** (Ctrl-Z). On a stop it reclaims the terminal with `tcsetpgrp` and
   restores its own saved `termios`, so your prompt still works.
5. `fg`/`bg` later `kill(-pgid, SIGCONT)` to resume the stopped group, with or
   without handing back the terminal.

There is **no `SIGCHLD` handler**: reaping is synchronous (`waitpid` blocking for
the foreground job, `WNOHANG` polling before each prompt for background ones),
which sidesteps every async-signal-safety hazard.

## Assembly notes

The assembly deliverable focuses on the shell's most instructive *pure-logic*
routine — glob matching — because it is header-free and compiles to clean,
readable code.

- **`asm/demo.c`** is a self-contained extraction: `glob_star`, the `*`/`?`/`\`
  matcher with the two-pointer backtracking, plus a `demo_run` driver.
- **`asm/demo.annotated.s`** annotates the `-O1` output (`asm/demo.s`) instruction
  by instruction. It keeps clang's real `.LBB0_*` labels (their meaning is given
  in the header and in each label's comment) so it **diffs clean against
  `demo.s`**. The lessons it draws out:
  - The `*` backtracking uses **no recursion and no stack** — `star_pat`/`star_str`
    live in `rax`/`rcx`, and a mismatch just rewrites the `pat`/`str` cursors.
    Because `star_str` only ever moves forward, the loop is **O(|pat|·|str|)**,
    never exponential.
  - The optimizer packed the input byte `c` **and** a "keep going?" boolean into
    the single register `dl`, and fused the trailing-`*` loop into one
    read-advance-test loop.
  - At `-O1`, `demo_run` **inlines `glob_star` four times** (one per call site)
    rather than emitting four `call`s — visible as four copies of the same loop.
- **`asm/match.s`** is the assembly of the **real** project file `match.c` (the
  full matcher *including* `[a-z]` bracket classes), generated with the same
  flags. Compare it to `demo.s` to see how the class scan compiles.
- **`asm/demo.O0.s`** (spill-everything, easiest to trace) and **`asm/demo.O2.s`**
  (tighter scheduling) round out the optimization-level comparison.

All `.s` files are genuine `clang --target=x86_64-pc-linux-gnu` output; regenerate
them with `make asm`.

## Going further (the `Stretch`)

**Stretch goal — raw-termios line editing.** Replace the plain `getline` prompt
with a real line editor: put the terminal in raw mode (`tcsetattr` with `ICANON`
and `ECHO` cleared), read byte by byte, and implement cursor movement, a history
ring you scroll with the arrow keys, and Tab completion against `$PATH` and the
filesystem — i.e. what `readline`/`libedit` do. The signal plumbing here already
does the hard half (restoring modes when a job stops), so this slots in at the
`main.c` read step.

**What this teaching core does _not_ cover** (all deliberately, to keep it
legible — each is a natural next project):

- **Sequencing:** one pipeline per line only. No `;`, `&&`, `||`, or subshells
  `( … )`.
- **More expansion:** no command substitution `$(…)`, no arithmetic `$((…))`, no
  here-docs `<<`, and an expanded `$VAR` is **not** re-split on `$IFS` or
  re-globbed (it is inserted as one literal chunk).
- **Globbing scope:** only the last path component is globbed (`dir/*.c` works,
  cross-directory patterns do not), and quoting *any* part of a word disables
  globbing for the whole word (bash globs the unquoted parts). Both are noted at
  their code sites.
- **Redirection:** `<`, `>`, `>>`, `2>` only — no fd duplication (`2>&1`) and no
  input here-strings.
- A production shell also handles `SIGCHLD` asynchronously, tracks the terminal's
  session leader, supports `wait`/`disown`/`kill %n`, and reuses freed low job
  numbers.

## References

- **The canonical source:** the GNU libc manual, *"Implementing a Job Control
  Shell"* — this project's job-control structure follows it closely on purpose.
- `man 2 fork`, `man 3 execvp`, `man 2 waitpid` (note `WUNTRACED`/`WNOHANG`),
  `man 2 dup2`, `man 2 pipe`.
- `man 2 setpgid`, `man 3 tcsetpgrp`, `man 3 tcgetpgrp`, `man 3 tcsetattr`.
- `man 7 signal` and `man 7 credentials` for process-group / terminal-signal
  semantics; `man 7 glob` for the pattern rules.
- Read the source of the real thing: `bash` (`jobs.c`, `execute_cmd.c`) and the
  much smaller `dash` (`jobs.c`) — both are readable once you know the dance
  above.
