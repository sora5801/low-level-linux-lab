/* ===========================================================================
 * arp.h — Address Resolution Protocol: IPv4 address -> Ethernet MAC.
 * ===========================================================================
 * To put an IP packet on an Ethernet link we need the destination's MAC. ARP is
 * the broadcast "who has 10.0.0.1? tell 10.0.0.2" question and its unicast
 * reply. We keep a small cache so we ask at most once per peer.
 * ========================================================================= */
#ifndef USERSPACE_TCPIP_ARP_H
#define USERSPACE_TCPIP_ARP_H

#include "netif.h"

/* Handle an inbound ARP message (already stripped of its Ethernet header). */
void arp_input(struct netif *nif, u8 *payload, size_t len);

/* Look up `ip` (NETWORK order) in the cache. On hit, copies the 6-byte MAC into
 * `mac_out` and returns 0. On miss returns -1 (caller should arp_request). */
int arp_lookup(u32 ip, u8 mac_out[ETH_ALEN]);

/* Broadcast an ARP request asking who owns `target_ip` (NETWORK order). */
int arp_request(struct netif *nif, u32 target_ip);

#endif /* USERSPACE_TCPIP_ARP_H */
