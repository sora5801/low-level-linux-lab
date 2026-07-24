/* ===========================================================================
 * dns.h — the DNS wire format, spelled out byte by byte.
 * ===========================================================================
 *
 * DNS (RFC 1035, with EDNS0 from RFC 6891) is a *binary* protocol carried over
 * UDP port 53 (and TCP 53 for large answers). Everything in it is big-endian
 * ("network byte order"): the most-significant byte comes first on the wire.
 * We build and parse every field by hand so you can SEE the format — no library
 * hides the layout from you.
 *
 * A DNS message is five consecutive regions:
 *
 *     +---------------------+  offset 0
 *     |       Header        |  fixed 12 bytes
 *     +---------------------+  offset 12
 *     |      Question       |  QDCOUNT entries (usually exactly 1)
 *     +---------------------+
 *     |       Answer        |  ANCOUNT resource records (RRs)
 *     +---------------------+
 *     |      Authority      |  NSCOUNT RRs (NS referrals live here)
 *     +---------------------+
 *     |      Additional     |  ARCOUNT RRs (glue A/AAAA + the EDNS0 OPT)
 *     +---------------------+
 *
 * The counts in the header tell the parser how many records to read from each
 * section; the sections themselves carry no length, so you MUST trust and walk
 * them record by record. A single malformed length can walk you off the end of
 * the buffer, which is why every read below is bounds-checked.
 * ===========================================================================
 */
#ifndef DNS_H
#define DNS_H

#include <stdint.h>   /* uintN_t: fixed-width types matter for a wire format */
#include <stddef.h>   /* size_t                                             */

/* ---------------------------------------------------------------------------
 * The 12-byte header. On the wire it is six 16-bit big-endian words:
 *
 *   byte 0..1 : ID       — a 16-bit query identifier, echoed in the reply so a
 *                          resolver can match answers to outstanding questions.
 *   byte 2..3 : flags    — a packed bitfield (see below).
 *   byte 4..5 : QDCOUNT  — number of entries in the Question section.
 *   byte 6..7 : ANCOUNT  — number of RRs in the Answer section.
 *   byte 8..9 : NSCOUNT  — number of RRs in the Authority section.
 *   byte 10..11: ARCOUNT — number of RRs in the Additional section.
 *
 * We keep the header in *host* integers (this struct) and convert to/from the
 * wire representation explicitly in wire.c — never by memcpy'ing this struct,
 * because struct padding and host endianness would corrupt the layout.
 * ------------------------------------------------------------------------- */
typedef struct {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
} dns_header;

/* The flags word (byte 2..3), most-significant bit first:
 *
 *   bit 15    QR      0 = query, 1 = response
 *   bit 11-14 Opcode  0 = standard query (QUERY); we only speak QUERY
 *   bit 10    AA      Authoritative Answer (set by an authoritative server)
 *   bit 9     TC      TruncateD — the answer did not fit in the UDP datagram;
 *                     the client must retry over TCP. This bit drives our
 *                     "UDP with TCP fallback" logic.
 *   bit 8     RD      Recursion Desired (client asks the server to recurse).
 *                     An ITERATIVE resolver sends RD=0 to root/TLD servers.
 *   bit 7     RA      Recursion Available (server advertises it will recurse)
 *   bit 6     Z       reserved, must be 0
 *   bit 5     AD      Authentic Data (DNSSEC); we do not validate, so we ignore
 *   bit 4     CD      Checking Disabled (DNSSEC); ignored here
 *   bit 3-0   RCODE   response code (0 = NOERROR, see DNS_RCODE_* below)
 *
 * We expose each as a shift/mask so the bit twiddling reads like the RFC. */
#define DNS_FLAG_QR      (1u << 15)     /* query/response bit                  */
#define DNS_FLAG_AA      (1u << 10)     /* authoritative answer                */
#define DNS_FLAG_TC      (1u << 9)      /* truncated -> fall back to TCP       */
#define DNS_FLAG_RD      (1u << 8)      /* recursion desired                   */
#define DNS_FLAG_RA      (1u << 7)      /* recursion available                 */

#define DNS_OPCODE_SHIFT 11             /* opcode occupies bits 11..14         */
#define DNS_OPCODE_MASK  0x0Fu
#define DNS_OPCODE_QUERY 0u

#define DNS_RCODE_MASK   0x0Fu          /* rcode occupies the low 4 bits       */
#define DNS_RCODE_NOERROR  0            /* success                             */
#define DNS_RCODE_FORMERR  1            /* the server could not parse our query*/
#define DNS_RCODE_SERVFAIL 2            /* server failed / upstream failure    */
#define DNS_RCODE_NXDOMAIN 3            /* the name definitively does not exist*/
#define DNS_RCODE_NOTIMP   4            /* opcode not implemented              */
#define DNS_RCODE_REFUSED  5            /* server refuses (policy)             */

/* Resource-record TYPE values (a 16-bit field). These identify what the RDATA
 * of a record means. We implement the handful the spec asks for. */
#define DNS_TYPE_A       1     /* IPv4 address, RDATA = 4 raw bytes            */
#define DNS_TYPE_NS      2     /* authoritative name server, RDATA = a name    */
#define DNS_TYPE_CNAME   5     /* canonical name (alias), RDATA = a name       */
#define DNS_TYPE_SOA     6     /* start of authority (zone apex metadata)      */
#define DNS_TYPE_PTR     12    /* reverse lookup, RDATA = a name               */
#define DNS_TYPE_MX      15    /* mail exchange: RDATA = 16-bit pref + a name  */
#define DNS_TYPE_TXT     16    /* text, RDATA = length-prefixed strings        */
#define DNS_TYPE_AAAA    28    /* IPv6 address, RDATA = 16 raw bytes           */
#define DNS_TYPE_OPT     41    /* EDNS0 pseudo-RR (RFC 6891), not real data    */

/* CLASS field. Everything on the public Internet is class IN ("Internet"). */
#define DNS_CLASS_IN     1

/* Name/label limits from RFC 1035 §2.3.4 — enforced to bound our buffers and
 * to reject hostile packets that would otherwise blow the stack. */
#define DNS_MAX_NAME     255   /* a domain name is at most 255 octets on wire  */
#define DNS_MAX_LABEL    63    /* a single label is at most 63 octets ...      */
/* ... which is *why* a length byte >= 0xC0 cannot be a label length: 0xC0 = 192
 * exceeds 63, so the two high bits (0b11) were free to repurpose as the
 * "this is a compression pointer" marker. See wire.c dns_read_name(). */
#define DNS_LABEL_PTR_MASK 0xC0   /* top two bits set => 14-bit offset pointer */

/* The maximum classic-DNS UDP payload is 512 bytes (RFC 1035). EDNS0 lets us
 * advertise a bigger reassembly buffer; 1232 is the modern safe default that
 * avoids IP fragmentation on most paths (DNS Flag Day 2020 guidance). */
#define DNS_UDP_CLASSIC_MAX 512
#define DNS_EDNS_UDP_SIZE   1232

/* A hard cap on any DNS message we build or accept. 64 KiB is the TCP limit
 * (the TCP framing uses a 16-bit length prefix), so nothing legal exceeds it. */
#define DNS_MSG_MAX  65535

/* ===========================================================================
 * A bounds-checked cursor over an immutable message buffer (the READER).
 *
 * Parsing DNS safely is 90% "did this read stay inside the buffer?" We funnel
 * every read through this cursor so a single malformed length can never walk us
 * off the end. `base` is the START of the whole message because compression
 * pointers are offsets from byte 0, not from the current position.
 * =========================================================================== */
typedef struct {
    const uint8_t *base;   /* &message[0]; pointer offsets are relative to this */
    size_t         len;    /* total number of valid bytes in the message        */
    size_t         pos;    /* current read cursor (0..len)                       */
} dns_reader;

/* ===========================================================================
 * A bounds-checked append buffer (the WRITER).
 *
 * `overflow` is a *sticky* error flag: once a write would exceed `cap` we set
 * it and make all further writes no-ops. Callers check it once at the end
 * instead of after every field, so the building code stays readable while
 * remaining safe. `len` is the number of bytes committed so far.
 * =========================================================================== */
typedef struct {
    uint8_t *buf;
    size_t   cap;
    size_t   len;
    int      overflow;     /* 1 once a write was truncated; sticky              */
} dns_writer;

/* ---- reader/writer construction ---------------------------------------- */
void dns_reader_init(dns_reader *r, const uint8_t *buf, size_t len);
void dns_writer_init(dns_writer *w, uint8_t *buf, size_t cap);

/* ---- primitive big-endian reads (return 0 on success, -1 on overrun) ---- */
int dns_read_u8 (dns_reader *r, uint8_t  *out);
int dns_read_u16(dns_reader *r, uint16_t *out);
int dns_read_u32(dns_reader *r, uint32_t *out);

/* ---- primitive big-endian writes (no-op after overflow) ---------------- */
void dns_write_u8 (dns_writer *w, uint8_t  v);
void dns_write_u16(dns_writer *w, uint16_t v);
void dns_write_u32(dns_writer *w, uint32_t v);
void dns_write_bytes(dns_writer *w, const void *p, size_t n);

/* ---------------------------------------------------------------------------
 * Names: encode/decode with length-prefixed labels and 0xC0 compression.
 *
 * dns_read_name: decode a possibly-compressed name at the cursor into a
 *   presentation string ("www.example.com"). Advances the cursor to just past
 *   the name AS IT APPEARS at the cursor (i.e. past the 2-byte pointer, if the
 *   name is compressed) — NOT to wherever the pointer jumped. Returns 0 on
 *   success, -1 on malformed input (bad length, out-of-bounds, or a pointer
 *   loop caught by the guard). `out` must hold at least DNS_MAX_NAME+1 bytes.
 *
 * dns_write_name: encode a dotted name as length-prefixed labels terminated by
 *   a zero byte. Uncompressed. Sets the writer's overflow flag if it won't fit.
 * ------------------------------------------------------------------------- */
int  dns_read_name(dns_reader *r, char *out, size_t out_cap);
void dns_write_name(dns_writer *w, const char *name);

/* ---- header pack/unpack ------------------------------------------------- */
void dns_write_header(dns_writer *w, const dns_header *h);
int  dns_read_header (dns_reader *r, dns_header *h);

/* ---------------------------------------------------------------------------
 * A decoded resource record. RDATA interpretation is type-specific; we keep
 * the raw slice plus decoded convenience fields for the types we understand.
 * The `name` and any embedded names are already decompressed to presentation
 * form. `rdata`/`rdlen` point INTO the original message buffer (no copy), so
 * the record is only valid while that buffer lives.
 * ------------------------------------------------------------------------- */
typedef struct {
    char           name[DNS_MAX_NAME + 1];
    uint16_t       type;
    uint16_t       rclass;
    uint32_t       ttl;
    uint16_t       rdlen;
    const uint8_t *rdata;               /* borrowed slice of the message       */

    /* Decoded views for the types we implement (valid per `type`):           */
    uint8_t        addr4[4];            /* TYPE_A    : the IPv4 address         */
    uint8_t        addr6[16];           /* TYPE_AAAA : the IPv6 address         */
    char           target[DNS_MAX_NAME + 1]; /* NS/CNAME/MX/PTR: the name in RDATA */
    uint16_t       mx_pref;             /* TYPE_MX   : preference (lower first) */
} dns_rr;

/* Decode one question (QNAME/QTYPE/QCLASS). Returns 0 / -1. */
int dns_read_question(dns_reader *r, char *qname, size_t qname_cap,
                      uint16_t *qtype, uint16_t *qclass);

/* Decode one resource record starting at the cursor. Returns 0 / -1. */
int dns_read_rr(dns_reader *r, dns_rr *rr);

/* ---------------------------------------------------------------------------
 * dns_build_query — assemble a complete query message into `buf`.
 *
 *   id        : the 16-bit transaction ID (echoed in the reply; pick randomly)
 *   qname     : dotted name to ask about ("" = root)
 *   qtype     : DNS_TYPE_A / _AAAA / _NS / _MX / ...
 *   rd        : set Recursion Desired (1 for a stub asking a recursor; 0 for
 *               iterative queries we send to root/TLD/authoritative servers)
 *   edns      : if nonzero, append an EDNS0 OPT record (RFC 6891) in the
 *               additional section advertising `udp_size` as our reassembly
 *               buffer — this is how you legally receive UDP answers > 512 B.
 *   udp_size  : advertised EDNS0 UDP payload size (e.g. DNS_EDNS_UDP_SIZE)
 *
 * Returns the message length in bytes, or 0 if it did not fit in `cap`.
 * ------------------------------------------------------------------------- */
size_t dns_build_query(uint8_t *buf, size_t cap, uint16_t id, const char *qname,
                       uint16_t qtype, int rd, int edns, uint16_t udp_size);

/* Append an EDNS0 OPT pseudo-RR to a writer (used by both queries and the
 * server's responses). `do_bit` requests DNSSEC records; we pass 0. */
void dns_write_opt(dns_writer *w, uint16_t udp_size, int do_bit);

/* ---------------------------------------------------------------------------
 * A fully-parsed response, split into its three record sections. Records
 * beyond the fixed capacities are parsed (to keep the cursor in sync) but not
 * stored — ample for teaching; a production parser would grow these.
 * ------------------------------------------------------------------------- */
#define DNS_MAX_RRS 32

typedef struct {
    dns_header h;
    char       qname[DNS_MAX_NAME + 1];    /* the echoed question name          */
    uint16_t   qtype;
    uint16_t   qclass;
    dns_rr     an[DNS_MAX_RRS];  int nan;   /* Answer section                    */
    dns_rr     ns[DNS_MAX_RRS];  int nns;   /* Authority section (NS referrals)  */
    dns_rr     ar[DNS_MAX_RRS];  int nar;   /* Additional section (glue + OPT)   */
} dns_response;

/* Parse a whole message into `out`. Returns 0 on success, -1 if malformed. */
int dns_parse_response(const uint8_t *buf, size_t len, dns_response *out);

#endif /* DNS_H */
