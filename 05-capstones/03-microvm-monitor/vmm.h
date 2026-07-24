/* ===========================================================================
 * vmm.h — a Firecracker-shaped microVM monitor over /dev/kvm: shared vocabulary.
 * ===========================================================================
 *
 * WHAT THIS CAPSTONE IS (AND HOW IT DIFFERS FROM 01-kernel/15-kvm-hypervisor)
 * --------------------------------------------------------------------------
 * The sibling project ../../01-kernel/15-kvm-hypervisor teaches the RAW KVM
 * primitives: open /dev/kvm, make a VM + vCPU, hand it RAM, and run the KVM_RUN
 * loop with a serial console built from bare `OUT 0x3f8` port writes. This
 * capstone builds the NEXT layer — the one that turns "a CPU in a box" into "a
 * machine a real OS could boot on": a **virtio device model over an MMIO
 * transport**, the thing that makes Firecracker/kvmtool/QEMU-microvm useful.
 *
 * The teaching-core here, runnable on Linux with /dev/kvm, demonstrates the
 * WHOLE virtio round-trip at small scale:
 *
 *     guest (a tiny hand-written 64-bit driver)                the monitor (us)
 *     ------------------------------------------      ---------------------------
 *     read the virtio-mmio MagicValue register  --->  KVM_EXIT_MMIO (read):
 *                                                     return 0x74726976 ('virt')
 *     write Status = DRIVER_OK                   --->  KVM_EXIT_MMIO (write):
 *                                                     record handshake
 *     publish a buffer: avail.idx = 1            (a plain store to guest RAM)
 *     write QueueNotify (the "kick")             --->  KVM_EXIT_MMIO (write):
 *                                                     WALK THE VIRTQUEUE:
 *                                                       - read the avail ring
 *                                                       - follow the descriptor
 *                                                         chain to the buffer
 *                                                       - print it (console TX)
 *                                                       - publish a used-ring elem
 *                                                       - bump used.idx
 *     poll used.idx until it moves               (a plain load from guest RAM)
 *     hlt                                        --->  KVM_EXIT_HLT: clean stop
 *
 * That is a genuine split virtqueue (descriptor table + available ring + used
 * ring) driven through a genuine virtio-mmio register block, serviced by a
 * genuine KVM_EXIT_MMIO handler. virtio-blk and virtio-net are present as honest
 * STUBS on the same bus — they answer the probe/negotiation registers correctly
 * (so a driver could enumerate them) but do not move real block/packet data.
 *
 * HONEST SCOPE (this is a teaching-core; see the README's Architecture table):
 *   - We run our OWN hand-written 64-bit payload, not a real Linux bzImage. We DO
 *     set up the exact 64-bit-entry CONTRACT a bzImage expects (long mode on, a
 *     GDT and page tables in guest RAM, and %rsi -> a filled `boot_params` zero
 *     page with an E820 map — see boot.h). Parsing a real bzImage setup header
 *     and copying the kernel to 0x100000 is documented as the remaining gap.
 *   - The DRIVER-side queue setup (programming the queue base addresses and
 *     QueueReady) is done by the host loader for legibility; a real guest driver
 *     would do it from inside the VM. The DEVICE-side processing — the part this
 *     capstone is about — is fully modeled.
 *   - We POLL the used ring instead of injecting an interrupt, so we need no
 *     in-kernel IRQCHIP. Polling is a valid virtio mode; IRQ injection is noted
 *     as the production path.
 *
 * THE KVM API IS ALL ioctl(). Three nested file descriptors:
 *     /dev/kvm (system) -- KVM_CREATE_VM --> vmfd -- KVM_CREATE_VCPU --> vcpufd
 * There is no libkvm; the ioctl numbers in <linux/kvm.h> ARE the ABI.
 *
 * LINUX ONLY to build/run: this file and vmm.c/virtio.c/guest.c include
 * <linux/kvm.h> and open /dev/kvm, which exist only on Linux. Run it on a Linux
 * box (bare metal or a VM with nested virt) where you are in the `kvm` group. The
 * asm/ deliverable is the one piece that builds anywhere.
 * =========================================================================== */

#ifndef VMM_H
#define VMM_H

#include <stddef.h>     /* size_t                                              */
#include <stdint.h>     /* uint8_t / uint16_t / uint32_t / uint64_t            */
#include <linux/kvm.h>  /* struct kvm_run, KVM_EXIT_*, every ioctl number      */

#include "virtio.h"     /* struct virtio_dev, the virtio-mmio bus              */

/* ===========================================================================
 * THE GUEST PHYSICAL MEMORY MAP
 * ===========================================================================
 * Everything the guest and the loader agree on lives here so vmm.c, guest.c, and
 * virtio.c never disagree about an address. All of these except the MMIO window
 * sit inside the first 2 MiB of guest RAM (which one 2 MiB page-table entry maps).
 * The MMIO window is DELIBERATELY outside RAM: a guest access there hits no
 * memory slot and so traps to us as KVM_EXIT_MMIO — that fault IS the device bus.
 * =========================================================================== */

/* Guest RAM: 2 MiB, page-aligned. 2 MiB exactly because a single x86-64 2 MiB
 * "huge" page-directory entry maps all of it, so the long-mode identity map for
 * RAM is one PD write (see guest.c). */
#define GUEST_RAM_SIZE   (2u * 1024u * 1024u)   /* 0x200000                     */

/* Guest code entry and the structures the loader lays down, by GPA. Each is on
 * its own 4 KiB page for clarity (page tables MUST be page-aligned; the rest is
 * just tidy). */
#define GPA_CODE          0x0000u   /* the hand-written 64-bit guest payload     */
#define GPA_PML4          0x1000u   /* level-4 page table (1 entry used)         */
#define GPA_PDPT          0x2000u   /* level-3 page table (1 entry used)         */
#define GPA_PD            0x3000u   /* level-2: 2 MiB pages (RAM + MMIO entries)  */
#define GPA_GDT           0x4000u   /* a real GDT in guest RAM (null,code,data)   */
#define GPA_VQ_DESC       0x5000u   /* virtqueue descriptor table                */
#define GPA_VQ_AVAIL      0x6000u   /* virtqueue available ring (driver-filled)   */
#define GPA_VQ_USED       0x7000u   /* virtqueue used ring (device-filled)        */
#define GPA_TX_BUF        0x8000u   /* the console buffer the guest "sends"        */
#define GPA_BOOT_PARAMS   0x9000u   /* the Linux boot protocol "zero page"         */
#define GPA_STACK_TOP     GUEST_RAM_SIZE  /* rsp starts at end of RAM, grows down */

/* The virtio-mmio bus: a window of guest-physical addresses with NO backing RAM,
 * so every access faults out to our handle_mmio(). 0x10000000 (256 MiB) is well
 * clear of our 2 MiB of RAM. Devices are spaced VIRTIO_MMIO_STRIDE (0x200) apart,
 * exactly as QEMU/Linux lay out virtio-mmio; the fault ADDRESS alone tells us
 * which device and which register (see virtio.c and asm/demo.c's mmio_* math). */
#define GPA_MMIO_BASE     0x10000000ull

/* The one legacy PIO port we still answer, for parity with the sibling project
 * and so a guest that prefers `OUT 0x3f8` to virtio still prints. Our default
 * guest uses virtio + hlt, not this. */
#define SERIAL_PORT       0x3f8

/* Queue size for our single console queue. MUST be a power of two (virtio rule):
 * that is what makes "index mod size" a bitmask AND (see asm/demo.c). 8 is tiny
 * on purpose — you can print the whole ring and still see it on one screen. */
#define VQ_SIZE           8u

/* ---------------------------------------------------------------------------
 * struct vm — everything we hold for one running microVM.
 *
 * kvmfd/vmfd/vcpufd are the three nested KVM descriptors. `mem` is the host VA of
 * guest-physical 0 (an anonymous mmap); the guest sees the same bytes at GPA 0.
 * `run` is the kvm_run communication page KVM shares with us over the vcpufd.
 * `devs`/`ndev` is the virtio-mmio bus: the array of device models handle_mmio
 * routes faults to. Keeping the bus on the vm makes the run loop's dispatch a
 * pure function of (exit reason, this struct).
 * ------------------------------------------------------------------------- */
struct vm {
    int              kvmfd;      /* fd for /dev/kvm (the KVM subsystem)          */
    int              vmfd;       /* fd for this VM  (KVM_CREATE_VM)              */
    int              vcpufd;     /* fd for the vCPU (KVM_CREATE_VCPU)            */
    uint8_t         *mem;        /* host VA of guest RAM; guest sees it at GPA 0 */
    size_t           mem_size;   /* bytes of guest RAM (== GUEST_RAM_SIZE)       */
    struct kvm_run  *run;        /* mmap'd shared communication page (kvm_run)   */
    size_t           run_size;   /* its length (KVM_GET_VCPU_MMAP_SIZE)          */

    struct virtio_dev *devs;     /* the virtio-mmio bus: device models           */
    size_t             ndev;     /* number of devices on the bus                 */
};

/* ---------------------------------------------------------------------------
 * gpa_to_host — translate a guest-physical address to a host pointer into guest
 * RAM, bounds-checked. This is THE trust boundary: the guest controls the GPAs
 * inside descriptors, so every virtqueue buffer address must be validated to lie
 * within [0, mem_size) and not to run off the end (gpa + len must not overflow or
 * exceed RAM) before we dereference it. A missing check here is the classic VMM
 * escape: a malicious descriptor pointing outside RAM would let the guest read or
 * write host memory. Returns NULL on any violation; callers MUST check.
 * ------------------------------------------------------------------------- */
void *gpa_to_host(struct vm *vm, uint64_t gpa, uint64_t len);

/* --- vmm.c: VM lifecycle and the KVM_RUN dispatch loop ------------------- */

/* Open /dev/kvm, create the VM, wire up `mem_size` bytes of guest RAM, create the
 * single vCPU, and mmap its kvm_run page. Aborts on any failure (teaching code:
 * every ioctl is checked and a clear message printed). */
void vm_create(struct vm *vm, size_t mem_size);

/* Reverse of vm_create: munmap the kvm_run page and guest RAM, close all fds. */
void vm_destroy(struct vm *vm);

/* The heart of the monitor. KVM_RUN in a loop, dispatching on exit_reason:
 * KVM_EXIT_MMIO -> the virtio-mmio bus, KVM_EXIT_IO -> the legacy serial port,
 * KVM_EXIT_HLT -> clean stop. Returns 0 on a clean halt, nonzero on error. */
int vm_run_loop(struct vm *vm);

/* Human-readable name for a KVM_EXIT_* code, for diagnostics. */
const char *kvm_exit_name(uint32_t exit_reason);

/* --- guest.c: the guest payload, its long-mode bring-up, and boot_params -- */

/* Lay down the 64-bit virtio-driver payload, build page tables + GDT, fill the
 * boot_params zero page, program the console queue on the guest's behalf, and set
 * the vCPU registers so KVM_RUN enters straight into 64-bit mode at GPA_CODE. */
void guest_setup(struct vm *vm);

#endif /* VMM_H */
