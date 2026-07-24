/* ===========================================================================
 * demo.c — the KV store's pure-logic core, extracted for annotated assembly.
 * ===========================================================================
 *
 * This file is DELIBERATELY self-contained: no #includes, its own integer
 * types, so `clang --target=x86_64-pc-linux-gnu -S` turns it into clean
 * Linux/SysV assembly with nothing from libc in the way. The full store
 * (../btree.c, ../pager.c, ../wal.c) can't be compiled to standalone asm — it
 * needs <unistd.h>/<fcntl.h> for pread/pwrite/fdatasync — so we lift out the
 * routines that are 100% register-and-pointer math and also the most
 * instructive to read as machine code:
 *
 *   1. align_up          — the "add (a-1), mask" trick that rounds an I/O buffer
 *                          or offset up to an O_DIRECT block boundary (no divide).
 *   2. le16 / le32       — the little-endian byte codecs: how bytes on disk
 *                          become an integer with shifts and ORs (endianness).
 *   3. key_cmp           — unsigned lexicographic key comparison (the memcmp-like
 *                          loop with a length tiebreak) that orders the tree.
 *   4. slot_lower_bound  — THE in-page binary search: over the sorted slot array
 *                          of a slotted page, doing the slot arithmetic
 *                          (NODE_HDR + i*2 -> cell offset -> key at +6) that
 *                          every get/put/del performs. This is the star routine.
 *   5. crc32_byte        — one byte of the reflected CRC32 (poly 0xEDB88320):
 *                          eight shift-and-XOR steps, the math the page/WAL
 *                          checksums are built from.
 *
 * These mirror the same-named logic in the real sources exactly, minus syscalls.
 * ===========================================================================
 */

/* Our own fixed-width types (LP64: int is 32-bit, long/pointer 64-bit). */
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef unsigned long  usize;

/* On-page geometry, identical to db.h. */
#define NODE_HDR  16u   /* header bytes before the slot array          */
#define SLOT_SZ    2u   /* each slot is one little-endian u16          */

/* ---- 1. alignment rounding (O_DIRECT / mmap) ------------------------------
 * Round n up to the next multiple of a (a MUST be a power of two). Adding (a-1)
 * pushes past the boundary, then masking with ~(a-1) clears the low bits back
 * down to the multiple. O_DIRECT requires the buffer address, the file offset,
 * and the length all be multiples of the device block size — this is how you
 * satisfy that with two arithmetic ops and no division. */
usize align_up(usize n, usize a)
{
    return (n + (a - 1)) & ~(a - 1);
}

/* ---- 2. little-endian decoders --------------------------------------------
 * The database file is little-endian regardless of host. We reassemble an
 * integer from its bytes by hand (never *(u32*)p): this is endian-correct
 * everywhere and needs no alignment — a cell may start at any byte offset. */
u16 le16(const u8 *p)
{
    return (u16)((u16)p[0] | ((u16)p[1] << 8));
}
u32 le32(const u8 *p)
{
    return (u32)p[0]        | ((u32)p[1] << 8)
         | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

/* ---- 3. key ordering ------------------------------------------------------
 * Unsigned lexicographic compare with "shorter sorts first" on a shared prefix
 * (memcmp, then length as the tiebreak). Bytes are compared UNSIGNED so binary
 * keys sort as raw bytes. Returns <0 / 0 / >0. */
int key_cmp(const u8 *a, u16 alen, const u8 *b, u16 blen)
{
    u16 m = alen < blen ? alen : blen;      /* compare the shared prefix length */
    for (u16 i = 0; i < m; i++) {
        if (a[i] != b[i])
            return a[i] < b[i] ? -1 : 1;    /* first differing byte decides     */
    }
    if (alen != blen)
        return alen < blen ? -1 : 1;        /* prefix match -> shorter is less  */
    return 0;                                /* identical                         */
}

/* ---- 4. the in-page binary search (THE routine) ---------------------------
 * A slotted leaf page stores `nslots` cells, indexed by a SORTED array of u16
 * offsets ("slots") that begins at byte NODE_HDR. This returns the LOWER BOUND:
 * the index of the first slot whose key is >= the search key (== the insertion
 * point, and the candidate for an exact-match check). Every lookup, insert, and
 * delete starts here.
 *
 * The slot arithmetic, spelled out:
 *   slot i is the u16 at   page + NODE_HDR + i*SLOT_SZ
 *   its value `off` is the cell's byte offset within the page
 *   a leaf cell is [u16 key_len][u32 val_len][key...][val...], so
 *     key_len = le16(page + off)   and   key bytes start at page + off + 6.
 * Watch this become a tight loop with a `bsr`-free integer midpoint and two
 * nested loads in the annotated assembly. */
int slot_lower_bound(const u8 *page, u16 nslots, const u8 *key, u16 klen)
{
    int lo = 0, hi = (int)nslots;
    while (lo < hi) {
        int mid = (lo + hi) >> 1;                       /* midpoint (no overflow: page-bounded) */
        u16 off = le16(page + NODE_HDR + (unsigned)mid * SLOT_SZ);  /* slot -> cell offset */
        u16 ml  = le16(page + off);                     /* that cell's key_len              */
        const u8 *mk = page + off + 6;                  /* that cell's key bytes            */
        if (key_cmp(mk, ml, key, klen) < 0)
            lo = mid + 1;                               /* mid's key < target: go right     */
        else
            hi = mid;                                   /* mid's key >= target: keep, go left*/
    }
    return lo;
}

/* ---- 5. one byte of CRC32 (page/WAL integrity) ----------------------------
 * The reflected CRC32 (polynomial 0xEDB88320) folded one input byte into the
 * running remainder `crc`: XOR the byte into the low bits, then do eight
 * shift-right / conditional-XOR steps (polynomial division over GF(2), LSB
 * first). The production code hoists these 8 steps into a 256-entry table; this
 * is the un-tabled math the table caches, and it makes the shift/XOR/branch
 * structure obvious in the assembly. */
u32 crc32_byte(u32 crc, u8 byte)
{
    crc ^= byte;                                        /* mix the byte into the low 8 bits */
    for (int k = 0; k < 8; k++) {
        if (crc & 1u)                                   /* bit shifted out is 1 ->          */
            crc = 0xEDB88320u ^ (crc >> 1);             /*   shift and XOR the generator    */
        else
            crc = crc >> 1;                             /* bit is 0 -> just shift           */
    }
    return crc;
}
