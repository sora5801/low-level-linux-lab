/* ===========================================================================
 * demo.c — the pure-arithmetic HEART of ramblk, carved out to compile alone.
 * ===========================================================================
 *
 * WHY THIS FILE EXISTS
 * --------------------
 * The lab requires every C project to ship annotated assembly. But ramblk.c is
 * a kernel module: it #includes <linux/blk-mq.h> and friends, which only exist
 * inside a configured kernel tree, so `clang -S ramblk.c` cannot run on a normal
 * host. So we extract the ONE piece of ramblk that is pure logic — the
 * sector<->byte conversion and the overflow-safe bounds check from
 * ramblk_transfer() — into this self-contained file that depends on nothing.
 * It declares its own fixed-width types; there is not a single #include. The
 * assembly clang emits for these functions is therefore real, host-portable,
 * and shows exactly the arithmetic the kernel hot path performs per segment.
 *
 * THE CONCEPT BEING TAUGHT
 * ------------------------
 * A block device speaks in 512-byte SECTORS (LBAs). Memory is byte-addressed.
 * So the driver constantly converts: byte_offset = sector << 9. And before it
 * memcpy's into a fixed-size backing store it must prove the transfer fits —
 * WITHOUT letting `offset + length` overflow a 64-bit value, which would let a
 * malicious/corrupt request slip past a naive check and corrupt kernel memory.
 * The assembly makes both the cheap shift and the careful compare visible.
 * =========================================================================== */

/* Our own types — no <stdint.h>, no kernel headers. On the LP64 model Linux
 * uses for x86-64, these widths are exact. */
typedef unsigned long long u64;   /* 64-bit: a byte offset or a sector index */
typedef unsigned int       u32;   /* 32-bit: a single segment's length       */

/* The block layer's fixed sector geometry. SECTOR_SHIFT is 9 everywhere and
 * forever: it is baked into what an LBA MEANS. 1 << 9 == 512. */
#define SECTOR_SHIFT 9u
#define SECTOR_SIZE  (1u << SECTOR_SHIFT)   /* 512 */

/* ---------------------------------------------------------------------------
 * sector_to_bytes — the conversion every block driver does constantly.
 * A left shift by 9, nothing more. Isolated here so you can see it compile to
 * a single `shl`/`lea` with no multiply. Returns byte offset of `sector`.
 * ------------------------------------------------------------------------- */
u64 sector_to_bytes(u64 sector)
{
    return sector << SECTOR_SHIFT;
}

/* ---------------------------------------------------------------------------
 * ramblk_range_ok — the bounds check from ramblk_transfer(), verbatim in
 * spirit. Given a start `sector`, a segment length `nbytes`, and the total
 * `store_bytes`, decide whether [offset, offset+nbytes) fits, and if so write
 * the byte offset through `out_off`.
 *
 * THE SUBTLE PART (this is the whole reason the function exists):
 * The naive test `off + nbytes > store_bytes` is WRONG. If `off` is near the
 * top of the u64 range (a corrupt request), `off + nbytes` wraps around past
 * zero and compares as SMALL, so the check passes and the caller memcpy's out
 * of bounds. We instead:
 *     1. prove  off <= store_bytes           (off itself is in range)
 *     2. then   nbytes <= store_bytes - off  (subtraction cannot underflow now)
 * Neither step can overflow, so a wrapping offset is rejected, not accepted.
 * Returns 1 if the range is OK (and *out_off is set), 0 otherwise.
 * ------------------------------------------------------------------------- */
int ramblk_range_ok(u64 sector, u32 nbytes, u64 store_bytes, u64 *out_off)
{
    u64 off = sector << SECTOR_SHIFT;       /* same conversion as above       */

    if (off > store_bytes)                  /* step 1: offset past the end?    */
        return 0;
    if ((u64)nbytes > store_bytes - off)    /* step 2: no wrap; length fits?   */
        return 0;

    *out_off = off;                         /* commit the validated offset     */
    return 1;
}

/* ---------------------------------------------------------------------------
 * ramblk_advance — model the per-segment cursor update in the copy loop.
 * After copying `nbytes` we advance the running byte position. Trivial, but it
 * shows how the loop in ramblk_transfer walks the store with a plain add. It
 * returns the new position so the value is observable in the assembly (a dead
 * store would be optimized away, hiding the lesson).
 * ------------------------------------------------------------------------- */
u64 ramblk_advance(u64 pos, u32 nbytes)
{
    return pos + nbytes;
}
