/* ===========================================================================
 * replay.h — anti-replay sliding window for received transport packets.
 * ===========================================================================
 *
 * A tunnel encrypts every packet under a strictly-increasing 64-bit counter
 * (the AEAD nonce). Authentication alone stops FORGERY, but not REPLAY: an
 * attacker can re-send a previously-valid encrypted packet verbatim and it will
 * still authenticate. The receiver must therefore remember which counters it has
 * already accepted and reject duplicates.
 *
 * Remembering every counter forever is impossible, and packets can arrive
 * slightly out of order (the network reorders), so we keep a SLIDING WINDOW: the
 * highest counter seen, plus a bitmap of the recent counters below it. A counter
 * is accepted iff it is newer than the window, or inside the window with its bit
 * still clear. This is the exact mechanism IPsec (RFC 4303/6479) and WireGuard
 * use; the implementation here follows RFC 6479 ("without bit shifting").
 * =========================================================================== */
#ifndef REPLAY_H
#define REPLAY_H

#include "wg.h"

/* We index the bitmap as a power-of-two RING of bits so advancing the window is
 * just "zero the words we are about to reuse" — no O(window) shift per packet.
 * REPLAY_WORDS must be a power of two; the effective window is one word smaller
 * than the ring (the spare word is the guard that keeps in-window words from
 * being cleared too early). 32 words * 64 bits = 2048-bit ring, ~1984 window. */
#define REPLAY_WORDS   32u
#define REPLAY_BITS    (REPLAY_WORDS * 64u)          /* ring size in bits         */
#define REPLAY_WINDOW  ((REPLAY_WORDS - 1u) * 64u)   /* usable window in packets  */

struct replay_window {
    u64 bitmap[REPLAY_WORDS];   /* one bit per counter, indexed modulo the ring  */
    u64 recv;                   /* highest counter accepted so far               */
    int initialized;            /* false until the first packet sets the baseline */
};

/* Reset a window to "nothing received yet". Call once per session. */
void replay_init(struct replay_window *w);

/* Check-and-update in one atomic step: returns 0 if `counter` is fresh (and
 * records it), or -1 if it is a replay / too old. MUST be called only AFTER the
 * packet's AEAD tag has verified — otherwise an attacker could poison the window
 * with forged counters. */
int replay_check(struct replay_window *w, u64 counter);

#endif /* REPLAY_H */
