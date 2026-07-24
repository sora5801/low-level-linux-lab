/* ===========================================================================
 * icmp.c — ICMP echo request/reply (RFC 792).
 * ===========================================================================
 *
 * `ping` sends an ICMP ECHO REQUEST (type 8) and expects an ECHO REPLY (type 0)
 * that carries back the SAME id, sequence, and payload bytes. That "echo the
 * payload verbatim" is why ping can measure round-trip time and detect
 * corruption. Implementing just this one message type is enough to make the
 * standard `ping` tool report replies from our userspace stack.
 *
 * The ICMP checksum covers the ICMP header + data ONLY (no pseudo-header —
 * that's a TCP/UDP thing). The clever shortcut we use: to turn a request into a
 * reply we only change the `type` byte from 8 to 0. Ones'-complement checksum
 * math lets us patch the checksum incrementally instead of recomputing it, but
 * for clarity we simply zero and recompute over the whole message.
 * ========================================================================= */

#include "icmp.h"
#include "ip.h"
#include "checksum.h"

#include <string.h>

void icmp_input(struct netif *nif, const struct ip_hdr *ip,
                u8 *payload, size_t len)
{
    if (len < sizeof(struct icmp_hdr)) {
        LOGF("icmp: runt (%zu)\n", len);
        return;
    }
    struct icmp_hdr *icmp = (struct icmp_hdr *)payload;

    /* Validate the checksum: a correct ICMP message sums to 0xFFFF, so
     * inet_checksum over it yields 0. A bad checksum means the payload we'd echo
     * is corrupt — drop rather than reflect garbage. */
    if (inet_checksum(icmp, len) != 0) {
        LOGF("icmp: bad checksum\n");
        return;
    }

    /* We only answer echo requests. Everything else (destination-unreachable,
     * time-exceeded, our own replies) is ignored by this teaching core. */
    if (icmp->type != ICMP_ECHO_REQUEST) {
        return;
    }

    /* Build the reply in a scratch buffer that is a copy of the request: id,
     * seq, and the opaque data must come back UNCHANGED — that is the contract
     * ping relies on. Reusing the bytes is the simplest way to guarantee it. */
    u8 reply[IP_MTU];
    if (len > sizeof(reply)) {
        LOGF("icmp: oversize echo (%zu)\n", len);
        return;
    }
    memcpy(reply, payload, len);

    struct icmp_hdr *rep = (struct icmp_hdr *)reply;
    rep->type     = ICMP_ECHO_REPLY;   /* 8 -> 0: the only semantic change     */
    rep->code     = 0;
    rep->checksum = 0;                  /* zero before recomputing              */
    rep->checksum = inet_checksum(rep, len);

    /* Reply goes back to the sender of the request (ip->src, network order). IP
     * fills in our address as the new source and resolves their MAC via ARP
     * (already cached from the incoming frame's ARP or a prior exchange). */
    ip_output(nif, ip->src, IPPROTO_ICMP_, reply, len);
}
