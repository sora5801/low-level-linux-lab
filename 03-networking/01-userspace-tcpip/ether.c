/* ===========================================================================
 * ether.c — Ethernet II framing: the demux at the bottom of the receive path
 * and the frame builder at the bottom of the send path.
 * ===========================================================================
 *
 * Every byte that enters or leaves our stack passes through here. On input we
 * read the 14-byte header, decide ARP vs IPv4 from the EtherType, and hand the
 * payload up. On output we glue a 14-byte header in front of an upper-layer
 * packet and push the frame to the TAP fd.
 * ========================================================================= */

#include "netif.h"
#include "tap.h"
#include "arp.h"
#include "ip.h"

#include <string.h>   /* memcpy */

/* ---------------------------------------------------------------------------
 * eth_input — parse one frame and demultiplex on EtherType.
 * ------------------------------------------------------------------------- */
void eth_input(struct netif *nif, u8 *frame, size_t len)
{
    /* A frame shorter than the header is truncated garbage; drop it. Guarding
     * here means every layer above may assume at least a full Ethernet header. */
    if (len < ETH_HDR_LEN) {
        LOGF("eth: runt frame (%zu bytes)\n", len);
        return;
    }

    /* Overlay the struct on the raw bytes. Because eth_hdr is packed, field
     * accesses become plain offset loads — no copy, no reformatting. */
    struct eth_hdr *eth = (struct eth_hdr *)frame;

    /* payload starts right after the header; its length is frame minus header. */
    u8    *payload   = frame + ETH_HDR_LEN;
    size_t paylen    = len   - ETH_HDR_LEN;

    /* ethertype is on the wire in network order; compare against htons(...) so
     * the constant is byte-swapped to match, not the field (cheaper, and keeps
     * the field pristine for any later re-send). We use if/else rather than a
     * switch because htons() of a runtime value is not a case-label constant. */
    if (eth->ethertype == htons(ETH_P_ARP)) {
        arp_input(nif, payload, paylen);
    } else if (eth->ethertype == htons(ETH_P_IP)) {
        ip_input(nif, payload, paylen);
    } else {
        /* IPv6 (0x86DD), VLAN tags, etc. — out of scope for this teaching core.
         * Dropping unknown EtherTypes is exactly what a minimal NIC filter does. */
        LOGF("eth: unhandled ethertype 0x%04x\n", ntohs(eth->ethertype));
    }
}

/* ---------------------------------------------------------------------------
 * eth_output — prepend an Ethernet header and transmit.
 *
 * We assemble into a local frame buffer: header first, then the caller's
 * payload. A production stack would instead reserve headroom in a shared packet
 * buffer to avoid this copy (see the kernel's sk_buff), but the copy keeps the
 * teaching version simple and the ownership obvious (the buffer is on our stack
 * and gone when we return).
 * ------------------------------------------------------------------------- */
int eth_output(struct netif *nif, const u8 dst_mac[ETH_ALEN],
                u16 ethertype, const void *payload, size_t len)
{
    u8 frame[FRAME_MAX];

    /* Refuse to build a frame that would exceed the link MTU; the caller (IP)
     * is responsible for fragmenting, which this teaching core does not do. */
    if (len > FRAME_MAX - ETH_HDR_LEN) {
        LOGF("eth: payload too big (%zu)\n", len);
        return -1;
    }

    struct eth_hdr *eth = (struct eth_hdr *)frame;
    memcpy(eth->dst, dst_mac, ETH_ALEN);   /* who we're sending to             */
    memcpy(eth->src, nif->mac, ETH_ALEN);  /* our own MAC as the source        */
    eth->ethertype = htons(ethertype);     /* host -> network order            */

    /* Copy the upper-layer packet in right after the header. */
    memcpy(frame + ETH_HDR_LEN, payload, len);

    /* Push the whole frame to the kernel. tap_write is atomic per frame. */
    if (tap_write(nif->fd, frame, ETH_HDR_LEN + len) < 0)
        return -1;
    return 0;
}
