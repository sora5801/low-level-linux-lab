/* ===========================================================================
 * crc32.c — table-driven CRC32 (reflected IEEE 802.3 / zlib polynomial).
 * ===========================================================================
 *
 * WHY A CHECKSUM AT ALL
 * ---------------------
 * A disk (or the page cache, or a flaky cable) can hand us back a page that was
 * only *partially* written before a crash — a "torn write". The bytes look like
 * a page but they are a Frankenstein of old and new. If we trusted them we would
 * navigate the B-tree off a cliff. So every page stores a CRC32 over its own
 * contents; on read we recompute and compare (pager.c: page_verify). A mismatch
 * means "this page is not what we wrote" and we refuse it.
 *
 * WHY THIS PARTICULAR CRC
 * -----------------------
 * We use the *reflected* CRC-32 with polynomial 0xEDB88320 (the bit-reversed
 * form of 0x04C11DB7). This is the exact variant used by zlib, gzip, PNG, and
 * Ethernet, so the numbers we store are the same ones `crc32`/`gzip` would
 * produce — you can cross-check a page with standard tooling. "Reflected" means
 * we process each byte least-significant-bit first, which is why the table is
 * built by shifting *right* and the update loop shifts *right*.
 *
 * THE MATH, BRIEFLY
 * -----------------
 * CRC treats the message as a big polynomial over GF(2) (coefficients are bits,
 * add == XOR, no carries) and returns the remainder after dividing by the
 * generator polynomial. Bit-at-a-time that is one shift + conditional XOR per
 * bit. The classic speedup precomputes, for each possible byte, the effect of
 * running those 8 bits through the division — that is `table[]` below — turning
 * the inner loop into one table lookup and one XOR per *byte*.
 * ===========================================================================
 */
#include "db.h"

/* The 256-entry lookup table. Built once, lazily, on first use. `table_ready`
 * is a plain int: this library is single-threaded per DB handle, so there is no
 * data race to protect against here (a multi-threaded build would compute the
 * table at load time or guard it with a once-flag). */
static uint32_t table[256];
static int      table_ready = 0;

/* Build table[b] = the CRC state after feeding byte value b into an all-zero
 * running remainder. For a reflected CRC we shift the 32-bit remainder toward
 * the least-significant end and, whenever the bit we shifted out was set, XOR in
 * the (already reflected) polynomial 0xEDB88320. Eight iterations = one byte. */
static void crc32_build_table(void)
{
    for (uint32_t b = 0; b < 256; b++) {
        uint32_t c = b;                        /* start with the byte in the low 8 bits */
        for (int k = 0; k < 8; k++) {          /* process 8 bits, LSB first             */
            if (c & 1u)                        /* bit shifted out is 1 ->                */
                c = 0xEDB88320u ^ (c >> 1);    /*   divide: shift and XOR the generator  */
            else
                c = c >> 1;                    /* bit is 0 -> just shift                 */
        }
        table[b] = c;                          /* remainder contributed by this byte     */
    }
    table_ready = 1;
}

/* Seed. A reflected CRC starts life as all-ones (0xFFFFFFFF). Pre-inverting the
 * register is what lets the CRC detect leading zero bytes being added/removed. */
uint32_t crc32_init(void)
{
    if (!table_ready) crc32_build_table();
    return 0xFFFFFFFFu;
}

/* Fold `n` bytes of `buf` into the running CRC `crc`.
 *   idx = (crc XOR next_byte) & 0xFF   -> which table row this byte selects
 *   crc = table[idx] XOR (crc >> 8)    -> mix the row in and shift the byte out
 * This is the table-driven restatement of "8 shift-and-XOR steps per byte". */
uint32_t crc32_update(uint32_t crc, const void *buf, size_t n)
{
    const uint8_t *p = (const uint8_t *)buf;
    if (!table_ready) crc32_build_table();     /* safe even if init() wasn't called first */
    for (size_t i = 0; i < n; i++) {
        uint8_t idx = (uint8_t)(crc ^ p[i]);   /* low byte of crc combined with the input */
        crc = table[idx] ^ (crc >> 8);         /* consume one input byte                  */
    }
    return crc;
}

/* Final inversion, mirroring the all-ones seed. The stored/compared value is
 * always the post-XOR result. */
uint32_t crc32_final(uint32_t crc)
{
    return crc ^ 0xFFFFFFFFu;
}

/* Convenience one-shot: init -> update -> final. */
uint32_t crc32(const void *buf, size_t n)
{
    return crc32_final(crc32_update(crc32_init(), buf, n));
}
