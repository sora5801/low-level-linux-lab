/* ===========================================================================
 * server.c — a small AUTHORITATIVE DNS server answering from a zone file.
 * ===========================================================================
 *
 * The mirror image of the resolver. Instead of walking the tree to find an
 * answer, this process IS the source of truth for one zone: it loads a master
 * file (see zone.c) and answers queries about names in that zone, setting the
 * AA (Authoritative Answer) bit. It speaks UDP and TCP on port 53 (default here
 * 5353 so it runs without root), sets TC and expects a TCP retry when a UDP
 * answer would overflow, and honours EDNS0.
 *
 * WIRE-WRITING HIGHLIGHT: this file WRITES a compression pointer. When an
 * answer record's owner equals the question name (the common case), we emit the
 * 2-byte pointer 0xC00C — "the name is the one at offset 12" — instead of
 * repeating the labels. That is the encoder half of the decoder in wire.c /
 * asm/demo.c.
 *
 * SCOPE (teaching core): single zone, iterative (one request at a time) TCP
 * handling, no AXFR/zone transfer, no DNSSEC signing, no wildcard/`*` matching.
 * Noted in the README.
 * ===========================================================================
 */
#include "dns.h"
#include "zone.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <strings.h>
#include <poll.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

/* The question name always begins at byte 12, immediately after the fixed
 * header. A compression pointer to it is therefore 0xC000 | 12 = 0xC00C. */
#define QNAME_OFFSET 12

/* ---------------------------------------------------------------------------
 * write_owner — write an RR owner name, using a compression pointer to the
 * question when they are equal (the usual case), else the full labels.
 * ------------------------------------------------------------------------- */
static void write_owner(dns_writer *w, const char *name, const char *qname)
{
    if (strcasecmp(name, qname) == 0) {
        /* Two-byte pointer: 0b11 flag bits + 14-bit offset (=12). Saves
         * repeating the whole name and demonstrates the encoder side of
         * compression. */
        dns_write_u16(w, (uint16_t)(0xC000 | QNAME_OFFSET));
    } else {
        dns_write_name(w, name);
    }
}

/* Patch a previously-reserved 16-bit RDLENGTH placeholder at `at` with the
 * number of RDATA bytes that followed it. No-op if the writer overflowed. */
static void patch_rdlength(dns_writer *w, size_t at)
{
    if (w->overflow) return;
    size_t rdlen = w->len - (at + 2);           /* bytes written after the u16 */
    w->buf[at]     = (uint8_t)(rdlen >> 8);
    w->buf[at + 1] = (uint8_t)(rdlen & 0xFF);
}

/* ---------------------------------------------------------------------------
 * write_rr — serialise one zone record. `qname` is the question name so the
 * owner can be compressed to a pointer when it matches.
 * ------------------------------------------------------------------------- */
static void write_rr(dns_writer *w, const zone_rr *rr, const char *qname)
{
    write_owner(w, rr->name, qname);
    dns_write_u16(w, rr->type);
    dns_write_u16(w, DNS_CLASS_IN);
    dns_write_u32(w, rr->ttl);

    size_t rdlen_at = w->len;         /* remember where RDLENGTH goes          */
    dns_write_u16(w, 0);              /* placeholder, patched below            */

    switch (rr->type) {
    case DNS_TYPE_A:
        dns_write_bytes(w, rr->addr4, 4);
        break;
    case DNS_TYPE_AAAA:
        dns_write_bytes(w, rr->addr6, 16);
        break;
    case DNS_TYPE_NS:
    case DNS_TYPE_CNAME:
        dns_write_name(w, rr->target);          /* rdata is a single name      */
        break;
    case DNS_TYPE_MX:
        dns_write_u16(w, rr->mx_pref);          /* 16-bit preference first     */
        dns_write_name(w, rr->target);          /* then the exchange host      */
        break;
    case DNS_TYPE_SOA:
        dns_write_name(w, rr->soa_mname);
        dns_write_name(w, rr->soa_rname);
        dns_write_u32(w, rr->soa_serial);
        dns_write_u32(w, rr->soa_refresh);
        dns_write_u32(w, rr->soa_retry);
        dns_write_u32(w, rr->soa_expire);
        dns_write_u32(w, rr->soa_minimum);
        break;
    default:
        break;   /* unsupported types emit an empty RDATA (RDLENGTH 0)         */
    }

    patch_rdlength(w, rdlen_at);
}

/* Is `name` inside this zone (equal to the origin or a subdomain of it)? Used
 * to decide NXDOMAIN (we are authoritative and the name is ours but absent)
 * versus REFUSED (the name isn't in a zone we serve). */
static int in_zone(const zone *z, const char *name)
{
    size_t nlen = strlen(name), olen = strlen(z->origin);
    if (olen == 0) return 1;                                   /* root zone    */
    if (strcasecmp(name, z->origin) == 0) return 1;            /* the apex     */
    if (nlen > olen &&
        name[nlen - olen - 1] == '.' &&
        strcasecmp(name + nlen - olen, z->origin) == 0)        /* a subdomain  */
        return 1;
    return 0;
}

/* Find the zone's SOA record (used in the Authority section of negative
 * answers so a caching resolver knows how long to cache the negative result). */
static const zone_rr *zone_soa(const zone *z)
{
    for (int i = 0; i < z->nrr; i++)
        if (z->rrs[i].type == DNS_TYPE_SOA)
            return &z->rrs[i];
    return NULL;
}

/* ===========================================================================
 * build_response — turn a query message into a response message.
 * ===========================================================================
 * Returns the response length. `over_tcp` disables truncation (TCP has room);
 * for UDP we cap at `max_udp` and set TC if we would exceed it.
 * =========================================================================== */
static size_t build_response(const zone *z, const uint8_t *query, size_t qlen,
                             uint8_t *resp, size_t resp_cap,
                             int over_tcp, uint16_t max_udp)
{
    dns_reader r;
    dns_reader_init(&r, query, qlen);

    dns_header qh;
    if (dns_read_header(&r, &qh)) return 0;      /* unparseable: drop it       */

    /* Parse the (first) question. If we can't, reply FORMERR (header + nothing). */
    char qname[DNS_MAX_NAME + 1];
    uint16_t qtype = 0, qclass = 0;
    int q_ok = (qh.qdcount >= 1) &&
               (dns_read_question(&r, qname, sizeof(qname), &qtype, &qclass) == 0);

    /* Does the query carry an EDNS0 OPT? If so we must echo one and may use its
     * advertised UDP size. Scan the additional section for TYPE=OPT. We reuse
     * the full parser for robustness. */
    int client_edns = 0;
    uint16_t client_udp = DNS_UDP_CLASSIC_MAX;
    {
        dns_response tmp;
        if (dns_parse_response(query, qlen, &tmp) == 0) {
            for (int i = 0; i < tmp.nar; i++) {
                if (tmp.ar[i].type == DNS_TYPE_OPT) {
                    client_edns = 1;
                    /* For an OPT RR the CLASS field carried the UDP size; our
                     * parser stored it in rclass. Clamp to something sane. */
                    client_udp = tmp.ar[i].rclass;
                    if (client_udp < DNS_UDP_CLASSIC_MAX) client_udp = DNS_UDP_CLASSIC_MAX;
                    break;
                }
            }
        }
    }
    /* If the client used EDNS0, honour the (>=512, already clamped) UDP buffer
     * it advertised — this is what lets us send answers larger than 512 bytes
     * over UDP instead of forcing a TCP retry. Without EDNS0 we stay at 512. */
    if (client_edns && !over_tcp)
        max_udp = client_udp;

    /* ---- header ------------------------------------------------------- */
    dns_writer w;
    dns_writer_init(&w, resp, resp_cap);

    dns_header h;
    memset(&h, 0, sizeof(h));
    h.id    = qh.id;                             /* echo the transaction ID     */
    h.flags = DNS_FLAG_QR;                       /* this is a response          */
    h.flags |= (qh.flags & DNS_FLAG_RD);         /* mirror Recursion Desired    */
    /* We do not offer recursion (RA stays 0): we are authoritative-only. */

    if (!q_ok) {
        h.flags |= DNS_RCODE_FORMERR;            /* couldn't parse the question */
        dns_write_header(&w, &h);
        return w.overflow ? 0 : w.len;
    }

    /* Reserve the header now; we know qdcount=1 but not the answer counts yet,
     * so we write the header at the end by tracking counts and patching. To
     * keep it simple we build the body first into local counters, then write
     * the header, then the body. Because the compression pointer references
     * offset 12, the question MUST occupy its canonical place — which it does,
     * as we always write header (12) + question next. */

    /* Determine the answer set, chasing an in-zone CNAME once if needed. */
    const zone_rr *ans[DNS_MAX_RRS];
    int nans = 0;
    char lookup[DNS_MAX_NAME + 1];
    snprintf(lookup, sizeof(lookup), "%s", qname);

    int authoritative = in_zone(z, qname);
    uint16_t rcode = DNS_RCODE_NOERROR;
    int add_soa = 0;

    if (!authoritative) {
        rcode = DNS_RCODE_REFUSED;               /* not our zone                */
    } else {
        /* Try direct + CNAME-substituted lookups (bounded). */
        int hops = 0;
        for (;;) {
            const zone_rr *hit[DNS_MAX_RRS];
            int nhit = zone_find(z, lookup, qtype, hit, DNS_MAX_RRS);

            /* Separate CNAMEs from direct-type hits. */
            const zone_rr *cname = NULL;
            int direct = 0;
            for (int i = 0; i < nhit; i++) {
                if (hit[i]->type == DNS_TYPE_CNAME && qtype != DNS_TYPE_CNAME)
                    cname = hit[i];
                else if (hit[i]->type == qtype)
                    { if (nans < DNS_MAX_RRS) ans[nans++] = hit[i]; direct++; }
            }
            if (direct > 0) break;               /* found the requested type    */

            if (cname && hops < 8) {
                /* Emit the CNAME, then follow it within the zone. */
                if (nans < DNS_MAX_RRS) ans[nans++] = cname;
                snprintf(lookup, sizeof(lookup), "%s", cname->target);
                hops++;
                /* Only keep chasing if the alias is still in this zone. */
                if (!in_zone(z, lookup)) break;
                continue;
            }

            /* No direct hit and no CNAME to follow. Is the NAME present at all
             * (some other type => NODATA) or absent (=> NXDOMAIN)? */
            int name_exists = 0;
            for (int i = 0; i < z->nrr; i++)
                if (strcasecmp(z->rrs[i].name, lookup) == 0) { name_exists = 1; break; }
            if (!name_exists) rcode = DNS_RCODE_NXDOMAIN;
            add_soa = 1;                          /* include SOA in authority    */
            break;
        }
        /* We reached this branch because the name is in our zone, so EVERY
         * outcome here — positive, NODATA, or NXDOMAIN — is authoritative. */
        h.flags |= DNS_FLAG_AA;
    }
    h.flags |= rcode;

    /* Authority/additional records. For a positive answer we volunteer the
     * zone's NS set (authority) and their addresses (additional glue). For a
     * negative answer we include the SOA so the resolver can cache the "no"
     * with the SOA MINIMUM TTL. */
    const zone_rr *authority[DNS_MAX_RRS]; int nauth = 0;
    const zone_rr *additional[DNS_MAX_RRS]; int nadd = 0;

    if (add_soa) {
        const zone_rr *soa = zone_soa(z);
        if (soa) authority[nauth++] = soa;
    } else if (nans > 0 && authoritative) {
        /* Apex NS set as authority (nice-to-have; a real server always does). */
        const zone_rr *ns[DNS_MAX_RRS];
        int nns = zone_find(z, z->origin, DNS_TYPE_NS, ns, DNS_MAX_RRS);
        for (int i = 0; i < nns && nauth < DNS_MAX_RRS; i++)
            if (ns[i]->type == DNS_TYPE_NS) {
                authority[nauth++] = ns[i];
                /* Add glue: the NS host's A/AAAA if we have it. */
                const zone_rr *glue[DNS_MAX_RRS];
                int ng = zone_find(z, ns[i]->target, DNS_TYPE_A, glue, DNS_MAX_RRS);
                for (int j = 0; j < ng && nadd < DNS_MAX_RRS; j++)
                    if (glue[j]->type == DNS_TYPE_A) additional[nadd++] = glue[j];
            }
    }

    /* Now that all counts are known, fill and write the header + question. */
    h.qdcount = 1;
    h.ancount = (uint16_t)nans;
    h.nscount = (uint16_t)nauth;
    /* +1 for our own OPT if the client used EDNS0 (echo it in additional). */
    h.arcount = (uint16_t)(nadd + (client_edns ? 1 : 0));
    dns_write_header(&w, &h);

    /* Question: copied verbatim (uncompressed) — this is what offset 12 points
     * to, so owner-name compression in the answers resolves correctly. */
    dns_write_name(&w, qname);
    dns_write_u16(&w, qtype);
    dns_write_u16(&w, qclass);

    for (int i = 0; i < nans;  i++) write_rr(&w, ans[i],        qname);
    for (int i = 0; i < nauth; i++) write_rr(&w, authority[i],  qname);
    for (int i = 0; i < nadd;  i++) write_rr(&w, additional[i], qname);

    /* Echo an EDNS0 OPT so the client knows we understood EDNS. */
    if (client_edns)
        dns_write_opt(&w, DNS_EDNS_UDP_SIZE, 0);

    if (w.overflow) return 0;                    /* somehow didn't fit at all   */

    /* UDP truncation: if the full response exceeds the client's UDP budget,
     * send a TC-flagged, answer-less response so the client retries over TCP. */
    if (!over_tcp && w.len > max_udp) {
        dns_writer tw;
        dns_writer_init(&tw, resp, resp_cap);
        dns_header th = h;
        th.flags |= DNS_FLAG_TC;                 /* set the truncated bit       */
        th.ancount = th.nscount = 0;
        th.arcount = client_edns ? 1 : 0;
        dns_write_header(&tw, &th);
        dns_write_name(&tw, qname);
        dns_write_u16(&tw, qtype);
        dns_write_u16(&tw, qclass);
        if (client_edns) dns_write_opt(&tw, DNS_EDNS_UDP_SIZE, 0);
        return tw.overflow ? 0 : tw.len;
    }

    return w.len;
}

/* ---------------------------------------------------------------------------
 * make_udp / make_tcp — create, set SO_REUSEADDR, and bind a listening socket.
 * ------------------------------------------------------------------------- */
static int make_socket(int type, uint16_t port)
{
    /* AF_INET/SOCK_DGRAM (UDP) or SOCK_STREAM (TCP). */
    int fd = socket(AF_INET, type, 0);
    if (fd < 0) { perror("socket"); return -1; }

    /* SO_REUSEADDR lets us rebind the port immediately after a restart instead
     * of waiting out the kernel's TIME_WAIT hold-down. */
    int one = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0)
        perror("setsockopt");

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);    /* bind all interfaces         */
    addr.sin_port = htons(port);                 /* port in network byte order  */

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(fd);
        return -1;
    }
    return fd;
}

/* Serve one TCP client synchronously: read the framed query, answer, close. */
static void serve_tcp_client(int cfd, const zone *z)
{
    uint8_t lenb[2];
    /* Read the 2-byte length prefix (looping over short reads). */
    size_t off = 0;
    while (off < 2) {
        ssize_t g = read(cfd, lenb + off, 2 - off);
        if (g > 0) { off += (size_t)g; continue; }
        if (g < 0 && errno == EINTR) continue;
        close(cfd); return;                      /* EOF or error                */
    }
    size_t need = ((size_t)lenb[0] << 8) | lenb[1];
    if (need == 0 || need > DNS_MSG_MAX) { close(cfd); return; }

    uint8_t qbuf[DNS_MSG_MAX];
    off = 0;
    while (off < need) {
        ssize_t g = read(cfd, qbuf + off, need - off);
        if (g > 0) { off += (size_t)g; continue; }
        if (g < 0 && errno == EINTR) continue;
        close(cfd); return;
    }

    uint8_t rbuf[DNS_MSG_MAX];
    size_t rlen = build_response(z, qbuf, need, rbuf, sizeof(rbuf),
                                 1 /* over_tcp */, DNS_MSG_MAX);
    if (rlen == 0) { close(cfd); return; }

    /* Write the 2-byte length prefix then the response (looping short writes). */
    uint8_t out_prefix[2] = { (uint8_t)(rlen >> 8), (uint8_t)(rlen & 0xFF) };
    off = 0;
    while (off < 2) {
        ssize_t p = write(cfd, out_prefix + off, 2 - off);
        if (p > 0) { off += (size_t)p; continue; }
        if (p < 0 && errno == EINTR) continue;
        close(cfd); return;
    }
    off = 0;
    while (off < rlen) {
        ssize_t p = write(cfd, rbuf + off, rlen - off);
        if (p > 0) { off += (size_t)p; continue; }
        if (p < 0 && errno == EINTR) continue;
        break;
    }
    close(cfd);
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr,
            "usage: %s <zonefile> [port]\n"
            "  authoritative DNS server. Default port 5353 (use 53 as root).\n"
            "example: %s zones/example.com.zone 5353\n", argv[0], argv[0]);
        return 2;
    }
    uint16_t port = (argc >= 3) ? (uint16_t)atoi(argv[2]) : 5353;

    zone z;
    if (zone_load(&z, argv[1]) < 0) {
        fprintf(stderr, "failed to load zone %s\n", argv[1]);
        return 1;
    }
    fprintf(stderr, "loaded zone '%s' with %d records; listening on port %u\n",
            z.origin[0] ? z.origin : ".", z.nrr, (unsigned)port);

    int ufd = make_socket(SOCK_DGRAM, port);
    int tfd = make_socket(SOCK_STREAM, port);
    if (ufd < 0 || tfd < 0) return 1;

    /* TCP must listen(); backlog 16 pending connections is plenty here. */
    if (listen(tfd, 16) < 0) { perror("listen"); return 1; }

    /* Event loop: block in poll() until either socket is readable, then handle
     * whichever fired. A UDP datagram is a whole query; a TCP readiness means a
     * pending connection to accept. This is single-threaded on purpose — the
     * README's "Going further" covers scaling it. */
    for (;;) {
        struct pollfd fds[2];
        fds[0].fd = ufd; fds[0].events = POLLIN; fds[0].revents = 0;
        fds[1].fd = tfd; fds[1].events = POLLIN; fds[1].revents = 0;

        int n = poll(fds, 2, -1 /* block forever */);
        if (n < 0) {
            if (errno == EINTR) continue;        /* a signal: just re-poll      */
            perror("poll");
            break;
        }

        /* --- UDP query --- */
        if (fds[0].revents & POLLIN) {
            uint8_t qbuf[DNS_MSG_MAX];
            struct sockaddr_storage cli;
            socklen_t clilen = sizeof(cli);
            /* recvfrom fills `cli` with the sender so we can reply to them. */
            ssize_t got = recvfrom(ufd, qbuf, sizeof(qbuf), 0,
                                   (struct sockaddr *)&cli, &clilen);
            if (got > 0) {
                uint8_t rbuf[DNS_MSG_MAX];
                size_t rlen = build_response(&z, qbuf, (size_t)got,
                                             rbuf, sizeof(rbuf),
                                             0 /* UDP */, DNS_UDP_CLASSIC_MAX);
                if (rlen > 0)
                    /* sendto the exact client address recvfrom gave us. */
                    sendto(ufd, rbuf, rlen, 0, (struct sockaddr *)&cli, clilen);
            } else if (got < 0 && errno != EINTR) {
                perror("recvfrom");
            }
        }

        /* --- TCP connection --- */
        if (fds[1].revents & POLLIN) {
            int cfd = accept(tfd, NULL, NULL);
            if (cfd >= 0) serve_tcp_client(cfd, &z);
            else if (errno != EINTR) perror("accept");
        }
    }

    close(ufd);
    close(tfd);
    return 0;
}
