/* ===========================================================================
 * vmm.h — shared vocabulary for a tiny Type-2 hypervisor built on /dev/kvm.
 * ===========================================================================
 *
 * WHAT A "TYPE-2 HYPERVISOR" IS, IN ONE PARAGRAPH
 * -----------------------------------------------
 * A Type-1 hypervisor (Xen, ESXi) runs on the bare metal. A Type-2 hypervisor
 * (this program, QEMU/KVM, Firecracker, kvmtool) is an ORDINARY Linux process
 * that borrows the CPU's hardware virtualization (Intel VT-x / AMD-V) through
 * the kernel's KVM subsystem. The kernel does the privileged part — it flips the
 * CPU into "guest mode" with the VMX/SVM instructions that ring 3 may not run —
 * and hands control back to us (a "VM exit") every time the guest does something
 * only the host may decide, such as touching an I/O port. Our whole job as the
 * "VMM" (virtual machine monitor) is:
 *
 *     1. open /dev/kvm and ask it to create a VM and a virtual CPU (vCPU);
 *     2. give the VM some guest physical RAM (a plain host mmap region);
 *     3. set the vCPU's architectural registers to a known start state;
 *     4. call KVM_RUN, which blocks until the guest exits back to us;
 *     5. look at *why* it exited, service the request, and KVM_RUN again.
 *
 * That loop — steps 4 and 5 — is the entire soul of a hypervisor. Everything a
 * real VMM adds (virtio devices, a BIOS, an interrupt controller, live
 * migration) hangs off the same KVM_RUN loop.
 *
 * THE KVM API IS ALL ioctl()
 * --------------------------
 * KVM exposes three *levels* of file descriptor, and you talk to each with
 * ioctl(2). This nesting is the mental model to hold onto:
 *
 *     /dev/kvm     (the "system" fd) -- KVM_CREATE_VM ---------->  vmfd
 *     vmfd         (one per VM)      -- KVM_CREATE_VCPU -------->  vcpufd
 *                                    -- KVM_SET_USER_MEMORY_REGION
 *     vcpufd       (one per vCPU)    -- KVM_RUN, KVM_GET/SET_REGS, ...
 *
 * A version query (KVM_GET_API_VERSION, which must be 12 — frozen at 12 for over
 * a decade) rounds it out. There is no libkvm; the ioctl numbers *are* the ABI,
 * and they live in <linux/kvm.h>.
 *
 * This header holds only the glue types shared between vmm.c (lifecycle + run
 * loop) and guest.c (the two guest payloads and their register setup). All the
 * heavy explanation lives at each ioctl call site in the .c files.
 *
 * LINUX ONLY. <linux/kvm.h> and /dev/kvm exist on Linux. This will not build on
 * the Windows host this repo may sit on; run it on a Linux box (or a QEMU VM
 * with nested virt). The asm/ deliverable is the one piece that builds anywhere.
 * =========================================================================== */

#ifndef VMM_H
#define VMM_H

#include <stddef.h>     /* size_t                                              */
#include <stdint.h>     /* uint8_t / uint32_t / uint64_t                       */
#include <linux/kvm.h>  /* struct kvm_run, KVM_EXIT_*, all the ioctl numbers   */

/* The I/O port our guests write bytes to. 0x3f8 is the legacy base address of
 * COM1, the first PC serial port. We do not emulate a real UART; we simply
 * agree with the guest that "an OUT to 0x3f8 means: print this byte." The guest
 * executes `out %al, (%dx)` with dx=0x3f8, the CPU traps to KVM, KVM cannot
 * service the port itself, so it exits to us with KVM_EXIT_IO and we do the
 * printing. This is the smallest possible paravirtual console. */
#define SERIAL_PORT 0x3f8

/* Guest physical RAM we hand the VM: 2 MiB, page-aligned. Two reasons for 2 MiB
 * exactly: (a) it is a whole number of 4 KiB pages, which KVM_SET_USER_MEMORY_
 * REGION requires; (b) it is exactly what a single x86-64 2 MiB "huge" page-
 * table entry maps, so the long-mode example can identity-map all of guest RAM
 * with ONE page-directory entry (see guest.c). */
#define GUEST_MEM_SIZE (2u * 1024u * 1024u)

/* ---------------------------------------------------------------------------
 * struct vm — everything we hold onto for one running virtual machine.
 *
 * The three ints are the nested KVM file descriptors described above. `mem` is
 * the host virtual address of the guest's physical RAM (a MAP_ANONYMOUS mmap);
 * from the guest's point of view this same memory starts at guest-physical 0.
 * `run` is the kvm_run communication page — a struct KVM shares with us by
 * mmap()ing it over the vcpufd, so that on every exit we can read exit_reason
 * and the exit-specific union WITHOUT another ioctl. It is the mailbox between
 * kernel and VMM.
 * ------------------------------------------------------------------------- */
struct vm {
    int             kvmfd;      /* fd for /dev/kvm  (the KVM subsystem itself)  */
    int             vmfd;       /* fd for this VM   (KVM_CREATE_VM)             */
    int             vcpufd;     /* fd for the vCPU  (KVM_CREATE_VCPU)           */
    uint8_t        *mem;        /* host VA of guest RAM; guest sees it at GPA 0 */
    size_t          mem_size;   /* bytes of guest RAM (== GUEST_MEM_SIZE)       */
    struct kvm_run *run;        /* mmap'd shared communication page (kvm_run)   */
    size_t          run_size;   /* its length (KVM_GET_VCPU_MMAP_SIZE)          */
};

/* --- vmm.c: VM lifecycle and the KVM_RUN dispatch loop ------------------- */

/* Open /dev/kvm, create the VM, wire up `mem_size` bytes of guest RAM, create
 * the single vCPU, and mmap its kvm_run page. Aborts the process on any failure
 * (this is teaching code; every ioctl is checked and a clear message printed). */
void vm_create(struct vm *vm, size_t mem_size);

/* Reverse of vm_create: munmap the kvm_run page and guest RAM, close all fds. */
void vm_destroy(struct vm *vm);

/* The heart of the hypervisor. KVM_RUN in a loop, dispatching on exit_reason
 * until the guest halts (KVM_EXIT_HLT) or something fails. Returns 0 on a clean
 * halt, nonzero on an error exit. */
int vm_run_loop(struct vm *vm);

/* Human-readable name for a KVM_EXIT_* code, for diagnostics. Defined in vmm.c;
 * its pure-logic twin is the star of asm/demo.c. */
const char *kvm_exit_name(uint32_t exit_reason);

/* --- guest.c: the two guest payloads and their initial CPU state --------- */

/* Load the 16-bit real-mode payload and set the vCPU to boot it. */
void guest_setup_real_mode(struct vm *vm);

/* Load the 64-bit long-mode payload, build its page tables, and set the vCPU
 * (control registers, EFER, segment descriptors) to boot straight into 64-bit
 * mode. */
void guest_setup_long_mode(struct vm *vm);

#endif /* VMM_H */
