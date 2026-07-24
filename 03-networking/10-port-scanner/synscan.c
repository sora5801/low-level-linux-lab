/* ===========================================================================
 * synscan.c — a raw-socket TCP SYN (half-open) port scanner, with a connect()
 *             fallback. TEACHING CODE: read it top-to-bottom.
 * ===========================================================================
 *
 * ⚠  AUTHORIZED TARGETS ONLY. Scanning hosts you do not own or have written
 *    permission to test is, in many jurisdictions, illegal and against every
 *    cloud/ISP acceptable-use policy. Point this at 127.0.0.1, your own boxes,
 *    or an explicitly sanctioned lab. See the README.
 *
 * WHAT IT DOES
 * ------------
 * Two scan engines that answer the same question ("is TCP port P open?") at two
 * different layers of the stack:
 *
 *   -sS  SYN / half-open scan (the interesting one; needs CAP_NET_RAW / root):
 *        We build the IP and TCP headers OURSELVES in a buffer, hand them to a
 *        raw socket opened with IP_HDRINCL, and send a lone SYN. We then read
 *        raw inbound TCP with the SAME socket, BELOW the kernel's TCP state
 *        machine, and classify the reply by its flags:
 *            SYN|ACK  -> port is OPEN   (the peer wants to complete the handshake)
 *            RST      -> port is CLOSED (the peer refuses)
 *            (silence)-> port is FILTERED (a firewall dropped SYN or the reply)
 *        We never send the final ACK, so the connection is never established —
 *        hence "half-open". This is stealthier and faster than a full connect.
 *
 *   -sT  connect() scan (the portable fallback; needs NO privileges):
 *        We ask the KERNEL to do a normal non-blocking connect() and just watch
 *        the outcome: success -> OPEN, ECONNREFUSED -> CLOSED, timeout -> FILTERED.
 *        Slower and noisier (it completes the handshake, so the peer's accept()
 *        sees it) but works as an ordinary user on any host.
 *
 * THE SUBSYSTEMS THIS EXERCISES (and where each lives below)
 *   * raw sockets + IP_HDRINCL ................. open_raw_socket(), build_syn_packet()
 *   * the Internet ones-complement checksum .... sum16(), fold_csum(), *_checksum()
 *   * the TCP pseudo-header .................... tcp_checksum()
 *   * SO_RCVTIMEO / non-blocking + select ...... syn_scan(), connect_scan()
 *   * response classification below the stack .. classify_reply()
 *   * send-rate limiting ....................... sleep_between_probes()
 *
 * Platform: LINUX. It uses Linux raw-socket semantics (a SOCK_RAW/IPPROTO_TCP
 * socket receives a copy of every inbound TCP segment) and IP_HDRINCL. Build and
 * run under Linux or WSL2. The teaching assembly in asm/ regenerates on any host.
 * ===========================================================================
 */

#define _GNU_SOURCE          /* for getaddrinfo() flags and friends */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>

#include <sys/socket.h>      /* socket, setsockopt, sendto, recv                 */
#include <sys/types.h>
#include <netinet/in.h>      /* IPPROTO_TCP, sockaddr_in, in_addr                */
#include <arpa/inet.h>       /* htons/htonl/ntohs, inet_ntop                     */
#include <netdb.h>           /* getaddrinfo (resolve a host or dotted-quad)      */
#include <fcntl.h>           /* fcntl, O_NONBLOCK (non-blocking connect)         */
#include <sys/select.h>      /* select (bounded wait on the connect)             */

/* Our fixed-width aliases; the kernel headers already gave us the standard ones,
 * but short names keep the packet-building code aligned and readable. */
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;

/* ---------------------------------------------------------------------------
 * TCP flag bits. The 13th byte of the TCP header is a bitmask of control flags.
 * A SYN scan cares about exactly three of them. (Values are the on-the-wire bit
 * positions, so `flags & TH_SYN` tests the SYN bit directly.)
 * --------------------------------------------------------------------------- */
#define TH_FIN 0x01u
#define TH_SYN 0x02u   /* "synchronize sequence numbers" — opens a connection    */
#define TH_RST 0x04u   /* "reset" — abort; a closed port answers a SYN with this */
#define TH_PSH 0x08u
#define TH_ACK 0x10u   /* "acknowledgement field is valid"                       */
#define TH_URG 0x20u

/* Defaults (all overridable on the command line). */
#define DEF_PORTS   "1-1024"
#define DEF_RATE    1000      /* probes per second (0 = as fast as possible)      */
#define DEF_WAIT_MS 1000      /* how long to wait for replies, in milliseconds    */
#define PKT_LEN     40u       /* 20-byte IP header + 20-byte TCP header, no options*/

/* ===========================================================================
 * SECTION 1 — the packet headers, laid out to the byte.
 * ===========================================================================
 * We declare our own PACKED structs instead of using <netinet/ip.h>'s `struct
 * iphdr` / <netinet/tcp.h>'s `struct tcphdr` for two teaching reasons: (1) the
 * field layout — and every byte offset — is right here in front of you, and
 * (2) we sidestep those headers' endian-dependent bitfields for the version/IHL
 * and data-offset nibbles, which are a portability trap. `__attribute__((packed))`
 * forces the compiler to lay the fields out with NO padding and alignment 1, so
 * `sizeof` equals the wire size and we may cast a raw byte buffer to these.
 *
 * ALL multi-byte fields are stored in NETWORK byte order (big-endian). We convert
 * with htons()/htonl() when filling and ntohs() when reading — see each field.
 * =========================================================================== */

/* IPv4 header, 20 bytes (no options). Offsets in the comments are from byte 0. */
struct ip4_hdr {
    u8  ver_ihl;     /* 0 : version (high nibble = 4) | IHL in 32-bit words (low = 5) */
    u8  tos;         /* 1 : type of service / DSCP+ECN (we send 0)                    */
    u16 total_len;   /* 2 : total datagram length incl. this header, network order   */
    u16 id;          /* 4 : identification (fragment reassembly id)                  */
    u16 frag_off;    /* 6 : flags (top 3 bits) | fragment offset (low 13 bits)       */
    u8  ttl;         /* 8 : time to live (hop budget)                                */
    u8  protocol;    /* 9 : L4 protocol; 6 = IPPROTO_TCP                             */
    u16 checksum;    /* 10: header checksum (over these 20 bytes, this field = 0)    */
    u32 src_addr;    /* 12: source IPv4 address, network order                       */
    u32 dst_addr;    /* 16: destination IPv4 address, network order                  */
} __attribute__((packed));

/* TCP header, 20 bytes (no options). */
struct tcp_hdr {
    u16 src_port;    /* 0 : source port, network order                              */
    u16 dst_port;    /* 2 : destination port, network order                         */
    u32 seq;         /* 4 : sequence number, network order                          */
    u32 ack_seq;     /* 8 : acknowledgement number (0 in a SYN)                     */
    u8  data_off;    /* 12: data offset in 32-bit words (high nibble) | reserved     */
    u8  flags;       /* 13: control bits (TH_SYN etc.)                              */
    u16 window;      /* 14: receive-window size advertised, network order           */
    u16 checksum;    /* 16: checksum over pseudo-header + TCP segment (this field=0) */
    u16 urg_ptr;     /* 18: urgent pointer (0 unless URG set)                       */
} __attribute__((packed));

/* The 12-byte TCP pseudo-header. NEVER transmitted — it exists only so the TCP
 * checksum also covers the IP addresses, protocol, and length. See asm/demo.c for
 * the long-form explanation; this is the same struct. */
struct pseudo_header {
    u32 src_addr;    /* 0 : source IPv4, network order      */
    u32 dst_addr;    /* 4 : destination IPv4, network order */
    u8  zero;        /* 8 : always 0                        */
    u8  protocol;    /* 9 : 6 = IPPROTO_TCP                 */
    u16 tcp_length;  /* 10: TCP header + payload length, net order */
} __attribute__((packed));

/* ===========================================================================
 * SECTION 2 — the Internet checksum (RFC 1071). Mirrors asm/demo.c exactly.
 * ===========================================================================
 * The checksum is a ones-complement 16-bit sum. We accumulate in a 32-bit int so
 * carries collect in the high half, then fold them back and complement. We read
 * each 16-bit word big-endian ((buf[0]<<8)|buf[1]) so the result is independent
 * of THIS host's byte order — critical, since the code must be correct whether it
 * runs on a little-endian x86 or a big-endian box.
 * =========================================================================== */

/* Accumulate the big-endian 16-bit words of `buf` into `sum`. `len` may be odd:
 * a trailing byte is the high half of a word whose low half is an implicit 0. */
static u32 sum16(const u8 *buf, u32 len, u32 sum)
{
    while (len > 1) {                                  /* whole 16-bit words   */
        sum += (u32)((u32)buf[0] << 8 | (u32)buf[1]); /* big-endian word      */
        buf += 2;
        len -= 2;
    }
    if (len == 1)                                      /* lone trailing byte   */
        sum += (u32)((u32)buf[0] << 8);                /* pad low half with 0  */
    return sum;
}

/* Fold the 32-bit accumulator down to 16 bits (end-around carry) and complement.
 * Returns a HOST-ORDER value; store it into a network-order field via htons(). */
static u16 fold_csum(u32 sum)
{
    while (sum >> 16)                                  /* any carry bits left? */
        sum = (sum & 0xFFFFu) + (sum >> 16);           /* fold them into low16 */
    return (u16)~sum;                                  /* ones-complement      */
}

/* IPv4 header checksum: covers only the 20-byte header, with its own checksum
 * field pre-zeroed. Returns host order; caller stores with htons(). */
static u16 ip_checksum(const struct ip4_hdr *ip)
{
    return fold_csum(sum16((const u8 *)ip, (u32)sizeof(*ip), 0));
}

/* TCP checksum: the pseudo-header FIRST, then the TCP segment, one running sum.
 * `tcp->checksum` must already be 0. Returns host order; store with htons().
 * This is the routine extracted verbatim into asm/demo.c for the annotated asm. */
static u16 tcp_checksum(u32 src_addr, u32 dst_addr,
                        const struct tcp_hdr *tcp, u16 tcp_len)
{
    struct pseudo_header ph;
    ph.src_addr   = src_addr;              /* already network order (from the socket) */
    ph.dst_addr   = dst_addr;
    ph.zero       = 0;
    ph.protocol   = IPPROTO_TCP;           /* 6 */
    ph.tcp_length = htons(tcp_len);        /* header+payload length, into net order   */

    u32 sum = 0;
    sum = sum16((const u8 *)&ph,  (u32)sizeof(ph), sum);   /* 12-byte pseudo-header */
    sum = sum16((const u8 *)tcp,  (u32)tcp_len,    sum);   /* the TCP segment       */
    return fold_csum(sum);
}

/* ===========================================================================
 * SECTION 3 — build one SYN probe into `buf`. Returns the packet length.
 * ===========================================================================
 * We fill the IP header and the TCP header ourselves. Under Linux IP_HDRINCL the
 * kernel will OVERWRITE the IP checksum and total length (and the source address
 * / IP id if we leave them 0) — see `man 7 raw`. We compute them anyway: it costs
 * nothing, keeps the buffer self-consistent for other OSes, and lets you SEE the
 * IP checksum algorithm. The TCP checksum, by contrast, the kernel does NOT touch
 * for a raw IP_HDRINCL socket — if we get it wrong the peer silently drops our SYN
 * and every port looks "filtered". So the TCP checksum is the load-bearing one.
 * =========================================================================== */
static u32 build_syn_packet(u8 *buf, u32 src_addr, u32 dst_addr,
                            u16 src_port, u16 dst_port, u32 seq, u16 ip_id)
{
    struct ip4_hdr  *ip  = (struct ip4_hdr  *)buf;
    struct tcp_hdr  *tcp = (struct tcp_hdr  *)(buf + sizeof(*ip));

    /* ---- IP header ------------------------------------------------------- */
    ip->ver_ihl  = (u8)((4u << 4) | 5u);   /* version 4, IHL 5 words (20 bytes) */
    ip->tos      = 0;
    ip->total_len= htons((u16)PKT_LEN);    /* 40 bytes total                    */
    ip->id       = htons(ip_id);           /* per-probe id (kernel fills if 0)  */
    ip->frag_off = htons(0x4000);          /* set the Don't-Fragment (DF) bit   */
    ip->ttl      = 64;                     /* a normal starting hop count       */
    ip->protocol = IPPROTO_TCP;            /* 6                                 */
    ip->checksum = 0;                      /* MUST be 0 before we checksum it   */
    ip->src_addr = src_addr;               /* network order already             */
    ip->dst_addr = dst_addr;
    ip->checksum = htons(ip_checksum(ip)); /* now fill it (kernel may redo this) */

    /* ---- TCP header ------------------------------------------------------ */
    tcp->src_port = htons(src_port);       /* our chosen ephemeral port         */
    tcp->dst_port = htons(dst_port);       /* the port we are probing           */
    tcp->seq      = htonl(seq);            /* initial sequence number           */
    tcp->ack_seq  = 0;                     /* no ACK in a first SYN             */
    tcp->data_off = (u8)(5u << 4);         /* data offset 5 words (20B); no opts */
    tcp->flags    = TH_SYN;                /* the whole point: a bare SYN       */
    tcp->window   = htons(64240);          /* a plausible receive window        */
    tcp->checksum = 0;                     /* MUST be 0 before we checksum it   */
    tcp->urg_ptr  = 0;
    /* Checksum covers the pseudo-header + these 20 TCP bytes. */
    tcp->checksum = htons(tcp_checksum(src_addr, dst_addr, tcp,
                                       (u16)sizeof(*tcp)));

    return PKT_LEN;
}

/* ===========================================================================
 * SECTION 4 — small helpers: source-IP discovery, rate limiting, port states.
 * =========================================================================== */

/* Which of OUR addresses will the kernel use to reach `dst`? We need it to fill
 * the IP source field and the TCP pseudo-header. The trick: connect() a UDP
 * socket to the destination. UDP connect sends NO packet — it only installs a
 * default route/peer — after which getsockname() reports the source address the
 * routing table picked. Returns 0 on success, -1 on error. */
static int discover_source_ip(u32 dst_addr, u32 *out_src)
{
    int s = socket(AF_INET, SOCK_DGRAM, 0);   /* cheap throwaway UDP socket        */
    if (s < 0) { perror("socket(UDP) for source-ip"); return -1; }

    /* All locals declared up front so the `goto out` never jumps over an
     * initializer (keeps -Wjump-misses-init and picky compilers happy). */
    struct sockaddr_in to;
    struct sockaddr_in local;
    socklen_t len = sizeof(local);
    int rc = -1;

    memset(&to, 0, sizeof(to));
    to.sin_family = AF_INET;
    to.sin_addr.s_addr = dst_addr;
    to.sin_port = htons(53);                  /* arbitrary; no datagram is sent    */

    if (connect(s, (struct sockaddr *)&to, sizeof(to)) != 0) {
        perror("connect(UDP) for source-ip");
        goto out;
    }
    if (getsockname(s, (struct sockaddr *)&local, &len) != 0) {
        perror("getsockname");
        goto out;
    }
    *out_src = local.sin_addr.s_addr;         /* network order                     */
    rc = 0;
out:
    close(s);                                 /* always release the fd             */
    return rc;
}

/* Sleep `ns` nanoseconds, resuming correctly if a signal interrupts us. clock
 * used is the default (CLOCK_MONOTONIC-like relative). nanosleep writes the
 * unslept remainder back into `rem` on EINTR, so we loop until it is exhausted. */
static void sleep_ns(long ns)
{
    if (ns <= 0) return;
    struct timespec req = { .tv_sec = ns / 1000000000L,
                            .tv_nsec = ns % 1000000000L };
    struct timespec rem;
    while (nanosleep(&req, &rem) != 0) {
        if (errno != EINTR) return;           /* real error: give up, not fatal    */
        req = rem;                            /* interrupted: sleep the remainder   */
    }
}

/* Space out probes to at most `rate` per second. rate==0 means "no limit". */
static void sleep_between_probes(int rate)
{
    if (rate > 0)
        sleep_ns(1000000000L / rate);
}

/* The three answers a scan can produce. calloc() zero-fills to PS_FILTERED, which
 * is exactly the right default: "we sent a probe and heard nothing back". */
enum port_state { PS_FILTERED = 0, PS_OPEN = 1, PS_CLOSED = 2 };

static const char *state_name(enum port_state s)
{
    switch (s) {
        case PS_OPEN:     return "open";
        case PS_CLOSED:   return "closed";
        default:          return "filtered";
    }
}

/* ===========================================================================
 * SECTION 5 — classify one raw inbound datagram against our probe.
 * ===========================================================================
 * The raw SOCK_RAW/IPPROTO_TCP socket hands us EVERY inbound TCP segment the host
 * receives, complete with its IP header — not just replies to us. So we must
 * filter hard: right source host, right ports, then read the flags. Returns the
 * probed port number (via *out_port) and its state (via *out_state), or false if
 * this datagram is not one of our replies.
 *
 * `buf`/`n` are the bytes recv() gave us. We must not trust `n`: validate lengths
 * before indexing, or a short/hostile packet walks us off the buffer.
 * =========================================================================== */
static bool classify_reply(const u8 *buf, ssize_t n, u32 my_src, u32 target,
                           u16 my_port, u16 *out_port, enum port_state *out_state)
{
    if (n < (ssize_t)sizeof(struct ip4_hdr))
        return false;                                 /* runt: no full IP header   */

    const struct ip4_hdr *ip = (const struct ip4_hdr *)buf;

    /* IHL is in 32-bit words; the TCP header starts after that many bytes. A
     * forged IHL could point past our buffer, so bound-check it. */
    u32 ihl = (u32)(ip->ver_ihl & 0x0F) * 4u;
    if (ihl < sizeof(struct ip4_hdr))
        return false;                                 /* nonsense IHL              */
    if (n < (ssize_t)(ihl + sizeof(struct tcp_hdr)))
        return false;                                 /* not enough for TCP header */

    if (ip->protocol != IPPROTO_TCP) return false;    /* should be TCP, but verify */
    if (ip->src_addr != target)      return false;    /* not from the host we probed*/
    if (ip->dst_addr != my_src)      return false;    /* not addressed to us        */

    const struct tcp_hdr *tcp = (const struct tcp_hdr *)(buf + ihl);

    /* The reply's DESTINATION port is our ephemeral source port; its SOURCE port
     * is the port we were probing. Match both so we ignore unrelated traffic. */
    if (ntohs(tcp->dst_port) != my_port) return false;

    *out_port = ntohs(tcp->src_port);

    /* The classification itself — the crux of a SYN scan:
     *   SYN|ACK  => the port is listening and just offered us the handshake.
     *   RST(|ACK)=> the port is closed; the peer's kernel refuses the SYN. */
    if ((tcp->flags & (TH_SYN | TH_ACK)) == (TH_SYN | TH_ACK)) {
        *out_state = PS_OPEN;
        return true;
    }
    if (tcp->flags & TH_RST) {
        *out_state = PS_CLOSED;
        return true;
    }
    return false;                                     /* some other TCP packet     */
}

/* ===========================================================================
 * SECTION 6 — the SYN (half-open) scan engine.
 * ===========================================================================
 * Structure: (1) open the raw socket with IP_HDRINCL and a receive timeout,
 * (2) blast one SYN per port, rate-limited, (3) drain replies until either every
 * port is decided or the socket read times out. Ports still undecided at the end
 * stay PS_FILTERED (their default), which is the correct meaning of "no reply".
 *
 * Returns 0 on success, -1 on a setup error (e.g. not privileged).
 * =========================================================================== */
static int syn_scan(u32 target, const u16 *ports, int nports,
                    int rate, int wait_ms, u32 src_addr, u16 my_port,
                    enum port_state *state /* indexed by port number, size 65536 */)
{
    /* --- open the raw socket -------------------------------------------------
     * socket(AF_INET, SOCK_RAW, IPPROTO_TCP): a raw IPv4 socket that (a) lets us
     * write TCP directly and (b) delivers inbound TCP to us below the kernel's
     * TCP layer. This is the privileged call — it needs CAP_NET_RAW (root). */
    int s = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (s < 0) {
        if (errno == EPERM || errno == EACCES)
            fprintf(stderr,
                "socket(SOCK_RAW): permission denied — a SYN scan needs raw-packet\n"
                "privileges. Run with sudo, or grant the capability once:\n"
                "    sudo setcap cap_net_raw+ep ./synscan\n"
                "Or use the unprivileged connect() scan instead:  synscan -sT ...\n");
        else
            perror("socket(SOCK_RAW)");
        return -1;
    }

    int rc = -1;   /* pessimistic; set to 0 only once we finish cleanly */

    /* --- IP_HDRINCL: "I supply the IP header myself" -------------------------
     * Without this, the kernel prepends its own IP header and our hand-built one
     * would be treated as payload. With it, the bytes we send start at the IP
     * header. (Level IPPROTO_IP, option IP_HDRINCL, value 1.) */
    int one = 1;
    if (setsockopt(s, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) != 0) {
        perror("setsockopt(IP_HDRINCL)");
        goto out;
    }

    /* --- SO_RCVTIMEO: bound how long recv() blocks --------------------------
     * We want recv() to return once no reply has arrived for `wait_ms`, so the
     * drain loop terminates instead of blocking forever. When the timer expires
     * recv() fails with EAGAIN/EWOULDBLOCK, which we treat as "quiet, stop". */
    struct timeval tv = { .tv_sec = wait_ms / 1000,
                          .tv_usec = (wait_ms % 1000) * 1000 };
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != 0) {
        perror("setsockopt(SO_RCVTIMEO)");
        goto out;
    }

    struct sockaddr_in dst;
    memset(&dst, 0, sizeof(dst));
    dst.sin_family = AF_INET;
    dst.sin_addr.s_addr = target;     /* routing uses this with IP_HDRINCL       */
    /* dst.sin_port is ignored for a raw socket — TCP ports live in our header.   */

    /* --- (2) transmit one SYN per port -------------------------------------- */
    int pending = nports;             /* how many ports have no verdict yet       */
    u32 seq_base = (u32)time(NULL) * 2654435761u; /* cheap per-run seq spread     */

    for (int i = 0; i < nports; i++) {
        u16 port = ports[i];
        u8 pkt[PKT_LEN];
        u32 len = build_syn_packet(pkt, src_addr, target, my_port, port,
                                   seq_base ^ (u32)(port * 2246822519u),
                                   (u16)(0x1000 + i));

        /* sendto(): with a connected-less raw socket we pass the destination each
         * time. Partial sends do not happen for a single datagram this small, but
         * we still check for -1. EINTR: retry the send. */
        ssize_t w;
        do {
            w = sendto(s, pkt, len, 0, (struct sockaddr *)&dst, sizeof(dst));
        } while (w < 0 && errno == EINTR);
        if (w < 0) {
            perror("sendto");
            /* keep going: one failed probe should not abort the whole scan */
        }

        /* We do NOT read replies here, mid-send: recv() would block up to
         * SO_RCVTIMEO and stall the transmit loop. Instead we let the kernel's
         * socket receive buffer hold early replies until the drain phase below.
         * For large port ranges that buffer can overflow and drop a reply, making
         * a port look filtered — the honest limitation noted in the README. */

        sleep_between_probes(rate);   /* pace the send rate                       */
    }

    /* --- (3) drain replies until decided or quiet --------------------------- */
    u8 rbuf[2048];                    /* replies are tiny (IP+TCP <= 120B); ample */
    while (pending > 0) {
        ssize_t n = recv(s, rbuf, sizeof(rbuf), 0);
        if (n < 0) {
            if (errno == EINTR) continue;                 /* signal: retry        */
            if (errno == EAGAIN || errno == EWOULDBLOCK)  /* SO_RCVTIMEO expired  */
                break;                                    /* no more replies       */
            perror("recv");
            break;
        }
        u16 port;
        enum port_state st;
        if (!classify_reply(rbuf, n, src_addr, target, my_port, &port, &st))
            continue;                                     /* not one of our replies*/
        if (state[port] == PS_FILTERED) {                 /* first verdict wins    */
            state[port] = st;
            pending--;                                    /* one fewer to wait for */
        }
    }

    rc = 0;
out:
    close(s);
    return rc;
}

/* ===========================================================================
 * SECTION 7 — the connect() scan engine (unprivileged fallback).
 * ===========================================================================
 * For each port we do a NON-BLOCKING connect() and watch the outcome. Because it
 * is non-blocking, connect() returns immediately with EINPROGRESS; we then use
 * select() to wait — up to `wait_ms` — for the socket to become writable (the
 * signal that the handshake finished or failed), and read SO_ERROR to learn how:
 *     SO_ERROR == 0            -> handshake completed  -> OPEN
 *     SO_ERROR == ECONNREFUSED -> got an RST           -> CLOSED
 *     select() timed out       -> no answer            -> FILTERED
 * This asks the KERNEL's TCP stack to do the work, so no special privilege is
 * needed — but it completes the 3-way handshake, so it is not "half-open".
 * =========================================================================== */
static int connect_scan(u32 target, const u16 *ports, int nports,
                        int rate, int wait_ms, enum port_state *state)
{
    for (int i = 0; i < nports; i++) {
        u16 port = ports[i];

        int s = socket(AF_INET, SOCK_STREAM, 0);   /* an ordinary TCP socket      */
        if (s < 0) { perror("socket(TCP)"); return -1; }

        /* Switch to non-blocking so connect() does not stall on a filtered port. */
        int fl = fcntl(s, F_GETFL, 0);
        if (fl < 0 || fcntl(s, F_SETFL, fl | O_NONBLOCK) < 0) {
            perror("fcntl(O_NONBLOCK)");
            close(s);
            return -1;
        }

        struct sockaddr_in to;
        memset(&to, 0, sizeof(to));
        to.sin_family = AF_INET;
        to.sin_addr.s_addr = target;
        to.sin_port = htons(port);

        enum port_state st = PS_FILTERED;          /* default if nothing decides  */
        int rc = connect(s, (struct sockaddr *)&to, sizeof(to));
        if (rc == 0) {
            st = PS_OPEN;                           /* connected instantly (local) */
        } else if (errno == EINPROGRESS) {
            /* Handshake underway: wait for writability with a deadline. */
            fd_set wfds;
            FD_ZERO(&wfds);
            FD_SET(s, &wfds);
            struct timeval tv = { .tv_sec = wait_ms / 1000,
                                  .tv_usec = (wait_ms % 1000) * 1000 };
            int sel;
            do {
                fd_set w = wfds;                    /* select() mutates the set    */
                sel = select(s + 1, NULL, &w, NULL, &tv);
            } while (sel < 0 && errno == EINTR);    /* restart on signal           */

            if (sel > 0) {
                /* Socket is writable: read the completion status via SO_ERROR. */
                int soerr = 0;
                socklen_t len = sizeof(soerr);
                if (getsockopt(s, SOL_SOCKET, SO_ERROR, &soerr, &len) == 0) {
                    if (soerr == 0)                st = PS_OPEN;
                    else if (soerr == ECONNREFUSED) st = PS_CLOSED;
                    else                            st = PS_FILTERED; /* unreachable etc.*/
                }
            } else if (sel == 0) {
                st = PS_FILTERED;                   /* timed out: no response      */
            } else {
                perror("select");
            }
        } else if (errno == ECONNREFUSED) {
            st = PS_CLOSED;                          /* immediate RST (localhost)   */
        } else {
            /* ENETUNREACH, EHOSTUNREACH, EACCES... treat as filtered/unreachable. */
            st = PS_FILTERED;
        }

        state[port] = st;
        close(s);                                    /* release the fd every loop   */
        sleep_between_probes(rate);
    }
    return 0;
}

/* ===========================================================================
 * SECTION 8 — parse a port spec like "22,80,443" or "1-1024" or "1-100,443".
 * ===========================================================================
 * We mark chosen ports in a 65536-entry bitmap (dedup + sort for free), then
 * collect them into a compact array. Returns the array (caller frees it) and the
 * count via *out_n, or NULL on a malformed spec.
 * =========================================================================== */
static u16 *parse_ports(const char *spec, int *out_n)
{
    /* One byte per possible port; calloc zero-fills. 64 KiB, freed before return. */
    u8 *seen = calloc(65536, 1);
    if (!seen) { perror("calloc"); return NULL; }

    const char *p = spec;
    while (*p) {
        /* read a number */
        if (!isdigit((unsigned char)*p)) { free(seen); return NULL; }
        long a = strtol(p, (char **)&p, 10);
        long b = a;
        if (*p == '-') {                       /* a range "a-b"                    */
            p++;
            if (!isdigit((unsigned char)*p)) { free(seen); return NULL; }
            b = strtol(p, (char **)&p, 10);
        }
        if (a < 1 || a > 65535 || b < 1 || b > 65535 || a > b) {
            free(seen); return NULL;           /* out of range or reversed         */
        }
        for (long v = a; v <= b; v++)
            seen[v] = 1;
        if (*p == ',') p++;                    /* more tokens follow               */
        else if (*p != '\0') { free(seen); return NULL; } /* junk                  */
    }

    /* Count, then gather into a right-sized array. */
    int n = 0;
    for (int v = 1; v <= 65535; v++) n += seen[v];
    if (n == 0) { free(seen); return NULL; }

    u16 *ports = malloc((size_t)n * sizeof(u16));
    if (!ports) { perror("malloc"); free(seen); return NULL; }
    int j = 0;
    for (int v = 1; v <= 65535; v++)
        if (seen[v]) ports[j++] = (u16)v;

    free(seen);
    *out_n = n;
    return ports;
}

/* Resolve a hostname or dotted-quad into a network-order IPv4 address. Uses
 * getaddrinfo so "scanme.example" and "192.0.2.5" both work; takes the first
 * AF_INET answer. Returns 0 / fills *out, or -1. */
static int resolve_target(const char *host, u32 *out)
{
    struct addrinfo hints, *res = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;             /* IPv4 only (our headers are IPv4)     */
    hints.ai_socktype = SOCK_STREAM;

    int e = getaddrinfo(host, NULL, &hints, &res);
    if (e != 0) {
        fprintf(stderr, "cannot resolve '%s': %s\n", host, gai_strerror(e));
        return -1;
    }
    struct sockaddr_in *sin = (struct sockaddr_in *)res->ai_addr;
    *out = sin->sin_addr.s_addr;           /* network order                        */
    freeaddrinfo(res);
    return 0;
}

static void usage(const char *argv0)
{
    fprintf(stderr,
        "synscan — a raw-socket SYN / connect() port scanner (AUTHORIZED TARGETS ONLY)\n"
        "\n"
        "usage: %s [options] <target>\n"
        "  <target>        hostname or IPv4 address (e.g. 127.0.0.1)\n"
        "  -p <spec>       ports: \"80\", \"1-1024\", \"22,80,443\", \"1-100,443\"  (default %s)\n"
        "  -sS             SYN half-open scan (default; needs CAP_NET_RAW/root)\n"
        "  -sT             TCP connect() scan (no privileges required)\n"
        "  -r <pps>        max probes per second, 0 = unlimited  (default %d)\n"
        "  -w <ms>         wait for replies, milliseconds        (default %d)\n"
        "  --open          print only open ports\n"
        "  -h, --help      this help\n",
        argv0, DEF_PORTS, DEF_RATE, DEF_WAIT_MS);
}

/* ===========================================================================
 * SECTION 9 — main: parse arguments, dispatch to an engine, print results.
 * =========================================================================== */
int main(int argc, char **argv)
{
    const char *port_spec = DEF_PORTS;
    const char *target_str = NULL;
    bool use_syn = true;          /* -sS default; -sT flips it                    */
    bool only_open = false;
    int rate = DEF_RATE;
    int wait_ms = DEF_WAIT_MS;

    /* --- hand-rolled argument parse (keeps the option semantics visible) ----- */
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (strcmp(a, "-h") == 0 || strcmp(a, "--help") == 0) {
            usage(argv[0]); return 0;
        } else if (strcmp(a, "-sS") == 0) {
            use_syn = true;
        } else if (strcmp(a, "-sT") == 0) {
            use_syn = false;
        } else if (strcmp(a, "--open") == 0) {
            only_open = true;
        } else if (strcmp(a, "-p") == 0) {
            if (++i >= argc) { usage(argv[0]); return 2; }
            port_spec = argv[i];
        } else if (strcmp(a, "-r") == 0) {
            if (++i >= argc) { usage(argv[0]); return 2; }
            rate = atoi(argv[i]);
            if (rate < 0) rate = 0;
        } else if (strcmp(a, "-w") == 0) {
            if (++i >= argc) { usage(argv[0]); return 2; }
            wait_ms = atoi(argv[i]);
            if (wait_ms < 1) wait_ms = 1;
        } else if (a[0] == '-' && a[1] != '\0') {
            fprintf(stderr, "unknown option: %s\n", a);
            usage(argv[0]); return 2;
        } else {
            target_str = a;        /* first bare word is the target                */
        }
    }
    if (!target_str) { usage(argv[0]); return 2; }

    /* --- resolve target and ports ------------------------------------------- */
    u32 target;
    if (resolve_target(target_str, &target) != 0) return 1;

    int nports = 0;
    u16 *ports = parse_ports(port_spec, &nports);
    if (!ports) {
        fprintf(stderr, "bad port spec: '%s'\n", port_spec);
        return 2;
    }

    /* Per-port verdict table, indexed by port number (0..65535). calloc gives us
     * PS_FILTERED everywhere — the right default for "no reply". Freed at the end. */
    enum port_state *state = calloc(65536, sizeof(enum port_state));
    if (!state) { perror("calloc"); free(ports); return 1; }

    char tbuf[INET_ADDRSTRLEN];
    struct in_addr ia = { .s_addr = target };
    if (!inet_ntop(AF_INET, &ia, tbuf, sizeof(tbuf))) {
        perror("inet_ntop");
        free(ports); free(state); return 1;
    }

    /* --- run the chosen engine, timing the whole thing ---------------------- */
    struct timespec t0 = {0}, t1 = {0};
    if (clock_gettime(CLOCK_MONOTONIC, &t0) != 0)
        perror("clock_gettime");   /* non-fatal: timing is cosmetic */

    int rc;
    if (use_syn) {
        /* SYN scan needs our own source IP (for the IP header + pseudo-header)
         * and a fixed ephemeral source port to match replies against. */
        u32 src_addr;
        if (discover_source_ip(target, &src_addr) != 0) {
            free(ports); free(state); return 1;
        }
        srand((unsigned)(time(NULL) ^ (unsigned)getpid()));
        u16 my_port = (u16)(0xC000u | (rand() & 0x3FFFu)); /* 49152..65535        */

        printf("SYN scan of %s (%d ports) from source port %u\n",
               tbuf, nports, (unsigned)my_port);
        rc = syn_scan(target, ports, nports, rate, wait_ms,
                      src_addr, my_port, state);
    } else {
        printf("connect() scan of %s (%d ports)\n", tbuf, nports);
        rc = connect_scan(target, ports, nports, rate, wait_ms, state);
    }

    if (clock_gettime(CLOCK_MONOTONIC, &t1) != 0)
        perror("clock_gettime");
    double elapsed = (double)(t1.tv_sec - t0.tv_sec)
                   + (double)(t1.tv_nsec - t0.tv_nsec) / 1e9;

    /* --- report ------------------------------------------------------------- */
    if (rc == 0) {
        int n_open = 0, n_closed = 0, n_filt = 0;
        printf("\nPORT      STATE\n");
        for (int i = 0; i < nports; i++) {
            enum port_state st = state[ports[i]];
            if (st == PS_OPEN)    n_open++;
            else if (st == PS_CLOSED) n_closed++;
            else                  n_filt++;
            if (only_open && st != PS_OPEN) continue;
            printf("%-9u %s\n", (unsigned)ports[i], state_name(st));
        }
        printf("\nscanned %d ports in %.2fs — %d open, %d closed, %d filtered\n",
               nports, elapsed, n_open, n_closed, n_filt);
    }

    free(ports);
    free(state);
    return rc == 0 ? 0 : 1;
}
