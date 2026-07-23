# Add a real system call 🟥

**What it is.** A complete, correct patch set that adds a brand-new system call —
`hello(char __user *buf, size_t len)` — to the Linux kernel, plus a fully-working
userspace program that invokes it by number. The syscall copies a greeting into
a user-supplied buffer with `copy_to_user()` and returns the byte count, the
same shape as `read(2)`. Because a syscall is compiled *into* the kernel image
(not loaded as a module), this ships as heavily-commented source + patches you
apply to a real kernel tree and boot in QEMU. This is a 🟥 project: the kernel
half must be built on Linux inside a VM. The userspace caller and the annotated
assembly are the parts you can build and read anywhere.

> **Teaching-core honesty.** Everything here is production-shaped and correct —
> the `SYSCALL_DEFINE2` body, the `copy_to_user` error handling, the table entry,
> the prototype, the Makefile wiring, and the userspace `syscall()` call are
> exactly what a real patch contains. The *scope* limit is only this: we do not
> rebuild the kernel on this Windows host (we can't), so the build-and-boot steps
> are given as an exact QEMU recipe rather than executed here. The syscall number
> `463` is the next free slot as of Linux 6.10; on another kernel you pick the
> next free number (details below).

## What you'll learn

- **What a syscall actually is**: the `syscall` instruction, `%rax` = number,
  args in `rdi, rsi, rdx, r10, r8, r9`, dispatch through `sys_call_table[]` in
  `entry_SYSCALL_64`.
- **`SYSCALL_DEFINE2`** and the post-4.17 `pt_regs` wrapper (`__x64_sys_hello` /
  `__se_sys_hello` / `__do_sys_hello`) — and why that indirection exists
  (Spectre-v1 hardening at the syscall boundary).
- **The user/kernel trust boundary**: why you must never dereference a user
  pointer directly, and how `copy_to_user()` makes a bad address fail safely.
  Its return-value convention (bytes *not* copied) and the `-EFAULT` / `-EINVAL`
  error idiom.
- **The four files a syscall touches**: `arch/x86/entry/syscalls/syscall_64.tbl`
  (the number), `include/linux/syscalls.h` (the prototype), `kernel/Makefile`
  (the build), and the implementation file itself.
- **Calling a syscall glibc doesn't know about** with `syscall(__NR_..., ...)`,
  and the kernel's `[-4095, -1]` "negative return means `-errno`" convention.

## Build & run

### Userspace caller (any Linux/WSL host)

```bash
make            # builds user/hello_user with clang
make run        # runs it
```

On a **stock (unpatched) kernel** this prints `Function not implemented`
(`ENOSYS`) — correct and expected: number 463 isn't wired up there. On a kernel
built with the patch it prints:

```
sys_hello returned 34 bytes:
Hello from a real Linux syscall!
```

### Kernel half (Linux, inside a QEMU VM — never on this host)

```bash
# 0. Get a kernel source tree (matching or close to 6.10 for the patch context).
cd linux/

# 1. Drop in the implementation and apply the three wiring patches.
cp /path/to/11-add-syscall/kernel/sys_hello.c kernel/sys_hello.c
git apply /path/to/11-add-syscall/patches/0001-syscall_64_tbl.patch
git apply /path/to/11-add-syscall/patches/0002-syscalls_h.patch
git apply /path/to/11-add-syscall/patches/0003-kernel_makefile.patch
#   (Each patch header explains a by-hand fallback if context has drifted.)

# 2. Configure and build.
make defconfig                 # or use your distro config
make -j"$(nproc)" bzImage

# 3. Boot it in QEMU with a minimal initramfs/rootfs that contains hello_user.
qemu-system-x86_64 -kernel arch/x86/boot/bzImage \
    -initrd rootfs.cpio.gz -nographic -append "console=ttyS0"

# 4. Inside the VM:
./hello_user            # -> prints the greeting
dmesg | tail            # -> "sys_hello: called with buf=... len=256"
```

If you set `__NR_hello` to a different number in your patch, set the same number
in `user/hello_user.c` (or better: rebuild glibc/uapi headers so `__NR_hello`
resolves automatically).

## How it works (file by file)

- **`kernel/sys_hello.c`** — the syscall body. `SYSCALL_DEFINE2(hello, char
  __user *, buf, size_t, len)` expands into the `pt_regs` entry wrapper plus the
  function you actually write. It validates `len`, **clamps the copy to the
  caller-provided size** (the one invariant that keeps the kernel from writing
  past the user's buffer), calls `copy_to_user()`, and maps failures to
  `-EFAULT` / `-EINVAL`. Every kernel API touched is commented with *what
  subsystem it is and what breaks if you get it wrong*.

- **`patches/0001-syscall_64_tbl.patch`** — adds `463 common hello sys_hello` to
  `arch/x86/entry/syscalls/syscall_64.tbl`. This allocates the number and, via
  the build-time `syscalltbl.sh`, generates the `sys_call_table[]` slot and the
  `__NR_hello` define. Note: numbers ≥ 512 are reserved for the x32 ABI, so a
  `common`/`64` syscall must stay below 512.

- **`patches/0002-syscalls_h.patch`** — adds the `asmlinkage long
  sys_hello(char __user *buf, size_t len);` prototype to
  `include/linux/syscalls.h`. The `__user` annotation lets `sparse` (`make C=1`)
  statically catch any raw dereference of the user pointer.

- **`patches/0003-kernel_makefile.patch`** — adds `obj-y += sys_hello.o` to
  `kernel/Makefile`. It's `obj-y` (built-in), **not** `obj-m`: a syscall can't be
  a module because `sys_call_table[]` is fixed at link time. This is why the
  whole project is a kernel patch rather than a `.ko`.

- **`user/hello_user.c`** — ordinary userspace. Uses `syscall(__NR_hello, buf,
  sizeof(buf))` because glibc has no wrapper for a syscall it's never heard of.
  Handles the error path (`ENOSYS` on an unpatched kernel, `EFAULT`, `EINVAL`).

- **`Makefile`** — builds the userspace caller and regenerates the teaching
  assembly. It intentionally has no kernel-module target; comments explain why.

## Assembly notes

Kernel C can't be compiled to standalone assembly on this host (it needs the
in-tree Linux headers and the `SYSCALL_DEFINE` machinery), so — per the lab
convention — the annotated assembly is generated from **`asm/demo.c`**, a
self-contained reimplementation of the project's instructive core: the userspace
side that sets up the raw `syscall` instruction. It has no `#include` and
declares its own types, so it compiles anywhere.

Generated with the exact commands (also `make asm`):

```bash
clang --target=x86_64-pc-linux-gnu -S -O0 -fno-asynchronous-unwind-tables -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
clang --target=x86_64-pc-linux-gnu -S -O1 -fno-asynchronous-unwind-tables -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
clang --target=x86_64-pc-linux-gnu -S -O2 -fno-asynchronous-unwind-tables -fno-jump-tables asm/demo.c -o asm/demo.O2.s
```

What [`asm/demo.annotated.s`](asm/demo.annotated.s) highlights:

- **`invoke_hello`** shows the number **`463` (`__NR_hello`) landing in `%rax`**
  literally, followed by the bare `syscall`. Because `hello`'s arguments
  (`buf, len`) already match the syscall arg registers (`rdi, rsi`), the wrapper
  is almost nothing — the raw instruction *is* the whole story.
- **The two ABIs disagree on arg 4**: C `call` uses `rcx`, but `syscall`
  destroys `rcx` (return RIP) and `r11` (RFLAGS), so the kernel entry ABI uses
  `r10`. The header block spells this out.
- **`hello_or_errno`** shows real optimizer work: (1) the `err_out` pointer must
  be moved *out* of `rdx` before `rdx` becomes syscall arg3 (register pressure at
  the ABI seam), and (2) clang folded the C test `r < 0 && r >= -4095` into a
  single **unsigned `cmpq $-4095, %rax` plus two `cmov`s** — the exact branchless
  `-errno` idiom glibc's own `syscall()` wrapper uses.
- Compare [`asm/demo.O0.s`](asm/demo.O0.s) (`raw_syscall3` is a real `call`,
  everything spilled to the stack) with [`asm/demo.O2.s`](asm/demo.O2.s) (frame
  pointer dropped: `invoke_hello` becomes just `mov $463,%eax; xor %edx,%edx;
  syscall; ret`).

## Going further (the `Stretch:` from the list)

- **Make it a proper generic call.** Add the prototype to the UAPI headers and
  regenerate them so `__NR_hello` appears without the `#ifndef` fallback; add an
  entry to `arch/arm64/.../syscall.tbl` too and make the implementation
  arch-neutral so it works on more than x86-64.
- **Do something real and safe.** Replace the static greeting with a call that
  returns per-task data (e.g. copy the caller's `current->comm`), which forces
  you to think about locking and about *what* is safe to expose to userspace.
- **Add a selftest.** Wire a test into `tools/testing/selftests/` so the syscall
  is exercised by the kernel's own CI harness — that's how new syscalls actually
  land upstream.
- **What production does.** Real syscalls go through an API review (flags for
  future extension, `size` arguments for struct-versioning, `copy_struct_from_user`
  for forward/backward-compatible structs), get a `man` page, and are added to
  every arch table plus `strace`/glibc. See how a recent one (e.g. `mseal`,
  `cachestat`) was merged.

## References

- `man 2 syscall`, `man 2 syscalls`, `man 2 intro` (the error convention).
- Kernel source: `arch/x86/entry/entry_64.S` (`entry_SYSCALL_64`),
  `arch/x86/entry/syscalls/syscall_64.tbl`, `include/linux/syscalls.h`,
  `include/linux/uaccess.h` (`copy_to_user`), `kernel/sys.c` (example bodies).
- The `SYSCALL_DEFINEn` macro: `include/linux/syscalls.h` and
  `arch/x86/include/asm/syscall_wrapper.h` (the `__x64_sys_*` wrappers).
- Documentation/process for adding a syscall:
  `Documentation/process/adding-syscalls.rst` in the kernel tree.
