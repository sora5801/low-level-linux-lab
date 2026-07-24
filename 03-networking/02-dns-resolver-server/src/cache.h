/* ===========================================================================
 * cache.h — a small TTL-respecting DNS record cache.
 * ===========================================================================
 *
 * Caching is what makes DNS scale: every RR carries a TTL (time-to-live, in
 * seconds) that says how long it may be reused before it must be re-fetched.
 * A resolver that ignores TTLs either hammers the roots (TTL treated as 0) or
 * serves stale data forever (TTL treated as infinite) — both are bugs.
 *
 * We store DECODED records (the self-contained fields of dns_rr: addresses,
 * target names, MX preference) rather than borrowed wire slices, and stamp each
 * with an ABSOLUTE monotonic expiry time = now + TTL. Lookups skip entries
 * whose expiry has passed. This is a fixed-size teaching cache with simple
 * eviction; a production cache would be a hash table with LRU and negative
 * caching (RFC 2308). See the README's "Going further".
 * =========================================================================== */
#ifndef CACHE_H
#define CACHE_H

#include "dns.h"

#define CACHE_CAP 256   /* number of records held before we evict             */

typedef struct {
    dns_rr   rr;        /* the record, with rr.rdata cleared (we own the data) */
    long     expiry;    /* CLOCK_MONOTONIC seconds at which rr becomes stale   */
    int      used;      /* slot occupied?                                      */
} cache_entry;

typedef struct {
    cache_entry ents[CACHE_CAP];
} dns_cache;

void dns_cache_init(dns_cache *c);

/* Insert `rr`, computing expiry from the current time + rr->ttl. A TTL of 0
 * means "do not cache" and is silently ignored. Evicts if full. */
void dns_cache_put(dns_cache *c, const dns_rr *rr);

/* Copy up to `max` live (non-expired) records matching (name, type) into out[].
 * The stored TTLs are decayed to the REMAINING seconds so the caller/downstream
 * sees an honest countdown. Returns the number copied. */
int dns_cache_get(dns_cache *c, const char *name, uint16_t type,
                  dns_rr *out, int max);

#endif /* CACHE_H */
