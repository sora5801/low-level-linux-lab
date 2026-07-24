/* ===========================================================================
 * chacha20poly1305.h — the AEAD (RFC 8439). One key, one nonce, one call to
 *                      seal/open a message plus its associated data.
 * ===========================================================================
 *
 * An AEAD ("Authenticated Encryption with Associated Data") is the workhorse of
 * a modern secure channel. `seal` turns plaintext into ciphertext + a 16-byte
 * tag; `open` verifies the tag and, only if it matches, returns the plaintext.
 * The "associated data" (AAD) is authenticated but NOT encrypted — in Noise it
 * is the running transcript hash `h`, which binds each encryption to everything
 * said so far in the handshake.
 *
 * NONCE DISCIPLINE — the one rule you must never break: with ChaCha20-Poly1305,
 * a (key, nonce) pair must NEVER encrypt two different messages. Reusing a nonce
 * under one key leaks the XOR of plaintexts AND lets an attacker forge tags. We
 * enforce this with strictly-increasing counters (the CipherState nonce in the
 * handshake, the per-packet counter in transport). See the callers.
 * =========================================================================== */
#ifndef CHACHA20POLY1305_H
#define CHACHA20POLY1305_H

#include "wg.h"

/* Produce one 64-byte ChaCha20 keystream block. Exposed because the AEAD uses
 * block 0 to derive the Poly1305 one-time key, and because asm/demo.c is an
 * extraction of exactly this routine. key: 32 bytes, nonce: 12 bytes. */
void chacha20_block(u8 out[64], const u8 key[AEAD_KEY_LEN],
                    u32 counter, const u8 nonce[AEAD_NONCE_LEN]);

/* AEAD seal: encrypt `plaintext_len` bytes and append a 16-byte tag.
 *   out        must have room for plaintext_len + AEAD_TAG_LEN bytes.
 *   nonce      12 bytes; MUST be unique per key (see the nonce rule above).
 *   ad/ad_len  associated data, authenticated but not encrypted (may be NULL/0).
 * `out` may alias `plaintext` (in-place encryption is supported). */
void aead_seal(u8 *out, const u8 key[AEAD_KEY_LEN],
               const u8 nonce[AEAD_NONCE_LEN],
               const u8 *ad, usize ad_len,
               const u8 *plaintext, usize plaintext_len);

/* AEAD open: verify the tag and decrypt. `ciphertext_len` INCLUDES the trailing
 * 16-byte tag. On success writes ciphertext_len - AEAD_TAG_LEN plaintext bytes
 * to `out` and returns 0. On tag mismatch returns -1 and writes NOTHING the
 * caller should trust (we still decrypt into out, but you MUST ignore it — the
 * whole point of an AEAD is that unauthenticated plaintext is worthless). The
 * tag comparison is constant-time. */
int aead_open(u8 *out, const u8 key[AEAD_KEY_LEN],
              const u8 nonce[AEAD_NONCE_LEN],
              const u8 *ad, usize ad_len,
              const u8 *ciphertext, usize ciphertext_len);

#endif /* CHACHA20POLY1305_H */
