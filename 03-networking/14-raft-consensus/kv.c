/* ===========================================================================
 * kv.c — the replicated state machine: a tiny key/value store.
 * ===========================================================================
 *
 * Raft replicates an ordered log of OPAQUE commands; it does not care what they
 * mean. The state machine is the component that gives them meaning by APPLYING
 * them in log order. The cast-iron rule Raft provides is: every node applies the
 * same committed commands in the same order, so every node's state machine ends
 * in the same state. This file is that state machine — deliberately trivial so
 * the interesting code stays in raft.c.
 *
 * DETERMINISM IS MANDATORY. A state machine fed to Raft must be a pure function
 * of its command sequence: same commands in, same state out, on every node,
 * forever. No clocks, no randomness, no reading the local filesystem — anything
 * that could differ between nodes would make the replicas diverge even though
 * their logs agree. "SET"/"DEL" over fixed-size strings is about as pure as it
 * gets, which is exactly why we chose it.
 *
 * Commands are ASCII for legibility in the harness:
 *     SET <key> <value>
 *     DEL <key>
 * Keys/values are bounded (KV_KEYLEN / KV_VALLEN). The store is a small open
 * array probed linearly — O(n) per op, but n <= KV_CAPACITY and clarity beats
 * speed here. Snapshotting (log compaction) serializes this whole store to a
 * byte buffer and restores it, which is why kv_serialize / kv_deserialize exist.
 * =========================================================================== */

#include "raft.h"
#include <string.h>   /* memcpy, memset, strncmp, memcmp                         */
#include <stdio.h>    /* snprintf for serialization framing                      */

/* Reset the store to empty. Called when a follower installs a snapshot that
 * starts from a *different* base than its current state — the snapshot is the
 * ground truth, so we wipe first, then load it. */
void kv_reset(struct kv_store *kv)
{
    memset(kv, 0, sizeof(*kv));
}

/* Find the slot holding `key`, or -1. Linear probe: `used` marks live slots.
 * We compare with memcmp over the FULL fixed key field (not strcmp) because keys
 * are stored zero-padded, so two keys are equal iff all KV_KEYLEN bytes match. */
static int kv_find(struct kv_store *kv, const char *key)
{
    char k[KV_KEYLEN];
    memset(k, 0, sizeof(k));
    /* Copy at most KV_KEYLEN-1 bytes so there is always a terminating zero and
     * over-long keys can never run off the field. */
    for (int i = 0; i < KV_KEYLEN - 1 && key[i]; i++)
        k[i] = key[i];
    for (int i = 0; i < KV_CAPACITY; i++)
        if (kv->pairs[i].used && memcmp(kv->pairs[i].key, k, KV_KEYLEN) == 0)
            return i;
    return -1;
}

/* SET: overwrite an existing key or claim the first free slot. Returns false
 * only if the store is full — a real system would grow; the teaching core caps
 * capacity so the snapshot stays a fixed, comprehensible size. */
static bool kv_set(struct kv_store *kv, const char *key, const char *val)
{
    int slot = kv_find(kv, key);
    if (slot < 0) {
        for (int i = 0; i < KV_CAPACITY; i++)
            if (!kv->pairs[i].used) { slot = i; break; }
        if (slot < 0) return false;          /* store full                       */
        memset(&kv->pairs[slot], 0, sizeof(kv->pairs[slot]));
        kv->pairs[slot].used = true;
        for (int i = 0; i < KV_KEYLEN - 1 && key[i]; i++)
            kv->pairs[slot].key[i] = key[i];
    }
    memset(kv->pairs[slot].val, 0, KV_VALLEN);
    for (int i = 0; i < KV_VALLEN - 1 && val[i]; i++)
        kv->pairs[slot].val[i] = val[i];
    return true;
}

/* DEL: free the slot if present (idempotent — deleting a missing key is fine,
 * which matters because the same DEL may be re-applied after a snapshot/replay). */
static void kv_del(struct kv_store *kv, const char *key)
{
    int slot = kv_find(kv, key);
    if (slot >= 0)
        kv->pairs[slot].used = false;
}

/* Read a key's value into `out`. Returns false if absent. This is a QUERY, not
 * a command: it is never logged or replicated. In real Raft even reads must go
 * through the leader (with a lease or a no-op barrier) to be linearizable; the
 * harness reads a node's local store directly, which is fine for verifying that
 * replicas converged. */
bool kv_get(struct kv_store *kv, const char *key, char *out, size_t outlen)
{
    int slot = kv_find(kv, key);
    if (slot < 0) return false;
    size_t i = 0;
    for (; i < outlen - 1 && i < KV_VALLEN && kv->pairs[slot].val[i]; i++)
        out[i] = kv->pairs[slot].val[i];
    out[i] = '\0';
    return true;
}

/* ---------------------------------------------------------------------------
 * kv_apply — THE state machine transition. Parse one committed command and
 * mutate the store. Called by raft.c exactly once per entry, in strictly
 * increasing commit order, on every node. Unknown commands are ignored rather
 * than fatal: a corrupt/foreign command must not crash a replica (and must be
 * treated identically on every node to stay deterministic — ignoring is the
 * simplest identical behavior).
 *
 * `cmd` is NOT guaranteed NUL-terminated by the log (it is length-prefixed), so
 * we copy into a local buffer with an explicit terminator before using string
 * ops on it.
 * --------------------------------------------------------------------------- */
void kv_apply(struct kv_store *kv, const char *cmd, uint16_t cmd_len)
{
    char buf[RAFT_CMD_MAX];
    uint16_t n = cmd_len < RAFT_CMD_MAX - 1 ? cmd_len : RAFT_CMD_MAX - 1;
    memcpy(buf, cmd, n);
    buf[n] = '\0';

    /* Split into up to three whitespace-separated tokens: op, key, value. */
    char op[8] = {0}, key[KV_KEYLEN] = {0}, val[KV_VALLEN] = {0};
    /* sscanf with width limits so a hostile command can never overflow the
     * fixed fields; %7s/%23s reserve room for the terminating NUL. */
    int fields = sscanf(buf, "%7s %23s %23s", op, key, val);

    if (fields >= 2 && strncmp(op, "SET", 4) == 0 && fields >= 3)
        (void)kv_set(kv, key, val);
    else if (fields >= 2 && strncmp(op, "DEL", 4) == 0)
        kv_del(kv, key);
    /* else: unknown/short command -> no-op, identically on every node. */
}

/* ---------------------------------------------------------------------------
 * Serialization for snapshots. Format is one "key=val\n" line per live pair:
 * human-readable, self-delimiting, and trivially deterministic since we iterate
 * slots in fixed order. Returns the number of bytes written (<= cap). The caller
 * (snapshot code in raft.c) ships this buffer verbatim in InstallSnapshot and
 * writes it to the on-disk `snapshot` file.
 * --------------------------------------------------------------------------- */
size_t kv_serialize(const struct kv_store *kv, char *buf, size_t cap)
{
    size_t off = 0;
    for (int i = 0; i < KV_CAPACITY; i++) {
        if (!kv->pairs[i].used) continue;
        /* snprintf returns the length it WOULD have written; guard against
         * truncation so we never claim to have serialized a clipped line. */
        int m = snprintf(buf + off, cap - off, "%s=%s\n",
                         kv->pairs[i].key, kv->pairs[i].val);
        if (m < 0 || (size_t)m >= cap - off)
            break;                           /* out of room: stop cleanly        */
        off += (size_t)m;
    }
    return off;
}

/* Rebuild the store from a serialized snapshot. Wipes first (the snapshot is the
 * complete truth), then parses each "key=val" line. Bytes are exactly what
 * kv_serialize produced. */
void kv_deserialize(struct kv_store *kv, const char *buf, size_t len)
{
    kv_reset(kv);
    size_t i = 0;
    while (i < len) {
        char key[KV_KEYLEN] = {0}, val[KV_VALLEN] = {0};
        size_t k = 0, v = 0;
        while (i < len && buf[i] != '=' && buf[i] != '\n' && k < KV_KEYLEN - 1)
            key[k++] = buf[i++];
        if (i < len && buf[i] == '=') {
            i++;                             /* skip '='                         */
            while (i < len && buf[i] != '\n' && v < KV_VALLEN - 1)
                val[v++] = buf[i++];
        }
        while (i < len && buf[i] != '\n') i++;   /* consume any overflow tail    */
        if (i < len) i++;                        /* skip the '\n'                */
        if (k > 0) (void)kv_set(kv, key, val);
    }
}
