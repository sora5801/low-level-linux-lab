# Packet sniffer (tcpdump-lite) 🟧

**What it is.** A miniature `tcpdump`: it opens an `AF_PACKET` raw socket, hands
the kernel a **classic-BPF filter** it compiled from a `tcpdump`-style
expression (`tcp port 80`, `host 1.2.3.4`, …), captures frames through a
zero-copy **PACKET_MMAP** ring, and dissects Ethernet / IPv4 / IPv6 / ARP /
TCP / UDP / ICMP into one readable line per packet. It is a **teaching core**:
every layer is small enough to read in one sitting, and the piece the kernel
runs on *every* packet — the cBPF accept/reject VM — is extracted into
[`asm/demo.c`](asm/demo.c) and annotated instruction-by-instruction.

This is 🟧 (intermediate): the capture path, filter compiler, and decoders are
complete and correct; see **Going further** for what a production sniffer adds.

## What you'll learn

- **`AF_PACKET` / `PF_PACKET` raw sockets**: `socket(AF_PACKET, SOCK_RAW,
  htons(ETH_P_ALL))` to receive a copy of every link-layer frame, `bind()` via
  `struct sockaddr_ll` to pin an interface, and refcounted promiscuous mode with
  `PACKET_ADD_MEMBERSHIP` / `PACKET_MR_PROMISC`.
- **Classic BPF** — the little in-kernel accept/reject VM (A/X registers, packet
  loads, forward-only jumps, `ret`). How a filter string compiles down to a
  `struct sock_filter[]` and attaches with `setsockopt(SO_ATTACH_FILTER)`, and
  how the interpreter executes it (`asm/demo.c`).
- **PACKET_MMAP (TPACKET_V2)**: a shared-memory RX ring, the per-frame
  `tp_status` ownership handshake, and the **acquire/release memory ordering**
  that makes the kernel↔user producer/consumer race-free.
- **Protocol decoding**: header byte offsets, **network byte order** and why
  `ntohs`/`ntohl` appear at every multi-byte field, the IPv4 **ones-complement
  checksum**, the variable IHL header length, and fragment handling.
- The syscalls: `socket`(41), `bind`(49), `setsockopt`(54), `getsockopt`(55),
  `mmap`(9), `poll`(7), `recvfrom`(45), `sigaction`(13).

## Build & run

**Platform: Linux only** (WSL2 works). `AF_PACKET`, `PACKET_MMAP`, and
`SO_ATTACH_FILTER` are Linux kernel interfaces; this does **not** build or run on
Windows or macOS. Capturing needs `CAP_NET_RAW` — run as root or grant the cap:

```bash
make                              # builds ./sniffer  (clang -Wall -Wextra)
sudo ./sniffer -i eth0 tcp port 80        # capture matching packets
sudo setcap cap_net_raw+ep ./sniffer      # …or grant the cap once, then no sudo

# no privileges needed — just see what a filter compiles to (like tcpdump -d):
./sniffer -d tcp port 80
./sniffer -d host 1.2.3.4
```

Options: `-i <iface>` (default: all), `-c <n>` (stop after n), `-p`
(promiscuous, needs `-i`), `--no-mmap` (use `recvfrom` instead of the ring),
`-d` (dump the compiled BPF and exit). The filter expression is the trailing
words. Grammar (a subset of pcap):

```
expr := [ ip|arp|tcp|udp|icmp ] [ src|dst ] [ host <IPv4> | port <N> ]
```

Examples: `udp`, `tcp port 443`, `host 10.0.0.5`, `src host 1.2.3.4`,
`udp dst port 53`, `icmp`, or empty (match everything).

```bash
make asm            # regenerate asm/demo.{O0.s,s,O2.s} (any host; clang cross-targets)
make dump           # ./sniffer -d tcp port 80
make demo           # capture 20 packets (needs root)
```

## How it works

Four small translation units, built bottom-up:

- **`filter.c` / `filter.h`** — the filter compiler. `filter_compile()` parses
  the expression, then a tiny label-based assembler emits `struct sock_filter`
  instructions and back-patches the forward jump offsets (cBPF jumps are 8-bit,
  forward-only — the structural reason a filter always terminates). The output
  is exactly what `tcpdump -d` prints. `filter_dump()` disassembles it back for
  the `-d` flag. Highlights: the Ethernet/IPv4 offset constants (`23` = IP
  proto, `26`/`30` = src/dst IP), the `ldx 4*([14]&0xf)` **IP-header-length
  trick** for reaching L4 ports past IP options, and the fragment guard.
- **`decode.c` / `decode.h`** — the dissectors. `decode_frame()` walks
  Ethernet → IP → L4, **bounds-checking every layer before it reads** (a capture
  can be snapped short, or a packet can lie about its lengths — trusting a length
  is how real sniffer CVEs happened). Includes an RFC 1071 **ones-complement IP
  checksum** verifier, with the carry-fold math spelled out.
- **`ring.c` / `ring.h`** — the PACKET_MMAP RX ring. `ring_setup()` negotiates
  `TPACKET_V2`, sizes the ring against the kernel's constraints (frame/block/page
  alignment), and `mmap`s it `MAP_SHARED`. `ring_drain()` walks frames, reading
  `tp_status` with `__ATOMIC_ACQUIRE` and releasing each slot back to the kernel
  with `__ATOMIC_RELEASE` — the comments explain exactly which read/write each
  barrier orders and the corruption that results from getting it wrong.
- **`sniffer.c`** — the driver: parse args, compile the filter, open the socket,
  `SO_ATTACH_FILTER`, bind, optional promisc, set up the ring (or fall back to
  `recvfrom`), then a `poll()`-then-drain loop with clean `SIGINT` teardown and
  `PACKET_STATISTICS` at exit.

Data flow: `NIC → kernel taps every frame → runs our cBPF (drops non-matches
with no copy) → writes accepted frames into the mmap ring → we poll(), drain,
decode`.

## Assembly notes

The C decoders and socket code all pull in Linux system headers, so **no real
translation unit is self-contained** — per CONVENTIONS we extract the project's
purest logic into [`asm/demo.c`](asm/demo.c): the **classic-BPF interpreter**
(`bpf_run`), the accept/reject VM the kernel runs per packet. It has its own
integer types and no headers, so `clang -S` yields clean, teachable assembly.

[`asm/demo.annotated.s`](asm/demo.annotated.s) annotates the `-O1` output
line-by-line. The big lessons visible there:

- **The SysV AMD64 ABI in practice**: args in `rdi/rsi/rdx/rcx`, result in
  `rax`, and the prologue/epilogue saving every callee-saved register the
  optimizer used (`rbx` = the BPF `X` register, `r12–r15`).
- **What `-O1` did to a two-level `switch`**: with `-fno-jump-tables` it became a
  **binary comparison tree** (watch it bisect the 8 instruction classes), and it
  **merged every common tail** — all the "compute A, `pc++`, continue" paths
  collapse into one shared block (`LBB0_79` for ALU, `LBB0_99` for loads,
  `LBB0_2` for the generic continue). One asm block now serves many C cases.
- **Branchless idioms**: `cmov` for the `ret A|k` select and the `TAX`/`TXA`
  move; `set<cc>` + `xor $3` to index `jt` vs `jf` with no branch; `(byte<<2) &
  60` to compute `4*(byte&0xf)` (the IP-header-length trick).
- **Every packet load is bounds-checked** (the `LBB0_96` funnel) — the property
  that lets the kernel run an untrusted filter on hostile packets safely.

Compare the three levels: [`demo.O0.s`](asm/demo.O0.s) (naive, everything
spilled to the stack — easiest to trace statement by statement),
[`demo.s`](asm/demo.s) (`-O1`, the annotated baseline), and
[`demo.O2.s`](asm/demo.O2.s) (`-O2`). Regenerate with `make asm`.

> Note: `asm/demo.c` fixes a subtlety the sniffer never exercises — classic BPF
> selects a `ret` value via its **RVAL** field (`code & 0x18`, so `BPF_A` =
> `0x10`), *not* the ALU/JMP operand-source bit (`code & 0x08`). Our compiler
> only emits `ret #k`, but the interpreter documents `ret A` correctly.

## Going further

The `Stretch:` direction — turn the teaching core into something closer to real
`tcpdump`/`libpcap`:

- **A real filter grammar.** pcap compiles arbitrary boolean expressions
  (`and`/`or`/`not`, netmasks, port ranges, `vlan`, `ip6`). Ours takes one
  primitive plus a protocol; extend the parser to a proper expression tree and
  emit the BPF for `and`/`or` (short-circuit jump wiring).
- **TPACKET_V3.** The block-based, kernel-timed ring that batches frames per
  block and lets the kernel choose frame boundaries — much better at high packet
  rates than V2's fixed per-frame slots.
- **`pcap` file output.** Write a `.pcap` global header + per-packet records so
  captures open in Wireshark.
- **Other link types & offloads.** Cooked `SLL` headers on `any`, 802.1Q VLAN
  tags, and GRO/GSO super-frames (which is why an on-wire length can exceed a
  frame).
- **eBPF.** The modern path: `SO_ATTACH_BPF` attaches a verified eBPF program
  (maps, tail calls, JIT) instead of classic BPF. `AF_XDP` goes further, DMA-ing
  frames into a user ring for line-rate capture.

Production sniffers (`libpcap`) also handle the attach race properly: attach a
*drop-all* filter first, drain the socket, then swap in the real filter, so no
unfiltered packet is ever queued. We attach the real filter right after
`socket()` (before `bind`) to keep the window tiny, and say so in the code.

## References

- `man 7 packet` (AF_PACKET), `man 2 socket`, `man 7 socket` (`SO_ATTACH_FILTER`).
- Linux kernel: `Documentation/networking/packet_mmap.rst` (the ring layout and
  `tp_status` protocol) and `Documentation/networking/filter.rst` (classic BPF
  opcode reference — the source for `asm/demo.c`).
- Kernel source: `net/packet/af_packet.c` (the ring) and `net/core/filter.c`
  (`bpf_prog_run` classic path — the interpreter we mirror).
- `libpcap` `gencode.c` (the real expression → BPF compiler) and `tcpdump`
  `print-*.c` (the real dissectors).
- RFC 791 (IPv4), RFC 793 (TCP), RFC 768 (UDP), RFC 1071 (the Internet checksum).
