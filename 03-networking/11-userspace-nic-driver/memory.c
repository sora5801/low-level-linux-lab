/* ===========================================================================
 * memory.c — hugepage DMA allocation, virt<->phys, and the mempool allocator.
 * ===========================================================================
 * See memory.h for the "why hugepages / why physical addresses" overview. This
 * file is the machinery. Everything here is Linux-specific (pagemap, hugetlbfs,
 * mlock) — that is inherent to talking to DMA hardware from userspace.
 * ========================================================================= */
#define _GNU_SOURCE
#include "memory.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <inttypes.h>   /* PRIuFAST32 for the unique hugepage filename */
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

/* Where the kernel mounts hugetlbfs. Files created here are backed by 2 MB
 * pages. The run guide mounts it and reserves pages; see README. */
#define HUGE_PAGE_DIR "/mnt/huge"

/* ---------------------------------------------------------------------------
 * virt_to_phys — translate a virtual address to its physical address.
 *
 * The kernel exposes the page-table mapping through /proc/self/pagemap: an
 * array indexed by virtual page number, 8 bytes per entry. Bits [54:0] of an
 * entry are the Page Frame Number (PFN) when bit 63 ("page present") is set.
 * physical = PFN * pagesize + offset_within_page.
 *
 * This is ONLY stable because the page is a mlock()'d hugepage: for ordinary
 * pageable memory the PFN could change out from under us the instant after we
 * read it, which is precisely the bug hugepages+mlock avoid.
 * --------------------------------------------------------------------------- */
static uintptr_t virt_to_phys(void *virt)
{
    long pagesize = sysconf(_SC_PAGESIZE);
    if (pagesize <= 0)
        error("sysconf(_SC_PAGESIZE) failed");

    /* Opening pagemap requires privilege (root or CAP_SYS_ADMIN); without it
     * the PFN reads back as 0 on modern kernels, which would silently point the
     * NIC at physical page 0. We check the read succeeded but the caller is
     * responsible for running privileged (README says so). */
    int fd = check_err(open("/proc/self/pagemap", O_RDONLY), "opening pagemap");

    /* Seek to the 8-byte entry for this virtual page. */
    uint64_t entry = 0;
    off_t offset = (off_t)((uintptr_t)virt / (uintptr_t)pagesize) * (off_t)sizeof(entry);
    if (pread(fd, &entry, sizeof(entry), offset) != (ssize_t)sizeof(entry))
        error("reading pagemap entry");
    check_err(close(fd), "closing pagemap");

    if (!(entry & (1ULL << 63)))
        error("page not present in RAM — is it mlock()'d? (need root)");

    uint64_t pfn = entry & 0x7FFFFFFFFFFFFFULL;         /* bits [54:0] */
    return (uintptr_t)(pfn * (uint64_t)pagesize
                       + (uintptr_t)virt % (uintptr_t)pagesize);
}

/* Monotonic id so concurrent allocations get unique hugetlbfs filenames.
 * Atomic because a multi-queue driver may allocate from several threads during
 * setup; relaxed ordering is fine — we only need uniqueness, not ordering. */
static atomic_uint_fast32_t huge_id = 0;

struct dma_memory memory_allocate_dma(size_t size, bool require_contiguous)
{
    /* Round the request up to a whole hugepage. Sub-page allocations would
     * waste a page anyway (hugetlbfs is page-granular) and rounding keeps every
     * allocation naturally hugepage-aligned. */
    if (size % HUGE_PAGE_SIZE != 0)
        size = ((size >> HUGE_PAGE_BITS) + 1) << HUGE_PAGE_BITS;

    /* A single hugepage is contiguous by definition; anything larger might span
     * several hugepages that are NOT physically adjacent. Descriptor rings and
     * mempools here are < 2 MB, so we never actually need the multi-page case. */
    if (require_contiguous && size > HUGE_PAGE_SIZE)
        error("cannot guarantee contiguity for %zu bytes (> one hugepage)", size);

    /* Create a uniquely-named file in hugetlbfs and size it. Mapping a FILE
     * (MAP_SHARED) rather than anonymous memory means the physical pages are
     * pinned to the file, which keeps their physical address stable and lets us
     * unlink immediately (the mapping keeps the pages alive). */
    uint_fast32_t id = atomic_fetch_add_explicit(&huge_id, 1, memory_order_relaxed);
    char path[128];
    snprintf(path, sizeof(path), "%s/ixy-%d-%" PRIuFAST32,
             HUGE_PAGE_DIR, getpid(), id);

    int fd = check_err(open(path, O_CREAT | O_RDWR, S_IRWXU), "opening hugepage file");
    check_err(ftruncate(fd, (off_t)size), "sizing hugepage file");

    /* MAP_HUGETLB forces 2 MB pages; MAP_SHARED so writes hit the file's pages;
     * we do not pass an address hint. On success the kernel has faulted in and
     * zeroed the pages. */
    void *virt = mmap(NULL, size, PROT_READ | PROT_WRITE,
                      MAP_SHARED | MAP_HUGETLB, fd, 0);
    if (virt == MAP_FAILED)
        error("mmap of hugepage failed — are hugepages reserved and mounted?");

    /* Pin the pages: mlock guarantees they are resident and never swapped, so
     * the physical address we are about to capture stays valid for the device's
     * lifetime. Without this, a DMA could land in a page the kernel reclaimed. */
    check_err(mlock(virt, size), "mlock of DMA memory");

    /* Unlink now: the open mapping holds a reference, so the pages persist, but
     * the name is gone so we never leak stale files across runs/crashes. */
    check_err(close(fd), "closing hugepage file");
    unlink(path); /* best-effort; page stays mapped */

    struct dma_memory mem = { .virt = virt, .phy = virt_to_phys(virt) };
    return mem;
}

struct mempool *memory_allocate_mempool(uint32_t num_entries, uint32_t entry_size)
{
    /* Default to 2048B buffers: 1500B MTU + our metadata, and 2048 is a power
     * of two so buffer boundaries never straddle a 4 KB page in a way that
     * would split a DMA. */
    entry_size = entry_size ? entry_size : 2048;

    /* The DMA region holds the buffers themselves. It must be physically
     * contiguous so that a buffer's physical address is simply base_phys + idx
     * * entry_size (no per-buffer translation on the hot path). */
    if (HUGE_PAGE_SIZE % entry_size != 0)
        error("entry_size %u must divide the hugepage size", entry_size);

    struct dma_memory mem =
        memory_allocate_dma((size_t)num_entries * entry_size, true);

    /* The pool bookkeeping (free stack) lives in ordinary heap memory — the NIC
     * never touches it, so it needs no DMA properties. */
    struct mempool *mp = malloc(sizeof(*mp) + num_entries * sizeof(uint32_t));
    if (!mp)
        error("allocating mempool bookkeeping");
    mp->base_addr   = mem.virt;
    mp->buf_size    = entry_size;
    mp->num_entries = num_entries;
    mp->free_stack_top = num_entries;   /* every buffer starts free */

    /* Initialise each buffer's embedded metadata and push it onto the free
     * stack. We compute each buffer's physical address ONCE here and cache it in
     * the buffer, so the TX fast path never calls virt_to_phys. */
    for (uint32_t i = 0; i < num_entries; i++) {
        mp->free_stack[i] = i;
        struct pkt_buf *buf =
            (struct pkt_buf *)((uint8_t *)mem.virt + (size_t)i * entry_size);
        /* base physical + offset within the contiguous region == this buffer's
         * physical address (valid because the region is one hugepage). */
        buf->buf_addr_phy = mem.phy + (uintptr_t)i * entry_size;
        buf->mempool      = mp;
        buf->mempool_idx  = i;
        buf->size         = 0;
    }
    return mp;
}

uint32_t pkt_buf_alloc_batch(struct mempool *mp, struct pkt_buf *bufs[], uint32_t num)
{
    if (mp->free_stack_top < num) {
        warn("mempool nearly empty: requested %u, only %u free",
             num, mp->free_stack_top);
        num = mp->free_stack_top;
    }
    for (uint32_t i = 0; i < num; i++) {
        /* Pop an index off the free stack (LIFO keeps the most-recently-freed,
         * cache-hot buffer). Reconstruct its address from base + idx*stride. */
        uint32_t idx = mp->free_stack[--mp->free_stack_top];
        bufs[i] = (struct pkt_buf *)((uint8_t *)mp->base_addr
                                     + (size_t)idx * mp->buf_size);
    }
    return num;
}

struct pkt_buf *pkt_buf_alloc(struct mempool *mp)
{
    struct pkt_buf *buf = NULL;
    pkt_buf_alloc_batch(mp, &buf, 1);
    return buf;
}

void pkt_buf_free(struct pkt_buf *buf)
{
    struct mempool *mp = buf->mempool;
    /* Push the index back. No bounds check on the hot path: freeing a buffer
     * twice would overflow the stack and hand the same buffer to two
     * descriptors — a class of bug the caller must prevent by construction (a
     * buffer is owned by exactly one descriptor slot at a time). */
    mp->free_stack[mp->free_stack_top++] = buf->mempool_idx;
}
