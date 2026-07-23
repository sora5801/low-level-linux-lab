# Type-2 hypervisor over /dev/kvm 🟥

**What it is.** A minimal but genuinely working **Type-2 hypervisor** (a.k.a. a
VMM — virtual machine monitor) written in userspace C. It opens `/dev/kvm`,
creates a VM and a virtual CPU, hands the guest 2 MiB of RAM, and runs two tiny
guests through the same `KVM_RUN` loop: first a **16-bit real-mode** program that
computes a digit from host-seeded registers and prints it via an `OUT`
instruction, then a **64-bit long-mode** program (with page tables and the full
control-register ritual) that prints a string the same way. Each `OUT` becomes a
`KVM_EXIT_IO` that our loop services — the smallest possible paravirtual console.
This is the same architecture QEMU/KVM, Firecracker, and kvmtool use; they just
add devices, a BIOS, and an interrupt controller on top of exactly this loop.

This is a **teaching-core**, and honestly scoped: it runs *our* hand-assembled
guests, not a real OS. It implements the `KVM_EXIT_IO` and `KVM_EXIT_MMIO` paths,
the memory-region and vCPU setup, and both CPU-mode bring-ups. It does **not**
boot a `bzImage`, provide virtio devices, an IOAPIC/LAPIC, SMP, or dirty-page
tracking — see [Going further](#going-further-the-stretch-from-the-list) for
where each of those would slot in.

## What you'll learn

- **The KVM ioctl API and its three-level fd tree**: `/dev/kvm` (system) →
  `KVM_CREATE_VM` → `vmfd` → `KVM_CREATE_VCPU` → `vcpufd`. Every KVM operation is
  an `ioctl(2)`; there is no library, the ioctl numbers *are* the ABI.
- **Guest physical memory**: how `KVM_SET_USER_MEMORY_REGION` makes a plain host
  `mmap` region *be* the guest's RAM, and the invariant that the host mapping
  must outlive the registration.
- **The `kvm_run` shared page**: why its size comes from
  `KVM_GET_VCPU_MMAP_SIZE`, why it is `MAP_SHARED`, and how it is the two-way
  mailbox between kernel and VMM on every exit.
- **The `KVM_RUN` loop and VM exits**: `KVM_EXIT_IO` (port in/out, including the
  self-relative `data_offset` pointer reconstruction), `KVM_EXIT_MMIO`,
  `KVM_EXIT_HLT`, `KVM_EXIT_FAIL_ENTRY`, `KVM_EXIT_SHUTDOWN`, and `EINTR`.
- **Setting architectural CPU state directly** with `KVM_SET_REGS`/`KVM_SET_SREGS`:
  segment descriptors (base/limit/`L`/`db`), the reserved `RFLAGS` bit-1, and the
  reset-state fixups real mode needs.
- **The long-mode bring-up checklist**: 4-level page tables (PML4→PDPT→PD, with a
  2 MiB `PS` leaf), `CR4.PAE`, `EFER.LME|LMA`, `CR0.PG|PE`, and a code segment
  with the `L` bit — all the things the CPU demands before it will run 64-bit code.
- **Reading the optimizer**: the exit-reason dispatch, extracted to `asm/demo.c`,
  shows clang turning grouped `switch` cases into a **bitmask + `bt`** test and
  returning a two-field struct **packed into `%rax`**.

## Build & run

> **Build & run: Linux only.** `vmm.c`/`guest.c` include `<linux/kvm.h>` and open
> `/dev/kvm`. You need a Linux machine (bare metal, or a VM with **nested virt**
> enabled) where `/dev/kvm` exists and you are in the `kvm` group. This will not
> build on Windows; do it in a Linux/WSL2 box or a QEMU VM. The **assembly**
> target below runs anywhere.

```bash
# 1. Confirm KVM is available and you can use it:
ls -l /dev/kvm                 # crw-rw----+ root kvm ...
groups | grep -q kvm && echo "in kvm group" || echo "add yourself: sudo usermod -aG kvm $USER; re-login"

# 2. Build the hypervisor (needs kernel UAPI headers: they ship with any distro).
make                           # -> ./vmm

# 3. Run both examples (no sudo needed if you are in the kvm group):
make run                       # == ./vmm both
```

Expected output:

```
== real mode: compute a digit, OUT to serial ==
4

-- guest exited (clean hlt) --

== long mode: print a string, OUT to serial ==
Hello from 64-bit long mode via OUT 0x3f8!

-- guest exited (clean hlt) --
```

Run one example at a time with `./vmm real` or `./vmm long` (or `make run-real` /
`make run-long`). Try editing `regs.rax`/`regs.rbx` in `guest_setup_real_mode`
and rebuilding — the printed digit changes, proving your `KVM_SET_REGS` reaches
the guest CPU.

Regenerate the teaching assembly (works on **any** host, no Linux/VM needed):

```bash
make asm                       # writes asm/demo.{s,O0.s,O2.s} via clang cross-target
```

## How it works

| File | Role |
|------|------|
| `vmm.h` | Shared types: `struct vm` (the three fds + guest RAM + the `kvm_run` page) and the prototypes wiring `vmm.c` to `guest.c`. Opens with the "what is a Type-2 hypervisor" primer. |
| `vmm.c` | The **engine**: `vm_create` (the six-ioctl bring-up), `vm_run_loop` (the `KVM_RUN` dispatch), the `KVM_EXIT_IO`/`MMIO` handlers, `vm_destroy`, and `main`. Knows *how* to run a guest, nothing about *what* the guest is. |
| `guest.c` | The **two guests**: the real-mode and long-mode machine-code payloads (raw bytes, each annotated with its mnemonic) plus the register/sreg/page-table setup that boots each mode. |
| `Makefile` | Plain userspace build (no Kbuild — this is a process, not a module) with `run*` and `asm` targets. |
| `asm/demo.c` | The VM-exit dispatch logic, lifted out standalone for the assembly study (see below). |

**The bring-up (`vm_create`).** Six steps, each one ioctl: open `/dev/kvm`;
check `KVM_GET_API_VERSION == 12`; `KVM_CREATE_VM`; `mmap` guest RAM and register
it with `KVM_SET_USER_MEMORY_REGION`; `KVM_CREATE_VCPU`; and `mmap` the `kvm_run`
page whose size comes from `KVM_GET_VCPU_MMAP_SIZE`. Every return value is checked
and routed to a loud `die()` — the error paths are half the lesson.

**The run loop (`vm_run_loop`).** `ioctl(KVM_RUN)` enters guest mode and *blocks*
until the guest exits; a VM exit is a synchronous call from the guest into us.
On return we `switch (run->exit_reason)`. For `KVM_EXIT_IO` we reconstruct the
data pointer from the self-relative `io.data_offset` (`(uint8_t *)run +
data_offset` — the single most bug-prone line in any VMM) and, for an `OUT` to
port `0x3f8`, print the bytes; an `IN` gets `0xff` floated back. `KVM_EXIT_HLT`
ends the loop cleanly; `FAIL_ENTRY`/`SHUTDOWN`/`INTERNAL_ERROR` report and stop;
`EINTR`/`KVM_EXIT_INTR` just re-enter (the mechanism a real VMM uses to break out
and inject an interrupt).

**Real mode (`guest_setup_real_mode`).** A fresh vCPU resets like a real CPU —
`CS:IP = F000:FFF0`, wanting a BIOS we do not have. We read `sregs`, zero
`CS.base`/`CS.selector` so a physical address is just the offset, load the code at
`0x1000`, and set `rip=0x1000`, `rax=2`, `rbx=2`, `rflags=0x2`. The guest does
`add %bl,%al` → `add $'0'` → `out`, printing the digit `4` computed from the
registers *we* chose.

**Long mode (`guest_setup_long_mode`).** The interesting one. We build a 4-level
identity map — `PML4[0]→PDPT[0]→PD[0]`, where the PD entry has the `PS` bit so it
maps a single **2 MiB page** directly (which is exactly why guest RAM is 2 MiB:
one entry covers it all, three 8-byte writes). Then we set `CR3` to the PML4,
`CR4.PAE`, `CR0.PG|PE|…`, `EFER.LME|LMA`, and a `CS` descriptor with `L=1`. KVM
lets us set this final architectural state directly, so we skip the real-mode →
protected → long far-jump dance a real boot performs. The 64-bit guest then walks
a string at `0x1000` and `OUT`s each byte.

## Assembly notes

Kernel-facing C is **not standalone-compilable on this host**: `vmm.c`/`guest.c`
pull in `<linux/kvm.h>`, `<sys/ioctl.h>`, `<sys/mman.h>`, which do not exist off
Linux, so clang cannot emit their assembly here. Per the repo convention, the
project's most instructive **pure-logic** helper — the **VM-exit dispatch**, the
routine that runs on *every* exit and decides what the loop does — is extracted
into a self-contained [`asm/demo.c`](asm/demo.c) that includes nothing and
declares its own types. Its `kvm_exit_action`, `io_batch_bytes`,
`serial_is_console_write`, and `decode_exit` are the exact classifications the run
loop performs, minus the ioctl plumbing.

[`asm/demo.annotated.s`](asm/demo.annotated.s) is the `-O1` output with a comment
on essentially every instruction. The three surprises it highlights:

- **A `switch` with grouped cases becomes a bitmask + `bt`.** We built with
  `-fno-jump-tables` expecting a compare ladder; the singleton cases are one, but
  the case *groups* `{8,9,17}→STOP_ERR` and `{7,10}→REENTER` were folded into the
  constants `0x20300` and `0x480` — literally the sets, one set bit per case —
  and membership is tested with a single `btl %edi, %eax`. Once you see it,
  `btl` reads as "is `reason` in this set?".
- **An 8-byte struct returns packed in one register.** `decode_exit` puts
  `action` in the low 32 bits of `%rax` and `is_error` in the high 32 (via
  `movabsq $1<<32` — no shift instruction) and `or`s them together. No hidden
  return pointer, no memory round-trip.
- **Booleans go branchless and constants erase code.**
  `serial_is_console_write` becomes `xor`/`or`/`sete`; and `demo_selftest`, a
  dozen assertions over constant inputs, collapses to `xor %eax,%eax` — the whole
  function evaporates once the optimizer proves every check passes.

Compare the three levels: [`asm/demo.O0.s`](asm/demo.O0.s) (naive, every value
spilled to the stack — easiest to trace), [`asm/demo.s`](asm/demo.s) (`-O1`, the
annotated baseline), [`asm/demo.O2.s`](asm/demo.O2.s) (`-O2` — same logic with the
frame-pointer prologue/epilogue gone). The `.s` files are genuine clang output;
regenerate them with `make asm`.

> **On the guest payloads:** the *other* hot bytes in this project are the guest
> machine-code blobs in `guest.c`. They are given as raw bytes with the mnemonic
> beside each, because that IS the point — the guest CPU executes exactly those
> bytes, with no assembler in the loop. Both were verified byte-for-byte with a
> disassembler; e.g. the long-mode loop's `je +6` and `jmp -12` displacements are
> hand-computed from the instruction offsets shown in the comments.

## Going further (the `Stretch:` from the list)

The stretch goal is **firecracker/kvmtool territory: boot a real `bzImage` +
initramfs and add virtio-blk/net.** Concretely, from this core you would add:

- **A boot protocol.** Load a Linux `bzImage` per `Documentation/x86/boot.rst`:
  fill `struct boot_params` (the "zero page"), set up an E820 memory map, put the
  command line and initramfs in guest RAM, and jump to the 64-bit entry — instead
  of our hand-written payload.
- **An interrupt controller and timer.** `KVM_CREATE_IRQCHIP` (in-kernel LAPIC/
  IOAPIC/PIC) and `KVM_CREATE_PIT2`, plus `KVM_IRQFD`/`KVM_IOEVENTFD` so devices
  can raise IRQs and be poked without a full userspace exit.
- **virtio devices.** A `virtio-mmio` (or PCI) transport backed by our
  `KVM_EXIT_MMIO` path: a virtqueue in guest RAM, a `virtio-blk` serving a disk
  image and a `virtio-net` bridged to a tap device. This is where MMIO stops being
  a stub and becomes the device bus.
- **A real console.** Emulate the 16550 UART properly (or use `virtio-console`)
  instead of "any `OUT 0x3f8` prints one byte."
- **SMP and live state.** Multiple vCPUs, each `KVM_RUN` on its own host thread;
  dirty-page tracking (`KVM_GET_DIRTY_LOG`) for migration; `KVM_SET_CPUID2` to
  present a sane CPU model to the guest.

Firecracker is the canonical "how small can a production VMM be" answer; kvmtool
is the most readable full example.

## References

- **LWN, "Using the KVM API"** (Josh Triplett) — the article this core is built
  from; its real-mode and long-mode examples are the ancestors of the two guests
  here.
- Kernel `Documentation/virt/kvm/api.rst` — the authoritative ioctl reference
  (`KVM_RUN`, `KVM_SET_USER_MEMORY_REGION`, every `KVM_EXIT_*`).
- `include/uapi/linux/kvm.h` — the structs and ioctl numbers that *are* the ABI.
- `Documentation/x86/boot.rst` — the `bzImage`/`boot_params` protocol for the
  stretch goal.
- **Firecracker** (`src/vmm`) and **kvmtool** (`tools/kvm` / `kvmtool`) — small,
  readable production VMMs. QEMU's `accel/kvm/kvm-all.c` is the big one.
- Intel SDM Vol. 3, ch. 3–4 (segmentation, paging) and ch. 9 (mode switching) —
  ground truth for the long-mode bring-up checklist.
