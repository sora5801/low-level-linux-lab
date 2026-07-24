/* ===========================================================================
 * net.c — the in-process simulated network (with a partition matrix).
 * ===========================================================================
 *
 * Raft's correctness argument assumes an ADVERSARIAL asynchronous network: it
 * may delay, reorder, duplicate, or DROP any message, and it may split the
 * cluster into groups that cannot talk (a "partition"). To *test* an algorithm
 * against that model you need a network you can abuse on demand. We give each
 * node an in-memory inbox and route messages through this file, where a single
 * boolean matrix `reachable[from][to]` decides delivery. Injecting a partition
 * is then one function call, and it is deterministic and instantaneous — far
 * easier to reason about (and to teach from) than iptables rules against real
 * TCP. The Raft authors tested their reference implementation exactly this way.
 *
 * The transport is intentionally the *only* thing standing between this and a
 * real socket cluster: net_send is where you would marshal the `struct message`
 * onto a TCP stream and where a receiving thread would unmarshal it into an
 * inbox. Everything in raft.c is transport-agnostic. See the README's "Going
 * further" for the swap.
 *
 * CONCURRENCY / LOCK ORDER (why this can't deadlock). Two locks appear here:
 *   - c->net_mu guards the partition matrix and the loss PRNG. It is a LEAF
 *     lock: taken alone, released before any node lock is acquired.
 *   - d->mu guards the destination node's inbox.
 * net_send takes net_mu, reads the matrix, RELEASES net_mu, then (only if the
 * message survives) takes the destination's d->mu to enqueue. A node thread
 * calls net_send while holding NO node lock of its own (it flushed its out-queue
 * after unlocking — see raft.c). So no thread ever holds two node locks, and
 * net_mu is never held while a node lock is taken. No lock cycle can form.
 * =========================================================================== */

#include "raft.h"
#include <stdlib.h>   /* calloc, free                                            */
#include <string.h>   /* memset                                                  */

/* xorshift32 — a tiny, fast pseudo-random generator for loss decisions. Not for
 * anything that needs statistical quality; it just has to be cheap and produce a
 * spread of values. State must be non-zero (xorshift can never leave the zero
 * state), which the cluster initializer guarantees. */
static uint32_t xorshift32(uint32_t *s)
{
    uint32_t x = *s;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return *s = x;
}

/* ---------------------------------------------------------------------------
 * msg_alloc — allocate a zeroed message envelope. calloc zeroes the whole union,
 * so any field the caller does not set reads as 0/false, which is the correct
 * default for every RPC field (a zero term, no entries, not granted, etc.).
 * OWNERSHIP: the returned pointer belongs to the caller until it is handed to
 * net_send, which then owns it (delivering or freeing it). Returns NULL on OOM;
 * callers treat a failed send as a dropped message, which Raft already tolerates.
 * --------------------------------------------------------------------------- */
struct message *msg_alloc(int from, int to, enum msg_type type)
{
    struct message *m = (struct message *)calloc(1, sizeof(struct message));
    if (!m) return NULL;
    m->from = from;
    m->to   = to;
    m->type = type;
    m->next = NULL;
    return m;
}

/* ---------------------------------------------------------------------------
 * net_send — deliver (or lose) one message, then take ownership of it either
 * way. A dropped message is simply freed: from Raft's perspective a drop is
 * indistinguishable from an arbitrarily long delay, and every RPC is retried by
 * the sender's timers (candidates re-request votes, leaders re-send heartbeats),
 * so losing a message can never break safety — only slow progress.
 * --------------------------------------------------------------------------- */
void net_send(struct cluster *c, struct message *m)
{
    if (!m) return;                              /* OOM at msg_alloc: nothing to do*/
    int from = m->from, to = m->to;

    /* --- Reachability + random loss decision, under the network lock. --- */
    bool deliver = true;
    pthread_mutex_lock(&c->net_mu);
    if (from < 0 || from >= c->n || to < 0 || to >= c->n) {
        deliver = false;                         /* bogus endpoint: drop           */
    } else if (!c->reachable[from][to]) {
        deliver = false;                         /* partitioned apart: drop        */
    } else if (c->drop_permille > 0) {
        /* Chaos knob: drop this message with probability drop_permille/1000. */
        if (xorshift32(&c->net_rng) % 1000u < c->drop_permille)
            deliver = false;
    }
    pthread_mutex_unlock(&c->net_mu);            /* release BEFORE any node lock   */

    if (!deliver) { free(m); return; }           /* lost in the "network"          */

    /* --- Enqueue into the destination's inbox, under ITS lock. --- */
    struct raft_node *d = c->nodes[to];
    if (!d) { free(m); return; }                 /* node never allocated: drop     */
    pthread_mutex_lock(&d->mu);
    /* A crashed (stopped) or torn-down node has no one draining its inbox;
     * dropping models the packet arriving at a dead machine. The sender will
     * retry once the node restarts and starts answering again. */
    if (d->stopped || !d->running) {
        pthread_mutex_unlock(&d->mu);
        free(m);
        return;
    }
    m->next = NULL;
    if (d->inbox_tail) d->inbox_tail->next = m;  /* append to FIFO tail            */
    else               d->inbox_head = m;        /* ... or start an empty list     */
    d->inbox_tail = m;
    /* Wake the node thread if it is asleep in cond_timedwait. The signal happens
     * while we hold d->mu, so the receiver's next mutex acquisition establishes a
     * happens-before edge and it is guaranteed to SEE this enqueued message (no
     * lost-wakeup: the message is in the queue before the signal is delivered). */
    pthread_cond_signal(&d->inbox_cv);
    pthread_mutex_unlock(&d->mu);
}

/* ===========================================================================
 * Fault injection — the harness's control surface over the network.
 * =========================================================================== */

/* net_partition — split the cluster into "inside `group`" vs "everyone else."
 * After this call, reachability is exactly "same side of the cut": two nodes can
 * exchange messages iff they are both in `group` or both outside it. This models
 * a clean network partition. Membership is looked up per node id; ids not in the
 * cluster are ignored. Immediate because reachability is consulted at send time. */
void net_partition(struct cluster *c, const int *group, int gsize)
{
    bool in_group[RAFT_MAX_NODES];
    memset(in_group, 0, sizeof(in_group));
    for (int i = 0; i < gsize; i++)
        if (group[i] >= 0 && group[i] < c->n)
            in_group[group[i]] = true;

    pthread_mutex_lock(&c->net_mu);
    for (int a = 0; a < c->n; a++)
        for (int b = 0; b < c->n; b++)
            /* deliverable iff a and b are on the same side of the partition */
            c->reachable[a][b] = (in_group[a] == in_group[b]);
    pthread_mutex_unlock(&c->net_mu);
}

/* net_heal — restore the full mesh: every node can reach every node again. Any
 * messages dropped during the partition are gone for good, but the nodes' timers
 * will re-drive elections/replication and the logs reconcile. */
void net_heal(struct cluster *c)
{
    pthread_mutex_lock(&c->net_mu);
    for (int a = 0; a < c->n; a++)
        for (int b = 0; b < c->n; b++)
            c->reachable[a][b] = true;
    pthread_mutex_unlock(&c->net_mu);
}

/* net_set_loss — set a uniform random drop probability (parts per thousand) on
 * ALL links, for stress-testing that Raft still makes progress under lossy
 * conditions. 0 disables it. */
void net_set_loss(struct cluster *c, unsigned permille)
{
    pthread_mutex_lock(&c->net_mu);
    c->drop_permille = permille > 1000 ? 1000 : permille;
    pthread_mutex_unlock(&c->net_mu);
}
