/* ===========================================================================
 * ip.h — IPv4: parse/validate inbound datagrams, build/checksum outbound ones.
 * ===========================================================================
 * IP is the "put these bytes on that host" layer. It carries a protocol number
 * that selects the transport handler (ICMP/UDP/TCP) and a header checksum that
 * protects the addressing fields against corruption in transit.
 * ========================================================================= */
#ifndef USERSPACE_TCPIP_IP_H
#define USERSPACE_TCPIP_IP_H

#include "netif.h"

/* Receive path: an IPv4 packet (Ethernet header already stripped). */
void ip_input(struct netif *nif, u8 *packet, size_t len);

/* Send path: wrap `payload` (`len` bytes, already a full ICMP/UDP/TCP message)
 * in an IPv4 header addressed to `dst_ip` (NETWORK order) with protocol
 * `proto`, resolve the next-hop MAC via ARP, and transmit. Returns 0, or -1 on
 * error / ARP-miss-drop. */
int ip_output(struct netif *nif, u32 dst_ip, u8 proto,
               const void *payload, size_t len);

#endif /* USERSPACE_TCPIP_IP_H */
