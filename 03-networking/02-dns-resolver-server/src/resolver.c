/* ===========================================================================
 * resolver.c — an ITERATIVE DNS resolver, from the root down.
 * ===========================================================================
 *
 * A stub resolver (what getaddrinfo() usually is) asks ONE recursive server
 * "please find me the answer" and waits. This program instead does what that
 * recursive server does: it walks the delegation tree itself.
 *
 *   1. Start at a ROOT server (we ship the well-known root hints below).
 *   2. Ask it, with Recursion Desired = 0, for the name. A root won't know the
 *      answer, but it REFERS us downward: "for anything under .com, go ask
 *      these .com nameservers" — an NS RRset in the Authority section, plus
 *      their addresses ("glue") in the Additional section.
 *   3. Follow the referral to the .com servers, ask again, get referred to the
 *      zone's own nameservers, ask them, and finally receive an authoritative
 *      Answer.
 *   4. Cache every record by its TTL so the next query is cheap, and follow
 *      CNAME aliases when they appear.
 *
 * This is the real machinery of the DNS. Run with `+trace` to watch each hop.
 *
 * SCOPE (honest teaching core): we do standard iterative resolution with glue,
 * NS-name resolution when glue is absent, CNAME chasing, EDNS0, UDP+TCP, and a
 * TTL cache. We do NOT do DNSSEC validation, QNAME minimisation, negative-
 * answer (NXDOMAIN) caching, or nameserver RTT ranking. Those are noted in the
 * README's "Going further".
 * ===========================================================================
 */
#include "dns.h"
#include "net.h"
#include "cache.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>      /* strcasecmp                                        */
#include <unistd.h>       /* getpid                                            */
#include <time.h>         /* time, clock_gettime                              */
#include <arpa/inet.h>    /* inet_ntop for pretty-printing addresses          */

/* Tunables. A resolver bounds EVERYTHING: a hostile or broken zone must not be
 * able to make us loop forever or fan out without limit. */
#define MAX_STEPS      16     /* delegation hops before we give up             */
#define MAX_CNAME      8      /* CNAME redirections we will follow             */
#define MAX_DEPTH      8      /* recursion depth when resolving NS names       */
#define QUERY_TIMEOUT  3000   /* per-server UDP timeout, milliseconds          */
#define QUERY_RETRIES  2      /* UDP attempts per server before moving on      */

/* ---------------------------------------------------------------------------
 * Root hints: the addresses of the 13 root servers (a–m.root-servers.net).
 * These are the ONE piece of bootstrap knowledge a resolver is born with; from
 * here it can discover everything else. They change rarely; a real resolver
 * ships this as a file and refreshes it by querying the roots for "." NS.
 * ------------------------------------------------------------------------- */
static const char *ROOT_HINTS[] = {
    "198.41.0.4",     /* a.root-servers.net */
    "170.247.170.2",  /* b */
    "192.33.4.12",    /* c */
    "199.7.91.13",    /* d */
    "192.203.230.10", /* e */
    "192.5.5.241",    /* f */
    "192.112.36.4",   /* g */
    "198.97.190.53",  /* h */
    "192.36.148.17",  /* i */
    "192.58.128.30",  /* j */
    "193.0.14.129",   /* k */
    "199.7.83.42",    /* l */
    "202.12.27.33",   /* m */
};
#define NUM_ROOTS ((int)(sizeof(ROOT_HINTS) / sizeof(ROOT_HINTS[0])))

/* A working set of nameserver addresses for the current delegation step. */
typedef struct {
    struct sockaddr_storage ss[16];
    socklen_t               slen[16];
    int                     count;
} nsset;

static void nsset_add(nsset *set, const struct sockaddr_storage *ss, socklen_t slen)
{
    if (set->count >= (int)(sizeof(set->ss) / sizeof(set->ss[0]))) return;
    set->ss[set->count]   = *ss;
    set->slen[set->count] = slen;
    set->count++;
}

/* Seed a nameserver set with the root hints. */
static void nsset_roots(nsset *set)
{
    set->count = 0;
    for (int i = 0; i < NUM_ROOTS; i++) {
        struct sockaddr_storage ss;
        socklen_t slen;
        if (dns_addr_from_ip(ROOT_HINTS[i], 53, &ss, &slen) == 0)
            nsset_add(set, &ss, slen);
    }
}

/* A tiny, non-cryptographic PRNG for query IDs. NOTE: a production resolver
 * MUST use unpredictable IDs *and* random source ports to resist off-path
 * cache-poisoning (Kaminsky). We keep it simple and say so in the README. */
static uint16_t rand_id(void)
{
    static unsigned long s;
    if (s == 0) s = (unsigned long)time(NULL) ^ ((unsigned long)getpid() << 16);
    /* xorshift-ish step */
    s ^= s << 13; s ^= s >> 7; s ^= s << 17;
    return (uint16_t)(s & 0xFFFF);
}

/* Print a record in a mydig/dig-ish one-line format (used by +trace and final
 * output). Only the types we decode are expanded; others show as TYPE<n>. */
static void print_rr(const dns_rr *rr)
{
    char buf[64];
    printf("%-28s %6u  IN  ", rr->name[0] ? rr->name : ".", rr->ttl);
    switch (rr->type) {
    case DNS_TYPE_A:
        inet_ntop(AF_INET, rr->addr4, buf, sizeof(buf));
        printf("A     %s\n", buf);
        break;
    case DNS_TYPE_AAAA:
        inet_ntop(AF_INET6, rr->addr6, buf, sizeof(buf));
        printf("AAAA  %s\n", buf);
        break;
    case DNS_TYPE_NS:
        printf("NS    %s\n", rr->target);
        break;
    case DNS_TYPE_CNAME:
        printf("CNAME %s\n", rr->target);
        break;
    case DNS_TYPE_MX:
        printf("MX    %u %s\n", (unsigned)rr->mx_pref, rr->target);
        break;
    case DNS_TYPE_PTR:
        printf("PTR   %s\n", rr->target);
        break;
    default:
        printf("TYPE%u (rdlen %u)\n", (unsigned)rr->type, (unsigned)rr->rdlen);
        break;
    }
}

/* Cache every decoded record in a response's three sections (except the OPT
 * pseudo-RR, which is not real data and carries no cacheable TTL). */
static void cache_response(dns_cache *cache, const dns_response *resp)
{
    const dns_rr *secs[3] = { resp->an, resp->ns, resp->ar };
    int counts[3] = { resp->nan, resp->nns, resp->nar };
    for (int s = 0; s < 3; s++)
        for (int i = 0; i < counts[s]; i++)
            if (secs[s][i].type != DNS_TYPE_OPT)
                dns_cache_put(cache, &secs[s][i]);
}

/* Forward declaration: NS-name resolution recurses back into resolve(). */
static int resolve(dns_cache *cache, const char *qname, uint16_t qtype,
                   dns_rr *answers, int max_answers, int *nanswers,
                   int depth, int verbose);

/* ---------------------------------------------------------------------------
 * build_referral_nsset — from a referral response, produce the set of server
 * addresses to query next.
 *
 * Strategy, in order of preference:
 *   1. Use GLUE: A/AAAA records in the Additional section whose name matches an
 *      NS target in the Authority section. This is the fast path — no extra
 *      lookup needed, and it is how in-bailiwick delegations (ns1.example.com
 *      for example.com) avoid a chicken-and-egg problem.
 *   2. If there is no usable glue, RESOLVE an NS name ourselves (recursively,
 *      depth-bounded) to obtain its address, then use that.
 * Returns the number of server addresses placed in `out`.
 * ------------------------------------------------------------------------- */
static int build_referral_nsset(dns_cache *cache, const dns_response *resp,
                                nsset *out, int depth, int verbose)
{
    out->count = 0;

    /* Pass 1: glue. For each NS record, look for a matching A/AAAA in the
     * additional section. */
    for (int i = 0; i < resp->nns && out->count < 16; i++) {
        if (resp->ns[i].type != DNS_TYPE_NS) continue;
        const char *nsname = resp->ns[i].target;

        for (int j = 0; j < resp->nar && out->count < 16; j++) {
            const dns_rr *g = &resp->ar[j];
            if (g->type != DNS_TYPE_A && g->type != DNS_TYPE_AAAA) continue;
            if (strcasecmp(g->name, nsname) != 0) continue;

            struct sockaddr_storage ss;
            socklen_t slen;
            char ip[64];
            if (g->type == DNS_TYPE_A)
                inet_ntop(AF_INET, g->addr4, ip, sizeof(ip));
            else
                inet_ntop(AF_INET6, g->addr6, ip, sizeof(ip));
            if (dns_addr_from_ip(ip, 53, &ss, &slen) == 0)
                nsset_add(out, &ss, slen);
        }
    }
    if (out->count > 0) return out->count;    /* glue was enough              */

    /* Pass 2: no glue — resolve one NS name to an address ourselves. We stop at
     * the first NS we can resolve; that is enough to make progress. */
    for (int i = 0; i < resp->nns; i++) {
        if (resp->ns[i].type != DNS_TYPE_NS) continue;
        const char *nsname = resp->ns[i].target;

        if (verbose)
            fprintf(stderr, "    (no glue; resolving NS %s)\n", nsname);

        dns_rr addrs[DNS_MAX_RRS];
        int naddr = 0;
        if (resolve(cache, nsname, DNS_TYPE_A, addrs, DNS_MAX_RRS,
                    &naddr, depth + 1, verbose) == 0) {
            for (int k = 0; k < naddr && out->count < 16; k++) {
                if (addrs[k].type != DNS_TYPE_A) continue;
                struct sockaddr_storage ss;
                socklen_t slen;
                char ip[64];
                inet_ntop(AF_INET, addrs[k].addr4, ip, sizeof(ip));
                if (dns_addr_from_ip(ip, 53, &ss, &slen) == 0)
                    nsset_add(out, &ss, slen);
            }
            if (out->count > 0) return out->count;
        }
    }
    return 0;   /* could not find any nameserver address */
}

/* ---------------------------------------------------------------------------
 * scan_answers — pull the requested records out of an answer section, chasing
 * CNAMEs WITHIN this response.
 *
 * Returns:
 *    1  answers of type `qtype` for `name` (or its CNAME chain) were found and
 *       copied to out[]; *nout is set.
 *    0  the chain ended in a CNAME whose target is NOT answered here; the final
 *       target is written to `final` for the caller to resolve afresh. Any
 *       CNAMEs walked are copied into out[] so the caller can print the chain.
 *   -1  nothing relevant found.
 * ------------------------------------------------------------------------- */
static int scan_answers(const dns_response *resp, const char *name,
                        uint16_t qtype, char *final, size_t final_cap,
                        dns_rr *out, int max, int *nout)
{
    char cur[DNS_MAX_NAME + 1];
    snprintf(cur, sizeof(cur), "%s", name);
    *nout = 0;

    for (int hops = 0; hops < MAX_CNAME; hops++) {
        /* First, a direct hit: a record of the requested type for `cur`. */
        int found = 0;
        for (int i = 0; i < resp->nan; i++) {
            if (resp->an[i].type == qtype &&
                strcasecmp(resp->an[i].name, cur) == 0) {
                if (*nout < max) out[(*nout)++] = resp->an[i];
                found = 1;
            }
        }
        if (found) return 1;

        /* No direct hit: is there a CNAME for `cur`? If so, record it and
         * continue the search from the alias target. */
        int followed = 0;
        for (int i = 0; i < resp->nan; i++) {
            if (resp->an[i].type == DNS_TYPE_CNAME &&
                strcasecmp(resp->an[i].name, cur) == 0) {
                if (*nout < max) out[(*nout)++] = resp->an[i];
                snprintf(cur, sizeof(cur), "%s", resp->an[i].target);
                followed = 1;
                break;
            }
        }
        if (!followed) break;    /* no progress: chain ends here              */
    }

    /* If we walked at least one CNAME, `cur` is the unresolved final target. */
    if (*nout > 0) {
        snprintf(final, final_cap, "%s", cur);
        return 0;
    }
    return -1;
}

/* ===========================================================================
 * resolve — the iterative loop for one (qname, qtype).
 * ===========================================================================
 * Returns 0 with answers[] filled on success, -1 on failure/NXDOMAIN.
 * =========================================================================== */
static int resolve(dns_cache *cache, const char *qname, uint16_t qtype,
                   dns_rr *answers, int max_answers, int *nanswers,
                   int depth, int verbose)
{
    *nanswers = 0;
    if (depth > MAX_DEPTH) return -1;    /* NS-resolution recursion too deep   */

    /* Cache shortcut: a live cached RRset answers immediately. */
    int cached = dns_cache_get(cache, qname, qtype, answers, max_answers);
    if (cached > 0) {
        if (verbose) fprintf(stderr, ";; cache hit for %s\n", qname);
        *nanswers = cached;
        return 0;
    }

    nsset servers;
    nsset_roots(&servers);               /* begin at the roots                 */

    char cur_name[DNS_MAX_NAME + 1];
    snprintf(cur_name, sizeof(cur_name), "%s", qname);

    for (int step = 0; step < MAX_STEPS; step++) {
        /* --- build the query for this hop (RD=0: we drive iteration) ------- */
        uint8_t qbuf[DNS_MSG_MAX];
        uint16_t id = rand_id();
        size_t qlen = dns_build_query(qbuf, sizeof(qbuf), id, cur_name, qtype,
                                      0 /* RD off */, 1 /* EDNS0 */,
                                      DNS_EDNS_UDP_SIZE);
        if (qlen == 0) return -1;        /* query didn't fit (impossible here) */

        /* --- try each candidate server until one answers ------------------ */
        uint8_t rbuf[DNS_MSG_MAX];
        size_t  rlen = 0;
        dns_response resp;
        int got = 0;

        for (int s = 0; s < servers.count; s++) {
            if (verbose) {
                char ip[64] = "?";
                const struct sockaddr *sa = (const struct sockaddr *)&servers.ss[s];
                if (sa->sa_family == AF_INET)
                    inet_ntop(AF_INET, &((struct sockaddr_in *)sa)->sin_addr, ip, sizeof(ip));
                else
                    inet_ntop(AF_INET6, &((struct sockaddr_in6 *)sa)->sin6_addr, ip, sizeof(ip));
                fprintf(stderr, ";; [step %d] asking %s for %s\n", step, ip, cur_name);
            }

            if (dns_exchange(&servers.ss[s], servers.slen[s], qbuf, qlen,
                             rbuf, sizeof(rbuf), &rlen, QUERY_TIMEOUT,
                             QUERY_RETRIES) < 0)
                continue;                /* dead/slow server: try the next     */

            if (dns_parse_response(rbuf, rlen, &resp) < 0)
                continue;                /* garbage reply: try the next        */

            /* Reject replies that don't match our query ID or aren't answers.
             * An off-path attacker must guess the 16-bit ID (and source port)
             * to forge a reply; a mismatch means "not the answer we want". */
            if (resp.h.id != id)              continue;
            if (!(resp.h.flags & DNS_FLAG_QR)) continue;

            got = 1;
            break;
        }
        if (!got) return -1;             /* no server in this set responded    */

        cache_response(cache, &resp);    /* learn from everything we received  */

        uint16_t rcode = resp.h.flags & DNS_RCODE_MASK;

        /* --- did we get answers (or a CNAME chain)? ----------------------- */
        if (resp.nan > 0) {
            char final[DNS_MAX_NAME + 1];
            dns_rr got_rrs[DNS_MAX_RRS];
            int ngot = 0;
            int r = scan_answers(&resp, cur_name, qtype, final, sizeof(final),
                                 got_rrs, DNS_MAX_RRS, &ngot);
            if (r == 1) {
                /* Complete: copy answers out. */
                for (int i = 0; i < ngot && *nanswers < max_answers; i++)
                    answers[(*nanswers)++] = got_rrs[i];
                return 0;
            }
            if (r == 0) {
                /* CNAME chain ended off-response: emit the CNAMEs we saw, then
                 * resolve the final alias target from scratch and append. */
                for (int i = 0; i < ngot && *nanswers < max_answers; i++)
                    answers[(*nanswers)++] = got_rrs[i];

                dns_rr more[DNS_MAX_RRS];
                int nmore = 0;
                if (resolve(cache, final, qtype, more, DNS_MAX_RRS, &nmore,
                            depth + 1, verbose) == 0) {
                    for (int i = 0; i < nmore && *nanswers < max_answers; i++)
                        answers[(*nanswers)++] = more[i];
                    return (*nanswers > 0) ? 0 : -1;
                }
                return -1;
            }
            /* r == -1: answers present but none relevant; fall through. */
        }

        /* --- authoritative "no such name" is final ----------------------- */
        if (rcode == DNS_RCODE_NXDOMAIN) {
            if (verbose) fprintf(stderr, ";; NXDOMAIN for %s\n", cur_name);
            return -1;
        }

        /* --- a referral? descend to the delegated nameservers ------------- */
        if (resp.nns > 0) {
            nsset next;
            if (build_referral_nsset(cache, &resp, &next, depth, verbose) > 0) {
                servers = next;          /* move one level down the tree       */
                continue;
            }
            if (verbose) fprintf(stderr, ";; referral had no usable NS addr\n");
            return -1;
        }

        /* No answer, no referral, not NXDOMAIN: NODATA or an empty response. */
        if (verbose) fprintf(stderr, ";; no answer and no referral for %s\n", cur_name);
        return -1;
    }

    return -1;   /* ran out of delegation steps (loop or very deep tree) */
}

/* ---------------------------------------------------------------------------
 * Type-name <-> number helpers for the CLI.
 * ------------------------------------------------------------------------- */
static uint16_t type_from_name(const char *s)
{
    if (!strcasecmp(s, "A"))     return DNS_TYPE_A;
    if (!strcasecmp(s, "AAAA"))  return DNS_TYPE_AAAA;
    if (!strcasecmp(s, "NS"))    return DNS_TYPE_NS;
    if (!strcasecmp(s, "CNAME")) return DNS_TYPE_CNAME;
    if (!strcasecmp(s, "MX"))    return DNS_TYPE_MX;
    if (!strcasecmp(s, "PTR"))   return DNS_TYPE_PTR;
    if (!strcasecmp(s, "TXT"))   return DNS_TYPE_TXT;
    if (!strcasecmp(s, "SOA"))   return DNS_TYPE_SOA;
    return 0;
}

static void usage(const char *argv0)
{
    fprintf(stderr,
        "usage: %s [+trace] <name> [type]\n"
        "  iterative DNS resolver — walks from the roots, honoring TTLs.\n"
        "  type defaults to A; supported: A AAAA NS CNAME MX PTR TXT SOA\n"
        "  +trace prints every delegation hop to stderr (like `dig +trace`).\n"
        "example: %s +trace www.example.com A\n",
        argv0, argv0);
}

int main(int argc, char **argv)
{
    int verbose = 0;
    const char *name = NULL;
    const char *type_str = "A";

    /* Minimal arg parse: an optional leading +trace, then name, then type. */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "+trace") == 0) { verbose = 1; continue; }
        if (!name)      name = argv[i];
        else            type_str = argv[i];
    }
    if (!name) { usage(argv[0]); return 2; }

    uint16_t qtype = type_from_name(type_str);
    if (qtype == 0) {
        fprintf(stderr, "unknown type '%s'\n", type_str);
        return 2;
    }

    /* One cache lives for the whole process, so repeated lookups (and the NS
     * lookups triggered internally) share cached data. */
    static dns_cache cache;              /* static: zero-initialised, big-ish  */
    dns_cache_init(&cache);

    dns_rr answers[DNS_MAX_RRS];
    int nanswers = 0;
    int rc = resolve(&cache, name, qtype, answers, DNS_MAX_RRS,
                     &nanswers, 0, verbose);

    if (rc != 0 || nanswers == 0) {
        printf(";; no answer for %s %s\n", name, type_str);
        return 1;
    }

    printf(";; ANSWER (%d record%s):\n", nanswers, nanswers == 1 ? "" : "s");
    for (int i = 0; i < nanswers; i++)
        print_rr(&answers[i]);
    return 0;
}
