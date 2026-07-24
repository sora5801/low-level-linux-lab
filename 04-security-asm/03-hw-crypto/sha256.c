/* ===========================================================================
 * sha256.c — the SHA-256 padding + length-encoding driver (back-end agnostic).
 * ===========================================================================
 *
 * SHA-256 (FIPS 180-4) processes the message in 512-bit (64-byte) blocks. The
 * message almost never ends on a block boundary, so it is PADDED:
 *
 *   1. append a single '1' bit  -> the byte 0x80 (messages here are byte-aligned),
 *   2. append '0' bits until the length is 56 mod 64,
 *   3. append the ORIGINAL message length in BITS as a 64-bit big-endian integer.
 *
 * That makes the padded length a multiple of 64 and encodes the length so two
 * different messages can't share a padded form (Merkle-Damgard strengthening).
 *
 * This driver is deliberately independent of HOW a block is compressed: it takes
 * a `sha256_transform_fn` and calls it for the bulk blocks and again for the 1-2
 * padding blocks. Both the software and SHA-NI back ends share it verbatim.
 * ===========================================================================
 */
#include "sha256.h"
#include <string.h>   /* memcpy/memset: operate on message bytes, not secrets  */

void sha256_with(const uint8_t *msg, size_t len, uint8_t out[SHA256_DIGEST_BYTES],
                 sha256_transform_fn tf)
{
    /* SHA-256 initial hash value H0: the fractional parts of the square roots of
     * the first 8 primes (FIPS 180-4 §5.3.3). Big-endian words in `state`. */
    uint32_t state[8] = {
        0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
        0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u
    };

    uint64_t bit_len = (uint64_t)len * 8u;      /* length in BITS for the pad   */
    size_t   full    = len / SHA256_BLOCK_BYTES; /* number of whole 64B blocks  */

    /* Compress all the complete blocks straight from the caller's buffer. */
    if (full) tf(state, msg, full);

    /* Build the final 1 or 2 padded blocks in a local buffer. `rem` bytes of
     * real message are left over (0..63). */
    uint8_t block[2 * SHA256_BLOCK_BYTES];
    size_t  rem = len - full * SHA256_BLOCK_BYTES;
    memcpy(block, msg + full * SHA256_BLOCK_BYTES, rem);

    block[rem++] = 0x80;                          /* the mandatory '1' bit       */

    size_t nblk;
    if (rem > SHA256_BLOCK_BYTES - 8) {
        /* Not enough room for the 8-byte length in this block -> use two. */
        memset(block + rem, 0, 2 * SHA256_BLOCK_BYTES - rem);
        nblk = 2;
    } else {
        memset(block + rem, 0, SHA256_BLOCK_BYTES - rem);
        nblk = 1;
    }

    /* Write the 64-bit big-endian bit length into the final 8 bytes. */
    size_t off = nblk * SHA256_BLOCK_BYTES - 8;
    for (int i = 0; i < 8; i++)
        block[off + i] = (uint8_t)(bit_len >> (56 - 8 * i));

    tf(state, block, nblk);

    /* Serialize the 8 state words big-endian into the 32-byte digest. */
    for (int i = 0; i < 8; i++) {
        out[4 * i + 0] = (uint8_t)(state[i] >> 24);
        out[4 * i + 1] = (uint8_t)(state[i] >> 16);
        out[4 * i + 2] = (uint8_t)(state[i] >>  8);
        out[4 * i + 3] = (uint8_t)(state[i]      );
    }
}
