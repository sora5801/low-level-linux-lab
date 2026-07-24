/* ===========================================================================
 * x25519.h — Curve25519 Diffie-Hellman (RFC 7748), the handshake's key exchange.
 * ===========================================================================
 *
 * X25519 is a Diffie-Hellman function on the Montgomery curve Curve25519. Both
 * operations we need take two 32-byte inputs and produce a 32-byte output:
 *
 *   x25519_base(pub, priv)        pub  = priv * G       (derive public key)
 *   x25519(shared, priv, peerpub) shared = priv * peerpub  (the shared secret)
 *
 * The magic property: x25519(a_priv, b_pub) == x25519(b_priv, a_pub), because
 * both equal (a*b)*G. Neither side ever transmits its private scalar, yet both
 * arrive at the same secret — the foundation of the whole tunnel.
 *
 * There are no "invalid public keys" to reject (every 32-byte string is a valid
 * u-coordinate), which is a large part of why Curve25519 is safe to use inside a
 * protocol without extra validation. The private scalar is "clamped" internally
 * (low 3 bits cleared, bit 254 set, bit 255 cleared) to dodge small-subgroup
 * and timing pitfalls; callers pass raw 32 random bytes as the private key.
 * =========================================================================== */
#ifndef X25519_H
#define X25519_H

#include "wg.h"

/* shared = scalar * point (the Montgomery ladder). All buffers are 32 bytes.
 * `out` may alias neither input. */
void x25519(u8 out[X25519_KEY_LEN],
            const u8 scalar[X25519_KEY_LEN],
            const u8 point[X25519_KEY_LEN]);

/* pub = scalar * basepoint(9): derive a public key from a private scalar. */
void x25519_base(u8 out[X25519_KEY_LEN], const u8 scalar[X25519_KEY_LEN]);

#endif /* X25519_H */
