/* ===========================================================================
 * dhcp_client.c — a DHCP client that performs the DORA exchange from scratch.
 * ===========================================================================
 *
 * THE PROBLEM. A freshly-booted host has a link (an Ethernet NIC) but no IP
 * address, no netmask, no gateway, no DNS. It must obtain them from a DHCP
 * server it has never heard of. It cannot use a normal UDP socket, because:
 *   - it has no source IP to bind to, and
 *   - it must reach the *limited broadcast* 255.255.255.255, which the routing
 *     table on an unconfigured host cannot resolve.
 * So the client speaks Ethernet directly through an AF_PACKET raw socket and
 * hand-builds every byte of every frame.
 *
 * THE DANCE (RFC 2131 §3.1), all four messages broadcast on the LAN:
 *   D  DISCOVER  client -> 255.255.255.255:67  "any servers? here is my MAC + xid"
 *   O  OFFER     server -> client              "you may have yiaddr, lease T"
 *   R  REQUEST   client -> 255.255.255.255:67  "I request yiaddr from server-id"
 *   A  ACK       server -> client              "confirmed, yiaddr is yours"
 * The xid (a random 32-bit transaction id) threads all four together.
 *
 * PLATFORM: Linux only. AF_PACKET needs CAP_NET_RAW (run as root, or grant the
 * binary `setcap cap_net_raw+ep`). See README.md.
 * ===========================================================================
 */
#include "dhcp.h"

#include <stdio.h>            /* printf, perror, fprintf                         */
#include <stdlib.h>          /* exit, EXIT_*                                    */
#include <string.h>          /* memcpy, memset, memcmp                          */
#include <unistd.h>          /* close, read                                     */
#include <errno.h>           /* errno, EINTR, EAGAIN                            */
#include <time.h>            /* clock_gettime for retransmit timing            */
#include <poll.h>            /* poll — wait for a reply with a timeout          */

#include <sys/socket.h>      /* socket, bind, recvfrom, sendto                  */
#include <sys/ioctl.h>       /* ioctl (SIOCGIFHWADDR to read our MAC)           */
#include <sys/random.h>      /* getrandom — a good random xid                   */
#include <arpa/inet.h>       /* inet_ntop for pretty-printing addresses         */
#include <net/if.h>          /* struct ifreq, if_nametoindex, IFNAMSIZ          */
#include <linux/if_packet.h> /* struct sockaddr_ll — the AF_PACKET address      */
#include <linux/if_ether.h>  /* ETH_P_IP, ETH_P_ALL                             */

/* The 6-byte Ethernet broadcast MAC. Every NIC on the segment accepts it, so a
 * client with no idea where the server lives can still reach it. */
static const uint8_t BROADCAST_MAC[MAC_LEN] = {0xff,0xff,0xff,0xff,0xff,0xff};

/* How long to wait for each reply, and how many times to retry before giving up.
 * Real clients use exponential backoff (RFC 2131 §4.1); we keep it simple. */
#define REPLY_TIMEOUT_MS 3000
#define MAX_TRIES        4

/* ---------------------------------------------------------------------------
 * open_packet_socket — create the AF_PACKET raw socket, learn our ifindex + MAC.
 *
 * socket(2): AF_PACKET gives us frames at the link layer; SOCK_RAW means the
 * Ethernet header is included (vs SOCK_DGRAM where the kernel cooks it). The
 * protocol argument htons(ETH_P_ALL) asks to receive every EtherType; we narrow
 * later by binding to ETH_P_IP. socket() returns a fd (>=0) or -1/errno.
 *
 * Returns the fd on success, or -1 on failure (message already printed).
 * On success *ifindex and mac[6] are filled.
 * --------------------------------------------------------------------------- */
static int open_packet_socket(const char *ifname, int *ifindex, uint8_t mac[MAC_LEN])
{
    /* Third arg is the EtherType filter, in NETWORK order — hence htons_. */
    int fd = socket(AF_PACKET, SOCK_RAW, htons_(ETH_P_ALL));
    if (fd < 0) {
        perror("socket(AF_PACKET, SOCK_RAW)");  /* usually EPERM: need CAP_NET_RAW */
        return -1;
    }

    /* Translate the interface name ("eth0") to its kernel index. if_nametoindex
     * wraps an ioctl; returns 0 on error and sets errno. */
    unsigned idx = if_nametoindex(ifname);
    if (idx == 0) {
        perror("if_nametoindex");
        close(fd);
        return -1;
    }
    *ifindex = (int)idx;

    /* Read the interface's hardware (MAC) address with ioctl SIOCGIFHWADDR.
     * We copy the name into ifr_name (bounded by IFNAMSIZ), then the kernel
     * fills ifr_hwaddr.sa_data with the 6 MAC bytes. */
    struct ifreq ifr;
    memset(&ifr, 0, sizeof ifr);
    /* strncpy leaves room for the implicit NUL because ifr_name is IFNAMSIZ and
     * we copy at most IFNAMSIZ-1. */
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
    if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0) {
        perror("ioctl(SIOCGIFHWADDR)");
        close(fd);
        return -1;
    }
    memcpy(mac, ifr.ifr_hwaddr.sa_data, MAC_LEN);

    /* Bind the socket to this interface and to IPv4 frames only. bind() with a
     * sockaddr_ll restricts both which interface we transmit on and which frames
     * we receive, so we don't see loopback or other NICs' traffic. */
    struct sockaddr_ll sll;
    memset(&sll, 0, sizeof sll);
    sll.sll_family   = AF_PACKET;
    sll.sll_ifindex  = *ifindex;
    sll.sll_protocol = htons_(ETH_P_IP);        /* only EtherType 0x0800 now     */
    if (bind(fd, (struct sockaddr *)&sll, sizeof sll) < 0) {
        perror("bind(AF_PACKET)");
        close(fd);
        return -1;
    }
    return fd;
}

/* ---------------------------------------------------------------------------
 * send_frame — push a fully-built frame out via sendto(2).
 *
 * For AF_PACKET the destination sockaddr_ll carries the ifindex and the target
 * MAC. sendto() returns the number of bytes queued or -1/errno. A short write
 * on a packet socket is not expected (a datagram is atomic), but we still verify
 * the full length went out.
 * --------------------------------------------------------------------------- */
static int send_frame(int fd, int ifindex, const uint8_t dst_mac[MAC_LEN],
                      const uint8_t *frame, size_t len)
{
    struct sockaddr_ll sll;
    memset(&sll, 0, sizeof sll);
    sll.sll_family   = AF_PACKET;
    sll.sll_ifindex  = ifindex;
    sll.sll_halen    = MAC_LEN;                 /* address length = 6            */
    sll.sll_protocol = htons_(ETH_P_IP);
    memcpy(sll.sll_addr, dst_mac, MAC_LEN);     /* the L2 destination            */

    for (;;) {
        ssize_t n = sendto(fd, frame, len, 0,
                           (struct sockaddr *)&sll, sizeof sll);
        if (n < 0) {
            if (errno == EINTR)                 /* interrupted by a signal: retry */
                continue;
            perror("sendto");
            return -1;
        }
        if ((size_t)n != len) {                 /* partial send: treat as error   */
            fprintf(stderr, "sendto: short write %zd/%zu\n", n, len);
            return -1;
        }
        return 0;
    }
}

/* ---------------------------------------------------------------------------
 * parse_reply — validate a received frame and, if it is a DHCP reply for us
 * with the wanted message type and matching xid, hand back the DHCP message and
 * a pointer/length into its options area.
 *
 * This is defensive parsing: a raw socket sees EVERY IPv4 frame on the segment,
 * so we reject anything that is not a well-formed UDP/DHCP BOOTREPLY to port 68
 * for our transaction. Every length check guards a potential over-read.
 *
 * Returns 1 on a matching reply, 0 to ignore this frame.
 * --------------------------------------------------------------------------- */
static int parse_reply(const uint8_t *buf, size_t len, uint32_t want_xid,
                       uint8_t want_type,
                       const struct dhcp_msg **out_msg,
                       const uint8_t **out_opts, size_t *out_opts_len)
{
    /* Must be at least Ethernet + IP + UDP + the fixed DHCP part + cookie. */
    if (len < ETH_HDR_LEN + IP_HDR_LEN + UDP_HDR_LEN + DHCP_FIXED_LEN + 4)
        return 0;

    const struct eth_hdr *eth = (const struct eth_hdr *)buf;
    if (eth->ethertype != htons_(ETHERTYPE_IPV4))   /* not IPv4 -> ignore        */
        return 0;

    const struct ip_hdr *ip = (const struct ip_hdr *)(buf + ETH_HDR_LEN);
    if ((ip->ver_ihl >> 4) != 4)                    /* not IPv4 version          */
        return 0;
    if (ip->proto != IPPROTO_UDP_)                  /* not UDP                   */
        return 0;
    /* Honour the real header length (IHL) in case a server emitted IP options,
     * so we locate UDP correctly rather than assuming 20 bytes. */
    size_t ihl = (size_t)(ip->ver_ihl & 0x0F) * 4;
    if (ihl < IP_HDR_LEN)
        return 0;
    /* Re-verify the full frame is present given the ACTUAL header length: if a
     * server emitted IP options (ihl > 20) the DHCP payload starts later, so the
     * fixed-length check above (which assumed 20) is no longer sufficient. */
    if (len < ETH_HDR_LEN + ihl + UDP_HDR_LEN + DHCP_FIXED_LEN + 4)
        return 0;

    const struct udp_hdr *udp =
        (const struct udp_hdr *)(buf + ETH_HDR_LEN + ihl);
    if (udp->dport != htons_(DHCP_CLIENT_PORT))     /* not addressed to us (68)  */
        return 0;
    if (udp->sport != htons_(DHCP_SERVER_PORT))     /* not from a server (67)    */
        return 0;

    const struct dhcp_msg *msg =
        (const struct dhcp_msg *)(buf + ETH_HDR_LEN + ihl + UDP_HDR_LEN);
    if (msg->op != BOOTREPLY)                       /* replies only              */
        return 0;
    if (msg->xid != want_xid)                       /* another client's exchange */
        return 0;

    /* Verify and step over the 4-byte magic cookie that precedes the options. */
    const uint8_t *cookie = (const uint8_t *)msg + DHCP_FIXED_LEN;
    uint32_t got = ((uint32_t)cookie[0] << 24) | ((uint32_t)cookie[1] << 16) |
                   ((uint32_t)cookie[2] << 8)  |  (uint32_t)cookie[3];
    if (got != DHCP_MAGIC_COOKIE)                   /* not DHCP options          */
        return 0;

    const uint8_t *opts = cookie + 4;
    /* Options run from just past the cookie to the end of the captured frame. */
    size_t opts_len = len - (size_t)(opts - buf);

    /* Require the wanted DHCP message type (option 53). */
    uint8_t tlen = 0;
    const uint8_t *mt = dhcp_opt_find(opts, opts_len, DHCP_OPT_MSG_TYPE, &tlen);
    if (!mt || tlen != 1 || mt[0] != want_type)
        return 0;

    *out_msg      = msg;
    *out_opts     = opts;
    *out_opts_len = opts_len;
    return 1;
}

/* ---------------------------------------------------------------------------
 * wait_for_reply — poll(2) the socket until a matching reply arrives or we time
 * out. Loops over unrelated frames (raw socket sees them all) without resetting
 * the deadline, so background chatter can't starve us of our timeout budget.
 *
 * Returns 1 (got it, outputs filled), 0 (timed out), or -1 (fatal error).
 * --------------------------------------------------------------------------- */
static int wait_for_reply(int fd, uint32_t xid, uint8_t want_type,
                          int timeout_ms, uint8_t *rxbuf, size_t rxcap,
                          const struct dhcp_msg **out_msg,
                          const uint8_t **out_opts, size_t *out_opts_len)
{
    /* Compute an absolute deadline so re-polling after ignoring a stray frame
     * subtracts the elapsed time rather than restarting the clock. */
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (;;) {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        long elapsed_ms = (now.tv_sec - start.tv_sec) * 1000 +
                          (now.tv_nsec - start.tv_nsec) / 1000000;
        int remaining = timeout_ms - (int)elapsed_ms;
        if (remaining <= 0)
            return 0;                       /* deadline reached, no match        */

        struct pollfd pfd = { .fd = fd, .events = POLLIN, .revents = 0 };
        int pr = poll(&pfd, 1, remaining);
        if (pr < 0) {
            if (errno == EINTR)             /* signal: recompute remaining, retry */
                continue;
            perror("poll");
            return -1;
        }
        if (pr == 0)                        /* poll itself timed out              */
            return 0;

        /* Data is ready. recvfrom fills rxbuf with a whole frame and, via the
         * sockaddr_ll, tells us the packet type (we skip our OWN outgoing
         * frames, which a raw socket also delivers). */
        struct sockaddr_ll from;
        socklen_t fromlen = sizeof from;
        ssize_t n = recvfrom(fd, rxbuf, rxcap, 0,
                             (struct sockaddr *)&from, &fromlen);
        if (n < 0) {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
                continue;                   /* transient: try again              */
            perror("recvfrom");
            return -1;
        }
        if (from.sll_pkttype == PACKET_OUTGOING) /* our own echoed frame         */
            continue;

        if (parse_reply(rxbuf, (size_t)n, xid, want_type,
                        out_msg, out_opts, out_opts_len))
            return 1;                        /* a real match — done              */
        /* Otherwise loop and keep waiting within the same deadline. */
    }
}

/* ---------------------------------------------------------------------------
 * build_dhcp — fill the fixed BOOTP part + magic cookie shared by DISCOVER and
 * REQUEST. Returns the offset at which the caller should keep appending options
 * (i.e. just after the cookie). `msgtype` (53) is written as the first option.
 * --------------------------------------------------------------------------- */
static size_t build_dhcp(uint8_t *pkt, uint32_t xid, const uint8_t mac[MAC_LEN],
                         uint8_t msgtype)
{
    struct dhcp_msg *m = (struct dhcp_msg *)pkt;
    memset(m, 0, sizeof *m);
    m->op    = BOOTREQUEST;                 /* client -> server                  */
    m->htype = HTYPE_ETHERNET;              /* 1                                 */
    m->hlen  = MAC_LEN;                      /* 6                                 */
    m->xid   = xid;                          /* same across the whole exchange    */
    m->flags = htons_(DHCP_FLAG_BROADCAST);  /* ask the server to broadcast back  */
    memcpy(m->chaddr, mac, MAC_LEN);         /* our MAC; server keys leases on it */

    /* Magic cookie, written MSB-first so the four wire bytes read 63 82 53 63. */
    uint8_t *cookie = pkt + DHCP_FIXED_LEN;
    cookie[0] = (DHCP_MAGIC_COOKIE >> 24) & 0xFF;
    cookie[1] = (DHCP_MAGIC_COOKIE >> 16) & 0xFF;
    cookie[2] = (DHCP_MAGIC_COOKIE >> 8)  & 0xFF;
    cookie[3] =  DHCP_MAGIC_COOKIE        & 0xFF;

    uint8_t *opts = pkt + DHCP_FIXED_LEN + 4;
    size_t off = 0;
    /* Option 53 (message type) must come first by convention. */
    off = dhcp_opt_append_u8(opts, off, DHCP_OPT_MSG_TYPE, msgtype);
    return off;                              /* option offset, relative to opts   */
}

/* Pretty-print a network-order IPv4 into a caller buffer (>= INET_ADDRSTRLEN). */
static const char *ip_str(uint32_t net, char *buf, size_t cap)
{
    struct in_addr a;
    a.s_addr = net;                          /* inet_ntop wants network order     */
    if (!inet_ntop(AF_INET, &a, buf, (socklen_t)cap))
        return "?";
    return buf;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s <interface>   (e.g. %s eth0)\n",
                argv[0], argv[0]);
        return EXIT_FAILURE;
    }
    const char *ifname = argv[1];

    int ifindex = 0;
    uint8_t mac[MAC_LEN];
    int fd = open_packet_socket(ifname, &ifindex, mac);
    if (fd < 0)
        return EXIT_FAILURE;

    printf("dhcp-client on %s (index %d) MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
           ifname, ifindex, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    /* A random transaction id ties our four messages together and lets us
     * ignore other clients' exchanges. getrandom() draws from the kernel CSPRNG;
     * we check the byte count returned. */
    uint32_t xid = 0;
    if (getrandom(&xid, sizeof xid, 0) != (ssize_t)sizeof xid) {
        perror("getrandom");
        close(fd);
        return EXIT_FAILURE;
    }

    uint8_t frame[FRAME_BUF_MAX];   /* the wire frame we transmit               */
    uint8_t rxbuf[FRAME_BUF_MAX];   /* a received frame                         */
    uint8_t dhcp[FRAME_BUF_MAX];    /* the DHCP payload we build                */

    const struct dhcp_msg *msg = NULL;
    const uint8_t *opts = NULL;
    size_t opts_len = 0;

    /* ---- D: DISCOVER --------------------------------------------------------
     * Broadcast at both L2 (dst MAC ff:..:ff) and L3 (dst IP 255.255.255.255),
     * from 0.0.0.0:68 to 255.255.255.255:67. We include a Parameter Request
     * List (option 55) naming the config we want (mask, router, DNS). */
    size_t off = build_dhcp(dhcp, xid, mac, DHCPDISCOVER);
    uint8_t *o = dhcp + DHCP_FIXED_LEN + 4;
    {
        const uint8_t wishlist[] = { DHCP_OPT_SUBNET_MASK,
                                     DHCP_OPT_ROUTER,
                                     DHCP_OPT_DNS,
                                     DHCP_OPT_LEASE_TIME };
        off = dhcp_opt_append(o, off, DHCP_OPT_PARAM_REQ_LIST,
                              wishlist, sizeof wishlist);
    }
    off = dhcp_opt_end(o, off);
    size_t dhcp_len = DHCP_FIXED_LEN + 4 + off;

    /* yiaddr the server ends up offering, and the server's identity (option 54),
     * captured from the OFFER and echoed in the REQUEST. */
    uint32_t offered_ip = 0;
    uint32_t server_id  = 0;

    int got = 0;
    for (int try = 0; try < MAX_TRIES && !got; try++) {
        size_t flen = dhcp_build_frame(frame, BROADCAST_MAC, mac,
                                       htonl_(0x00000000),   /* src 0.0.0.0      */
                                       htonl_(0xFFFFFFFF),   /* dst 255.255.255.255 */
                                       DHCP_CLIENT_PORT, DHCP_SERVER_PORT,
                                       dhcp, dhcp_len);
        /* xid is just 32 random bits; print the raw value for correlation. */
        printf("-> DISCOVER (xid 0x%08x, try %d)\n", xid, try + 1);
        if (send_frame(fd, ifindex, BROADCAST_MAC, frame, flen) < 0) {
            close(fd);
            return EXIT_FAILURE;
        }
        int r = wait_for_reply(fd, xid, DHCPOFFER, REPLY_TIMEOUT_MS,
                               rxbuf, sizeof rxbuf, &msg, &opts, &opts_len);
        if (r < 0) { close(fd); return EXIT_FAILURE; }
        if (r == 1) got = 1;
        else printf("   (no OFFER, retrying)\n");
    }
    if (!got) {
        fprintf(stderr, "no DHCP OFFER received; giving up\n");
        close(fd);
        return EXIT_FAILURE;
    }

    /* Parse the OFFER: yiaddr is the offered address; option 54 identifies the
     * server we must name in the REQUEST. */
    offered_ip = msg->yiaddr;
    {
        uint8_t l = 0;
        const uint8_t *sid = dhcp_opt_find(opts, opts_len, DHCP_OPT_SERVER_ID, &l);
        if (sid && l == 4)
            memcpy(&server_id, sid, 4);       /* stays in network order          */
    }
    {
        char b[INET_ADDRSTRLEN];
        printf("<- OFFER  yiaddr %s\n", ip_str(offered_ip, b, sizeof b));
    }

    /* ---- R: REQUEST ---------------------------------------------------------
     * Still broadcast (other servers must see that we declined their offers).
     * We add option 50 (requested IP = the offered yiaddr) and option 54
     * (server identifier), which together say "I accept THIS offer from THIS
     * server." ciaddr stays 0 during initial acquisition (RFC 2131 §4.3.2). */
    off = build_dhcp(dhcp, xid, mac, DHCPREQUEST);
    o = dhcp + DHCP_FIXED_LEN + 4;
    off = dhcp_opt_append_u32(o, off, DHCP_OPT_REQUESTED_IP, offered_ip);
    if (server_id)
        off = dhcp_opt_append_u32(o, off, DHCP_OPT_SERVER_ID, server_id);
    {
        const uint8_t wishlist[] = { DHCP_OPT_SUBNET_MASK, DHCP_OPT_ROUTER,
                                     DHCP_OPT_DNS, DHCP_OPT_LEASE_TIME };
        off = dhcp_opt_append(o, off, DHCP_OPT_PARAM_REQ_LIST,
                              wishlist, sizeof wishlist);
    }
    off = dhcp_opt_end(o, off);
    dhcp_len = DHCP_FIXED_LEN + 4 + off;

    got = 0;
    for (int try = 0; try < MAX_TRIES && !got; try++) {
        size_t flen = dhcp_build_frame(frame, BROADCAST_MAC, mac,
                                       htonl_(0x00000000),
                                       htonl_(0xFFFFFFFF),
                                       DHCP_CLIENT_PORT, DHCP_SERVER_PORT,
                                       dhcp, dhcp_len);
        {
            char b[INET_ADDRSTRLEN];
            printf("-> REQUEST %s (try %d)\n", ip_str(offered_ip, b, sizeof b), try + 1);
        }
        if (send_frame(fd, ifindex, BROADCAST_MAC, frame, flen) < 0) {
            close(fd);
            return EXIT_FAILURE;
        }
        int r = wait_for_reply(fd, xid, DHCPACK, REPLY_TIMEOUT_MS,
                               rxbuf, sizeof rxbuf, &msg, &opts, &opts_len);
        if (r < 0) { close(fd); return EXIT_FAILURE; }
        if (r == 1) got = 1;
        else printf("   (no ACK, retrying)\n");
    }
    if (!got) {
        fprintf(stderr, "no DHCP ACK received; lease not obtained\n");
        close(fd);
        return EXIT_FAILURE;
    }

    /* ---- A: ACK — the lease is ours. Print the bound configuration. --------- */
    {
        char b1[INET_ADDRSTRLEN], b2[INET_ADDRSTRLEN];
        printf("<- ACK    lease bound\n");
        printf("   address : %s\n", ip_str(msg->yiaddr, b1, sizeof b1));

        uint8_t l = 0;
        const uint8_t *mask = dhcp_opt_find(opts, opts_len, DHCP_OPT_SUBNET_MASK, &l);
        if (mask && l == 4) {
            uint32_t m; memcpy(&m, mask, 4);
            printf("   netmask : %s\n", ip_str(m, b2, sizeof b2));
        }
        const uint8_t *rtr = dhcp_opt_find(opts, opts_len, DHCP_OPT_ROUTER, &l);
        if (rtr && l >= 4) {
            uint32_t g; memcpy(&g, rtr, 4);
            printf("   gateway : %s\n", ip_str(g, b2, sizeof b2));
        }
        const uint8_t *dns = dhcp_opt_find(opts, opts_len, DHCP_OPT_DNS, &l);
        if (dns && l >= 4) {
            uint32_t d; memcpy(&d, dns, 4);
            printf("   dns     : %s\n", ip_str(d, b2, sizeof b2));
        }
        const uint8_t *lt = dhcp_opt_find(opts, opts_len, DHCP_OPT_LEASE_TIME, &l);
        if (lt && l == 4) {
            uint32_t secs;
            memcpy(&secs, lt, 4);
            printf("   lease   : %u seconds\n", ntohl_(secs)); /* wire is net order */
        }
    }

    close(fd);
    return EXIT_SUCCESS;
}
