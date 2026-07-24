/* ===========================================================================
 * wg.h — shared types, constants, wire-format layout, and small helpers for the
 *        mini-WireGuard VPN. Included by every translation unit.
 * ===========================================================================
 *
 * This project is a *point-to-point* encrypted tunnel modelled on WireGuard. The
 * cryptographic core is a faithful Noise_IK handshake:
 *
 *     Noise_IK_25519_ChaChaPoly_BLAKE2s
 *      \____/ \/ \___/ \________/ \_____/
 *        |    |    |        |         |
 *        |    |    |        |         +-- BLAKE2s      : hash + HKDF + keyed MAC
 *        |    |    |        +-- ChaCha20-Poly1305      : the AEAD (RFC 8439)
 *        |    |    +-- Curve25519 (X25519)             : the DH (RFC 7748)
 *        |    +-- IK pattern: initiator Knows responder's static key in advance,
 *        |                    initiator's own static Identity is transmitted.
 *        +-- Noise Protocol Framework.
 *
 * WHY THESE PRIMITIVES (the "explain the construction" requirement)
 * ----------------------------------------------------------------
 *   * X25519 gives us a Diffie-Hellman on Curve25519 whose only input is a
 *     32-byte scalar and 32-byte u-coordinate; there are no invalid points to
 *     validate, which is what makes it safe to drop into a protocol.
 *   * ChaCha20-Poly1305 is an AEAD: one key + one nonce turns a plaintext into
 *     ciphertext + a 16-byte authentication tag that also covers "associated
 *     data". Forgery is infeasible without the key; a flipped bit fails the tag.
 *   * BLAKE2s is the hash. Noise needs a hash for (a) the running transcript hash
 *     `h`, (b) HKDF to ratchet the chaining key `ck` into fresh symmetric keys.
 *   * The IK handshake mixes FOUR DH results (ee, es, se, ss) into `ck` so the
 *     final session keys depend on both parties' ephemeral AND static keys —
 *     that is what buys forward secrecy (ephemerals) and identity binding
 *     (statics) at once.
 *
 * Everything on the wire is LITTLE-ENDIAN, matching WireGuard and every
 * primitive here (ChaCha/Poly/BLAKE2s all serialise words little-endian). We
 * still convert explicitly with load_le/store_le so the code is correct on a
 * big-endian host and so the byte layout is visible to the reader.
 *
 * Platform: Linux (TUN + UDP). See README for build/run and privileges.
 * =========================================================================== */
#ifndef WG_H
#define WG_H

#include <stdint.h>     /* uintN_t                                            */
#include <stddef.h>     /* size_t                                             */

/* --- Fixed-width shorthands used throughout ------------------------------- */
typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;
typedef size_t    usize;

/* ---------------------------------------------------------------------------
 * Cryptographic sizes. These are protocol constants, not tunables: changing any
 * of them changes the wire format and breaks interop.
 * --------------------------------------------------------------------------- */
#define X25519_KEY_LEN     32   /* both scalars and u-coordinates are 32 bytes */
#define AEAD_KEY_LEN       32   /* ChaCha20 key                                */
#define AEAD_NONCE_LEN     12   /* ChaCha20-Poly1305 nonce (96 bits)           */
#define AEAD_TAG_LEN       16   /* Poly1305 authentication tag                 */
#define BLAKE2S_HASH_LEN   32   /* our hash output and HKDF output width        */
#define BLAKE2S_BLOCK_LEN  64   /* BLAKE2s compression block (also HMAC block) */
#define TIMESTAMP_LEN      12   /* TAI64N-style handshake timestamp            */

/* ---------------------------------------------------------------------------
 * Wire message types (the first 4 bytes of every UDP datagram, little-endian).
 * We follow WireGuard's numbering; type 3 (cookie reply) is intentionally
 * omitted from this teaching core (see README "Going further").
 * --------------------------------------------------------------------------- */
#define MSG_HANDSHAKE_INIT  1u
#define MSG_HANDSHAKE_RESP  2u
#define MSG_TRANSPORT_DATA  4u

/* ---------------------------------------------------------------------------
 * Wire layout, byte-offset for byte-offset. We build and parse messages by
 * writing fixed-width fields at these offsets rather than casting a packed
 * struct over the buffer — that keeps endianness explicit and avoids unaligned
 * struct access. Every "_enc" field is ciphertext followed by a 16-byte tag.
 *
 * HANDSHAKE INITIATION  (initiator -> responder), total 116 bytes:
 *   off  size  field
 *    0     4   type = 1                     (little-endian u32)
 *    4     4   sender_index                 (initiator's random session id)
 *    8    32   unencrypted_ephemeral        (initiator ephemeral public key)
 *   40    48   encrypted_static             (32-byte static pubkey + 16 tag)
 *   88    28   encrypted_timestamp          (12-byte timestamp + 16 tag)
 *  = 116                                     (WireGuard adds mac1/mac2 = +32)
 *
 * HANDSHAKE RESPONSE  (responder -> initiator), total 60 bytes:
 *   off  size  field
 *    0     4   type = 2
 *    4     4   sender_index                 (responder's session id)
 *    8     4   receiver_index               (echoes initiator's sender_index)
 *   12    32   unencrypted_ephemeral        (responder ephemeral public key)
 *   44    16   encrypted_nothing            (empty plaintext -> just a 16 tag)
 *  = 60
 *
 * TRANSPORT DATA  (either direction), header 16 bytes + ciphertext + 16 tag:
 *   off  size  field
 *    0     4   type = 4
 *    4     4   receiver_index               (the peer's session id)
 *    8     8   counter                      (per-packet nonce, little-endian u64)
 *   16    N    encrypted_payload            (inner IP packet + 16 tag)
 * --------------------------------------------------------------------------- */
#define INIT_OFF_TYPE        0
#define INIT_OFF_SENDER      4
#define INIT_OFF_EPHEMERAL   8
#define INIT_OFF_STATIC      40   /* encrypted static: 32 + 16                  */
#define INIT_OFF_TIMESTAMP   88   /* encrypted timestamp: 12 + 16               */
#define INIT_MSG_LEN         116

#define RESP_OFF_TYPE        0
#define RESP_OFF_SENDER      4
#define RESP_OFF_RECEIVER    8
#define RESP_OFF_EPHEMERAL   12
#define RESP_OFF_EMPTY       44   /* encrypted empty: 0 + 16                    */
#define RESP_MSG_LEN         60

#define DATA_OFF_TYPE        0
#define DATA_OFF_RECEIVER    4
#define DATA_OFF_COUNTER     8
#define DATA_OFF_PAYLOAD     16
#define DATA_HEADER_LEN      16

/* ---------------------------------------------------------------------------
 * MTU arithmetic (a first-class WireGuard concern). Every inner IP packet is
 * wrapped as:  outer-IP(20) + UDP(8) + data-header(16) + payload + tag(16).
 * To never fragment on a standard 1500-byte Ethernet link:
 *
 *     inner MTU = 1500 - 20 (IPv4) - 8 (UDP) - 16 (our header) - 16 (tag) = 1440
 *
 * WireGuard picks 1420 so the same number is safe even when the OUTER packet is
 * IPv6 (40-byte header). We mirror that 1420 and set it on the TUN device, so
 * the kernel hands us inner packets that always fit one UDP datagram. See
 * README "How it works" for the full table.
 * --------------------------------------------------------------------------- */
#define TUNNEL_MTU          1420
/* Largest inner packet we ever encrypt, plus room for the transport header and
 * tag. 2048 comfortably covers a 1500-byte frame and keeps buffers page-ish. */
#define MAX_PACKET          2048

/* ===========================================================================
 * Small inline helpers (endianness, rotates, zeroing). Inline so every caller
 * sees them and the optimizer can fold them; no .c file needed.
 * =========================================================================== */

/* Load a little-endian u32/u64 from a byte buffer. Byte-wise, so it is correct
 * regardless of host endianness OR pointer alignment (WireGuard messages are
 * not aligned in our recv buffer). */
static inline u32 load_le32(const u8 *p)
{
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}
static inline u64 load_le64(const u8 *p)
{
    return (u64)load_le32(p) | ((u64)load_le32(p + 4) << 32);
}

/* Store a u32/u64 little-endian into a byte buffer (mirror of the loads). */
static inline void store_le32(u8 *p, u32 v)
{
    p[0] = (u8)(v);       p[1] = (u8)(v >> 8);
    p[2] = (u8)(v >> 16); p[3] = (u8)(v >> 24);
}
static inline void store_le64(u8 *p, u64 v)
{
    store_le32(p, (u32)v);
    store_le32(p + 4, (u32)(v >> 32));
}

/* 32-bit rotations — the "R" in the ARX (Add-Rotate-Xor) primitives ChaCha and
 * BLAKE2s are built from. Written as (x<<n)|(x>>(32-n)); the compiler pattern-
 * matches this to a single `rol`/`ror` instruction (you can SEE it in asm/). The
 * `& 31` is not needed because n is always a nonzero literal here, but the shape
 * matters: `x >> (32 - n)` with n in 1..31 never invokes the UB of a 32-shift. */
static inline u32 rotl32(u32 x, unsigned n) { return (x << n) | (x >> (32 - n)); }
static inline u32 rotr32(u32 x, unsigned n) { return (x >> n) | (x << (32 - n)); }

/* Overwrite a buffer through a volatile pointer so the compiler may NOT elide
 * the write as "dead" (a plain memset on an about-to-be-freed key often IS
 * elided, which is how secret key bytes leak into freed memory / core dumps).
 * This is the poor-man's memset_s; every secret is wiped with it. */
static inline void secure_zero(void *p, usize n)
{
    volatile u8 *v = (volatile u8 *)p;
    while (n--) *v++ = 0;
}

/* ===========================================================================
 * util.c — process-wide helpers (randomness, base64) declared here.
 * =========================================================================== */

/* Fill `buf` with `n` cryptographically-secure random bytes via getrandom(2).
 * Returns 0 on success, -1 on error. Used for ephemeral keys and session ids —
 * the whole security of the handshake rests on these bytes being unpredictable. */
int rng_bytes(void *buf, usize n);

/* Base64 (the WireGuard key encoding). encode writes a NUL-terminated string of
 * ceil(n/3)*4 chars into out (caller sizes it). decode parses exactly enough
 * base64 to fill out_len bytes, returning 0 on success or -1 on malformed input
 * / wrong length. Keys are 32 bytes -> 44 base64 chars. */
void base64_encode(char *out, const u8 *in, usize n);
int  base64_decode(u8 *out, usize out_len, const char *in);

#endif /* WG_H */
