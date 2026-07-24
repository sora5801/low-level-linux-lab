/* ===========================================================================
 * demo.c — backend selection math, extracted to pure logic for annotated asm.
 * ===========================================================================
 *
 * WHY THIS FILE EXISTS
 * --------------------
 * The real load balancer (../lb.c, ../hashring.c) pulls in <sys/epoll.h>,
 * <sys/socket.h>, <netinet/in.h>, <fcntl.h> (for splice), timerfd/signalfd —
 * a pile of Linux-only headers. It therefore cannot be lowered to standalone
 * assembly on an arbitrary host. Per the lab convention we lift out the single
 * most instructive, purely-computational part — how the proxy *chooses a
 * backend* — into this header-free file so clang can emit clean SysV asm for it.
 *
 * There are NO system headers here: we declare our own fixed-width types. The
 * three routines are exactly the ones the proxy runs on the hot path, minus the
 * socket plumbing, so the assembly you read in demo.annotated.s is the assembly
 * that actually selects a backend:
 *
 *   fnv1a_32     — hash a key (client IP, or "host:port#vnode") to 32 bits.
 *   ring_lookup  — the CONSISTENT-HASH ring: binary-search a sorted array of
 *                  (hash -> backend) virtual nodes for the first node clockwise
 *                  of the key, wrapping around the ring. This is policy "hash".
 *   least_conn_pick — the LEAST-CONNECTIONS policy: linear min-scan over the
 *                  eligible backends' in-flight connection counts.
 *
 * WHY CONSISTENT HASHING? (the idea ring_lookup implements)
 * ---------------------------------------------------------
 * A naive "hash(key) % N backends" pins each client to a backend, but changing N
 * (a backend dies, or you add one) remaps almost EVERY key: (h % 4) and (h % 5)
 * agree for very few h. In a proxy that means a health blip reshuffles every
 * client to a new backend and throws away all its warmed connection/session
 * state. Consistent hashing fixes this: place both backends and keys on one
 * fixed 2^32 ring; a key is served by the first backend hash you meet walking
 * clockwise. Remove a backend and only the keys that mapped to *its* arc move —
 * to the next backend clockwise — everyone else is undisturbed. Adding a backend
 * likewise steals only one arc's worth of keys. On average just K/N keys remap
 * when a backend joins or leaves, instead of nearly all of them.
 *
 * VIRTUAL NODES. One hash per backend would carve the ring into N wildly uneven
 * arcs (hashing is random, not fair), so load would be lopsided. Instead each
 * backend is hashed under many labels ("host:port#0", "#1", ... "#159"),
 * scattering ~160 tiny arcs per backend around the ring. The law of large
 * numbers then evens the load, and the arcs a departing backend leaves behind
 * are handed to many different survivors rather than dumped on one neighbour.
 * The ring array below holds those virtual nodes, pre-sorted by hash so a lookup
 * is an O(log V) binary search instead of an O(V) scan.
 * ===========================================================================
 */

/* Our own fixed-width types — no <stdint.h>. LP64 (Linux x86-64): int is 32-bit,
 * long is 64-bit, so u32 is `unsigned int` and a size is `unsigned long`. */
typedef unsigned int   u32;
typedef unsigned long  usize;

/* ---------------------------------------------------------------------------
 * fnv1a_32 — FNV-1a, a fast non-cryptographic hash (Fowler-Noll-Vo).
 *
 * We do NOT need cryptographic strength here, only good avalanche/dispersion so
 * that (a) client IPs scatter evenly across backends and (b) a backend's virtual
 * nodes land at well-mixed positions on the ring. FNV-1a delivers that in a
 * handful of instructions per byte: seed with the "offset basis", then for each
 * byte XOR it in and multiply by the "FNV prime". The XOR-before-multiply order
 * is what distinguishes FNV-1a from FNV-1 and gives it better low-bit mixing.
 *
 * The multiply is modulo 2^32 (it just overflows a u32), which is why the result
 * is naturally a ring coordinate in [0, 2^32). Both the proxy's ring builder and
 * its lookup hash through THIS function, so the client-IP hash and the virtual-
 * node hashes live in the same coordinate space and can be compared directly.
 * --------------------------------------------------------------------------- */
u32 fnv1a_32(const unsigned char *data, usize len)
{
    u32 h = 2166136261u;                 /* FNV offset basis (a fixed 32-bit seed) */
    for (usize i = 0; i < len; i++) {
        h ^= (u32)data[i];               /* FNV-1a: XOR the byte in FIRST ...      */
        h *= 16777619u;                  /* ... THEN multiply by the FNV prime.    */
    }                                     /* (mul wraps mod 2^32 — that IS the ring) */
    return h;
}

/* One entry on the hash ring: a virtual node. `hash` is its position in [0,2^32),
 * `idx` is the backend it belongs to. The proxy keeps an array of these SORTED by
 * ascending `hash`, which is the precondition the binary search below relies on. */
struct rnode {
    u32 hash;                            /* position of this vnode on the 2^32 ring */
    int idx;                             /* which backend this vnode routes to      */
};

/* ---------------------------------------------------------------------------
 * ring_lookup — map a key's hash to a backend via the consistent-hash ring.
 *
 * Contract: `nodes[0..n)` is sorted by ascending `.hash`. We want the first node
 * whose hash is >= `key` — i.e. the next virtual node CLOCKWISE from the key's
 * position — and if the key sits past the largest node we WRAP around to nodes[0]
 * (the ring is circular). That is a textbook lower_bound / left-most binary
 * search: maintain [lo, hi) as the still-possible range, halve it each step.
 *
 * Loop invariant: every node in [0, lo) has hash < key, and every node in
 * [hi, n) has hash >= key. When lo == hi the two facts meet at the boundary, so
 * `lo` is the first index with hash >= key. If that ran off the end (lo == n),
 * every node was < key, so the key is in the arc that wraps past the top of the
 * ring back to the smallest node: return nodes[0].
 *
 * O(log n): with ~160 vnodes per backend and a handful of backends, a lookup is
 * ~11 comparisons — cheap enough to run on every incoming connection.
 * --------------------------------------------------------------------------- */
int ring_lookup(const struct rnode *nodes, int n, u32 key)
{
    if (n <= 0)
        return -1;                       /* empty ring: no eligible backend at all  */

    int lo = 0;
    int hi = n;                          /* half-open range [lo, hi); search space   */
    while (lo < hi) {
        int mid = lo + ((hi - lo) >> 1); /* midpoint without (lo+hi) overflow        */
        if (nodes[mid].hash < key)
            lo = mid + 1;                /* mid is too small -> answer is to the right*/
        else
            hi = mid;                    /* mid could be the answer -> keep it in range*/
    }
    if (lo == n)                         /* key is past the last vnode ...            */
        lo = 0;                          /* ... so it wraps to the first (ring is round)*/
    return nodes[lo].idx;                /* the backend that owns this arc            */
}

/* ---------------------------------------------------------------------------
 * least_conn_pick — the "least connections" policy.
 *
 * Round-robin spreads connections evenly by COUNT, but if some connections are
 * long-lived (a slow download) and others are brief, equal counts can still mean
 * lopsided load. Least-connections instead sends each new client to whichever
 * eligible backend currently has the FEWEST in-flight connections, which tracks
 * real occupancy and naturally shifts load off a backend that is draining slowly.
 *
 * `eligible[i]` is a 0/1 byte: a backend is skipped when it is down or draining.
 * We scan linearly (N backends is tiny), keeping the running best. `best < 0`
 * seeds "nothing chosen yet" so the very first eligible backend always wins the
 * initial comparison; ties keep the earliest index (a stable, predictable pick).
 * Returns -1 if no backend is eligible.
 * --------------------------------------------------------------------------- */
int least_conn_pick(const int *conns, const unsigned char *eligible, int n)
{
    int best  = -1;                      /* index of the best backend so far (-1=none)*/
    int bestc = 0;                       /* its connection count (valid once best>=0) */
    for (int i = 0; i < n; i++) {
        if (!eligible[i])
            continue;                    /* down/draining: not a candidate            */
        if (best < 0 || conns[i] < bestc) {
            best  = i;                   /* first candidate, or a strictly-emptier one */
            bestc = conns[i];
        }
    }
    return best;
}
