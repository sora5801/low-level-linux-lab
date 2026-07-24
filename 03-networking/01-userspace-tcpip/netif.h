/* ===========================================================================
 * netif.h — the network interface object every layer shares.
 * ===========================================================================
 * `struct netif` bundles the one link we own: its fd, our MAC, our IP. Passing
 * it explicitly (rather than using globals) keeps ownership obvious and the code
 * re-entrant if it ever grows a second interface.
 * ========================================================================= */
#ifndef USERSPACE_TCPIP_NETIF_H
#define USERSPACE_TCPIP_NETIF_H

#include "common.h"

/* An Ethernet frame maxes out at 14 (header) + 1500 (MTU payload) = 1514; we
 * round up for safety. All output builders assemble into a buffer this size. */
#define FRAME_MAX 1600
#define IP_MTU    1500        /* max IP packet we will emit on this link       */

struct netif {
    int fd;                   /* the TAP file descriptor                       */
    u8  mac[ETH_ALEN];        /* our hardware address                          */
    u32 ip;                   /* our IPv4 address, NETWORK byte order           */
    char name[16];            /* interface name, e.g. "tap0" (for logs)        */
};

/* ---- Ethernet (ether.c) ------------------------------------------------- */

/* Top of the receive path: a full frame arrived on the tap. Parse the Ethernet
 * header and dispatch to ARP or IP. `len` is the frame length from tap_read. */
void eth_input(struct netif *nif, u8 *frame, size_t len);

/* Send `payload` (`len` bytes) as one Ethernet frame to `dst_mac` with the
 * given `ethertype` (host order; we htons it). Returns 0 on success, -1 on I/O
 * error. Prepends the 14-byte Ethernet header in place of a copy by using a
 * caller-agnostic scratch frame. */
int eth_output(struct netif *nif, const u8 dst_mac[ETH_ALEN],
                u16 ethertype, const void *payload, size_t len);

#endif /* USERSPACE_TCPIP_NETIF_H */
