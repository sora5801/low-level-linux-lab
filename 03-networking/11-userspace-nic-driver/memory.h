/* ===========================================================================
 * memory.h — DMA memory, hugepages, and a packet-buffer mempool.
 * ===========================================================================
 *
 * A NIC does DMA: it reads/writes host RAM using PHYSICAL addresses, bypassing
 * the CPU's page tables. That creates two problems a userspace driver must
 * solve itself, which the kernel normally hides:
 *
 *   1. PHYSICAL CONTIGUITY & PINNING. A normal malloc'd buffer may be scattered
 *      across physical pages and can be swapped out or migrated by the kernel
 *      at any moment. If the NIC DMAs to the physical address we captured a
 *      millisecond ago, it may now belong to another process. We defeat this
 *      with HUGEPAGES (2 MB, so a whole ring fits in one physically-contiguous
 *      page) that are mlock()'d (never swapped) and whose physical address we
 *      look up once via /proc/self/pagemap.
 *
 *   2. VIRTUAL<->PHYSICAL TRANSLATION. Descriptors must hold physical addresses
 *      (what the NIC understands); our code manipulates virtual ones (what the
 *      CPU understands). Every DMA buffer therefore carries both.
 *
 * NOTE ON IOMMU: with VFIO the IOMMU gives the device its own address space, so
 * you would program IOVAs instead of raw physical addresses and would not need
 * pagemap at all. This file implements the simpler UIO/sysfs path (raw physical
 * addresses); the README explains the VFIO alternative and its safety benefit.
 * ========================================================================= */
#ifndef IXY_MEMORY_H
#define IXY_MEMORY_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* 2 MB hugepage. 21 = log2(2*1024*1024). A 512-entry ring of 16-byte
 * descriptors is 8 KB, and even 512 * 2 KB packet buffers = 1 MB, so a single
 * hugepage holds a whole ring or pool contiguously — no scatter-gather. */
#define HUGE_PAGE_BITS  21
#define HUGE_PAGE_SIZE  (1 << HUGE_PAGE_BITS)

/* Every packet buffer reserves headroom before the data so an app can prepend
 * headers (e.g. a tunnel encap) without copying. 40 bytes is enough for the
 * struct's own metadata to sit in the same cache line region. */
#define SIZE_PKT_BUF_HEADROOM 40

/* A chunk of DMA-able memory: the virtual address our code uses and the
 * physical address the NIC's DMA engine uses for the SAME bytes. */
struct dma_memory {
    void     *virt;  /* CPU-visible mapping (from mmap of a hugepage)          */
    uintptr_t phy;   /* physical address for the device's DMA engine           */
};

/* A packet buffer. The layout is deliberate: buf_addr_phy is first so the hot
 * TX path can read the physical address with a single load, and `data[]` is the
 * flexible array the NIC actually DMAs into. Ownership: a buffer belongs to
 * exactly one mempool and is either "free" (index on the pool's free stack) or
 * "in flight" (referenced by a descriptor). Double-free = ring corruption. */
struct mempool; /* forward decl */

struct pkt_buf {
    uintptr_t       buf_addr_phy;  /* phys addr of THIS struct (for DMA descr) */
    struct mempool *mempool;       /* owning pool (for pkt_buf_free)           */
    uint32_t        mempool_idx;   /* our slot index within that pool          */
    uint32_t        size;          /* packet length in bytes (set by RX)       */
    uint8_t         head_room[SIZE_PKT_BUF_HEADROOM];
    uint8_t         data[];        /* the actual frame bytes; NIC DMAs here    */
};

/* A fixed-size free-list allocator ("mempool") of equal-sized packet buffers,
 * all carved from one DMA region. Allocation is O(1): pop an index off a stack.
 * This is a single-producer/single-consumer pool used by ONE queue thread — it
 * is intentionally NOT thread-safe, which is exactly why it is fast. */
struct mempool {
    void    *base_addr;    /* start of the buffer array (DMA virtual addr)     */
    uint32_t buf_size;     /* stride between buffers (a power of two)          */
    uint32_t num_entries;  /* how many buffers exist                          */
    uint32_t free_stack_top;         /* number of currently-free buffers       */
    uint32_t free_stack[];           /* stack of free buffer indices           */
};

/* Allocate `size` bytes of hugepage-backed, pinned, DMA-able memory. If
 * require_contiguous is true the whole allocation must fit one hugepage (so it
 * is physically contiguous). Aborts on failure. */
struct dma_memory memory_allocate_dma(size_t size, bool require_contiguous);

/* Build a mempool of `num_entries` buffers, each `entry_size` bytes (0 =>
 * default 2048, which holds a 1500-byte MTU frame plus our metadata). */
struct mempool *memory_allocate_mempool(uint32_t num_entries,
                                        uint32_t entry_size);

/* O(1) alloc/free of a single buffer from a pool. alloc returns NULL only if
 * the pool is exhausted (all buffers in flight). */
struct pkt_buf *pkt_buf_alloc(struct mempool *mp);
void            pkt_buf_free(struct pkt_buf *buf);

/* Bulk alloc used by RX-refill / TX-generate hot paths: fill bufs[0..num) and
 * return how many were actually allocated (< num if the pool ran dry). */
uint32_t pkt_buf_alloc_batch(struct mempool *mp, struct pkt_buf *bufs[],
                             uint32_t num);

#endif /* IXY_MEMORY_H */
