# userspace TCP/IP stack 🟥

**What it is.** A from-scratch TCP/IP stack that lives entirely in one Linux
userspace process. Its "wire" is a **TAP device** (`/dev/net/tun`): the kernel
hands us raw Ethernet frames on a file descriptor, and we implement everything
above the link ourselves — **Ethernet framing, ARP, IPv4, ICMP echo, UDP, and
TCP** (handshake, sequence/ack bookkeeping, the state machine, an RTO timer with
RTT estimation, a sliding window, and connection teardown). With the stack
running you can `ping 10.0.0.2` and `nc 10.0.0.2 7` from the host and get answers
from code you can read top to bottom.

This is a **teaching core**, not lwIP. It gets ping + a TCP three-way handshake
+ bidirectional data transfer (an echo service) working end-to-end, and it is
explicit — in code and in [Going further](#6-going-further) — about the two big
things production TCP adds that this omits: **congestion control** and
**out-of-order/SACK reassembly**.

---

## 1. What you'll learn

- **The TUN/TAP interface**: `open("/dev/net/tun")` + the `TUNSETIFF` ioctl with
  `IFF_TAP | IFF_NO_PI`, and why a TAP fd behaves like a virtual NIC.
- **Layering by offset arithmetic**: every header is a `__attribute__((packed))`
  struct overlaid on the frame buffer — parsing is just a pointer cast.
- **Network byte order** (big-endian) and the discipline of converting only at
  the wire boundary with `htons`/`ntohs`/`htonl`/`ntohl`.
- **The Internet checksum** (RFC 1071): ones'-complement 16-bit sum with an
  end-around carry fold, why it is byte-order independent, and the TCP/UDP
  **pseudo-header**. This is the routine extracted for the assembly deliverable.
- **ARP** (RFC 826): the cache, request/reply, and where ARP spoofing lives.
- **IPv4** (RFC 791): header validation, the header-only checksum, TTL, and what
  fragmentation *would* require (we set `DF` and skip it).
- **ICMP echo** (RFC 792): enough to satisfy the real `ping` tool.
- **TCP** (RFC 793 / 6298): the 11-state machine, 32-bit **sequence-number**
  arithmetic that survives wraparound, cumulative ACKs, a **retransmission
  timeout** with **Jacobson/Karels RTT estimation** and **Karn's algorithm**,
  **flow control** via the advertised window, and the **FIN** teardown including
  `TIME_WAIT`'s `2*MSL` linger.
- **A single-threaded event loop**: `poll()` one fd, react, tick the timers —
  and why "one place touches the state" means there are no races to reason about.

## 2. Build & run (Linux; needs root or `CAP_NET_ADMIN`)

The stack is **Linux-only** — it uses `/dev/net/tun` and Linux ioctls. On
Windows/macOS use a Linux VM or WSL2 (WSL2 has `/dev/net/tun`).

```bash
make                     # builds ./tcpip with -Wall -Wextra

# Terminal A: run the stack (needs privilege to create the tap device)
sudo ./tcpip             # creates tap0, takes IP 10.0.0.2
#   or: sudo ./tcpip 10.0.0.5   to choose a different address

# Terminal B: give the HOST side of the link an address and bring it up
sudo ip addr add 10.0.0.1/24 dev tap0
sudo ip link set tap0 up

# Now talk to the stack:
ping 10.0.0.2                       # ICMP echo — our stack replies
printf 'hello\n' | nc -q1 10.0.0.2 7   # TCP echo on port 7 (handshake+data+FIN)
printf 'hi\n'    | nc -u -q1 10.0.0.2 7 # UDP echo on port 7
```

Watch it work with `tcpdump -i tap0 -n` in a third terminal: you will see the
ARP who-has/reply, the ICMP echo pair, and the full TCP `SYN → SYN,ACK → ACK`,
data + ACKs, and `FIN,ACK` exchanges. `Ctrl-C` stops the stack cleanly (the
signal handler just flips a flag the loop checks).

> If `ping`/`nc` hang: confirm you ran the `ip addr`/`ip link` commands on the
> host side, that `tcpip` still runs in Terminal A, and that no firewall drops
> traffic on `tap0`. The stack logs every packet it handles to stderr.

## 3. How it works — a tour of the code

The receive path is a chain of `*_input()` functions; the send path is a chain of
`*_output()` functions. Data flows **up** on receive and **down** on send.

| File | Role |
|------|------|
| `common.h` | Fixed-width types and every on-the-wire header struct (packed), plus the byte-order rules. |
| `checksum.{c,h}` | The Internet checksum: `csum_accumulate` / `csum_fold` / `inet_checksum`. Used by IP, ICMP, UDP, TCP. |
| `tap.{c,h}` | `tap_open` (the `TUNSETIFF` ioctl) and frame read/write, with `EINTR` retries. |
| `netif.h` | `struct netif` (fd, our MAC, our IP) — the interface object passed everywhere. |
| `ether.c` | `eth_input` demuxes on EtherType (ARP vs IPv4); `eth_output` prepends the 14-byte header. |
| `arp.{c,h}` | The ARP cache and request/reply. Answers "who has 10.0.0.2?". |
| `ip.{c,h}` | IPv4 parse/validate/checksum on input; header build + ARP-resolved send on output. |
| `icmp.{c,h}` | Echo-request → echo-reply (makes `ping` work). |
| `udp.{c,h}` | UDP + the pseudo-header checksum; a port-7 echo service. |
| `tcp.{c,h}` | The whole TCP: TCB table, state machine, output engine, ACK/RTT/RTO logic, timers, teardown, port-7 echo. |
| `main.c` | Interface bring-up and the `poll()` event loop that drives input and the timers. |

**Key ideas to read for:**

- **Sequence arithmetic** (`tcp.c`, `seq_lt`/`seq_gt`): comparisons are done on
  the *signed* difference `(int32_t)(a - b)` so they stay correct across the
  32-bit wraparound. A naive `a < b` breaks near the wrap.
- **The send buffer model** (`tcp.c`): `sndbuf[0]` always represents sequence
  `snd_data_start`; `snd_una..snd_nxt` is in flight, `snd_nxt..end` is queued.
  A SYN and a FIN each consume one sequence number without occupying a `sndbuf`
  byte — `tcp_process_ack` intersects the acked range with the data range to
  free exactly the right bytes.
- **The RTO timer** (`tcp.c`): one timer per connection tracks the oldest
  unacked segment. On timeout we retransmit it, double the RTO (exponential
  backoff), and — per **Karn** — refuse to take an RTT sample from the ambiguous
  retransmission.
- **Flow control** (`tcp.c`): we advertise our receive-buffer free space as the
  window and never put more than the peer's advertised window in flight.

## 4. Assembly notes

The assembly deliverable dissects the **Internet checksum**, the one piece of
pure, allocation-free, header-free logic every layer shares. The rest of the
stack talks to Linux (`<linux/if_tun.h>`, `poll`, `sigaction`, …), so it is not
self-contained; `asm/demo.c` is a headerless extraction of the checksum with its
own integer typedefs, and it is what the committed `.s` files come from.

- [`asm/demo.c`](asm/demo.c) — the self-contained source.
- [`asm/demo.O0.s`](asm/demo.O0.s) — `-O0`: every C statement mapped literally,
  helpers still called, values spilled to the stack. Easiest to trace.
- [`asm/demo.s`](asm/demo.s) — `-O1`, the annotated baseline.
- [`asm/demo.O2.s`](asm/demo.O2.s) — `-O2`: the loop unrolled/vectorized.
- [`asm/demo.annotated.s`](asm/demo.annotated.s) — the `-O1` output with a
  comment on essentially every instruction and a SysV AMD64 ABI header.

The lesson the annotation pulls out: our C wrote `(p[0] << 8) | p[1]` to assemble
a big-endian 16-bit word, and clang compiled that into a single 16-bit **load
plus a `rolw $8` byte-swap** — one memory access, not two. And the 32-bit
accumulator is load-bearing: each `addl` can carry out of bit 15, and the
`while (sum >> 16)` **end-around carry fold** is what folds those carries back.
All three routines are leaf functions — no `call`, no stack locals, no red-zone
use — so every value lives in a register.

Regenerate the `.s` files (host-portable; clang cross-targets Linux) with
`make asm`. `demo.annotated.s` is hand-written and never overwritten.

## 5. What this teaching core covers vs. omits

**Covered, end-to-end:** TAP bring-up; ARP; IPv4 in/out with checksums; ICMP
echo; UDP echo; TCP passive open (`LISTEN → SYN_RCVD → ESTABLISHED`); reliable
in-order data with cumulative ACKs; RTO + RTT estimation + Karn + exponential
backoff; advertised-window flow control; teardown (`FIN`, `CLOSE_WAIT`,
`LAST_ACK`, and the active-close `FIN_WAIT_1/2`, `CLOSING`, `TIME_WAIT` states).

**Deliberately omitted (stated honestly):**

- **Congestion control.** There is no congestion window (`cwnd`), slow start, or
  congestion avoidance. We send up to the peer's *flow-control* window only.
  On an isolated tap link with one peer this is fine; on a shared network it is
  not — see below.
- **Out-of-order buffering / SACK.** We accept only the in-order segment
  (`seg_seq == rcv_nxt`) and drop the rest, re-ACKing to prompt retransmission.
- **Active open (`connect`)**, delayed ACKs, Nagle, window scaling, timestamps/
  PAWS, zero-window persist, and IP fragment reassembly (we set `DF`).
- **RST generation** for stray segments (we drop; the hook is marked in `tcp.c`).

## 6. Going further

The list's **Stretch** goals, and what production stacks do:

- **Congestion control: Reno → CUBIC.** Add a `cwnd` and send
  `min(cwnd, snd_wnd)`. Reno does slow start (double `cwnd` per RTT until a
  threshold), congestion avoidance (linear growth), and multiplicative decrease
  on loss; fast retransmit/fast recovery react to **3 duplicate ACKs** instead of
  waiting for the RTO. CUBIC replaces the linear growth with a cubic function of
  time since the last loss, which fills high bandwidth-delay-product links far
  faster. This is the single biggest gap between this core and real TCP.
- **SACK** (RFC 2018): let the receiver acknowledge non-contiguous blocks so the
  sender retransmits only the actual holes, not everything after them. Requires
  the out-of-order receive buffer this core skips.
- **A Berkeley-sockets shim.** Wrap the TCB API in `socket()/bind()/listen()/
  accept()/read()/write()/close()` so existing programs link against your stack.
- **Timestamps + PAWS + window scaling** for correctness and throughput on fast,
  long links; **delayed ACKs** and **Nagle** to cut small-packet overhead.

The canonical read-along for this project is Saminiir's "Let's code a TCP/IP
stack" series and lwIP's source — both structured almost exactly like this tree.

## 7. References

- RFC 791 (IPv4), RFC 792 (ICMP), RFC 768 (UDP), RFC 826 (ARP).
- RFC 793 (TCP) and RFC 9293 (its modern consolidation); RFC 6298 (computing the
  RTO); RFC 5681 (Reno congestion control); RFC 2018 (SACK); RFC 8312 (CUBIC).
- RFC 1071 — "Computing the Internet Checksum" (the assembly demo's subject).
- `Documentation/networking/tuntap.rst` in the Linux tree; `man 2 ioctl`,
  `man 7 tcp`, `man 2 poll`.
- lwIP (`lwip/src/core/`) and the `saminiir/level-ip` project — production and
  teaching implementations shaped like this one.
```
