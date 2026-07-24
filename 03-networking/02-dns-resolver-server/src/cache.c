/* ===========================================================================
 * cache.c — TTL-respecting record cache implementation.
 * ===========================================================================
 */
#include "cache.h"

#include <string.h>       /* memcpy, memset, strcasecmp                        */
#include <strings.h>      /* strcasecmp (case-insensitive: DNS names are so)   */
#include <time.h>         /* clock_gettime(CLOCK_MONOTONIC)                    */

/* Current monotonic time in whole seconds. CLOCK_MONOTONIC never jumps
 * backward (unlike wall-clock time, which NTP or an admin can set), so an
 * expiry computed as now()+ttl stays correct across clock adjustments. */
static long mono_now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long)ts.tv_sec;
}

void dns_cache_init(dns_cache *c)
{
    memset(c, 0, sizeof(*c));   /* all slots start unused (used == 0)          */
}

/* Names in DNS are case-insensitive ("Example.COM" == "example.com"), so all
 * matching goes through a case-fold compare. */
static int name_type_match(const cache_entry *e, const char *name, uint16_t type)
{
    return e->used && e->rr.type == type && strcasecmp(e->rr.name, name) == 0;
}

/* Do two decoded records carry the same payload? Compared field by field over
 * the self-contained (non-borrowed) fields, so we never dereference rr.rdata. */
static int same_payload(const dns_rr *a, const dns_rr *b)
{
    return a->mx_pref == b->mx_pref &&
           memcmp(a->addr4, b->addr4, sizeof(a->addr4)) == 0 &&
           memcmp(a->addr6, b->addr6, sizeof(a->addr6)) == 0 &&
           strcasecmp(a->target, b->target) == 0;
}

void dns_cache_put(dns_cache *c, const dns_rr *rr)
{
    /* A zero TTL means "use once, never cache" (RFC 1035). Respect it. */
    if (rr->ttl == 0) return;

    long now    = mono_now();
    long expiry = now + (long)rr->ttl;

    /* Find a slot: prefer (a) an existing entry for the same name+type+rdata to
     * refresh in place, else (b) any free or already-expired slot, else (c) the
     * entry that expires soonest (evict the least-useful record). */
    int free_slot = -1;
    int soonest   = 0;
    long soonest_expiry = 0;
    int have_soonest = 0;

    for (int i = 0; i < CACHE_CAP; i++) {
        cache_entry *e = &c->ents[i];

        if (!e->used) { if (free_slot < 0) free_slot = i; continue; }

        if (e->expiry <= now) {                     /* stale: reuse freely     */
            e->used = 0;
            if (free_slot < 0) free_slot = i;
            continue;
        }

        /* Same key AND same payload => refresh this exact record's TTL. */
        if (name_type_match(e, rr->name, rr->type) && same_payload(&e->rr, rr)) {
            e->expiry = expiry;
            return;
        }

        /* Track the soonest-expiring live entry as an eviction candidate. */
        if (!have_soonest || e->expiry < soonest_expiry) {
            have_soonest   = 1;
            soonest        = i;
            soonest_expiry = e->expiry;
        }
    }

    int slot = (free_slot >= 0) ? free_slot : soonest;
    cache_entry *e = &c->ents[slot];

    e->rr = *rr;              /* copy the decoded record by value              */
    e->rr.rdata = NULL;       /* CRITICAL: drop the borrowed wire pointer — the
                               * source message will be freed; only the decoded
                               * self-contained fields remain valid. */
    e->expiry = expiry;
    e->used   = 1;
}

int dns_cache_get(dns_cache *c, const char *name, uint16_t type,
                  dns_rr *out, int max)
{
    long now = mono_now();
    int n = 0;

    for (int i = 0; i < CACHE_CAP && n < max; i++) {
        cache_entry *e = &c->ents[i];
        if (!name_type_match(e, name, type)) continue;

        if (e->expiry <= now) {   /* expired: reap it and skip                 */
            e->used = 0;
            continue;
        }

        out[n] = e->rr;
        /* Report the REMAINING lifetime, not the original TTL, so anything we
         * hand this record to (a downstream client, a log) sees an honest
         * countdown rather than a TTL frozen at fetch time. */
        out[n].ttl = (uint32_t)(e->expiry - now);
        n++;
    }
    return n;
}
