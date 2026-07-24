/* ===========================================================================
 * wire.c — encode and decode the DNS wire format by hand.
 * ===========================================================================
 *
 * This file is the whole point of the project: it turns C structures into the
 * exact big-endian bytes DNS puts on the wire, and back again, with every
 * length bounds-checked. The trickiest part — name compression pointers — is
 * extracted verbatim into asm/demo.c so you can read its assembly.
 *
 * ENDIANNESS. DNS is big-endian ("network byte order"): the high byte first.
 * x86-64 is little-endian, so we cannot just cast a uint16_t* at the buffer —
 * that would read the bytes swapped. We assemble/disassemble each multi-byte
 * field one byte at a time with explicit shifts, which is correct on ANY host
 * regardless of its native endianness. That is exactly what htons()/ntohs()
 * do; doing it by hand makes the byte order impossible to get wrong silently.
 * ===========================================================================
 */
#include "dns.h"
#include <string.h>    /* memcpy, strlen, memchr — pure libc, no networking    */

/* ---------------------------------------------------------------------------
 * Reader/writer construction.
 * ------------------------------------------------------------------------- */
void dns_reader_init(dns_reader *r, const uint8_t *buf, size_t len)
{
    r->base = buf;
    r->len  = len;
    r->pos  = 0;
}

void dns_writer_init(dns_writer *w, uint8_t *buf, size_t cap)
{
    w->buf      = buf;
    w->cap      = cap;
    w->len      = 0;
    w->overflow = 0;
}

/* ---------------------------------------------------------------------------
 * Primitive big-endian reads. Each checks that the field fits before touching
 * memory, so a caller can read a whole message without any read escaping the
 * buffer. Returns 0 on success, -1 if the read would run past the end.
 * ------------------------------------------------------------------------- */
int dns_read_u8(dns_reader *r, uint8_t *out)
{
    if (r->pos + 1 > r->len) return -1;        /* one byte must remain         */
    *out = r->base[r->pos];
    r->pos += 1;
    return 0;
}

int dns_read_u16(dns_reader *r, uint16_t *out)
{
    if (r->pos + 2 > r->len) return -1;        /* need two bytes               */
    /* High byte first (network order), then low byte. Shifting into a uint16_t
     * is host-endianness-independent: we build the number, not reinterpret it. */
    *out = (uint16_t)((r->base[r->pos] << 8) | r->base[r->pos + 1]);
    r->pos += 2;
    return 0;
}

int dns_read_u32(dns_reader *r, uint32_t *out)
{
    if (r->pos + 4 > r->len) return -1;        /* need four bytes              */
    *out = ((uint32_t)r->base[r->pos]     << 24) |
           ((uint32_t)r->base[r->pos + 1] << 16) |
           ((uint32_t)r->base[r->pos + 2] <<  8) |
           ((uint32_t)r->base[r->pos + 3]);
    r->pos += 4;
    return 0;
}

/* ---------------------------------------------------------------------------
 * Primitive big-endian writes. After overflow they become no-ops so callers
 * can build a whole message and check `w->overflow` exactly once at the end.
 * ------------------------------------------------------------------------- */
void dns_write_u8(dns_writer *w, uint8_t v)
{
    if (w->overflow) return;
    if (w->len + 1 > w->cap) { w->overflow = 1; return; }
    w->buf[w->len++] = v;
}

void dns_write_u16(dns_writer *w, uint16_t v)
{
    if (w->overflow) return;
    if (w->len + 2 > w->cap) { w->overflow = 1; return; }
    w->buf[w->len++] = (uint8_t)(v >> 8);      /* high byte first              */
    w->buf[w->len++] = (uint8_t)(v & 0xFF);    /* then low byte                */
}

void dns_write_u32(dns_writer *w, uint32_t v)
{
    if (w->overflow) return;
    if (w->len + 4 > w->cap) { w->overflow = 1; return; }
    w->buf[w->len++] = (uint8_t)(v >> 24);
    w->buf[w->len++] = (uint8_t)(v >> 16);
    w->buf[w->len++] = (uint8_t)(v >> 8);
    w->buf[w->len++] = (uint8_t)(v & 0xFF);
}

void dns_write_bytes(dns_writer *w, const void *p, size_t n)
{
    if (w->overflow) return;
    if (w->len + n > w->cap) { w->overflow = 1; return; }
    memcpy(w->buf + w->len, p, n);
    w->len += n;
}

/* ===========================================================================
 * dns_read_name — decode a (possibly compressed) domain name.
 * ===========================================================================
 *
 * A name on the wire is a sequence of LABELS. Each label is one length byte
 * followed by that many raw bytes:
 *
 *     3 'w' 'w' 'w' 7 'e' 'x' 'a' 'm' 'p' 'l' 'e' 3 'c' 'o' 'm' 0
 *     \--- label ---/ \-------- label --------/ \-- label --/ ^root (len 0)
 *
 * A length byte of 0 ends the name (it is the empty root label). To save space,
 * DNS also allows a COMPRESSION POINTER: if the top two bits of a length byte
 * are set (byte & 0xC0 == 0xC0), then that byte plus the next form a 14-bit
 * offset from the start of the message, and decoding CONTINUES from there:
 *
 *     0xC0 0x0C   ->  "jump to offset 12 and keep reading labels"
 *
 * This is what lets an answer say "the name is whatever was at offset 12"
 * (usually the question name) in two bytes. Two hazards make this the classic
 * DNS parser bug:
 *
 *   1. A pointer can point ANYWHERE, including forward or at itself, forming an
 *      infinite loop. We guard with a jump counter capped at the message length
 *      — a legal name follows at most len/2 pointers, so exceeding that means a
 *      loop, and we reject the packet.
 *   2. After following a pointer, the cursor we RETURN to the caller must sit
 *      just past the FIRST pointer we encountered, not deep in the region we
 *      jumped to. We therefore snapshot the position of the first pointer and
 *      restore the caller's cursor to two bytes past it.
 *
 * `out` receives a presentation-format string ("www.example.com", "" = root)
 * and is always NUL-terminated on success. Returns 0 on success, -1 on any
 * malformed input.
 * =========================================================================== */
int dns_read_name(dns_reader *r, char *out, size_t out_cap)
{
    size_t pos       = r->pos;    /* our private walk cursor (may jump around) */
    size_t out_len   = 0;         /* bytes written to `out` so far             */
    int    jumped    = 0;         /* have we followed at least one pointer?    */
    size_t first_ptr = 0;         /* where the caller's cursor should resume   */
    /* Loop guard: a name may follow at most this many pointers before we call
     * it a loop. Bounding by the message length is a safe over-estimate — no
     * acyclic pointer chain can be longer than the buffer. */
    unsigned jumps   = 0;
    const unsigned max_jumps = (unsigned)r->len + 1;

    if (out_cap == 0) return -1;
    out[0] = '\0';

    for (;;) {
        if (pos >= r->len) return -1;          /* cursor left the buffer       */
        uint8_t lenb = r->base[pos];

        if ((lenb & DNS_LABEL_PTR_MASK) == DNS_LABEL_PTR_MASK) {
            /* ---- compression pointer (0b11xxxxxx xxxxxxxx) ---------------- */
            if (pos + 2 > r->len) return -1;   /* pointer needs a second byte  */
            /* 14-bit offset: low 6 bits of this byte are the high bits of the
             * offset, next byte is the low 8 bits. Mask off the two flag bits. */
            size_t target = (size_t)((lenb & 0x3F) << 8) | r->base[pos + 1];

            if (!jumped) {
                /* Remember where the on-the-wire name ended for the caller:
                 * the pointer is two bytes, so resume just after them. */
                first_ptr = pos + 2;
                jumped = 1;
            }
            if (++jumps > max_jumps) return -1; /* too many hops => a loop      */
            if (target >= r->len) return -1;    /* pointer must stay in bounds  */
            pos = target;                       /* continue decoding there      */
            continue;
        }

        if ((lenb & DNS_LABEL_PTR_MASK) != 0) {
            /* Top bits 0b10 or 0b01 are reserved and never valid label lengths. */
            return -1;
        }

        if (lenb == 0) {                       /* the root label ends the name */
            pos += 1;
            break;
        }

        /* ---- an ordinary label of `lenb` bytes -------------------------- */
        if (lenb > DNS_MAX_LABEL) return -1;   /* labels are <= 63 octets      */
        if (pos + 1 + lenb > r->len) return -1;/* label body must be in bounds */

        /* Presentation form joins labels with '.'; add one before every label
         * except the first. Enforce the 255-octet name limit as we go so a
         * crafted packet cannot overflow `out`. */
        if (out_len != 0) {
            if (out_len + 1 >= out_cap) return -1;
            out[out_len++] = '.';
        }
        if (out_len + lenb >= out_cap) return -1;
        if (out_len + lenb > DNS_MAX_NAME) return -1;
        memcpy(out + out_len, r->base + pos + 1, lenb);
        out_len += lenb;

        pos += 1 + lenb;                       /* advance past length + label  */
    }

    out[out_len] = '\0';

    /* Advance the CALLER's cursor. If we ever jumped, the name occupied only
     * up to the first pointer (2 bytes); otherwise it ran to `pos` inline. */
    r->pos = jumped ? first_ptr : pos;
    return 0;
}

/* ===========================================================================
 * dns_write_name — encode a dotted name as uncompressed length-prefixed labels.
 * ===========================================================================
 * We never emit compression pointers here (queries are tiny and the extra code
 * would obscure the format). The server uses a dedicated 2-byte pointer to the
 * question name instead; see server.c. An empty string encodes the root: a
 * single zero byte.
 * =========================================================================== */
void dns_write_name(dns_writer *w, const char *name)
{
    const char *p = name;

    /* Walk label by label. `dot` marks the end of the current label. */
    while (*p) {
        const char *dot = strchr(p, '.');
        size_t label_len = dot ? (size_t)(dot - p) : strlen(p);

        if (label_len == 0 || label_len > DNS_MAX_LABEL) {
            /* An empty label (e.g. "a..b") or an over-long one is illegal.
             * Mark the writer bad so the caller aborts building the message. */
            w->overflow = 1;
            return;
        }
        dns_write_u8(w, (uint8_t)label_len);        /* the length prefix       */
        dns_write_bytes(w, p, label_len);           /* the label bytes         */

        if (!dot) break;                            /* last label, no trailing */
        p = dot + 1;                                /* skip the '.'            */
    }
    dns_write_u8(w, 0);                             /* root label terminates   */
}

/* ---------------------------------------------------------------------------
 * Header pack/unpack. Six 16-bit words, big-endian, no surprises.
 * ------------------------------------------------------------------------- */
void dns_write_header(dns_writer *w, const dns_header *h)
{
    dns_write_u16(w, h->id);
    dns_write_u16(w, h->flags);
    dns_write_u16(w, h->qdcount);
    dns_write_u16(w, h->ancount);
    dns_write_u16(w, h->nscount);
    dns_write_u16(w, h->arcount);
}

int dns_read_header(dns_reader *r, dns_header *h)
{
    /* '||' short-circuits: the first failing read stops the rest and returns
     * the error, so we never read a field that isn't fully present. */
    if (dns_read_u16(r, &h->id)      ||
        dns_read_u16(r, &h->flags)   ||
        dns_read_u16(r, &h->qdcount) ||
        dns_read_u16(r, &h->ancount) ||
        dns_read_u16(r, &h->nscount) ||
        dns_read_u16(r, &h->arcount))
        return -1;
    return 0;
}

/* ---------------------------------------------------------------------------
 * Read one question: QNAME, then QTYPE and QCLASS (each 16 bits).
 * ------------------------------------------------------------------------- */
int dns_read_question(dns_reader *r, char *qname, size_t qname_cap,
                      uint16_t *qtype, uint16_t *qclass)
{
    if (dns_read_name(r, qname, qname_cap)) return -1;
    if (dns_read_u16(r, qtype))             return -1;
    if (dns_read_u16(r, qclass))            return -1;
    return 0;
}

/* ===========================================================================
 * dns_read_rr — decode one resource record.
 * ===========================================================================
 * Layout:  NAME  TYPE(16)  CLASS(16)  TTL(32)  RDLENGTH(16)  RDATA[RDLENGTH]
 *
 * The subtlety is RDATA. For NS/CNAME/MX/PTR the RDATA itself contains a name
 * that MAY use compression pointing back earlier in the message — so we decode
 * it with the same pointer-following reader. We remember where RDATA ends and
 * hard-set the cursor there afterward, so a compressed name inside RDATA (whose
 * inline bytes are fewer than RDLENGTH) can't desynchronize the outer walk.
 * =========================================================================== */
int dns_read_rr(dns_reader *r, dns_rr *rr)
{
    memset(rr, 0, sizeof(*rr));

    if (dns_read_name(r, rr->name, sizeof(rr->name))) return -1;
    if (dns_read_u16(r, &rr->type))    return -1;
    if (dns_read_u16(r, &rr->rclass))  return -1;
    if (dns_read_u32(r, &rr->ttl))     return -1;
    if (dns_read_u16(r, &rr->rdlen))   return -1;

    /* RDATA must lie entirely within the message. */
    if (r->pos + rr->rdlen > r->len) return -1;
    rr->rdata = r->base + r->pos;                  /* borrow, don't copy       */
    size_t rdata_end = r->pos + rr->rdlen;         /* where the RR truly ends  */

    /* Type-specific decoding. We build a sub-reader over the FULL message so
     * embedded compression pointers (which are message-relative) resolve, but
     * only advance the outer cursor to rdata_end at the end. */
    switch (rr->type) {
    case DNS_TYPE_A:
        if (rr->rdlen != 4) return -1;             /* an A record is exactly 4 */
        memcpy(rr->addr4, rr->rdata, 4);
        break;

    case DNS_TYPE_AAAA:
        if (rr->rdlen != 16) return -1;            /* an AAAA record is 16     */
        memcpy(rr->addr6, rr->rdata, 16);
        break;

    case DNS_TYPE_NS:
    case DNS_TYPE_CNAME:
    case DNS_TYPE_PTR: {
        /* RDATA is a single (possibly compressed) name. */
        dns_reader sub = *r;                       /* copy: base/len shared    */
        if (dns_read_name(&sub, rr->target, sizeof(rr->target))) return -1;
        break;
    }

    case DNS_TYPE_MX: {
        /* RDATA = 16-bit preference, then a (possibly compressed) exchange. */
        dns_reader sub = *r;
        if (dns_read_u16(&sub, &rr->mx_pref)) return -1;
        if (dns_read_name(&sub, rr->target, sizeof(rr->target))) return -1;
        break;
    }

    default:
        /* Types we don't specially decode (SOA, TXT, OPT, ...) keep only the
         * raw rdata slice. That is enough for the resolver's needs. */
        break;
    }

    r->pos = rdata_end;    /* authoritative: skip exactly RDLENGTH bytes       */
    return 0;
}

/* ===========================================================================
 * dns_write_opt — the EDNS0 OPT pseudo-RR (RFC 6891 §6.1).
 * ===========================================================================
 * EDNS0 smuggles extra capabilities into the DNS header by repurposing an RR
 * in the additional section. It looks like a normal RR but its fields mean
 * something else entirely:
 *
 *   NAME     = 0            the root (empty) name — OPT is not about any name
 *   TYPE     = 41 (OPT)
 *   CLASS    = 4096         REPURPOSED: the sender's UDP reassembly buffer size.
 *                           This is the whole reason EDNS0 exists — vanilla DNS
 *                           caps UDP answers at 512 bytes, and advertising e.g.
 *                           1232 here says "you may send me a bigger datagram."
 *   TTL      = 32-bit field REPURPOSED as: extended-RCODE(8) | version(8) |
 *                           DO(1) | Z(15). We set version 0 and, optionally,
 *                           the DO bit to request DNSSEC records.
 *   RDLENGTH = 0            no options (we send none)
 * =========================================================================== */
void dns_write_opt(dns_writer *w, uint16_t udp_size, int do_bit)
{
    dns_write_u8(w, 0);                    /* NAME = root (single zero byte)    */
    dns_write_u16(w, DNS_TYPE_OPT);        /* TYPE = 41                         */
    dns_write_u16(w, udp_size);            /* CLASS = advertised UDP payload    */
    /* TTL field: ext-rcode(bits 24-31)=0, version(bits 16-23)=0, then flags.
     * The DO ("DNSSEC OK") bit is bit 15 of the low 16 bits. */
    uint32_t ttl = do_bit ? (1u << 15) : 0u;
    dns_write_u32(w, ttl);
    dns_write_u16(w, 0);                   /* RDLENGTH = 0 (no EDNS options)    */
}

/* ---------------------------------------------------------------------------
 * Read one section of `count` RRs. Store up to `cap` into `dst`, but ALWAYS
 * parse every record so the cursor stays aligned for the next section (an RR we
 * skip still occupies bytes we must step over). Returns 0 / -1.
 * ------------------------------------------------------------------------- */
static int read_section(dns_reader *r, uint16_t count,
                        dns_rr *dst, int cap, int *nstored)
{
    int stored = 0;
    for (uint16_t i = 0; i < count; i++) {
        dns_rr tmp;
        dns_rr *slot = (stored < cap) ? &dst[stored] : &tmp;
        if (dns_read_rr(r, slot)) return -1;   /* malformed record -> reject   */
        if (stored < cap) stored++;
    }
    *nstored = stored;
    return 0;
}

/* ===========================================================================
 * dns_parse_response — header + question(s) + the three RR sections.
 * =========================================================================== */
int dns_parse_response(const uint8_t *buf, size_t len, dns_response *out)
{
    dns_reader r;
    dns_reader_init(&r, buf, len);
    memset(out, 0, sizeof(*out));

    if (dns_read_header(&r, &out->h)) return -1;

    /* Read the question section. There is normally exactly one question; we
     * keep the first and parse-and-discard any extras to stay in sync. */
    for (uint16_t i = 0; i < out->h.qdcount; i++) {
        char     qn[DNS_MAX_NAME + 1];
        uint16_t qt, qc;
        if (dns_read_question(&r, qn, sizeof(qn), &qt, &qc)) return -1;
        if (i == 0) {
            memcpy(out->qname, qn, sizeof(qn));
            out->qtype  = qt;
            out->qclass = qc;
        }
    }

    /* The three RR sections, in wire order. */
    if (read_section(&r, out->h.ancount, out->an, DNS_MAX_RRS, &out->nan)) return -1;
    if (read_section(&r, out->h.nscount, out->ns, DNS_MAX_RRS, &out->nns)) return -1;
    if (read_section(&r, out->h.arcount, out->ar, DNS_MAX_RRS, &out->nar)) return -1;
    return 0;
}

/* ===========================================================================
 * dns_build_query — assemble a full query message (header + question + OPT).
 * =========================================================================== */
size_t dns_build_query(uint8_t *buf, size_t cap, uint16_t id, const char *qname,
                       uint16_t qtype, int rd, int edns, uint16_t udp_size)
{
    dns_writer w;
    dns_writer_init(&w, buf, cap);

    dns_header h;
    h.id      = id;
    /* Opcode QUERY (0) with only RD optionally set. Everything else is 0: we
     * are asking, not answering (QR=0), and we are not authoritative. */
    h.flags   = rd ? DNS_FLAG_RD : 0;
    h.qdcount = 1;                         /* exactly one question              */
    h.ancount = 0;
    h.nscount = 0;
    h.arcount = edns ? 1 : 0;              /* the OPT RR lives in additional    */
    dns_write_header(&w, &h);

    /* The question: QNAME, QTYPE, QCLASS=IN. */
    dns_write_name(&w, qname);
    dns_write_u16(&w, qtype);
    dns_write_u16(&w, DNS_CLASS_IN);

    if (edns)
        dns_write_opt(&w, udp_size, 0 /* no DNSSEC */);

    /* If any write overflowed the caller's buffer the message is incomplete
     * and must not be sent. Report 0 so the caller enlarges the buffer. */
    if (w.overflow) return 0;
    return w.len;
}
