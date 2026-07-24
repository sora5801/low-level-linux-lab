/* ===========================================================================
 * driver.h — the device handle, the ring/queue state, and MMIO accessors.
 * ===========================================================================
 *
 * This is the heart of the teaching model. Two ideas dominate:
 *
 *   MMIO ORDERING. Registers live in an uncacheable device window. Reads and
 *   writes to them are real PCIe transactions, but the COMPILER doesn't know
 *   that — it may reorder, coalesce, or elide "plain" memory accesses. We defeat
 *   that with `volatile` (forces the access to actually happen, every time, in
 *   program order relative to other volatiles) plus a compiler barrier. On
 *   x86-64 (TSO) that is sufficient for register ordering; on weakly-ordered
 *   ISAs you would additionally need real fence instructions. The classic bug
 *   this prevents: writing a descriptor ring in RAM and then "ringing the
 *   doorbell" (a tail-register write) BEFORE the descriptor write is visible to
 *   the device, so the NIC reads a stale descriptor.
 *
 *   DMA COHERENCE. Descriptors and packet buffers are ordinary cacheable RAM
 *   that BOTH the CPU and the NIC's DMA engine read and write. On x86 the PCIe
 *   root complex keeps DMA cache-coherent (the NIC's writes snoop our caches),
 *   so we do NOT need explicit cache flushes — but we DO need the CPU to
 *   re-read descriptor status from memory each poll (hence `volatile` on the
 *   descriptor pointers) and we need a load-load barrier after seeing the DD bit
 *   before trusting the length field the NIC wrote alongside it.
 * ========================================================================= */
#ifndef IXY_DRIVER_H
#define IXY_DRIVER_H

#include <stdint.h>
#include <stddef.h>
#include "ixgbe_regs.h"
#include "memory.h"

/* Ring sizes MUST be powers of two: the NIC wraps the head/tail indices with a
 * bitmask, and so do we (idx & (N-1)). 512 is a good default — big enough to
 * absorb bursts without cache-thrashing the descriptor array. */
#define NUM_RX_QUEUE_ENTRIES 512
#define NUM_TX_QUEUE_ENTRIES 512

/* We only set the Report-Status (RS) bit — and thus only reclaim finished TX
 * buffers — every 32 descriptors, to amortise the writeback DMA. So TX cleaning
 * happens in batches of 32. */
#define TX_CLEAN_BATCH 32

/* Software view of one RX queue. The `descriptors` array is DMA memory shared
 * with the NIC; everything else is our private bookkeeping. */
struct ixgbe_rx_queue {
    volatile union ixgbe_adv_rx_desc *descriptors; /* DMA ring (NIC + CPU)     */
    struct mempool *mempool;      /* pool the RX buffers come from             */
    uint16_t num_entries;         /* ring size (power of two)                  */
    uint16_t rx_index;            /* next descriptor WE will check (SW head)   */
    /* For each descriptor slot, the pkt_buf currently posted there, so that on
     * completion we can hand the right buffer to the application. */
    void *virtual_addresses[NUM_RX_QUEUE_ENTRIES];
};

/* Software view of one TX queue. */
struct ixgbe_tx_queue {
    volatile union ixgbe_adv_tx_desc *descriptors; /* DMA ring (NIC + CPU)     */
    uint16_t num_entries;         /* ring size (power of two)                  */
    uint16_t clean_index;         /* oldest not-yet-reclaimed descriptor       */
    uint16_t tx_index;            /* next descriptor WE will fill (SW tail)    */
    void *virtual_addresses[NUM_TX_QUEUE_ENTRIES];
};

/* The device handle. `addr` is the mmap'd BAR0 base — the anchor for every MMIO
 * access. Queues are allocated inline for simplicity (single-NIC teaching core;
 * ixy supports many). */
struct ixy_device {
    uint8_t *addr;                /* BAR0 MMIO base pointer                    */
    char     pci_addr[32];        /* e.g. "0000:03:00.0"                       */
    uint16_t num_rx_queues;
    uint16_t num_tx_queues;
    struct ixgbe_rx_queue rx_queues[1];
    struct ixgbe_tx_queue tx_queues[1];
};

/* Simple counters snapshot for throughput reporting. */
struct device_stats {
    uint64_t rx_pkts;
    uint64_t tx_pkts;
    uint64_t rx_bytes;
    uint64_t tx_bytes;
};

/* ===========================================================================
 * MMIO register accessors — the ONLY correct way to touch device registers.
 * =========================================================================*/

/* Compiler-only barrier: prevents the compiler from moving memory accesses
 * across this point. It emits NO instruction. On x86-64's strong (TSO) model
 * this is all that is required to order our MMIO and DMA-descriptor accesses
 * with respect to each other; it is the software analogue of a fence. */
static inline void compiler_barrier(void)
{
    __asm__ volatile ("" ::: "memory");
}

/* Write a 32-bit device register at byte offset `reg` from BAR0.
 * - The `volatile` store guarantees the write is actually performed exactly
 *   once and is not reordered with other volatile accesses.
 * - The trailing barrier prevents LATER plain memory writes (e.g. filling the
 *   next descriptor) from being reordered before this doorbell write by the
 *   compiler. This is the doorbell-ordering safeguard described in the header. */
static inline void set_reg32(uint8_t *addr, uint32_t reg, uint32_t value)
{
    compiler_barrier();
    *((volatile uint32_t *)(addr + reg)) = value;
}

/* Read a 32-bit device register. `volatile` forces a fresh PCIe read every call
 * (status registers change under us); the barrier keeps the read from being
 * hoisted above earlier writes whose effect we are polling for. */
static inline uint32_t get_reg32(const uint8_t *addr, uint32_t reg)
{
    compiler_barrier();
    return *((volatile uint32_t *)(addr + reg));
}

/* Set/clear specific bits in a register via read-modify-write. Used constantly
 * during init where we must not disturb reserved or unrelated bits. */
static inline void set_flags32(uint8_t *addr, uint32_t reg, uint32_t flags)
{
    set_reg32(addr, reg, get_reg32(addr, reg) | flags);
}
static inline void clear_flags32(uint8_t *addr, uint32_t reg, uint32_t flags)
{
    set_reg32(addr, reg, get_reg32(addr, reg) & ~flags);
}

/* Spin until (reg & mask) == mask. Used to wait for self-clearing/handshake
 * bits like EEPROM-auto-read-done or queue-enable. Callers only use this on
 * bits the datasheet guarantees will be set within microseconds. */
static inline void wait_set_reg32(const uint8_t *addr, uint32_t reg, uint32_t mask)
{
    /* Busy-wait: init happens once at startup, so a spin is fine and avoids
     * pulling in a sleep dependency. get_reg32's volatile read re-fetches each
     * iteration (a non-volatile read here would be hoisted and spin forever). */
    while ((get_reg32(addr, reg) & mask) != mask)
        __asm__ volatile ("pause"); /* hint the CPU we're spin-waiting */
}
static inline void wait_clear_reg32(const uint8_t *addr, uint32_t reg, uint32_t mask)
{
    while ((get_reg32(addr, reg) & mask) != 0)
        __asm__ volatile ("pause");
}

/* ===========================================================================
 * Public driver API (implemented in ixgbe.c).
 * =========================================================================*/

/* Full bring-up: unbind kernel, enable DMA, map BAR0, reset, configure RX/TX,
 * allocate rings + mempools, wait for link. Returns a ready-to-poll handle. */
struct ixy_device *ixgbe_init(const char *pci_addr,
                              uint16_t num_rx_queues,
                              uint16_t num_tx_queues);

/* Poll one RX queue: harvest up to `num` completed frames into bufs[], refill
 * the freed descriptors with new buffers, advance the tail. Returns the count
 * received (0 if the NIC has produced nothing — the common poll result). */
uint32_t ixgbe_rx_batch(struct ixy_device *dev, uint16_t queue_id,
                        struct pkt_buf *bufs[], uint32_t num);

/* Enqueue up to `num` frames on one TX queue: reclaim finished descriptors,
 * post the new ones, ring the tail doorbell. Returns how many were accepted
 * (< num if the ring was full). Ownership of accepted buffers passes to the
 * driver, which frees them after the NIC signals completion. */
uint32_t ixgbe_tx_batch(struct ixy_device *dev, uint16_t queue_id,
                        struct pkt_buf *bufs[], uint32_t num);

/* Read the hardware good-packet/good-octet counters (read-to-clear). */
void ixgbe_read_stats(struct ixy_device *dev, struct device_stats *stats);

#endif /* IXY_DRIVER_H */
