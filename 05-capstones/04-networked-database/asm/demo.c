/* ===========================================================================
 * asm/demo.c — the WAL/log record framing + CRC32, distilled and freestanding.
 * ===========================================================================
 *
 * This is the single most representative pure-logic routine of the whole node:
 * turning a key/value mutation into a self-describing, checksummed, length-framed
 * record — the exact operation that both store.wal (../src/wal.c) and the peer
 * transport (../src/server.c) perform on every write. It is written to become
 * assembly, so it obeys the lab's freestanding rules:
 *
 *   - NO #include, NO system headers  -> clang emits clean teaching asm, no libc.
 *   - its own integer types            -> depends on nothing.
 *   - a table-FREE CRC32 inner loop    -> the shift/xor bit math is visible in
 *                                         the emitted code (../src/crc32.c uses a
 *                                         256-entry table for speed; here we want
 *                                         to SEE the polynomial division).
 *
 * Generate the three optimization levels (these are the committed .s files):
 *
 *   clang --target=x86_64-pc-linux-gnu -S -O0 -fno-asynchronous-unwind-tables \
 *         -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
 *   clang --target=x86_64-pc-linux-gnu -S -O1 -fno-asynchronous-unwind-tables \
 *         -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
 *   clang --target=x86_64-pc-linux-gnu -S -O2 -fno-asynchronous-unwind-tables \
 *         -fno-jump-tables asm/demo.c -o asm/demo.O2.s
 *
 * demo.annotated.s is hand-written from demo.s (-O1). Two things to watch there:
 *   1. crc32_ieee's inner loop is a genuine polynomial long-division: the
 *      "if low bit set, xor the reflected polynomial" becomes a branchless
 *      shift + conditional-xor idiom.
 *   2. wal_frame_record writes multi-byte lengths ONE BYTE AT A TIME (put_u32),
 *      which is what portable little-endian serialization looks like as machine
 *      code — no assumption about the host's byte order survives.
 * ===========================================================================
 */

/* --- freestanding fixed-width types (LP64: unsigned long is 64-bit here) --- */
typedef unsigned char      u8;
typedef unsigned int       u32;
typedef unsigned long      u64;
typedef unsigned long      usize;

/* Opcodes, mirroring enum cmd_op in ../src/db.h. */
#define OP_PUT 1u
#define OP_DEL 2u

/* ---------------------------------------------------------------------------
 * put_u32 — store a 32-bit value little-endian (low byte first).
 *
 * We NEVER memcpy an integer to a buffer that will be read on another machine:
 * that bakes in the writer's byte order. Spelling out each byte makes the format
 * host-independent. In the asm this becomes four byte-stores with shifts.
 * ------------------------------------------------------------------------- */
static inline void put_u32(u8 *p, u32 v)
{
    p[0] = (u8)(v);          /* bits 0..7   */
    p[1] = (u8)(v >> 8);     /* bits 8..15  */
    p[2] = (u8)(v >> 16);    /* bits 16..23 */
    p[3] = (u8)(v >> 24);    /* bits 24..31 */
}

/* Minimal byte copy (no <string.h> in a freestanding unit). */
static inline void copy_bytes(u8 *dst, const u8 *src, u32 n)
{
    for (u32 i = 0; i < n; i++) dst[i] = src[i];
}

/* ---------------------------------------------------------------------------
 * crc32_ieee — CRC-32/IEEE, bit-at-a-time (table-free) so the math shows in asm.
 *
 * Reflected form: init 0xFFFFFFFF, polynomial 0xEDB88320 (the bit-reversed
 * 0x04C11DB7), final invert. For each byte we xor it into the low 8 bits of the
 * register, then for each of its 8 bits: shift the register right one, and — iff
 * the bit we shifted out was 1 — xor in the polynomial. That "shift then maybe
 * xor" IS binary polynomial long division; the CRC is the remainder. The final
 * xor with all-ones is what lets the CRC notice leading/trailing zero runs.
 * ------------------------------------------------------------------------- */
u32 crc32_ieee(const u8 *data, usize len)
{
    u32 crc = 0xFFFFFFFFu;                 /* initial remainder = all ones       */
    for (usize i = 0; i < len; i++) {
        crc ^= data[i];                    /* fold the next message byte in      */
        for (int k = 0; k < 8; k++)        /* process its 8 bits, LSB first      */
            crc = (crc >> 1) ^ (0xEDB88320u & (~(crc & 1u) + 1u));
        /* The mask trick: (crc & 1) is 0 or 1; negate two's-complement to get
         * 0x00000000 or 0xFFFFFFFF; AND with the polynomial. So we xor the
         * polynomial exactly when the low bit was set — branchless, which is how
         * the optimizer wants to emit it (watch for the `neg`/`and` in the asm). */
    }
    return crc ^ 0xFFFFFFFFu;              /* final invert                        */
}

/* ---------------------------------------------------------------------------
 * wal_frame_record — serialize one KV mutation into a framed, checksummed record.
 *
 * Output layout (identical to ../src/wal.c):
 *
 *     [u32 reclen][u32 crc32][ u8 op | u32 klen | key | u32 vlen | val ]
 *      \____ 8-byte header ____/ \_____________ body (reclen bytes) ______/
 *
 *   reclen = length of the body.
 *   crc32  = CRC-32/IEEE over the body (NOT the header) — see crc32_ieee.
 *
 * `out` must have room for 8 + body bytes. Returns the total record length. The
 * caller (a WAL) then write()s exactly that many bytes and fsync()s. On replay,
 * a reader takes reclen, reads that many body bytes, and recomputes the CRC: a
 * torn write fails either the length read or the CRC and is discarded.
 *
 * SysV AMD64 ABI: out=%rdi, op=%esi, key=%rdx, klen=%ecx, val=%r8, vlen=%r9d;
 * the 64-bit result (record length) comes back in %rax.
 * ------------------------------------------------------------------------- */
u64 wal_frame_record(u8 *out, u8 op, const u8 *key, u32 klen,
                     const u8 *val, u32 vlen)
{
    /* A delete carries no value, whatever the caller passed. */
    if (op == OP_DEL) vlen = 0;

    /* Build the body starting 8 bytes in, leaving room for the header. */
    u8 *body = out + 8;
    usize n = 0;
    body[n++] = op;                        /* 1 byte: the opcode                 */
    put_u32(body + n, klen); n += 4;       /* 4 bytes: key length (LE)           */
    copy_bytes(body + n, key, klen); n += klen;   /* key bytes                   */
    put_u32(body + n, vlen); n += 4;       /* 4 bytes: value length (LE)         */
    copy_bytes(body + n, val, vlen); n += vlen;   /* value bytes (none for DEL)  */

    /* Header: reclen describes the body; crc protects it. Order matters — we can
     * only checksum the body after it is fully written. */
    put_u32(out, (u32)n);                          /* reclen                     */
    put_u32(out + 4, crc32_ieee(body, n));         /* crc over the body          */

    return (u64)(8 + n);                           /* total bytes in the record  */
}
