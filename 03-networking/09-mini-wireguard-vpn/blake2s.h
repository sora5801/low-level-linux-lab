/* ===========================================================================
 * blake2s.h — BLAKE2s hash, plus the HMAC and HKDF the Noise handshake needs.
 * ===========================================================================
 *
 * Noise uses a hash for two jobs:
 *   * HASH(data)      — fold a value into the running transcript hash `h`.
 *   * HKDF(ck, ikm)   — ratchet the chaining key `ck` and a fresh Diffie-Hellman
 *                       result into new symmetric keys. HKDF is built from HMAC,
 *                       and HMAC is built from the plain hash. So we expose all
 *                       three layers here.
 *
 * BLAKE2s is a 32-bit ARX hash (same family of Add/Rotate/Xor operations as
 * ChaCha). Its 64-byte block matches the HMAC block size, which is why WireGuard
 * pairs it with HMAC cleanly. We only ever need a 32-byte digest.
 * =========================================================================== */
#ifndef BLAKE2S_H
#define BLAKE2S_H

#include "wg.h"

/* Streaming hash state. `h` is the 8-word chaining value, `t` the 64-bit byte
 * counter (as two words), `f` the finalization flags, `buf` a partial block. */
struct blake2s {
    u32   h[8];
    u32   t[2];
    u32   f[2];
    u8    buf[BLAKE2S_BLOCK_LEN];
    usize buflen;
    usize outlen;
};

void blake2s_init(struct blake2s *s, usize outlen);
void blake2s_update(struct blake2s *s, const void *data, usize len);
void blake2s_final(struct blake2s *s, void *out, usize outlen);

/* One-shot 32-byte hash: HASH(data) used by Noise MixHash. */
void blake2s256(u8 out[BLAKE2S_HASH_LEN], const void *data, usize len);

/* HMAC-BLAKE2s(key, data) -> 32-byte MAC (the PRF underlying HKDF). */
void hmac_blake2s(u8 out[BLAKE2S_HASH_LEN],
                  const u8 *key, usize key_len,
                  const u8 *data, usize data_len);

/* Noise's HKDF: from a chaining key and input key material, derive up to three
 * 32-byte outputs. Pass the addresses of the outputs you want (out2/out3 may be
 * NULL if num < 2 / 3). This is exactly the WireGuard KDF_n. */
void hkdf(u8 *out1, u8 *out2, u8 *out3, int num,
          const u8 chaining_key[BLAKE2S_HASH_LEN],
          const u8 *ikm, usize ikm_len);

#endif /* BLAKE2S_H */
