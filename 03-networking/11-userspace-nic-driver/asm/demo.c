/* ===========================================================================
 * demo.c — the RX/TX ring hot-loop kernels, extracted to compile standalone.
 * ===========================================================================
 *
 * WHY THIS FILE EXISTS
 * --------------------
 * The real driver source (ixgbe.c, memory.c, pci.c) all pull in Linux system
 * headers (sys/mman.h, fcntl.h, ...) and cannot be compiled on a non-Linux host
 * to produce teaching assembly. So we lift the driver's single most instructive
 * PURE-LOGIC routine — the descriptor-ring poll: index advance + DD status-bit
 * check + the volatile/barrier reasoning — into this self-contained file with
 * its own types and NO #includes. The generated asm/demo*.s come from THIS file.
 *
 * WHAT TO WATCH FOR IN THE ASSEMBLY (see demo.annotated.s):
 *   1. The `volatile` load of the descriptor status compiles to a real memory
 *      load INSIDE the loop — the optimizer may not hoist it to a register, even
 *      at -O2. That is the whole point: the NIC writes DD via DMA, so a cached
 *      copy would spin forever.
 *   2. `__asm__ volatile("" ::: "memory")` emits ZERO instructions but forces
 *      the compiler to keep the DD-check load ordered before the length load.
 *   3. `(i + 1) & (size - 1)` becomes a single `and` (no branch, no modulo)
 *      BECAUSE size is a power of two — the reason ring sizes must be 2^n.
 * ========================================================================= */

/* --- our own fixed-width types (no <stdint.h>) -----------------------------
 * On the x86-64 System V LP64 model: int=32, long=64, short=16, char=8. */
typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

/* A 16-byte descriptor, simplified from the 82599 advanced descriptor union to
 * exactly the fields the poll loop reads. `status` is the writeback word the NIC
 * DMAs into; `length` is the bytes it received. Layout/size must match the real
 * hardware descriptor (16 bytes) — we keep it so. */
struct desc {
    u64 addr;    /* read format: physical buffer address (driver writes)       */
    u32 status;  /* writeback:  NIC ORs in DD (bit 0), EOP (bit 1)             */
    u32 length;  /* writeback:  bytes the NIC DMA'd into the buffer           */
};

#define RXD_STAT_DD  0x01u   /* Descriptor Done — NIC has filled this slot     */
#define RXD_STAT_EOP 0x02u   /* End Of Packet                                  */
#define ADVTXD_STAT_DD 0x01u /* TX writeback Done bit                          */

/* ---------------------------------------------------------------------------
 * ring_next — advance a ring index with power-of-two wraparound.
 *
 * Because `size` is a power of two, wrapping is a bitmask AND, not a modulo:
 * (i + 1) & (size - 1). This is branchless and single-cycle, versus a `div`
 * (tens of cycles). It is THE reason descriptor rings are sized 2^n. If size
 * were not a power of two, (size-1) would not be a clean low-bit mask and this
 * would silently produce wrong indices.
 * --------------------------------------------------------------------------- */
u16 ring_next(u16 i, u16 size)
{
    return (u16)((i + 1) & (size - 1));
}

/* ---------------------------------------------------------------------------
 * read_status — a forced fresh load of the descriptor status word.
 *
 * The cast-through-volatile is the crux of DMA polling. The NIC updated
 * d->status asynchronously via a DMA write into a cache-coherent line. Without
 * `volatile`, the compiler is entitled to read d->status ONCE and reuse the
 * register value on every loop iteration — an infinite spin on a stale 'not
 * done' value. `volatile` forces the load to actually execute each call.
 * --------------------------------------------------------------------------- */
static inline u32 read_status(const struct desc *d)
{
    return *(const volatile u32 *)&d->status;
}

/* ---------------------------------------------------------------------------
 * load_load_barrier — order the DD-check load before later loads.
 *
 * Emits NO instruction. It tells the COMPILER not to move the subsequent reads
 * of d->length above the DD check. On x86-64's TSO memory model the hardware
 * never reorders load-after-load, so no fence instruction is needed; a compiler
 * barrier is sufficient. On a weakly-ordered ISA (ARM/POWER) you would instead
 * need a real acquire load / dmb here, or the CPU could read length before DD.
 * --------------------------------------------------------------------------- */
static inline void load_load_barrier(void)
{
    __asm__ volatile ("" ::: "memory");
}

/* ---------------------------------------------------------------------------
 * rx_poll — the receive hot loop, distilled.
 *
 * Walk the ring from *rx_index. For each descriptor the NIC has completed (DD
 * set), accumulate its length and advance; stop at the first not-done slot or
 * after `budget` packets. Returns the number of completed descriptors and
 * writes the new ring position back through rx_index.
 *
 * This is exactly ixgbe_rx_batch minus the buffer bookkeeping, so its assembly
 * shows the essential pattern: volatile status load, branch on DD, barrier,
 * length load, masked index advance.
 * --------------------------------------------------------------------------- */
u32 rx_poll(struct desc *ring, u16 size, u16 *rx_index, u16 budget,
            u32 *bytes_out)
{
    u16 idx   = *rx_index;
    u32 done  = 0;
    u32 bytes = 0;

    while (done < budget) {
        u32 status = read_status(&ring[idx]);   /* forced volatile load       */
        if (!(status & RXD_STAT_DD))
            break;                               /* NIC hasn't filled this slot */

        load_load_barrier();                     /* DD seen -> now length valid */
        bytes += ring[idx].length;               /* trust length only after DD  */

        idx = ring_next(idx, size);              /* masked wrap advance         */
        done++;
    }

    *rx_index  = idx;
    *bytes_out = bytes;
    return done;
}

/* ---------------------------------------------------------------------------
 * tx_clean — reclaim finished TX descriptors in fixed batches.
 *
 * Only every `batch`-th descriptor carries the Report-Status bit, so we test the
 * DD bit on the last descriptor of each group; if set, the whole group is done.
 * Returns the number of descriptors reclaimed and advances *clean_index. Same
 * volatile-load discipline as RX. Demonstrates the (cur - clean) & mask distance
 * arithmetic that a ring buffer uses to measure outstanding entries.
 * --------------------------------------------------------------------------- */
u32 tx_clean(struct desc *ring, u16 size, u16 *clean_index, u16 tx_index,
             u16 batch)
{
    u16 clean  = *clean_index;
    u16 mask   = (u16)(size - 1);
    u32 freed  = 0;

    for (;;) {
        u16 outstanding = (u16)((tx_index - clean) & mask);
        if (outstanding < batch)
            break;                               /* not a full group yet       */

        u16 check = (u16)((clean + batch - 1) & mask); /* the RS-marked desc   */
        u32 status = read_status(&ring[check]);  /* forced volatile load       */
        if (!(status & ADVTXD_STAT_DD))
            break;                               /* group not sent yet         */

        clean = (u16)((clean + batch) & mask);   /* whole group reclaimed      */
        freed += batch;
    }

    *clean_index = clean;
    return freed;
}
