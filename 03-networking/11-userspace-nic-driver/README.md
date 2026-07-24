# A userspace NIC driver (DPDK-style) 🟥

**What it is.** A polling userspace driver for the Intel 82599 10-GbE
controller (the `ixgbe` family), modelled on the [ixy](https://github.com/emmericp/ixy)
educational driver and the DPDK architecture. It takes the NIC *away from the
kernel*, memory-maps the device's registers, allocates DMA-able descriptor rings
and packet buffers out of hugepages, and drives receive/transmit entirely by
**polling** the rings from userspace — no interrupts, no kernel networking stack,
no syscalls on the data path. The bundled `app_reflector` demo swaps each frame's
MAC addresses and echoes it back out at line rate. This is a 🟥 giant, so what
ships here is an **honest teaching core**: the real 82599 register bring-up and
the complete RX/TX poll loops, with the production-only concerns (RSS, offloads,
jumbo frames, SR-IOV, adaptive interrupts) deliberately omitted and listed below.

> **This driver only runs on Linux, as root, on a real Intel 82599 NIC that has
> been unbound from the kernel.** It cannot run in this dev environment. The code
> is written to be correct on that target; the committed assembly is host-portable
> (clang cross-targets Linux). Everything you need to run it for real is in
> [Build & run](#build--run).

## What you'll learn

- **MMIO via `mmap`.** How mapping a PCI BAR resource file turns register access
  into pointer dereferences, and why those accesses must be `volatile` + fenced
  to preserve ordering (the *doorbell* problem).
- **DMA descriptor rings.** The producer/consumer ring shared between CPU and
  NIC, the `read` vs `writeback` descriptor formats, and the **DD (Descriptor
  Done)** status bit the driver polls.
- **Hugepages & DMA memory.** Why DMA needs *physically contiguous, pinned*
  memory, how to get it from `hugetlbfs`, and how to translate virtual to
  physical addresses via `/proc/self/pagemap`.
- **VFIO / UIO.** How a device is unbound from its kernel driver
  (`/sys/.../driver/unbind`), made a PCI **bus master** (config register 0x04,
  bit 2) so it may DMA, and why VFIO+IOMMU is the safe way to do all this.
- **Memory barriers & DMA coherence.** Why x86-64's TSO model lets a bare
  compiler barrier order MMIO and descriptor accesses, where a weakly-ordered
  ISA would need real fences, and why the PCIe root complex keeps DMA
  cache-coherent so we never flush caches.
- **Why polling beats interrupts at high PPS** — the central design choice of
  DPDK-style dataplanes.

Kernel/OS surfaces exercised: `mmap(MAP_HUGETLB)`, `mlock`, `pread`/`pwrite` on
`/sys/bus/pci/.../config`, `open`/`write` on `.../driver/unbind`, `mmap` of
`.../resource0`, `/proc/self/pagemap`. Instructions of note: the `pause`
spin-wait hint and the empty `#APP` compiler barrier in the generated assembly.

## Build & run

Platform: **Linux only**, kernel with `hugetlbfs` and (ideally) VFIO, **root**,
and a real Intel 82599 (`lspci | grep 82599`). Build first:

```bash
make            # builds ./ixy-reflector  (Linux; needs the headers above)
```

Bring up the environment and run (this is what `make run` also prints):

```bash
# 1) Reserve 2 MB hugepages and mount hugetlbfs where memory.c expects it.
echo 512 | sudo tee /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
sudo mkdir -p /mnt/huge
sudo mount -t hugetlbfs nodev /mnt/huge

# 2) Identify the NIC's PCI address (e.g. 0000:03:00.0).
lspci -Dnn | grep 82599

# 3) (Recommended) bind it to vfio-pci so the IOMMU sandboxes its DMA.
sudo modprobe vfio-pci
#   ...then bind via /sys/bus/pci/drivers/vfio-pci/bind (see driver docs).
#   This teaching core also works by simply unbinding ixgbe itself (it calls
#   pci_remove_driver for you), but then DMA uses RAW PHYSICAL addresses with
#   no IOMMU protection — only do that on a machine you can afford to crash.

# 4) Run the reflector on that port (root: it mmaps the BAR and locks memory).
sudo ./ixy-reflector 0000:03:00.0
# -> "link is up", then "reflected N.NNN Mpps" once traffic arrives.
```

Regenerate the teaching assembly on **any** host (no hardware needed):

```bash
make asm        # writes asm/demo.{O0.s,s,O2.s} from the header-free asm/demo.c
```

## How it works

The code is layered bottom-up; read it in this order:

- **`ixgbe_regs.h`** — the 82599 register map (byte offsets into BAR0), the bit
  definitions, and the 16-byte advanced RX/TX descriptor `union`s (`read` format
  the driver writes, `wb` writeback format the NIC writes). Every magic number is
  annotated with what it controls.
- **`memory.{h,c}`** — DMA memory from hugepages (`mmap(MAP_HUGETLB)` +
  `mlock`), virtual→physical translation via `/proc/self/pagemap`, and an O(1)
  free-stack **mempool** of packet buffers. This is where the "DMA needs pinned
  physical memory" story lives.
- **`pci.{h,c}`** — the three sysfs operations that take the card from the
  kernel: `pci_remove_driver` (unbind), `pci_enable_dma` (set bus-master), and
  `pci_map_bar0` (`mmap` the register window).
- **`driver.h`** — the device/queue structs and the **MMIO accessors**
  (`set_reg32`/`get_reg32`/`wait_set_reg32`). Read the header comment: it is the
  full account of MMIO ordering and DMA coherence on x86-64.
- **`ixgbe.c`** — the driver proper. `reset_and_init` performs the datasheet
  bring-up handshake (mask interrupts → global reset → wait for EEPROM/DMA init →
  link → configure RX/TX). `ixgbe_rx_batch` and `ixgbe_tx_batch` are the poll
  loops: check the DD bit, move buffers, ring the tail doorbell once per batch.
- **`app_reflector.c`** — the demo `main`: a single-core busy-poll loop that RX
  batches, swaps MACs, TX batches, and prints Mpps.

The **doorbell invariant** appears twice and is worth internalising: after
writing descriptors into the ring (plain cacheable RAM), the tail-register MMIO
write (`set_reg32` → `RDT`/`TDT`) tells the NIC to look. `set_reg32`'s
compiler barrier guarantees the descriptor stores are visible *before* the
doorbell store, so the NIC never reads a stale descriptor.

## Assembly notes

`asm/demo.c` is a **self-contained** extraction (its own types, zero
`#include`s) of the driver's most instructive pure-logic routine: the descriptor
ring poll — index advance + DD status-bit check + the volatile/barrier
reasoning. None of the real `.c` files compile without Linux headers, so per
CONVENTIONS.md the annotated assembly comes from this extraction; the routines
mirror `ixgbe_rx_batch`/`clean_tx` exactly.

[`asm/demo.annotated.s`](asm/demo.annotated.s) walks the `-O1` output
instruction by instruction. The three lessons it makes visible:

1. **The volatile status load stays inside the loop.** `movl 8(%rdi,%rbx),
   %r11d` is re-issued every iteration — the optimizer may not hoist it, because
   the NIC sets DD asynchronously via DMA and a cached copy would spin forever.
2. **The barrier costs zero instructions.** `load_load_barrier()` emits only the
   `#APP`/`#NO_APP` marker — on x86-64 TSO, ordering the DD check before the
   length read is a *compiler*-only constraint. A weak ISA would show a fence.
3. **Ring wrap is one `and`.** `(i + 1) & (size - 1)` compiles to a single
   `andl`, never a `div` — the payoff of power-of-two ring sizes.

Compare [`asm/demo.O0.s`](asm/demo.O0.s) (naive, everything spilled to the
stack) with [`asm/demo.O2.s`](asm/demo.O2.s) (branch-merged and tighter). At
`-O1` clang already merges `tx_clean`'s two `break`s and the loop-continue into
one flags-driven tail (a `negl`/`andl` branchless accumulate).

## Going further

- **Stretch: use VFIO instead of raw physical addresses.** Bind the device to
  `vfio-pci`, open the group in `/dev/vfio/`, program the IOMMU with
  `VFIO_IOMMU_MAP_DMA`, and use the returned IOVAs in descriptors. The IOMMU then
  confines the NIC to memory you explicitly mapped — a rogue or buggy descriptor
  can no longer DMA over arbitrary RAM. This is how DPDK/ixy run in production and
  is the single most important safety upgrade over the sysfs/UIO path here.
- **What production drivers add** that this core omits, and why:
  - **RSS + multiple queues** to scale across cores (one poll loop per core).
  - **Checksum/TSO/LRO offloads** and **jumbo frames** (multi-descriptor frames;
    this core rejects non-EOP descriptors on purpose).
  - **Adaptive interrupt moderation** — poll under load, sleep on an interrupt
    (`MSI-X`) when idle, to reclaim the power/latency that pure polling burns.
  - **Link-state and error handling** — timeouts on the link wait, recovery from
    `RXDCTL`/`TXDCTL` hangs, statistics for drops and errors.
  - **SR-IOV / virtual functions**, flow steering, and the full EEPROM/PHY code.

## References

- **ixy** — the ~1,000-line educational driver this is modelled on:
  <https://github.com/emmericp/ixy>, and the paper *"User Space Network Drivers"*
  (Emmerich et al.). Read `src/driver/ixgbe.c` alongside this project's `ixgbe.c`.
- **Intel 82599 10 GbE Controller Datasheet** — ground truth for every register
  offset and descriptor field used in `ixgbe_regs.h`.
- **DPDK** — the production incarnation of this architecture: <https://www.dpdk.org>.
- **Linux docs** — `Documentation/vfio.rst`, `Documentation/admin-guide/mm/hugetlbpage.rst`,
  and `man 2 mmap` / `man 7 pci`.
