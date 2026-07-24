/* ===========================================================================
 * sniffer.c — tcpdump-lite: AF_PACKET capture with a compiled cBPF filter.
 * ===========================================================================
 *
 * WHAT AF_PACKET GIVES YOU
 * ------------------------
 * socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL)) opens a socket that receives a
 * COPY of every frame the kernel's network stack sees on the wire, BEFORE the
 * normal IP/TCP processing strips the headers off. SOCK_RAW means "give me the
 * link-layer header too" (the 14-byte Ethernet header), versus SOCK_DGRAM which
 * cooks it away. ETH_P_ALL (in network byte order, hence htons) means "every
 * ethertype, not just IPv4". This is the same primitive tcpdump/libpcap use on
 * Linux. It requires CAP_NET_RAW (run as root, or `setcap cap_net_raw+ep`).
 *
 * THE PIPELINE THIS PROGRAM BUILDS
 * --------------------------------
 *   1. socket()          — open the AF_PACKET capture socket.            [sys 41]
 *   2. SO_ATTACH_FILTER  — push a compiled classic-BPF program so the    [sys 54]
 *                          KERNEL drops non-matching frames before they
 *                          ever reach us (cheap: no copy for rejects).
 *   3. bind()            — pin capture to one interface via sockaddr_ll. [sys 49]
 *   4. PACKET_ADD_MEMBERSHIP/PROMISC — optionally see frames not addressed
 *                          to us (promiscuous mode).                      [sys 54]
 *   5. PACKET_RX_RING+mmap — set up the zero-copy ring (ring.c).      [sys 54, 9]
 *   6. poll()+drain loop — sleep until frames arrive, decode each.    [sys 7]
 *
 * Everything the kernel does per-packet — run the BPF, copy on accept — is the
 * subject of the assembly deliverable (asm/demo.c is the BPF interpreter step).
 * ===========================================================================
 */

#include "filter.h"
#include "decode.h"
#include "ring.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <poll.h>
#include <sys/socket.h>
#include <arpa/inet.h>          /* htons, ntohs                               */
#include <net/if.h>            /* if_nametoindex                             */
#include <linux/if_ether.h>    /* ETH_P_ALL                                  */
#include <linux/if_packet.h>   /* sockaddr_ll, packet_mreq, tpacket_stats    */

/* SOL_PACKET (263) is the setsockopt level for AF_PACKET; usually from
 * <sys/socket.h>, defined defensively here for stripped libcs. */
#ifndef SOL_PACKET
#define SOL_PACKET 263
#endif

/* ---------------------------------------------------------------------------
 * Signal handling. A capture loop must stop CLEANLY on Ctrl-C so we can print
 * statistics and unmap the ring. The only thing a signal handler may safely
 * touch is a `volatile sig_atomic_t` flag; everything else risks async-signal
 * unsafety (e.g. calling printf from a handler can deadlock on a held lock).
 * The `volatile` forbids the compiler from caching the flag in a register
 * across the loop test; sig_atomic_t guarantees the store is a single,
 * uninterruptible write. --------------------------------------------------- */
static volatile sig_atomic_t g_stop = 0;
static void on_signal(int sig) { (void)sig; g_stop = 1; }

/* Per-run counters passed through ring_drain / recvfrom to the frame callback. */
struct capture_ctx {
    unsigned long shown;    /* frames decoded and printed                     */
    unsigned long max;      /* stop after this many (0 = run until Ctrl-C)    */
};

/* Called once per captured frame. Honors the -c count cap without overshooting
 * the display. */
static void on_frame(const uint8_t *data, uint32_t caplen,
                     uint32_t wirelen, void *user)
{
    struct capture_ctx *c = (struct capture_ctx *)user;
    if (c->max && c->shown >= c->max) return;      /* already hit the cap      */
    printf("#%lu ", c->shown + 1);
    decode_frame(data, caplen, wirelen);
    c->shown++;
}

static void usage(const char *argv0)
{
    fprintf(stderr,
        "usage: %s [-i iface] [-c count] [-p] [--no-mmap] [-d] [filter...]\n"
        "  -i iface    capture on this interface (default: all interfaces)\n"
        "  -c count    stop after `count` matching packets\n"
        "  -p          enable promiscuous mode\n"
        "  --no-mmap   use recvfrom() instead of the PACKET_MMAP ring\n"
        "  -d          compile the filter, dump it (tcpdump -d style), exit\n"
        "  filter      e.g.  tcp port 80   |   host 1.2.3.4   |   udp\n",
        argv0);
}

/* Attach a compiled classic-BPF program to the socket. The kernel copies the
 * instruction array in, verifies it (no backward jumps, in-range offsets, valid
 * opcodes), and from then on runs it on every frame; only frames the program
 * returns non-zero for are queued to us. */
static int attach_filter(int fd, const struct compiled_filter *f)
{
    if (f->len == 0) return 0;                     /* "match all" => no filter */

    /* sock_fprog is the (len, ptr) pair SO_ATTACH_FILTER expects. We cast away
     * const only because the kernel API is not const-correct; the kernel copies
     * the program and never writes through this pointer. */
    struct sock_fprog prog = {
        .len    = f->len,
        .filter = (struct sock_filter *)f->prog,
    };
    /* setsockopt is syscall 54; SOL_SOCKET/SO_ATTACH_FILTER is the classic-BPF
     * attach point (SO_ATTACH_BPF would attach an eBPF fd instead). */
    if (setsockopt(fd, SOL_SOCKET, SO_ATTACH_FILTER, &prog, sizeof prog) < 0) {
        perror("setsockopt(SO_ATTACH_FILTER)");
        return -1;
    }
    return 0;
}

/* Bind the capture socket to one interface so we only see its traffic. Without
 * a bind, an ETH_P_ALL socket captures on ALL interfaces. */
static int bind_iface(int fd, int ifindex)
{
    /* sockaddr_ll is the AF_PACKET address family's sockaddr. For a bind we
     * only need the family, the protocol (again ETH_P_ALL, network order), and
     * the interface index; the MAC fields are for sends, not captures. */
    struct sockaddr_ll sll;
    memset(&sll, 0, sizeof sll);
    sll.sll_family   = AF_PACKET;
    sll.sll_protocol = htons(ETH_P_ALL);           /* big-endian ethertype     */
    sll.sll_ifindex  = ifindex;
    /* bind is syscall 49. EADDRNOTAVAIL here usually means a bad ifindex. */
    if (bind(fd, (struct sockaddr *)&sll, sizeof sll) < 0) {
        perror("bind(AF_PACKET)");
        return -1;
    }
    return 0;
}

/* Turn on promiscuous mode via a packet membership. Unlike the old SIOCSIFFLAGS
 * approach, PACKET_ADD_MEMBERSHIP is refcounted per-socket, so the kernel
 * automatically drops promisc when our socket closes — no cleanup, no leaving
 * the NIC promiscuous if we crash. */
static int set_promisc(int fd, int ifindex)
{
    struct packet_mreq mr;
    memset(&mr, 0, sizeof mr);
    mr.mr_ifindex = ifindex;
    mr.mr_type    = PACKET_MR_PROMISC;
    if (setsockopt(fd, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof mr) < 0) {
        perror("setsockopt(PACKET_ADD_MEMBERSHIP)");
        return -1;
    }
    return 0;
}

/* Print the kernel's own capture accounting: how many frames it delivered and
 * how many it DROPPED because our ring/queue was full (we weren't draining fast
 * enough). Reading PACKET_STATISTICS also RESETS the counters. */
static void print_stats(int fd)
{
    struct tpacket_stats st;
    socklen_t len = sizeof st;
    if (getsockopt(fd, SOL_PACKET, PACKET_STATISTICS, &st, &len) == 0)
        fprintf(stderr, "\n%u packets received by filter, %u dropped by kernel\n",
                st.tp_packets, st.tp_drops);
}

/* The recvfrom() fallback path: one syscall + one copy per packet. Simpler than
 * the ring, and the useful contrast for the README. Returns on Ctrl-C or cap. */
static void capture_recvfrom(int fd, struct capture_ctx *ctx)
{
    /* A frame is at most ~64 KiB (with jumbo/GSO offload it can appear larger,
     * but this is fine for teaching). Stack buffer, no allocation to free. */
    uint8_t buf[65536];
    while (!g_stop && (ctx->max == 0 || ctx->shown < ctx->max)) {
        /* recvfrom is syscall 45. We pass NULL src addr because we don't need
         * to know which interface each frame came from here. */
        ssize_t n = recvfrom(fd, buf, sizeof buf, 0, NULL, NULL);
        if (n < 0) {
            /* EINTR: a signal (probably our SIGINT) interrupted the blocking
             * call. Loop; the while-condition sees g_stop and exits. */
            if (errno == EINTR) continue;
            perror("recvfrom");
            break;
        }
        /* With recvfrom we can't tell a snapped length from the true one, so
         * caplen == wirelen == n. */
        on_frame(buf, (uint32_t)n, (uint32_t)n, ctx);
    }
}

/* The PACKET_MMAP path: poll() to sleep until frames are ready, then drain the
 * ring with zero further syscalls. */
static void capture_ring(int fd, struct ring *r, struct capture_ctx *ctx)
{
    struct pollfd pfd;
    pfd.fd     = fd;
    pfd.events = POLLIN | POLLERR;                  /* wake on data or error    */

    while (!g_stop && (ctx->max == 0 || ctx->shown < ctx->max)) {
        /* First, drain anything already sitting in the ring. Only block in
         * poll() when the ring is momentarily empty — this keeps latency low
         * under bursts. */
        if (ring_drain(r, on_frame, ctx) > 0)
            continue;

        /* poll is syscall 7. Timeout -1 = block indefinitely; a signal makes it
         * return -1/EINTR, which we treat as "re-check g_stop and loop". */
        int rv = poll(&pfd, 1, -1);
        if (rv < 0) {
            if (errno == EINTR) continue;
            perror("poll");
            break;
        }
        /* POLLERR on an AF_PACKET ring typically signals drops; the next
         * getsockopt(PACKET_STATISTICS) will show them. We just loop and drain. */
    }
}

int main(int argc, char **argv)
{
    const char *iface = NULL;
    int   promisc = 0, use_mmap = 1, dump_only = 0;
    unsigned long count = 0;
    char  expr[256] = "";                           /* joined filter tokens     */

    /* -- argument parsing (deliberately tiny, no getopt dependency) --------- */
    int i = 1;
    for (; i < argc; i++) {
        if (!strcmp(argv[i], "-i") && i + 1 < argc)      iface = argv[++i];
        else if (!strcmp(argv[i], "-c") && i + 1 < argc) count = strtoul(argv[++i], NULL, 10);
        else if (!strcmp(argv[i], "-p"))                 promisc = 1;
        else if (!strcmp(argv[i], "--no-mmap"))          use_mmap = 0;
        else if (!strcmp(argv[i], "-d"))                 dump_only = 1;
        else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            usage(argv[0]); return 0;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "unknown option: %s\n", argv[i]);
            usage(argv[0]); return 2;
        } else {
            /* Everything else is part of the filter expression; join with a
             * space so `tcp port 80` (three argv words) reassembles. */
            if (expr[0]) strncat(expr, " ", sizeof expr - strlen(expr) - 1);
            strncat(expr, argv[i], sizeof expr - strlen(expr) - 1);
        }
    }

    /* -- compile the filter first: fail fast on a bad expression ------------ */
    struct compiled_filter filt;
    char errbuf[128];
    if (filter_compile(expr, &filt, errbuf, sizeof errbuf) != 0) {
        fprintf(stderr, "filter error: %s\n", errbuf);
        return 1;
    }

    /* -d: just show what the expression compiled to (like `tcpdump -d`). This
     * needs no privileges — great for understanding the codegen. */
    if (dump_only) {
        if (filt.len == 0) printf("(empty filter — matches all packets)\n");
        else filter_dump(&filt, stdout);
        return 0;
    }

    /* -- open the capture socket ------------------------------------------- *
     * socket() is syscall 41. AF_PACKET+SOCK_RAW+ETH_P_ALL = "every frame,
     * link header included". This is the line that needs CAP_NET_RAW. */
    int fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (fd < 0) {
        perror("socket(AF_PACKET)");
        if (errno == EPERM)
            fprintf(stderr, "  (need root or CAP_NET_RAW: try sudo)\n");
        return 1;
    }

    /* Attach the filter IMMEDIATELY, before bind, to minimize the window in
     * which unfiltered packets could be queued (the socket starts capturing the
     * instant it exists). The fully-race-free idiom attaches a drop-all filter,
     * drains the socket, then swaps in the real filter; we keep it simple and
     * note the tradeoff. */
    if (attach_filter(fd, &filt) != 0) { close(fd); return 1; }

    /* Resolve and bind the interface, if one was named. */
    int ifindex = 0;
    if (iface) {
        ifindex = (int)if_nametoindex(iface);      /* 0 => no such interface    */
        if (ifindex == 0) {
            fprintf(stderr, "no such interface: %s\n", iface);
            close(fd);
            return 1;
        }
        if (bind_iface(fd, ifindex) != 0) { close(fd); return 1; }
        if (promisc && set_promisc(fd, ifindex) != 0) { close(fd); return 1; }
    } else if (promisc) {
        fprintf(stderr, "note: -p needs -i <iface>; ignoring promiscuous mode\n");
    }

    /* Install the SIGINT handler so Ctrl-C stops the loop cleanly. We do NOT
     * set SA_RESTART: we WANT blocking syscalls (poll/recvfrom) to return EINTR
     * so the loop can observe g_stop and exit promptly. */
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = on_signal;
    /* sigaction is syscall 13. It effectively cannot fail for a valid signal +
     * handler, but we honor "check every syscall" and warn rather than abort —
     * a missing handler only means Ctrl-C kills us hard instead of cleanly. */
    if (sigaction(SIGINT,  &sa, NULL) < 0) perror("sigaction(SIGINT)");
    if (sigaction(SIGTERM, &sa, NULL) < 0) perror("sigaction(SIGTERM)");

    fprintf(stderr, "capturing on %s%s%s ... (Ctrl-C to stop)\n",
            iface ? iface : "all interfaces",
            filt.len ? ", filter: " : "",
            filt.len ? expr : "");

    struct capture_ctx ctx = { .shown = 0, .max = count };

    /* -- set up the ring (or fall back to recvfrom) and run ----------------- */
    struct ring r;
    memset(&r, 0, sizeof r);
    if (use_mmap && ring_setup(&r, fd) == 0) {
        capture_ring(fd, &r, &ctx);
        ring_destroy(&r);
    } else {
        if (use_mmap)
            fprintf(stderr, "PACKET_MMAP setup failed; using recvfrom()\n");
        capture_recvfrom(fd, &ctx);
    }

    print_stats(fd);
    fprintf(stderr, "%lu packets shown\n", ctx.shown);

    /* Closing the socket drops the filter, the ring, and (refcounted) promisc
     * mode automatically — the kernel unwinds everything tied to the fd. */
    close(fd);
    return 0;
}
