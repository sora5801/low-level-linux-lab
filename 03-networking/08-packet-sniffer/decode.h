/* ===========================================================================
 * decode.h — turn a raw captured frame into one human-readable line.
 * ===========================================================================
 *
 * The sniffer hands each captured frame to decode_frame(), which walks the
 * layered headers (Ethernet -> IPv4/IPv6/ARP -> TCP/UDP/ICMP) and prints a
 * tcpdump-style summary. All the "why" — byte offsets, endianness, checksum
 * math — lives in decode.c.
 * ===========================================================================
 */
#ifndef SNIFFER_DECODE_H
#define SNIFFER_DECODE_H

#include <stddef.h>
#include <stdint.h>

/* Decode one link-layer frame and print a summary line to stdout.
 *
 *   data     : pointer to the first byte of the Ethernet header.
 *   caplen   : bytes actually captured (may be < wirelen if snapped short).
 *   wirelen  : the packet's true length on the wire.
 *
 * decode_frame never reads past `caplen` bytes: every layer bounds-checks
 * before dereferencing, because a snapshot length (or a truncated/hostile
 * packet) can end anywhere. Returns nothing — this is a display function. */
void decode_frame(const uint8_t *data, uint32_t caplen, uint32_t wirelen);

#endif /* SNIFFER_DECODE_H */
