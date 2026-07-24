/* ===========================================================================
 * arp.c — ARP cache + request/reply (RFC 826).
 * ===========================================================================
 *
 * WHY ARP EXISTS
 * --------------
 * IP addresses are a routing abstraction; the Ethernet hardware only knows how
 * to deliver a frame to a 6-byte MAC. ARP bridges the two: before host A can
 * send an IP packet to B on the same link, it broadcasts "who has B's IP?" and
 * B answers "that's me, here's my MAC." The answer is cached so the question is
 * asked at most once per peer (until the entry ages out — we never expire ours,
 * which is fine for a single-peer teaching link).
 *
 * A packed ARP message is 28 bytes for IPv4-over-Ethernet (see struct arp_hdr).
 * Note the addresses inside ARP are stored as RAW BYTES already in network
 * order; we memcpy them rather than htonl/ntohl, because ARP treats them as
 * opaque protocol addresses of length `plen`.
 * ========================================================================= */

#include "arp.h"
#include "netif.h"

#include <string.h>   /* memcpy, memcmp, memset */

/* ---------------------------------------------------------------------------
 * The cache. A fixed array — no allocation, no ownership questions, and O(n)
 * lookups that are trivially fast for the handful of peers a teaching link
 * ever sees. Each slot maps an IPv4 address (network order) to a MAC.
 * ------------------------------------------------------------------------- */
#define ARP_CACHE_SIZE 16

struct arp_entry {
    int valid;                 /* 0 = empty slot                              */
    u32 ip;                    /* NETWORK order (matches the wire directly)   */
    u8  mac[ETH_ALEN];
};

static struct arp_entry g_arp_cache[ARP_CACHE_SIZE];

/* The broadcast MAC: a request must reach every station on the segment. */
static const u8 broadcast_mac[ETH_ALEN] = { 0xff,0xff,0xff,0xff,0xff,0xff };

/* ---------------------------------------------------------------------------
 * arp_cache_update — insert or refresh ip->mac. Idempotent.
 *
 * RFC 826's rule: if we already have the sender, update it (their MAC may have
 * changed); otherwise, only insert on the packets addressed to us. We simplify
 * to "always learn," which is what a host on a trusted point-to-point link
 * wants. On a hostile LAN this is the door through which ARP spoofing walks —
 * worth stating out loud in a teaching stack.
 * ------------------------------------------------------------------------- */
static void arp_cache_update(u32 ip, const u8 mac[ETH_ALEN])
{
    int free_slot = -1;

    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (g_arp_cache[i].valid && g_arp_cache[i].ip == ip) {
            memcpy(g_arp_cache[i].mac, mac, ETH_ALEN);   /* refresh existing   */
            return;
        }
        if (!g_arp_cache[i].valid && free_slot < 0)
            free_slot = i;                               /* remember a hole    */
    }

    if (free_slot < 0) {
        /* Cache full: a real stack would evict LRU. We overwrite slot 0 and
         * note it — acceptable because the working set here is tiny. */
        LOGF("arp: cache full, evicting slot 0\n");
        free_slot = 0;
    }
    g_arp_cache[free_slot].valid = 1;
    g_arp_cache[free_slot].ip    = ip;
    memcpy(g_arp_cache[free_slot].mac, mac, ETH_ALEN);
}

int arp_lookup(u32 ip, u8 mac_out[ETH_ALEN])
{
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (g_arp_cache[i].valid && g_arp_cache[i].ip == ip) {
            memcpy(mac_out, g_arp_cache[i].mac, ETH_ALEN);
            return 0;
        }
    }
    return -1;   /* miss: caller should arp_request and retry later           */
}

/* ---------------------------------------------------------------------------
 * arp_reply — answer a request for OUR address.
 * We swap the roles: the requester becomes the target, we become the sender.
 * ------------------------------------------------------------------------- */
static void arp_reply(struct netif *nif, const struct arp_hdr *req)
{
    struct arp_hdr rep;

    rep.htype = htons(ARP_HTYPE_ETH);
    rep.ptype = htons(ETH_P_IP);
    rep.hlen  = ETH_ALEN;
    rep.plen  = 4;
    rep.oper  = htons(ARP_OP_REPLY);

    /* Sender = us. Our MAC and our IP (nif->ip is already network order, and
     * ARP wants the 4 protocol-address bytes exactly as on the wire). */
    memcpy(rep.sha, nif->mac, ETH_ALEN);
    memcpy(rep.spa, &nif->ip, 4);

    /* Target = whoever asked (copy their hardware/protocol address straight
     * from the request's SENDER fields). */
    memcpy(rep.tha, req->sha, ETH_ALEN);
    memcpy(rep.tpa, req->spa, 4);

    /* A reply is unicast back to the requester's MAC. */
    eth_output(nif, req->sha, ETH_P_ARP, &rep, sizeof(rep));
}

/* ---------------------------------------------------------------------------
 * arp_input — process an inbound ARP message.
 * ------------------------------------------------------------------------- */
void arp_input(struct netif *nif, u8 *payload, size_t len)
{
    if (len < sizeof(struct arp_hdr)) {
        LOGF("arp: runt (%zu)\n", len);
        return;
    }
    struct arp_hdr *arp = (struct arp_hdr *)payload;

    /* We only speak IPv4-over-Ethernet. Reject anything else so we never
     * misinterpret a field whose length isn't 6/4. */
    if (arp->htype != htons(ARP_HTYPE_ETH) ||
        arp->ptype != htons(ETH_P_IP) ||
        arp->hlen  != ETH_ALEN || arp->plen != 4) {
        LOGF("arp: unsupported htype/ptype\n");
        return;
    }

    /* Learn the sender's mapping regardless of op: both requests and replies
     * carry a valid sender IP+MAC, and caching it now often saves a round trip.
     * spa is 4 raw network-order bytes; read them into a u32 with memcpy to
     * avoid any alignment assumption about `payload`. */
    u32 sender_ip;
    memcpy(&sender_ip, arp->spa, 4);
    arp_cache_update(sender_ip, arp->sha);

    /* Is this a request for one of our addresses? Compare the 4 target-protocol
     * bytes against our IP (both network order). */
    u32 target_ip;
    memcpy(&target_ip, arp->tpa, 4);

    switch (ntohs(arp->oper)) {
    case ARP_OP_REQUEST:
        if (target_ip == nif->ip)
            arp_reply(nif, arp);        /* "that's me" */
        break;
    case ARP_OP_REPLY:
        /* Nothing more to do: the cache update above already recorded it. A
         * real stack would now flush any packets queued waiting on this MAC. */
        break;
    default:
        LOGF("arp: unknown oper %u\n", ntohs(arp->oper));
        break;
    }
}

/* ---------------------------------------------------------------------------
 * arp_request — broadcast "who has target_ip? tell <us>".
 * ------------------------------------------------------------------------- */
int arp_request(struct netif *nif, u32 target_ip)
{
    struct arp_hdr req;

    req.htype = htons(ARP_HTYPE_ETH);
    req.ptype = htons(ETH_P_IP);
    req.hlen  = ETH_ALEN;
    req.plen  = 4;
    req.oper  = htons(ARP_OP_REQUEST);

    memcpy(req.sha, nif->mac, ETH_ALEN);   /* sender = us                      */
    memcpy(req.spa, &nif->ip, 4);
    memset(req.tha, 0, ETH_ALEN);          /* target MAC unknown -> zeroes     */
    memcpy(req.tpa, &target_ip, 4);        /* the address we're asking about   */

    /* Requests go to the Ethernet broadcast address so every host can answer. */
    return eth_output(nif, broadcast_mac, ETH_P_ARP, &req, sizeof(req));
}
