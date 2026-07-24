/* ===========================================================================
 * crc32.c — CRC-32/IEEE (the gzip/zlib/Ethernet polynomial), table-driven.
 * ===========================================================================
 *
 * WHY A CHECKSUM AT ALL
 * ---------------------
 * A durable log is only useful if you can tell where it STOPS being valid. When
 * a node crashes mid-append (power loss between the write() and the platter, or
 * a partial fsync), the tail record is torn: some bytes made it, the rest are
 * zero/garbage. On restart we replay the log; without a per-record checksum we
 * would happily decode the torn tail as a real command and corrupt the state
 * machine. The CRC lets recovery say "this record's bytes don't match its CRC ⇒
 * we crashed here ⇒ truncate to the previous record and move on." That single
 * property — detect the torn tail — is why every serious WAL (PostgreSQL,
 * SQLite, RocksDB, the Raft WAL in etcd) checksums each record.
 *
 * WHICH CRC
 * ---------
 * CRC-32/IEEE: reflected input and output, polynomial 0xEDB88320 (the
 * bit-reversed form of 0x04C11DB7), initial value 0xFFFFFFFF, final XOR
 * 0xFFFFFFFF. This is the exact CRC `gzip`, `zlib` (crc32()), and Ethernet FCS
 * use, so our logs are checkable with off-the-shelf tools. It is NOT
 * cryptographic — an adversary can forge a matching CRC trivially — but we are
 * defending against random bit-rot and torn writes, not adversaries.
 *
 * WHY A TABLE
 * -----------
 * The bit-at-a-time CRC does 8 shift/conditional-xor steps per byte. Precomputing
 * the 256-entry table (one entry = the CRC of a single byte with the register
 * empty) collapses each byte to one table lookup + one xor + one shift. The
 * table is built lazily on first use so there is no static initializer to keep in
 * sync. asm/demo.c shows the *table-free* inner loop, so you can see both forms.
 * ===========================================================================
 */
#include "db.h"

/* The 256-entry lookup table and a one-shot "is it built yet?" flag. Single
 * threaded (the whole node is one epoll thread), so no locking is needed to
 * lazily initialize this — a genuine benefit of the reactor design. */
static uint32_t crc_table[256];
static int      crc_table_ready = 0;

/* Build crc_table[b] = CRC32 of the single byte b (register starting empty).
 * For each of the 8 bits: if the low bit is set, shift right and xor the
 * reflected polynomial; else just shift right. This is the reflected (LSB-first)
 * CRC, which is why we shift RIGHT and use 0xEDB88320 rather than shifting left
 * with 0x04C11DB7. */
static void crc32_build_table(void)
{
    for (uint32_t b = 0; b < 256; b++) {
        uint32_t c = b;                       /* seed with the byte value       */
        for (int k = 0; k < 8; k++)           /* process 8 bits                 */
            c = (c & 1u) ? (0xEDB88320u ^ (c >> 1)) /* poly xor on a set low bit */
                         : (c >> 1);               /* else plain shift           */
        crc_table[b] = c;                     /* CRC contribution of this byte  */
    }
    crc_table_ready = 1;
}

/* Public entry point: CRC-32/IEEE over `len` bytes at `data`.
 *
 * The register starts all-ones (init = 0xFFFFFFFF). For each input byte we xor
 * it into the low 8 bits of the register, use that as a table index to fetch the
 * precomputed contribution, and combine with the register shifted right 8. At
 * the end we invert (final xor = 0xFFFFFFFF). Init-ones + final-invert is what
 * makes CRC detect leading/trailing zero-byte errors that a raw CRC would miss. */
uint32_t crc32_ieee(const void *data, size_t len)
{
    if (!crc_table_ready)                     /* lazy one-time table build      */
        crc32_build_table();

    const uint8_t *p = (const uint8_t *)data;
    uint32_t crc = 0xFFFFFFFFu;               /* init: all ones                 */
    for (size_t i = 0; i < len; i++)
        /* index = (current low byte) xor (next input byte); then fold in the
         * high 24 bits by shifting the register right 8. */
        crc = crc_table[(crc ^ p[i]) & 0xFFu] ^ (crc >> 8);
    return crc ^ 0xFFFFFFFFu;                 /* final xor / invert             */
}
