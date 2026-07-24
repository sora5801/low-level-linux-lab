/* ===========================================================================
 * replay.c — the sliding-window anti-replay check (RFC 6479 style).
 * ===========================================================================
 *
 * State: `recv` (highest counter accepted) and a bitmap organised as a RING of
 * REPLAY_BITS bits. A counter c maps to ring position (c mod REPLAY_BITS); since
 * REPLAY_BITS is a power of two, that is just `c & (REPLAY_BITS-1)`, split into a
 * word index and a bit index. Because it is a ring, position(c) and
 * position(c + REPLAY_BITS) collide — which is fine, because those two counters
 * can never both be "in the window" at once, and we clear a word before reusing
 * it for a newer counter.
 *
 * The RFC 6479 insight: advancing the window does NOT shift the whole bitmap
 * (O(window) work per packet). Instead we zero only the WORDS that lie between
 * the old top and the new top — the words about to hold new counters. The one
 * spare word (REPLAY_WINDOW is one word short of the ring) guarantees the words
 * still inside the window are never among those cleared.
 * =========================================================================== */

#include "replay.h"

void replay_init(struct replay_window *w)
{
    for (u32 i = 0; i < REPLAY_WORDS; i++) w->bitmap[i] = 0;
    w->recv = 0;
    w->initialized = 0;
}

int replay_check(struct replay_window *w, u64 counter)
{
    const u64 WORD_BITS = 64;
    const u64 word_mask = REPLAY_WORDS - 1;   /* power-of-two ring index mask     */
    const u64 bit_mask  = WORD_BITS - 1;

    /* First packet of the session establishes the baseline: accept it, remember
     * it, and set its bit. The sender's counter starts at 0, so counter 0 is a
     * legitimate first value — we cannot special-case "0 == not yet seen". */
    if (!w->initialized) {
        w->initialized = 1;
        w->recv = counter;
        w->bitmap[(counter / WORD_BITS) & word_mask] |= (u64)1 << (counter & bit_mask);
        return 0;
    }

    if (counter > w->recv) {
        /* NEW HIGH: the window slides forward to `counter`. Zero every word that
         * lies strictly after the old top's word, up to and including the new
         * top's word — those words are about to be (re)used for fresh counters
         * and may still hold stale bits from REPLAY_BITS packets ago. */
        u64 old_word = w->recv   / WORD_BITS;
        u64 new_word = counter   / WORD_BITS;
        u64 span = new_word - old_word;           /* how many word-rows we crossed */
        if (span >= REPLAY_WORDS) {
            /* Jumped so far the entire ring is stale; clear all of it. */
            for (u32 i = 0; i < REPLAY_WORDS; i++) w->bitmap[i] = 0;
        } else {
            for (u64 wnum = old_word + 1; wnum <= new_word; wnum++)
                w->bitmap[wnum & word_mask] = 0;
        }
        w->recv = counter;
        w->bitmap[new_word & word_mask] |= (u64)1 << (counter & bit_mask);
        return 0;                                  /* fresh                        */
    }

    /* counter <= recv: it is at or behind the top of the window. */
    if (w->recv - counter >= REPLAY_WINDOW)
        return -1;                                 /* too old — fell off the window */

    u64 wi = (counter / WORD_BITS) & word_mask;
    u64 bit = (u64)1 << (counter & bit_mask);
    if (w->bitmap[wi] & bit)
        return -1;                                 /* duplicate — already accepted  */

    w->bitmap[wi] |= bit;                           /* record this in-window counter */
    return 0;
}
