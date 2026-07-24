# DNS resolver + authoritative server 🟧

**What it is.** A from-scratch implementation of the DNS wire protocol, with two
programs built on top of it: `resolve`, an **iterative resolver** that walks the
delegation tree from the root servers down (like `dig +trace`), honouring TTLs
with a cache; and `dnsd`, a small **authoritative server** that answers from a
BIND-style zone file. Every byte of the DNS message — the 12-byte header, the
length-prefixed labels, the `0xC0` **compression pointers**, the A/AAAA/NS/CNAME/
MX record types, and the EDNS0 OPT pseudo-record — is assembled and parsed by
hand in `src/wire.c`. Transport is UDP with an automatic **TCP fallback** on
truncation. This is a teaching *core*: correct and complete for the common path,
with the production-grade parts it omits called out honestly below.

## What you'll learn

- **The DNS message format** in full: header flags (QR/AA/TC/RD/RA/RCODE),
  the question and resource-record layout, and big-endian ("network byte order")
  encoding done by hand with shifts instead of trusting a struct cast.
- **Name compression** — the `0xC0` pointer scheme, why the two high bits were
  free to repurpose, and the two classic parser bugs (pointer loops and cursor
  desync) with the guards that stop them. This is the routine in `asm/demo.c`.
- **Iterative resolution**: root hints, following NS referrals, using glue vs.
  resolving nameserver names, CNAME chasing, and TTL-respecting caching.
- **The sockets/syscalls**: `socket`/`connect`/`send`/`recv` for UDP,
  `bind`/`listen`/`accept`/`read`/`write` with 2-byte length framing for TCP,
  `recvfrom`/`sendto` for the server, and `poll(2)` for timeouts — with the
  `EINTR`, `POLLERR/HUP`, partial-I/O, and truncation error paths handled.
- **EDNS0** (RFC 6891): how an OPT record smuggles a larger UDP buffer size and
  extended flags into a header that had no room left.

## Build & run (Linux / WSL)

```bash
make                 # builds ./resolve and ./dnsd  (clang -Wall -Wextra)
```

**Platform:** Linux (or WSL). The programs use POSIX sockets and `poll(2)`, so
they do **not** build on native Windows. No special privileges are needed for
the resolver or for the server on its default port 5353. Binding the server to
the real DNS port 53 needs root or the capability:

```bash
sudo setcap 'cap_net_bind_service=+ep' ./dnsd   # then ./dnsd ... 53
```

**Iterative resolver** (needs outbound UDP/53 to the Internet):

```bash
./resolve www.example.com A            # just the answer
./resolve +trace www.example.com A     # show every delegation hop (stderr)
./resolve example.com MX
./resolve example.com NS
```

**Authoritative server** for the sample zone:

```bash
make run                               # ./dnsd zones/example.com.zone 5353
# in another terminal, query it with the real dig:
dig @127.0.0.1 -p 5353 www.example.com A
dig @127.0.0.1 -p 5353 ftp.example.com A     # CNAME + target A
dig @127.0.0.1 -p 5353 example.com MX
dig @127.0.0.1 -p 5353 nope.example.com A    # NXDOMAIN with SOA
dig +tcp @127.0.0.1 -p 5353 example.com ANY  # force TCP
```

## How it works

The code is layered so each file teaches one idea:

- **`src/dns.h`** — the wire format as C: the header struct, every flag/type/
  rcode constant with its bit position, the length limits, and the two
  bounds-checked cursors (`dns_reader` for parsing, `dns_writer` for building).
  The header comment diagrams the five message sections.

- **`src/wire.c`** — the heart. Big-endian primitive reads/writes done by hand;
  `dns_read_name` (compression-pointer following with a loop guard and a
  correct caller-cursor restore); `dns_write_name`; header, question, and RR
  parsing (`dns_read_rr` decodes A/AAAA/NS/CNAME/MX/PTR, including names that
  are themselves compressed inside RDATA); `dns_build_query` and the EDNS0
  `dns_write_opt`; and `dns_parse_response`, which splits a message into its
  answer/authority/additional sections.

- **`src/net.c` / `net.h`** — transport. `dns_send_udp` (connected UDP + a
  `poll`-based timeout that correctly resumes after `EINTR`), `dns_send_tcp`
  (2-byte length framing with full read/write loops over the byte stream), and
  `dns_exchange`, which retries UDP and transparently falls back to TCP when a
  reply has the TC bit set.

- **`src/cache.c` / `cache.h`** — a fixed-size TTL cache. Records are stored
  with an absolute expiry computed from a **monotonic** clock (immune to
  wall-clock jumps), lookups skip and reap expired entries, and returned TTLs
  are decayed to the remaining lifetime.

- **`src/resolver.c`** — the iterative loop. Starts at the root hints, sends
  `RD=0` queries, and at each hop either returns the answer (chasing CNAMEs),
  stops on NXDOMAIN, or follows the NS referral downward — using glue addresses
  when present, otherwise resolving a nameserver's name recursively. Everything
  received is cached. Also the `resolve`/`dig`-style CLI.

- **`src/zone.c` / `zone.h`** — a BIND master-file parser: `$ORIGIN`/`$TTL`
  directives, `@` and relative/absolute names, owner inheritance, optional
  TTL/class, `( … )` multi-line RDATA (for SOA), and A/AAAA/NS/CNAME/MX/SOA
  records, normalised to lower-case FQDNs for O(n) lookups.

- **`src/server.c`** — the authoritative server. A `poll` loop over one UDP and
  one TCP socket; per query it builds a response from the zone (positive,
  CNAME-followed, NODATA, NXDOMAIN-with-SOA, or REFUSED), sets AA, echoes EDNS0,
  and **writes a compression pointer** (`0xC00C`) for answer owners that match
  the question — the encoder half of the decoder above.

### The resolution walk, concretely

`./resolve +trace www.example.com A` does, roughly:

```
ask a root (198.41.0.4) for www.example.com A, RD=0
  -> referral: .com NS + glue  (Authority + Additional sections)
ask a .com server for www.example.com A
  -> referral: example.com NS + glue
ask an example.com server for www.example.com A
  -> Answer: www.example.com A 93.184.216.34   (AA=1)   [cached by TTL]
```

## Assembly notes

`asm/demo.c` is a self-contained extraction of `dns_read_name` — the
compression-pointer-following decoder, with its own types and **no system
headers** — so its assembly is pure logic. Generate/refresh it with:

```bash
make asm     # or the three explicit clang -S commands in CONVENTIONS.md
```

The committed files are **genuine clang 20 output** (`x86_64-pc-linux-gnu`):
`demo.O0.s` (naive, literal C→asm mapping), `demo.s` (`-O1`, the annotated
baseline), and `demo.O2.s`. The hand-written **`asm/demo.annotated.s`** comments
essentially every instruction, with a header on the SysV AMD64 ABI.

The lesson this file makes visible: at `-O1` clang **did not** keep the loop's
`continue` / `break` (root label) / `return -1` (malformed) as ordinary jumps.
It *synthesised* a small integer "what to do next" code in `%r14d`
(`1`=error → return −1, `2`=break → return length, `0`/`3`=continue) and
dispatches on it — structured control flow turned into a **data value**. You can
also watch `cmov`/`setb`/`adc` compute the pointer-bounds and loop-guard
decisions branchlessly, and confirm that the security-critical checks (the
`++hops > max_hops` loop guard and every bounds test) survive optimisation
intact. `demo.O0.s` shows the same routine the naive way for contrast.

Note: the real source files (`wire.c`, `net.c`, …) all include system headers,
so per CONVENTIONS.md the self-contained `demo.c` is the annotated unit.

## Going further (the `Stretch:` goals)

- **Negative caching (RFC 2308).** Cache NXDOMAIN/NODATA for the SOA MINIMUM so
  repeated misses don't re-walk the tree. (We cache positive records only.)
- **DNSSEC validation.** Set the DO bit (the plumbing is in `dns_write_opt`),
  request RRSIG/DNSKEY/DS, and validate the chain from the root trust anchor.
- **QNAME minimisation (RFC 7816).** Ask each server only for the next label, so
  the root never sees the full name — a privacy win.
- **Robustness the core skips:** unpredictable query IDs **and** random source
  ports (anti-Kaminsky), nameserver RTT ranking, per-query concurrency in the
  server (it handles one TCP client at a time), wildcard (`*`) matching, AXFR
  zone transfers, and `$INCLUDE`/`$GENERATE` in the zone parser.
- **What production does:** read Unbound (validating iterative resolver), Knot
  Resolver, and the authoritative servers NSD/Knot DNS/BIND.

## References

- RFC 1035 — Domain Names, Implementation and Specification (the wire format,
  master files, and the UDP/TCP rules). RFC 1034 for concepts.
- RFC 6891 — Extension Mechanisms for DNS (EDNS0), the OPT record.
- RFC 2308 — Negative Caching of DNS Queries. RFC 7816 — QNAME minimisation.
- `man 2 socket`, `man 2 poll`, `man 2 recvfrom`, `man 7 ip`, `man 3 inet_pton`.
- The IANA root hints file (`named.root`) — the source of the addresses in
  `ROOT_HINTS[]` in `src/resolver.c`.
