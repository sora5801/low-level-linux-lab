/* ===========================================================================
 * ixgbe.c — a polling userspace driver core for the Intel 82599 (ixgbe).
 * ===========================================================================
 *
 * This is the teaching core, modelled on the ixy educational driver. It brings
 * the NIC up entirely from userspace and drives RX/TX by POLLING descriptor
 * rings — no interrupts, no kernel networking stack. Read the three phases:
 *
 *   1. ixgbe_init         — reset the chip and configure the RX and TX engines.
 *   2. ixgbe_rx_batch     — the receive hot path (poll DD bit, refill, advance).
 *   3. ixgbe_tx_batch     — the transmit hot path (reclaim, post, doorbell).
 *
 * WHY POLLING BEATS INTERRUPTS AT HIGH PPS: a 10 GbE link at 64-byte frames is
 * 14.88 million packets/second. One interrupt per packet means ~15M interrupts/s
 * — each costs a pipeline flush, a mode switch, and cache pollution, so the CPU
 * would spend all its time entering/leaving the handler and never process a
 * packet. Polling a descriptor's DD bit is an L1-cache read of a few
 * nanoseconds; the DMA coherence fabric pushes the NIC's writeback straight into
 * our cache. So at line rate, spinning on memory is strictly cheaper than being
 * interrupted. (At LOW rates interrupts win on power/latency — real drivers
 * switch adaptively; DPDK/ixy just poll.)
 *
 * SCOPE / HONESTY: single traffic class, single RX + single TX queue, no RSS,
 * no checksum/TSO offload, no jumbo frames, no VF/SR-IOV, no link-flap handling.
 * The register programming is the real 82599 sequence; the omissions are called
 * out in the README. Requires real 82599 hardware bound away from the kernel.
 * ========================================================================= */
#define _GNU_SOURCE
#include "driver.h"
#include "pci.h"
#include "memory.h"
#include "log.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* ---------------------------------------------------------------------------
 * init_rx — configure the receive engine and one RX queue.
 * --------------------------------------------------------------------------- */
static void init_rx(struct ixy_device *dev)
{
    /* Disable RX while we reconfigure it; the datasheet requires RXCTRL.RXEN=0
     * before touching the per-queue setup, otherwise the DMA engine may act on
     * half-written ring pointers. */
    clear_flags32(dev->addr, IXGBE_RXCTRL, IXGBE_RXCTRL_RXEN);

    /* Give all of packet-buffer pool 0 the full 128 KB of on-chip RX FIFO; zero
     * the other 7 pools since we use a single traffic class. */
    set_reg32(dev->addr, IXGBE_RXPBSIZE(0), IXGBE_RXPBSIZE_128KB);
    for (int i = 1; i < 8; i++)
        set_reg32(dev->addr, IXGBE_RXPBSIZE(i), 0);

    /* Enable CRC stripping. Two registers must agree: RDRXCTL for the DMA path,
     * HLREG0 for the MAC. If they disagree the 82599 wedges the RX unit. */
    set_flags32(dev->addr, IXGBE_RDRXCTL, IXGBE_RDRXCTL_CRCSTRIP);
    set_flags32(dev->addr, IXGBE_HLREG0, IXGBE_HLREG0_RXCRCSTRP);

    /* Accept broadcast frames (ARP etc.) — otherwise the card silently filters
     * them and the demo looks dead. Promiscuous mode is set later. */
    set_flags32(dev->addr, IXGBE_FCTRL, IXGBE_FCTRL_BAM);

    for (uint16_t i = 0; i < dev->num_rx_queues; i++) {
        struct ixgbe_rx_queue *q = &dev->rx_queues[i];

        /* SRRCTL: advanced descriptors, one 2 KB buffer per packet, and DROP_EN
         * so that when we fall behind the NIC DROPS frames instead of stalling
         * the whole RX FIFO (head-of-line blocking). Dropping is the right
         * choice for a poll-mode driver under overload. */
        uint32_t srrctl = get_reg32(dev->addr, IXGBE_SRRCTL(i));
        srrctl &= ~0x1Fu;                          /* clear BSIZEPACKET field   */
        srrctl |= 2048u >> IXGBE_SRRCTL_BSIZEPKT_SHIFT; /* 2048B -> value 2 (KB)*/
        srrctl |= IXGBE_SRRCTL_DESCTYPE_ADV_ONEBUF;
        srrctl |= IXGBE_SRRCTL_DROP_EN;
        set_reg32(dev->addr, IXGBE_SRRCTL(i), srrctl);

        /* Allocate the descriptor ring in DMA-contiguous hugepage memory. It
         * must be contiguous because the NIC walks it by physical address with a
         * single base + stride, and 16-byte aligned (a hugepage is 2 MB
         * aligned, so this holds trivially). */
        size_t ring_size_bytes =
            (size_t)NUM_RX_QUEUE_ENTRIES * sizeof(union ixgbe_adv_rx_desc);
        struct dma_memory ring = memory_allocate_dma(ring_size_bytes, true);
        /* Poison the ring to 0xFF so a forgotten init shows up as an obviously
         * bogus descriptor rather than a plausible-looking zero. */
        memset(ring.virt, 0xFF, ring_size_bytes);

        q->descriptors  = (volatile union ixgbe_adv_rx_desc *)ring.virt;
        q->num_entries  = NUM_RX_QUEUE_ENTRIES;
        q->rx_index     = 0;

        /* Tell the NIC where the ring is (split across two 32-bit registers) and
         * how long it is. RDBAL/RDBAH take the PHYSICAL address. */
        set_reg32(dev->addr, IXGBE_RDBAL(i), (uint32_t)(ring.phy & 0xFFFFFFFFu));
        set_reg32(dev->addr, IXGBE_RDBAH(i), (uint32_t)(ring.phy >> 32));
        set_reg32(dev->addr, IXGBE_RDLEN(i), (uint32_t)ring_size_bytes);

        /* Head/tail start at 0; we fill descriptors and bump tail in start_rx. */
        set_reg32(dev->addr, IXGBE_RDH(i), 0);
        set_reg32(dev->addr, IXGBE_RDT(i), 0);
    }

    /* CTRL_EXT.NS_DIS: disable "no-snoop" so DMA stays cache-coherent with the
     * CPU — required for our coherence assumptions (we never flush caches). */
    set_flags32(dev->addr, IXGBE_CTRL_EXT, IXGBE_CTRL_EXT_NS_DIS);

    /* Re-enable the RX master switch now that every queue is described. */
    set_flags32(dev->addr, IXGBE_RXCTRL, IXGBE_RXCTRL_RXEN);
}

/* Post one fresh buffer into RX descriptor `di` of queue `q`: point the NIC's
 * read-format descriptor at the buffer's DATA region (skipping our metadata +
 * headroom) and remember the buffer so we can return it on completion. */
static void rx_post_buffer(struct ixgbe_rx_queue *q, uint16_t di,
                           struct pkt_buf *buf)
{
    /* The NIC DMAs into buf->data; its physical address is the buffer's base
     * physical address plus the byte offset of the flexible `data[]` member. */
    uintptr_t data_phy = buf->buf_addr_phy + offsetof(struct pkt_buf, data);
    q->descriptors[di].read.pkt_addr = data_phy;
    q->descriptors[di].read.hdr_addr = 0;   /* one-buffer mode: no split header */
    q->virtual_addresses[di] = buf;
}

/* start_rx_queue — allocate the RX mempool, fill every descriptor with a fresh
 * buffer, enable the queue, and hand ALL descriptors to the NIC by pushing the
 * tail to num_entries-1. */
static void start_rx_queue(struct ixy_device *dev, uint16_t i)
{
    struct ixgbe_rx_queue *q = &dev->rx_queues[i];

    /* One buffer per descriptor, plus a few spare so refill never starves. */
    q->mempool = memory_allocate_mempool(NUM_RX_QUEUE_ENTRIES * 2, 2048);

    for (uint16_t di = 0; di < q->num_entries; di++) {
        struct pkt_buf *buf = pkt_buf_alloc(q->mempool);
        if (!buf)
            error("failed to allocate RX buffer during queue start");
        rx_post_buffer(q, di, buf);
    }

    /* Enable the queue and wait for the hardware to acknowledge. */
    set_flags32(dev->addr, IXGBE_RXDCTL(i), IXGBE_RXDCTL_ENABLE);
    wait_set_reg32(dev->addr, IXGBE_RXDCTL(i), IXGBE_RXDCTL_ENABLE);

    /* Reset head to 0 and set tail to the LAST descriptor: RDT is the last
     * descriptor the SOFTWARE owns, so [RDH..RDT] is the pool of empty buffers
     * the NIC may fill. Handing it all num_entries-1 descriptors primes RX. */
    set_reg32(dev->addr, IXGBE_RDH(i), 0);
    set_reg32(dev->addr, IXGBE_RDT(i), q->num_entries - 1);
}

/* ---------------------------------------------------------------------------
 * init_tx — configure the transmit engine and one TX queue.
 * --------------------------------------------------------------------------- */
static void init_tx(struct ixy_device *dev)
{
    /* MAC must insert the CRC and pad runt frames to 64 bytes, or the wire
     * frames are malformed and the link partner drops them. */
    set_flags32(dev->addr, IXGBE_HLREG0,
                IXGBE_HLREG0_TXCRCEN | IXGBE_HLREG0_TXPADEN);

    /* Give pool 0 the full 40 KB TX FIFO; zero the rest (single traffic class).*/
    set_reg32(dev->addr, IXGBE_TXPBSIZE(0), IXGBE_TXPBSIZE_40KB);
    for (int i = 1; i < 8; i++)
        set_reg32(dev->addr, IXGBE_TXPBSIZE(i), 0);

    /* Allow the maximum number of outstanding TX byte requests, and disable DCB
     * arbitration (we have one queue, so arbitration is meaningless overhead).*/
    set_reg32(dev->addr, IXGBE_DTXMXSZRQ, 0xFFF);
    clear_flags32(dev->addr, IXGBE_RTTDCS, IXGBE_RTTDCS_ARBDIS);

    for (uint16_t i = 0; i < dev->num_tx_queues; i++) {
        struct ixgbe_tx_queue *q = &dev->tx_queues[i];

        size_t ring_size_bytes =
            (size_t)NUM_TX_QUEUE_ENTRIES * sizeof(union ixgbe_adv_tx_desc);
        struct dma_memory ring = memory_allocate_dma(ring_size_bytes, true);
        memset(ring.virt, 0xFF, ring_size_bytes);

        q->descriptors = (volatile union ixgbe_adv_tx_desc *)ring.virt;
        q->num_entries = NUM_TX_QUEUE_ENTRIES;
        q->clean_index = 0;
        q->tx_index    = 0;

        set_reg32(dev->addr, IXGBE_TDBAL(i), (uint32_t)(ring.phy & 0xFFFFFFFFu));
        set_reg32(dev->addr, IXGBE_TDBAH(i), (uint32_t)(ring.phy >> 32));
        set_reg32(dev->addr, IXGBE_TDLEN(i), (uint32_t)ring_size_bytes);

        /* TXDCTL thresholds (PTHRESH/HTHRESH/WTHRESH) control how the NIC
         * prefetches and writes back descriptors. The values below (prefetch 36,
         * host 8, writeback 4) are the datasheet-recommended magic for good
         * throughput; the exact numbers matter less than that we set them and
         * do not leave WTHRESH at 0 (which would defer writebacks unbounded). */
        uint32_t txdctl = get_reg32(dev->addr, IXGBE_TXDCTL(i));
        txdctl &= ~(0x7Fu | (0x7Fu << 8) | (0x7Fu << 16)); /* clear 3 fields   */
        txdctl |= (36u) | (8u << 8) | (4u << 16);
        set_reg32(dev->addr, IXGBE_TXDCTL(i), txdctl);
    }

    /* Master transmit enable. From here the NIC will act on any descriptor we
     * expose by advancing the tail register. */
    set_flags32(dev->addr, IXGBE_DMATXCTL, IXGBE_DMATXCTL_TE);
}

static void start_tx_queue(struct ixy_device *dev, uint16_t i)
{
    struct ixgbe_tx_queue *q = &dev->tx_queues[i];

    /* TX starts EMPTY: head == tail == 0 means "no descriptors to send". We do
     * not pre-fill anything; tx_batch fills descriptors on demand. */
    set_reg32(dev->addr, IXGBE_TDH(i), 0);
    set_reg32(dev->addr, IXGBE_TDT(i), 0);

    set_flags32(dev->addr, IXGBE_TXDCTL(i), IXGBE_TXDCTL_ENABLE);
    wait_set_reg32(dev->addr, IXGBE_TXDCTL(i), IXGBE_TXDCTL_ENABLE);
    q->clean_index = 0;
    q->tx_index    = 0;
}

/* ---------------------------------------------------------------------------
 * reset_and_init — the full chip bring-up handshake.
 * --------------------------------------------------------------------------- */
static void reset_and_init(struct ixy_device *dev)
{
    /* Step 1: mask ALL interrupts. We are a poll-mode driver; an unmasked
     * interrupt cause with no handler installed would be undefined behaviour. */
    set_reg32(dev->addr, IXGBE_EIMC, 0x7FFFFFFFu);

    /* Step 2: global reset (link + device). The RST bit is self-clearing; the
     * datasheet requires ~10 ms before the chip is usable again. We spin on the
     * reset bit clearing rather than sleeping to avoid a timing dependency. */
    set_reg32(dev->addr, IXGBE_CTRL, IXGBE_CTRL_RST_MASK);
    wait_clear_reg32(dev->addr, IXGBE_CTRL, IXGBE_CTRL_RST_MASK);

    /* Step 3: mask interrupts again — a reset re-arms some causes. */
    set_reg32(dev->addr, IXGBE_EIMC, 0x7FFFFFFFu);

    /* Step 4: wait for the EEPROM auto-read to finish; MAC address and default
     * config are loaded from EEPROM and are invalid until this bit sets. */
    wait_set_reg32(dev->addr, IXGBE_EEC, IXGBE_EEC_ARD);

    /* Step 5: wait for the RX DMA engine's self-init to complete. */
    wait_set_reg32(dev->addr, IXGBE_RDRXCTL, IXGBE_RDRXCTL_DMAIDONE);

    /* Step 6: bring up the physical link (10G serial, restart autoneg). */
    uint32_t autoc = get_reg32(dev->addr, IXGBE_AUTOC);
    autoc &= ~IXGBE_AUTOC_LMS_MASK;
    autoc |= IXGBE_AUTOC_LMS_10G_SERIAL;
    autoc |= IXGBE_AUTOC_AN_RESTART;
    set_reg32(dev->addr, IXGBE_AUTOC, autoc);

    /* Step 7: configure the datapaths. */
    init_rx(dev);
    init_tx(dev);

    for (uint16_t i = 0; i < dev->num_rx_queues; i++)
        start_rx_queue(dev, i);
    for (uint16_t i = 0; i < dev->num_tx_queues; i++)
        start_tx_queue(dev, i);

    /* Step 8: promiscuous mode so we receive every frame on the wire (handy for
     * the reflector demo; a real app would program the unicast filter instead).*/
    set_flags32(dev->addr, IXGBE_FCTRL, IXGBE_FCTRL_UPE | IXGBE_FCTRL_MPE);

    /* Step 9: wait for link (up to ~1s of spinning; a real driver would time
     * out and report). LINKS.LINK_UP reflects the PHY state. */
    info("waiting for link...");
    wait_set_reg32(dev->addr, IXGBE_LINKS, IXGBE_LINKS_UP);
    info("link is up");
}

struct ixy_device *ixgbe_init(const char *pci_addr,
                              uint16_t num_rx_queues,
                              uint16_t num_tx_queues)
{
    /* This teaching core hard-codes a single queue each; the ring/queue arrays
     * in struct ixy_device are sized [1]. Guard the assumption loudly. */
    if (num_rx_queues != 1 || num_tx_queues != 1)
        error("teaching core supports exactly 1 RX and 1 TX queue");

    struct ixy_device *dev = calloc(1, sizeof(*dev));
    if (!dev)
        error("allocating device handle");
    snprintf(dev->pci_addr, sizeof(dev->pci_addr), "%s", pci_addr);
    dev->num_rx_queues = num_rx_queues;
    dev->num_tx_queues = num_tx_queues;

    /* Take the card away from the kernel, let it master DMA, and map its
     * registers. Order matters: unbind first (so the kernel driver is not
     * concurrently poking the same registers), then enable DMA, then map. */
    pci_remove_driver(pci_addr);
    pci_enable_dma(pci_addr);
    size_t bar_len = 0;
    dev->addr = pci_map_bar0(pci_addr, &bar_len);

    reset_and_init(dev);
    return dev;
}

/* ---------------------------------------------------------------------------
 * ixgbe_rx_batch — the RECEIVE hot path.
 *
 * Poll the ring starting at our software index. For each descriptor the NIC has
 * completed (DD status bit set via DMA writeback), hand the posted buffer to the
 * caller, post a FRESH buffer in its place, and advance. Finally bump the tail
 * register once for the whole batch so the NIC sees the newly-freed descriptors.
 * --------------------------------------------------------------------------- */
uint32_t ixgbe_rx_batch(struct ixy_device *dev, uint16_t queue_id,
                        struct pkt_buf *bufs[], uint32_t num)
{
    struct ixgbe_rx_queue *q = &dev->rx_queues[queue_id];
    uint16_t rx_index   = q->rx_index;   /* next descriptor to inspect */
    uint16_t last_index = rx_index;      /* last one we consumed (for RDT) */
    uint32_t received   = 0;

    for (; received < num; received++) {
        volatile union ixgbe_adv_rx_desc *desc = &q->descriptors[rx_index];

        /* Read the writeback status. This MUST be a volatile load: the NIC set
         * the DD bit via DMA into cacheable memory, and only a volatile access
         * forces the CPU to re-read it from the coherent cache line each poll
         * instead of caching a stale 'not done' value in a register. */
        uint32_t status = desc->wb.upper.status_error;
        if (!(status & IXGBE_RXD_STAT_DD))
            break;   /* NIC hasn't finished this descriptor: stop the batch. */

        /* LOAD-LOAD BARRIER. We observed DD; the length/EOP fields were DMA'd by
         * the NIC in the SAME writeback, but the compiler must not read them
         * before the DD check (and on weak ISAs the CPU must not either). On
         * x86-64 TSO this compiler barrier suffices; a real driver targeting ARM
         * would use a load-acquire on the status word. */
        compiler_barrier();

        if (!(status & IXGBE_RXD_STAT_EOP))
            error("multi-descriptor (jumbo) frame — unsupported in this core");

        /* Recover the buffer we posted here and record how many bytes the NIC
         * wrote into it (from the descriptor's length field). */
        struct pkt_buf *buf = q->virtual_addresses[rx_index];
        buf->size = desc->wb.upper.length;
        bufs[received] = buf;

        /* Refill: this descriptor slot is now empty, so post a new buffer so the
         * NIC can reuse the slot. If the pool is momentarily dry we stop early
         * rather than leave a descriptor pointing at a freed buffer. */
        struct pkt_buf *newbuf = pkt_buf_alloc(q->mempool);
        if (!newbuf) {
            warn("mempool exhausted during RX refill");
            break;
        }
        rx_post_buffer(q, rx_index, newbuf);

        last_index = rx_index;
        rx_index = (uint16_t)((rx_index + 1) & (q->num_entries - 1)); /* wrap */
    }

    if (received > 0) {
        /* Advance our software head and ring the RX doorbell ONCE for the batch:
         * RDT = index of the LAST descriptor software now owns (the freshly
         * refilled one). The set_reg32 barrier guarantees all the descriptor
         * refills above are visible to the NIC before this tail write. */
        q->rx_index = rx_index;
        set_reg32(dev->addr, IXGBE_RDT(queue_id), last_index);
    }
    return received;
}

/* clean_tx — reclaim finished TX descriptors so their buffers can be freed and
 * their slots reused. We only marked every 32nd descriptor with RS (Report
 * Status), so we check completion at 32-descriptor granularity. */
static void clean_tx(struct ixy_device *dev, struct ixgbe_tx_queue *q)
{
    uint16_t clean = q->clean_index;
    uint16_t cur   = q->tx_index;
    uint16_t mask  = q->num_entries - 1;

    while (true) {
        /* Distance from clean to the newest posted descriptor. */
        int32_t cleanable = (int32_t)((cur - clean) & mask);
        if (cleanable < TX_CLEAN_BATCH)
            break;   /* not a full batch outstanding yet */

        /* Look at the last descriptor of the next 32-descriptor group — the one
         * that carried the RS bit. If its DD writeback bit is set, the NIC has
         * finished the ENTIRE group, so all 32 buffers can be freed. */
        uint16_t check = (uint16_t)((clean + TX_CLEAN_BATCH - 1) & mask);
        volatile union ixgbe_adv_tx_desc *desc = &q->descriptors[check];
        /* volatile load: the NIC set DD via DMA; force a fresh read each poll. */
        uint32_t status = desc->wb.status;
        if (!(status & IXGBE_ADVTXD_STAT_DD))
            break;   /* group not sent yet */

        /* Free every buffer in the completed group. */
        for (int i = 0; i < TX_CLEAN_BATCH; i++) {
            struct pkt_buf *buf = q->virtual_addresses[clean];
            pkt_buf_free(buf);
            clean = (uint16_t)((clean + 1) & mask);
        }
    }
    q->clean_index = clean;
}

/* ---------------------------------------------------------------------------
 * ixgbe_tx_batch — the TRANSMIT hot path.
 *
 * Reclaim completed descriptors, then fill as many descriptors as we have
 * buffers and free ring slots for. Ring the tail doorbell once at the end.
 * Buffers we accept are owned by the driver until clean_tx frees them.
 * --------------------------------------------------------------------------- */
uint32_t ixgbe_tx_batch(struct ixy_device *dev, uint16_t queue_id,
                        struct pkt_buf *bufs[], uint32_t num)
{
    struct ixgbe_tx_queue *q = &dev->tx_queues[queue_id];
    uint16_t mask = q->num_entries - 1;

    /* Reclaim first so freed slots become available for THIS batch. */
    clean_tx(dev, q);

    uint32_t sent = 0;
    uint16_t cur  = q->tx_index;
    for (; sent < num; sent++) {
        /* The ring is full if advancing tail would collide with clean_index
         * (we must always leave the clean pointer distinguishable from tail). */
        uint16_t next = (uint16_t)((cur + 1) & mask);
        if (next == q->clean_index)
            break;   /* no free descriptor: stop; caller keeps the rest */

        struct pkt_buf *buf = bufs[sent];
        q->virtual_addresses[cur] = buf;

        /* Report Status only on every 32nd descriptor to batch the completion
         * writeback DMA (clean_tx checks exactly these). */
        uint32_t rs = ((cur & (TX_CLEAN_BATCH - 1)) == (TX_CLEAN_BATCH - 1))
                          ? IXGBE_ADVTXD_DCMD_RS : 0;

        volatile union ixgbe_adv_tx_desc *desc = &q->descriptors[cur];
        /* Point the descriptor at the packet DATA (past our metadata+headroom)
         * by physical address, and pack the command word: advanced data desc,
         * insert CRC, end-of-packet, plus the (optional) RS bit and the length.*/
        desc->read.buffer_addr = buf->buf_addr_phy + offsetof(struct pkt_buf, data);
        desc->read.cmd_type_len = IXGBE_ADVTXD_DTYP_DATA
                                | IXGBE_ADVTXD_DCMD_DEXT
                                | IXGBE_ADVTXD_DCMD_IFCS
                                | IXGBE_ADVTXD_DCMD_EOP
                                | rs
                                | buf->size;   /* PAYLEN of this descriptor */
        /* olinfo carries the total payload length in bits [31:14]; the NIC uses
         * it for segmentation accounting. Single-buffer frame => whole size. */
        desc->read.olinfo_status =
            (uint32_t)buf->size << IXGBE_ADVTXD_PAYLEN_SHIFT;

        cur = next;
    }

    /* Publish the new descriptors: update our tail index and ring the doorbell.
     * The set_reg32 barrier ensures every descriptor write above is globally
     * visible before the NIC reads them in response to this tail write — the
     * doorbell-ordering invariant. */
    q->tx_index = cur;
    set_reg32(dev->addr, IXGBE_TDT(queue_id), cur);
    return sent;
}

/* ---------------------------------------------------------------------------
 * ixgbe_read_stats — snapshot the good-packet/good-octet counters.
 * The 82599 stats registers are READ-TO-CLEAR: reading them resets them, so the
 * caller accumulates deltas. Octet counters are split into low(32)+high(4).
 * --------------------------------------------------------------------------- */
void ixgbe_read_stats(struct ixy_device *dev, struct device_stats *stats)
{
    uint32_t rx_pkts = get_reg32(dev->addr, IXGBE_GPRC);
    uint32_t tx_pkts = get_reg32(dev->addr, IXGBE_GPTC);
    /* Read low THEN high; the 82599 latches the high bits when the low word is
     * read, so this order gives a consistent 36-bit octet count. */
    uint64_t rx_lo = get_reg32(dev->addr, IXGBE_GORCL);
    uint64_t rx_hi = get_reg32(dev->addr, IXGBE_GORCH) & 0xF;
    uint64_t tx_lo = get_reg32(dev->addr, IXGBE_GOTCL);
    uint64_t tx_hi = get_reg32(dev->addr, IXGBE_GOTCH) & 0xF;

    stats->rx_pkts  += rx_pkts;
    stats->tx_pkts  += tx_pkts;
    stats->rx_bytes += rx_lo | (rx_hi << 32);
    stats->tx_bytes += tx_lo | (tx_hi << 32);
}
