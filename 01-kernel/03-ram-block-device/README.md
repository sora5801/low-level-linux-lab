# RAM-backed block device 🟧

**What it is.** A loadable Linux kernel module that registers a block device
`/dev/myram0` whose storage is just a slab of kernel RAM (`vmalloc`). It plugs
into the **blk-mq** request path: it builds a tag set, allocates a `gendisk` +
`request_queue`, sets the sector geometry, and services every I/O request by
`memcpy`-ing between the request's `bio_vec` pages and the backing store. From
the block layer's point of view it *is* a disk — you can `mkfs` it, mount it,
`dd` to it, benchmark it. This is a readable re-implementation of the in-tree
`drivers/block/brd.c` (which backs `/dev/ram0`), written to expose every moving
part of the modern block stack.

This is a **teaching core**, and honest about it: it implements the READ/WRITE
hot path, geometry, and registration completely and correctly, but deliberately
omits the features a production RAM disk grows (see
[Going further](#going-further)). It must be built and loaded on **Linux, inside
a throwaway QEMU/KVM VM** — a bug in a block driver can corrupt a mounted
filesystem or panic the kernel.

## What you'll learn

- The **blk-mq** object graph: `blk_mq_tag_set` → `request_queue` → `gendisk`,
  and the single `->queue_rq` callback that turns a `request` into work.
- How a `request` decomposes into `bio`s and single-page `bio_vec` segments, and
  how `rq_for_each_segment` walks them.
- **Sector geometry**: why the block layer counts in fixed 512-byte sectors
  (`blk_rq_pos`, `SECTOR_SHIFT`) no matter what logical block size you advertise,
  and how `set_capacity` / `blk_queue_logical_block_size` describe the disk.
- Mapping a `bio_vec`'s page with **`kmap_local_page`** (the modern, preemptible
  replacement for `kmap_atomic`) to get a pointer to `memcpy`.
- Why the backing store is **`vmalloc`** (virtually contiguous, large) rather
  than `kmalloc` (physically contiguous, small), and what that costs you.
- Kernel registration lifetime: `register_blkdev`, `blk_mq_alloc_disk`,
  `add_disk` (which can now fail), and unwinding it all in the exact reverse
  order so a partial failure leaks nothing.
- The **overflow-safe bounds check** on every segment — the arithmetic extracted
  into `asm/demo.c` and annotated instruction-by-instruction.

## Build & run (Linux, in a VM)

You need the kernel headers for your running kernel
(`sudo apt install linux-headers-$(uname -r)` or the distro equivalent).

```bash
cd 01-kernel/03-ram-block-device
make                       # -> ramblk.ko   (delegates to the kernel's Kbuild)

sudo insmod ramblk.ko size_mb=64          # optional: logical_block_size=4096
dmesg | tail -1                            # "myram: /dev/myram0 ready — 64 MiB..."
ls -l /dev/myram0                          # the device node appears

# use it like any disk:
sudo mkfs.ext4 /dev/myram0
sudo mount /dev/myram0 /mnt && echo hi | sudo tee /mnt/f && sudo umount /mnt

sudo rmmod ramblk                          # unload; RAM is reclaimed
```

### Benchmarking (compare against the in-tree /dev/ram0)

```bash
sudo modprobe brd rd_nr=1 rd_size=65536    # /dev/ram0, 64 MiB, for comparison

# raw sequential throughput with dd (O_DIRECT bypasses the page cache):
sudo dd if=/dev/zero of=/dev/myram0 bs=1M count=64 oflag=direct
sudo dd if=/dev/zero of=/dev/ram0   bs=1M count=64 oflag=direct

# mixed random I/O with fio, if installed:
sudo fio --name=ramtest --filename=/dev/myram0 --direct=1 --rw=randrw \
         --bs=4k --iodepth=32 --runtime=10s --time_based --group_reporting
```

Expect `myram0` to land in the same ballpark as `ram0` for large sequential
transfers (both are ultimately `memcpy`), with `brd` usually ahead on small
random I/O because it maps pages lazily per 4 KiB and skips a copy on aligned
access, whereas our teaching core always copies. That gap *is* the lesson about
where the cost goes.

### Regenerate the teaching assembly (works on any host)

```bash
make asm      # clang cross-targets Linux; no kernel headers needed
```

## How it works

- **`ramblk.c`** — the whole driver, built top-down:
  - `ramblk_transfer()` is the hot path. It converts `blk_rq_pos(rq) << 9` to a
    byte offset, walks the request's segments with `rq_for_each_segment`,
    `kmap_local_page`s each `bio_vec`'s page, and `memcpy`s in the direction the
    request asks. Each segment is range-checked with the overflow-safe test
    before the copy.
  - `ramblk_queue_rq()` is the blk-mq callback: `blk_mq_start_request`, dispatch
    READ/WRITE to `ramblk_transfer` (rejecting everything else), then
    `blk_mq_end_request`. Comments explain the return-value contract
    (`BLK_STS_OK` = "dispatch accepted", not "I/O succeeded").
  - `ramblk_getgeo()` fabricates a legacy CHS geometry for `HDIO_GETGEO`, the
    one place the 1980s disk model still leaks through.
  - `ramblk_alloc()` (module init) builds the store (`vmalloc`), the tag set
    (`blk_mq_alloc_tag_set`), the disk + queue (`blk_mq_alloc_disk`), sets the
    geometry/capacity, and goes live with `add_disk` — with an error ladder that
    unwinds every step in reverse.
  - `ramblk_exit()` tears down in the correctness-critical order: `del_gendisk`
    (drain in-flight I/O) → `put_disk` → `blk_mq_free_tag_set` →
    `unregister_blkdev` → `vfree`. Freeing the store any earlier would be a
    use-after-free reachable from an in-flight request.
- **`Makefile`** — the standard two-pass out-of-tree module Makefile: run
  directly it delegates to `make -C /lib/modules/$(uname -r)/build M=$(PWD)
  modules`; Kbuild re-reads it and sees only `obj-m := ramblk.o`. A separate
  `make asm` target regenerates the committed assembly.

The block API moves fast; each call site in `ramblk.c` notes what changed and
when (e.g. `blk_mq_alloc_disk`'s argument list across 5.14 → 6.9, `add_disk`
becoming fallible in 5.15, `BLK_MQ_F_SHOULD_MERGE` going away around 6.11). The
code targets **Linux 6.1 LTS**.

## Assembly notes

Kernel C cannot be compiled to assembly on a normal host: `ramblk.c` includes
`<linux/blk-mq.h>` and other headers that exist only inside a configured kernel
tree, so `clang -S ramblk.c` won't run here. Per the lab convention, the most
instructive **pure-logic** core is extracted into a self-contained
[`asm/demo.c`](asm/demo.c) — no `#include`s, its own `typedef`s — holding the
sector↔byte conversion and the overflow-safe bounds check that `ramblk_transfer`
runs on every segment.

[`asm/demo.annotated.s`](asm/demo.annotated.s) annotates the `-O1` output
instruction by instruction. The payoff is `ramblk_range_ok`: the source has two
separate `if` checks, but clang computes `store_bytes - off` **once** and reuses
it — the subtraction's *borrow flag* answers check (1) (`off > store_bytes`) and
its *value* feeds check (2) (`nbytes > store_bytes - off`) — then ORs the two
failure bits into a **single** branch. The annotation also shows why collapsing
the two checks stays correct even when the first one's failure makes the
intermediate subtraction wrap. Compare the four files:

- [`asm/demo.O0.s`](asm/demo.O0.s) — naive: each `if` is its own
  compare-and-branch, every value spilled to the stack. Easiest to trace.
- [`asm/demo.s`](asm/demo.s) — `-O1`, the annotated baseline.
- [`asm/demo.O2.s`](asm/demo.O2.s) — identical to `-O1` minus the frame-pointer
  prologue.
- [`asm/demo.annotated.s`](asm/demo.annotated.s) — the hand-commented walkthrough.

Regenerate with `make asm` (any host — clang cross-targets Linux SysV).

## Going further

The `Stretch:` from the project list, and what a production RAM disk adds:

- **A write-back cache / device-mapper target.** Layer a small dirty-page cache
  in front of the store, or expose the device as a `dm` target so it can be
  stacked (e.g. as the fast tier under `dm-cache`, or the origin under
  `dm-writecache`). That means implementing the `target_type` ops
  (`.ctr`/`.dtr`/`.map`) instead of a bare gendisk.
- What **`brd`** does that this core does not: it allocates backing pages
  **lazily** in a radix tree (an untouched RAM disk uses no memory until
  written), handles `REQ_OP_DISCARD` by freeing pages, and avoids a copy on
  page-aligned access. Adding lazy allocation is the natural next step.
- **Multiple devices.** Turn the single `ramblk_device` into an array driven by
  an `nr_devices` module param, bumping `disk->minors` for partition support
  (`myram0p1`…). The registration code is already structured for it.
- **Blocking / async completion.** Real drivers return from `->queue_rq`
  immediately and complete later via `blk_mq_complete_request` from an IRQ; this
  core completes synchronously because a `memcpy` never waits.

## References

- Linux source: `drivers/block/brd.c` (the real RAM disk) and
  `drivers/block/null_blk/` (the canonical blk-mq skeleton to read).
- `include/linux/blk-mq.h`, `include/linux/blkdev.h` — the API this uses.
- Documentation/block/ in the kernel tree, esp. the blk-mq overview.
- `man 4 ram`, `man 8 fio`, `man 1 dd` for the benchmarking side.
