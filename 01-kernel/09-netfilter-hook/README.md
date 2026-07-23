# Netfilter hook module 🟧

**What it is.** A loadable Linux kernel module that plants a **Netfilter hook**
into the IPv4 stack — the same framework `iptables`/`nftables` are built on. At
two of the kernel's five packet-processing checkpoints (`PRE_ROUTING` and
`LOCAL_IN`) it parses the IPv4 + TCP/UDP headers straight out of the `sk_buff`,
matches them against a tiny rule table, and returns a **verdict**: `NF_ACCEPT`
(continue) or `NF_DROP` (discard). Matches are logged with rate-limited
`pr_info`. It is a real, working in-kernel packet filter — deliberately small so
the *mechanism* (registration, sk_buff parsing, the verdict) is the whole point.

This is a **teaching core**, not a firewall you'd ship. It covers the hook
lifecycle and safe packet parsing end to end; it does **not** implement stateful
connection tracking, IPv6, REJECT-with-ICMP, or per-namespace registration.
Those gaps are named explicitly in *Going further*.

> **This is kernel code. Build and run it only inside a throwaway QEMU/KVM VM.**
> A buggy hook can drop all your traffic or panic the box. It compiles only
> against real Linux kernel headers and will not build on this Windows host —
> which is exactly why the assembly deliverable lives in a standalone extract
> (`asm/demo.c`). See *Assembly notes*.

## What you'll learn

- **The Netfilter hook API**: `struct nf_hook_ops`, `nf_register_net_hook()` /
  `nf_unregister_net_hook()`, the five `NF_INET_*` hook points, hook priorities
  (`NF_IP_PRI_FIRST`), and the verdict codes `NF_ACCEPT` / `NF_DROP`.
- **Parsing an `sk_buff` safely**: `ip_hdr()`, and — the key defensive idiom —
  `skb_header_pointer()` to read the transport header without assuming the skb
  is *linear* (its bytes may live in page fragments; a raw cast reads the wrong
  memory).
- **Softirq context rules**: the hook runs in the receive bottom half — no
  sleeping, no `copy_*_user`, and shared state races across CPUs.
- **Why logging must be rate-limited** (`net_ratelimit()`): an un-throttled
  `pr_info` in the packet path is a self-inflicted DoS.
- **Module init/exit discipline**: ordered registration with rollback on
  failure, and why `nf_unregister_net_hook()` must complete before unload.
- **The Internet checksum** (RFC 1071) as pure bit-twiddling — the assembly
  deliverable — including how the optimizer vectorizes and simplifies it.

## Build & run (Linux, inside a VM)

```bash
# On a Linux box or VM with matching kernel headers installed:
make                      # builds netfilter_hook.ko via Kbuild
sudo insmod netfilter_hook.ko              # load with defaults (drop TCP:23)
# or tune at load time:
sudo insmod netfilter_hook.ko drop_tcp_port=2222 drop_udp_port=1900 log_accept=1

dmesg | tail                               # see the load line + DROP logs
```

Exercise it (from another host, or a network namespace) — hits to the blocked
port vanish, everything else flows:

```bash
# with drop_tcp_port=23 loaded, this connection is silently dropped:
nc <vm-ip> 23
# retune a LIVE module without reloading:
echo 8080 | sudo tee /sys/module/netfilter_hook/parameters/drop_tcp_port
```

Unload (prints the inspected/dropped totals):

```bash
sudo rmmod netfilter_hook ; dmesg | tail -n 1
```

Regenerate the teaching assembly (works on **any** host with clang, including
this one — no kernel needed):

```bash
make asm
```

## How it works

**`netfilter_hook.c`** — the whole module, built top-down:

- **Module parameters** (`drop_tcp_port`, `drop_udp_port`, `log_accept`) — wired
  via `module_param(..., 0644)` so they are settable at `insmod` time *and*
  live-writable through `/sys/module/netfilter_hook/parameters/`.
- **`struct fw_rule` + `drop_rules[]`** — a static, `const` rule table
  (telnet 23, telnet-alt 2323, SSDP/UDP 1900). A real firewall is just an
  elaborate, mutable version of this list. `const` means it needs no locking.
- **`fw_match_port()`** — a pure function: does `(proto, dport)` hit any rule or
  module-param knob? Returns the human reason string, or `NULL`.
- **`fw_hook_fn()`** — *the hook itself*. Fixed signature
  `(void *priv, struct sk_buff *skb, const struct nf_hook_state *state)`. It
  null-checks the skb, grabs the IPv4 header with `ip_hdr()`, and reaches the
  transport header with **`skb_header_pointer()`** (into an on-stack scratch
  `struct tcphdr`/`udphdr`) — the one idiom that makes non-linear skbs safe.
  It consults the rules, logs via `net_ratelimit()`-gated `pr_info` with `%pI4`
  address formatting, and returns `NF_DROP` or `NF_ACCEPT`.
- **`nfho_prerouting` / `nfho_local_in`** — two `struct nf_hook_ops` binding the
  same hook to `NF_INET_PRE_ROUTING` and `NF_INET_LOCAL_IN` at `NF_IP_PRI_FIRST`.
- **`nf_hook_init()` / `nf_hook_exit()`** — register both hooks with rollback if
  the second fails; unregister in reverse order on unload (which also waits for
  in-flight packets to leave the hook before the module text can be freed).

**`Makefile`** — the standard out-of-tree Kbuild recipe (`obj-m`,
`make -C $(KDIR) M=$(PWD) modules`), plus `make asm` to regenerate the assembly
and `make load`/`unload`/`log` convenience targets.

## Assembly notes

Kernel C cannot be compiled to standalone assembly on this host — it needs
hundreds of kernel headers and only links inside the kernel. So, per the repo
convention, the project's most instructive **pure-logic** routine is extracted
into a header-free `asm/demo.c`: the **IP header checksum** (RFC 1071 ones-
complement sum), the classic every packet filter touches. It is compiled to real
Linux SysV assembly with the exact commands in `make asm`:

- [`asm/demo.O0.s`](asm/demo.O0.s) — `-O0`: the literal mapping. `csum_fold` is a
  real function reached by `call`; `ip_checksum_valid` really `call`s
  `ip_checksum`; every value spills to the stack.
- [`asm/demo.s`](asm/demo.s) — `-O1`, the annotated baseline.
- [`asm/demo.O2.s`](asm/demo.O2.s) — `-O2`: the summation loop is **auto-
  vectorized with SSE2** (`punpcklwd` widens four 16-bit words to 32-bit lanes,
  `paddd` sums four per iteration, a `pshufd`/`paddd` cascade reduces back to a
  scalar).
- [`asm/demo.annotated.s`](asm/demo.annotated.s) — the hand-written, per-
  instruction walkthrough of the `-O1` output.

The annotation highlights two things worth seeing: at `-O1` clang **inlined
`csum_fold` into both callers** (the symbol vanishes entirely), and in
`ip_checksum_valid` it did algebra on the ones-complement identity — replacing
`(~sum == 0)` with `(sum == 0xFFFF)` and dropping the `notl` instruction. The
header block documents the SysV AMD64 ABI contract (arg registers
`rdi, rsi, rdx, rcx, r8, r9`; return in `rax`; callee-saved `rbx, rbp, r12–r15`)
and the prologue/epilogue.

## Going further (the `Stretch:` from the list)

The teaching core stops at *stateless* filtering. Production Netfilter adds:

- **Connection tracking (conntrack).** Match on connection *state*
  (`NEW`/`ESTABLISHED`/`RELATED`) instead of individual packets — the basis of a
  stateful firewall. Study `net/netfilter/nf_conntrack_core.c`; hang state off
  `skb_nfct()` / `nf_ct_get()`.
- **Push decisions to userspace with `nfnetlink_queue`.** Return `NF_QUEUE`
  instead of a verdict and let a userspace daemon (via `libnetfilter_queue`)
  decide — how DPI/IDS integrate. See `net/netfilter/nfnetlink_queue.c`.
- **Register per network namespace.** This core binds to `&init_net`, so
  containers in other netns are unfiltered. `register_pernet_subsys()` installs
  the hook in every present and future namespace.
- **REJECT, IPv6, checksum recompute.** REJECT sends an ICMP/RST rather than a
  silent drop; a parallel `NFPROTO_IPV6` hook with `ipv6_hdr()` covers v6; any
  header field you *mutate* requires recomputing the checksum from `asm/demo.c`.

## References

- **`man 7 netfilter`**; the kernel source `include/linux/netfilter.h`,
  `include/uapi/linux/netfilter.h` (verdict codes, hook numbers).
- **`net/ipv4/netfilter/`** and `net/netfilter/core.c` — where real hooks live
  and how `nf_hook_slow()` dispatches them.
- **`skb_header_pointer()`** in `include/linux/skbuff.h`, and the sk_buff
  documentation `Documentation/networking/skbuff.rst`.
- **RFC 1071** — "Computing the Internet Checksum" (the algorithm in
  `asm/demo.c`); `include/net/checksum.h` and `arch/x86/.../checksum_*.S` for the
  kernel's hand-tuned versions.
- The nftables example modules and `Documentation/networking/netfilter-*` in the
  kernel tree.
