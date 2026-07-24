/* ===========================================================================
 * app_reflector.c — a line-rate packet reflector on top of the ixgbe driver.
 * ===========================================================================
 *
 * The demo: poll the RX ring, swap each frame's Ethernet source/destination MAC
 * (so it is a valid "reply"), and transmit it back out the same port. This
 * exercises the entire driver — RX polling, buffer refill, TX posting, and TX
 * completion reclaim — in the tightest possible loop, and prints throughput.
 *
 * It is single-threaded and busy-polls: one CPU core drives the whole NIC with
 * no interrupts and no syscalls in the steady state. That is the shape of every
 * DPDK/ixy dataplane. Run it pinned to an isolated core for real numbers.
 *
 * REQUIRES: real Intel 82599 hardware bound away from the kernel, hugepages,
 * and root. See the README run guide — this cannot run in this dev environment.
 * ========================================================================= */
#define _GNU_SOURCE
#include "driver.h"
#include "memory.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

/* How many packets to move per driver call. Batching amortises the per-call
 * overhead (the tail-register doorbell, the loop setup) across many frames — the
 * single most important throughput knob in a poll-mode driver. */
#define BATCH_SIZE 32

/* Return a monotonic timestamp in nanoseconds for rate calculation. CLOCK_
 * MONOTONIC never jumps (unlike wall-clock), so deltas are always sane. */
static uint64_t monotonic_ns(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
        error("clock_gettime");
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

/* Swap the destination and source MAC addresses in-place. An Ethernet frame is
 * [ dst MAC : 6 ][ src MAC : 6 ][ ethertype : 2 ][ payload ... ]. Reflecting
 * means the reply goes back to whoever sent it, so dst<->src. We touch bytes
 * 0..11 only; endianness is irrelevant because MACs are byte arrays, not
 * multi-byte integers. */
static void swap_mac(uint8_t *frame)
{
    for (int i = 0; i < 6; i++) {
        uint8_t tmp   = frame[i];      /* dst[i] */
        frame[i]      = frame[6 + i];  /* dst[i] = src[i] */
        frame[6 + i]  = tmp;           /* src[i] = old dst[i] */
    }
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s <pci-addr>   e.g. %s 0000:03:00.0\n",
                argv[0], argv[0]);
        return 1;
    }
    const char *pci_addr = argv[1];

    /* Bring the NIC up: 1 RX queue, 1 TX queue. Blocks until link is up. */
    struct ixy_device *dev = ixgbe_init(pci_addr, 1, 1);

    struct pkt_buf *bufs[BATCH_SIZE];
    uint64_t total_fwd = 0;             /* packets reflected since last report */
    uint64_t last_report = monotonic_ns();

    info("reflector running on %s — Ctrl-C to stop", pci_addr);
    for (;;) {
        /* 1) RECEIVE: harvest up to BATCH_SIZE completed frames. Returns 0 very
         *    often (the NIC produced nothing since we last looked) — that is a
         *    normal poll result, not an error. */
        uint32_t rx = ixgbe_rx_batch(dev, 0, bufs, BATCH_SIZE);

        /* 2) TRANSFORM: rewrite each frame so it is a valid reflected reply. */
        for (uint32_t i = 0; i < rx; i++)
            swap_mac(bufs[i]->data);

        /* 3) TRANSMIT: hand the buffers to the TX ring. The driver takes
         *    ownership of the ones it accepts and frees them after the NIC
         *    signals completion. */
        uint32_t tx = ixgbe_tx_batch(dev, 0, bufs, rx);

        /* 4) BACKPRESSURE: if the TX ring was full, tx < rx. We still OWN the
         *    unsent buffers, so we must free them ourselves or leak the pool. */
        for (uint32_t i = tx; i < rx; i++)
            pkt_buf_free(bufs[i]);

        total_fwd += tx;

        /* 5) REPORT roughly once per second. We only read the clock when we did
         *    work, so an idle poll loop stays a pure memory spin. */
        if (rx > 0) {
            uint64_t now = monotonic_ns();
            uint64_t elapsed = now - last_report;
            if (elapsed >= 1000000000ull) {
                double mpps = (double)total_fwd / (double)elapsed * 1000.0;
                printf("reflected %.3f Mpps\n", mpps);
                fflush(stdout);
                total_fwd   = 0;
                last_report = now;
            }
        }
    }

    /* Unreachable in this demo (loop is infinite); a real app would tear down
     * the device and free the mempools here. */
    return 0;
}
