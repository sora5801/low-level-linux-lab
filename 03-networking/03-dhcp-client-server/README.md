# A DHCP client/server 🟧

**What it is.** A from-scratch DHCP client and server that perform the full
**DORA** exchange — *DISCOVER → OFFER → REQUEST → ACK* — by crafting every byte
of every frame. Because a booting client has **no IP address yet**, we cannot
use ordinary UDP sockets; instead both programs open **AF_PACKET raw sockets**
and build the entire stack by hand: Ethernet + IPv4 + UDP + BOOTP/DHCP, with
correct checksums, and parse DHCP option TLVs (magic cookie `0x63825363`,
message-type option 53, requested-IP 50, lease-time 51, …). The server manages a
**lease pool with expiry**. This is a teaching *core*: it implements the happy
path of RFC 2131 plus NAK and RELEASE, and honestly omits relays, renewal
timers (T1/T2), ARP conflict probing, and persistence (see *Going further*).

## What you'll learn

- **AF_PACKET raw sockets** (`socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))`):
  how to send and receive complete Ethernet frames below the IP stack, and why
  DHCP *must* work there — an unconfigured host has no source IP and no route to
  the `255.255.255.255` limited broadcast.
- **Building the four headers by hand** and the two Internet checksums: the
  IPv4 header checksum and the UDP checksum over the **12-byte IPv4
  pseudo-header** (RFC 768/1071 ones-complement with end-around carry).
- **Network byte order**: which fields are big-endian and why, and how a
  network-order 16-bit load becomes a `movzwl` + `rolw $8` in the assembly.
- **DHCP option parsing**: the TLV format, the PAD/END specials, the magic
  cookie, and the **bounds checks** that keep a hostile length byte from causing
  an over-read.
- **A lease pool with expiry**: keying leases by client MAC, tentative OFFER
  holds vs. committed BOUND leases, and reclaiming expired slots.
- Syscalls: `socket`, `bind`, `sendto`, `recvfrom`, `poll`, `ioctl`
  (`SIOCGIFHWADDR`), `if_nametoindex`, `getrandom`, `sigaction`.

## Build & run

**Platform: Linux only.** AF_PACKET needs the **CAP_NET_RAW** capability — run
as root, or grant the binaries `setcap cap_net_raw+ep ./dhcp_client ./dhcp_server`.
On Windows/macOS you can still regenerate and read the assembly (`make asm`).

```bash
make                 # builds ./dhcp_client and ./dhcp_server (-Wall -Wextra)
```

### The self-contained demo (recommended)

`make demo` builds a private two-host network with **network namespaces** joined
by a **veth pair**, so nothing touches your real LAN, then runs a real DORA:

```bash
sudo make demo
# == creating namespaces and the veth pair ==
# == starting dhcp_server in netns dhcpsrv ==
# dhcp-server on veth-srv: server-id 192.168.50.1, pool 192.168.50.100-192.168.50.119 (20), lease 3600s
# == running dhcp_client in netns dhcpcli ==
# -> DISCOVER (xid 0x1a2b3c4d, try 1)
# <- OFFER  yiaddr 192.168.50.100
# -> REQUEST 192.168.50.100 (try 1)
# <- ACK    lease bound
#    address : 192.168.50.100
#    netmask : 255.255.255.0
#    gateway : 192.168.50.1
#    dns     : 192.168.50.1
#    lease   : 3600 seconds
```

(If `demo.sh` is not executable after checkout: `chmod +x demo.sh`.)

### Running the pieces by hand

```bash
sudo ./dhcp_server eth0 [server-ip] [pool-start] [pool-count]
sudo ./dhcp_client eth0
# watch the wire in another terminal:
sudo tcpdump -i eth0 -vv -n port 67 or port 68
```

## How it works

| file | role |
|------|------|
| `dhcp.h` | The four packed wire structs (`eth_hdr`, `ip_hdr`, `udp_hdr`, `dhcp_msg`), option/message-type constants, portable `htons_/htonl_`, and the shared-helper prototypes. Heavy comments give every field's byte offset and byte order. |
| `dhcp_common.c` | The **pure logic**, shared by both programs: `ip_checksum`, `udp_checksum` (pseudo-header), the option builders, the **`dhcp_opt_find` TLV walker**, and `dhcp_build_frame` which stitches the four layers together and checksums them. No syscalls live here — which is why the assembly demo is carved out of it. |
| `dhcp_client.c` | Opens the raw socket, learns the NIC index+MAC, picks a random `xid`, and drives **D→O→R→A** with retransmission (`poll` + a monotonic deadline). `parse_reply` defensively validates every layer of each received frame before trusting it. |
| `dhcp_server.c` | The lease pool (`LEASE_FREE/OFFERED/BOUND`, keyed by MAC, with expiry), the DISCOVER→OFFER and REQUEST→ACK/NAK handlers, RELEASE handling, and event-driven reclamation of expired leases. |
| `asm/demo.c` | Header-free extraction of `dhcp_opt_find` + `udp_checksum` for the assembly deliverable (below). |
| `demo.sh` | The netns + veth harness `make demo` runs. |

**The DORA state, end to end.** The client sends **DISCOVER** to
`255.255.255.255:67` from `0.0.0.0:68`, broadcast at both L2 (dst MAC
`ff:ff:ff:ff:ff:ff`) and L3, tagged with a random 32-bit `xid` and the
BROADCAST flag. The server picks a free pool slot, marks it *OFFERED* (a short
hold), and replies with an **OFFER** carrying `yiaddr` and options 54/51/1/3/6.
The client echoes the chosen address (option 50) and server-id (option 54) in a
broadcast **REQUEST**; the server commits the lease (*BOUND*, full lease time)
and sends **ACK**. The single `xid` threads all four messages; every reply is
filtered on EtherType/protocol/ports/`op`/`xid`/magic-cookie before it is
believed.

**Why raw sockets and broadcast.** With no address and no route, a normal
`sendto` on an `AF_INET` socket would fail. `AF_PACKET`/`SOCK_RAW` hands the
kernel a finished frame to transmit verbatim on one interface; the client is,
for these packets, its own IP stack. The BROADCAST flag asks the server to reply
via broadcast so a still-unconfigured client (which cannot yet answer a unicast
ARP) will accept the datagram.

## Assembly notes

Per CONVENTIONS §4, the real translation units need Linux headers and cannot be
compiled standalone here, so the teaching assembly comes from **`asm/demo.c`** —
a header-free copy of the two most instructive routines, byte-for-byte identical
in logic to `dhcp_common.c`:

- **`dhcp_opt_find`** — the TLV walker. The annotation's headline lesson: at
  `-O1` clang turned the loop's four outcomes (END/break, PAD/continue,
  match/return, skip/advance) into a small **state machine in `%r9d`** with a
  shared dispatch tail, rather than one tidy back-edge. Every bounds check
  survives in the asm as a real `cmp` against `opts_len` — the compiler will not
  optimize away memory-safety checks on attacker-controlled lengths.
- **`udp_checksum`** — the pseudo-header ones-complement sum. Watch the optimizer
  **reassociate** the address/header additions and **strength-reduce** the UDP
  length (which the source adds twice — once for the pseudo-header, once for the
  real header) into a single `lea` computing `2*((udp[4]<<8)|udp[5])`. Every
  network-order 16-bit load is a little-endian `movzwl` followed by `rolw $8`.

Files (regenerate with `make asm`; clang cross-targets Linux from any host):

- [`asm/demo.O0.s`](asm/demo.O0.s) — naive, every value spilled; read this to map
  each C statement to instructions.
- [`asm/demo.s`](asm/demo.s) — the `-O1` baseline we annotate.
- [`asm/demo.annotated.s`](asm/demo.annotated.s) — hand-written, one comment per
  instruction, with the SysV AMD64 ABI header.
- [`asm/demo.O2.s`](asm/demo.O2.s) — `-O2`, the same tricks with tighter scheduling.

## Going further

**Stretch:** make the client survive a reboot and *renew* its lease. That means
implementing the T1/T2 timers (RFC 2131 §4.4.5): at 50% of the lease the client
unicasts a REQUEST to the server (RENEWING); at 87.5% it broadcasts (REBINDING);
on expiry it falls back to DISCOVER. Add ARP conflict detection (send an ARP
probe for the offered address before binding; DECLINE on a reply).

What a production server (`dnsmasq`, ISC `dhcpd`, `kea`) adds that this core
omits: a persistent lease database (survive a restart), **relay-agent** support
(`giaddr` and option 82) so DHCP crosses subnets, static reservations, option 55
honouring, rate limiting, and a **timer wheel / periodic sweep** for expiry
instead of the event-driven reclamation used here.

## References

- **RFC 2131** — Dynamic Host Configuration Protocol (the DORA state machine).
- **RFC 2132** — DHCP Options and BOOTP Vendor Extensions (the option codes).
- **RFC 951** — BOOTP (the frame DHCP reuses); **RFC 768** — UDP; **RFC 791** —
  IPv4; **RFC 1071** — computing the Internet checksum.
- `man 7 packet` (AF_PACKET), `man 2 sendto`/`recvfrom`, `man 7 netdevice`
  (`SIOCGIFHWADDR`), `man 8 ip-netns`.
- Read the real thing: `busybox` `networking/udhcpc.c` + `udhcpd.c` (compact and
  very readable), then ISC `dhcp` / ISC `kea`.
