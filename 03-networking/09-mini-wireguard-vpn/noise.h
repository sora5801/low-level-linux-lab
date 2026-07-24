/* ===========================================================================
 * noise.h — the Noise_IK handshake state machine and transport keys.
 * ===========================================================================
 *
 * This is where the primitives (x25519, blake2s, chacha20poly1305) become a
 * PROTOCOL. Noise describes a handshake as a sequence of "tokens" run against a
 * SymmetricState (a chaining key `ck` + transcript hash `h` + a CipherState).
 * The IK pattern is exactly two messages:
 *
 *     initiator  -> responder :  e, es, s, ss    (handshake initiation)
 *     responder  -> initiator :  e, ee, se       (handshake response)
 *   pre-message  <- s                            (initiator already knows rs)
 *
 * Token meanings while WRITING/READING a message:
 *   e   : generate/emit (or read) an ephemeral public key; MixHash it.
 *   s   : encrypt-and-send (or read-and-decrypt) our static public key.
 *   ee  : MixKey( DH(our ephemeral, their ephemeral) )   — forward secrecy
 *   es  : MixKey( DH(ephemeral, static) across the pair ) — binds identities
 *   se  : MixKey( DH(static, ephemeral) across the pair )
 *   ss  : MixKey( DH(our static, their static) )          — mutual auth
 *
 * After the two messages, Split() derives the two directional transport keys.
 * From then on each data packet is an AEAD box under a per-packet counter, with
 * the receiver running the anti-replay window.
 *
 * WHAT IK BUYS US
 *   * Mutual authentication: both sides prove ownership of their static key.
 *   * Forward secrecy: the ephemerals mean compromising a static key later does
 *     not decrypt past traffic.
 *   * Initiator identity hiding: the initiator's static key is sent ENCRYPTED
 *     (token s in message 1), so a passive eavesdropper cannot see who dialed.
 * =========================================================================== */
#ifndef NOISE_H
#define NOISE_H

#include "wg.h"
#include "replay.h"

/* Our long-term ("static") identity: a Curve25519 keypair loaded at startup. */
struct noise_static {
    u8 private_key[X25519_KEY_LEN];
    u8 public_key[X25519_KEY_LEN];
};

/* The evolving handshake state for ONE in-flight handshake. It embeds the Noise
 * SymmetricState (ck, h) and CipherState (key, has_key, nonce). */
struct noise_handshake {
    u8  chaining_key[BLAKE2S_HASH_LEN];   /* ck: ratcheted by every MixKey       */
    u8  hash[BLAKE2S_HASH_LEN];           /* h: running transcript hash          */
    u8  key[AEAD_KEY_LEN];                /* k: current handshake AEAD key       */
    int has_key;                          /* is `key` set yet? (no MixKey => no)  */
    u64 nonce;                            /* n: CipherState nonce, resets on MixKey */

    u8  ephemeral_private[X25519_KEY_LEN];
    u8  ephemeral_public[X25519_KEY_LEN];
    u8  remote_ephemeral[X25519_KEY_LEN];

    const struct noise_static *s;         /* our static keypair (not owned)      */
    u8  remote_static[X25519_KEY_LEN];    /* peer static pubkey (known/learned)  */

    int initiator;
    u32 local_index;                      /* our session id (random)             */
    u32 remote_index;                     /* peer's session id                   */
};

/* An established data channel after Split(): two keys (one per direction), a
 * send counter, and the receive-side replay window. */
struct noise_session {
    int established;
    u32 local_index;
    u32 remote_index;
    u8  send_key[AEAD_KEY_LEN];
    u8  recv_key[AEAD_KEY_LEN];
    u64 send_counter;                     /* next nonce we will use to send       */
    struct replay_window replay;          /* guards the recv direction            */
};

/* Initialize a handshake. `remote_static` is the peer's static public key: it is
 * KNOWN for the initiator (IK's premise) and may be NULL for the responder,
 * which learns it from message 1. `local_index` is our random session id. */
void noise_handshake_init(struct noise_handshake *hs,
                          const struct noise_static *s,
                          const u8 remote_static[X25519_KEY_LEN],
                          int initiator, u32 local_index);

/* Message 1. Initiator writes INIT_MSG_LEN bytes into `out`; `timestamp` is the
 * 12-byte TAI64N handshake timestamp (replay-guards the handshake itself). */
void noise_create_initiation(struct noise_handshake *hs, u8 out[INIT_MSG_LEN],
                             const u8 timestamp[TIMESTAMP_LEN]);

/* Responder consumes message 1. On success returns 0, fills `out_timestamp`, and
 * records the initiator's static key + session id in `hs`. Returns -1 if any
 * AEAD tag fails (wrong keys, tampering) — the packet is then dropped silently. */
int noise_consume_initiation(struct noise_handshake *hs, const u8 in[INIT_MSG_LEN],
                             u8 out_timestamp[TIMESTAMP_LEN]);

/* Message 2. Responder writes RESP_MSG_LEN bytes into `out`. */
void noise_create_response(struct noise_handshake *hs, u8 out[RESP_MSG_LEN]);

/* Initiator consumes message 2. Returns 0 on success, -1 on tag failure. */
int noise_consume_response(struct noise_handshake *hs, const u8 in[RESP_MSG_LEN]);

/* Split(): derive the two transport keys and hand back a ready session. Call on
 * BOTH sides once their side of the handshake has completed. */
void noise_begin_session(const struct noise_handshake *hs, struct noise_session *out);

/* Transport data. Encrypt returns the 64-bit counter used (write it into the
 * data header); `out` receives plaintext_len + AEAD_TAG_LEN bytes. */
u64 noise_transport_encrypt(struct noise_session *s, u8 *out,
                            const u8 *plaintext, usize plaintext_len);

/* Decrypt a transport packet: verify the tag under `counter`, then run replay
 * protection. Returns the plaintext length (>=0) on success, or -1 on a bad tag
 * or a replayed/stale counter. */
long noise_transport_decrypt(struct noise_session *s, u64 counter, u8 *out,
                             const u8 *ciphertext, usize ciphertext_len);

#endif /* NOISE_H */
