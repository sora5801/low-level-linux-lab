/* ===========================================================================
 * virtio.c — the virtio-mmio transport register machine and the split-virtqueue
 *            walk. This is the DEVICE side of virtio, driven by KVM_EXIT_MMIO.
 * ===========================================================================
 *
 * Read this file as two halves:
 *
 *   1. THE TRANSPORT (virtio_mmio_read / virtio_mmio_write). A small state
 *      machine over the register block. The driver reads MagicValue/Version/
 *      DeviceID to discover us, walks the Status handshake, tells us where the
 *      virtqueue lives, and finally writes QueueNotify — the "kick." Only these
 *      register touches cause VM exits.
 *
 *   2. THE DATA PLANE (process_queue). On a kick we walk the split virtqueue in
 *      guest RAM: read the available ring, follow each descriptor chain to its
 *      buffers, act on them (a console PRINTS driver-readable buffers), then
 *      publish a used-ring element and bump used.idx. This is the real thing —
 *      the exact ring/index/wrap arithmetic the assembly study in asm/demo.c
 *      dissects.
 *
 * THE SECURITY BOUNDARY. Every address inside a descriptor is chosen by the
 * GUEST. A VMM that dereferences a guest-supplied GPA without bounds-checking it
 * against guest RAM is how VMs escape to the host. So every buffer here is routed
 * through gpa_to_host(), which returns NULL for anything outside [0, mem_size);
 * we refuse to touch a buffer we cannot validate.
 * =========================================================================== */

#include "vmm.h"        /* struct vm, gpa_to_host, VQ_SIZE, SERIAL_PORT         */

#include <stdio.h>      /* fwrite, fprintf, fflush                             */
#include <string.h>     /* memcpy, memset                                      */

/* ---------------------------------------------------------------------------
 * Little-endian loads/stores from guest RAM, bounds-checked, via memcpy.
 *
 * virtqueue structures are packed on-wire layouts; we use memcpy (not a struct
 * dereference) so unaligned access is always defined and the byte-for-byte spec
 * layout is explicit. The host is x86-64 (little-endian) and so is the virtio
 * wire format, hence no byte-swapping. Each returns 0 / does nothing and sets
 * *ok=0 if the GPA range is not backed by guest RAM — the caller MUST honor *ok.
 * ------------------------------------------------------------------------- */
static uint16_t g_ld16(struct vm *vm, uint64_t gpa, int *ok)
{
    void *p = gpa_to_host(vm, gpa, 2);
    if (!p) { *ok = 0; return 0; }
    uint16_t v; memcpy(&v, p, 2); return v;
}
static uint32_t g_ld32(struct vm *vm, uint64_t gpa, int *ok)
{
    void *p = gpa_to_host(vm, gpa, 4);
    if (!p) { *ok = 0; return 0; }
    uint32_t v; memcpy(&v, p, 4); return v;
}
static uint64_t g_ld64(struct vm *vm, uint64_t gpa, int *ok)
{
    void *p = gpa_to_host(vm, gpa, 8);
    if (!p) { *ok = 0; return 0; }
    uint64_t v; memcpy(&v, p, 8); return v;
}
static void g_st16(struct vm *vm, uint64_t gpa, uint16_t v, int *ok)
{
    void *p = gpa_to_host(vm, gpa, 2);
    if (!p) { *ok = 0; return; }
    memcpy(p, &v, 2);
}
static void g_st32(struct vm *vm, uint64_t gpa, uint32_t v, int *ok)
{
    void *p = gpa_to_host(vm, gpa, 4);
    if (!p) { *ok = 0; return; }
    memcpy(p, &v, 4);
}

/* ---------------------------------------------------------------------------
 * virtio_dev_init — zero a device model and stamp its identity.
 * ------------------------------------------------------------------------- */
void virtio_dev_init(struct virtio_dev *dev, enum virtio_kind kind, uint64_t mmio_base)
{
    memset(dev, 0, sizeof(*dev));
    dev->kind      = kind;
    dev->mmio_base = mmio_base;
    switch (kind) {
    case VDEV_CONSOLE: dev->name = "virtio-console"; dev->device_id = VIRTIO_ID_CONSOLE; break;
    case VDEV_BLOCK:   dev->name = "virtio-blk";     dev->device_id = VIRTIO_ID_BLOCK;   break;
    case VDEV_NET:     dev->name = "virtio-net";     dev->device_id = VIRTIO_ID_NET;     break;
    }
    /* Advertise the maximum queue size we support on every queue up front; the
     * driver reads QUEUE_NUM_MAX and picks a size <= this. */
    for (int i = 0; i < VIRTIO_MAX_QUEUES; i++)
        dev->vq[i].num = VQ_SIZE;   /* provisional until the driver sets QUEUE_NUM */
}

/* ---------------------------------------------------------------------------
 * virtio_bus_find — which device window contains `gpa`?
 *
 * Devices sit at mmio_base + i*VIRTIO_MMIO_STRIDE. We just scan our small device
 * array and check whether gpa falls in [base, base+STRIDE). (The index/offset
 * math this mirrors — (gpa-base)/stride and %stride — is the mmio_* pair in
 * asm/demo.c, shown compiling to a shift and an AND.)
 * ------------------------------------------------------------------------- */
struct virtio_dev *virtio_bus_find(struct vm *vm, uint64_t gpa)
{
    for (size_t i = 0; i < vm->ndev; i++) {
        struct virtio_dev *d = &vm->devs[i];
        if (gpa >= d->mmio_base && gpa < d->mmio_base + VIRTIO_MMIO_STRIDE)
            return d;
    }
    return NULL;
}

/* ===========================================================================
 * THE DATA PLANE: walk one virtqueue on a kick.
 * ===========================================================================
 * Precondition: the driver has set the queue's desc/avail/used GPAs, its size,
 * and QUEUE_READY. We process every buffer the driver made available since we
 * last looked (from last_avail up to the current avail.idx), completing each.
 * ------------------------------------------------------------------------- */

/* device_consume_readable — what a device DOES with a driver-readable buffer.
 * For a console that is "print it." For net-tx it would be "transmit it"; our
 * net stub drops it. For blk it would be the request header/data. */
static void device_consume_readable(struct virtio_dev *dev,
                                    const uint8_t *buf, uint32_t len)
{
    switch (dev->kind) {
    case VDEV_CONSOLE:
        /* THE console: the guest's bytes go straight to our stdout. This is the
         * payoff of the whole round-trip — a real paravirtual console. */
        fwrite(buf, 1, len, stdout);
        fflush(stdout);
        dev->bytes_out += len;
        break;
    case VDEV_NET:
        /* Stub: a real virtio-net would hand these frame bytes to a tap device.
         * We only account for them so the run can report "N bytes 'sent'." */
        dev->bytes_out += len;
        break;
    case VDEV_BLOCK:
        /* Stub: a real virtio-blk's first readable descriptor is the 16-byte
         * request header (type/sector). We ignore its content and just complete. */
        dev->bytes_out += len;
        break;
    }
}

/* device_fill_writable — what a device DOES with a device-writable buffer: it
 * fills it and reports how many bytes it wrote (that count goes in used.len).
 * Console/net have no writable data here; the blk stub zero-fills (a read of an
 * all-zero disk) to model completing an IN buffer. Returns bytes written. */
static uint32_t device_fill_writable(struct virtio_dev *dev,
                                     uint8_t *buf, uint32_t len)
{
    switch (dev->kind) {
    case VDEV_BLOCK:
        memset(buf, 0, len);    /* "read" returns zeros from our empty stub disk  */
        return len;
    case VDEV_CONSOLE:
    case VDEV_NET:
    default:
        return 0;               /* nothing to write back on these paths           */
    }
}

/* process_queue — the split-virtqueue engine. This is the function to read. */
static void process_queue(struct vm *vm, struct virtio_dev *dev, uint32_t qidx)
{
    if (qidx >= VIRTIO_MAX_QUEUES) {
        fprintf(stderr, "[%s] notify on bad queue %u\n", dev->name, qidx);
        return;
    }
    struct virtq_state *q = &dev->vq[qidx];

    if (!q->ready) {
        fprintf(stderr, "[%s] notify on queue %u before QUEUE_READY\n", dev->name, qidx);
        return;
    }
    /* queue size must be a nonzero power of two (a virtio requirement) — it is
     * what makes `idx & (num-1)` a valid modulo. Refuse anything else rather than
     * compute a wrong slot. */
    if (q->num == 0 || (q->num & (q->num - 1)) != 0) {
        fprintf(stderr, "[%s] queue %u has non-power-of-two size %u\n",
                dev->name, qidx, q->num);
        return;
    }
    const uint16_t mask = (uint16_t)(q->num - 1);   /* the ring-slot bitmask       */

    int ok = 1;

    /* Read the available ring's idx (a free-running 16-bit counter). The driver
     * wrote the new ring[] entry BEFORE bumping idx; an ACQUIRE fence here pairs
     * with the driver's release so that once we have seen the new idx we are
     * guaranteed to see the ring[] slot it published. Without it, on a weakly
     * ordered CPU we could read a stale ring slot for a fresh idx. */
    uint16_t avail_idx = g_ld16(vm, q->avail_gpa + 2 /*offset of .idx*/, &ok);
    __atomic_thread_fence(__ATOMIC_ACQUIRE);
    if (!ok) { fprintf(stderr, "[%s] avail ring GPA unbacked\n", dev->name); return; }

    dev->notifications++;

    /* Consume every newly-available buffer. `(uint16_t)(avail_idx - last_avail)`
     * is the wrapping count of pending buffers (asm/demo.c: vq_pending). */
    while (q->last_avail != avail_idx) {
        uint16_t slot = (uint16_t)(q->last_avail & mask);      /* vq_ring_slot     */

        /* avail.ring[] starts at offset 4 (after flags+idx); each entry is 2 bytes.*/
        uint16_t head = g_ld16(vm, q->avail_gpa + 4 + (uint64_t)slot * 2, &ok);
        if (!ok) { fprintf(stderr, "[%s] avail ring slot unbacked\n", dev->name); return; }

        /* --- Walk the descriptor chain starting at `head`. ------------------ */
        uint32_t used_len = 0;      /* total bytes written to WRITABLE buffers     */
        uint16_t di = head;
        uint32_t steps = 0;         /* loop guard: a chain can't exceed queue size */
        for (;;) {
            if (di >= q->num) {     /* a descriptor index must be in-range          */
                fprintf(stderr, "[%s] descriptor index %u >= num %u\n",
                        dev->name, di, q->num);
                break;
            }
            /* Each descriptor is 16 bytes at desc_gpa + di*16. */
            uint64_t dgpa = q->desc_gpa + (uint64_t)di * 16;
            uint64_t addr  = g_ld64(vm, dgpa + 0, &ok);
            uint32_t len   = g_ld32(vm, dgpa + 8, &ok);
            uint16_t flags = g_ld16(vm, dgpa + 12, &ok);
            uint16_t next  = g_ld16(vm, dgpa + 14, &ok);
            if (!ok) { fprintf(stderr, "[%s] descriptor unbacked\n", dev->name); break; }

            /* Validate the guest-controlled buffer address+length BEFORE use. */
            uint8_t *buf = gpa_to_host(vm, addr, len);
            if (!buf && len != 0) {
                fprintf(stderr, "[%s] descriptor buffer gpa=0x%llx len=%u out of range\n",
                        dev->name, (unsigned long long)addr, len);
                break;
            }
            if (flags & VIRTQ_DESC_F_WRITE)
                used_len += device_fill_writable(dev, buf, len);   /* device -> driver */
            else
                device_consume_readable(dev, buf, len);            /* driver -> device */

            if (!(flags & VIRTQ_DESC_F_NEXT))
                break;                          /* end of the scatter/gather chain  */
            di = next;
            if (++steps >= q->num) {            /* cycle guard: malformed chain     */
                fprintf(stderr, "[%s] descriptor chain too long (loop?)\n", dev->name);
                break;
            }
        }

        /* --- Publish the completion into the used ring. -------------------- */
        uint16_t used_idx = g_ld16(vm, q->used_gpa + 2 /*offset of .idx*/, &ok);
        if (!ok) { fprintf(stderr, "[%s] used ring GPA unbacked\n", dev->name); return; }
        uint16_t used_slot = (uint16_t)(used_idx & mask);          /* vq_ring_slot  */

        /* used.ring[] starts at offset 4; each virtq_used_elem is 8 bytes
         * (u32 id, u32 len). */
        uint64_t egpa = q->used_gpa + 4 + (uint64_t)used_slot * 8;
        g_st32(vm, egpa + 0, head, &ok);        /* which chain completed            */
        g_st32(vm, egpa + 4, used_len, &ok);    /* bytes we wrote into it           */
        if (!ok) { fprintf(stderr, "[%s] used ring slot unbacked\n", dev->name); return; }

        /* RELEASE fence: the used_elem stores above MUST be visible to the driver
         * before it observes the bumped idx, or the driver would read a stale slot
         * for a fresh completion. On x86's TSO this is a compiler barrier, but we
         * state the ordering explicitly so it is correct on any host and obvious
         * to the reader. This is the exact dual of the driver's avail-ring
         * release we acquire above. */
        __atomic_thread_fence(__ATOMIC_RELEASE);
        g_st16(vm, q->used_gpa + 2, (uint16_t)(used_idx + 1), &ok);   /* vq_next    */
        if (!ok) { fprintf(stderr, "[%s] used idx unbacked\n", dev->name); return; }

        dev->buffers_used++;
        q->last_avail = (uint16_t)(q->last_avail + 1);   /* advance our cursor      */
    }

    /* Signal "the used ring advanced." We do not inject an IRQ (no in-kernel
     * IRQCHIP in this teaching-core); the guest polls used.idx. A production VMM
     * would raise this device's IRQ line here via KVM_IRQFD / KVM_INTERRUPT. */
    dev->interrupt_status |= VIRTIO_MMIO_INT_VRING;
}

/* ===========================================================================
 * THE TRANSPORT: the virtio-mmio register block.
 * ===========================================================================
 * `gpa` is the faulting guest-physical address; the register offset is
 * gpa - dev->mmio_base. We handle it as the width the guest used (nearly always
 * 4 bytes for these registers).
 * ======================================================================== */

void virtio_mmio_read(struct vm *vm, struct virtio_dev *dev,
                      uint64_t gpa, uint32_t len, uint64_t *out)
{
    (void)vm; (void)len;
    uint64_t off = gpa - dev->mmio_base;
    uint32_t v = 0;

    switch (off) {
    case VIRTIO_MMIO_MAGIC_VALUE:  v = VIRTIO_MMIO_MAGIC;          break; /* 'virt' */
    case VIRTIO_MMIO_VERSION:      v = VIRTIO_MMIO_VERSION_MODERN; break; /* 2      */
    case VIRTIO_MMIO_DEVICE_ID:    v = dev->device_id;             break;
    case VIRTIO_MMIO_VENDOR_ID:    v = 0x4b4d564d;                 break; /* 'KMVM' */

    case VIRTIO_MMIO_DEVICE_FEATURES:
        /* Feature bits come in 32-bit words selected by DEVICE_FEATURES_SEL. Word
         * 0 is device-specific (we offer none here to keep the stubs minimal);
         * word 1 carries VIRTIO_F_VERSION_1 (bit 32), which every modern device
         * MUST offer so the driver uses the 1.0 layout. */
        v = (dev->device_features_sel == VIRTIO_F_VERSION_1_WORD)
              ? VIRTIO_F_VERSION_1_BIT : 0u;
        break;

    case VIRTIO_MMIO_QUEUE_NUM_MAX: v = VQ_SIZE;                          break;
    case VIRTIO_MMIO_QUEUE_READY:
        v = (dev->queue_sel < VIRTIO_MAX_QUEUES) ? dev->vq[dev->queue_sel].ready : 0;
        break;
    case VIRTIO_MMIO_INTERRUPT_STATUS: v = dev->interrupt_status;         break;
    case VIRTIO_MMIO_STATUS:           v = dev->status;                   break;
    case VIRTIO_MMIO_CONFIG_GENERATION:v = dev->config_generation;        break;

    default:
        /* Device-specific config space (>=0x100) and anything else reads as 0.
         * Our guest reads none of it; a real console would expose cols/rows here. */
        v = 0;
        break;
    }
    *out = v;
}

void virtio_mmio_write(struct vm *vm, struct virtio_dev *dev,
                       uint64_t gpa, uint32_t len, uint64_t val)
{
    (void)len;
    uint64_t off = gpa - dev->mmio_base;
    uint32_t v = (uint32_t)val;
    struct virtq_state *q = (dev->queue_sel < VIRTIO_MAX_QUEUES)
                              ? &dev->vq[dev->queue_sel] : &dev->vq[0];

    switch (off) {
    case VIRTIO_MMIO_DEVICE_FEATURES_SEL: dev->device_features_sel = v; break;
    case VIRTIO_MMIO_DRIVER_FEATURES_SEL: dev->driver_features_sel = v; break;
    case VIRTIO_MMIO_DRIVER_FEATURES:
        /* The driver tells us which features it accepts, one 32-bit word at a
         * time. We store them; a strict device would verify VERSION_1 was
         * accepted before allowing DRIVER_OK. */
        if (dev->driver_features_sel == 0)
            dev->driver_features = (dev->driver_features & 0xffffffff00000000ull) | v;
        else
            dev->driver_features = (dev->driver_features & 0x00000000ffffffffull)
                                 | ((uint64_t)v << 32);
        break;

    case VIRTIO_MMIO_QUEUE_SEL:
        dev->queue_sel = v;                 /* subsequent QUEUE_* target this queue */
        break;
    case VIRTIO_MMIO_QUEUE_NUM:   q->num       = v; break;  /* chosen queue size    */
    case VIRTIO_MMIO_QUEUE_READY: q->ready     = v; break;  /* 1 => queue is live   */

    case VIRTIO_MMIO_QUEUE_DESC_LOW:
        q->desc_gpa  = (q->desc_gpa  & 0xffffffff00000000ull) | v;               break;
    case VIRTIO_MMIO_QUEUE_DESC_HIGH:
        q->desc_gpa  = (q->desc_gpa  & 0x00000000ffffffffull) | ((uint64_t)v << 32); break;
    case VIRTIO_MMIO_QUEUE_DRIVER_LOW:
        q->avail_gpa = (q->avail_gpa & 0xffffffff00000000ull) | v;               break;
    case VIRTIO_MMIO_QUEUE_DRIVER_HIGH:
        q->avail_gpa = (q->avail_gpa & 0x00000000ffffffffull) | ((uint64_t)v << 32); break;
    case VIRTIO_MMIO_QUEUE_DEVICE_LOW:
        q->used_gpa  = (q->used_gpa  & 0xffffffff00000000ull) | v;               break;
    case VIRTIO_MMIO_QUEUE_DEVICE_HIGH:
        q->used_gpa  = (q->used_gpa  & 0x00000000ffffffffull) | ((uint64_t)v << 32); break;

    case VIRTIO_MMIO_QUEUE_NOTIFY:
        /* ===== THE KICK ===== the whole reason MMIO matters. The written value
         * is the queue index the driver just added buffers to. This is the single
         * VM exit per batch of work; everything else was plain memory. */
        process_queue(vm, dev, v);
        break;

    case VIRTIO_MMIO_INTERRUPT_ACK:
        dev->interrupt_status &= ~v;        /* driver clears the bits it handled    */
        break;

    case VIRTIO_MMIO_STATUS:
        /* A write of 0 is a device RESET (the driver re-initializing). Otherwise
         * the driver ORs in its handshake progress; DRIVER_OK means it is live. */
        if (v == 0) {
            fprintf(stderr, "[%s] driver reset the device\n", dev->name);
            /* reset transport + queue state but keep identity */
            uint64_t base = dev->mmio_base; enum virtio_kind k = dev->kind;
            virtio_dev_init(dev, k, base);
        } else {
            dev->status = v;
            if (v & VIRTIO_STATUS_DRIVER_OK)
                fprintf(stderr, "[%s] driver is up (STATUS=0x%x, DRIVER_OK)\n",
                        dev->name, v);
        }
        break;

    default:
        /* Config-space writes and unknown registers: ignore, but say so once so a
         * misbehaving driver is visible. */
        fprintf(stderr, "[%s] write to unhandled reg 0x%llx = 0x%x\n",
                dev->name, (unsigned long long)off, v);
        break;
    }
}
