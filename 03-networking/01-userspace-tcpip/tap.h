/* ===========================================================================
 * tap.h — the one file descriptor that IS our network link.
 * ===========================================================================
 * A TAP device is a virtual Ethernet NIC whose "wire" is a file descriptor:
 * read() returns a frame the kernel handed us, write() injects a frame into the
 * kernel as if it arrived on the NIC. Our whole stack lives on top of this fd.
 * ========================================================================= */
#ifndef USERSPACE_TCPIP_TAP_H
#define USERSPACE_TCPIP_TAP_H

#include "common.h"

/* Open /dev/net/tun in TAP mode and bind it to interface name `dev` (e.g.
 * "tap0"). Returns the fd (>= 0) or -1 on error. On success, `dev` is filled
 * with the kernel-assigned name if you passed an empty string. */
int tap_open(char *dev);

/* Read one Ethernet frame into `buf` (capacity `len`). Returns bytes read, or
 * -1 on a real error. Blocks unless the fd was made non-blocking. */
long tap_read(int fd, void *buf, size_t len);

/* Write one Ethernet frame. Returns bytes written or -1. */
long tap_write(int fd, const void *buf, size_t len);

#endif /* USERSPACE_TCPIP_TAP_H */
