// SPDX-License-Identifier: GPL-2.0
/* ===========================================================================
 * ramblk.c — a RAM-backed block device driven by the modern blk-mq layer.
 * ===========================================================================
 *
 * WHAT THIS IS
 * ------------
 * A loadable kernel module that registers a block device named /dev/myram0
 * whose "platters" are just a slab of kernel RAM (allocated with vmalloc).
 * You can partition it, mkfs it, mount it, dd to it — the kernel treats it
 * exactly like a disk, because from the block layer's point of view a disk is
 * nothing but "an object that turns a (sector, length, direction) request into
 * bytes moved." We satisfy each request with a memcpy. That's the whole trick.
 *
 * This is the same idea as the in-tree drivers/block/brd.c ("brd" = block RAM
 * disk, which backs /dev/ram0), but rewritten to be *read*, using the blk-mq
 * request path so you can see every moving part: the tag set, the per-request
 * callback, bio_vec iteration, page mapping, and gendisk registration.
 *
 * WHERE THIS RUNS
 * ---------------
 * Linux only, and you should load it inside a throwaway QEMU/KVM VM — a buggy
 * block driver can corrupt any filesystem mounted on it and can panic the host
 * kernel. It is written against the Linux 6.1 LTS block API. The block layer
 * churns (see the version notes at each call site); comments flag what moved.
 *
 * THE BLOCK-LAYER MENTAL MODEL (read this once, the code will make sense)
 * ----------------------------------------------------------------------
 *   gendisk        the disk object userspace sees as /dev/myram0. Holds the
 *                  capacity, the name, the fops, and a pointer to its queue.
 *   request_queue  the funnel every I/O flows through. blk-mq owns it.
 *   blk_mq_tag_set describes how the queue is staffed: how many hardware
 *                  queues, how deep, which callbacks. One tag set can back
 *                  several disks; here it backs one.
 *   request        a unit of work the block layer hands us. It carries a
 *                  starting sector, a length, a direction, and one-or-more
 *                  bios stitched together.
 *   bio            a description of a transfer: a target sector plus a vector
 *                  of <page, offset, len> tuples (bio_vec) naming the memory
 *                  on the *other* side of the copy (the page cache, a user
 *                  buffer pinned for O_DIRECT, etc.).
 *   bio_vec        one <page, offset, len>. Always within a single page, so a
 *                  single kmap + memcpy moves it.
 *
 * Our job in ->queue_rq is: for each bio_vec of the request, memcpy between
 * that page and the correct byte offset in our vmalloc store, where the offset
 * is (sector << 9). Everything else is registration boilerplate.
 * =========================================================================== */

#include <linux/module.h>       /* module_init/module_exit, MODULE_* macros    */
#include <linux/moduleparam.h>  /* module_param — expose size_mb as a load arg */
#include <linux/kernel.h>       /* pr_info / pr_err logging                     */
#include <linux/init.h>         /* __init / __exit section annotations          */
#include <linux/slab.h>         /* kzalloc for our small device struct          */
#include <linux/vmalloc.h>      /* vmalloc/vfree — the backing store            */
#include <linux/blkdev.h>       /* gendisk, request, set_capacity, SECTOR_*     */
#include <linux/blk-mq.h>       /* blk_mq_tag_set, ->queue_rq, blk_mq_alloc_*   */
#include <linux/bio.h>          /* bio_vec, req_iterator, rq_for_each_segment   */
#include <linux/highmem.h>      /* kmap_local_page / kunmap_local              */
#include <linux/hdreg.h>        /* struct hd_geometry for ->getgeo             */

/* ---------------------------------------------------------------------------
 * Module parameters. These appear in /sys/module/ramblk/parameters/ and can be
 * set at load time (`insmod ramblk.ko size_mb=64`). Defaults are chosen so the
 * device is big enough to mkfs but small enough to not stress a tiny VM.
 * ------------------------------------------------------------------------- */
static int size_mb = 16;
module_param(size_mb, int, 0444);       /* 0444 = world-readable in sysfs, not writable */
MODULE_PARM_DESC(size_mb, "Backing-store size in MiB (default 16)");

static int logical_block_size = 512;
module_param(logical_block_size, int, 0444);
MODULE_PARM_DESC(logical_block_size,
                 "Logical block size in bytes: 512 or 4096 (default 512)");

/* The kernel counts *everything* in 512-byte sectors regardless of the logical
 * block size we advertise. blk_rq_pos() returns a sector_t in these units, so
 * "bytes = sectors << SECTOR_SHIFT". SECTOR_SHIFT is 9 (2^9 == 512) and is
 * fixed forever — it is baked into the on-disk meaning of every LBA. */
#define RAMBLK_NAME "myram"             /* base name; the disk is myram0        */

/* ---------------------------------------------------------------------------
 * struct ramblk_dev — everything one device owns. One instance for myram0.
 *
 * We keep the tag set INSIDE the device (not a global) so its lifetime is tied
 * to the device and cleanup is a straight-line teardown in the reverse order
 * of setup. `data` is the RAM that pretends to be a platter.
 * ------------------------------------------------------------------------- */
struct ramblk_dev {
    void                *data;          /* vmalloc'd backing store              */
    size_t               size;          /* store size in BYTES (sector-aligned) */
    int                  major;         /* dynamically allocated block major    */
    struct blk_mq_tag_set tag_set;      /* staffing description for the queue   */
    struct gendisk      *disk;          /* the /dev/myram0 object               */
};

/* We only ever create one device in this teaching core. A production driver
 * (like brd) keeps a list and a module param for the count; extending this to
 * an array of N devices is the first "going further" exercise. */
static struct ramblk_dev *ramblk_device;

/* ===========================================================================
 * THE HOT PATH: turning a block request into memcpys.
 * =========================================================================== */

/* ---------------------------------------------------------------------------
 * ramblk_transfer — service ONE request by copying between its bio pages and
 * our RAM store.
 *
 * INVARIANTS this function relies on, and what breaks if they are violated:
 *   - blk_rq_pos(rq) is the start sector in 512-byte units. Shifting left by
 *     SECTOR_SHIFT gives the byte offset into our store. If we used the logical
 *     block size here instead of 9, every offset past sector 0 would be wrong
 *     and filesystems would see garbage.
 *   - rq_for_each_segment walks the request's bios and yields a bio_vec that
 *     lies WITHIN A SINGLE PAGE (the block layer splits segments at page
 *     boundaries). That single-page guarantee is what lets us kmap exactly one
 *     page per iteration and memcpy bv_len bytes safely.
 *   - The capacity we gave set_capacity() bounds where the block layer will
 *     ever address, so the range check below should never fire in normal
 *     operation — but a corrupt caller or an off-by-one in our own geometry
 *     would, and silently memcpy'ing past the vmalloc region is a kernel-memory
 *     corruption bug. So we check, and fail the I/O cleanly with -EIO instead.
 * ------------------------------------------------------------------------- */
static blk_status_t ramblk_transfer(struct request *rq)
{
    /* queuedata was wired to our device when we allocated the disk; this is how
     * a per-request callback finds its driver state without a global. */
    struct ramblk_dev *dev = rq->q->queuedata;

    struct bio_vec      bvec;           /* the current <page, offset, len>      */
    struct req_iterator iter;           /* rq_for_each_segment's cursor         */

    /* Byte offset of this request's start within the store. blk_rq_pos is in
     * 512-byte sectors; SECTOR_SHIFT == 9. See the invariant note above. */
    loff_t pos = blk_rq_pos(rq) << SECTOR_SHIFT;

    /* Direction is decided once per request: WRITE means "pages -> store". */
    const bool is_write = (rq_data_dir(rq) == WRITE);

    /* rq_for_each_segment expands to a nested loop over every bio in the
     * request and every single-page segment in each bio, filling `bvec`. */
    rq_for_each_segment(bvec, rq, iter) {
        unsigned int len = bvec.bv_len;         /* bytes in this segment        */

        /* Overflow-safe bounds check (this exact arithmetic is what asm/demo.c
         * extracts and annotates). We must NOT write `pos + len > size` because
         * pos+len could wrap a 64-bit value on a corrupt request and pass the
         * test. Comparing `len > size - pos` after proving `pos <= size` cannot
         * wrap. If it fails we abort the whole request with an I/O error. */
        if ((loff_t)pos > (loff_t)dev->size ||
            (size_t)len > dev->size - (size_t)pos) {
            pr_err_ratelimited("%s: out-of-range I/O at pos=%lld len=%u size=%zu\n",
                               RAMBLK_NAME, pos, len, dev->size);
            return BLK_STS_IOERR;
        }

        /* kmap_local_page gives us a kernel virtual address for this page. On
         * 64-bit kernels every page already has a permanent lowmem mapping so
         * this is nearly free, but on 32-bit HIGHMEM systems the page may not
         * be mapped at all — kmap_local_page sets up a temporary CPU-local
         * mapping. It replaced kmap_atomic (which disabled preemption); the
         * _local variant only pins us to the CPU, so it is preemptible and the
         * modern default. The mapping is valid until the matching kunmap_local
         * and must be released on the SAME CPU, in stack (LIFO) order. */
        void *kaddr = kmap_local_page(bvec.bv_page);
        void *buf   = kaddr + bvec.bv_offset;    /* start of THIS segment       */

        if (is_write)
            memcpy(dev->data + pos, buf, len);   /* page  -> store              */
        else
            memcpy(buf, dev->data + pos, len);   /* store -> page               */

        kunmap_local(kaddr);                     /* release the temp mapping     */

        pos += len;                              /* advance within the store     */
    }

    return BLK_STS_OK;
}

/* ---------------------------------------------------------------------------
 * ramblk_queue_rq — the ONE callback blk-mq requires. The block layer calls it
 * (in process or soft-irq context, with a tag reserved) to hand us a request.
 *
 * Contract:
 *   - We MUST call blk_mq_start_request() before touching the request data —
 *     it starts the timeout timer and moves the request to the "in flight"
 *     state so the layer accounts for it.
 *   - We complete synchronously here (RAM is instant; there is nothing to wait
 *     on), so we call blk_mq_end_request() to release the request and its tag.
 *   - The RETURN value tells blk-mq what to do with the *dispatch*, not the
 *     I/O: BLK_STS_OK means "accepted, don't call me again for this one."
 *     Returning BLK_STS_RESOURCE would ask the layer to back off and retry
 *     later (used by real drivers when a hardware ring is full). We never run
 *     out of memcpy capacity, so we always accept.
 * ------------------------------------------------------------------------- */
static blk_status_t ramblk_queue_rq(struct blk_mq_hw_ctx *hctx,
                                    const struct blk_mq_queue_data *bd)
{
    struct request *rq = bd->rq;
    blk_status_t status;

    blk_mq_start_request(rq);           /* arm timeout, mark in-flight          */

    /* We only understand plain data movement. Everything else — cache flushes,
     * discards, zeroing — we reject. A RAM disk has no volatile write cache, so
     * a filesystem's REQ_OP_FLUSH is a no-op we could also complete as OK; we
     * keep the teaching core to READ/WRITE and reject the rest explicitly so
     * nothing is silently mishandled. */
    switch (req_op(rq)) {
    case REQ_OP_READ:
    case REQ_OP_WRITE:
        status = ramblk_transfer(rq);
        break;
    default:
        status = BLK_STS_NOTSUPP;
        break;
    }

    blk_mq_end_request(rq, status);     /* complete + free the tag              */
    return BLK_STS_OK;                  /* dispatch consumed successfully       */
}

/* The tag set points at exactly one callback. blk-mq calls ->queue_rq for
 * every request; the other members (->init_request etc.) are optional and we
 * do not need per-request scratch space (cmd_size == 0). */
static const struct blk_mq_ops ramblk_mq_ops = {
    .queue_rq = ramblk_queue_rq,
};

/* ===========================================================================
 * gendisk operations.
 * =========================================================================== */

/* ---------------------------------------------------------------------------
 * ramblk_getgeo — answer the legacy HDIO_GETGEO ioctl with a fake CHS geometry.
 *
 * Cylinder/Head/Sector addressing died with BIOS int 13h, but fdisk and a few
 * partition tools still ask for it to place partitions on "cylinder"
 * boundaries. The kernel addresses the device purely by LBA (our set_capacity),
 * so the numbers here only need to multiply out to <= the capacity. We pick a
 * conventional 4 heads x 16 sectors and derive the cylinder count. This is
 * pure geometry theater, included to show where the block layer still exposes
 * the 1980s disk model.
 * ------------------------------------------------------------------------- */
static int ramblk_getgeo(struct block_device *bdev, struct hd_geometry *geo)
{
    struct ramblk_dev *dev = bdev->bd_disk->private_data;
    sector_t total_sectors = dev->size >> SECTOR_SHIFT;

    geo->heads     = 4;
    geo->sectors   = 16;                                 /* sectors per track   */
    geo->cylinders = total_sectors / (geo->heads * geo->sectors);
    geo->start     = 0;                                  /* LBA of this disk's 0 */
    return 0;
}

static const struct block_device_operations ramblk_fops = {
    .owner   = THIS_MODULE,             /* pin the module while the disk is open */
    .getgeo  = ramblk_getgeo,
};

/* ===========================================================================
 * Setup and teardown.
 * =========================================================================== */

/* ---------------------------------------------------------------------------
 * ramblk_alloc — build the one device: store, tag set, disk, and go live.
 *
 * The ordering matters and the error ladder unwinds it exactly in reverse, so
 * that a failure at any step leaves nothing half-registered (a leaked major or
 * a live-but-broken gendisk is a reboot-to-recover bug).
 * ------------------------------------------------------------------------- */
static int __init ramblk_alloc(void)
{
    struct ramblk_dev *dev;
    struct gendisk *disk;
    int ret;

    /* Validate the block size: it must be a power of two between 512 and the
     * page size, or the block layer will reject it. We only allow the two the
     * README documents to keep the teaching surface small. */
    if (logical_block_size != 512 && logical_block_size != 4096) {
        pr_err("%s: logical_block_size must be 512 or 4096\n", RAMBLK_NAME);
        return -EINVAL;
    }
    if (size_mb <= 0) {
        pr_err("%s: size_mb must be positive\n", RAMBLK_NAME);
        return -EINVAL;
    }

    /* kzalloc: a small (<1 page) physically-contiguous, zeroed allocation from
     * the slab. GFP_KERNEL is fine — module init runs in process context and
     * may sleep to reclaim memory. Zeroing means every pointer starts NULL, so
     * a partial-failure teardown can safely test-then-free. */
    dev = kzalloc(sizeof(*dev), GFP_KERNEL);
    if (!dev)
        return -ENOMEM;

    /* Round the requested size DOWN to a whole number of 512-byte sectors. A
     * block device whose capacity isn't a sector multiple is malformed. */
    dev->size = ((size_t)size_mb << 20) & ~((size_t)SECTOR_SIZE - 1);

    /* vmalloc, not kmalloc: we may want tens or hundreds of MiB, and kmalloc
     * needs PHYSICALLY contiguous pages, which the buddy allocator can rarely
     * find above a few MiB once memory is fragmented. vmalloc stitches together
     * arbitrary physical pages behind a single VIRTUALLY contiguous mapping, so
     * a large request succeeds. The trade-off is that vmalloc memory is NOT
     * physically contiguous, so we could never hand its physical address to a
     * DMA engine — fine here, because our "device" is a memcpy, not real DMA. */
    dev->data = vmalloc(dev->size);
    if (!dev->data) {
        ret = -ENOMEM;
        goto out_free_dev;
    }

    /* --- register a block-device major number -----------------------------
     * register_blkdev(0, name) asks the kernel to allocate a free major and
     * return it. The major identifies the driver in /proc/devices; minors
     * distinguish disks/partitions under it. Passing 0 (dynamic) avoids
     * colliding with a static major already in use. */
    ret = register_blkdev(0, RAMBLK_NAME);
    if (ret < 0) {
        pr_err("%s: register_blkdev failed: %d\n", RAMBLK_NAME, ret);
        goto out_free_store;
    }
    dev->major = ret;

    /* --- build the blk-mq tag set -----------------------------------------
     * The tag set is the "staffing plan" for the queue. Zero it first so every
     * field we don't set is a defined default. */
    memset(&dev->tag_set, 0, sizeof(dev->tag_set));
    dev->tag_set.ops           = &ramblk_mq_ops;    /* our ->queue_rq          */
    dev->tag_set.nr_hw_queues  = 1;                 /* one submission queue;
                                                     * real NVMe uses one per
                                                     * CPU for lock-free submit */
    dev->tag_set.queue_depth   = 128;               /* max in-flight requests;
                                                     * == number of tags        */
    dev->tag_set.numa_node     = NUMA_NO_NODE;      /* don't pin to a NUMA node */
    dev->tag_set.cmd_size      = 0;                 /* no per-request scratch   */
    dev->tag_set.flags         = BLK_MQ_F_SHOULD_MERGE; /* let the layer merge
                                                     * adjacent requests before
                                                     * handing them to us — big
                                                     * sequential I/O becomes
                                                     * fewer, larger memcpys.
                                                     * (Flag removed in ~6.11
                                                     * where merging is always
                                                     * on; drop it there.)      */
    dev->tag_set.driver_data   = dev;

    /* Allocate the tags and per-CPU software-queue plumbing. After this the
     * tag set is a reusable resource that a disk can be attached to. */
    ret = blk_mq_alloc_tag_set(&dev->tag_set);
    if (ret) {
        pr_err("%s: blk_mq_alloc_tag_set failed: %d\n", RAMBLK_NAME, ret);
        goto out_unregister;
    }

    /* --- allocate the gendisk AND its request_queue in one shot -----------
     * blk_mq_alloc_disk(set, queuedata) creates the request_queue backed by
     * `set`, allocates the gendisk, links them, and stashes `queuedata` where
     * rq->q->queuedata will find it (that's how ramblk_transfer got `dev`).
     *
     * VERSION NOTE: pre-5.14 you called blk_mq_init_queue() + alloc_disk()
     * separately; 5.14 merged them into blk_mq_alloc_disk(set, lock); 5.15
     * changed the 2nd arg to queuedata; 6.9 added a queue_limits arg
     * (blk_mq_alloc_disk(set, lim, queuedata)). This is the 6.1 form. */
    disk = blk_mq_alloc_disk(&dev->tag_set, dev);
    if (IS_ERR(disk)) {
        ret = PTR_ERR(disk);
        pr_err("%s: blk_mq_alloc_disk failed: %d\n", RAMBLK_NAME, ret);
        goto out_free_tagset;
    }
    dev->disk = disk;

    /* --- describe the disk to the block layer -----------------------------*/
    disk->major        = dev->major;
    disk->first_minor  = 0;
    disk->minors       = 1;             /* minors for this disk + its partitions;
                                         * 1 == whole disk only, no partition
                                         * scanning. Bump to e.g. 16 to allow
                                         * myram0p1.. partitions.               */
    disk->fops         = &ramblk_fops;
    disk->private_data = dev;           /* ->getgeo and friends recover `dev`   */
    snprintf(disk->disk_name, DISK_NAME_LEN, "%s0", RAMBLK_NAME); /* "myram0"   */

    /* Tell the block layer the device's block geometry. logical_block_size is
     * the smallest addressable/atomic unit the device claims; the page cache
     * and filesystems align to it. We advertise physical == logical (no larger
     * internal sector), which is honest for a RAM disk. */
    blk_queue_logical_block_size(disk->queue, logical_block_size);
    blk_queue_physical_block_size(disk->queue, logical_block_size);

    /* QUEUE_FLAG_NONROT: "non-rotational". Tells the I/O scheduler there is no
     * seek penalty, so it won't waste effort sorting requests by sector — the
     * right hint for RAM/SSD. QUEUE_FLAG_ADD_RANDOM cleared: don't feed this
     * device's completion timing into the kernel entropy pool (its timing is
     * not physically random, and stirring the pool on every I/O is pure
     * overhead). */
    blk_queue_flag_set(QUEUE_FLAG_NONROT, disk->queue);
    blk_queue_flag_clear(QUEUE_FLAG_ADD_RANDOM, disk->queue);

    /* THE capacity. set_capacity takes 512-byte sectors, ALWAYS, independent of
     * logical_block_size. Get this wrong and the device is either truncated or
     * reads off the end of the store. */
    set_capacity(disk, dev->size >> SECTOR_SHIFT);

    /* --- go live ----------------------------------------------------------
     * add_disk publishes /dev/myram0, kicks off partition scanning, and makes
     * the device openable. It can FAIL (since 5.15 it returns an int) — e.g. a
     * sysfs collision — and on failure the disk must NOT be del_gendisk'd, only
     * put_disk'd, which is exactly what the error ladder below does. */
    ret = add_disk(disk);
    if (ret) {
        pr_err("%s: add_disk failed: %d\n", RAMBLK_NAME, ret);
        goto out_put_disk;
    }

    ramblk_device = dev;
    pr_info("%s: /dev/%s0 ready — %zu MiB, %d-byte blocks, major %d\n",
            RAMBLK_NAME, RAMBLK_NAME, dev->size >> 20, logical_block_size,
            dev->major);
    return 0;

    /* ---- error unwind: exact reverse of setup ---------------------------- */
out_put_disk:
    put_disk(disk);                     /* frees disk AND its queue             */
out_free_tagset:
    blk_mq_free_tag_set(&dev->tag_set);
out_unregister:
    unregister_blkdev(dev->major, RAMBLK_NAME);
out_free_store:
    vfree(dev->data);
out_free_dev:
    kfree(dev);
    return ret;
}

/* ---------------------------------------------------------------------------
 * ramblk_exit — tear down in reverse. Order is a correctness requirement:
 *   1. del_gendisk stops new opens and waits for in-flight I/O to drain, so no
 *      ->queue_rq can be running when we free the store underneath it.
 *   2. put_disk drops the last gendisk reference, which also releases the
 *      request_queue that blk_mq_alloc_disk created.
 *   3. blk_mq_free_tag_set releases the tags — only safe once no queue uses it.
 *   4. unregister_blkdev returns the major.
 *   5. vfree/kfree release memory last, when nothing can reference it.
 * Freeing the store before del_gendisk would be a use-after-free reachable
 * from any process still doing I/O — a kernel-corruption bug and likely panic.
 * ------------------------------------------------------------------------- */
static void __exit ramblk_exit(void)
{
    struct ramblk_dev *dev = ramblk_device;

    if (!dev)
        return;

    del_gendisk(dev->disk);             /* 1: unpublish + drain                 */
    put_disk(dev->disk);                /* 2: free disk + queue                 */
    blk_mq_free_tag_set(&dev->tag_set); /* 3: free tags                         */
    unregister_blkdev(dev->major, RAMBLK_NAME); /* 4: return major              */
    vfree(dev->data);                   /* 5a: free the RAM store               */
    kfree(dev);                         /* 5b: free the device struct           */

    ramblk_device = NULL;
    pr_info("%s: unloaded\n", RAMBLK_NAME);
}

module_init(ramblk_alloc);
module_exit(ramblk_exit);

MODULE_LICENSE("GPL");                  /* GPL: needed to use GPL-only block symbols */
MODULE_AUTHOR("low-level-linux-lab");
MODULE_DESCRIPTION("Teaching RAM-backed block device on the blk-mq API");
MODULE_VERSION("1.0");
