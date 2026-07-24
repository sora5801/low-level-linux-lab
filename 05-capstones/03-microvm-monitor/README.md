# microVM monitor — a KVM VMM with virtio-mmio devices 🟥 🧩

**What it is.** A **microVM monitor** in the shape of Firecracker/kvmtool: a
userspace process that drives `/dev/kvm` to create a VM, a vCPU, and guest RAM,
enters a guest directly in **64-bit long mode**, and gives it devices over a
**virtio-mmio** bus. Its centerpiece is a genuine **split virtqueue** — the
descriptor table + available ring + used ring in shared guest RAM — driven end to
end: the guest publishes a buffer and "kicks" the device with one MMIO write; the
monitor walks the queue and prints the buffer (a real **virtio-console**), then
publishes a used-ring completion the guest polls for. `virtio-blk` and
`virtio-net` ride the same bus as honest **stubs** (they answer discovery and
feature negotiation but move no real data).

This is a **teaching-core (🧩) of a 🟥 giant**, and scoped honestly:

- It runs **our own hand-written 64-bit payload**, not a real Linux `bzImage`. It
  *does* set up the exact **64-bit-entry contract** a `bzImage` expects — long
  mode on, a GDT and 4-level page tables in guest RAM, and `%rsi` pointing at a
  filled `boot_params` "zero page" with an E820 map (see `boot.h`). Parsing a real
  `bzImage` setup header and copying the kernel to `0x100000` is documented below
  as the remaining gap, not implemented.
- The **device side** of virtio — the virtqueue walk, the MMIO register machine,
  the ring index/wrap math — is **fully modeled**. The **driver-side** queue
  *setup* (programming the queue base addresses + `QUEUE_READY`) is done by the
  host loader for legibility; a real guest driver would do it from inside the VM.
- It **polls** the used ring instead of injecting an interrupt, so it needs no
  in-kernel IRQCHIP. Polling is a valid virtio mode; IRQ injection is the noted
  production path.

The layer *below* this — the raw KVM ioctls, the `KVM_RUN` loop, real vs. long
mode bring-up — is the sibling project
[`../../01-kernel/15-kvm-hypervisor`](../../01-kernel/15-kvm-hypervisor). This
capstone assumes that and builds the **device model** on top.

---

## Architecture

```
                         microVM monitor process  (this capstone)
   +----------------------------------------------------------------------------+
   |  main(): build virtio-mmio bus  ->  vm_create()  ->  guest_setup()          |
   |                                                                             |
   |   vm_run_loop():   ioctl(KVM_RUN) --------------------------.                |
   |        ^                                                    v                |
   |        |                              +---------------------------------+    |
   |        |   KVM_EXIT_HLT   <-----------|  switch (run->exit_reason)       |    |
   |        |   KVM_EXIT_MMIO  ----------->|  handle_mmio -> virtio bus       |    |
   |        |   KVM_EXIT_IO    ----------->|  handle_io   -> legacy 0x3f8     |    |
   |        `----------------------- (re-enter KVM_RUN) ----------------------'    |
   |                                        |                                     |
   |          virtio-mmio bus (virtio.c):   v                                     |
   |          +----------------+  +----------------+  +----------------+          |
   |          | virtio-console |  | virtio-blk STUB|  | virtio-net STUB|          |
   |          | (real: prints) |  | (probe only)   |  | (probe only)   |          |
   |          +-------+--------+  +----------------+  +----------------+          |
   +------------------|---------------------------------------------------------+
                      |  ioctl()  — the ONLY syscall the KVM API rides on
   ===================|===============  /dev/kvm  ================================
                      |  kernel: VT-x / AMD-V, EPT/NPT translate guest memory
   +------------------v---------------------------------------------------------+
   |  guest (2 MiB RAM @ GPA 0, entered in 64-bit long mode)                     |
   |                                                                            |
   |   code@0   page-tables   GDT      [ split virtqueue in guest RAM ]   boot_  |
   |                                     desc[] | avail ring | used ring  params |
   |                                                                            |
   |   driver payload:  read MagicValue ....... (MMIO read)                      |
   |                    Status = DRIVER_OK .... (MMIO write)                     |
   |                    avail.idx = 1 ......... (plain RAM store: publish)       |
   |                    QueueNotify = 0 ....... (MMIO write: THE KICK) ----------+
   |                    poll used.idx ......... (plain RAM load: completion)     |
   |                    hlt ................... (KVM_EXIT_HLT)                    |
   +----------------------------------------------------------------------------+

   The KICK is the only VM exit on the data path; the rings are shared memory,
   touched with no exits. That is why virtio is fast and why a microVM needs no
   legacy device emulation.
```

**Each subsystem of a full microVM monitor, and the sibling project in this lab
that implements/teaches that piece.** Links are relative paths.

| Subsystem (of a real VMM) | What it needs | Sibling project that teaches it | In this capstone |
|---|---|---|---|
| **KVM primitives** — VM/vCPU/RAM, `KVM_RUN`, exit dispatch | `/dev/kvm` ioctls | [`../../01-kernel/15-kvm-hypervisor`](../../01-kernel/15-kvm-hypervisor) | **reused + extended** with an MMIO device bus |
| **Guest boot** — real→protected→long mode, **GDT/IDT, paging** | bring a CPU to 64-bit | [`../../05-capstones/01-from-scratch-os`](../../05-capstones/01-from-scratch-os) | we set long-mode state **directly** via `KVM_SET_SREGS` + build a GDT & page tables in guest RAM (`guest.c`) |
| **The virtqueue** = DMA **descriptor rings** over MMIO | MMIO + producer/consumer rings | [`../../03-networking/11-userspace-nic-driver`](../../03-networking/11-userspace-nic-driver) (VFIO/MMIO/DMA rings, ixy-style) | **fully implemented** (split virtqueue, `virtio.c`) |
| **virtio-net backend** | userspace TCP/IP + a tap device | [`../../03-networking/01-userspace-tcpip`](../../03-networking/01-userspace-tcpip) · [`../../03-networking/08-packet-sniffer`](../../03-networking/08-packet-sniffer) | **stubbed** (answers probe/negotiation) |
| **virtio-blk backend** | a real storage engine (B-tree/WAL/`fsync`) | [`../../02-systems-tools/13-embedded-db`](../../02-systems-tools/13-embedded-db) | **stubbed** (completes reads with zeros) |
| **Guest machine code** — encode & verify | x86-64 encode/decode, the ABI | [`../../04-security-asm/05-x86-64-disassembler`](../../04-security-asm/05-x86-64-disassembler) · [`../../04-security-asm/01-nolibc-programs`](../../04-security-asm/01-nolibc-programs) | hand-assembled payload, byte-annotated (`guest.c`) |
| **Guest RAM** = a host `mmap` region | `mmap`/`brk` allocation | [`../../02-systems-tools/05-malloc`](../../02-systems-tools/05-malloc) | `mmap(MAP_ANONYMOUS)`, registered with `KVM_SET_USER_MEMORY_REGION` |
| **The "jailer"** — sandbox the VMM itself | seccomp, namespaces, cgroups | [`../../02-systems-tools/02-container-runtime`](../../02-systems-tools/02-container-runtime) · [`../../04-security-asm/12-syscall-sandbox`](../../04-security-asm/12-syscall-sandbox) · [`../../05-capstones/02-container-engine`](../../05-capstones/02-container-engine) | **documented, not applied** (see Going further) |
| **Device-backend I/O** — many devices, async | `epoll` / `io_uring` event loop | [`../../03-networking/04-c10k-http-server`](../../03-networking/04-c10k-http-server) · [`../../03-networking/05-io-uring-server`](../../03-networking/05-io-uring-server) | synchronous, single vCPU (kept legible) |

---

## What you'll learn

- **The virtio split virtqueue, device side.** The three shared arrays
  (descriptor table, available ring, used ring), the **free-running 16-bit ring
  indices** and their **wrap arithmetic** (`idx & (size-1)` for the slot,
  `(u16)(avail.idx - last_seen)` for the pending count), descriptor **chaining**
  and the **readable vs. writable** buffer distinction, and the **kick / poll**
  handshake.
- **The virtio-mmio transport.** The register block (`MagicValue`, `Version`,
  `DeviceID`, the feature-negotiation and `Status` handshake, the queue-address
  registers, and `QueueNotify` — the one register whose write is the fast-path VM
  exit).
- **`KVM_EXIT_MMIO` as a device bus.** How an *unbacked* guest-physical window
  turns every access into an exit, how `run->mmio` carries the address/length/
  direction, and how a fault **address decodes** to a device + register.
- **Booting a 64-bit guest without a BIOS.** Building 4-level page tables (with a
  2 MiB `PS` leaf) and a real **GDT** in guest RAM, the `CR0/CR3/CR4/EFER` +
  segment ritual, and the **Linux boot protocol** hand-off: a filled `boot_params`
  zero page (E820 map, `loadflags`, command line) in `%rsi`.
- **The host/guest trust boundary.** Why every guest-supplied descriptor address
  is bounds-checked through `gpa_to_host()` before the monitor dereferences it —
  the check that stands between a device model and a VM escape.
- **Memory ordering across the ring.** The acquire/release pairing that makes the
  driver's ring writes visible before its index bump, and vice versa.
- **Reading the optimizer** (`asm/demo.c`): "mod power-of-two" becomes one `and`,
  "divide by a power-of-two stride" one `shr`, a 16-bit **wrap** is just a
  truncation, and a grouped `switch` becomes a **bitmask + `bt`**.

## Build & run

> **Build & run: Linux only.** `vmm.c` / `virtio.c` / `guest.c` include
> `<linux/kvm.h>` and open `/dev/kvm`. You need Linux (bare metal, or a VM with
> **nested virtualization** enabled) where `/dev/kvm` exists and you are in the
> `kvm` group. It will **not** build on the Windows host this repo may live on;
> use a Linux/WSL2 box or a QEMU VM. The **assembly** target below runs anywhere.

```bash
# 1. Confirm KVM is available and usable:
ls -l /dev/kvm                 # crw-rw----+ root kvm ...
groups | grep -q kvm && echo "in kvm group" \
  || echo "add yourself: sudo usermod -aG kvm $USER; then re-login"

# 2. Build the monitor (needs kernel UAPI headers; they ship with every distro):
make                           # -> ./microvm

# 3. Run it (no sudo needed if you are in the kvm group):
make run                       # == ./microvm
```

Expected output (the `[virtio-console] ...` line is a monitor log on **stderr**;
the message body is the guest's console output on **stdout**):

```
== microVM monitor: virtio-console over MMIO ==
guest RAM: 2048 KiB at GPA 0; virtio-mmio bus at GPA 0x10000000
devices: console@0x10000000  blk@0x10000200(stub)  net@0x10000400(stub)
---- guest console output ----
[virtio-console] driver is up (STATUS=0xf, DRIVER_OK)
Hello from the microVM guest -- this text traveled a real virtio split
virtqueue (descriptor table + available ring + used ring) over an MMIO
transport, and the device side printed it. That round-trip is the lesson.
---- end guest console ----
virtio-console: 1 kick(s), 216 byte(s) printed, 1 buffer(s) used
guest exited: clean hlt
```

Regenerate the teaching assembly (works on **any** host — no Linux/KVM needed):

```bash
make asm                       # writes asm/demo.{s,O0.s,O2.s} via clang cross-target
```

## How it works

| File | Role |
|------|------|
| `vmm.h` | Shared vocabulary: the **guest physical memory map** (every GPA the loader and guest agree on), `struct vm`, and the `gpa_to_host()` trust-boundary contract. Opens with the microVM-vs-sibling framing and the honest-scope statement. |
| `virtio.h` | The **virtio-mmio register offsets**, the **split-virtqueue** on-wire structs (`virtq_desc` / `virtq_avail` / `virtq_used`), status/feature bits, and the `struct virtio_dev` device model. |
| `virtio.c` | The device side: the **MMIO register machine** (`virtio_mmio_read/write`) and **`process_queue`** — the virtqueue walk that reads the available ring, follows the descriptor chain, prints the buffer, and publishes a used-ring completion. The console is real; blk/net are stubs on the same code path. |
| `vmm.c` | The **engine**: `vm_create` (the six-ioctl bring-up), `gpa_to_host`, the `KVM_RUN` loop, `handle_mmio` (routes to the virtio bus), `handle_io` (legacy serial), and `main` (assembles the bus, runs the guest, reports stats). |
| `boot.h` | The **Linux boot protocol** zero page: `boot_params` field offsets, the packed `boot_e820_entry`, and the loadflags/E820 constants — self-contained, no `<asm/bootparam.h>`. |
| `guest.c` | **What boots**: the hand-assembled 64-bit virtio-driver payload (raw bytes, each annotated with its mnemonic), the long-mode page-table + GDT + control-register setup, `fill_boot_params`, and `setup_console_queue` (lays out the virtqueue and programs the console's queue registers). |
| `asm/demo.c` | The pure-logic core lifted out for the assembly study: the virtqueue index/wrap math and the KVM exit-reason dispatch (see below). |

**The data-plane round trip, concretely.** The guest reads `MagicValue` (an MMIO
read → `KVM_EXIT_MMIO` → we return `0x74726976`), writes `Status = DRIVER_OK` (an
MMIO write we record), then stores `avail.idx = 1` **into shared guest RAM with no
exit** to publish descriptor 0 (which points at the message buffer). It writes
`QueueNotify = 0` — **the kick**, the one data-path VM exit — and we run
`process_queue`: read `avail.idx`, take `avail.ring[last_avail & (size-1)]` as the
chain head, walk the descriptor chain, `fwrite` each device-readable buffer to
stdout, then write a `virtq_used_elem` and bump `used.idx` (with a release fence).
The guest polls `used.idx` in RAM (no exit), sees it move, and `hlt`s. That is the
whole of virtio at small scale.

## Assembly notes

Kernel-facing C is **not standalone-compilable on this host**: `vmm.c` / `virtio.c`
/ `guest.c` pull in `<linux/kvm.h>` and friends. Per the repo convention, the
project's most instructive **pure-logic** pieces are extracted into a
self-contained [`asm/demo.c`](asm/demo.c) (it includes nothing, declares its own
types) and compiled to real Linux assembly. `demo.c` holds the two things this
capstone turns on: the **virtqueue index/wrap math** (`vq_ring_slot`, `vq_pending`,
`vq_next`, the descriptor-flag decoders, the `mmio_*` address decode) and the
**KVM exit-reason dispatch** (`kvm_exit_action`).

[`asm/demo.annotated.s`](asm/demo.annotated.s) is the `-O1` output with a comment
on essentially every instruction. The payoffs it highlights:

- **A virtqueue's "mod queue-size" is a single `and`, and "divide by the device
  stride" a single `shr`** — because both the queue size and the MMIO stride are
  powers of two. `vq_ring_slot` is `leal -1(%rsi),%eax ; andl %edi,%eax`;
  `mmio_device_index` is `subq %rsi,%rax ; shrq $9,%rax`. Virtio *requires*
  power-of-two queue sizes precisely so the per-buffer hot path is this cheap.
- **A 16-bit ring-index wrap costs nothing.** `vq_pending` subtracts in 32 bits
  and the result is narrowed to `%ax`; that truncation *is* the mod-65536 that
  makes `avail.idx` wrapping past `0xffff` come out correct.
- **The grouped `switch` becomes a bitmask + `bt`.** Even with `-fno-jump-tables`,
  `kvm_exit_action`'s case groups `{8,9,17}` and `{7,10}` fold into the constants
  `0x20300` and `0x480` — the sets themselves — tested with one `btl`.

Compare the three levels: [`asm/demo.O0.s`](asm/demo.O0.s) (naive, everything
spilled — easiest to trace), [`asm/demo.s`](asm/demo.s) (`-O1`, the annotated
baseline), [`asm/demo.O2.s`](asm/demo.O2.s) (`-O2`; `demo_selftest`, a dozen
constant-input assertions, has evaporated to `xor %eax,%eax`). Regenerate with
`make asm`.

> **On the guest payload:** the *other* hot bytes are the machine-code blob in
> `guest.c`, given as raw bytes with a mnemonic beside each — the guest CPU
> executes exactly those bytes, no assembler in the loop. The `jne .poll`
> displacement is hand-computed from the instruction offsets shown in the header
> comment.

## Going further (the `Stretch:` from the list)

The stretch is **full Firecracker/kvmtool: boot a real `bzImage` + initramfs with
virtio-blk/net.** From this core:

- **Boot a real kernel.** Read the `bzImage` **setup header** (magic `HdrS`,
  `xloadflags` bit 0 for the 64-bit entry), copy the protected-mode kernel to
  `0x100000`, put the command line and initramfs in guest RAM, finish filling the
  `boot_params` we already model, and jump to the 64-bit entry with `%rsi` →
  `boot_params`. `boot.h`/`fill_boot_params` are the scaffold; the setup-header
  parse is the gap.
- **Make the stubs real.** `virtio-blk`: parse the 16-byte request header, service
  `IN`/`OUT` against a disk image with `pread`/`pwrite` + `fsync` (storage engine:
  [`../../02-systems-tools/13-embedded-db`](../../02-systems-tools/13-embedded-db)).
  `virtio-net`: move frames to/from a **tap** device and a userspace stack
  ([`../../03-networking/01-userspace-tcpip`](../../03-networking/01-userspace-tcpip)).
- **Inject interrupts** instead of polling: `KVM_CREATE_IRQCHIP` + `KVM_IRQFD`, or
  MSI-X, so the device can raise the driver's IRQ line — and `KVM_IOEVENTFD` so a
  `QueueNotify` write is fielded without a full userspace exit.
- **SMP and speed.** Multiple vCPUs, each `KVM_RUN` on its own host thread; an
  `epoll`/`io_uring` device-backend loop
  ([`../../03-networking/05-io-uring-server`](../../03-networking/05-io-uring-server));
  `KVM_SET_CPUID2` to present a sane CPU model.
- **Jail the monitor.** Wrap it in the Firecracker "jailer": a seccomp allowlist
  ([`../../04-security-asm/12-syscall-sandbox`](../../04-security-asm/12-syscall-sandbox))
  plus namespaces/cgroups
  ([`../../05-capstones/02-container-engine`](../../05-capstones/02-container-engine)),
  so a device-model bug cannot reach the host.

## References

- **VIRTIO 1.2 spec** (OASIS) — §2.7 *Split Virtqueues*, §4.2 *virtio over MMIO*,
  §5 the device types. The authority for every register offset and ring field here.
- Kernel `Documentation/virt/kvm/api.rst` — `KVM_RUN`, `KVM_EXIT_MMIO`,
  `KVM_SET_USER_MEMORY_REGION`, and the rest of the ioctl ABI.
- `Documentation/x86/boot.rst` and `arch/x86/include/uapi/asm/bootparam.h` — the
  `bzImage`/`boot_params` protocol reproduced in `boot.h`.
- **Firecracker** (`src/vmm`, `src/devices/src/virtio`) — the canonical "how small
  can a production microVM be" answer; its virtio-mmio device models mirror this
  file's structure. **kvmtool** (`tools/kvm`) is the most readable full example.
- The LWN series *"Using the KVM API"* (Josh Triplett) — the ancestor of the
  sibling [`../../01-kernel/15-kvm-hypervisor`](../../01-kernel/15-kvm-hypervisor)
  this capstone builds on.
- Intel SDM Vol. 3, ch. 3–4 (segmentation, paging), ch. 9 (mode switching) —
  ground truth for the long-mode bring-up in `guest.c`.
