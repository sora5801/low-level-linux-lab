/* ===========================================================================
 * tun.h — the TUN virtual network device: userspace's tap into the IP stack.
 * ===========================================================================
 *
 * A TUN device is a virtual network interface whose "wire" is a file descriptor.
 * When the kernel routes an IP packet to the interface, we read() it as raw
 * bytes; when we write() an IP packet to the fd, the kernel injects it as if it
 * had arrived on that interface. That is the whole trick of a userspace VPN: the
 * OS does normal routing to `wg0`, and OUR process is the cable — we encrypt
 * outbound packets and send them over UDP, and decrypt inbound UDP into packets.
 *
 *      apps ── kernel routing ──> [wg0 TUN] ──read()──> us ──encrypt──> UDP ─╮
 *      apps <── kernel routing ── [wg0 TUN] <─write()── us <──decrypt── UDP ─╯
 *
 * "TUN" carries layer-3 (IP) packets; its sibling "TAP" carries layer-2
 * (Ethernet) frames. WireGuard is layer 3, so we use TUN.
 * =========================================================================== */
#ifndef TUN_H
#define TUN_H

#include "wg.h"

/* Create/attach a TUN device named `name` (e.g. "wg0"); if it already exists and
 * is a TUN, we attach to it. On success returns the fd to read/write packets on
 * and copies the kernel-assigned name back into `name` (sized >= 16). Returns -1
 * on error (errno set). Requires CAP_NET_ADMIN (root). */
int tun_open(char name[16]);

/* Set the device MTU (SIOCSIFMTU) and bring the link UP (SIOCSIFFLAGS|IFF_UP)
 * via a control socket. Returns 0 or -1. */
int tun_configure(const char *name, int mtu);

#endif /* TUN_H */
