/* ===========================================================================
 * network.c — give the container real, routable networking: a veth pair + NAT.
 * ===========================================================================
 *
 * A fresh network namespace (CLONE_NEWNET) starts EMPTY except for a down `lo`.
 * To connect the box to the outside world we build the standard bridge-less
 * "veth + NAT" topology every container runtime uses at its core:
 *
 *      host netns                         container netns (child)
 *   ┌───────────────┐                     ┌───────────────┐
 *   │  ceh<pid>     │◄══ veth pair ══════►│  cec<pid>     │
 *   │  10.0.42.1/24 │   (a virtual        │  10.0.42.2/24 │
 *   └──────┬────────┘    "patch cable")   │  default via  │
 *          │  MASQUERADE (SNAT)           │   10.0.42.1   │
 *      ┌───▼────┐                         └───────────────┘
 *      │ eth0   │──► internet
 *      └────────┘
 *
 * A veth pair is two linked virtual NICs: a packet written to one comes out the
 * other. We put one end on the host and MOVE the other into the container's
 * netns. The host end is the container's gateway; a MASQUERADE (source-NAT) rule
 * on the host rewrites the container's private 10.0.42.x source address to the
 * host's, so replies find their way back. That masquerade lives in the same
 * netfilter framework the sibling kernel module hooks into
 * (../../01-kernel/09-netfilter-hook) — POSTROUTING is one of its five hook
 * points; iptables/nft just install rules the kernel evaluates there.
 *
 * WHAT IS DONE IN RAW NETLINK VS. SHELLED OUT (and WHY, honestly):
 *   * Creating the veth pair and moving the peer into the child's netns is done
 *     here in RAW rtnetlink — that is the genuinely low-level, instructive part:
 *     constructing an nlmsghdr + ifinfomsg + NESTED rtattrs (IFLA_LINKINFO ->
 *     IFLA_INFO_KIND="veth" -> IFLA_INFO_DATA -> VETH_INFO_PEER). It is exactly
 *     what `ip link add ... type veth peer ...` does under the hood.
 *   * Assigning addresses/routes and installing the NAT rule are delegated to
 *     `ip`/`iptables` via run_cmd(). Encoding RTM_NEWADDR/RTM_NEWROUTE follows
 *     the identical nested-attribute pattern (a fine exercise), and programming
 *     netfilter rules over raw netlink (xtables/nftables) is a project of its
 *     own — out of scope for a teaching core. Real runtimes delegate this too:
 *     runc hands networking to CNI plugins that ultimately call these same
 *     tools. This is documented, not hidden.
 *
 * ALL PARENT-SIDE. Every step runs in the PARENT while the child is still
 * blocked on the sync pipe. We reach into the child's netns by PID: netlink uses
 * IFLA_NET_NS_PID to move the peer, and `nsenter -t <pid> -n` runs `ip` inside
 * the child's netns. So by the time the child is released, its network is up.
 *
 * NEEDS ROOT. Creating links, moving namespaces, editing iptables, and enabling
 * ip_forward all require real privilege on the host. Best-effort: on failure we
 * warn and the container still runs (just without egress). The README says so.
 * ===========================================================================
 */
#define _GNU_SOURCE
#include "engine.h"
#include "util.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <net/if.h>            /* if_nametoindex                                */
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/if_link.h>     /* IFLA_*, IFLA_INFO_*, IFLA_NET_NS_PID          */
#include <linux/veth.h>        /* VETH_INFO_PEER                                */

/* ---------------------------------------------------------------------------
 * A small rtnetlink request buffer. The message is: a fixed nlmsghdr, then a
 * fixed ifinfomsg, then a variable stream of rtattrs we append into `buf`.
 * 2 KiB is far more than a veth-create message needs. */
struct nlreq {
    struct nlmsghdr  n;
    struct ifinfomsg i;
    char             buf[2048];
};

/* NLMSG_TAIL: the address just past the currently-used bytes of the message,
 * where the next rtattr goes. Netlink aligns everything to 4 bytes. */
#define NLMSG_TAIL(nmsg) \
    ((struct rtattr *)(((char *)(nmsg)) + NLMSG_ALIGN((nmsg)->nlmsg_len)))

/* Append one attribute { type, len, payload } to the message, growing
 * nlmsg_len. Returns -1 if it would overflow `maxlen`. This is the iproute2
 * libnetlink addattr_l pattern, reproduced so the mechanism is visible. */
static int addattr_l(struct nlmsghdr *n, int maxlen, int type,
                     const void *data, int alen)
{
    int len = RTA_LENGTH(alen);          /* payload + the 4-byte rtattr header   */
    if ((int)(NLMSG_ALIGN(n->nlmsg_len) + RTA_ALIGN(len)) > maxlen)
        return -1;
    struct rtattr *rta = NLMSG_TAIL(n);
    rta->rta_type = type;
    rta->rta_len  = len;
    if (alen)
        memcpy(RTA_DATA(rta), data, alen);
    /* Advance the message length past this (aligned) attribute. */
    n->nlmsg_len = NLMSG_ALIGN(n->nlmsg_len) + RTA_ALIGN(len);
    return 0;
}

/* Open a nested attribute: emit a header with no payload yet and return a
 * pointer to it; children are appended after it, then addattr_nest_end() back-
 * patches its length to span them all. This is how IFLA_LINKINFO wraps
 * IFLA_INFO_KIND + IFLA_INFO_DATA, and IFLA_INFO_DATA wraps VETH_INFO_PEER. */
static struct rtattr *addattr_nest(struct nlmsghdr *n, int maxlen, int type)
{
    struct rtattr *nest = NLMSG_TAIL(n);
    addattr_l(n, maxlen, type, NULL, 0);
    return nest;
}
static void addattr_nest_end(struct nlmsghdr *n, struct rtattr *nest)
{
    /* The nest spans from its header up to the current tail. */
    nest->rta_len = (int)((char *)NLMSG_TAIL(n) - (char *)nest);
}

/* ---------------------------------------------------------------------------
 * nl_talk — send one request on a fresh rtnetlink socket and wait for its ACK.
 * Returns 0 on success, or a negative errno the kernel reported. Opening a
 * socket per call is wasteful but keeps each operation self-contained and easy
 * to read (this is a teaching core, not a hot path). */
static int nl_talk(struct nlreq *req)
{
    int fd = socket(AF_NETLINK, SOCK_RAW | SOCK_CLOEXEC, NETLINK_ROUTE);
    if (fd < 0)
        return -errno;

    /* NLM_F_ACK asks the kernel to reply with an NLMSG_ERROR carrying the result
     * (error==0 means success). We fill seq/pid to 0 and let the kernel stamp. */
    req->n.nlmsg_flags |= NLM_F_ACK;
    req->n.nlmsg_seq    = 1;

    struct sockaddr_nl kernel;
    memset(&kernel, 0, sizeof kernel);
    kernel.nl_family = AF_NETLINK;        /* nl_pid=0 => the destination is the kernel */

    /* send the exactly-nlmsg_len bytes we built. */
    ssize_t s = sendto(fd, req, req->n.nlmsg_len, 0,
                       (struct sockaddr *)&kernel, sizeof kernel);
    if (s < 0) { int e = -errno; close(fd); return e; }

    /* Read the ACK. NLMSG_ERROR's payload is a struct nlmsgerr whose `error`
     * field is 0 on success or a negative errno on failure. */
    char resp[4096];
    ssize_t r = recv(fd, resp, sizeof resp, 0);
    close(fd);
    if (r < 0)
        return -errno;

    struct nlmsghdr *rh = (struct nlmsghdr *)resp;
    if (!NLMSG_OK(rh, (unsigned)r))
        return -EIO;
    if (rh->nlmsg_type == NLMSG_ERROR) {
        struct nlmsgerr *err = (struct nlmsgerr *)NLMSG_DATA(rh);
        return err->error;               /* 0 == success                         */
    }
    return 0;
}

/* ---------------------------------------------------------------------------
 * veth_create — create a veth pair {host_name, peer_name} in the current netns.
 *
 * The message mirrors `ip link add <host> type veth peer name <peer>`:
 *   RTM_NEWLINK
 *     IFLA_IFNAME       = host_name
 *     IFLA_LINKINFO {
 *         IFLA_INFO_KIND = "veth"
 *         IFLA_INFO_DATA {
 *             VETH_INFO_PEER {
 *                 struct ifinfomsg (zeroed)   <-- the peer's link header
 *                 IFLA_IFNAME = peer_name
 *             }
 *         }
 *     }
 * The VETH_INFO_PEER value BEGINS with a bare ifinfomsg (that is the veth
 * driver's contract), which is why we bump nlmsg_len by sizeof(ifinfomsg) after
 * opening that nest — reserving a zeroed header — before adding the peer name. */
static int veth_create(const char *host_name, const char *peer_name)
{
    struct nlreq req;
    memset(&req, 0, sizeof req);         /* zero => the reserved peer ifinfomsg is clean */

    req.n.nlmsg_len   = NLMSG_LENGTH(sizeof req.i);
    req.n.nlmsg_type  = RTM_NEWLINK;
    req.n.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL;
    req.i.ifi_family  = AF_UNSPEC;

    int cap = sizeof req;                /* overflow ceiling for addattr_*        */

    /* IFLA_IFNAME for the host end (iproute2 uses strlen, no trailing NUL). */
    addattr_l(&req.n, cap, IFLA_IFNAME, host_name, (int)strlen(host_name));

    struct rtattr *linkinfo = addattr_nest(&req.n, cap, IFLA_LINKINFO);
    addattr_l(&req.n, cap, IFLA_INFO_KIND, "veth", 4);

    struct rtattr *infodata = addattr_nest(&req.n, cap, IFLA_INFO_DATA);
    struct rtattr *peer     = addattr_nest(&req.n, cap, VETH_INFO_PEER);
    /* Reserve the peer's ifinfomsg (already zeroed by the memset above). */
    req.n.nlmsg_len += sizeof(struct ifinfomsg);
    addattr_l(&req.n, cap, IFLA_IFNAME, peer_name, (int)strlen(peer_name));
    addattr_nest_end(&req.n, peer);       /* close VETH_INFO_PEER                 */
    addattr_nest_end(&req.n, infodata);   /* close IFLA_INFO_DATA                 */
    addattr_nest_end(&req.n, linkinfo);   /* close IFLA_LINKINFO                  */

    return nl_talk(&req);
}

/* link_set_netns — move interface `ifname` into the netns owned by process
 * `pid`. Mirrors `ip link set <ifname> netns <pid>`: RTM_SETLINK with the
 * link's index and an IFLA_NET_NS_PID attribute. After this the interface
 * vanishes from the host and appears inside the container's netns. */
static int link_set_netns(const char *ifname, pid_t pid)
{
    unsigned int idx = if_nametoindex(ifname);   /* resolve name -> ifindex      */
    if (idx == 0)
        return -errno;

    struct nlreq req;
    memset(&req, 0, sizeof req);
    req.n.nlmsg_len   = NLMSG_LENGTH(sizeof req.i);
    req.n.nlmsg_type  = RTM_SETLINK;
    req.n.nlmsg_flags = NLM_F_REQUEST;
    req.i.ifi_family  = AF_UNSPEC;
    req.i.ifi_index   = (int)idx;                /* which link to modify          */

    unsigned int ns_pid = (unsigned int)pid;
    addattr_l(&req.n, sizeof req, IFLA_NET_NS_PID, &ns_pid, sizeof ns_pid);

    return nl_talk(&req);
}

/* ---------------------------------------------------------------------------
 * runv — variadic front-end to run_cmd(): pass a NULL-terminated list of argv
 * strings. Keeps the plumbing below readable (no manual argv[] arrays). */
static int runv(int quiet, ...)
{
    char *argv[24];
    int n = 0;
    va_list ap;
    va_start(ap, quiet);
    char *a;
    while ((a = va_arg(ap, char *)) != NULL && n < 23)
        argv[n++] = a;
    va_end(ap);
    argv[n] = NULL;
    return run_cmd(argv, quiet);
}

int network_setup(struct engine_cfg *cfg, pid_t child)
{
    if (!cfg->net_enable)
        return 0;                         /* opt-out: leave the netns with only lo */

    /* Interface names, unique per child pid and safely under IFNAMSIZ (16). */
    snprintf(cfg->veth_host, sizeof cfg->veth_host, "ceh%d", (int)(child % 100000));
    snprintf(cfg->veth_cont, sizeof cfg->veth_cont, "cec%d", (int)(child % 100000));

    /* (1) Create the veth pair on the host (raw netlink). */
    int rc = veth_create(cfg->veth_host, cfg->veth_cont);
    if (rc != 0) {
        errno = -rc;
        warn("veth_create (need root; is another run using this name?)");
        return -1;
    }

    /* (2) Move the container end into the child's netns (raw netlink, by pid). */
    rc = link_set_netns(cfg->veth_cont, child);
    if (rc != 0) {
        errno = -rc;
        warn("move veth into container netns");
        network_cleanup(cfg);             /* undo the half-built pair             */
        return -1;
    }

    /* Precompute the strings the `ip`/`iptables` steps need. The /24 prefix is
     * fixed to match net_subnet; documented in the README's honest-scope note. */
    char host_cidr[32], cont_cidr[32], pidstr[16];
    snprintf(host_cidr, sizeof host_cidr, "%s/24", cfg->host_ip);
    snprintf(cont_cidr, sizeof cont_cidr, "%s/24", cfg->cont_ip);
    snprintf(pidstr,    sizeof pidstr,    "%d", (int)child);

    /* (3) Host end: bring it up and give it the gateway address. */
    runv(0, "ip", "link", "set", cfg->veth_host, "up", (char *)NULL);
    runv(0, "ip", "addr", "add", host_cidr, "dev", cfg->veth_host, (char *)NULL);

    /* (4) Container end: configure it from the host by entering the child's
     *     netns with nsenter -t <pid> -n. Bring up lo and the veth, address it,
     *     and add a default route out through the host end. */
    runv(0, "nsenter", "-t", pidstr, "-n", "ip", "link", "set", "lo", "up", (char *)NULL);
    runv(0, "nsenter", "-t", pidstr, "-n", "ip", "link", "set", cfg->veth_cont, "up", (char *)NULL);
    runv(0, "nsenter", "-t", pidstr, "-n", "ip", "addr", "add", cont_cidr, "dev", cfg->veth_cont, (char *)NULL);
    runv(0, "nsenter", "-t", pidstr, "-n", "ip", "route", "add", "default", "via", cfg->host_ip, (char *)NULL);

    /* (5) NAT: enable IPv4 forwarding, then MASQUERADE the container subnet on
     *     egress (any interface that is NOT the host veth). The -C probe (quiet)
     *     checks whether the rule already exists so repeated runs do not stack
     *     duplicates; if it is absent (-C fails) we -A append it. */
    runv(0, "sysctl", "-w", "net.ipv4.ip_forward=1", (char *)NULL);

    char *subnet = (char *)cfg->net_subnet;
    if (runv(1, "iptables", "-t", "nat", "-C", "POSTROUTING",
             "-s", subnet, "!", "-o", cfg->veth_host, "-j", "MASQUERADE", (char *)NULL) != 0) {
        runv(0, "iptables", "-t", "nat", "-A", "POSTROUTING",
             "-s", subnet, "!", "-o", cfg->veth_host, "-j", "MASQUERADE", (char *)NULL);
    }
    /* Some hosts default the FORWARD chain to DROP; explicitly accept traffic to
     * and from the container veth so egress + replies pass. */
    if (runv(1, "iptables", "-C", "FORWARD", "-i", cfg->veth_host, "-j", "ACCEPT", (char *)NULL) != 0)
        runv(0, "iptables", "-A", "FORWARD", "-i", cfg->veth_host, "-j", "ACCEPT", (char *)NULL);
    if (runv(1, "iptables", "-C", "FORWARD", "-o", cfg->veth_host, "-j", "ACCEPT", (char *)NULL) != 0)
        runv(0, "iptables", "-A", "FORWARD", "-o", cfg->veth_host, "-j", "ACCEPT", (char *)NULL);

    cfg->nat_added = 1;                    /* remember to remove our rules later   */
    return 0;
}

void network_cleanup(struct engine_cfg *cfg)
{
    if (!cfg->net_enable)
        return;

    /* Remove only the rules we added (quiet: they may already be gone). */
    if (cfg->nat_added) {
        char *subnet = (char *)cfg->net_subnet;
        runv(1, "iptables", "-t", "nat", "-D", "POSTROUTING",
             "-s", subnet, "!", "-o", cfg->veth_host, "-j", "MASQUERADE", (char *)NULL);
        runv(1, "iptables", "-D", "FORWARD", "-i", cfg->veth_host, "-j", "ACCEPT", (char *)NULL);
        runv(1, "iptables", "-D", "FORWARD", "-o", cfg->veth_host, "-j", "ACCEPT", (char *)NULL);
    }

    /* Deleting either end of a veth pair removes BOTH; the container end already
     * vanished when its netns was destroyed at child exit, so we delete the host
     * end. Quiet, because it may already be gone. */
    if (cfg->veth_host[0])
        runv(1, "ip", "link", "del", cfg->veth_host, (char *)NULL);
}
