/* ===========================================================================
 * vpn.c — the driver: TUN + UDP + the Noise handshake wired into one event loop.
 * ===========================================================================
 *
 * This is the "device" that behaves like a tiny WireGuard interface between two
 * peers. It is deliberately single-threaded and poll()-driven: no locks, no
 * memory-ordering puzzles — just one loop multiplexing two file descriptors.
 *
 *   TUN fd readable  -> a local app sent an IP packet -> encrypt -> UDP to peer
 *   UDP fd readable  -> a datagram arrived            -> dispatch by type:
 *        type 1 (init)  responder: authenticate, reply, open a session
 *        type 2 (resp)  initiator: finish handshake, open a session
 *        type 4 (data)  decrypt, replay-check, write the IP packet to the TUN
 *   poll timeout     -> if we are the initiator and have no session, (re)dial
 *
 * ROLES. The side given a peer endpoint (-e) is the INITIATOR and dials; the
 * other side just listens and RESPONDS. Once the two handshake messages cross,
 * both hold a session and data flows both ways. This teaching core keeps a
 * SINGLE current session (no rekey timer, no roaming) — see README.
 *
 * Platform: Linux. Needs CAP_NET_ADMIN for the TUN device (run as root).
 * =========================================================================== */

#include "wg.h"
#include "noise.h"
#include "x25519.h"       /* x25519_base for key derivation / keygen mode       */
#include "tun.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <poll.h>
#include <arpa/inet.h>    /* sockaddr_in, htons, inet_ntop/pton                 */
#include <sys/socket.h>

/* ---------------------------------------------------------------------------
 * Device state. One peer, one optional session, held on the stack in main().
 * --------------------------------------------------------------------------- */
struct device {
    struct noise_static  s;             /* our static identity                   */
    u8   peer_static[X25519_KEY_LEN];   /* the one authorized peer               */

    int  tun_fd;
    int  udp_fd;
    char ifname[16];

    int  is_initiator;                  /* were we given a peer endpoint?        */
    struct sockaddr_in peer_ep;         /* where to send (configured or learned) */
    int  have_endpoint;

    struct noise_handshake hs;          /* the in-flight handshake               */
    int  handshaking;

    struct noise_session   session;     /* the current data channel              */

    u8   last_timestamp[TIMESTAMP_LEN]; /* responder: greatest handshake ts seen */
    int  have_last_ts;
};

/* SIGINT/SIGTERM flip this so the loop exits and we close fds cleanly. */
static volatile sig_atomic_t g_stop = 0;
static void on_signal(int sig) { (void)sig; g_stop = 1; }

/* ---------------------------------------------------------------------------
 * A 12-byte TAI64N-style timestamp: 8 bytes seconds + 4 bytes nanoseconds, BIG-
 * endian so that a byte-wise memcmp orders timestamps chronologically. The
 * responder rejects any initiation whose timestamp is not strictly greater than
 * the last it accepted from this peer — that defeats replay of a whole recorded
 * handshake (a different, coarser guard than the per-packet data replay window).
 * --------------------------------------------------------------------------- */
static void make_timestamp(u8 out[TIMESTAMP_LEN])
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    u64 secs = 0x4000000000000000ULL + (u64)ts.tv_sec;   /* TAI64 base offset    */
    for (int i = 0; i < 8; i++) out[i] = (u8)(secs >> (56 - 8 * i));   /* BE       */
    u32 ns = (u32)ts.tv_nsec;
    for (int i = 0; i < 4; i++) out[8 + i] = (u8)(ns >> (24 - 8 * i)); /* BE       */
}

/* ---------------------------------------------------------------------------
 * (Re)start a handshake as the initiator: fresh ephemeral, fresh session id,
 * send message 1 to the configured endpoint. Called at startup and on the poll
 * timeout while we still have no session (handles a lost first packet).
 * --------------------------------------------------------------------------- */
static void start_handshake(struct device *dev)
{
    u32 local_index;
    rng_bytes(&local_index, sizeof local_index);   /* our random session id       */

    noise_handshake_init(&dev->hs, &dev->s, dev->peer_static, 1, local_index);

    u8 timestamp[TIMESTAMP_LEN];
    make_timestamp(timestamp);

    u8 msg[INIT_MSG_LEN];
    noise_create_initiation(&dev->hs, msg, timestamp);

    if (sendto(dev->udp_fd, msg, sizeof msg, 0,
               (struct sockaddr *)&dev->peer_ep, sizeof dev->peer_ep) < 0)
        fprintf(stderr, "sendto(init): %s\n", strerror(errno));
    dev->handshaking = 1;
    fprintf(stderr, "-> sent handshake initiation (session id %08x)\n", local_index);
}

/* Encrypt one inner IP packet and send it as a transport-data datagram. */
static void send_data_packet(struct device *dev, const u8 *pkt, usize len)
{
    /* Layout: [type|receiver|counter] header, then ciphertext+tag. */
    u8 out[DATA_HEADER_LEN + MAX_PACKET + AEAD_TAG_LEN];
    u64 counter = noise_transport_encrypt(&dev->session,
                                          out + DATA_OFF_PAYLOAD, pkt, len);

    store_le32(out + DATA_OFF_TYPE, MSG_TRANSPORT_DATA);
    store_le32(out + DATA_OFF_RECEIVER, dev->session.remote_index);
    store_le64(out + DATA_OFF_COUNTER, counter);

    usize total = DATA_HEADER_LEN + len + AEAD_TAG_LEN;
    if (sendto(dev->udp_fd, out, total, 0,
               (struct sockaddr *)&dev->peer_ep, sizeof dev->peer_ep) < 0)
        fprintf(stderr, "sendto(data): %s\n", strerror(errno));
}

/* ---------------------------------------------------------------------------
 * UDP datagram arrived: parse the 4-byte type and dispatch. `src` is the sender
 * address, used to learn/refresh the peer endpoint (basic roaming).
 * --------------------------------------------------------------------------- */
static void handle_udp(struct device *dev, u8 *buf, ssize_t n,
                       struct sockaddr_in *src)
{
    if (n < 4) return;                              /* too short for a type field  */
    u32 type = load_le32(buf + DATA_OFF_TYPE);

    if (type == MSG_HANDSHAKE_INIT && n == INIT_MSG_LEN) {
        /* We are acting as responder for this exchange. Build a fresh state and
         * try to consume the initiation (this authenticates the initiator). */
        u32 local_index;
        rng_bytes(&local_index, sizeof local_index);
        struct noise_handshake rhs;
        noise_handshake_init(&rhs, &dev->s, NULL, 0, local_index);

        u8 timestamp[TIMESTAMP_LEN];
        if (noise_consume_initiation(&rhs, buf, timestamp) != 0) {
            fprintf(stderr, "<- initiation failed to authenticate; dropped\n");
            return;
        }
        /* Authorization: we only speak to the ONE configured peer. */
        if (memcmp(rhs.remote_static, dev->peer_static, X25519_KEY_LEN) != 0) {
            fprintf(stderr, "<- initiation from unknown peer; dropped\n");
            return;
        }
        /* Handshake anti-replay: the timestamp must strictly increase. */
        if (dev->have_last_ts &&
            memcmp(timestamp, dev->last_timestamp, TIMESTAMP_LEN) <= 0) {
            fprintf(stderr, "<- replayed initiation (old timestamp); dropped\n");
            return;
        }
        memcpy(dev->last_timestamp, timestamp, TIMESTAMP_LEN);
        dev->have_last_ts = 1;

        /* Learn where to reply/send from the source address (roaming). */
        dev->peer_ep = *src;
        dev->have_endpoint = 1;

        /* Reply with message 2 and open the session. */
        u8 resp[RESP_MSG_LEN];
        noise_create_response(&rhs, resp);
        if (sendto(dev->udp_fd, resp, sizeof resp, 0,
                   (struct sockaddr *)src, sizeof *src) < 0)
            fprintf(stderr, "sendto(resp): %s\n", strerror(errno));

        noise_begin_session(&rhs, &dev->session);
        dev->handshaking = 0;
        secure_zero(&rhs, sizeof rhs);
        fprintf(stderr, "<- responded; session established (peer id %08x)\n",
                dev->session.remote_index);

    } else if (type == MSG_HANDSHAKE_RESP && n == RESP_MSG_LEN) {
        if (!dev->handshaking) return;             /* not expecting a response    */
        /* The response echoes our session id at RESP_OFF_RECEIVER; ignore any
         * that is not answering our current in-flight initiation. */
        if (load_le32(buf + RESP_OFF_RECEIVER) != dev->hs.local_index) return;
        if (noise_consume_response(&dev->hs, buf) != 0) {
            fprintf(stderr, "<- response failed to authenticate; dropped\n");
            return;
        }
        noise_begin_session(&dev->hs, &dev->session);
        dev->handshaking = 0;
        dev->peer_ep = *src;
        dev->have_endpoint = 1;
        fprintf(stderr, "<- handshake complete; session established (peer id %08x)\n",
                dev->session.remote_index);

    } else if (type == MSG_TRANSPORT_DATA && n >= DATA_HEADER_LEN + AEAD_TAG_LEN) {
        if (!dev->session.established) return;      /* no keys yet                 */
        if (load_le32(buf + DATA_OFF_RECEIVER) != dev->session.local_index) return;
        u64 counter = load_le64(buf + DATA_OFF_COUNTER);

        u8 plain[MAX_PACKET];
        long plen = noise_transport_decrypt(&dev->session, counter, plain,
                                            buf + DATA_OFF_PAYLOAD,
                                            (usize)(n - DATA_HEADER_LEN));
        if (plen < 0) return;                       /* bad tag or replay -> drop   */

        /* Inject the recovered IP packet into the kernel via the TUN device. */
        if (write(dev->tun_fd, plain, (usize)plen) < 0)
            fprintf(stderr, "write(tun): %s\n", strerror(errno));
    }
    /* Unknown types / wrong sizes are silently ignored (robustness). */
}

/* ---------------------------------------------------------------------------
 * The event loop.
 * --------------------------------------------------------------------------- */
static int run(struct device *dev)
{
    if (dev->is_initiator) start_handshake(dev);   /* dial immediately            */

    struct pollfd fds[2];
    fds[0].fd = dev->tun_fd; fds[0].events = POLLIN;
    fds[1].fd = dev->udp_fd; fds[1].events = POLLIN;

    while (!g_stop) {
        /* 1 s timeout so an initiator without a session can retransmit and so we
         * notice g_stop promptly. poll(2): returns >0 fds ready, 0 on timeout,
         * -1 on error (EINTR when a signal fires — we just loop). */
        int r = poll(fds, 2, 1000);
        if (r < 0) {
            if (errno == EINTR) continue;
            fprintf(stderr, "poll: %s\n", strerror(errno));
            break;
        }
        if (r == 0) {   /* timeout */
            if (dev->is_initiator && !dev->session.established)
                start_handshake(dev);              /* first packet may be lost    */
            continue;
        }

        /* TUN -> encrypt -> UDP. */
        if (fds[0].revents & POLLIN) {
            u8 pkt[MAX_PACKET];
            ssize_t n = read(dev->tun_fd, pkt, sizeof pkt);
            if (n > 0) {
                if (dev->session.established && dev->have_endpoint)
                    send_data_packet(dev, pkt, (usize)n);
                /* else: no tunnel yet; drop. The handshake completes in one RTT. */
            } else if (n < 0 && errno != EAGAIN && errno != EINTR) {
                fprintf(stderr, "read(tun): %s\n", strerror(errno));
            }
        }

        /* UDP -> dispatch. */
        if (fds[1].revents & POLLIN) {
            u8 buf[MAX_PACKET];
            struct sockaddr_in src;
            socklen_t slen = sizeof src;
            ssize_t n = recvfrom(dev->udp_fd, buf, sizeof buf, 0,
                                 (struct sockaddr *)&src, &slen);
            if (n > 0) handle_udp(dev, buf, n, &src);
            else if (n < 0 && errno != EAGAIN && errno != EINTR)
                fprintf(stderr, "recvfrom: %s\n", strerror(errno));
        }
    }
    return 0;
}

/* ---------------------------------------------------------------------------
 * Setup: keys, TUN, UDP socket.
 * --------------------------------------------------------------------------- */

/* Load a 32-byte key from a base64 string; exit with a message on error. */
static void load_key(u8 out[X25519_KEY_LEN], const char *b64, const char *what)
{
    if (!b64 || base64_decode(out, X25519_KEY_LEN, b64) != 0) {
        fprintf(stderr, "invalid or missing %s (expected 44-char base64)\n", what);
        exit(2);
    }
}

/* Parse "A.B.C.D:port" into a sockaddr_in. Returns 0 or -1. */
static int parse_endpoint(struct sockaddr_in *sa, const char *s)
{
    char host[64];
    const char *colon = strrchr(s, ':');
    if (!colon || (usize)(colon - s) >= sizeof host) return -1;
    memcpy(host, s, (usize)(colon - s));
    host[colon - s] = '\0';

    memset(sa, 0, sizeof *sa);
    sa->sin_family = AF_INET;
    sa->sin_port = htons((u16)atoi(colon + 1));
    if (inet_pton(AF_INET, host, &sa->sin_addr) != 1) return -1;
    return 0;
}

static int open_udp(u16 port)
{
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) return -1;

    /* Bind to the listen port on all interfaces so both peers can reach us. */
    struct sockaddr_in a;
    memset(&a, 0, sizeof a);
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_ANY);
    a.sin_port = htons(port);
    if (bind(fd, (struct sockaddr *)&a, sizeof a) < 0) {
        int e = errno; close(fd); errno = e; return -1;
    }
    return fd;
}

static void usage(const char *argv0)
{
    fprintf(stderr,
        "usage:\n"
        "  %s -g                                   generate a keypair (base64)\n"
        "  %s -k PRIV -R PEERPUB [options]         run the tunnel\n"
        "\n"
        "options:\n"
        "  -k PRIV      our static private key   (base64, 44 chars)\n"
        "  -R PEERPUB   peer static public key   (base64, 44 chars)\n"
        "  -l PORT      local UDP port to bind   (default 51820)\n"
        "  -e IP:PORT   peer endpoint => we INITIATE the handshake\n"
        "  -i NAME      TUN interface name       (default wg0)\n"
        "\n"
        "after it is up, assign an address, e.g.:\n"
        "  sudo ip addr add 10.9.0.1/24 dev wg0\n",
        argv0, argv0);
}

int main(int argc, char **argv)
{
    struct device dev;
    memset(&dev, 0, sizeof dev);
    strncpy(dev.ifname, "wg0", sizeof dev.ifname - 1);

    const char *priv_b64 = NULL, *peer_b64 = NULL, *endpoint = NULL;
    u16 port = 51820;                                /* WireGuard's default port   */
    int keygen = 0;

    int opt;
    while ((opt = getopt(argc, argv, "gk:R:l:e:i:h")) != -1) {
        switch (opt) {
        case 'g': keygen = 1; break;
        case 'k': priv_b64 = optarg; break;
        case 'R': peer_b64 = optarg; break;
        case 'l': port = (u16)atoi(optarg); break;
        case 'e': endpoint = optarg; break;
        case 'i': strncpy(dev.ifname, optarg, sizeof dev.ifname - 1); break;
        case 'h': default: usage(argv[0]); return opt == 'h' ? 0 : 2;
        }
    }

    /* Keygen mode: emit a private key and its derived public key, then exit.
     * The private key is 32 raw random bytes; clamping happens at use time. */
    if (keygen) {
        u8 priv[X25519_KEY_LEN], pub[X25519_KEY_LEN];
        if (rng_bytes(priv, sizeof priv) != 0) {
            fprintf(stderr, "getrandom failed: %s\n", strerror(errno));
            return 1;
        }
        x25519_base(pub, priv);
        char pb[64], ub[64];
        base64_encode(pb, priv, sizeof priv);
        base64_encode(ub, pub, sizeof pub);
        printf("private = %s\npublic  = %s\n", pb, ub);
        secure_zero(priv, sizeof priv);
        return 0;
    }

    /* Load and derive keys. */
    load_key(dev.s.private_key, priv_b64, "private key (-k)");
    x25519_base(dev.s.public_key, dev.s.private_key);
    load_key(dev.peer_static, peer_b64, "peer public key (-R)");

    if (endpoint) {
        if (parse_endpoint(&dev.peer_ep, endpoint) != 0) {
            fprintf(stderr, "bad endpoint '%s' (want IP:PORT)\n", endpoint);
            return 2;
        }
        dev.is_initiator = 1;
        dev.have_endpoint = 1;
    }

    /* Bring up the TUN device. */
    dev.tun_fd = tun_open(dev.ifname);
    if (dev.tun_fd < 0) {
        fprintf(stderr, "tun_open: %s (need root / CAP_NET_ADMIN?)\n", strerror(errno));
        return 1;
    }
    if (tun_configure(dev.ifname, TUNNEL_MTU) != 0)
        fprintf(stderr, "warning: could not set MTU/up on %s: %s\n",
                dev.ifname, strerror(errno));

    dev.udp_fd = open_udp(port);
    if (dev.udp_fd < 0) {
        fprintf(stderr, "open_udp(%u): %s\n", port, strerror(errno));
        close(dev.tun_fd);
        return 1;
    }

    /* Install signal handlers for a clean shutdown. */
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = on_signal;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    fprintf(stderr, "mini-wireguard up on %s, UDP port %u, MTU %d%s\n",
            dev.ifname, port, TUNNEL_MTU,
            dev.is_initiator ? " (initiator)" : " (responder)");
    fprintf(stderr, "assign an address, e.g.:  sudo ip addr add 10.9.0.X/24 dev %s\n",
            dev.ifname);

    int rc = run(&dev);

    close(dev.tun_fd);
    close(dev.udp_fd);
    secure_zero(&dev, sizeof dev);                   /* wipe all key material      */
    return rc;
}
