/* ===========================================================================
 * demo.c — the TCP pseudo-header checksum, extracted for annotated assembly.
 * ===========================================================================
 *
 * This file is DELIBERATELY self-contained: it includes NO system headers and
 * declares its own fixed-width types, so
 *     clang --target=x86_64-pc-linux-gnu -S
 * turns it into clean Linux/SysV assembly with nothing from libc in the way.
 *
 * The real scanner (../synscan.c) cannot be compiled to asm standalone because
 * it needs <sys/socket.h> and the netinet headers. So we lift out the one routine
 * that is 100% register-and-pointer math AND is the heart of the project: the
 * Internet (ones-complement) checksum, computed over the TCP *pseudo-header*
 * plus the TCP segment. This is exactly what synscan.c's tcp_checksum() does,
 * minus the socket plumbing, so the assembly you read here is the assembly that
 * runs in the scanner.
 *
 * WHY A "PSEUDO-HEADER"?
 * ---------------------
 * TCP's checksum must protect not just the TCP segment but also the IP addresses
 * and protocol it was sent under, so a datagram delivered to the wrong host or
 * demuxed to the wrong protocol is detected. But those fields live in the IP
 * header, not the TCP header. RFC 793's fix: for checksum purposes ONLY, prepend
 * a 12-byte "pseudo-header" (src IP, dst IP, a zero byte, the protocol number,
 * and the TCP length) to the segment, sum over the whole thing, and throw the
 * pseudo-header away. It is never transmitted — it is a shared fiction the sender
 * and receiver both construct so their checksums agree.
 *
 *   pseudo-header (12 bytes, all fields in NETWORK byte order):
 *     0  +--------+--------+--------+--------+
 *        |            source IPv4            |   4 bytes
 *     4  +--------+--------+--------+--------+
 *        |         destination IPv4          |   4 bytes
 *     8  +--------+--------+--------+--------+
 *        |  zero  |  proto |   TCP length    |   1 + 1 + 2 bytes  (proto=6)
 *    10  +--------+--------+-----------------+
 *
 * WHY NETWORK BYTE ORDER (big-endian)?
 * ------------------------------------
 * IP was standardised on big-endian ("network byte order") so that hosts of any
 * native endianness interpret a multi-byte field identically on the wire. Every
 * multi-byte field in a real IP/TCP header is therefore stored big-endian in
 * memory. This checksum reads the bytes back as big-endian 16-bit words
 * (buf[0]<<8 | buf[1]), so the arithmetic below is INDEPENDENT of the host's
 * endianness: it works bit-for-bit on x86 (little-endian) and on a big-endian
 * box alike. That is the whole point of doing the byte assembly by hand instead
 * of casting to `unsigned short *` and hoping the host matches the wire.
 * ===========================================================================
 */

/* Our own fixed-width types. LP64 (Linux x86-64): int is 32-bit, long is 64. */
typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;

#define IPPROTO_TCP  6u        /* the "protocol" byte carried in the pseudo-header */

/* ---------------------------------------------------------------------------
 * The pseudo-header, laid out to match the wire exactly. 4+4+1+1+2 = 12 bytes
 * with NO padding: every field is already at its natural alignment (the u16 at
 * offset 10 is 2-aligned, the struct's own alignment is 4, size is a multiple of
 * 4), so sizeof(struct pseudo_header) == 12 and we can safely checksum it as a
 * flat byte array. All fields hold values ALREADY in network byte order, i.e.
 * exactly the bytes that sit in the real IP header, so summing them here matches
 * what the receiver reconstructs from the datagram it got.
 * --------------------------------------------------------------------------- */
struct pseudo_header {
    u32 src_addr;    /* offset 0 : source IPv4 address,      network order  */
    u32 dst_addr;    /* offset 4 : destination IPv4 address, network order  */
    u8  zero;        /* offset 8 : always 0 (pads proto to a 16-bit word)   */
    u8  protocol;    /* offset 9 : 6 = IPPROTO_TCP                          */
    u16 tcp_length;  /* offset 10: TCP header + payload length, network order*/
};

/* ---------------------------------------------------------------------------
 * 1. sum16 — accumulate the big-endian 16-bit words of `buf` into a 32-bit sum.
 *
 * The Internet checksum is a *ones-complement* sum: we add 16-bit words in a
 * wider (32-bit) accumulator so carries pile up in the high half instead of
 * being lost, and fold them back in later (see fold_csum). We read each word as
 * (buf[0]<<8 | buf[1]) — the big-endian interpretation — so the result does not
 * depend on this CPU's endianness (see the header comment).
 *
 * `len` need not be even: a trailing odd byte is the high half of a final word
 * whose low half is an implicit zero, i.e. (buf[0] << 8). RFC 1071 specifies
 * exactly this zero-padding for odd-length data.
 *
 * The caller threads `sum` through multiple calls (pseudo-header, then segment)
 * so the two contributions accumulate into one running total.
 * --------------------------------------------------------------------------- */
u32 sum16(const u8 *buf, u32 len, u32 sum)
{
    /* Consume whole 16-bit words while at least two bytes remain. */
    while (len > 1) {
        sum += (u32)((u32)buf[0] << 8 | (u32)buf[1]); /* big-endian word    */
        buf += 2;                                     /* advance two bytes   */
        len -= 2;                                      /* two fewer to go     */
    }
    /* One leftover byte: pad its low half with zero, per RFC 1071. */
    if (len == 1)
        sum += (u32)((u32)buf[0] << 8);
    return sum;
}

/* ---------------------------------------------------------------------------
 * 2. fold_csum — collapse the 32-bit accumulator to the final 16-bit checksum.
 *
 * "End-around carry": repeatedly add the high 16 bits back into the low 16 bits
 * until the high half is zero. One add can itself produce a carry (e.g. summing
 * to 0x1FFFF), so we loop; in practice at most two iterations run. Then take the
 * ONES-COMPLEMENT (~): the transmitted checksum is the bitwise NOT of the folded
 * sum, chosen so the receiver's sum over segment + checksum comes to 0xFFFF,
 * whose complement is 0 — the "checksum OK" sentinel.
 * --------------------------------------------------------------------------- */
u16 fold_csum(u32 sum)
{
    while (sum >> 16)                    /* while any carry bits remain high... */
        sum = (sum & 0xFFFFu) + (sum >> 16);   /* ...fold them into the low 16 */
    return (u16)~sum;                    /* ones-complement -> the wire value   */
}

/* ---------------------------------------------------------------------------
 * 3. tcp_checksum — the star routine: checksum a TCP segment WITH its pseudo-hdr.
 *
 * Steps, in the order a sender performs them:
 *   (a) zero the checksum field inside `segment` before calling (the caller does
 *       this; a field summed as part of its own checksum would be circular),
 *   (b) sum the 12-byte pseudo-header,
 *   (c) continue the same running sum over the `seg_len`-byte TCP segment,
 *   (d) fold + complement.
 *
 * The value returned is the "logical" 16-bit checksum; the caller stores it into
 * the TCP header's 16-bit checksum field via htons() so the bytes land big-endian
 * on the wire. (Equivalently one may sum native-endian words and store the raw
 * result — the ones-complement sum is symmetric under byte-swapping — but doing
 * it byte-explicitly here keeps the endianness reasoning airtight.)
 * --------------------------------------------------------------------------- */
u16 tcp_checksum(const struct pseudo_header *ph, const u8 *segment, u32 seg_len)
{
    u32 sum = 0;
    sum = sum16((const u8 *)ph, (u32)sizeof(*ph), sum); /* (b) 12-byte pseudo-hdr */
    sum = sum16(segment, seg_len, sum);                 /* (c) the TCP segment    */
    return fold_csum(sum);                              /* (d) fold + complement  */
}
