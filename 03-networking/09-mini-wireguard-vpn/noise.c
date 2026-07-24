/* ===========================================================================
 * noise.c — the Noise_IK handshake, step by step.
 * ===========================================================================
 *
 * Read noise.h first for the token overview. This file implements the Noise
 * SymmetricState operations (MixHash, MixKey, EncryptAndHash, DecryptAndHash)
 * and then drives them through the IK message sequence. Every DH result is mixed
 * into the chaining key; every wire byte is mixed into the transcript hash; the
 * final chaining key is split into the two transport keys.
 *
 * The protocol name below fixes the exact cipher suite. It is 33 bytes (> 32),
 * so per the Noise spec the initial chaining key is HASH(name) rather than the
 * zero-padded name. Both peers therefore start from an identical ck and h.
 * =========================================================================== */

#include "noise.h"
#include "x25519.h"
#include "blake2s.h"
#include "chacha20poly1305.h"
#include <string.h>   /* memcpy — a real TU may use system headers (see README)   */

/* The suite identifier. Any change here (different DH/AEAD/hash) yields a
 * different ck/h and cannot interoperate — which is the point of naming it. */
static const char NOISE_NAME[] = "Noise_IK_25519_ChaChaPoly_BLAKE2s";

/* ---------------------------------------------------------------------------
 * SymmetricState primitives.
 * --------------------------------------------------------------------------- */

/* MixHash(data): h = HASH(h ‖ data). Folds every handshake byte into a single
 * running digest so the two sides can detect any transcript disagreement — the
 * final AEAD tags only verify if both computed the same h all the way through. */
static void mix_hash(struct noise_handshake *hs, const u8 *data, usize len)
{
    struct blake2s b;
    blake2s_init(&b, BLAKE2S_HASH_LEN);
    blake2s_update(&b, hs->hash, BLAKE2S_HASH_LEN);
    blake2s_update(&b, data, len);
    blake2s_final(&b, hs->hash, BLAKE2S_HASH_LEN);
}

/* MixKey(ikm): ratchet the chaining key and derive a fresh AEAD key from a new
 * DH secret. HKDF(ck, ikm, 2) -> (new ck, new k). The nonce resets to 0 because
 * `k` is brand new. After this, EncryptAndHash/DecryptAndHash are keyed. */
static void mix_key(struct noise_handshake *hs, const u8 ikm[X25519_KEY_LEN])
{
    u8 new_ck[BLAKE2S_HASH_LEN], new_k[BLAKE2S_HASH_LEN];
    hkdf(new_ck, new_k, NULL, 2, hs->chaining_key, ikm, X25519_KEY_LEN);
    memcpy(hs->chaining_key, new_ck, BLAKE2S_HASH_LEN);
    memcpy(hs->key, new_k, AEAD_KEY_LEN);
    hs->has_key = 1;
    hs->nonce = 0;
    secure_zero(new_ck, sizeof new_ck);
    secure_zero(new_k, sizeof new_k);
}

/* Encode the CipherState nonce n as a 12-byte AEAD nonce: 4 zero bytes then the
 * 64-bit little-endian n (the Noise convention for ChaChaPoly). */
static void nonce_bytes(u8 out[AEAD_NONCE_LEN], u64 n)
{
    out[0] = out[1] = out[2] = out[3] = 0;
    store_le64(out + 4, n);
}

/* EncryptAndHash(plaintext): if we have a key, AEAD-seal the plaintext with the
 * transcript hash `h` as associated data, then MixHash the resulting ciphertext.
 * Writes plaintext_len (+ tag if keyed) bytes to `out`; returns bytes written. */
static usize encrypt_and_hash(struct noise_handshake *hs, u8 *out,
                              const u8 *plaintext, usize len)
{
    if (hs->has_key) {
        u8 nonce[AEAD_NONCE_LEN];
        nonce_bytes(nonce, hs->nonce);
        aead_seal(out, hs->key, nonce, hs->hash, BLAKE2S_HASH_LEN, plaintext, len);
        hs->nonce++;                         /* never reuse a (key,nonce) pair    */
        mix_hash(hs, out, len + AEAD_TAG_LEN);
        return len + AEAD_TAG_LEN;
    }
    /* No key yet (only the very first `e`): send in clear but still hash it. */
    if (len) memcpy(out, plaintext, len);
    mix_hash(hs, out, len);
    return len;
}

/* DecryptAndHash(ciphertext): AEAD-open with `h` as AAD, then MixHash the RAW
 * ciphertext (so the transcript matches the sender's, who hashed ciphertext).
 * Returns 0 and writes the plaintext to `out`, or -1 if the tag fails. */
static int decrypt_and_hash(struct noise_handshake *hs, u8 *out,
                            const u8 *ciphertext, usize clen)
{
    if (hs->has_key) {
        u8 nonce[AEAD_NONCE_LEN];
        nonce_bytes(nonce, hs->nonce);
        if (aead_open(out, hs->key, nonce, hs->hash, BLAKE2S_HASH_LEN,
                      ciphertext, clen) != 0)
            return -1;                       /* authentication failure -> drop    */
        hs->nonce++;
        mix_hash(hs, ciphertext, clen);
        return 0;
    }
    if (clen) memcpy(out, ciphertext, clen);
    mix_hash(hs, ciphertext, clen);
    return 0;
}

/* Diffie-Hellman shorthand: shared = DH(our private, their public). */
static void dh(u8 out[X25519_KEY_LEN], const u8 priv[X25519_KEY_LEN],
               const u8 pub[X25519_KEY_LEN])
{
    x25519(out, priv, pub);
}

/* ---------------------------------------------------------------------------
 * Handshake setup: InitializeSymmetric + MixHash(prologue) + pre-message.
 * --------------------------------------------------------------------------- */
void noise_handshake_init(struct noise_handshake *hs,
                          const struct noise_static *s,
                          const u8 remote_static[X25519_KEY_LEN],
                          int initiator, u32 local_index)
{
    memset(hs, 0, sizeof *hs);
    hs->s = s;
    hs->initiator = initiator;
    hs->local_index = local_index;

    /* InitializeSymmetric: ck = h = HASH(protocol_name) (name > 32 bytes). */
    blake2s256(hs->chaining_key, NOISE_NAME, sizeof NOISE_NAME - 1);
    memcpy(hs->hash, hs->chaining_key, BLAKE2S_HASH_LEN);

    /* MixHash(prologue): our prologue is empty, so this is a no-op we skip. */

    /* Pre-message `<- s`: BOTH sides fold the RESPONDER's static public key into
     * the transcript before message 1. For the initiator that is the known peer
     * key; for the responder it is its own public key. This is what lets the
     * initiator's first encryption already depend on the responder's identity. */
    const u8 *responder_static = initiator ? remote_static : s->public_key;
    mix_hash(hs, responder_static, X25519_KEY_LEN);

    if (initiator && remote_static)
        memcpy(hs->remote_static, remote_static, X25519_KEY_LEN);
}

/* ---------------------------------------------------------------------------
 * Message 1 (initiator): tokens  e, es, s, ss  + encrypted timestamp payload.
 * --------------------------------------------------------------------------- */
void noise_create_initiation(struct noise_handshake *hs, u8 out[INIT_MSG_LEN],
                             const u8 timestamp[TIMESTAMP_LEN])
{
    /* token e: fresh ephemeral keypair. Its secrecy gives forward secrecy; if it
     * leaks, only THIS session is exposed. */
    rng_bytes(hs->ephemeral_private, X25519_KEY_LEN);
    x25519_base(hs->ephemeral_public, hs->ephemeral_private);
    mix_hash(hs, hs->ephemeral_public, X25519_KEY_LEN);   /* e is sent in clear   */

    /* token es: DH(our ephemeral, their static). First shared secret; keys the
     * encryption of our static key below. */
    u8 secret[X25519_KEY_LEN];
    dh(secret, hs->ephemeral_private, hs->remote_static);
    mix_key(hs, secret);

    /* token s: send our static public key ENCRYPTED (identity hiding). */
    u8 enc_static[X25519_KEY_LEN + AEAD_TAG_LEN];
    encrypt_and_hash(hs, enc_static, hs->s->public_key, X25519_KEY_LEN);

    /* token ss: DH(our static, their static). Mutual-auth secret; only the two
     * real endpoints can compute it. */
    dh(secret, hs->s->private_key, hs->remote_static);
    mix_key(hs, secret);

    /* payload: the timestamp, encrypted. The responder uses it to reject replays
     * of this whole initiation message (see vpn.c greatest-timestamp check). */
    u8 enc_ts[TIMESTAMP_LEN + AEAD_TAG_LEN];
    encrypt_and_hash(hs, enc_ts, timestamp, TIMESTAMP_LEN);

    /* Assemble the wire message (header fields are framing, NOT hashed). */
    store_le32(out + INIT_OFF_TYPE, MSG_HANDSHAKE_INIT);
    store_le32(out + INIT_OFF_SENDER, hs->local_index);
    memcpy(out + INIT_OFF_EPHEMERAL, hs->ephemeral_public, X25519_KEY_LEN);
    memcpy(out + INIT_OFF_STATIC, enc_static, sizeof enc_static);
    memcpy(out + INIT_OFF_TIMESTAMP, enc_ts, sizeof enc_ts);

    secure_zero(secret, sizeof secret);
}

int noise_consume_initiation(struct noise_handshake *hs, const u8 in[INIT_MSG_LEN],
                             u8 out_timestamp[TIMESTAMP_LEN])
{
    /* Framing: learn the initiator's session id (we echo it in the response). */
    hs->remote_index = load_le32(in + INIT_OFF_SENDER);

    /* token e: read their ephemeral and fold it into the transcript. */
    memcpy(hs->remote_ephemeral, in + INIT_OFF_EPHEMERAL, X25519_KEY_LEN);
    mix_hash(hs, hs->remote_ephemeral, X25519_KEY_LEN);

    /* token es (responder view): DH(our static, their ephemeral). */
    u8 secret[X25519_KEY_LEN];
    dh(secret, hs->s->private_key, hs->remote_ephemeral);
    mix_key(hs, secret);

    /* token s: decrypt the initiator's static public key. Failure here means the
     * message was not built by someone holding a matching es secret. */
    u8 remote_static[X25519_KEY_LEN];
    if (decrypt_and_hash(hs, remote_static,
                         in + INIT_OFF_STATIC, X25519_KEY_LEN + AEAD_TAG_LEN) != 0) {
        secure_zero(secret, sizeof secret);
        return -1;
    }
    memcpy(hs->remote_static, remote_static, X25519_KEY_LEN);

    /* token ss: DH(our static, their static). Now keys depend on the initiator's
     * long-term identity, so the timestamp tag below authenticates the peer. */
    dh(secret, hs->s->private_key, hs->remote_static);
    mix_key(hs, secret);

    /* payload: decrypt+verify the timestamp. A good tag proves the initiator
     * really holds the static private key it just claimed. */
    if (decrypt_and_hash(hs, out_timestamp,
                         in + INIT_OFF_TIMESTAMP, TIMESTAMP_LEN + AEAD_TAG_LEN) != 0) {
        secure_zero(secret, sizeof secret);
        return -1;
    }
    secure_zero(secret, sizeof secret);
    return 0;
}

/* ---------------------------------------------------------------------------
 * Message 2 (responder): tokens  e, ee, se  + empty encrypted payload.
 * --------------------------------------------------------------------------- */
void noise_create_response(struct noise_handshake *hs, u8 out[RESP_MSG_LEN])
{
    /* token e: responder's fresh ephemeral. */
    rng_bytes(hs->ephemeral_private, X25519_KEY_LEN);
    x25519_base(hs->ephemeral_public, hs->ephemeral_private);
    mix_hash(hs, hs->ephemeral_public, X25519_KEY_LEN);

    /* token ee: DH(our ephemeral, their ephemeral). Both ephemerals now in the
     * mix -> full forward secrecy for the session keys. */
    u8 secret[X25519_KEY_LEN];
    dh(secret, hs->ephemeral_private, hs->remote_ephemeral);
    mix_key(hs, secret);

    /* token se (responder view): DH(our ephemeral, their static). */
    dh(secret, hs->ephemeral_private, hs->remote_static);
    mix_key(hs, secret);

    /* payload: empty. EncryptAndHash of nothing still emits a 16-byte tag that
     * the initiator verifies, confirming the responder derived identical keys. */
    u8 enc_empty[AEAD_TAG_LEN];
    encrypt_and_hash(hs, enc_empty, NULL, 0);

    store_le32(out + RESP_OFF_TYPE, MSG_HANDSHAKE_RESP);
    store_le32(out + RESP_OFF_SENDER, hs->local_index);
    store_le32(out + RESP_OFF_RECEIVER, hs->remote_index);
    memcpy(out + RESP_OFF_EPHEMERAL, hs->ephemeral_public, X25519_KEY_LEN);
    memcpy(out + RESP_OFF_EMPTY, enc_empty, sizeof enc_empty);

    secure_zero(secret, sizeof secret);
}

int noise_consume_response(struct noise_handshake *hs, const u8 in[RESP_MSG_LEN])
{
    hs->remote_index = load_le32(in + RESP_OFF_SENDER);
    /* (in + RESP_OFF_RECEIVER echoes our local_index; the caller matches it to
     *  the right handshake before getting here.) */

    /* token e: responder's ephemeral. */
    memcpy(hs->remote_ephemeral, in + RESP_OFF_EPHEMERAL, X25519_KEY_LEN);
    mix_hash(hs, hs->remote_ephemeral, X25519_KEY_LEN);

    /* token ee (initiator view): DH(our ephemeral, their ephemeral). */
    u8 secret[X25519_KEY_LEN];
    dh(secret, hs->ephemeral_private, hs->remote_ephemeral);
    mix_key(hs, secret);

    /* token se (initiator view): DH(our static, their ephemeral). */
    dh(secret, hs->s->private_key, hs->remote_ephemeral);
    mix_key(hs, secret);

    /* payload: verify the empty box. A good tag proves the responder reached the
     * same ck/h — i.e. the handshake succeeded and the session keys agree. */
    u8 empty[1];
    int rc = decrypt_and_hash(hs, empty, in + RESP_OFF_EMPTY, AEAD_TAG_LEN);
    secure_zero(secret, sizeof secret);
    return rc;
}

/* ---------------------------------------------------------------------------
 * Split(): the final chaining key becomes the two transport keys.
 * --------------------------------------------------------------------------- */
void noise_begin_session(const struct noise_handshake *hs, struct noise_session *out)
{
    u8 t1[BLAKE2S_HASH_LEN], t2[BLAKE2S_HASH_LEN];
    /* HKDF(ck, empty, 2): the two outputs are the directional keys. Whichever
     * side is the initiator uses t1 to send / t2 to receive; the responder is
     * the mirror. Both sides therefore agree on which key protects which flow. */
    hkdf(t1, t2, NULL, 2, hs->chaining_key, (const u8 *)"", 0);

    out->established = 1;
    out->local_index = hs->local_index;
    out->remote_index = hs->remote_index;
    if (hs->initiator) {
        memcpy(out->send_key, t1, AEAD_KEY_LEN);
        memcpy(out->recv_key, t2, AEAD_KEY_LEN);
    } else {
        memcpy(out->send_key, t2, AEAD_KEY_LEN);
        memcpy(out->recv_key, t1, AEAD_KEY_LEN);
    }
    out->send_counter = 0;
    replay_init(&out->replay);

    secure_zero(t1, sizeof t1);
    secure_zero(t2, sizeof t2);
}

/* ---------------------------------------------------------------------------
 * Transport data: an AEAD box per packet, nonce = the send counter.
 * --------------------------------------------------------------------------- */
u64 noise_transport_encrypt(struct noise_session *s, u8 *out,
                            const u8 *plaintext, usize plaintext_len)
{
    u64 counter = s->send_counter++;      /* consume this counter, never reuse it */
    u8 nonce[AEAD_NONCE_LEN];
    nonce_bytes(nonce, counter);
    /* AAD is empty for transport (the counter is carried in the plaintext-free
     * header and is implicitly authenticated by being the nonce). */
    aead_seal(out, s->send_key, nonce, NULL, 0, plaintext, plaintext_len);
    return counter;
}

long noise_transport_decrypt(struct noise_session *s, u64 counter, u8 *out,
                             const u8 *ciphertext, usize ciphertext_len)
{
    if (ciphertext_len < AEAD_TAG_LEN) return -1;
    u8 nonce[AEAD_NONCE_LEN];
    nonce_bytes(nonce, counter);
    /* 1. Authenticate+decrypt under the claimed counter. A forged counter makes
     *    the nonce wrong, so the tag fails and we reject here. */
    if (aead_open(out, s->recv_key, nonce, NULL, 0, ciphertext, ciphertext_len) != 0)
        return -1;
    /* 2. ONLY after the tag verifies, consult the replay window. Doing it in this
     *    order stops an attacker from poisoning the window with junk counters. */
    if (replay_check(&s->replay, counter) != 0)
        return -1;
    return (long)(ciphertext_len - AEAD_TAG_LEN);
}
