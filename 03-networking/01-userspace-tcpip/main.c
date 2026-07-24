/* ===========================================================================
 * main.c — bring up the interface and run the single-threaded event loop.
 * ===========================================================================
 *
 * THE LOOP
 * --------
 * A userspace stack is fundamentally: "wait for the wire, react, repeat." We
 * poll() the one TAP fd with a short timeout. On readability we pull one frame
 * and push it through eth_input (which fans out to ARP/IP/ICMP/UDP/TCP). On
 * every wakeup — data or timeout — we run tcp_timer_tick so retransmission and
 * TIME_WAIT timers fire even when the link is idle. Single-threaded and
 * lock-free: there is exactly one place that touches stack state, so there are
 * no data races to reason about. That simplicity is a feature of the design.
 *
 * SETUP (Linux; needs root or CAP_NET_ADMIN to create the tap):
 *     sudo ./tcpip                       # creates tap0, uses 10.0.0.2
 *     # in another terminal, give the HOST side of the link an address:
 *     sudo ip addr add 10.0.0.1/24 dev tap0
 *     sudo ip link set tap0 up
 *     ping 10.0.0.2                       # our ICMP echo answers
 *     nc 10.0.0.2 7                       # our TCP echo answers (type, see echo)
 *     nc -u 10.0.0.2 7                    # our UDP echo answers
 * ========================================================================= */

#include "common.h"
#include "netif.h"
#include "tap.h"
#include "tcp.h"
#include "udp.h"

#include <stdlib.h>     /* exit, EXIT_*                                        */
#include <string.h>     /* memcpy, strncpy                                     */
#include <unistd.h>     /* close                                              */
#include <signal.h>     /* sigaction, SIGINT/SIGTERM                          */
#include <poll.h>       /* poll                                               */
#include <errno.h>
#include <arpa/inet.h>  /* inet_pton                                          */

/* Set by the signal handler; checked by the loop. `volatile sig_atomic_t` is
 * the ONLY type the C standard guarantees is safe to write in a handler and
 * read in the main flow without a lock — reads/writes are atomic and the
 * `volatile` stops the compiler from caching it in a register across the loop. */
static volatile sig_atomic_t g_stop = 0;

static void on_signal(int signo)
{
    (void)signo;
    g_stop = 1;   /* just flip the flag; do real work back in the main loop    */
}

int main(int argc, char **argv)
{
    struct netif nif;
    memset(&nif, 0, sizeof(nif));

    /* --- Our IP address (default 10.0.0.2; overridable as argv[1]). --------
     * inet_pton writes the address in NETWORK byte order, which is exactly how
     * we store and compare it throughout the stack (matches the wire). */
    const char *ipstr = (argc > 1) ? argv[1] : "10.0.0.2";
    if (inet_pton(AF_INET, ipstr, &nif.ip) != 1) {
        LOGF("main: bad IP '%s'\n", ipstr);
        return EXIT_FAILURE;
    }

    /* --- Our MAC address. -------------------------------------------------
     * We pick a LOCALLY ADMINISTERED, UNICAST address: in the first octet, bit
     * 1 (0x02) = "locally administered" (not a vendor-assigned OUI), and bit 0
     * (0x01) = 0 means unicast. So 02:00:00:00:00:02 is a safe made-up MAC that
     * won't collide with real hardware. */
    static const u8 our_mac[ETH_ALEN] = { 0x02, 0x00, 0x00, 0x00, 0x00, 0x02 };
    memcpy(nif.mac, our_mac, ETH_ALEN);

    /* --- Open the TAP device. --------------------------------------------- */
    char dev[16];
    strncpy(dev, "tap0", sizeof(dev) - 1);
    dev[sizeof(dev) - 1] = '\0';
    nif.fd = tap_open(dev);
    if (nif.fd < 0)
        return EXIT_FAILURE;   /* tap_open already explained why               */
    strncpy(nif.name, dev, sizeof(nif.name) - 1);

    LOGF("main: interface %s up, our IP %s, MAC 02:00:00:00:00:02\n",
         nif.name, ipstr);
    LOGF("main: configure the host side with:\n"
         "        sudo ip addr add 10.0.0.1/24 dev %s\n"
         "        sudo ip link set %s up\n", nif.name, nif.name);

    /* --- Install signal handlers for a clean shutdown. -------------------- */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_signal;   /* no SA_RESTART: we WANT poll() to return EINTR
                                  *   so the loop notices g_stop promptly.      */
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    /* --- Applications: an echo service on TCP and UDP port 7. -------------- */
    tcp_init();
    tcp_listen(7);
    /* (UDP echo on port 7 is handled directly in udp_input.) */

    /* --- The event loop. -------------------------------------------------- */
    struct pollfd pfd;
    pfd.fd     = nif.fd;
    pfd.events = POLLIN;   /* wake us when a frame is readable                  */

    u8 frame[FRAME_MAX];

    while (!g_stop) {
        /* poll(2): number 7. args: rdi=&fds, rsi=nfds=1, rdx=timeout_ms=100.
         * The 100 ms timeout bounds how long we sleep so timers stay responsive
         * even with zero traffic. Returns >0 (fds ready), 0 (timeout), or -1. */
        int r = poll(&pfd, 1, 100);
        if (r < 0) {
            if (errno == EINTR)
                break;              /* a signal (probably our SIGINT) — stop     */
            LOGF("main: poll: %s\n", strerror(errno));
            break;
        }

        /* Drive the timers on EVERY wakeup, including timeouts (r == 0). */
        tcp_timer_tick(&nif);

        if (r > 0 && (pfd.revents & POLLIN)) {
            /* One frame per readable wakeup; poll re-fires if more are queued.
             * A blocking read here returns immediately because poll just told us
             * data is present. */
            long n = tap_read(nif.fd, frame, sizeof(frame));
            if (n > 0)
                eth_input(&nif, frame, (size_t)n);
            else if (n == 0)
                break;              /* EOF: the device went away                 */
            /* n < 0 was already logged by tap_read; keep looping.               */
        }

        /* POLLERR/POLLHUP mean the fd is broken (interface removed). Stop. */
        if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
            LOGF("main: tap fd error (revents=0x%x)\n", pfd.revents);
            break;
        }
    }

    LOGF("main: shutting down\n");
    close(nif.fd);   /* releases the tap; the kernel tears the interface down   */
    return EXIT_SUCCESS;
}
