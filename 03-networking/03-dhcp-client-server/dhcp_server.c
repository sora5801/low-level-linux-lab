/* ===========================================================================
 * dhcp_server.c — a minimal DHCP server: a lease pool with expiry, over
 *                 AF_PACKET raw sockets.
 * ===========================================================================
 *
 * The server is the mirror image of the client. It listens on port 67 for
 * BOOTREQUESTs (DISCOVER and REQUEST), hands out addresses from a configured
 * pool, and tracks each lease's owner MAC and expiry time. Because the client
 * has no IP yet, the server also builds full Ethernet+IP+UDP frames and
 * broadcasts its replies (honouring the client's BROADCAST flag).
 *
 * DORA from the server's side:
 *   DISCOVER in  -> pick/allocate a lease -> OFFER out (yiaddr = candidate IP)
 *   REQUEST  in  -> validate requested IP -> ACK out (lease committed) or NAK
 *
 * LEASE POOL. A fixed array of slots, each mapping an IP to the MAC currently
 * holding it and an absolute expiry timestamp. A slot is reusable when it is
 * unused OR its lease has expired; on every incoming packet we first reclaim
 * expired slots (event-driven expiry — see README for the production approach
 * of a timer wheel / periodic sweep).
 *
 * PLATFORM: Linux only; needs CAP_NET_RAW (root or `setcap cap_net_raw+ep`).
 * ===========================================================================
 */
#include "dhcp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>            /* time(), time_t — absolute lease expiry          */
#include <signal.h>         /* catch SIGINT/SIGTERM to exit the accept loop     */

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>      /* inet_ntop / inet_pton                            */
#include <net/if.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>

static const uint8_t BROADCAST_MAC[MAC_LEN] = {0xff,0xff,0xff,0xff,0xff,0xff};

/* Reservation windows. A DISCOVER only *offers* an address; we hold it briefly
 * so a racing client cannot be handed the same IP before it REQUESTs. A REQUEST
 * commits the full lease. */
#define OFFER_HOLD_SECS  30       /* how long an OFFER ties up a slot            */
#define DEFAULT_LEASE    3600     /* committed lease length (seconds)           */
#define MAX_POOL         256      /* upper bound on pool slots                   */

/* ---------------------------------------------------------------------------
 * Server configuration and lease table.
 * --------------------------------------------------------------------------- */
struct srv_config {
    uint32_t server_ip;    /* our identity (option 54), network order          */
    uint32_t netmask;      /* option 1                                          */
    uint32_t router;       /* option 3                                          */
    uint32_t dns;          /* option 6                                          */
    uint32_t lease_secs;   /* option 51                                         */
    uint8_t  mac[MAC_LEN]; /* our NIC MAC, source of reply frames              */
    int      ifindex;
};

/* One lease slot. `ip` is fixed at init (the pool is a contiguous range); the
 * rest changes as clients come and go. */
struct lease {
    uint32_t ip;               /* offered/assigned address, network order       */
    uint8_t  mac[MAC_LEN];     /* client holding it (valid when state != FREE)   */
    time_t   expiry;           /* absolute time the hold/lease ends              */
    int      state;            /* LEASE_FREE / LEASE_OFFERED / LEASE_BOUND       */
};
enum { LEASE_FREE = 0, LEASE_OFFERED, LEASE_BOUND };

static struct lease g_pool[MAX_POOL];
static int          g_pool_n = 0;

/* A flag flipped by the signal handler so the main loop can exit cleanly and we
 * can close(fd). `volatile sig_atomic_t` is the only type the C standard lets a
 * handler write and the main flow read without a data race. */
static volatile sig_atomic_t g_stop = 0;
static void on_signal(int sig) { (void)sig; g_stop = 1; }

/* ---------------------------------------------------------------------------
 * Pool management.
 * --------------------------------------------------------------------------- */

/* Reclaim any slot whose lease/hold has expired. Called at the top of each
 * packet handler so stale reservations don't leak addresses. */
static void pool_reclaim(time_t now)
{
    for (int i = 0; i < g_pool_n; i++) {
        if (g_pool[i].state != LEASE_FREE && g_pool[i].expiry <= now) {
            char b[INET_ADDRSTRLEN];
            struct in_addr a; a.s_addr = g_pool[i].ip;
            inet_ntop(AF_INET, &a, b, sizeof b);
            printf("   [pool] reclaimed expired %s\n", b);
            g_pool[i].state = LEASE_FREE;
            memset(g_pool[i].mac, 0, MAC_LEN);
        }
    }
}

/* Find the slot currently associated with `mac` (offered or bound), or -1. A
 * client that DISCOVERs twice, or REQUESTs after an OFFER, must land on the
 * SAME slot — leases are keyed by client hardware address. */
static int pool_find_by_mac(const uint8_t mac[MAC_LEN])
{
    for (int i = 0; i < g_pool_n; i++)
        if (g_pool[i].state != LEASE_FREE &&
            memcmp(g_pool[i].mac, mac, MAC_LEN) == 0)
            return i;
    return -1;
}

/* Find a free slot to allocate, or -1 if the pool is exhausted. */
static int pool_find_free(void)
{
    for (int i = 0; i < g_pool_n; i++)
        if (g_pool[i].state == LEASE_FREE)
            return i;
    return -1;
}

/* Return the slot index whose IP == `ip`, or -1. Used to validate a REQUEST's
 * option-50 requested address against the pool. */
static int pool_find_by_ip(uint32_t ip)
{
    for (int i = 0; i < g_pool_n; i++)
        if (g_pool[i].ip == ip)
            return i;
    return -1;
}

/* ---------------------------------------------------------------------------
 * Socket setup — identical shape to the client's (see dhcp_client.c comments).
 * --------------------------------------------------------------------------- */
static int open_packet_socket(const char *ifname, struct srv_config *cfg)
{
    int fd = socket(AF_PACKET, SOCK_RAW, htons_(ETH_P_ALL));
    if (fd < 0) { perror("socket(AF_PACKET, SOCK_RAW)"); return -1; }

    unsigned idx = if_nametoindex(ifname);
    if (idx == 0) { perror("if_nametoindex"); close(fd); return -1; }
    cfg->ifindex = (int)idx;

    struct ifreq ifr;
    memset(&ifr, 0, sizeof ifr);
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
    if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0) {
        perror("ioctl(SIOCGIFHWADDR)"); close(fd); return -1;
    }
    memcpy(cfg->mac, ifr.ifr_hwaddr.sa_data, MAC_LEN);

    struct sockaddr_ll sll;
    memset(&sll, 0, sizeof sll);
    sll.sll_family   = AF_PACKET;
    sll.sll_ifindex  = cfg->ifindex;
    sll.sll_protocol = htons_(ETH_P_IP);
    if (bind(fd, (struct sockaddr *)&sll, sizeof sll) < 0) {
        perror("bind(AF_PACKET)"); close(fd); return -1;
    }
    return fd;
}

/* ---------------------------------------------------------------------------
 * build_reply — assemble the DHCP payload for an OFFER or ACK. Shares the
 * option set: server-id (54), lease-time (51), subnet mask (1), router (3),
 * DNS (6). Returns the DHCP payload length (fixed 236 + 4 cookie + options).
 * --------------------------------------------------------------------------- */
static size_t build_reply(uint8_t *dhcp, const struct dhcp_msg *req,
                          uint8_t msgtype, uint32_t yiaddr,
                          const struct srv_config *cfg)
{
    struct dhcp_msg *m = (struct dhcp_msg *)dhcp;
    memset(m, 0, sizeof *m);
    m->op    = BOOTREPLY;            /* server -> client                        */
    m->htype = HTYPE_ETHERNET;
    m->hlen  = MAC_LEN;
    m->xid   = req->xid;             /* echo the client's transaction id        */
    m->flags = req->flags;           /* echo BROADCAST flag so it round-trips   */
    m->yiaddr = yiaddr;              /* the address we are giving out           */
    m->siaddr = cfg->server_ip;      /* "next server" = us                      */
    memcpy(m->chaddr, req->chaddr, MAC_LEN);  /* the client's MAC               */

    /* Magic cookie (network order: 63 82 53 63). */
    uint8_t *cookie = dhcp + DHCP_FIXED_LEN;
    cookie[0] = 0x63; cookie[1] = 0x82; cookie[2] = 0x53; cookie[3] = 0x63;

    uint8_t *opts = dhcp + DHCP_FIXED_LEN + 4;
    size_t off = 0;
    off = dhcp_opt_append_u8 (opts, off, DHCP_OPT_MSG_TYPE, msgtype);
    off = dhcp_opt_append_u32(opts, off, DHCP_OPT_SERVER_ID, cfg->server_ip);

    /* A NAK is a pure rejection: RFC 2131 §4.3.2 says it carries only the
     * message type and server identifier (no lease time, no config), because
     * there is no lease to describe. OFFER/ACK carry the full configuration. */
    if (msgtype != DHCPNAK) {
        off = dhcp_opt_append_u32(opts, off, DHCP_OPT_LEASE_TIME, htonl_(cfg->lease_secs));
        off = dhcp_opt_append_u32(opts, off, DHCP_OPT_SUBNET_MASK, cfg->netmask);
        off = dhcp_opt_append_u32(opts, off, DHCP_OPT_ROUTER, cfg->router);
        off = dhcp_opt_append_u32(opts, off, DHCP_OPT_DNS, cfg->dns);
    }
    off = dhcp_opt_end(opts, off);

    return DHCP_FIXED_LEN + 4 + off;
}

/* Send a reply frame. Honours the client's BROADCAST flag: if set (or giaddr is
 * 0 and the client has no ciaddr), we broadcast at L2/L3 so the not-yet-
 * configured client's stack will accept the packet; otherwise we unicast. */
static int send_reply(int fd, const struct srv_config *cfg,
                      const struct dhcp_msg *req, uint32_t yiaddr,
                      const uint8_t *dhcp, size_t dhcp_len)
{
    uint8_t frame[FRAME_BUF_MAX];
    const uint8_t *dst_mac;
    uint32_t dst_ip;

    int bcast = (req->flags & htons_(DHCP_FLAG_BROADCAST)) != 0;
    if (bcast) {
        dst_mac = BROADCAST_MAC;
        dst_ip  = htonl_(0xFFFFFFFF);          /* 255.255.255.255               */
    } else {
        dst_mac = req->chaddr;                  /* unicast to the client's MAC   */
        dst_ip  = yiaddr;                       /* to the address we assigned    */
    }

    size_t flen = dhcp_build_frame(frame, dst_mac, cfg->mac,
                                   cfg->server_ip, dst_ip,
                                   DHCP_SERVER_PORT, DHCP_CLIENT_PORT,
                                   dhcp, dhcp_len);

    struct sockaddr_ll sll;
    memset(&sll, 0, sizeof sll);
    sll.sll_family   = AF_PACKET;
    sll.sll_ifindex  = cfg->ifindex;
    sll.sll_halen    = MAC_LEN;
    sll.sll_protocol = htons_(ETH_P_IP);
    memcpy(sll.sll_addr, dst_mac, MAC_LEN);

    for (;;) {
        ssize_t n = sendto(fd, frame, flen, 0,
                           (struct sockaddr *)&sll, sizeof sll);
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("sendto");
            return -1;
        }
        if ((size_t)n != flen) {
            fprintf(stderr, "sendto: short write %zd/%zu\n", n, flen);
            return -1;
        }
        return 0;
    }
}

/* ---------------------------------------------------------------------------
 * Handlers for the two request types.
 * --------------------------------------------------------------------------- */

/* DISCOVER: reserve an address for this MAC and OFFER it. */
static void handle_discover(int fd, struct srv_config *cfg,
                            const struct dhcp_msg *req,
                            const uint8_t *opts, size_t opts_len, time_t now)
{
    (void)opts; (void)opts_len;   /* a fuller server would honour option 50/55 */

    /* Reuse an existing reservation for this client, else grab a free slot. */
    int idx = pool_find_by_mac(req->chaddr);
    if (idx < 0)
        idx = pool_find_free();
    if (idx < 0) {
        printf("   pool exhausted; no OFFER\n");
        return;
    }

    /* Tentatively reserve the slot: OFFERED, held for OFFER_HOLD_SECS. */
    g_pool[idx].state  = LEASE_OFFERED;
    g_pool[idx].expiry = now + OFFER_HOLD_SECS;
    memcpy(g_pool[idx].mac, req->chaddr, MAC_LEN);

    uint8_t dhcp[FRAME_BUF_MAX];
    size_t dlen = build_reply(dhcp, req, DHCPOFFER, g_pool[idx].ip, cfg);

    char b[INET_ADDRSTRLEN];
    struct in_addr a; a.s_addr = g_pool[idx].ip;
    inet_ntop(AF_INET, &a, b, sizeof b);
    printf("   -> OFFER %s\n", b);

    if (send_reply(fd, cfg, req, g_pool[idx].ip, dhcp, dlen) < 0)
        printf("   (failed to send OFFER)\n");
}

/* REQUEST: validate the client's chosen address and either ACK (commit the
 * lease) or NAK (tell it to restart). */
static void handle_request(int fd, struct srv_config *cfg,
                           const struct dhcp_msg *req,
                           const uint8_t *opts, size_t opts_len, time_t now)
{
    /* If the client named a server-id (option 54) that is not us, this REQUEST
     * is meant for a different server; stay silent (RFC 2131 §4.3.2). */
    uint8_t l = 0;
    const uint8_t *sid = dhcp_opt_find(opts, opts_len, DHCP_OPT_SERVER_ID, &l);
    if (sid && l == 4) {
        uint32_t chosen; memcpy(&chosen, sid, 4);
        if (chosen != cfg->server_ip)
            return;                         /* not our client                    */
    }

    /* The requested address is option 50 (during SELECTING) or ciaddr (during
     * RENEWING). We accept either. */
    uint32_t want = 0;
    const uint8_t *rip = dhcp_opt_find(opts, opts_len, DHCP_OPT_REQUESTED_IP, &l);
    if (rip && l == 4)
        memcpy(&want, rip, 4);
    else
        want = req->ciaddr;

    int idx = pool_find_by_ip(want);
    int mine = (idx >= 0) &&
               (g_pool[idx].state == LEASE_FREE ||
                memcmp(g_pool[idx].mac, req->chaddr, MAC_LEN) == 0);

    char b[INET_ADDRSTRLEN];
    struct in_addr a; a.s_addr = want;
    inet_ntop(AF_INET, &a, b, sizeof b);

    uint8_t dhcp[FRAME_BUF_MAX];
    if (idx < 0 || !mine) {
        /* The address is outside our pool or held by someone else: NAK. */
        printf("   -> NAK  (%s not available)\n", b);
        size_t dlen = build_reply(dhcp, req, DHCPNAK, htonl_(0), cfg);
        /* A NAK carries yiaddr 0; still broadcast so the client hears it. */
        struct dhcp_msg *m = (struct dhcp_msg *)dhcp;
        m->yiaddr = 0;
        if (send_reply(fd, cfg, req, 0, dhcp, dlen) < 0)
            printf("   (failed to send NAK)\n");
        return;
    }

    /* Commit the lease: BOUND until now + lease_secs. */
    g_pool[idx].state  = LEASE_BOUND;
    g_pool[idx].expiry = now + cfg->lease_secs;
    memcpy(g_pool[idx].mac, req->chaddr, MAC_LEN);

    size_t dlen = build_reply(dhcp, req, DHCPACK, g_pool[idx].ip, cfg);
    printf("   -> ACK  %s (lease %u s)\n", b, cfg->lease_secs);
    if (send_reply(fd, cfg, req, g_pool[idx].ip, dhcp, dlen) < 0)
        printf("   (failed to send ACK)\n");
}

/* RELEASE: the client is done; free its slot immediately. */
static void handle_release(const struct dhcp_msg *req)
{
    int idx = pool_find_by_mac(req->chaddr);
    if (idx >= 0) {
        char b[INET_ADDRSTRLEN];
        struct in_addr a; a.s_addr = g_pool[idx].ip;
        inet_ntop(AF_INET, &a, b, sizeof b);
        printf("   client released %s\n", b);
        g_pool[idx].state = LEASE_FREE;
        memset(g_pool[idx].mac, 0, MAC_LEN);
    }
}

/* ---------------------------------------------------------------------------
 * process_frame — validate one received frame and dispatch by message type.
 * Mirrors the client's parse_reply but accepts BOOTREQUESTs to port 67.
 * --------------------------------------------------------------------------- */
static void process_frame(int fd, struct srv_config *cfg,
                          const uint8_t *buf, size_t len)
{
    if (len < ETH_HDR_LEN + IP_HDR_LEN + UDP_HDR_LEN + DHCP_FIXED_LEN + 4)
        return;

    const struct eth_hdr *eth = (const struct eth_hdr *)buf;
    if (eth->ethertype != htons_(ETHERTYPE_IPV4)) return;

    const struct ip_hdr *ip = (const struct ip_hdr *)(buf + ETH_HDR_LEN);
    if ((ip->ver_ihl >> 4) != 4 || ip->proto != IPPROTO_UDP_) return;
    size_t ihl = (size_t)(ip->ver_ihl & 0x0F) * 4;
    if (ihl < IP_HDR_LEN) return;
    /* Re-check the frame is long enough given the real IP header length before
     * dereferencing the UDP/DHCP structs at their (ihl-dependent) offsets. */
    if (len < ETH_HDR_LEN + ihl + UDP_HDR_LEN + DHCP_FIXED_LEN + 4) return;

    const struct udp_hdr *udp = (const struct udp_hdr *)(buf + ETH_HDR_LEN + ihl);
    if (udp->dport != htons_(DHCP_SERVER_PORT)) return;   /* must target 67      */

    const struct dhcp_msg *msg =
        (const struct dhcp_msg *)(buf + ETH_HDR_LEN + ihl + UDP_HDR_LEN);
    if (msg->op != BOOTREQUEST) return;                   /* requests only       */

    const uint8_t *cookie = (const uint8_t *)msg + DHCP_FIXED_LEN;
    uint32_t got = ((uint32_t)cookie[0] << 24) | ((uint32_t)cookie[1] << 16) |
                   ((uint32_t)cookie[2] << 8)  |  (uint32_t)cookie[3];
    if (got != DHCP_MAGIC_COOKIE) return;

    const uint8_t *opts = cookie + 4;
    size_t opts_len = len - (size_t)(opts - buf);

    uint8_t tlen = 0;
    const uint8_t *mt = dhcp_opt_find(opts, opts_len, DHCP_OPT_MSG_TYPE, &tlen);
    if (!mt || tlen != 1) return;

    time_t now = time(NULL);
    pool_reclaim(now);                        /* event-driven expiry sweep       */

    printf("<- from %02x:%02x:%02x:%02x:%02x:%02x  ",
           msg->chaddr[0], msg->chaddr[1], msg->chaddr[2],
           msg->chaddr[3], msg->chaddr[4], msg->chaddr[5]);

    switch (mt[0]) {
    case DHCPDISCOVER:
        printf("DISCOVER\n");
        handle_discover(fd, cfg, msg, opts, opts_len, now);
        break;
    case DHCPREQUEST:
        printf("REQUEST\n");
        handle_request(fd, cfg, msg, opts, opts_len, now);
        break;
    case DHCPRELEASE:
        printf("RELEASE\n");
        handle_release(msg);
        break;
    default:
        printf("(ignored type %u)\n", mt[0]);
        break;
    }
}

/* Parse a dotted-quad into a network-order uint32_t, exiting on failure. */
static uint32_t parse_ip(const char *s)
{
    struct in_addr a;
    if (inet_pton(AF_INET, s, &a) != 1) {
        fprintf(stderr, "bad IPv4 address: %s\n", s);
        exit(EXIT_FAILURE);
    }
    return a.s_addr;                          /* already network order           */
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr,
            "usage: %s <interface> [server-ip] [pool-start] [pool-count]\n"
            "  defaults: server 192.168.50.1, pool 192.168.50.100 x 20\n",
            argv[0]);
        return EXIT_FAILURE;
    }
    const char *ifname = argv[1];

    struct srv_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.server_ip  = parse_ip(argc > 2 ? argv[2] : "192.168.50.1");
    cfg.netmask    = parse_ip("255.255.255.0");
    cfg.router     = cfg.server_ip;           /* we double as the gateway        */
    cfg.dns        = cfg.server_ip;           /* ...and the DNS forwarder        */
    cfg.lease_secs = DEFAULT_LEASE;

    uint32_t pool_start = parse_ip(argc > 3 ? argv[3] : "192.168.50.100");
    int      pool_count = argc > 4 ? atoi(argv[4]) : 20;
    if (pool_count < 1 || pool_count > MAX_POOL) {
        fprintf(stderr, "pool-count must be 1..%d\n", MAX_POOL);
        return EXIT_FAILURE;
    }

    /* Build the contiguous pool. The addresses are network order, so to step by
     * one we convert to host order, add, and convert back — otherwise "+1" would
     * increment the wrong (most-significant on-wire) byte on a little-endian host. */
    uint32_t base_host = ntohl_(pool_start);
    for (int i = 0; i < pool_count; i++) {
        g_pool[i].ip    = htonl_(base_host + (uint32_t)i);
        g_pool[i].state = LEASE_FREE;
        g_pool[i].expiry = 0;
    }
    g_pool_n = pool_count;

    int fd = open_packet_socket(ifname, &cfg);
    if (fd < 0)
        return EXIT_FAILURE;

    /* Install signal handlers so Ctrl-C exits the loop and closes the socket. */
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = on_signal;
    /* No SA_RESTART: we WANT recvfrom to return EINTR on a signal so the loop
     * notices g_stop and exits promptly. Check the returns per house style. */
    if (sigaction(SIGINT,  &sa, NULL) < 0 || sigaction(SIGTERM, &sa, NULL) < 0) {
        perror("sigaction");
        close(fd);
        return EXIT_FAILURE;
    }

    {
        char s[INET_ADDRSTRLEN], p0[INET_ADDRSTRLEN], p1[INET_ADDRSTRLEN];
        struct in_addr a; a.s_addr = cfg.server_ip;
        inet_ntop(AF_INET, &a, s, sizeof s);
        a.s_addr = g_pool[0].ip;             inet_ntop(AF_INET, &a, p0, sizeof p0);
        a.s_addr = g_pool[pool_count-1].ip;  inet_ntop(AF_INET, &a, p1, sizeof p1);
        printf("dhcp-server on %s: server-id %s, pool %s-%s (%d), lease %us\n",
               ifname, s, p0, p1, pool_count, cfg.lease_secs);
    }

    /* The accept loop: block in recvfrom, dispatch, repeat until signalled. */
    uint8_t rxbuf[FRAME_BUF_MAX];
    while (!g_stop) {
        struct sockaddr_ll from;
        socklen_t fromlen = sizeof from;
        ssize_t n = recvfrom(fd, rxbuf, sizeof rxbuf, 0,
                             (struct sockaddr *)&from, &fromlen);
        if (n < 0) {
            if (errno == EINTR)              /* interrupted (maybe our signal)    */
                continue;                    /* loop condition re-checks g_stop   */
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                continue;
            perror("recvfrom");
            break;
        }
        if (from.sll_pkttype == PACKET_OUTGOING) /* skip our own replies         */
            continue;
        process_frame(fd, &cfg, rxbuf, (size_t)n);
    }

    printf("\nshutting down\n");
    close(fd);
    return EXIT_SUCCESS;
}
