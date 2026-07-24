# A port scanner (raw-socket SYN / half-open) 🟧

**What it is.** A TCP **SYN (half-open) port scanner** that builds its own IP and
TCP headers, hands them to a **raw socket** opened with **`IP_HDRINCL`**, and reads
replies *below* the kernel's TCP state machine to classify each port —
**SYN|ACK = open, RST = closed, silence = filtered**. It computes the TCP checksum
over the **pseudo-header** by hand, paces sending with a **rate limiter**, and
falls back to an unprivileged **`connect()` scan** when you can't (or don't want to)
use raw sockets. It is a **teaching core**: one host, IPv4, no options/payload,
no retransmission — but the packet-crafting, checksum, and classification paths are
complete and correct.

> ⚠ **Authorized targets only.** Port scanning hosts you do not own or lack written
> permission to test is illegal in many places and violates essentially every ISP
> and cloud acceptable-use policy. Use `127.0.0.1`, machines you own, or an
> explicitly sanctioned lab. The demo target is your own loopback.

## What you'll learn

- **Raw sockets** — `socket(AF_INET, SOCK_RAW, IPPROTO_TCP)`: writing TCP by hand and
  receiving a copy of *every* inbound TCP segment, below the kernel's socket layer.
- **`IP_HDRINCL`** — the `setsockopt` that says "I supply the IP header myself", and
  exactly which fields the Linux kernel still rewrites for you (`raw(7)`).
- **The Internet checksum** (RFC 1071) — the ones-complement 16-bit sum, carry
  folding, and *why* it's endianness-independent when you read words big-endian.
- **The TCP pseudo-header** — why TCP's checksum must also cover the IP addresses,
  protocol, and length, and how you construct the 12-byte fiction to do it.
- **Network byte order** — `htons`/`htonl`/`ntohs`, where each conversion happens,
  and what it compiles to (one `rol`/`bswap`; see the assembly notes).
- **`SO_RCVTIMEO`** and **non-blocking `connect()` + `select()`** — two ways to put a
  deadline on "wait for a reply", and the `EAGAIN`/`EWOULDBLOCK`/`EINPROGRESS`/
  `EINTR`/`SO_ERROR` error paths that make them work.
- **Response classification** — parsing a raw datagram safely (bounds-checking a
  possibly-hostile `IHL`), matching it to your probe, and reading the TCP flags.

## Build & run (Linux / WSL)

This is **Linux-only** — it relies on Linux raw-socket semantics and `IP_HDRINCL`.
On Windows use WSL2. (The teaching assembly in `asm/` regenerates on any host.)

```bash
make                 # builds ./synscan

# Unprivileged connect() scan of your own loopback — works as any user:
make demo
./synscan -sT -p 1-1024 127.0.0.1

# The real SYN (half-open) scan needs raw-packet privilege. Either:
sudo ./synscan -sS -p 1-1024 127.0.0.1
# ...or grant just the one capability (CAP_NET_RAW, far less than full root):
make cap             # runs: sudo setcap cap_net_raw+ep ./synscan
./synscan -sS -p 1-1024 127.0.0.1
```

Options:

```
-p <spec>   ports: "80", "1-1024", "22,80,443", "1-100,443"   (default 1-1024)
-sS         SYN half-open scan (default; needs CAP_NET_RAW/root)
-sT         TCP connect() scan (no privileges)
-r <pps>    max probes per second, 0 = unlimited              (default 1000)
-w <ms>     wait for replies, milliseconds                    (default 1000)
--open      print only open ports
```

Watch what actually goes on the wire while you scan:

```bash
sudo tcpdump -ni lo 'tcp'      # you'll see your SYN, the target's SYN/ACK or RST,
                               # and — for -sS — the kernel's own RST (see below)
```

## How it works

**`synscan.c`** is one file, built bottom-up; read it in section order:

- **§1 Headers.** Our own `__attribute__((packed))` `struct ip4_hdr` / `struct
  tcp_hdr`, with every byte offset in the comments, so the wire layout is explicit
  and we dodge `<netinet/*>`'s endian-dependent version/IHL bitfields.
- **§2 Checksum.** `sum16` (ones-complement accumulate), `fold_csum` (end-around
  carry + complement), `ip_checksum`, and `tcp_checksum` (pseudo-header **then**
  segment, one running sum). Words are read big-endian so the result is correct on
  any host. This is the code extracted into `asm/demo.c`.
- **§3 `build_syn_packet`.** Fills both headers and both checksums into a 40-byte
  buffer. Note the honesty comment: under `IP_HDRINCL` Linux **overwrites** the IP
  total-length and checksum (and fills source/ID when zero), but it does **not**
  touch the **TCP** checksum — so getting *that* right is load-bearing; a wrong TCP
  checksum makes the peer silently drop the SYN and every port looks filtered.
- **§4 Helpers.** `discover_source_ip` (the "UDP `connect` + `getsockname`" trick to
  learn our outgoing source address without sending a packet); `sleep_ns` /
  `sleep_between_probes` (EINTR-safe rate limiting); the `port_state` enum whose
  `calloc`-zero default is `PS_FILTERED`.
- **§5 `classify_reply`.** The crux: bounds-check the datagram (a forged `IHL` must
  not walk us off the buffer), confirm it's TCP from the probed host to our source
  port, then read the flags — `SYN|ACK` ⇒ open, `RST` ⇒ closed.
- **§6 `syn_scan`.** Opens the raw socket, sets `IP_HDRINCL` and `SO_RCVTIMEO`,
  sends one SYN per port (rate-limited), then drains replies until every port is
  decided or `recv` times out (`EAGAIN`). Undecided ⇒ stays filtered.
- **§7 `connect_scan`.** The unprivileged fallback: non-blocking `connect`,
  `select` for writability with a deadline, then `SO_ERROR` — `0` ⇒ open,
  `ECONNREFUSED` ⇒ closed, timeout ⇒ filtered.
- **§8–9 `parse_ports` / `resolve_target` / `main`.** Port-spec parsing via a
  65536-entry bitmap (dedup + sort for free), `getaddrinfo` resolution, argument
  parsing, dispatch, and a timed results table.

**A quirk worth knowing (the kernel's RST).** In a SYN scan *our* program never
completes the handshake, but the SYN/ACK from an open port is also delivered to the
host's real TCP stack — which has no socket for it and therefore answers with an
**RST**, tearing the half-open connection down. That stray RST is normal and is why
the scan is "half open"; production scanners often add an `iptables` rule to drop
outbound RSTs so the target sees an even cleaner half-open.

**Honest limitations of this core.** (1) No **retransmission**: a single dropped SYN
reads as "filtered". (2) The drain happens *after* the send loop, so on very large
ranges the socket receive buffer can overflow and lose a reply. (3) No parsing of
**ICMP** "destination unreachable" (which distinguishes admin-filtered from
dropped). (4) IPv4 only, no TCP options, no IPv6, no UDP. The "Going further"
section is the map from here to a real scanner.

## Assembly notes

The scanner itself pulls in `<sys/socket.h>` and the netinet headers, so it can't be
compiled to assembly standalone. `asm/demo.c` therefore lifts out the one routine
that is pure register-and-pointer math **and** is the heart of the project — the
TCP pseudo-header checksum (`sum16` + `fold_csum` + `tcp_checksum`, identical to
§2). Regenerate with the repo's exact flags:

```bash
make asm     # writes asm/demo.{O0.s, s, O2.s}  (clang cross-targets Linux)
```

[`asm/demo.annotated.s`](asm/demo.annotated.s) is the hand-written, per-instruction
walkthrough of the `-O1` output. The highlights it calls out:

- **Network byte order is one instruction.** `(buf[0]<<8)|buf[1]` compiles to a
  16-bit load plus **`rolw $8`** (swap the two bytes) — literally what `htons`/`ntohs`
  emit. Endianness handling is not magic; it's a rotate.
- **Wide accumulation.** The ones-complement sum runs in a 32-bit register so carries
  pile into the high half; `fold_csum`'s loop does the RFC 1071 end-around carry.
- **Constant-length specialization.** Because the pseudo-header length is the
  compile-time constant `sizeof(*ph) == 12`, clang drops the loop's length variable
  *and deletes the odd-byte tail as dead code* — you can only see that in the asm.
- **Frame-pointer placement.** In `tcp_checksum`, clang *defers* the `push %rbp`
  until after the first (stack-free) loop and pops it *before* the final fold —
  bookkeeping slid to bracket exactly the region a debugger would care about.

Compare [`asm/demo.O0.s`](asm/demo.O0.s) (every value spilled to the stack, every
branch real) with [`asm/demo.O2.s`](asm/demo.O2.s) (the loops laid out for the
branch predictor).

## Going further

The **`Stretch:`** direction for this project is **timing/rate control and a
`connect()` fallback**, both implemented here (`-r` and `-sT`). What a *production*
scanner such as **nmap** or **masscan** adds on top of this core:

- **Retransmission + adaptive timing.** Send each probe *k* times with an RTT-tracking
  timeout, so a single dropped packet isn't misread as "filtered". nmap's timing
  templates (`-T0..5`) and RTT/congestion estimator live here.
- **Stateless, asynchronous sending** (masscan-style): encode the target port into
  the TCP sequence number (a SYN cookie), send at line rate from one thread, and
  validate replies statelessly by checking `ack_seq == cookie(port)+1` — no per-port
  table, millions of packets/second.
- **ICMP-aware classification.** Parse ICMP type 3 (dest unreachable) codes to tell
  *admin-prohibited* (filtered) from *port unreachable* (closed) for UDP, and to
  detect rate-limiting.
- **Randomized port/host order + decoys** to spread load and evade naive IDS.
- **Service/version and OS detection** — banner grabbing, TLS `ClientHello`, and TCP/IP
  stack fingerprinting (nmap's `-sV` / `-O`).
- **IPv6, UDP, and TCP options** (MSS, window scale) for realistic probes.

## References

- **`man 7 raw`** — Linux raw-socket semantics and the exact `IP_HDRINCL` field-rewrite
  table this code relies on.
- **`man 7 ip` / `man 2 socket` / `man 2 setsockopt`** — `IP_HDRINCL`, `SO_RCVTIMEO`,
  `SO_ERROR`, `AF_INET`/`SOCK_RAW`.
- **RFC 791 (IP)** §3.1 header format & checksum; **RFC 793 (TCP)** §3.1 header and the
  pseudo-header; **RFC 1071** — "Computing the Internet Checksum".
- **nmap** — *Nmap Network Scanning* (Fyodor), esp. the SYN-scan and timing chapters;
  the `scan_engine.cc` source is the production version of everything here.
- **masscan** (`robertdavidgraham/masscan`) — the stateless, sequence-cookie design
  named under "Going further".
```
