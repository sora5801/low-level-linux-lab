/* ===========================================================================
 * zone.c — parse a BIND-style master file into an in-memory zone.
 * ===========================================================================
 *
 * The master file format (RFC 1035 §5) is deceptively fiddly. This parser
 * handles the constructs you meet in practice:
 *
 *   $ORIGIN example.com.        directive: names below are relative to this
 *   $TTL 3600                   directive: default TTL for records without one
 *   @      IN SOA ns1 admin (   '@' = the origin; '(' begins multi-line RDATA
 *              2024010101       serial   ) parenthesised RDATA spans lines until
 *              3600 900         refresh/retry   the matching ')' — we join them
 *              604800 86400 )   expire/minimum
 *          IN NS  ns1.example.com.   blank owner => inherit the previous owner
 *   www    IN A   192.0.2.2     relative owner 'www' => www.example.com
 *   www    IN AAAA 2001:db8::2
 *   mail   IN MX 10 mail        MX has a 16-bit preference before the exchange
 *
 * Names are normalised to fully-qualified, lower-case, no trailing dot.
 *
 * DELIBERATE OMISSIONS (teaching core): no $INCLUDE, no $GENERATE, no TXT
 * quoting rules, no class other than IN. These are noted in the README.
 * ===========================================================================
 */
#include "zone.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>      /* strcasecmp — DNS names compare case-insensitively */
#include <ctype.h>
#include <arpa/inet.h>    /* inet_pton for A/AAAA rdata                        */

/* Lower-case a string in place (DNS names are case-insensitive; storing them
 * folded lets lookups be a plain strcmp). */
static void str_tolower(char *s)
{
    for (; *s; s++) *s = (char)tolower((unsigned char)*s);
}

/* ---------------------------------------------------------------------------
 * qualify — turn a name token from the file into a stored FQDN.
 *   "@"            -> the origin
 *   ends with '.'  -> already absolute; drop the trailing dot
 *   otherwise      -> relative: append "." + origin
 * The result is lower-cased and has no trailing dot. Returns 0 / -1.
 * ------------------------------------------------------------------------- */
static int qualify(const char *tok, const char *origin, char *out, size_t cap)
{
    if (strcmp(tok, "@") == 0) {
        snprintf(out, cap, "%s", origin);
    } else {
        size_t n = strlen(tok);
        if (n > 0 && tok[n - 1] == '.') {
            /* absolute — copy without the trailing dot */
            if (n - 1 >= cap) return -1;
            memcpy(out, tok, n - 1);
            out[n - 1] = '\0';
        } else if (origin[0]) {
            snprintf(out, cap, "%s.%s", tok, origin);
        } else {
            snprintf(out, cap, "%s", tok);   /* root origin: name stands alone */
        }
    }
    str_tolower(out);
    return 0;
}

/* Split `line` into whitespace-separated tokens (in place). Returns the count;
 * tok[] point into `line`. Simple: no quoted-string handling (we don't parse
 * TXT rdata). */
static int tokenize(char *line, char *tok[], int max)
{
    int n = 0;
    char *p = line;
    while (*p && n < max) {
        while (*p && isspace((unsigned char)*p)) p++;   /* skip whitespace     */
        if (!*p) break;
        tok[n++] = p;                                    /* token starts here   */
        while (*p && !isspace((unsigned char)*p)) p++;   /* run to next space   */
        if (*p) *p++ = '\0';                             /* terminate the token */
    }
    return n;
}

/* Strip a ';' comment to end-of-line (we do not support ';' inside TXT). */
static void strip_comment(char *s)
{
    char *c = strchr(s, ';');
    if (c) *c = '\0';
}

/* Parse one already-assembled logical record line (parentheses removed, blanks
 * squeezed) into the zone. `owner_inherited` is 1 when the physical line began
 * with whitespace (owner carries over from the previous record). Returns 0/-1. */
static int parse_record(zone *z, char *line, int owner_inherited,
                        char prev_owner[DNS_MAX_NAME + 1], int lineno)
{
    char *tok[32];
    int nt = tokenize(line, tok, 32);
    if (nt == 0) return 0;              /* blank after stripping: nothing to do */

    int idx = 0;
    char owner[DNS_MAX_NAME + 1];

    if (owner_inherited) {
        snprintf(owner, sizeof(owner), "%s", prev_owner);
    } else {
        if (qualify(tok[idx++], z->origin, owner, sizeof(owner)) < 0) {
            fprintf(stderr, "zone: line %d: bad owner name\n", lineno);
            return -1;
        }
        snprintf(prev_owner, DNS_MAX_NAME + 1, "%s", owner);
    }

    /* Optional TTL (all digits) and CLASS (IN) may appear in either order. */
    uint32_t ttl = z->default_ttl;
    for (; idx < nt; idx++) {
        char *t = tok[idx];
        if (strcasecmp(t, "IN") == 0) continue;          /* class, assumed IN  */
        int all_digit = 1;
        for (char *d = t; *d; d++) if (!isdigit((unsigned char)*d)) all_digit = 0;
        if (all_digit && t[0]) { ttl = (uint32_t)strtoul(t, NULL, 10); continue; }
        break;   /* not a TTL or class: this must be the TYPE token */
    }
    if (idx >= nt) { fprintf(stderr, "zone: line %d: missing type\n", lineno); return -1; }

    const char *type_s = tok[idx++];
    if (z->nrr >= ZONE_MAX_RR) {
        fprintf(stderr, "zone: too many records (max %d)\n", ZONE_MAX_RR);
        return -1;
    }
    zone_rr *rr = &z->rrs[z->nrr];
    memset(rr, 0, sizeof(*rr));
    snprintf(rr->name, sizeof(rr->name), "%s", owner);
    rr->ttl = ttl;

    /* Dispatch on the record type; each expects a specific RDATA shape. */
    if (strcasecmp(type_s, "A") == 0) {
        rr->type = DNS_TYPE_A;
        if (idx >= nt || inet_pton(AF_INET, tok[idx], rr->addr4) != 1) {
            fprintf(stderr, "zone: line %d: bad A address\n", lineno); return -1;
        }
    } else if (strcasecmp(type_s, "AAAA") == 0) {
        rr->type = DNS_TYPE_AAAA;
        if (idx >= nt || inet_pton(AF_INET6, tok[idx], rr->addr6) != 1) {
            fprintf(stderr, "zone: line %d: bad AAAA address\n", lineno); return -1;
        }
    } else if (strcasecmp(type_s, "NS") == 0) {
        rr->type = DNS_TYPE_NS;
        if (idx >= nt || qualify(tok[idx], z->origin, rr->target, sizeof(rr->target)) < 0) {
            fprintf(stderr, "zone: line %d: bad NS target\n", lineno); return -1;
        }
    } else if (strcasecmp(type_s, "CNAME") == 0) {
        rr->type = DNS_TYPE_CNAME;
        if (idx >= nt || qualify(tok[idx], z->origin, rr->target, sizeof(rr->target)) < 0) {
            fprintf(stderr, "zone: line %d: bad CNAME target\n", lineno); return -1;
        }
    } else if (strcasecmp(type_s, "MX") == 0) {
        rr->type = DNS_TYPE_MX;
        /* MX rdata: <preference> <exchange> */
        if (idx + 1 >= nt) { fprintf(stderr, "zone: line %d: MX needs pref+host\n", lineno); return -1; }
        rr->mx_pref = (uint16_t)strtoul(tok[idx], NULL, 10);
        if (qualify(tok[idx + 1], z->origin, rr->target, sizeof(rr->target)) < 0) {
            fprintf(stderr, "zone: line %d: bad MX host\n", lineno); return -1;
        }
    } else if (strcasecmp(type_s, "SOA") == 0) {
        rr->type = DNS_TYPE_SOA;
        /* SOA rdata: <mname> <rname> <serial> <refresh> <retry> <expire> <min> */
        if (idx + 6 >= nt) { fprintf(stderr, "zone: line %d: SOA needs 7 fields\n", lineno); return -1; }
        if (qualify(tok[idx],     z->origin, rr->soa_mname, sizeof(rr->soa_mname)) < 0 ||
            qualify(tok[idx + 1], z->origin, rr->soa_rname, sizeof(rr->soa_rname)) < 0) {
            fprintf(stderr, "zone: line %d: bad SOA name\n", lineno); return -1;
        }
        rr->soa_serial  = (uint32_t)strtoul(tok[idx + 2], NULL, 10);
        rr->soa_refresh = (uint32_t)strtoul(tok[idx + 3], NULL, 10);
        rr->soa_retry   = (uint32_t)strtoul(tok[idx + 4], NULL, 10);
        rr->soa_expire  = (uint32_t)strtoul(tok[idx + 5], NULL, 10);
        rr->soa_minimum = (uint32_t)strtoul(tok[idx + 6], NULL, 10);
    } else {
        fprintf(stderr, "zone: line %d: unsupported type '%s'\n", lineno, type_s);
        return -1;
    }

    z->nrr++;
    return 0;
}

/* ===========================================================================
 * zone_load — read the whole file, assembling logical lines and directives.
 * ===========================================================================
 */
int zone_load(zone *z, const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) { fprintf(stderr, "zone: cannot open %s\n", path); return -1; }

    memset(z, 0, sizeof(*z));
    z->default_ttl = 3600;             /* a sane default if $TTL is absent      */
    z->origin[0] = '\0';

    char prev_owner[DNS_MAX_NAME + 1] = "";

    /* Logical-line assembly across parenthesised continuations. */
    char logical[4096];
    size_t lcap = sizeof(logical);
    logical[0] = '\0';
    int  in_parens = 0;                /* inside a ( ... ) RDATA block?         */
    int  logical_owner_ws = 0;         /* did the logical record start indented?*/
    int  logical_start_line = 0;

    char raw[2048];
    int  physical_lineno = 0;
    int  rc = 0;

    while (fgets(raw, sizeof(raw), f)) {
        physical_lineno++;

        /* Note whether THIS physical line started with whitespace BEFORE we
         * strip anything — that is how "inherit the owner" is signalled. */
        int starts_ws = (raw[0] == ' ' || raw[0] == '\t');

        strip_comment(raw);

        /* Directives ($ORIGIN/$TTL) only at the start of a logical line and
         * never inside parentheses. */
        if (!in_parens && raw[0] == '$') {
            char *tok[4];
            char tmp[2048];
            snprintf(tmp, sizeof(tmp), "%s", raw);
            int nt = tokenize(tmp, tok, 4);
            if (nt >= 2 && strcasecmp(tok[0], "$ORIGIN") == 0) {
                /* Store origin FQDN without trailing dot, lower-cased. */
                char o[DNS_MAX_NAME + 1];
                snprintf(o, sizeof(o), "%s", tok[1]);
                size_t on = strlen(o);
                if (on && o[on - 1] == '.') o[on - 1] = '\0';
                str_tolower(o);
                snprintf(z->origin, sizeof(z->origin), "%s", o);
            } else if (nt >= 2 && strcasecmp(tok[0], "$TTL") == 0) {
                z->default_ttl = (uint32_t)strtoul(tok[1], NULL, 10);
            } else {
                fprintf(stderr, "zone: line %d: unknown directive\n", physical_lineno);
                rc = -1; goto done;
            }
            continue;
        }

        /* Track parenthesis depth and replace the paren characters with spaces
         * so the eventual token stream is clean. */
        int had_content_before = (logical[0] != '\0');
        for (char *p = raw; *p; p++) {
            if (*p == '(') { in_parens++; *p = ' '; }
            else if (*p == ')') { if (in_parens > 0) in_parens--; *p = ' '; }
        }

        /* If this line begins a new logical record, remember its indentation. */
        if (!had_content_before) {
            logical_owner_ws   = starts_ws;
            logical_start_line = physical_lineno;
        }

        /* Append this physical line to the logical buffer. */
        size_t cur = strlen(logical);
        size_t add = strlen(raw);
        if (cur + add + 1 >= lcap) {
            fprintf(stderr, "zone: line %d: logical line too long\n", physical_lineno);
            rc = -1; goto done;
        }
        memcpy(logical + cur, raw, add + 1);

        /* A logical line is complete when we are not inside parentheses. */
        if (!in_parens) {
            /* Skip lines that are entirely blank/comment. */
            int only_ws = 1;
            for (char *p = logical; *p; p++)
                if (!isspace((unsigned char)*p)) { only_ws = 0; break; }
            if (!only_ws) {
                if (parse_record(z, logical, logical_owner_ws, prev_owner,
                                 logical_start_line) < 0) {
                    rc = -1; goto done;
                }
            }
            logical[0] = '\0';        /* reset for the next logical line        */
        }
    }

    if (in_parens) {
        fprintf(stderr, "zone: unbalanced '(' at EOF\n");
        rc = -1;
    }

done:
    fclose(f);
    if (rc == 0 && z->origin[0] == '\0')
        fprintf(stderr, "zone: warning: no $ORIGIN; relative names may be wrong\n");
    return rc;
}

/* ---------------------------------------------------------------------------
 * zone_find — collect records for (name, type). A CNAME at the name matches
 * any type: the caller substitutes the alias and re-queries.
 * ------------------------------------------------------------------------- */
int zone_find(const zone *z, const char *name, uint16_t type,
              const zone_rr **out, int max)
{
    int n = 0;
    for (int i = 0; i < z->nrr && n < max; i++) {
        const zone_rr *rr = &z->rrs[i];
        if (strcasecmp(rr->name, name) != 0) continue;
        if (rr->type == type || rr->type == DNS_TYPE_CNAME)
            out[n++] = rr;
    }
    return n;
}
