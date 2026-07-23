# Character device driver with ioctl 🟧

**What it is.** A loadable kernel module that registers `/dev/foo`, a blocking
byte **FIFO** (a ring buffer, like a named pipe) built on the `misc` device
framework. It implements the full `file_operations` surface — `open`, `read`,
`write`, `llseek`, `release` — plus a typed **ioctl** command set shared with
userspace via a common header, **blocking reads** that sleep on a wait queue
until data arrives, `poll()`/`select()`/`epoll` support, and (the stretch)
`mmap()` so userspace gets a zero-copy window onto the kernel's backing pages.
It is a genuinely working teaching-core: everything described here compiles
against real kernel headers and runs — you just need a Linux/QEMU kernel to load
it into (a kernel module cannot build or run on Windows).

## What you'll learn

- **`misc_register`** — the low-ceremony path to a real `/dev` node (borrows
  major 10, needs only a name + `file_operations`; udev creates the node).
- **`file_operations`** — the VFS vtable: how `open/read/write/llseek/release/
  unlocked_ioctl/poll/mmap` map one-to-one onto syscalls on your fd.
- **`copy_to_user` / `copy_from_user` / `get_user` / `put_user`** — the only
  sanctioned user↔kernel byte movers, and *why* a raw `memcpy` to a user pointer
  is a security bug.
- **Wait queues** — `wait_event_interruptible` to sleep a reader until a
  condition holds, `wake_up_interruptible` to re-run it; the drop-lock-then-wait
  dance that avoids the lost-wakeup race and the sleep-holding-a-lock deadlock.
- **`poll`** — `poll_wait` + a readiness mask, the machinery under select/epoll,
  including a `SET_LOWAT` ioctl that gates read-readiness on a low-water mark.
- **ioctl number encoding** — how `_IO/_IOR/_IOW` pack direction + size + magic +
  ordinal into one 32-bit word so kernel and userspace can't silently disagree.
- **Ring buffer index math** — power-of-two masking and free-running counters
  (the kfifo trick), extracted verbatim into `asm/demo.c` for the assembly study.
- **`mmap`** — sharing a `vmalloc_user` buffer into userspace with
  `remap_vmalloc_range` for true zero-copy reads.
- **Concurrency** — why this driver uses a **mutex, not a spinlock** (because
  `copy_*_user` can page-fault and sleep).

## Build & run

> **Linux only.** A kernel module builds against a configured kernel tree, which
> exists on Linux / WSL2 / a QEMU VM — never on the Windows host this repo may
> live on. Do the kernel steps in a VM. The **assembly** target below is the one
> thing that runs anywhere (clang cross-compiles).

```bash
# 1. Build the module + the userspace tester (needs kernel headers installed:
#    e.g. `apt install linux-headers-$(uname -r)` or build inside a kernel tree).
make                      # -> foo.ko  and  ./test_foo

# 2. Load it. dmesg shows the registration line and the assigned minor.
sudo insmod foo.ko
dmesg | tail -n 2         # "foo: registered /dev/foo (minor N), FIFO capacity 4096 bytes"
ls -l /dev/foo            # crw-rw-rw-  ... created by udev

# 3. Drive every code path (open dmesg -w in another terminal to watch).
./test_foo

# 4. Poke it by hand, too:
echo -n "hi" > /dev/foo   # write()
cat /dev/foo              # blocking read(): will HANG until you write more —
                          #   Ctrl-C to stop. That hang IS wait_event working.

# 5. Unload.
sudo rmmod foo
```

Regenerate the teaching assembly (works on **any** host, no VM needed):

```bash
make asm                  # writes asm/demo.{s,O0.s,O2.s} via clang cross-target
```

## How it works

| File | Role |
|------|------|
| `foo.h` | The **shared ioctl contract**. Both the module and `test_foo.c` include it, so the `_IO/_IOR/_IOW` command numbers and `struct foo_info` layout are computed once and are provably identical on both sides. |
| `foo.c` | The **module**. Registration, the `file_operations`, the ring buffer, wait queues, poll, ioctl dispatch, and mmap. |
| `test_foo.c` | The **userspace exerciser**. Walks every feature (write/inspect, FIFO read, poll gating, a blocking read across `fork`, `lseek` skip, and an mmap read) and checks every return value. |
| `Makefile` | Kbuild two-branch Makefile (`obj-m := foo.o` + kernel-tree recursion) that also builds the tester and the asm. |
| `asm/demo.c` | The ring buffer index math, lifted out standalone (see below). |

**The FIFO core.** `head` (producer) and `tail` (consumer) are *free-running*
`unsigned int` counters that are never wrapped to the buffer size. Two identities
make the whole driver branch-light:

- `count = head - tail` — unsigned subtraction, correct across the 2³² wrap as
  long as `count ≤ capacity` (which the write path enforces).
- `offset = index & (capacity-1)` — a mask, valid because `capacity == PAGE_SIZE`
  is a power of two. That also makes the buffer a whole page, so it is `mmap`-able.

The classic "is `head==tail` empty or full?" ambiguity cannot occur: empty is
`count==0`, full is `count==capacity` — two distinct values. This is exactly how
the kernel's own `kfifo` works.

**Blocking read (the heart).** `foo_read` takes the mutex, then loops:
`while (empty) { unlock; wait_event_interruptible(readq, !empty); relock; }`.
Dropping the lock before sleeping is mandatory — otherwise no writer could ever
run to make the buffer non-empty. `wait_event_interruptible` atomically re-checks
the condition against `wake_up`, closing the lost-wakeup race; a signal makes it
return `-ERESTARTSYS`. `foo_write` is the mirror image on `writeq`. Reads/writes
split their copy at the wrap boundary into a contiguous chunk plus a wrapped
remainder — the arithmetic isolated in `asm/demo.c`.

**ioctl set** (`foo.h`): `FOO_IOC_RESET` (`_IO`, empty the FIFO),
`FOO_IOC_GET_LEVEL` (`_IOR int`, bytes readable now), `FOO_IOC_SET_LOWAT`
(`_IOW int`, poll read-readiness threshold), `FOO_IOC_GET_INFO`
(`_IOR struct foo_info`, a consistent capacity/count/head/tail snapshot taken
under the lock). The handler rejects foreign magic and out-of-range ordinals with
`-ENOTTY` before dispatching.

**llseek**, honestly scoped: a FIFO is not randomly seekable, so `foo_llseek`
implements only what *is* meaningful — a **forward skip** that discards buffered
bytes (fast-forwarding the stream) and wakes writers. Backward seeks return
`-EINVAL` and `SEEK_END` returns `-ESPIPE`, the same errno a real pipe gives.

## Assembly notes

Kernel C isn't standalone-compilable on this host (or any host without a kernel
build tree): `foo.c` pulls in `<linux/*.h>`. So, per the repo convention, the
project's most instructive **pure-logic** helper — the ring buffer read/write
index arithmetic — is extracted into a self-contained `asm/demo.c` that declares
its own types and includes nothing. The functions there (`rb_count`, `rb_space`,
`rb_first_chunk`, `rb_plan_xfer`) are byte-for-byte the computations `foo_read`/
`foo_write` perform; only the `copy_*_user` calls around them are removed.

[`asm/demo.annotated.s`](asm/demo.annotated.s) is the `-O1` output with a comment
on essentially every instruction. What it highlights:

- **Power-of-two modulo becomes one `and`.** `index & (cap-1)` — no division.
- **The live byte count is one `sub`.** `head - tail` on free-running counters,
  wraparound and all, needs no separate full/empty flag.
- **`min()` is branchless** — a `cmp` + `cmovb`, no jump to mispredict.
- **`lea` used as arithmetic** — the compiler folds `cap + tail - head` into a
  single `leal (%rsi,%rdx),%eax`.
- **A ≤16-byte struct returns packed in `%rax`** (SysV ABI): `rb_plan_xfer`
  assembles `{first, second}` with a `shl $32` + `lea`, no hidden pointer, no
  memory round-trip.
- **Constant folding erases code**: `rb_selftest` runs a dozen assertions over
  constant inputs and the optimizer deletes the entire body down to
  `xor %eax,%eax` ("return 0").

Compare the three levels: [`asm/demo.O0.s`](asm/demo.O0.s) (naive, every value
spilled to the stack — easiest to trace), [`asm/demo.s`](asm/demo.s) (`-O1`, the
annotated baseline), [`asm/demo.O2.s`](asm/demo.O2.s) (`-O2` — the frame-pointer
prologue/epilogue vanishes entirely, leaving pure arithmetic).

## Going further (the `Stretch:` from the list)

The `mmap` handler is implemented: it maps the `vmalloc_user` backing store into
userspace with `remap_vmalloc_range`, so a reader can pull bytes straight out of
the shared page with **no `copy_to_user`**. This teaching-core shares the
*storage*; it does not yet share the *protocol*. In a production zero-copy ring:

- Put `head`/`tail` in the shared page too (like io_uring's SQ/CQ rings) so
  userspace advances them directly, with **acquire/release** ordering and memory
  barriers instead of the mutex — the producer publishes `head` with a release
  store, the consumer reads it with an acquire load, and vice versa.
- Use a second mapping or `SetPageDirty`/cache-coherency care if writers also
  mmap. On x86-64 the caches are coherent, but portable code needs `smp_wmb()`/
  `smp_rmb()` at the index updates.
- Support partial mappings and a `vm_operations_struct` with `.fault` for
  demand-paged buffers larger than one page.

Other natural extensions: a per-open (per-minor) device instead of one global
FIFO; `O_NONBLOCK` already works; add `FASYNC`/`SIGIO` support; enforce
`capable(CAP_SYS_ADMIN)` on the mutating ioctls.

## References

- **LDD3**, *Linux Device Drivers* 3rd ed., ch. 3 (char drivers), ch. 6 (advanced
  char: `ioctl`, blocking I/O, `poll`), ch. 15 (`mmap`). The canonical `scull`
  and `scullpipe` drivers are this project's ancestors.
- Kernel source: `drivers/char/misc.c` (`misc_register`), `include/linux/fs.h`
  (`file_operations`), `include/linux/wait.h` (`wait_event_interruptible`),
  `lib/kfifo.c` (the production version of the index math here),
  `mm/vmalloc.c` (`remap_vmalloc_range`).
- `Documentation/userspace-api/ioctl/ioctl-number.rst` — how magic bytes are
  registered so drivers don't collide.
- `man 2 ioctl`, `man 2 poll`, `man 2 mmap`, `man 9 copy_to_user`.
