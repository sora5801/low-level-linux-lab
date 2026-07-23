# eBPF / XDP packet filter 🟥

**What it is.** A working eBPF program that runs at the **XDP hook** — the
earliest point in the Linux receive path, inside the NIC driver, *before* the
kernel builds an `sk_buff` — together with a **libbpf CO-RE** userspace loader
that attaches it, configures it, and prints live statistics. The BPF program
parses each frame (Ethernet + up to two VLAN tags + IPv4 + TCP/UDP/ICMP) with a
bounds check before every field access, counts packets/bytes per verdict, per
L4 protocol and per source IP in **BPF maps**, drops any source IP on a
userspace-managed **blocklist**, and computes a 5-tuple **flow hash** to show
which load-balancer bucket each packet would land in.

This is a **teaching core**, and it is honest about that: it demonstrates the
full XDP data-plane/control-plane loop end to end (parse → decide → account →
DROP/PASS), but it *counts* the load-balancer buckets rather than actually
redirecting, and it does IPv4 only. What a production scrubber/LB adds on top is
spelled out in **Going further**.

> Difficulty 🟥 — you are writing code the kernel verifier must *prove* safe,
> reasoning about per-CPU concurrency, byte order, and packet bounds, and wiring
> a two-world (kernel bytecode ⇄ userspace) program together through maps.

## What you'll learn

- **XDP**: the driver-level hook, its verdicts (`XDP_DROP`, `XDP_PASS`, `XDP_TX`,
  `XDP_REDIRECT`, `XDP_ABORTED`), and why dropping here is the cheapest possible
  place to shed a flood.
- **The BPF verifier's constraints**, concretely:
  - **Direct packet access** — you get two pointers, `data` and `data_end`, and
    the verifier rejects any load it cannot prove stays `< data_end`. Every
    header access in this program is preceded by an explicit bounds check.
  - **Bounded loops** — the verifier must prove termination. We peel VLAN tags
    with a `#pragma unroll` fixed-count loop (the always-accepted form) and
    explain the modern `bpf_loop()` alternative.
  - **No arbitrary pointer math** — you may only advance packet pointers by
    checked amounts, and a map value pointer must be NULL-checked before use.
- **BPF maps** as the only kernel⇄userspace channel: `PERCPU_ARRAY` (lock-free
  hot-path counters), `LRU_HASH` (self-evicting top-talkers table), and `HASH`
  (the blocklist). Why per-CPU maps let the data plane skip atomics, and why the
  shared HASH needs `__sync_fetch_and_add`.
- **libbpf CO-RE + skeletons**: open → set `rodata` config → load (verify) →
  attach → read maps, all through a generated typed API.
- **Byte order and struct layout** discipline: network vs host endianness, and
  why an unzeroed key-struct padding byte is a classic map bug.

## Build & run (Linux only; do the run in a VM)

XDP attaches to a real NIC and needs `CAP_NET_ADMIN`/root and a kernel ≥ ~5.5.
**Do this in a throwaway QEMU/cloud VM, not on your host** — a buggy XDP program
attached to your primary NIC can black-hole your connectivity. On Windows/macOS
you can still regenerate and read the teaching assembly (`make asm`).

Install the toolchain (Debian/Ubuntu):

```bash
sudo apt install clang llvm libbpf-dev bpftool libelf-dev zlib1g-dev \
                 linux-tools-common linux-tools-$(uname -r)
```

Build:

```bash
make            # -> xdp_filter.bpf.o, xdp_filter.skel.h, ./xdp_loader
```

Run it against a spare interface (here we make a veth pair so nothing important
is at risk):

```bash
# create an isolated veth pair; attach XDP to one end
sudo ip link add veth0 type veth peer name veth1
sudo ip link set veth0 up ; sudo ip link set veth1 up
sudo ip addr add 10.0.0.1/24 dev veth0

# attach the filter to veth0, report every 2s, drop all ICMP, block one source
sudo ./xdp_loader veth0 -i -t 2 -b 10.0.0.66

# in another shell, generate traffic INTO veth0 (from the peer side):
sudo ip netns add t; sudo ip link set veth1 netns t
sudo ip -n t addr add 10.0.0.2/24 dev veth1; sudo ip -n t link set veth1 up
sudo ip netns exec t ping -c3 10.0.0.1        # ICMP -> should be dropped & counted
sudo ip netns exec t hping3 --udp 10.0.0.1    # or any traffic generator
```

You'll see a live table of verdicts, protocols, flow-hash buckets, top talkers,
and blocklist hits. `Ctrl-C` detaches cleanly. Inspect from the side with the
standard tools:

```bash
sudo bpftool prog show           # your xdp program, its id and attach point
sudo bpftool map dump name src_map
sudo bpftool net show dev veth0  # confirms the XDP attachment
```

## How it works (file by file)

- **`xdp_filter.h`** — the ABI shared by both sides: `struct datarec` (the
  packet/byte counter), `enum proto_idx` (dense protocol indices for the array
  map), `struct flow_key` (the 5-tuple), and the map-sizing constants. Defining
  these *once* and including from both `.bpf.c` and the loader is what stops the
  two separately-compiled worlds from disagreeing on layout.

- **`xdp_filter.bpf.c`** — the data plane (BPF C, compiled with `-target bpf`):
  1. Casts `ctx->data`/`ctx->data_end` and parses Ethernet, a bounded VLAN loop,
     the variable-length IPv4 header (it re-checks bounds using the attacker-
     controlled `ihl`), then TCP/UDP/ICMP — each behind a bounds check.
  2. Declares five maps (see the heavy comments): a per-CPU verdict histogram, a
     per-CPU per-protocol counter, a per-CPU flow-hash bucket counter, an
     LRU-hash top-talkers table, and a HASH blocklist.
  3. `flow_hash()` computes FNV-1a over the 5-tuple; `& (LB_BUCKETS-1)` folds it
     to a bucket — the exact per-packet decision of a hash-based L4 LB.
  4. A `const volatile __u8 drop_all_icmp` `rodata` flag the loader patches
     before load, so turning ICMP-drop off costs *zero* runtime instructions
     (the verifier dead-code-strips the branch).

- **`xdp_loader.c`** — the control plane (host C, libbpf): opens the skeleton,
  sets `rodata`, loads (this is where the **verifier** runs), fills the
  blocklist from `-b` args, attaches to the NIC by ifindex, then every interval
  reads the maps and prints them. Reading a per-CPU map returns one value **per
  CPU**; `sum_percpu()` folds them — the deliberate cost we moved off the hot
  path so the data plane could stay atomic-free.

- **`Makefile`** — the three-step BPF build (compile → `bpftool gen skeleton` →
  link loader), plus `make asm` (regenerate the teaching assembly) and
  `make bpf-asm` (emit the BPF ISA for the flow hash, for comparison).

## Assembly notes

Kernel/BPF C cannot be compiled to standalone **x86-64** assembly on this host:
`xdp_filter.bpf.c` needs `<linux/bpf.h>` and targets the BPF ISA, not x86. So,
per the lab convention, the project's most instructive pure-logic routine — the
**flow-hash key computation** — is lifted into a dependency-free
[`asm/demo.c`](asm/demo.c) (it declares its own `u8/u16/u32` and `struct
flow_key`, includes nothing), and we generate real x86-64 assembly from *that*:

```bash
make asm     # runs the three clang --target=x86_64-pc-linux-gnu -S commands
```

- [`asm/demo.O0.s`](asm/demo.O0.s) — naive mapping; every byte spilled to the
  stack, and `flow_bucket` makes a real `call flow_hash`.
- [`asm/demo.s`](asm/demo.s) — **-O1, the annotated baseline.**
- [`asm/demo.O2.s`](asm/demo.O2.s) — same math, frame pointer omitted.
- [`asm/demo.annotated.s`](asm/demo.annotated.s) — the -O1 output with a comment
  on essentially every instruction and a header explaining the SysV AMD64 ABI.

The annotation makes two things visible. First, **FNV-1a is a perfectly regular,
branchless chain**: for each of the 13 meaningful bytes, `{ movzbl load; xorl
into hash; imull $0x01000193 }`. No loop, no table, no branch — which is exactly
why it is cheap enough to run on every packet at tens of millions of pps.
Second, the optimizer **inlined `flow_hash` into `flow_bucket`** (the `call`
disappears; the 13 groups reappear verbatim) and implemented the power-of-two
modulo `hash & (nbuckets-1)` as a single `leal -1(%rsi)`/`andl` pair — the
reason real load balancers size their bucket tables to powers of two. clang
keeps the hardware `imul` at every optimization level rather than expanding it
into a shift/add chain.

## Going further (the `Stretch:` from the list)

- **XDP L4 load balancer / DDoS scrubber (Katran / Facebook, Cloudflare style).**
  Turn the count-only bucketing into real forwarding: keep a map of backend
  MAC/IP per bucket, rewrite the destination (or encapsulate) and return
  `XDP_TX`/`XDP_REDIRECT`. Replace plain modulo hashing with **Maglev consistent
  hashing** so adding/removing a backend reshuffles only `1/N` of flows instead
  of all of them, and add a connection-tracking map so mid-connection backend
  changes don't reset established flows. Read Katran (`facebookincubator/katran`).
- **AF_XDP zero-copy.** Instead of processing in-kernel, `XDP_REDIRECT` selected
  flows into an **AF_XDP** socket whose UMEM frames are shared with userspace —
  the packet bytes are delivered to your process with no copy and no syscall per
  packet, feeding a userspace fast path (DPDK-class throughput without DPDK).
- **IPv6 and deeper parsing.** Add an `ETH_P_IPV6` branch (walk the fixed 40-byte
  header + any extension headers with another bounded loop) and TCP-flag / rate
  heuristics (SYN-flood detection) driving the drop decision.

## References

- Kernel docs: `Documentation/bpf/` (esp. the verifier and map docs), and
  `Documentation/networking/af_xdp.rst`.
- The **verifier** source: `kernel/bpf/verifier.c` — the thing that proves your
  program safe; skimming its checks demystifies every rejection.
- libbpf: `tools/lib/bpf/` in the kernel tree; `libbpf/libbpf` on GitHub; and the
  `libbpf-bootstrap` and `xdp-project/xdp-tutorial` repos (the canonical XDP
  teaching code this project follows).
- Production: `facebookincubator/katran` (XDP L4 LB), Cilium (`cilium/cilium`),
  and the XDP paper: Høiland-Jørgensen et al., *"The eXpress Data Path"*, CoNEXT
  2018.
- `man 2 bpf`, `man 7 bpf-helpers`, `bpftool` man pages.
