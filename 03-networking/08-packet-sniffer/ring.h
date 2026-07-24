/* ===========================================================================
 * ring.h — a PACKET_MMAP (TPACKET_V2) RX ring for zero-copy capture.
 * ===========================================================================
 *
 * The naive way to capture is recvfrom() per packet: one syscall AND one copy
 * from kernel to user for every frame. PACKET_MMAP replaces that with a shared
 * memory ring: the kernel writes each captured frame into a slot of an mmap'd
 * buffer and flips an ownership flag; userspace reads the frame in place (no
 * copy) and flips the flag back. A whole burst of packets is then drained with
 * ZERO syscalls between the poll() that woke us and the next empty ring.
 *
 * This header is the small API the capture loop uses; ring.c has the detail.
 * ===========================================================================
 */
#ifndef SNIFFER_RING_H
#define SNIFFER_RING_H

#include <stddef.h>
#include <stdint.h>

/* Opaque-ish handle describing one mmap'd RX ring. Fields are public so the
 * teaching code in ring.c/sniffer.c can read them, but treat it as owned by the
 * ring_* functions. */
struct ring {
    int       fd;               /* the AF_PACKET socket the ring is attached to */
    uint8_t  *map;              /* mmap base of the whole ring (all blocks)     */
    size_t    map_len;          /* total mapped bytes (for munmap)             */

    unsigned  block_size;      /* bytes per block (multiple of page size)      */
    unsigned  frame_size;      /* bytes per frame slot                         */
    unsigned  frame_nr;        /* total frame slots across all blocks          */
    unsigned  frames_per_block;/* frame_size divides block_size this many times*/

    unsigned  idx;             /* next frame slot to inspect (round-robin)     */
};

/* Build the RX ring on socket `fd` and mmap it. Returns 0 on success, -1 on
 * failure (errno set; a message is printed by the caller). On success the ring
 * owns an mmap that ring_destroy() must release. */
int  ring_setup(struct ring *r, int fd);

/* Release the mmap. Safe to call on a zeroed/failed ring. */
void ring_destroy(struct ring *r);

/* Callback invoked once per ready frame: (data, caplen, wirelen, user). */
typedef void (*frame_cb)(const uint8_t *data, uint32_t caplen,
                         uint32_t wirelen, void *user);

/* Drain every frame the kernel currently owns to us, calling `cb` on each and
 * then handing the slot back to the kernel. Does NOT block; call it after
 * poll() reports POLLIN. Returns the number of frames processed. */
unsigned ring_drain(struct ring *r, frame_cb cb, void *user);

#endif /* SNIFFER_RING_H */
