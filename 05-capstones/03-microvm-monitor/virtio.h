/* ===========================================================================
 * virtio.h — the virtio-mmio transport and the split virtqueue, on the DEVICE
 *            side (what a VMM implements). VIRTIO spec 1.x, §2.7 and §4.2.
 * ===========================================================================
 *
 * VIRTIO IN ONE PARAGRAPH
 * -----------------------
 * A guest driver and a host device talk through shared guest RAM, not through
 * emulated hardware registers byte by byte (that would be one VM exit per byte —
 * agonizingly slow). Instead they share a **virtqueue**: three arrays the driver
 * writes and the device reads (and vice versa). The only actual traps are (a) the
 * driver "kicking" the device by writing one MMIO register when it has queued
 * work, and (b) the device "interrupting" the driver when it has completed some.
 * Everything else is plain memory. That is why virtio is fast, and why a microVM
 * can boot with nothing but virtio devices and no legacy hardware emulation.
 *
 * THE TRANSPORT vs THE QUEUE
 * --------------------------
 * Two layers, kept distinct here:
 *   - The TRANSPORT (virtio-mmio) is the small register block at a fixed GPA that
 *     the driver pokes to discover the device, negotiate features, tell the
 *     device where the virtqueue lives, and kick it. It is the ONLY part that
 *     causes VM exits. Registers are below (VIRTIO_MMIO_*).
 *   - The VIRTQUEUE is the data plane in guest RAM: three sub-structures (the
 *     descriptor table, the available ring, the used ring) whose layout is fixed
 *     by the spec. Structs are below (virtq_desc / virtq_avail / virtq_used).
 *
 * This header declares both plus a device model (struct virtio_dev) that our
 * KVM_EXIT_MMIO handler drives. virtio.c implements the register semantics, the
 * virtqueue walk, and the three device kinds (console real; blk/net stubbed).
 * =========================================================================== */

#ifndef VIRTIO_H
#define VIRTIO_H

#include <stdint.h>

struct vm;   /* forward decl; the device model needs gpa_to_host() from vmm.h  */

/* ===========================================================================
 * THE virtio-mmio TRANSPORT REGISTER BLOCK (offsets from the device's base GPA)
 * ===========================================================================
 * These offsets are the ABI (VIRTIO 1.x §4.2.2). "R" = driver reads, "W" = driver
 * writes. We implement the MODERN (version 2) layout: the driver hands us the
 * three virtqueue sub-array addresses SEPARATELY (Desc/Driver/Device Low+High)
 * rather than the legacy single page-frame-number. Version 2 is what current
 * Linux and Firecracker speak.
 * =========================================================================== */
#define VIRTIO_MMIO_MAGIC_VALUE        0x000  /* R: 0x74726976 = little-endian "virt" */
#define VIRTIO_MMIO_VERSION            0x004  /* R: 2 (modern). 1 would be legacy      */
#define VIRTIO_MMIO_DEVICE_ID          0x008  /* R: 1=net, 2=blk, 3=console, ...       */
#define VIRTIO_MMIO_VENDOR_ID          0x00c  /* R: any nonzero vendor stamp           */
#define VIRTIO_MMIO_DEVICE_FEATURES    0x010  /* R: 32 feature bits of the sel'd word  */
#define VIRTIO_MMIO_DEVICE_FEATURES_SEL 0x014 /* W: which 32-bit word of features (0/1)*/
#define VIRTIO_MMIO_DRIVER_FEATURES    0x020  /* W: bits the driver ACCEPTS            */
#define VIRTIO_MMIO_DRIVER_FEATURES_SEL 0x024 /* W: which word the driver is writing   */
#define VIRTIO_MMIO_QUEUE_SEL          0x030  /* W: select which virtqueue to configure*/
#define VIRTIO_MMIO_QUEUE_NUM_MAX      0x034  /* R: largest queue size the device allows*/
#define VIRTIO_MMIO_QUEUE_NUM          0x038  /* W: queue size the driver chose        */
#define VIRTIO_MMIO_QUEUE_READY        0x044  /* RW: 1 => this queue is live            */
#define VIRTIO_MMIO_QUEUE_NOTIFY       0x050  /* W: THE KICK. value = queue index       */
#define VIRTIO_MMIO_INTERRUPT_STATUS   0x060  /* R: why the device interrupted (bitmask)*/
#define VIRTIO_MMIO_INTERRUPT_ACK      0x064  /* W: driver clears the bits it handled   */
#define VIRTIO_MMIO_STATUS             0x070  /* RW: the device-status handshake byte   */
#define VIRTIO_MMIO_QUEUE_DESC_LOW     0x080  /* W: GPA of the descriptor table (lo 32) */
#define VIRTIO_MMIO_QUEUE_DESC_HIGH    0x084  /* W:                             (hi 32) */
#define VIRTIO_MMIO_QUEUE_DRIVER_LOW   0x090  /* W: GPA of the available ring   (lo 32) */
#define VIRTIO_MMIO_QUEUE_DRIVER_HIGH  0x094  /* W:                             (hi 32) */
#define VIRTIO_MMIO_QUEUE_DEVICE_LOW   0x0a0  /* W: GPA of the used ring        (lo 32) */
#define VIRTIO_MMIO_QUEUE_DEVICE_HIGH  0x0a4  /* W:                             (hi 32) */
#define VIRTIO_MMIO_CONFIG_GENERATION  0x0fc  /* R: bumps when device config changes    */
#define VIRTIO_MMIO_CONFIG             0x100  /* R/W: device-specific config space      */

#define VIRTIO_MMIO_MAGIC   0x74726976u  /* the four bytes 'v''i''r''t', LE           */
#define VIRTIO_MMIO_VERSION_MODERN 2u    /* we speak version 2                        */
#define VIRTIO_MMIO_STRIDE  0x200u       /* bytes between adjacent device windows     */

/* Device IDs (VIRTIO 1.x §5). We model these three. */
#define VIRTIO_ID_NET      1u
#define VIRTIO_ID_BLOCK    2u
#define VIRTIO_ID_CONSOLE  3u

/* Device-status bits (the STATUS register handshake, §2.1). The driver walks
 * these in order during init; DRIVER_OK means "I'm ready to drive it." A device
 * that sees FAILED knows the driver gave up. */
#define VIRTIO_STATUS_ACKNOWLEDGE  1u   /* driver saw the device                     */
#define VIRTIO_STATUS_DRIVER       2u   /* driver knows how to drive it              */
#define VIRTIO_STATUS_DRIVER_OK    4u   /* driver is up; device may run              */
#define VIRTIO_STATUS_FEATURES_OK  8u   /* feature negotiation is complete           */
#define VIRTIO_STATUS_FAILED       0x80u/* driver has given up                       */

/* InterruptStatus bits (§4.2.2). Bit 0 = a used-ring buffer was consumed. */
#define VIRTIO_MMIO_INT_VRING   0x1u
#define VIRTIO_MMIO_INT_CONFIG  0x2u

/* The one feature bit every modern device must offer and every modern driver
 * must accept: VIRTIO_F_VERSION_1 (bit 32) — "this is a 1.0+ device, use the
 * modern layout." It lives in the high feature word (bit 0 of word 1). */
#define VIRTIO_F_VERSION_1_WORD  1u
#define VIRTIO_F_VERSION_1_BIT   0x1u   /* bit 32 == bit 0 of features word 1        */

/* ===========================================================================
 * THE SPLIT VIRTQUEUE (the data plane in guest RAM). VIRTIO 1.x §2.7.
 * ===========================================================================
 * Three arrays, each at a GPA the driver told us via the transport registers:
 *
 *   descriptor table  virtq_desc[queue_size]     — the buffers (addr,len,flags)
 *   available ring    virtq_avail                — driver -> device: "process these"
 *   used ring         virtq_used                 — device -> driver: "done with these"
 *
 * The rings carry FREE-RUNNING 16-bit indices (avail.idx, used.idx) that count
 * every buffer ever offered/consumed and wrap at 65536. You turn an index into a
 * slot with `idx & (queue_size - 1)` (queue_size is a power of two). All fields
 * are little-endian, which on our x86-64 host means "just read them." The exact
 * index/wrap arithmetic is extracted into asm/demo.c for the assembly study.
 * =========================================================================== */

/* One descriptor: a pointer to a single guest-RAM buffer, plus chaining. The
 * driver builds a CHAIN of these to describe a scatter/gather request (e.g. a
 * virtio-blk request header + data + status byte are three chained descriptors).*/
struct virtq_desc {
    uint64_t addr;    /* GPA of the buffer (guest-controlled — must be validated!)  */
    uint32_t len;     /* buffer length in bytes                                     */
    uint16_t flags;   /* VIRTQ_DESC_F_* below                                       */
    uint16_t next;    /* index of the next descriptor, iff F_NEXT is set            */
} __attribute__((packed));   /* packed: the spec fixes this 16-byte on-wire layout  */

#define VIRTQ_DESC_F_NEXT   1u   /* buffer continues in desc[this.next]              */
#define VIRTQ_DESC_F_WRITE  2u   /* device WRITES this buffer (else device reads it) */
#define VIRTQ_DESC_F_INDIRECT 4u /* buffer is itself a table of descriptors          */

/* The available ring: the driver appends the HEAD descriptor index of each new
 * request to ring[], then bumps idx. `flags` bit 0 (NO_INTERRUPT) is a hint. The
 * array is queue_size entries; we read only up to the negotiated size. Declared
 * with a 1-length flexible-ish tail; real code indexes it by hand against the
 * driver-supplied base GPA, which is what virtio.c does. */
struct virtq_avail {
    uint16_t flags;        /* bit0 = VIRTQ_AVAIL_F_NO_INTERRUPT (a hint)            */
    uint16_t idx;          /* FREE-RUNNING count of buffers made available          */
    uint16_t ring[];       /* ring[i % queue_size] = head descriptor index          */
} __attribute__((packed));

/* One completed request the device hands back: which chain (by head index) and
 * how many bytes it wrote into that chain's writable buffers. */
struct virtq_used_elem {
    uint32_t id;           /* head descriptor index of the completed chain          */
    uint32_t len;          /* total bytes the device wrote into the chain           */
} __attribute__((packed));

/* The used ring: the device appends a used_elem per completed chain, then bumps
 * idx. The driver reads idx to learn what finished. */
struct virtq_used {
    uint16_t flags;                 /* bit0 = VIRTQ_USED_F_NO_NOTIFY (a hint)       */
    uint16_t idx;                   /* FREE-RUNNING count of buffers consumed        */
    struct virtq_used_elem ring[];  /* ring[i % queue_size] = a completion           */
} __attribute__((packed));

/* ===========================================================================
 * THE DEVICE MODEL (what the monitor holds per device)
 * ===========================================================================
 * One struct virtio_dev per device on the bus. It holds the transport-register
 * state the driver programs, plus the per-queue addresses and the device's own
 * cursor into the available ring (`last_avail`). The `kind` selects behavior when
 * the queue is kicked: a console prints the buffer; blk/net account and complete.
 * =========================================================================== */
enum virtio_kind {
    VDEV_CONSOLE,   /* real: prints driver-readable buffers to stdout             */
    VDEV_BLOCK,     /* stub: answers probe/negotiation, completes with zeros       */
    VDEV_NET,       /* stub: answers probe/negotiation, drops "transmitted" frames */
};

/* Per-virtqueue state the driver programs through the transport registers. */
struct virtq_state {
    uint32_t num;         /* negotiated queue size (<= VQ_SIZE)                    */
    uint32_t ready;       /* QUEUE_READY: 1 once the driver says the queue is live */
    uint64_t desc_gpa;    /* GPA of the descriptor table (QUEUE_DESC_LOW/HIGH)     */
    uint64_t avail_gpa;   /* GPA of the available ring   (QUEUE_DRIVER_LOW/HIGH)   */
    uint64_t used_gpa;    /* GPA of the used ring        (QUEUE_DEVICE_LOW/HIGH)   */
    uint16_t last_avail;  /* device cursor: avail.idx we have processed up to      */
};

#define VIRTIO_MAX_QUEUES 2   /* console uses 1 here; 2 leaves room for rx+tx      */

struct virtio_dev {
    const char       *name;         /* "virtio-console" etc, for logging          */
    enum virtio_kind  kind;
    uint32_t          device_id;    /* VIRTIO_ID_*                                */
    uint64_t          mmio_base;    /* GPA of this device's VIRTIO_MMIO_STRIDE win */

    /* transport / negotiation state (written via the MMIO registers) */
    uint32_t status;                /* the STATUS handshake bits set so far       */
    uint32_t device_features_sel;   /* which feature word DEVICE_FEATURES reads    */
    uint32_t driver_features_sel;   /* which feature word the driver is writing    */
    uint64_t driver_features;       /* the 64 bits the driver accepted            */
    uint32_t queue_sel;             /* which queue the NUM/READY/GPA regs target   */
    uint32_t interrupt_status;      /* INTERRUPT_STATUS bits pending               */
    uint32_t config_generation;

    struct virtq_state vq[VIRTIO_MAX_QUEUES];

    /* stats, purely so the run can print what happened */
    unsigned long long notifications;   /* number of kicks received               */
    unsigned long long bytes_out;       /* bytes printed (console) / "sent"        */
    unsigned long long buffers_used;    /* completions published to the used ring  */
};

/* ---------------------------------------------------------------------------
 * The virtio-mmio bus interface, driven by vmm.c's handle_mmio().
 * ------------------------------------------------------------------------- */

/* Find the device whose 0x200 window contains `gpa`, or NULL if the fault is in
 * the bus window but hits no device. */
struct virtio_dev *virtio_bus_find(struct vm *vm, uint64_t gpa);

/* Service one MMIO READ of `len` bytes at `gpa` on `dev`; the value goes into
 * *out (little-endian, zero-extended). */
void virtio_mmio_read(struct vm *vm, struct virtio_dev *dev,
                      uint64_t gpa, uint32_t len, uint64_t *out);

/* Service one MMIO WRITE of `len` bytes (`val`) at `gpa` on `dev`. A write to
 * QUEUE_NOTIFY is the kick that runs the virtqueue. */
void virtio_mmio_write(struct vm *vm, struct virtio_dev *dev,
                       uint64_t gpa, uint32_t len, uint64_t val);

/* Initialize a device model (zeroes state, sets name/kind/id/base and the
 * per-queue max). Called once per device before the run. */
void virtio_dev_init(struct virtio_dev *dev, enum virtio_kind kind, uint64_t mmio_base);

#endif /* VIRTIO_H */
