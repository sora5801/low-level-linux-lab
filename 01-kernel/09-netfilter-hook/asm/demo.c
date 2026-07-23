/* ===========================================================================
 * demo.c — the Internet Checksum (RFC 1071), extracted for assembly study.
 * ===========================================================================
 *
 * WHY THIS FILE EXISTS
 * --------------------
 * The netfilter module in ../netfilter_hook.c cannot be compiled to standalone
 * assembly on a non-Linux host: it pulls in hundreds of kernel headers and only
 * links inside the kernel. But the repo's rule is that every C project ships
 * real, hand-annotated assembly. So we lift out the single most instructive
 * pure-logic routine and compile THAT on its own.
 *
 * The natural pick for a packet-filter project is the *IP header checksum* —
 * the ones-complement sum defined in RFC 1071. Every IPv4 header carries one;
 * the kernel verifies it on receive (ip_rcv -> ip_fast_csum) and recomputes it
 * whenever a field like TTL changes. It is a tiny loop of adds, a carry fold,
 * and a bitwise NOT — perfect for reading in assembly, and it touches no kernel
 * types at all, so it is completely self-contained here.
 *
 * THE ALGORITHM (RFC 1071, "the ones' complement sum")
 * ----------------------------------------------------
 *   1. Treat the data as a sequence of 16-bit words.
 *   2. Sum them all into a wider (32-bit) accumulator so carries aren't lost.
 *   3. "Fold" the accumulator: keep adding the high 16 bits back into the low
 *      16 bits until nothing is left up top. This is what makes the sum a
 *      *ones-complement* (end-around-carry) sum rather than a plain one.
 *   4. The checksum is the bitwise complement (~) of the folded 16-bit value.
 *
 * A neat property, exploited by the verify path: if you run the SAME sum over a
 * header that already contains a correct checksum, the words plus their own
 * complement add up to all-ones, so the folded result is 0xFFFF and ~0xFFFF==0.
 * "Valid header" therefore reduces to "the ones-complement sum is zero."
 *
 * This file declares its own fixed-width types and includes NO headers, so it
 * compiles anywhere clang runs — which is exactly why we can generate the .s
 * files for it on this Windows host.
 * ===========================================================================
 */

/* Our own fixed-width integer types. On every target this repo cares about,
 * short is 16-bit and int is 32-bit; spelling them out keeps the file header-
 * free and makes the widths (which the assembly makes visible) explicit. */
typedef unsigned short u16;   /* a 16-bit word — the checksum's unit           */
typedef unsigned int   u32;   /* a 32-bit accumulator — holds the carries      */

/* ---------------------------------------------------------------------------
 * csum_fold — collapse a 32-bit ones-complement accumulator down to 16 bits.
 *
 * Adding the high half back into the low half can itself produce a carry (e.g.
 * 0xFFFF + 0xFFFF = 0x1FFFE), so we loop until the top 16 bits are clear. In
 * practice at most two iterations ever run, but the `while` states the invariant
 * exactly: "no carry may be left above bit 15." Marked static so the optimizer
 * can inline it into the callers, which is worth watching for in the -O2 asm.
 * --------------------------------------------------------------------------- */
static u16 csum_fold(u32 sum)
{
	while (sum >> 16)                 /* while any carry sits in bits 16..31   */
		sum = (sum & 0xffff) + (sum >> 16);  /* end-around carry: fold it in  */
	return (u16)sum;                 /* the low 16 bits are now the final sum */
}

/* ---------------------------------------------------------------------------
 * ip_checksum — compute the RFC 1071 checksum over `len` bytes at `data`.
 *
 * Returns the 16-bit value to store in a header's checksum field. `data` is
 * byte-addressed (const void *) because a real IP header is not guaranteed to
 * be 16-bit aligned in memory; we read it a byte-pair at a time to stay
 * alignment-safe, which the assembly shows as two byte loads + a shift-or.
 * --------------------------------------------------------------------------- */
u16 ip_checksum(const void *data, u32 len)
{
	const unsigned char *p = data;   /* byte cursor over the buffer            */
	u32 sum = 0;                      /* wide accumulator: carries live here    */

	/* Consume full 16-bit words while at least two bytes remain. We assemble
	 * each word from two bytes in a fixed order (low byte first) so the result
	 * is independent of the host's endianness: the ones-complement sum has the
	 * property that byte-swapping every word swaps the sum's bytes too, and the
	 * final complement preserves that — so a big- and little-endian machine
	 * compute checksums that are byte-swaps of each other and both verify. */
	while (len > 1) {
		u16 word = (u16)(p[0] | (p[1] << 8));  /* one 16-bit word            */
		sum += word;                           /* accumulate (no carry lost) */
		p   += 2;
		len -= 2;
	}

	/* A trailing odd byte (headers are always even, but data payloads may not
	 * be) is padded with an implicit zero high byte and added in. */
	if (len == 1)
		sum += p[0];

	/* Fold the carries down and take the ones' complement — that complement is
	 * the checksum you write into the packet. */
	return (u16)~csum_fold(sum);
}

/* ---------------------------------------------------------------------------
 * ip_checksum_valid — verify a header that ALREADY contains its checksum.
 *
 * Because the stored checksum is the complement of the sum of the other words,
 * summing everything (checksum field included) yields 0xFFFF, whose fold-and-
 * complement is 0. So "valid" == "ip_checksum over the whole thing is 0". This
 * is exactly the test ip_rcv() applies to every arriving packet. Returns 1 for
 * a good header, 0 for a corrupted one.
 * --------------------------------------------------------------------------- */
int ip_checksum_valid(const void *data, u32 len)
{
	return ip_checksum(data, len) == 0;
}
