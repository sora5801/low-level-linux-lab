# A from-scratch OS 🟥 (teaching-core)

**What it is.** A bootable, from-scratch operating-system **teaching-core** for
x86-64. It is a real disk image you boot in QEMU: a **512-byte MBR boot sector**
carries the CPU by hand from **16-bit real mode → 32-bit protected mode → 64-bit
long mode** — setting `CR0.PE`, loading a flat **GDT**, far-jumping to reload
`CS`, building **4-level PML4 page tables**, and enabling **PAE + EFER.LME +
`CR0.PG`** — then jumps into a **freestanding C kernel** (built `-ffreestanding
-nostdlib -mno-red-zone` with a linker script). The kernel prints to the **VGA
text buffer at `0xB8000`** and the **`0x3F8` serial port**, installs a 256-gate
**IDT**, **remaps the 8259 PIC** off the CPU's exception vectors, programs the
**PIT** timer, and services **timer (IRQ0)** and **keyboard (IRQ1)** interrupts.
Type on the keyboard and it echoes; every second the timer prints its tick count.

This is a 🟥 **teaching-core, and it is honest about scope.** It runs end-to-end
— boot, mode switches, paging, interrupts, two device drivers — which is exactly
the hard, under-documented "how does a computer even get to `main`" part that
most tutorials skip. It deliberately stops at a single-tasking, ring-0,
interrupt-driven kernel. It does **not** yet implement a physical/virtual memory
manager, a preemptive scheduler, system calls, a userspace/ring-3, or a
filesystem. The **[Architecture](#architecture)** section maps each of those
missing pieces to the sibling project in this lab that builds that exact concept
in isolation, and **[Going further](#going-further)** describes the path to a
full OS. xv6 and the OSDev wiki are the guides throughout.

## What you'll learn

- **The three CPU modes and the transitions between them** — why real mode can
  only see 1 MiB, why you `lgdt` a flat GDT and set `CR0.PE` to enter protected
  mode, why the **far jump is the only way to reload `CS`**, and why long mode
  *requires* paging so you must build page tables before you can enter it.
- **The long-mode bring-up checklist** — PML4→PDPT→PD with 2 MiB pages, `CR3`,
  `CR4.PAE`, the `EFER` MSR's `LME` bit, and `CR0.PG`, in the one order that works.
- **Loading code off a disk with the BIOS** — INT 13h extended read (AH=42h) and
  a Disk Address Packet, the geometry-independent way to pull the kernel into RAM.
- **The A20 gate**, that 1980s-compatibility wart you must un-stick to address
  memory above 1 MiB.
- **Freestanding C** — building with no libc/CRT, providing your own `memcpy`/
  `memset`, zeroing `.bss` yourself, and why `-mno-red-zone` is mandatory once
  interrupts exist.
- **The IDT and interrupt handling on x86-64** — 16-byte gate descriptors, the
  CPU-pushed interrupt frame, the assembly stub that saves the GP registers and
  the *exact* stack layout it must build, and `iretq`.
- **The 8259 PIC** — why vectors 0–31 are off-limits, the ICW1–ICW4 remap
  handshake, masking, and the End-Of-Interrupt that keeps IRQs flowing.
- **Port-mapped I/O** — `in`/`out`, the VGA CRTC index/data register pair, the
  16550 UART, the PIT divisor, and the PS/2 keyboard scancode stream.

## Build & run

**Platform: Linux or WSL2.** You need an ELF toolchain (gcc *or* clang + GNU
binutils `ld`/`objcopy`), coreutils `truncate`, and `qemu-system-x86_64`. On
Debian/Ubuntu:

```bash
sudo apt install build-essential binutils qemu-system-x86 coreutils
```

Build the bootable image and run it:

```bash
make            # -> os.img (512-byte MBR + padded kernel)
make run        # boot in QEMU: a VGA window opens; COM1 is mirrored to your terminal
```

Headless (everything on the serial console — good over SSH or in CI logs):

```bash
make run-serial # qemu-system-x86_64 ... -nographic
```

Expected output (on both VGA and serial):

```
from-scratch-os: reached 64-bit C kernel (long mode active)
  [ok] serial COM1 @ 38400 8N1, VGA text @ 0xB8000
  [ok] 8259 PIC remapped to vectors 0x20..0x2F
  [ok] IDT loaded (256 gates: 32 exceptions + 16 IRQs)
  [ok] PIT @ 100 Hz; IRQ0/IRQ1 unmasked
interrupts enabled — type on the keyboard; timer prints each second
[timer] ticks=100
[timer] ticks=200
```

…and keys you press echo to the screen. Quit QEMU with `Ctrl-A x` (in
`-nographic`) or by closing the window.

**Debug it with gdb** (single-step the mode switches — the best way to *watch*
real→protected→long happen):

```bash
make debug                       # QEMU freezes, listening on tcp:1234
# in another terminal:
gdb -ex 'target remote :1234' -ex 'set architecture i386:x86-64' \
    -ex 'break *0x7c00' -ex 'continue'
```

**Regenerate the teaching assembly** (host-independent — clang cross-targets
Linux):

```bash
make asm        # writes asm/demo.{O0.s,s,O2.s}; asm/demo.annotated.s is hand-written
```

## How it works

A tour in boot order. The image is `boot.bin` (sector 0) followed by the padded
`kernel.bin`; the BIOS loads only sector 0, and sector 0 loads the rest.

| File | Role |
|------|------|
| **`boot/boot.S`** | The star. The 512-byte MBR: real mode setup, A20, INT 13h kernel load, then the full real→protected→long transition, ending in a jump to the C kernel at `0x10000`. Every instruction is annotated. |
| **`boot/boot.ld`** | Links `boot.S` to a flat image at `0x7C00` and stamps the `0x55AA` signature at byte 510; errors if the code ever exceeds 510 bytes. |
| **`kernel/entry.S`** | The kernel image's first byte (`_start`): sets up `RSP`, zeroes `.bss`, calls `kmain`. |
| **`kernel/kmain.c`** | Init in hardware-mandated order: consoles → PIC remap → IDT → PIT → unmask IRQs → `sti` → `hlt` idle loop. |
| **`kernel/vga.c` / `.h`** | The `0xB8000` text console: the **(row,col)→cell-offset math**, color attributes, scrolling, and the hardware-cursor CRTC writes. |
| **`kernel/serial.c` / `.h`** | The COM1 16550 UART: divisor/line-control bring-up and polled transmit. |
| **`kernel/idt.c` / `.h`** | Builds all 256 IDT gates from the stub table and `lidt`s it. Defines `struct regs`, the saved-register frame. |
| **`kernel/isr.S`** | One stub per vector (macro-generated), the uniform-frame trick for error-code vs no-error-code exceptions, and the common save/call/restore/`iretq` body. |
| **`kernel/isr.c`** | The C dispatcher: prints+halts on an exception; services timer/keyboard and sends the PIC its EOI on an IRQ. |
| **`kernel/pic.c` / `.h`** | The 8259 remap handshake, per-line masking, and EOI. |
| **`kernel/mem.c`** | `memcpy`/`memmove`/`memset`/`memcmp` — the four the compiler may call on its own, which a freestanding image must supply. |
| **`kernel/io.h`** | `inb`/`outb`/`io_wait` inline-asm port helpers. |
| **`linker.ld`** | Lays the kernel out at `0x10000`, `_start` first, and exports `__bss_start`/`__bss_end`. |

**The memory map the boot code imposes:**

```
  0x00000000 ┌───────────────────────────┐
             │ real-mode IVT + BIOS data │  (left alone)
  0x00001000 ├───────────────────────────┤
             │ page tables               │  PML4 @1000, PDPT @2000, PD @3000
  0x00007C00 ├───────────────────────────┤
             │ boot sector (512 B)       │  <- BIOS loads + runs this
             │ real-mode stack (grows ↓) │
  0x00010000 ├───────────────────────────┤
             │ kernel image (from disk)  │  _start, code, rodata, data, bss, stack
             │  ... runs in long mode    │
  0x000B8000 ├───────────────────────────┤
             │ VGA text buffer (80x25)   │  <- kernel writes glyphs here
             └───────────────────────────┘
  The PD identity-maps the first 1 GiB (512 × 2 MiB pages), so every address
  above has virtual == physical — no address translation surprises.
```

## Architecture

The teaching-core is the **highlighted** band below. Everything above the dashed
line is what a *complete* OS layers on top; this project stops at the line and
documents the climb. Each higher subsystem names the **sibling project in this
lab that implements that concept in isolation** — build those, then imagine them
hosted inside this kernel.

```
        ┌──────────────────────────────────────────────────────────────┐
        │  USERSPACE                                                     │
        │   shells, tools, a libc, ELF programs        (ring 3)          │
        ├──────────────────────────────────────────────────────────────┤
        │  SYSTEM CALL INTERFACE   syscall/sysret, the syscall table     │
        │  FILESYSTEM              a VFS + a RAM/disk fs                  │
        │  SCHEDULER               preemptive, context switch in asm     │
        │  MEMORY MANAGER          phys frame allocator + kernel heap    │
        │  MORE DRIVERS            block, PS/2 fully, framebuffer        │
        ┆┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄ teaching-core boundary ┄┄┄┄┄┄┄┄┄┄┄┄┄┆
        │ ██████████████████  THIS PROJECT RUNS  ██████████████████████  │
        │  INTERRUPTS   IDT (256 gates) · PIC remap · PIT · IRQ0/IRQ1    │
        │  DRIVERS      VGA text (0xB8000) · serial UART (0x3F8) · kbd   │
        │  PAGING       PML4→PDPT→PD, 1 GiB identity map, PAE+LME+PG     │
        │  CPU MODES    real → protected (GDT, CR0.PE) → long (far jmp)  │
        │  BOOT         512-byte MBR, A20, INT 13h kernel load           │
        ├──────────────────────────────────────────────────────────────┤
        │  FIRMWARE / HARDWARE   BIOS (SeaBIOS) · QEMU x86-64            │
        └──────────────────────────────────────────────────────────────┘
```

**Subsystem → the sibling project that implements it.** The teaching-core builds
the bottom five rows itself; the table maps the *full-OS* rows (and the deeper
theory behind the core rows) to where this lab teaches them standalone.

| Full-OS subsystem | What it needs | Sibling project (relative path) |
|-------------------|---------------|---------------------------------|
| Physical/virtual memory manager, kernel heap | frame allocator, `brk`/free-list allocator design, coalescing | [`../../02-systems-tools/05-malloc`](../../02-systems-tools/05-malloc) |
| Preemptive scheduler | the actual register-save **context switch in assembly**, an M:N run queue | [`../../02-systems-tools/18-green-threads`](../../02-systems-tools/18-green-threads) |
| Scheduling policy | how a scheduler *decides* (CFS/EEVDF-style, sched_ext) | [`../../01-kernel/12-sched-ext-scheduler`](../../01-kernel/12-sched-ext-scheduler) |
| System calls | the syscall table, entry/dispatch, `SYSCALL_DEFINEn` | [`../../01-kernel/11-add-syscall`](../../01-kernel/11-add-syscall) |
| The userspace/ABI side of a syscall | raw `syscall`, own `_start`, the SysV ABI | [`../../04-security-asm/01-nolibc-programs`](../../04-security-asm/01-nolibc-programs) |
| A C runtime for userspace | CRT startup, `auxv`/TLS, `printf`, syscalls | [`../../04-security-asm/04-mini-libc`](../../04-security-asm/04-mini-libc) |
| Filesystem (the RAM FS in "going further") | VFS: `super`/`inode`/`dentry`, `mount` | [`../../01-kernel/04-in-memory-fs`](../../01-kernel/04-in-memory-fs) |
| Loading user ELF programs | ELF load, relocations, PLT/GOT, `.init_array` | [`../../02-systems-tools/07-dynamic-linker`](../../02-systems-tools/07-dynamic-linker) |
| Concurrency once IRQs/SMP exist | atomics, memory ordering, `futex`-style waits | [`../../02-systems-tools/19-sync-primitives`](../../02-systems-tools/19-sync-primitives) |
| Lock-free kernel data structures | CAS, hazard pointers, RCU-flavored queues | [`../../03-networking/13-lockfree-structures`](../../03-networking/13-lockfree-structures) |
| Device-driver model (chardev-like) | `file_operations`, `ioctl`, wait queues, `mmap` | [`../../01-kernel/02-char-device-ioctl`](../../01-kernel/02-char-device-ioctl) |
| The paging/segment-descriptor theory (from the *other* side) | building `CR3`/PML4, segment descriptors, long-mode entry for a guest | [`../../01-kernel/15-kvm-hypervisor`](../../01-kernel/15-kvm-hypervisor) |
| Running/instrumenting a kernel in a VM | a KVM VMM; a microVM monitor | [`../../01-kernel/15-kvm-hypervisor`](../../01-kernel/15-kvm-hypervisor), [`../03-microvm-monitor`](../03-microvm-monitor) |
| The raw-syscall/ABI reference for all asm | the annotated `_start` example | [`../../04-security-asm/01-nolibc-programs/asm/hello.annotated.s`](../../04-security-asm/01-nolibc-programs/asm/hello.annotated.s) |

Read this honestly: the KVM hypervisor project already does the **long-mode
page-table + segment-descriptor bring-up** — from userspace, for a *guest*. This
capstone does the same ritual for real, on the *bare metal*. Comparing the two
is one of the most instructive pairings in the lab.

## Assembly notes

Two hand-written assembly artifacts, to two different standards:

- **`boot/boot.S`** is the **star annotated artifact** for this capstone — it is
  the real boot code *and* its own line-by-line explanation. Read it for the
  mode transitions: the `lgdt`/`CR0.PE`/far-jump into protected mode, the page-
  table build loop, the `CR4.PAE` / `EFER.LME` / `CR0.PG` sequence, and the
  second far jump (into a code descriptor with the **Long bit** set) that finally
  makes the CPU 64 bits wide. `kernel/isr.S` and `kernel/entry.S` are heavily
  commented too — `isr.S` in particular explains the *exact* push order that
  makes the on-stack frame match `struct regs`.

- **`asm/demo.c` → `asm/demo.annotated.s`** is the standard C-to-asm deliverable.
  `demo.c` is the console driver's **pure integer logic** with every privileged
  instruction and header removed, so clang can turn it into portable assembly on
  any host. It contains the **(row,col)→cell-offset math**, the cell/attribute
  packing, the **cursor hi/lo byte split**, the **port-address helper**, the UART
  status-bit test, and the PIT divisor. The hand-annotated `demo.annotated.s`
  (from the `-O1` output) walks every instruction and highlights the lessons:
  `row*80` lowered to `lea (%rdi,%rdi,4)` + `shl $4` (**no `imul`**), a narrow
  `u8` return deleting an `& 0xFF`, a runtime `divl` (vs a constant's multiply),
  and the whole self-test folding to `xor %eax,%eax`. Compare `demo.O0.s` (naive)
  and `demo.O2.s` (optimizer unleashed).

## Going further

The list's **stretch** for this project is the climb to a real OS. In rough
dependency order, and mapped to where each piece is taught above:

1. **A physical frame allocator + kernel heap** — parse the BIOS/E820 memory map,
   hand out 4 KiB frames, build a `kmalloc`. (Allocator design:
   [`05-malloc`](../../02-systems-tools/05-malloc).)
2. **A real virtual-memory manager** — per-process address spaces, demand paging,
   a `#PF` handler that actually maps pages instead of halting.
3. **Preemptive multitasking** — a TSS + per-task kernel stacks, a timer-driven
   scheduler, and the assembly context switch (mechanism:
   [`18-green-threads`](../../02-systems-tools/18-green-threads); policy:
   [`12-sched-ext-scheduler`](../../01-kernel/12-sched-ext-scheduler)).
4. **Ring 3 + system calls** — a user code/data GDT segment, `syscall`/`sysret`
   MSR setup, and a dispatch table ([`11-add-syscall`](../../01-kernel/11-add-syscall),
   userspace side [`01-nolibc-programs`](../../04-security-asm/01-nolibc-programs),
   [`04-mini-libc`](../../04-security-asm/04-mini-libc)).
5. **A RAM filesystem + VFS** so those user programs have files
   ([`04-in-memory-fs`](../../01-kernel/04-in-memory-fs)), and an ELF loader to
   run them ([`07-dynamic-linker`](../../02-systems-tools/07-dynamic-linker)).

What **production** does that this core does not: use UEFI + a real bootloader
(GRUB/Limine) instead of a hand-rolled MBR; run on all cores (SMP: APIC/x2APIC,
per-CPU data, real locking — [`19-sync-primitives`](../../02-systems-tools/19-sync-primitives),
[`13-lockfree-structures`](../../03-networking/13-lockfree-structures)); set up
the LAPIC/IOAPIC timer instead of the legacy PIT/PIC; enable `NX`, SMEP/SMAP,
KASLR, and W^X; and carry hundreds of drivers behind a device model
([`02-char-device-ioctl`](../../01-kernel/02-char-device-ioctl)).

## References

- **xv6** (MIT) — `bootasm.S`, `bootmain.c`, `entry.S`, `trapasm.S`, `vm.c`. The
  canonical readable teaching OS; this core mirrors its boot path.
- **The OSDev wiki** — *Bare Bones*, *Setting Up Long Mode*, *GDT Tutorial*,
  *Interrupt Descriptor Table*, *8259 PIC*, *Serial Ports*, *A20 Line*.
- **Intel SDM Vol. 3A** — ch. 9 (mode switching), ch. 4 (paging), ch. 6
  (interrupts/exceptions), and the segment/gate descriptor formats.
- **AMD64 APM Vol. 2** — long-mode activation (`EFER.LME`/`LMA`, `CR0.PG`) and the
  long-mode segmentation model.
- Sibling lab projects, per the [Architecture](#architecture) table above —
  build them to fill in each layer this core leaves for later.
```
