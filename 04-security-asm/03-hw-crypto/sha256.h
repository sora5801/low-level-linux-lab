/* ===========================================================================
 * sha256.h — SHA-256 with two interchangeable block transforms.
 * ===========================================================================
 *
 * SHA-256 hashes a message in 64-byte blocks, updating an 8-word (256-bit)
 * state. Two things vary between our back ends, and only one of them:
 *
 *   - the PADDING + length-encoding + the 8x32 state plumbing is IDENTICAL for
 *     both, so it lives once in sha256.c as a driver, and
 *   - the per-block COMPRESSION FUNCTION differs: sha256_soft.c computes it with
 *     ordinary 32-bit arithmetic; sha256_ni.c computes it with the x86 SHA
 *     extensions (sha256rnds2 / sha256msg1 / sha256msg2).
 *
 * The driver takes the transform as a function pointer, so the same padding
 * code drives either back end. A block transform has this contract:
 *
 *   void transform(uint32_t state[8], const uint8_t *data, size_t nblocks);
 *
 * It reads `nblocks` * 64 bytes from `data` and folds them into `state`.
 *
 * SIDE-CHANNEL NOTE.  Unlike AES, SHA-256 has NO secret-indexed table lookups in
 * the first place — its round constants are fixed and its schedule is pure
 * arithmetic — so even the software transform is naturally constant-time. The
 * SHA extensions are here for SPEED (and, as with AES-NI, they also do the work
 * with no data-dependent memory access).
 * ===========================================================================
 */
#ifndef HWCRYPTO_SHA256_H
#define HWCRYPTO_SHA256_H

#include <stdint.h>
#include <stddef.h>

#define SHA256_DIGEST_BYTES 32
#define SHA256_BLOCK_BYTES  64

/* A per-block compression function (see contract above). */
typedef void (*sha256_transform_fn)(uint32_t state[8], const uint8_t *data, size_t nblocks);

/* Pad `msg`, then hash it with `tf`, writing 32 bytes to `out`. Shared driver. */
void sha256_with(const uint8_t *msg, size_t len, uint8_t out[SHA256_DIGEST_BYTES],
                 sha256_transform_fn tf);

/* The two block transforms. sha256_ni_transform must only be called after cpuid
 * confirms the SHA extensions (calling it otherwise is SIGILL). */
void sha256_soft_transform(uint32_t state[8], const uint8_t *data, size_t nblocks);
void sha256_ni_transform  (uint32_t state[8], const uint8_t *data, size_t nblocks);

/* Convenience wrappers so callers don't juggle function pointers. */
static inline void sha256_soft(const uint8_t *msg, size_t len, uint8_t out[SHA256_DIGEST_BYTES])
{ sha256_with(msg, len, out, sha256_soft_transform); }
static inline void sha256_ni(const uint8_t *msg, size_t len, uint8_t out[SHA256_DIGEST_BYTES])
{ sha256_with(msg, len, out, sha256_ni_transform); }

#endif /* HWCRYPTO_SHA256_H */
