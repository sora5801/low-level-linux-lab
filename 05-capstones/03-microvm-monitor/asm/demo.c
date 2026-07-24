/* ===========================================================================
 * asm/demo.c — the microVM monitor's PURE-LOGIC core, standalone for asm study.
 * ===========================================================================
 *
 * WHY THIS FILE EXISTS
 * --------------------
 * vmm.c / virtio.c / guest.c are real userspace C, but they #include
 * <linux/kvm.h>, <sys/ioctl.h>, <sys/mman.h> — Linux-only headers absent on the
 * Windows host this repo may live on, so clang cannot turn those files into
 * assembly here. The *decision-making heart* of the monitor, though, is pure
 * integer arithmetic with no kernel dependency, and there are two hot, subtle
 * pieces the whole design rests on:
 *
 *   1. THE VIRTQUEUE INDEX / WRAP MATH. A virtio split virtqueue is three arrays
 *      in guest RAM — a descriptor table, an "available" ring the driver fills,
 *      and a "used" ring the device fills. The ring indices (`avail.idx`,
 *      `used.idx`) are *free-running 16-bit counters that never reset*: they
 *      count total buffers ever offered/consumed and wrap modulo 65536. You turn
 *      one into a slot with `idx & (qsize - 1)` (queue size is always a power of
 *      two), and you count unconsumed buffers with a 16-bit wrapping subtraction
 *      `(u16)(avail.idx - last_seen)`. Getting this masking and wrap arithmetic
 *      right IS the virtqueue; getting it wrong desyncs driver and device
 *      silently. So it is exactly the code to read in assembly.
 *
 *   2. THE KVM EXIT-REASON DISPATCH. On every VM exit KVM hands us a reason code
 *      and we must decide what the run loop does. That runs on every single exit.
 *
 * These functions are byte-for-byte the classifications vmm.c and virtio.c
 * perform; only the ioctl()/memory plumbing around them is gone. Read the
 * generated asm to SEE the compiler turn `& (qsize-1)` into a single `and`, a
 * 16-bit subtract into `subl`+`movzwl`, a divide/modulo by a power-of-two stride
 * into `shr`/`and`, and a `switch` (built with -fno-jump-tables) into a compare
 * chain or a bitmask `bt`.
 *
 * The constants below are the REAL values from the VIRTIO 1.x spec and
 * <linux/kvm.h>; we spell them out so this file needs no headers. They are
 * ABI-stable (KVM_EXIT_IO has been 2 for the life of the interface;
 * VIRTQ_DESC_F_NEXT has been 1 since virtio 0.9).
 * =========================================================================== */

/* Our own fixed-width types, so this file needs no <stdint.h>. On the x86-64
 * SysV target these are exactly the kernel's __u16 / __u32 / __u64. */
typedef unsigned short u16;   /* a virtqueue ring index: wraps modulo 65536     */
typedef unsigned int   u32;   /* a KVM exit reason / a 32-bit MMIO register      */
typedef unsigned long long u64; /* a guest-physical address (GPA)                */

/* ---------------------------------------------------------------------------
 * VIRTIO split-virtqueue descriptor flags (struct virtq_desc.flags), verbatim
 * from the virtio 1.x spec §2.7.5. A descriptor points at one guest-RAM buffer;
 * these two bits say how the chain and the direction work.
 * ------------------------------------------------------------------------- */
#define VIRTQ_DESC_F_NEXT   1u   /* this descriptor CHAINS to desc[.next]        */
#define VIRTQ_DESC_F_WRITE  2u   /* buffer is device-WRITABLE (device -> driver) */
                                 /* absent => device-READABLE (driver -> device) */

/* virtio-mmio device windows are laid out at a fixed stride so the fault address
 * alone tells us which device AND which register was touched. QEMU/Linux use
 * 0x200-byte windows; we do too. A power of two on purpose — see the asm. */
#define MMIO_STRIDE 0x200u

/* ---------------------------------------------------------------------------
 * KVM_EXIT_* reason codes (from <linux/kvm.h>), verbatim. The kernel writes one
 * of these into run->exit_reason before KVM_RUN returns.
 * ------------------------------------------------------------------------- */
#define KVM_EXIT_UNKNOWN         0u
#define KVM_EXIT_EXCEPTION       1u
#define KVM_EXIT_IO              2u   /* guest did IN/OUT on a port (debug console)*/
#define KVM_EXIT_HYPERCALL       3u
#define KVM_EXIT_DEBUG           4u
#define KVM_EXIT_HLT             5u   /* guest executed `hlt`                     */
#define KVM_EXIT_MMIO            6u   /* access to a GPA with no memory slot ...  */
                                     /* ...this is how a virtio-mmio kick reaches us*/
#define KVM_EXIT_IRQ_WINDOW_OPEN 7u
#define KVM_EXIT_SHUTDOWN        8u   /* triple fault / reset                     */
#define KVM_EXIT_FAIL_ENTRY      9u   /* VMENTRY refused our guest state          */
#define KVM_EXIT_INTR           10u   /* host signal interrupted KVM_RUN          */
#define KVM_EXIT_INTERNAL_ERROR 17u

/* ===========================================================================
 * PART 1 — THE VIRTQUEUE INDEX / WRAP MATH
 * ===========================================================================
 * These are the load-bearing one-liners of the split virtqueue. Each looks
 * trivial in C; the point is to read what the compiler does with the wrap and
 * the power-of-two, because a real VMM lives or dies on getting them exact.
 * =========================================================================== */

/* -------------------------------------------------------------------------
 * vq_ring_slot — map a free-running 16-bit ring index to an array slot.
 *
 * `avail.idx` and `used.idx` count every buffer ever published; they are NOT
 * array indices. The actual slot in the (power-of-two-sized) ring is the low
 * log2(qsize) bits: idx mod qsize == idx & (qsize - 1). Because qsize is always
 * a power of two (a virtio requirement), the compiler needs no divide — this
 * becomes a single `and`. That equivalence is the reason virtqueue sizes must be
 * powers of two.
 * ABI: idx in %di, qsize in %si; slot returned in %ax.
 * ------------------------------------------------------------------------- */
u16 vq_ring_slot(u16 idx, u16 qsize)
{
    return (u16)(idx & (u16)(qsize - 1));   /* mod-power-of-two via bitmask      */
}

/* -------------------------------------------------------------------------
 * vq_pending — how many buffers has the driver made available but the device
 * has not yet consumed?  It is a 16-bit WRAPPING subtraction.
 *
 * The device remembers `last_seen` (the avail.idx value it last processed up
 * to). The driver keeps bumping avail.idx. The count of new work is
 * avail_idx - last_seen computed in 16 bits: when avail.idx has wrapped past
 * 0xffff back to 0x0003 while last_seen is 0xfffe, the true gap is 5, and
 * (u16)(0x0003 - 0xfffe) = 5 falls out for free precisely because we truncate to
 * 16 bits. Read the asm: a 32-bit `subl` followed by a `movzwl` (zero-extend the
 * low 16 bits) — the truncation IS the modular arithmetic.
 * ABI: avail_idx %di, last_seen %si; count %ax.
 * ------------------------------------------------------------------------- */
u16 vq_pending(u16 avail_idx, u16 last_seen)
{
    return (u16)(avail_idx - last_seen);    /* modular gap; truncation = mod 2^16 */
}

/* -------------------------------------------------------------------------
 * vq_next — advance a ring index by one, wrapping at 65536.
 *
 * After the device consumes one buffer it advances its cursor: last_seen + 1,
 * truncated to 16 bits so 0xffff -> 0x0000. Same lesson as above in miniature:
 * the `+1` is done wide and the store to a u16 does the wrap.
 * ------------------------------------------------------------------------- */
u16 vq_next(u16 idx)
{
    return (u16)(idx + 1);                  /* 0xffff + 1 -> 0x0000 (wrap)        */
}

/* -------------------------------------------------------------------------
 * desc_is_writable / desc_has_next — decode one descriptor's flags.
 *
 * When the device walks a descriptor chain it must know, per descriptor,
 * (a) whether the buffer is device-writable (an "in" buffer it fills, e.g. a
 * virtio-blk read landing zone) or device-readable (an "out" buffer it consumes,
 * e.g. the bytes a virtio-console prints), and (b) whether the chain continues.
 * These fold a mask + nonzero test into a branchless `and`/`setne`.
 * ------------------------------------------------------------------------- */
int desc_is_writable(u16 flags)
{
    return (flags & VIRTQ_DESC_F_WRITE) != 0;   /* device fills it? (IN buffer)  */
}
int desc_has_next(u16 flags)
{
    return (flags & VIRTQ_DESC_F_NEXT) != 0;    /* follow .next to keep walking? */
}

/* ===========================================================================
 * PART 2 — MMIO ADDRESS DECODE (route a fault to a device + register)
 * ===========================================================================
 * A KVM_EXIT_MMIO gives us the faulting guest-physical address. Because devices
 * sit at a fixed power-of-two stride from a common base, that one address tells
 * us BOTH which device (which stride window) and which register within it. With
 * a constant power-of-two stride the divide becomes a shift and the modulo an
 * `and`: the two functions below compile to `sub` + `shr`/`and`.
 * =========================================================================== */

/* Which device window does `gpa` fall in?  (gpa - base) / stride.
 * ABI: gpa %rdi, base %rsi; index %eax. */
u32 mmio_device_index(u64 gpa, u64 base)
{
    return (u32)((gpa - base) / MMIO_STRIDE);   /* /0x200 -> shr $9              */
}

/* Which register within the device?  (gpa - base) % stride.
 * ABI: gpa %rdi, base %rsi; offset %eax. */
u32 mmio_reg_offset(u64 gpa, u64 base)
{
    return (u32)((gpa - base) % MMIO_STRIDE);   /* %0x200 -> and $0x1ff          */
}

/* ===========================================================================
 * PART 3 — THE KVM EXIT-REASON DISPATCH
 * ===========================================================================
 * The other hot routine: on every VM exit, map the reason code to what the run
 * loop should do. Built with -fno-jump-tables so the `switch` is a transparent
 * compare chain (or, for grouped cases, a bitmask `bt` — read the asm to see
 * which the optimizer chose).
 * =========================================================================== */

/* What the run loop should do about an exit — this monitor's whole policy. */
enum vmm_action {
    ACT_UNHANDLED  = 0,   /* reason we do not model; report it                   */
    ACT_MMIO       = 1,   /* KVM_EXIT_MMIO -> route to the virtio-mmio bus       */
    ACT_PIO        = 2,   /* KVM_EXIT_IO   -> the debug/exit port                */
    ACT_STOP_OK    = 3,   /* clean end (guest executed hlt)                      */
    ACT_STOP_ERR   = 4,   /* fatal (shutdown / failed entry / internal error)    */
    ACT_REENTER    = 5,   /* transient (host signal / irq window); KVM_RUN again */
};

/* -------------------------------------------------------------------------
 * kvm_exit_action — THE dispatch. Note MMIO is action #1: for a microVM whose
 * devices are all virtio-mmio, the memory-mapped exit is the common, hot case
 * (every virtqueue kick is an MMIO write), so it is listed first.
 * ABI: reason %edi; action (an int) %eax.
 * ------------------------------------------------------------------------- */
enum vmm_action kvm_exit_action(u32 reason)
{
    switch (reason) {
    case KVM_EXIT_MMIO:
        return ACT_MMIO;                    /* the virtio kick path — hot        */
    case KVM_EXIT_IO:
        return ACT_PIO;                     /* the debug/exit port               */
    case KVM_EXIT_HLT:
        return ACT_STOP_OK;                 /* guest said "done"                 */
    case KVM_EXIT_SHUTDOWN:
    case KVM_EXIT_FAIL_ENTRY:
    case KVM_EXIT_INTERNAL_ERROR:
        return ACT_STOP_ERR;                /* three roads to the same failure   */
    case KVM_EXIT_INTR:
    case KVM_EXIT_IRQ_WINDOW_OPEN:
        return ACT_REENTER;                 /* transient; loop around again      */
    default:
        return ACT_UNHANDLED;               /* everything else: surface it       */
    }
}

/* -------------------------------------------------------------------------
 * demo_selftest — prove the math is right without any kernel. Returns 0 on
 * success, or a small nonzero code naming the first failing check (inspect
 * `echo $?`). The optimizer folds every check over constant inputs, so at higher
 * -O this collapses toward `xor %eax,%eax`; that erasure is a lesson in itself.
 * ------------------------------------------------------------------------- */
int demo_selftest(void)
{
    /* ring-slot masking: with qsize 8, index 11 lands in slot 3 (11 & 7). */
    if (vq_ring_slot(11u, 8u) != 3u) return 1;
    if (vq_ring_slot(8u,  8u) != 0u) return 2;   /* the wrap point of an 8-ring   */

    /* pending count with a 16-bit wrap: driver at 0x0003, device saw 0xfffe. */
    if (vq_pending(0x0003u, 0xfffeu) != 5u) return 3;
    if (vq_pending(1u, 0u) != 1u) return 4;      /* the simple, no-wrap case      */

    /* index increment wraps 0xffff -> 0. */
    if (vq_next(0xffffu) != 0u) return 5;

    /* descriptor flag decode. */
    if (!desc_has_next(VIRTQ_DESC_F_NEXT)) return 6;
    if ( desc_is_writable(VIRTQ_DESC_F_NEXT)) return 7;   /* NEXT is not WRITE     */
    if (!desc_is_writable(VIRTQ_DESC_F_WRITE)) return 8;

    /* MMIO decode: device 2 (net window), register 0x50 (QueueNotify). */
    if (mmio_device_index(0x10000450ull, 0x10000000ull) != 2u) return 9;
    if (mmio_reg_offset  (0x10000450ull, 0x10000000ull) != 0x50u) return 10;

    /* exit dispatch: the hot MMIO case and a fatal case. */
    if (kvm_exit_action(KVM_EXIT_MMIO)     != ACT_MMIO)     return 11;
    if (kvm_exit_action(KVM_EXIT_HLT)      != ACT_STOP_OK)  return 12;
    if (kvm_exit_action(KVM_EXIT_SHUTDOWN) != ACT_STOP_ERR) return 13;
    if (kvm_exit_action(KVM_EXIT_INTR)     != ACT_REENTER)  return 14;

    return 0;
}
