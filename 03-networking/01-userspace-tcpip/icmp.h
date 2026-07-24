/* ===========================================================================
 * icmp.h — ICMP echo, i.e. the thing that makes `ping` work.
 * ========================================================================= */
#ifndef USERSPACE_TCPIP_ICMP_H
#define USERSPACE_TCPIP_ICMP_H

#include "netif.h"

/* Handle an ICMP message. `ip` is the parsed IPv4 header (we need its source
 * address to know where to reply); `payload`/`len` is the ICMP message. */
void icmp_input(struct netif *nif, const struct ip_hdr *ip,
                u8 *payload, size_t len);

#endif /* USERSPACE_TCPIP_ICMP_H */
