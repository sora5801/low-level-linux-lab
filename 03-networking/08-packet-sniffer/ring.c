/* ===========================================================================
 * ring.c — PACKET_MMAP TPACKET_V2 receive ring: setup, drain, teardown.
 * ===========================================================================
 *
 * THE SHARED-MEMORY PROTOCOL (why this is fast, and where the races are)
 * ---------------------------------------------------------------------
 * After PACKET_RX_RING + mmap, the kernel and our process share one big buffer
 * divided into fixed-size FRAME slots. Each slot begins with a `tpacket2_hdr`
 * whose `tp_status` word is a SINGLE-WRITER-EACH-WAY ownership flag:
 *
 *     tp_status == TP_STATUS_KERNEL (0)  -> the KERNEL owns the slot; it may be
 *                                           writing a packet into it right now.
 *     tp_status &  TP_STATUS_USER   (1)  -> the kernel has FINISHED a packet
 *                                           here and handed the slot to us.
 *
 * The handoff is a classic producer/consumer over shared memory, so it needs
 * MEMORY ORDERING, not just a plain read/write of the flag:
 *
 *   - Consuming (us): we must LOAD tp_status with ACQUIRE semantics. The acquire
 *     prevents the CPU/compiler from hoisting our reads of the packet BYTES
 *     above the flag check. Without it we could read header/payload fields the
 *     kernel hasn't finished writing (it sets TP_STATUS_USER *after* filling the
 *     frame; acquire makes "flag says ready" imply "bytes are visible").
 *
 *   - Releasing (us): after we finish reading the frame we STORE
 *     TP_STATUS_KERNEL with RELEASE semantics. Release prevents our reads of the
 *     frame from sinking BELOW the store, so the kernel can't start overwriting
 *     the slot until we're truly done. Getting this backwards is a silent
 *     data-corruption bug that only shows up under load.
 *
 * We use the GCC/Clang __atomic builtins with __ATOMIC_ACQUIRE / _RELEASE on the
 * tp_status word for exactly this. On x86 these compile to plain MOVs (x86 is
 * strongly ordered) PLUS a compiler barrier — but writing the ordering
 * explicitly keeps the code correct on weakly-ordered CPUs (ARM/POWER) too.
 * ===========================================================================
 */

#include "ring.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>             /* sysconf(_SC_PAGESIZE)                       */
#include <sys/mman.h>          /* mmap, munmap                                */
#include <sys/socket.h>        /* setsockopt, SOL_PACKET                      */
#include <linux/if_packet.h>   /* tpacket_req, tpacket2_hdr, TP_STATUS_*      */

/* SOL_PACKET is the setsockopt "level" for AF_PACKET options. Glibc/musl put it
 * in <sys/socket.h>; define it defensively in case a stripped libc omits it. */
#ifndef SOL_PACKET
#define SOL_PACKET 263
#endif

/* Ring geometry. The kernel imposes constraints we must satisfy exactly, or
 * PACKET_RX_RING returns EINVAL:
 *   - frame_size >= TPACKET2_HDRLEN and a multiple of TPACKET_ALIGNMENT(16)
 *   - block_size is a multiple of the page size
 *   - block_size is a multiple of frame_size (frames never straddle a block)
 * We pick round numbers: 2 KiB frames, 128 KiB blocks (32 pages), 16 blocks.
 * That is 64 frames/block * 16 = 1024 slots ~= 2 MiB of mapped RAM. */
#define RING_FRAME_SIZE   2048u
#define RING_BLOCK_SIZE   (128u * 1024u)
#define RING_BLOCK_NR     16u

int ring_setup(struct ring *r, int fd)
{
    memset(r, 0, sizeof *r);
    r->fd = fd;

    /* 1. Ask for TPACKET_V2 explicitly. V1 has a 32-bit timestamp and no
     *    reliable per-frame vlan info; V2 fixes those and is the sane default.
     *    V3 uses a block-based, kernel-timed layout that is great for high rates
     *    but more complex — V2's per-frame model is clearer to teach. */
    int ver = TPACKET_V2;
    if (setsockopt(fd, SOL_PACKET, PACKET_VERSION, &ver, sizeof ver) < 0) {
        perror("setsockopt(PACKET_VERSION)");
        return -1;
    }

    /* 2. Describe the ring geometry and ask the kernel to allocate it. */
    struct tpacket_req req;
    memset(&req, 0, sizeof req);
    req.tp_frame_size = RING_FRAME_SIZE;
    req.tp_block_size = RING_BLOCK_SIZE;
    req.tp_block_nr   = RING_BLOCK_NR;
    /* Total frames = frames-per-block * blocks. Integer division is exact here
     * because block_size is a multiple of frame_size (checked by the kernel). */
    req.tp_frame_nr   = (RING_BLOCK_SIZE / RING_FRAME_SIZE) * RING_BLOCK_NR;

    if (setsockopt(fd, SOL_PACKET, PACKET_RX_RING, &req, sizeof req) < 0) {
        perror("setsockopt(PACKET_RX_RING)");
        return -1;
    }

    /* 3. mmap the whole ring into our address space. The kernel maps all blocks
     *    contiguously starting at offset 0. MAP_SHARED is REQUIRED — the whole
     *    point is that writes by the kernel are visible to us and vice-versa;
     *    MAP_PRIVATE would give us a private copy-on-write view and break the
     *    protocol. MAP_LOCKED (omitted) would pin it against swap. */
    size_t len = (size_t)req.tp_block_size * req.tp_block_nr;
    void *base = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (base == MAP_FAILED) {
        perror("mmap(PACKET_RX_RING)");
        return -1;
    }

    r->map              = (uint8_t *)base;
    r->map_len          = len;
    r->block_size       = req.tp_block_size;
    r->frame_size       = req.tp_frame_size;
    r->frame_nr         = req.tp_frame_nr;
    r->frames_per_block = req.tp_block_size / req.tp_frame_size;
    r->idx              = 0;
    return 0;
}

void ring_destroy(struct ring *r)
{
    if (r->map && r->map != MAP_FAILED) {
        munmap(r->map, r->map_len);      /* return value of munmap on teardown
                                          * is not actionable; the address space
                                          * is reclaimed on exit regardless. */
        r->map = NULL;
    }
}

/* Compute the address of frame slot `i`. Frames live inside blocks and never
 * straddle a block boundary, so the slot's block and its offset within that
 * block are separate divisions. This is the arithmetic PACKET_MMAP relies on. */
static uint8_t *frame_at(struct ring *r, unsigned i)
{
    unsigned block    = i / r->frames_per_block;
    unsigned in_block = i % r->frames_per_block;
    return r->map + (size_t)block * r->block_size + (size_t)in_block * r->frame_size;
}

unsigned ring_drain(struct ring *r, frame_cb cb, void *user)
{
    unsigned processed = 0;

    /* Walk forward from where we left off. We stop as soon as we hit a slot the
     * kernel still owns (TP_STATUS_USER clear): frames are filled in order, so
     * the first kernel-owned slot means "no more ready packets right now". */
    for (unsigned scanned = 0; scanned < r->frame_nr; scanned++) {
        struct tpacket2_hdr *h = (struct tpacket2_hdr *)frame_at(r, r->idx);

        /* ACQUIRE load: see the big comment at the top. If the kernel hasn't
         * handed us this slot, there is nothing ready; return to the poll(). */
        unsigned status = __atomic_load_n(&h->tp_status, __ATOMIC_ACQUIRE);
        if ((status & TP_STATUS_USER) == 0)
            break;

        /* tp_mac is the offset from the frame start to the link-layer (MAC)
         * header; tp_snaplen is how many bytes were captured (<= tp_len, the
         * true wire length, when the frame was snapped short). tp_status may
         * also carry TP_STATUS_COPY (the packet was longer than a frame and was
         * truncated) — we still decode what we have. */
        const uint8_t *data = (const uint8_t *)h + h->tp_mac;
        cb(data, h->tp_snaplen, h->tp_len, user);
        processed++;

        /* RELEASE store: publish "kernel may reuse this slot" only AFTER the
         * callback has finished reading the bytes. */
        __atomic_store_n(&h->tp_status, TP_STATUS_KERNEL, __ATOMIC_RELEASE);

        /* Advance round-robin to the next slot. */
        r->idx = (r->idx + 1) % r->frame_nr;
    }

    return processed;
}
