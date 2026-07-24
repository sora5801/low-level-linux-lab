/* ===========================================================================
 * guest.c — the hand-written 64-bit guest, its long-mode+GDT bring-up, the
 *           boot_params zero page, and the console virtqueue the loader lays out.
 * ===========================================================================
 *
 * vmm.c/virtio.c know how to RUN a microVM and service a virtio bus; this file
 * defines WHAT boots. It has four jobs, in the order guest_setup() does them:
 *
 *   1. Place a tiny 64-bit machine-code payload at GPA 0 that acts as a minimal
 *      virtio-console DRIVER: it reads the device's MagicValue, completes the
 *      status handshake, publishes one buffer into the available ring, kicks the
 *      device via an MMIO write, polls the used ring for completion, and halts.
 *
 *   2. Build the long-mode CPU state a bzImage's 64-bit entry expects: a 4-level
 *      identity page map (RAM at GPA 0, and the virtio-mmio window at 0x10000000
 *      mapped-but-unbacked so it traps), a real 3-entry GDT in guest RAM, and the
 *      control registers / EFER / segments that put the CPU in 64-bit mode.
 *
 *   3. Fill the boot_params "zero page" (E820 map, loadflags, command line) and
 *      point %rsi at it — the exact hand-off contract a real Linux kernel entry
 *      consumes. (Our demo payload doesn't parse it; see the note at %rsi below.)
 *
 *   4. Lay out the split virtqueue in guest RAM (descriptor table + available +
 *      used rings) and program the console device's queue registers. In a real
 *      system the guest driver would do this from inside the VM; we do it in the
 *      host loader so the guest machine code stays short and legible. The DEVICE
 *      side — the part this capstone teaches — is fully in virtio.c.
 *
 * The payload is given as raw bytes with the mnemonic beside each, because that
 * IS the lesson: the guest CPU executes exactly these bytes, no assembler in the
 * loop. Displacements (the `jne` below) are hand-computed from the offsets shown.
 * =========================================================================== */

#include "vmm.h"
#include "boot.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>     /* memcpy, memset, strlen                              */
#include <sys/ioctl.h>

static void guest_die(const char *what) { perror(what); exit(EXIT_FAILURE); }

/* ===========================================================================
 * 1. THE 64-BIT VIRTIO-CONSOLE DRIVER PAYLOAD
 * ===========================================================================
 * Hand-assembled. Offsets on the left are the guest-physical address of each
 * instruction (entry is GPA 0), which is how the `jne` displacement is computed.
 *
 *  off  bytes                     instruction              meaning
 *  0    bf 00 00 00 10            mov  $0x10000000,%edi    edi = console MMIO base
 *  5    8b 07                     mov  (%rdi),%eax         eax = MagicValue  [MMIO READ]
 *                                                          (a real driver checks == 'virt')
 *  7    c7 47 70 0f 00 00 00      movl $0xf,0x70(%rdi)     STATUS = ACK|DRIVER|          [MMIO WRITE]
 *                                                          DRIVER_OK|FEATURES_OK (0xf)
 *  14   be 00 60 00 00            mov  $0x6000,%esi        esi = avail ring GPA
 *  19   66 c7 46 02 01 00         movw $1,2(%rsi)          avail.idx = 1  (publish the buffer)
 *  25   c7 47 50 00 00 00 00      movl $0,0x50(%rdi)       QueueNotify = 0  (KICK)       [MMIO WRITE]
 *  32   ba 00 70 00 00            mov  $0x7000,%edx        edx = used ring GPA
 *  37   .poll:
 *  37   0f b7 4a 02               movzwl 2(%rdx),%ecx      ecx = used.idx  (poll for completion)
 *  41   83 f9 01                  cmp  $1,%ecx             used.idx == 1 ?
 *  44   75 f7                     jne  .poll               spin (target 37, from next-ip 46: -9)
 *  46   f4                        hlt                      -> KVM_EXIT_HLT (clean stop)
 *
 * What it exercises: an MMIO READ (MagicValue), two MMIO WRITEs (Status, the
 * QueueNotify kick), a plain-RAM store to publish into the available ring, and a
 * plain-RAM poll of the used ring — i.e. the entire virtio fast path, where only
 * the kick and the discovery reads are actual VM exits.
 */
static const uint8_t guest_code[] = {
    0xbf, 0x00, 0x00, 0x00, 0x10,               /* mov  $0x10000000, %edi          */
    0x8b, 0x07,                                 /* mov  (%rdi), %eax               */
    0xc7, 0x47, 0x70, 0x0f, 0x00, 0x00, 0x00,   /* movl $0xf, 0x70(%rdi)  (STATUS)  */
    0xbe, 0x00, 0x60, 0x00, 0x00,               /* mov  $0x6000, %esi              */
    0x66, 0xc7, 0x46, 0x02, 0x01, 0x00,         /* movw $1, 2(%rsi)   (avail.idx=1) */
    0xc7, 0x47, 0x50, 0x00, 0x00, 0x00, 0x00,   /* movl $0, 0x50(%rdi) (QueueNotify)*/
    0xba, 0x00, 0x70, 0x00, 0x00,               /* mov  $0x7000, %edx              */
    /* .poll: */
    0x0f, 0xb7, 0x4a, 0x02,                     /* movzwl 2(%rdx), %ecx            */
    0x83, 0xf9, 0x01,                           /* cmp  $1, %ecx                   */
    0x75, 0xf7,                                 /* jne  .poll                      */
    0xf4,                                       /* hlt                             */
};

/* The message the guest "sends" over the console. The device (virtio.c) prints
 * exactly this many bytes; no NUL terminator is needed on the wire. */
static const char console_msg[] =
    "Hello from the microVM guest -- this text traveled a real virtio split\n"
    "virtqueue (descriptor table + available ring + used ring) over an MMIO\n"
    "transport, and the device side printed it. That round-trip is the lesson.\n";

/* ===========================================================================
 * 2. LONG-MODE PAGE TABLES, GDT, AND CONTROL STATE
 * =========================================================================== */

/* Page-table entry flags (same meaning at every level). */
#define PDE64_PRESENT (1u << 0)   /* entry valid; absent => #PF                    */
#define PDE64_RW      (1u << 1)   /* writable                                      */
#define PDE64_PCD     (1u << 4)   /* cache-disable: correct for device (MMIO) memory*/
#define PDE64_PS      (1u << 7)   /* Page Size: this PD entry maps a 2 MiB page      */

/* Control-register / EFER bits that define 64-bit mode. */
#define CR0_PE (1u << 0)
#define CR0_MP (1u << 1)
#define CR0_ET (1u << 4)
#define CR0_NE (1u << 5)
#define CR0_WP (1u << 16)
#define CR0_AM (1u << 18)
#define CR0_PG (1u << 31)
#define CR4_PAE (1u << 5)
#define EFER_LME (1u << 8)
#define EFER_LMA (1u << 10)

/* The PD index whose 2 MiB page covers the virtio-mmio window. 0x10000000 /
 * 0x200000 = 128. Mapping it PRESENT (so translation succeeds) but leaving the
 * GPA unbacked (no memory slot) is what turns a guest access there into a clean
 * KVM_EXIT_MMIO instead of a page fault. */
#define MMIO_PD_INDEX (GPA_MMIO_BASE / 0x200000ull)   /* == 128                     */

/* Small pokes into guest RAM at a GPA (host writes while the vCPU is stopped). */
static void poke8 (struct vm *vm, uint64_t gpa, uint8_t  v){ memcpy(vm->mem+gpa,&v,1);}
static void poke16(struct vm *vm, uint64_t gpa, uint16_t v){ memcpy(vm->mem+gpa,&v,2);}
static void poke32(struct vm *vm, uint64_t gpa, uint32_t v){ memcpy(vm->mem+gpa,&v,4);}
static void poke64(struct vm *vm, uint64_t gpa, uint64_t v){ memcpy(vm->mem+gpa,&v,8);}

/* Build a flat segment descriptor cache for KVM_SET_SREGS (base 0, 4 GiB limit).
 * `code`=1 gives a 64-bit code segment (L=1); `code`=0 a flat data segment. KVM
 * uses these cached fields on VMENTRY, so the guest runs 64-bit even though it
 * never executes a far jump to reload CS. */
static struct kvm_segment flat_segment(uint16_t selector, int code)
{
    struct kvm_segment s;
    memset(&s, 0, sizeof(s));
    s.base = 0; s.limit = 0xffffffff; s.selector = selector;
    s.present = 1; s.dpl = 0; s.s = 1; s.g = 1;
    if (code) { s.type = 11; s.l = 1; s.db = 0; }   /* code, exec/read/accessed, L=1 */
    else      { s.type = 3;  s.l = 0; s.db = 1; }   /* data, read/write/accessed     */
    return s;
}

/* ===========================================================================
 * 3. THE boot_params ZERO PAGE
 * =========================================================================== */

/* Fill the minimal boot_params a Linux 64-bit entry would consume: an E820 map
 * (usable RAM + the reserved MMIO hole), the loadflags, a loader stamp, and a
 * command line. We write fields by their fixed page offsets (see boot.h). Our
 * demo payload never reads this — it exists to show the exact hand-off contract,
 * and it is where a real bzImage boot would diverge from this teaching core. */
static void fill_boot_params(struct vm *vm)
{
    /* zero the whole 4 KiB zero page first (fields default to 0). */
    memset(vm->mem + GPA_BOOT_PARAMS, 0, 4096);

    /* E820 memory map: two entries. */
    poke8 (vm, GPA_BOOT_PARAMS + BP_OFF_E820_ENTRIES, 2);
    /* entry 0: usable RAM [0, GUEST_RAM_SIZE). */
    uint64_t e0 = GPA_BOOT_PARAMS + BP_OFF_E820_TABLE;      /* 20 bytes             */
    poke64(vm, e0 + 0, 0);
    poke64(vm, e0 + 8, GUEST_RAM_SIZE);
    poke32(vm, e0 + 16, BP_E820_RAM);
    /* entry 1: the virtio-mmio window, reserved (not RAM the kernel may allocate).*/
    uint64_t e1 = e0 + 20;
    poke64(vm, e1 + 0, GPA_MMIO_BASE);
    poke64(vm, e1 + 8, 0x200000);                          /* one 2 MiB window      */
    poke32(vm, e1 + 16, BP_E820_RESERVED);

    /* loadflags: protected-mode kernel loaded high (0x100000); loader = custom. */
    poke8(vm, GPA_BOOT_PARAMS + BP_OFF_LOADFLAGS, BP_LOADFLAG_LOADED_HIGH);
    poke8(vm, GPA_BOOT_PARAMS + BP_OFF_TYPE_OF_LOADER, 0xff);

    /* command line: a NUL-terminated string placed in spare room of the zero page
     * (0x800, past the E820 table), with cmd_line_ptr pointing at its GPA. */
    static const char cmdline[] = "console=hvc0 reboot=k panic=1";
    uint64_t cmd_gpa = GPA_BOOT_PARAMS + 0x800;
    memcpy(vm->mem + cmd_gpa, cmdline, sizeof(cmdline));    /* includes the NUL      */
    poke32(vm, GPA_BOOT_PARAMS + BP_OFF_CMD_LINE_PTR, (uint32_t)cmd_gpa);
}

/* ===========================================================================
 * 4. THE CONSOLE VIRTQUEUE (laid out by the loader on the guest's behalf)
 * =========================================================================== */

/* Write the split-virtqueue memory the guest+device will share, and program the
 * console device's queue-0 registers to point at it. One descriptor is enough:
 * it points at the console message as a device-READABLE ("out") buffer. */
static void setup_console_queue(struct vm *vm)
{
    /* --- the shared memory in guest RAM --- */

    /* descriptor[0]: {addr=TX buffer, len=msglen, flags=0 (readable,no next), next=0} */
    uint32_t msglen = (uint32_t)strlen(console_msg);
    poke64(vm, GPA_VQ_DESC + 0,  GPA_TX_BUF);   /* addr  (guest-physical)           */
    poke32(vm, GPA_VQ_DESC + 8,  msglen);       /* len                              */
    poke16(vm, GPA_VQ_DESC + 12, 0);            /* flags: 0 => device reads it (TX)  */
    poke16(vm, GPA_VQ_DESC + 14, 0);            /* next: unused                     */

    /* available ring: flags=0, idx=0 (the GUEST bumps this to 1), ring[0]=head=0. */
    poke16(vm, GPA_VQ_AVAIL + 0, 0);            /* flags                            */
    poke16(vm, GPA_VQ_AVAIL + 2, 0);            /* idx (guest publishes by setting 1)*/
    poke16(vm, GPA_VQ_AVAIL + 4, 0);            /* ring[0] = descriptor head index 0 */

    /* used ring: flags=0, idx=0 (the DEVICE bumps this after it prints). */
    poke16(vm, GPA_VQ_USED + 0, 0);             /* flags                            */
    poke16(vm, GPA_VQ_USED + 2, 0);             /* idx                              */

    /* the console message itself, in the TX buffer the descriptor points at. */
    memcpy(vm->mem + GPA_TX_BUF, console_msg, msglen);

    /* --- program the console device's queue-0 transport state --- */
    /* (In a real VM the guest driver writes these via the MMIO QUEUE_* registers;
     * we set them directly for legibility. virtio.c would accept either path.) */
    struct virtio_dev *con = &vm->devs[0];      /* device 0 is the console          */
    con->queue_sel        = 0;
    con->vq[0].num        = VQ_SIZE;            /* negotiated queue size            */
    con->vq[0].ready      = 1;                  /* queue is live                    */
    con->vq[0].desc_gpa   = GPA_VQ_DESC;
    con->vq[0].avail_gpa  = GPA_VQ_AVAIL;
    con->vq[0].used_gpa   = GPA_VQ_USED;
    con->vq[0].last_avail = 0;                  /* device has processed nothing yet  */
}

/* ===========================================================================
 * PUTTING IT TOGETHER
 * =========================================================================== */

void guest_setup(struct vm *vm)
{
    /* 1. code payload at GPA 0. */
    memcpy(vm->mem + GPA_CODE, guest_code, sizeof(guest_code));

    /* 2a. 4-level identity page map. One PD entry maps the 2 MiB of RAM; a second
     * maps the 2 MiB MMIO window (PRESENT so translation succeeds, but the GPA is
     * unbacked so the access traps as MMIO). PML4 -> PDPT -> PD. */
    poke64(vm, GPA_PML4, PDE64_PRESENT | PDE64_RW | GPA_PDPT);
    poke64(vm, GPA_PDPT, PDE64_PRESENT | PDE64_RW | GPA_PD);
    poke64(vm, GPA_PD + 0,                     /* PD[0]   -> 2 MiB @ phys 0 (RAM)   */
           PDE64_PRESENT | PDE64_RW | PDE64_PS | 0x0);
    poke64(vm, GPA_PD + MMIO_PD_INDEX * 8,     /* PD[128] -> 2 MiB @ 0x10000000     */
           PDE64_PRESENT | PDE64_RW | PDE64_PS | PDE64_PCD | GPA_MMIO_BASE);

    /* 2b. a real GDT in guest RAM: null, 64-bit code (0x08), flat data (0x10).
     * Each entry is 8 bytes; these encode the same descriptors we cache below. */
    poke64(vm, GPA_GDT + 0,  0x0000000000000000ull);  /* [0] null (required)         */
    poke64(vm, GPA_GDT + 8,  0x00af9b000000ffffull);  /* [1] code: G,L, P,DPL0,exec  */
    poke64(vm, GPA_GDT + 16, 0x00cf93000000ffffull);  /* [2] data: G,DB,P,DPL0,r/w   */

    /* 2c. long-mode control state via sregs. */
    struct kvm_sregs sregs;
    if (ioctl(vm->vcpufd, KVM_GET_SREGS, &sregs) < 0) guest_die("KVM_GET_SREGS");

    sregs.gdt.base  = GPA_GDT;                 /* GDTR -> our in-RAM table           */
    sregs.gdt.limit = 3 * 8 - 1;               /* 3 descriptors, byte limit          */

    sregs.cr3  = GPA_PML4;                     /* top-level page table base          */
    sregs.cr4  = CR4_PAE;                      /* PAE is mandatory for long mode     */
    sregs.cr0  = CR0_PE | CR0_MP | CR0_ET | CR0_NE | CR0_WP | CR0_AM | CR0_PG;
    sregs.efer = EFER_LME | EFER_LMA;          /* long mode enabled + active         */

    sregs.cs = flat_segment(0x08, /*code=*/1); /* selector 0x08 = GDT index 1        */
    struct kvm_segment data = flat_segment(0x10, /*code=*/0);
    sregs.ds = sregs.es = sregs.fs = sregs.gs = sregs.ss = data;

    if (ioctl(vm->vcpufd, KVM_SET_SREGS, &sregs) < 0) guest_die("KVM_SET_SREGS");

    /* 3. the boot_params zero page. */
    fill_boot_params(vm);

    /* 4. the console virtqueue + device programming. */
    setup_console_queue(vm);

    /* 5. general registers: entry point, stack, and the boot hand-off in %rsi.
     * rflags bit 1 is reserved and MUST be 1 (a clear bit-1 is a classic
     * KVM_EXIT_FAIL_ENTRY). We set %rsi = boot_params to honor the exact 64-bit
     * entry contract a Linux kernel expects; NOTE our demo payload immediately
     * reuses %rsi as scratch (it does not parse boot_params), which is why the
     * printed output comes from the virtqueue, not the kernel that would normally
     * consume %rsi. */
    struct kvm_regs regs;
    memset(&regs, 0, sizeof(regs));
    regs.rip    = GPA_CODE;                     /* start of our 64-bit payload        */
    regs.rsi    = GPA_BOOT_PARAMS;              /* the boot protocol hand-off          */
    regs.rsp    = GPA_STACK_TOP;                /* stack top = end of RAM, grows down  */
    regs.rflags = 0x2;                          /* reserved bit 1 set                  */

    if (ioctl(vm->vcpufd, KVM_SET_REGS, &regs) < 0) guest_die("KVM_SET_REGS");
}
