/* ===========================================================================
 * zone.h — an in-memory DNS zone loaded from a master (BIND-style) file.
 * ===========================================================================
 *
 * An authoritative server answers from a ZONE: the complete set of records for
 * a domain it is responsible for (e.g. everything at and under example.com).
 * Zones are conventionally written in the RFC 1035 §5 "master file" text
 * format, the same one BIND uses. We parse a practical subset of it.
 *
 * Records are stored fully-qualified and lower-cased so lookups are simple,
 * case-insensitive string compares. RDATA is kept in decoded form (addresses,
 * target names, SOA fields) so the server can emit wire bytes directly.
 * =========================================================================== */
#ifndef ZONE_H
#define ZONE_H

#include "dns.h"

#define ZONE_MAX_RR 512     /* records a single loaded zone may hold           */

typedef struct {
    char     name[DNS_MAX_NAME + 1];   /* owner, FQDN, lower-case, no trailing '.' */
    uint16_t type;                     /* DNS_TYPE_*                            */
    uint32_t ttl;

    /* Decoded RDATA (interpret per `type`): */
    uint8_t  addr4[4];                 /* A                                     */
    uint8_t  addr6[16];                /* AAAA                                  */
    char     target[DNS_MAX_NAME + 1]; /* NS / CNAME / MX exchange (FQDN)       */
    uint16_t mx_pref;                  /* MX preference                         */

    /* SOA rdata (only when type == DNS_TYPE_SOA): */
    char     soa_mname[DNS_MAX_NAME + 1];
    char     soa_rname[DNS_MAX_NAME + 1];
    uint32_t soa_serial, soa_refresh, soa_retry, soa_expire, soa_minimum;
} zone_rr;

typedef struct {
    char     origin[DNS_MAX_NAME + 1]; /* the zone apex, FQDN, no trailing '.'  */
    uint32_t default_ttl;              /* $TTL: TTL for records that omit one   */
    zone_rr  rrs[ZONE_MAX_RR];
    int      nrr;
} zone;

/* Load `path` into `z`. Returns 0 on success, -1 on a parse or I/O error
 * (a diagnostic is printed to stderr with the offending line number). */
int zone_load(zone *z, const char *path);

/* Copy up to `max` records matching (name, type) into out[]. Returns the count.
 * A CNAME at `name` matches ANY query type (the server substitutes it). */
int zone_find(const zone *z, const char *name, uint16_t type,
              const zone_rr **out, int max);

#endif /* ZONE_H */
