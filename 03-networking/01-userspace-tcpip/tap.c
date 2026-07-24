/* ===========================================================================
 * tap.c — opening and driving the TUN/TAP device.
 * ===========================================================================
 *
 * WHAT A TAP DEVICE IS
 * --------------------
 * /dev/net/tun is a "clone" character device. When you open it and issue the
 * TUNSETIFF ioctl, the kernel creates (or attaches you to) a virtual network
 * interface. Two flavors:
 *
 *   IFF_TUN  — a layer-3 device: read()/write() move bare IPv4/IPv6 PACKETS.
 *              No Ethernet header, no ARP. Good for VPNs (point-to-point).
 *   IFF_TAP  — a layer-2 device: read()/write() move full ETHERNET FRAMES.
 *              You see/emit MAC headers and must answer ARP yourself.
 *
 * We choose TAP so this project can teach the whole stack from Ethernet up —
 * including ARP, which only exists on a broadcast layer-2 link. The kernel side
 * of the interface (e.g. "tap0", address 10.0.0.1) behaves like a real NIC:
 * `ping 10.0.0.2` from the host emits an ARP request and an ICMP echo onto the
 * tap, which our read() delivers; our write() of the replies makes the host's
 * kernel believe a peer at 10.0.0.2 answered.
 *
 * IFF_NO_PI ("no packet information") strips the 4-byte metadata prefix the tun
 * driver would otherwise prepend to every frame. Without it, buf[0..3] would be
 * flags+protocol and every offset in the rest of the stack would be off by 4.
 * ========================================================================= */

#include "tap.h"

#include <fcntl.h>       /* open, O_RDWR                                       */
#include <unistd.h>      /* read, write, close                                */
#include <string.h>      /* memset, strncpy                                   */
#include <errno.h>       /* errno, EINTR                                      */
#include <sys/ioctl.h>   /* ioctl                                             */
#include <linux/if.h>    /* struct ifreq, IFF_* interface flags               */
#include <linux/if_tun.h>/* IFF_TAP, IFF_NO_PI, TUNSETIFF                     */

/* ---------------------------------------------------------------------------
 * tap_open — create/attach a TAP interface and return its fd.
 * ------------------------------------------------------------------------- */
int tap_open(char *dev)
{
    struct ifreq ifr;
    int fd, err;

    /* open("/dev/net/tun", O_RDWR) — syscall openat(2) under the hood.
     *   arg fd/dirfd : AT_FDCWD (relative to cwd), supplied by the wrapper
     *   arg pathname : "/dev/net/tun"
     *   arg flags    : O_RDWR (we both read frames and write them)
     * Returns a new fd, or -1 with errno set (ENOENT if the tun module is not
     * loaded, EACCES/EPERM if we lack CAP_NET_ADMIN and are not root). */
    fd = open("/dev/net/tun", O_RDWR);
    if (fd < 0) {
        LOGF("tap: open(/dev/net/tun): %s\n", strerror(errno));
        return -1;
    }

    /* TUNSETIFF wants an `ifreq` describing what kind of device we want and,
     * optionally, its name. Zero it first so no stale stack bytes leak into the
     * request (the padding is interpreted by the kernel). */
    memset(&ifr, 0, sizeof(ifr));

    /* IFF_TAP  : layer-2 (Ethernet frames).  IFF_NO_PI : no 4-byte prefix. */
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;

    /* If the caller named an interface, request it; else the kernel assigns one
     * (e.g. "tap0") and writes the chosen name back into ifr for us. strncpy
     * with IFNAMSIZ-1 leaves room for the guaranteed NUL. */
    if (dev && *dev)
        strncpy(ifr.ifr_name, dev, IFNAMSIZ - 1);

    /* ioctl(fd, TUNSETIFF, &ifr) — the request that actually materializes the
     * interface. On failure (commonly EPERM: need CAP_NET_ADMIN) we must close
     * the fd we opened so we don't leak it. */
    err = ioctl(fd, TUNSETIFF, (void *)&ifr);
    if (err < 0) {
        LOGF("tap: ioctl(TUNSETIFF): %s "
             "(need root or CAP_NET_ADMIN?)\n", strerror(errno));
        close(fd);
        return -1;
    }

    /* Hand the kernel-assigned name back to the caller so it can `ip addr` it. */
    if (dev)
        strcpy(dev, ifr.ifr_name);

    return fd;
}

/* ---------------------------------------------------------------------------
 * tap_read — pull exactly one Ethernet frame off the device.
 *
 * The tun driver is packet-oriented: ONE read() returns ONE whole frame (never
 * a partial frame, never two coalesced), so we do not need to reassemble a byte
 * stream the way a TCP socket would. We retry on EINTR — a signal that
 * interrupts a blocking read is not a network error, just a nudge.
 * ------------------------------------------------------------------------- */
long tap_read(int fd, void *buf, size_t len)
{
    long n;
    do {
        /* read(2): number 0. args: rdi=fd, rsi=buf, rdx=len. Returns bytes
         * read (a full frame), 0 at EOF (device gone), or -1/errno. */
        n = read(fd, buf, len);
    } while (n < 0 && errno == EINTR);   /* restart if only a signal hit us    */

    if (n < 0)
        LOGF("tap: read: %s\n", strerror(errno));
    return n;
}

/* ---------------------------------------------------------------------------
 * tap_write — inject one Ethernet frame into the kernel.
 *
 * Like read, a write() to the tun device is atomic per frame: it accepts the
 * whole frame or fails; there is no "partial frame written" to loop over the way
 * a stream socket has. We still retry EINTR. A short write here would indicate a
 * malformed length, which we surface as an error.
 * ------------------------------------------------------------------------- */
long tap_write(int fd, const void *buf, size_t len)
{
    long n;
    do {
        /* write(2): number 1. args: rdi=fd, rsi=buf, rdx=len. */
        n = write(fd, buf, len);
    } while (n < 0 && errno == EINTR);

    if (n < 0)
        LOGF("tap: write: %s\n", strerror(errno));
    else if ((size_t)n != len)
        LOGF("tap: short write %ld/%zu\n", n, len);
    return n;
}
