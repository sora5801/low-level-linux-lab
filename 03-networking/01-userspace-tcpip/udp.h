/* ===========================================================================
 * udp.h — UDP: connectionless datagrams over IP (RFC 768).
 * ========================================================================= */
#ifndef USERSPACE_TCPIP_UDP_H
#define USERSPACE_TCPIP_UDP_H

#include "netif.h"

/* Receive path: a UDP datagram. `ip` supplies the addresses the checksum's
 * pseudo-header needs and the reply destination. */
void udp_input(struct netif *nif, const struct ip_hdr *ip,
               u8 *payload, size_t len);

/* Send `data`/`len` from `src_port` to `dst_ip:dst_port` (ports host order,
 * ip network order). Returns 0 or -1. */
int udp_output(struct netif *nif, u32 dst_ip,
               u16 src_port, u16 dst_port, const void *data, size_t len);

#endif /* USERSPACE_TCPIP_UDP_H */
