# mini-git 🟧

**What it is.** A working reimplementation of git's *core object model* in ~1000
lines of heavily-commented C: a content-addressed object store where every blob,
tree, and commit is named by the SHA-1 of its own contents, zlib-compressed, and
filed under `.mygit/objects/xx/rest`. It has a staging index and the commands you
need to go from files to history: `init`, `hash-object`, `cat-file`, `add`,
`write-tree`, `commit-tree`/`commit`, `log`, `status`, and a working-tree-vs-index
`diff`. Because it computes the *exact* bytes git does, its object ids are
**byte-identical to git's** — real `git` can read a repository this program wrote
(demonstrated below). Difficulty 🟧 (medium): the concepts are simple, but getting
the hashing, tree serialization, and sort order right enough to match git is the
work.

This is a genuinely working teaching **core**, not a full git. See
[Scope & honesty](#scope--honesty) for exactly what it does and does not do.

## What you'll learn

- **Content addressing / Merkle trees.** Why "the name is the hash of the
  contents" makes objects immutable, deduplicated for free, and tamper-evident —
  and why that makes a commit a permanent snapshot of an entire tree.
- **git's object formats**, exactly: the `"<type> <size>\0<content>"` canonical
  form that gets hashed; the binary tree entry `"<mode> <name>\0<20 raw bytes>"`;
  the text commit; and the `HEAD` → `refs/heads/<branch>` ref indirection.
- **SHA-1 from scratch** (`sha1.c`): the 80-round compression function, the
  message schedule, big-endian I/O, and the padding rule — plus *why* SHA-1 is
  cryptographically retired and what git does about it.
- **zlib** streaming inflate/deflate, including how to inflate a stream whose
  decompressed size you don't know yet.
- **POSIX file I/O at the syscall level**: `open`/`read`/`write`/`close` with the
  error paths that matter — `EINTR`, short reads, short writes — plus
  `opendir`/`readdir` tree walking and `mkdir -p`.

## Build & run (Linux / WSL)

Needs a POSIX host and the **zlib** development headers:

```bash
sudo apt install zlib1g-dev      # Debian/Ubuntu; zlib is the only dependency
make                             # builds ./mygit  (cc -Wall -Wextra -O2 ... -lz)
make test                        # full init→add→commit→log→diff demo in a scratch dir
```

A hands-on session:

```bash
mkdir demo && cd demo
mygit init
printf 'hello\nworld\n' > a.txt
mygit add a.txt
mygit write-tree                       # -> a tree id
mygit commit -m "first commit"
mygit log
echo 'a third line' >> a.txt
mygit diff                             # working tree vs the staged blob
mygit status
```

**Prove it really is git.** The ids match, so the real tool interoperates:

```bash
mygit hash-object a.txt      # == `git hash-object a.txt`, exactly
git --git-dir=.mygit cat-file -p HEAD   # real git reads mygit's commit + tree + blob
git --git-dir=.mygit log --oneline      # ...and walks mygit's refs and history
```

`make asm` regenerates the teaching assembly (works on any host, including native
Windows, because clang cross-targets Linux).

## How it works

The store is built bottom-up; each file is one layer.

- **`sha1.c` / `sha1.h`** — SHA-1, self-contained (no libc, declares its own
  types). `sha1_block()` is the 80-round core; `init`/`update`/`final` handle
  streaming and the padding rule. This unit compiles to assembly with no headers
  present, which is why it is one of the asm deliverables.
- **`util.c`** — the foundation: `die()`/`xmalloc()`, hex encode/decode, `mkdir -p`,
  and the syscall-level `read_file`/`write_file` (this is where `EINTR` and
  short-read/short-write loops are commented in detail).
- **`object.c`** — the object store. `make_canonical()` builds
  `"<type> <size>\0<content>"`; `write_object()` hashes it, zlib-deflates it, and
  stores it under the two-char fan-out directory (skipping the write if the id
  already exists — identical content is already stored). `read_object()` inflates
  and splits the header back off. `hash-object` and `cat-file` live here.
- **`index.c`** — the staging area. A sorted **text** index (one
  `"<mode> <hexoid> <path>"` line per entry) — a deliberate simplification of
  git's binary index, so you can `cat .mygit/index` and read it. `add` hashes a
  file into a blob and upserts its entry.
- **`tree.c`** — the snapshot step. `write-tree` turns the flat index into
  **nested tree objects**, one per directory, using git's exact tree-entry sort
  (directories compared as if their name ended in `/`). This is where the Merkle
  structure is built, and why our tree ids match git's.
- **`commit.c`** — commit objects, the `HEAD`/branch refs, and `log`. `commit`
  composes `write-tree` + `commit-tree -p HEAD` + advancing the branch ref; `log`
  is a first-parent graph walk over immutable commits.
- **`diff.c`** — working tree vs index. Re-hashes each tracked file to detect
  changes, and prints a simple line diff (common prefix/suffix trim) for modified
  files. `status` reports modified / deleted / untracked.
- **`main.c`** — argument dispatch and `init`.

### Why commits are immutable snapshots

A tree's id is `SHA-1` over a body that literally contains the ids of its
children; a commit's id is `SHA-1` over text that contains its tree's id. So:

```
edit one byte of src/main.c
  → its blob id changes
    → the src/ tree's body (which lists that id) changes → src tree id changes
      → the root tree's body changes → root tree id changes
        → the commit's text changes → commit id changes
```

The commit id transitively fixes every byte of every file. You cannot rewrite
history without minting new ids — that is a Merkle tree, and it is git's entire
integrity story. (It is also why SHA-1's collision weakness matters; see below.)

## Assembly notes

`asm/demo.c` is a self-contained extraction of the two most instructive
pure-logic routines — the **SHA-1 compression round** and **hex encoding** — the
endpoints of git's identity pipeline (`bytes → 20 raw id bytes → 40 hex chars`).
`asm/demo.annotated.s` annotates the `-O1` output essentially instruction by
instruction. The highlights it calls out:

- The hand-written big-endian word load `(b0<<24)|(b1<<16)|(b2<<8)|b3` compiles to
  a single **`bswap`**.
- The rotate idiom `(x<<n)|(x>>(32-n))` compiles to a single **`rol`** (by 1, 5,
  and 30).
- The "choose" function `(b&c)|(~b&d)` is emitted as the cheaper equivalent
  `d ^ (b & (c^d))` — no `not` instruction.
- The 80-round variable rotation `e=d; d=c; c=ROTL(b,30); b=a; a=T` becomes a
  chain of `mov`s reusing five fixed registers — **zero memory traffic per round**.
- `sha1_block` is a **leaf** function, so it borrows part of the 128-byte **red
  zone** for the 320-byte `w[80]` schedule instead of reserving it all with `subq`.
- `demo_run` shows the optimizer's judgement: it constant-folds the block
  construction but still *calls* `sha1_block` (an 80-round loop-carried dependency
  is too much to evaluate at compile time).

The **real** `sha1.c` is also compiled to assembly (`asm/sha1.{O0,s,O2}.s`) since
it is dependency-free, so the committed asm reflects real project code, not only
the demo. Compare `demo.O0.s` (everything spilled, the rotate emitted as two
shifts + or) with `demo.O2.s` (loops partially unrolled) to watch `-O` levels
diverge. Regenerate everything with `make asm`.

## Scope & honesty

What this teaching core **does**: real content-addressed storage, SHA-1 ids that
match git, blobs/trees/commits, nested trees with correct git sort order, a
staging index, `commit`/`log`/`diff`/`status`, and byte-level git interoperability.

What it **does not** do (all noted at the relevant code):

- **Loose objects only — no pack files.** Every object is its own zlib file. Real
  git packs thousands of objects into a delta-compressed `.pack` + index; that is
  the "Going further" stretch below.
- **Text index, not git's binary index.** We drop the cached `stat(2)` data, so
  `status`/`diff` must re-hash files instead of trusting mtime/size.
- **SHA-1 only.** git also supports a SHA-256 object format (`--object-format`);
  see below. We note SHA-256 but implement SHA-1 (git's default).
- **A coarse diff** (common prefix/suffix), not git's minimal-edit Myers diff.
- **No branches/merge/checkout/remote/`.gitignore`**, and `add` does not recurse
  into directories (name files explicitly).

## Going further

- **Stretch — pack files.** Implement a `.pack` reader: parse the pack header and
  the per-object varint type+size, inflate each entry, and resolve `OBJ_REF_DELTA`
  / `OBJ_OFS_DELTA` against their base objects. This is how git actually stores
  history at scale and how `clone`/`fetch` move it over the wire; loose objects are
  just the write-ahead form that `git gc` later packs.
- **SHA-256.** git's newer object format hashes the identical canonical
  `"<type> <size>\0<content>"` with SHA-256 (64 hex chars). Because our hashing is
  isolated in `sha1.c` behind `hash_object()`, swapping the digest is a localized
  change — the store, trees, and commits are otherwise format-agnostic.
- **Read the real thing.** git's `object.c`, `tree-walk.c`, `sha1-file.c`
  (loose-object read/write), and `pack-objects.c` are the production versions of
  every file here. `base_name_compare()` in git's `read-cache.c` is the tree sort
  we replicate.

## References

- Pro Git, ch. 10 "Git Internals" — objects, refs, and the packfile format.
- RFC 3174 — US Secure Hash Algorithm 1 (SHA-1). The SHAttered attack (2017) and
  git's `sha1dc` hardened SHA-1 explain why plain SHA-1 is for learning only.
- `man 2 read`, `man 2 write` (short reads/writes, `EINTR`); zlib manual
  (`compress2`, streaming `inflate`).
- git source: `sha1-file.c`, `object.c`, `tree-walk.c`, `read-cache.c`.
