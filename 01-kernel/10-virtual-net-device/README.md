# Virtual network device driver 🟥

**What it is.** A Linux kernel module that registers a software Ethernet
interface — `vnet0` — the same family of device as `lo`, `veth`, `dummy`, and
`tun`. It implements the full `net_device_ops` open/stop/xmit path, receives
frames back through a **NAPI poll loop** fed by a software RX ring, keeps
per-CPU statistics, and answers `ethtool`. The teaching-core is a **loopback
NIC**: every frame the stack transmits out of `vnet0` is looped straight back in
as a received frame, so a single interface behaves like a wire soldered to
itself. That closed loop is what lets you watch a `struct sk_buff` travel its
entire life — TX → ring → NAPI → RX — inside one small, readable driver.

This is a 🟥 project. It ships a genuinely working **teaching core**, not a
production NIC; the "Going further" section lists exactly what is left out.

> This is kernel code. It builds and runs **only on Linux**, ideally inside a
> throwaway QEMU VM (a buggy net driver can wedge networking or panic the box).
> It does **not** build on the host that generated the `asm/` files.

## What you'll learn

- **`net_device` lifecycle**: `alloc_netdev` + a `setup` constructor,
  `register_netdev` / `unregister_netdev`, and the `ndo_init` / `ndo_uninit`
  hooks that bracket registration.
- **`net_device_ops`**: `ndo_open`, `ndo_stop`, `ndo_start_xmit` (returning
  `NETDEV_TX_OK`, never an errno), and `ndo_get_stats64`.
- **The `sk_buff` life cycle**: who owns the skb at each hand-off, why xmit must
  never free it on the success path, `skb_orphan`, and `eth_type_trans` turning
  a TX frame into an RX frame by pulling the 14-byte Ethernet header.
- **NAPI polling**: `netif_napi_add`, `napi_enable`/`napi_disable`,
  `napi_schedule`, the `poll(napi, budget)` contract, `napi_gro_receive`, and
  the `napi_complete_done` lost-wakeup race.
- **Per-CPU statistics with `u64_stats_sync`**: why counters are per-CPU, and
  what the seqcount protects against on 32-bit CPUs.
- **`ethtool_ops`**: `get_drvinfo`, `get_link`, `get_ringparam`.
- **Carrier/queue state**: `netif_carrier_on/off`, `netif_start/stop_queue`.

## Build & run (Linux / QEMU VM)

You need kernel headers matching your running kernel
(`sudo apt install linux-headers-$(uname -r)` on Debian/Ubuntu), and a
**kernel ≥ 6.1** for the 3-argument `netif_napi_add` (see the note below).

```bash
make                              # builds vnetdev.ko via the kernel's Kbuild
sudo insmod vnetdev.ko            # dmesg: "vnet: registered vnet0 ..."
sudo ip link set vnet0 up         # dmesg: "up: NAPI armed, TX queue started"

# Give it an address and talk to ITSELF (loopback): pinging our own IP makes
# the stack transmit out vnet0; our driver loops the frame back as RX.
sudo ip addr add 10.0.0.1/24 dev vnet0
ping -c 3 10.0.0.1

ip -s link show vnet0             # watch rx_/tx_ packet + byte counters climb
ethtool -i vnet0                  # driver: vnet, version: 1.0, bus-info: software
ethtool -g vnet0                  # RX ring: 64 slots

sudo ip link set vnet0 down       # dmesg: "down: ... ring drained"
sudo rmmod vnetdev                # dmesg: "vnet: unregistered"
```

`make load` / `make unload` wrap the insmod + `ip link` dance. Regenerate the
teaching assembly on any host (no kernel needed) with `make asm`.

**Kernel-version note.** `netif_napi_add(dev, napi, poll)` is the 3-arg form
present since v6.1 (default budget `NAPI_POLL_WEIGHT` = 64). On 5.x it took a
4th `weight` argument; if you build there, change the call to
`netif_napi_add(dev, &priv->napi, vnet_poll, NAPI_POLL_WEIGHT)`.

## How it works (file by file)

### `vnetdev.c` — the whole driver

Read it top to bottom; it is built up in the order the kernel uses it.

- **`struct vnet_pcpu_stats`** — per-CPU `rx/tx_packets/bytes` guarded by a
  `u64_stats_sync`. Per-CPU so the two hot paths never contend on a shared cache
  line; the seqcount stops a reader from seeing a 64-bit counter torn mid-write
  on 32-bit builds (and compiles to nothing on 64-bit).
- **`struct vnet_priv`** — lives in the net_device's private tail
  (`netdev_priv`). Holds the `napi_struct`, the software RX ring (a fixed
  `sk_buff *` array plus free-running `head`/`tail` cursors and a spinlock), and
  the per-CPU stats pointer.
- **`vnet_setup`** — the constructor `alloc_netdev` runs: `ether_setup` for the
  Ethernet defaults, then our ops tables, `IFF_NOARP`, a few honest offload
  flags, MTU bounds, and a random MAC.
- **`vnet_dev_init` / `vnet_dev_uninit`** — allocate/free the per-CPU stats,
  bracketed to registration.
- **`vnet_open` / `vnet_stop`** — up/down. Open arms NAPI, starts the queue,
  raises carrier. Stop does the reverse **in the safe order** (stop producers →
  quiesce the consumer with `napi_disable` → drain and free leftover skbs).
- **`vnet_start_xmit`** — the TX path and the ring **producer**. Snapshots
  `skb->len` before releasing the skb (a use-after-free guard), `skb_orphan`s it
  to unthrottle the sender, reserves a ring slot under the lock (or drops on a
  full ring), bumps TX stats, and `napi_schedule`s — our stand-in for a
  hardware RX interrupt.
- **`vnet_poll`** — the NAPI callback and ring **consumer**. Pops up to `budget`
  skbs, runs `eth_type_trans` (the TX→RX transform), marks the checksum
  unnecessary, bumps RX stats, and `napi_gro_receive`s each frame up the stack.
  Below budget, it `napi_complete_done`s and re-checks the ring to close the
  lost-wakeup race.
- **`vnet_get_stats64`** — sums the per-CPU counters inside a `u64_stats` retry
  loop. `tx_dropped` is tracked via `dev_core_stats_tx_dropped_inc` and merged
  by the core, so it is deliberately not summed here.
- **`vnet_ethtool_ops`** and the **`net_device_ops`** table wire the callbacks
  in; `vnet_init` / `vnet_exit` allocate, `register_netdev`, and tear down.

### `Makefile`

A Kbuild wrapper: `obj-m += vnetdev.o` plus the recursive
`make -C $(KDIR) M=$(PWD) modules` invocation, and an `asm` target that runs the
exact clang commands.

## Assembly notes

Kernel C **cannot be compiled standalone to assembly** on a normal host — the
first `#include <linux/netdevice.h>` fails without a configured kernel tree. So,
per the lab convention, `asm/demo.c` extracts the driver's most instructive
piece of **pure logic**: the **RX-ring index arithmetic**. It declares its own
integer types, includes no headers, and therefore compiles to real,
inspectable assembly with the exact commands in the Makefile's `asm` target.

`demo.c` mirrors the driver's ring exactly:

- `rb_count`, `rb_is_full`, `rb_is_empty` — occupancy via modular unsigned
  subtraction (`head - tail`), valid across the 2^32 counter wrap.
- `rb_slot` — the power-of-two payoff: `index % SIZE` becomes **one `andl $63`**,
  never a divide. This is why the ring size must be a power of two.
- `rb_reserve` — the annotated centerpiece: the producer's "reserve a slot or
  return −1 if full" decision, exactly what `ndo_start_xmit` does under the
  spinlock, compiled to a subtract, an unsigned compare, a branch, an `and`, and
  an increment.
- `skb_has_headroom` — a bonus: the `data - head >= need` headroom check that
  governs whether `skb_push` can prepend a header without reallocating.

[`asm/demo.annotated.s`](asm/demo.annotated.s) is the `-O1` output
([`asm/demo.s`](asm/demo.s)) with a comment on essentially every instruction and
a header block on the SysV AMD64 ABI. The lessons it makes visible: every
"modulo" is a single AND, every "occupancy" a single subtract, booleans become
`cmp`+`setCC` with no branch, and clang rewrites the C's `>= 64` as `> 63`
(`ja`) to reuse an immediate. Compare with [`asm/demo.O0.s`](asm/demo.O0.s) (the
naive per-statement mapping) and [`asm/demo.O2.s`](asm/demo.O2.s) (where the
frame-pointer prologue disappears).

## Going further (the `Stretch:` goal)

- **Make it a real point-to-point pair like `veth`.** Register two devices and
  have each one's `ndo_start_xmit` deliver into the *other's* RX ring, so
  `vnet0`↔`vnet1` form a cable across network namespaces.
- **A configurable backend.** Add a `tun`-style character device so a userspace
  program supplies/consumes frames, or forward to a real device.
- **Multiqueue + XDP.** Use `alloc_etherdev_mqs` for several TX/RX queues,
  register an `ndo_bpf` hook, and run an XDP program at the RX ring.

What a **production** driver adds that this core omits: DMA descriptor rings and
`dma_map_single` instead of a pointer array; real interrupt handlers with
`napi_schedule_irqoff` and IRQ coalescing; `ndo_tx_timeout`, `ndo_change_mtu`,
`ndo_set_rx_mode` (multicast), VLAN/offload plumbing, `ethtool` link settings
and self-tests, and `devlink`/`ndo_bpf`. The `sk_buff`, NAPI, stats, and
`ethtool` mechanics here are the same ones those drivers use.

## References

- Kernel source: `drivers/net/loopback.c` (the minimal real loopback),
  `drivers/net/veth.c` (the paired variant), `drivers/net/dummy.c`.
- `Documentation/networking/napi.rst` — the NAPI contract, budget, and
  `napi_complete_done` semantics.
- `Documentation/networking/kapi.rst` and `include/linux/netdevice.h` — the
  `net_device_ops` / NAPI / carrier API surface.
- `include/linux/u64_stats_sync.h` — what the seqcount protects and why per-CPU.
- *Linux Device Drivers, 3rd ed.*, chapter 17 ("Network Drivers") — the `snull`
  example this project is a modern descendant of.
