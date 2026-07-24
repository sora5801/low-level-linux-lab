/* ===========================================================================
 * tun.c — open and configure the TUN device with the Linux ioctl interface.
 * ===========================================================================
 * Linux-only: /dev/net/tun and the TUNSETIFF ioctl are kernel interfaces.
 * =========================================================================== */

#include "tun.h"

#include <string.h>       /* strncpy, memset                                   */
#include <errno.h>
#include <fcntl.h>        /* open, O_RDWR                                       */
#include <unistd.h>       /* close                                             */
#include <sys/ioctl.h>    /* ioctl                                             */
#include <sys/socket.h>   /* socket (for the config ioctls)                    */
#include <net/if.h>       /* struct ifreq, IFF_UP                              */
#include <linux/if_tun.h> /* TUNSETIFF, IFF_TUN, IFF_NO_PI                     */

int tun_open(char name[16])
{
    /* /dev/net/tun is the clone device: opening it and then issuing TUNSETIFF
     * either creates a new TUN interface or attaches to an existing one. */
    int fd = open("/dev/net/tun", O_RDWR);
    if (fd < 0) return -1;                 /* ENOENT if the tun module is absent */

    struct ifreq ifr;
    memset(&ifr, 0, sizeof ifr);
    /* IFF_TUN   : layer-3, we exchange bare IP packets (no Ethernet header).
     * IFF_NO_PI : no 4-byte "packet information" prefix on each read/write, so
     *             what we read() is exactly the IP packet — nothing to strip. */
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
    if (name[0])
        strncpy(ifr.ifr_name, name, IFNAMSIZ - 1);   /* request a specific name  */

    /* TUNSETIFF binds this fd to the (new or existing) interface described by
     * ifr. On return ifr.ifr_name holds the actual name the kernel chose. */
    if (ioctl(fd, TUNSETIFF, &ifr) < 0) {
        int e = errno;
        close(fd);
        errno = e;
        return -1;
    }

    strncpy(name, ifr.ifr_name, IFNAMSIZ - 1);
    name[IFNAMSIZ - 1] = '\0';
    return fd;
}

int tun_configure(const char *name, int mtu)
{
    /* netdevice ioctls (man 7 netdevice) go through ANY socket, not the tun fd:
     * the socket is just a handle into the networking stack, and ifr_name
     * selects which interface to poke. A datagram socket is the conventional
     * choice. */
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) return -1;

    struct ifreq ifr;
    memset(&ifr, 0, sizeof ifr);
    strncpy(ifr.ifr_name, name, IFNAMSIZ - 1);

    /* SIOCSIFMTU: cap the interface MTU so the kernel never hands us an inner
     * packet too large to fit one encrypted UDP datagram (see wg.h MTU math). */
    ifr.ifr_mtu = mtu;
    if (ioctl(s, SIOCSIFMTU, &ifr) < 0) goto fail;

    /* SIOCGIFFLAGS then SIOCSIFFLAGS with IFF_UP|IFF_RUNNING: read the current
     * flags, OR in "up", write them back — the equivalent of `ip link set up`. */
    if (ioctl(s, SIOCGIFFLAGS, &ifr) < 0) goto fail;
    ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
    if (ioctl(s, SIOCSIFFLAGS, &ifr) < 0) goto fail;

    close(s);
    return 0;
fail:
    {
        int e = errno;
        close(s);
        errno = e;
        return -1;
    }
}
