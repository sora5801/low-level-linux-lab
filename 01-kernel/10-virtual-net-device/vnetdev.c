// SPDX-License-Identifier: GPL-2.0
/* ===========================================================================
 * vnetdev.c — a software network interface: a loopback-style virtual NIC.
 * ===========================================================================
 *
 * WHAT THIS IS
 * ------------
 * A minimal but *complete and correct* teaching driver for a virtual Ethernet
 * device — the same family as `lo`, `veth`, `tun`, and `dummy`. It registers a
 * `struct net_device` named vnet0 with the kernel networking core. Anything the
 * stack "transmits" out of vnet0 we loop straight back in as a "received"
 * frame, so a single interface behaves like a wire soldered to itself:
 *
 *        user socket / IP stack
 *              |  send()               ^  recv()
 *              v                       |
 *        dev_queue_xmit ---> [ our ndo_start_xmit ]
 *                                   |  push sk_buff into a software RX ring
 *                                   |  napi_schedule()
 *                                   v
 *                            NET_RX softirq  --->  [ our NAPI poll() ]
 *                                   |  pop sk_buff, eth_type_trans()
 *                                   v
 *                            napi_gro_receive() ---> up the stack again
 *
 * The point is to see the FULL life of a `struct sk_buff` and to exercise the
 * NAPI polling contract on a device that has no real hardware or interrupts.
 *
 * WHY NAPI FOR A SOFTWARE DEVICE?
 * -------------------------------
 * The kernel's simplest RX entry point is netif_rx()/__netif_rx(): "here is a
 * frame, queue it." The real loopback driver (drivers/net/loopback.c) uses
 * exactly that. We deliberately go one level deeper and implement the NAPI
 * *poll* model that every serious NIC driver uses, because that is the concept
 * worth learning: instead of taking one interrupt (or one call) per packet, the
 * driver is asked to pull up to `budget` packets in a single softirq pass. That
 * amortizes overhead and prevents receive-livelock under load. Our "interrupt"
 * is simply napi_schedule() raised from the xmit path; our "DMA ring" is the
 * software sk_buff ring below.
 *
 * TEACHING-CORE SCOPE (be honest — see README for the full list)
 * --------------------------------------------------------------
 *  * ONE device (vnet0), loopback semantics, single hardware queue.
 *  * A fixed-size software RX ring instead of a DMA descriptor ring.
 *  * No offloads beyond advertising a couple of feature flags; no ethtool
 *    link settings, no multiqueue, no XDP. Those are named in the README.
 * Everything on the sk_buff / NAPI / stats / ethtool path, however, is the real
 * kernel API used the way a production driver uses it.
 *
 * BUILD & RUN: this is Linux kernel code. It does not and cannot build on the
 * host that generated the asm/ files — see README.md for the QEMU/WSL recipe.
 * ===========================================================================
 */

#include <linux/module.h>        /* module_init/exit, MODULE_* macros           */
#include <linux/kernel.h>        /* pr_info, container_of                       */
#include <linux/init.h>          /* __init / __exit section annotations         */
#include <linux/netdevice.h>     /* struct net_device, net_device_ops, NAPI     */
#include <linux/etherdevice.h>   /* ether_setup, eth_type_trans, eth_hw_addr_*  */
#include <linux/skbuff.h>        /* struct sk_buff and its accessors            */
#include <linux/ethtool.h>       /* struct ethtool_ops, ethtool_op_get_link     */
#include <linux/if_ether.h>      /* ETH_HLEN, ETH_DATA_LEN, ETH_MIN_MTU         */
#include <linux/spinlock.h>      /* spinlock_t, spin_lock_irqsave               */
#include <linux/u64_stats_sync.h>/* u64_stats_sync: torn-free 64-bit counters   */
#include <linux/percpu.h>        /* this_cpu_ptr, per_cpu_ptr, for_each_*_cpu   */
#include <linux/types.h>         /* u32, u64, netdev_tx_t, bool                 */
#include <linux/errno.h>         /* ENOMEM and friends                          */

#define DRV_NAME    "vnet"       /* module + ethtool driver name                */
#define DRV_VERSION "1.0"        /* reported via `ethtool -i vnet0`             */

/* ---------------------------------------------------------------------------
 * Software RX ring geometry.
 *
 * Capacity MUST be a power of two so that "index modulo capacity" is a single
 * bitwise AND (see asm/demo.c for the extracted arithmetic and its assembly).
 * 64 slots is plenty for a loopback: the NAPI poll drains it almost as fast as
 * xmit fills it. RING_MASK is the low-6-bits mask used to map a free-running
 * counter to a physical array slot.
 * ------------------------------------------------------------------------- */
#define VNET_RING_ORDER 6u
#define VNET_RING_SIZE  (1u << VNET_RING_ORDER)   /* 64 slots                  */
#define VNET_RING_MASK  (VNET_RING_SIZE - 1u)     /* 63                        */

/* ---------------------------------------------------------------------------
 * Per-CPU statistics.
 *
 * WHY PER-CPU, and WHY u64_stats_sync?
 *   - Two hot paths update counters: ndo_start_xmit (can run on ANY CPU) and
 *     the NAPI poll (runs on ONE CPU at a time). If every CPU shared one set of
 *     counters, each increment would need an atomic or a lock and would bounce
 *     that cache line between cores — the classic scalability killer. Instead
 *     each CPU bumps its OWN counters with no contention, and the rare reader
 *     (ndo_get_stats64) sums across CPUs.
 *   - u64_stats_sync guards a 64-bit counter against being read *torn* on
 *     32-bit CPUs, where a u64 write is two 32-bit stores. The writer brackets
 *     its updates with begin()/end(); the reader retries if it observed a write
 *     in progress. On 64-bit builds the sync object compiles to nothing (a u64
 *     store is already atomic), so this is free where it can be.
 *
 * The member MUST be named `syncp` — netdev_alloc_pcpu_stats() looks for that
 * name to call u64_stats_init() on each CPU's copy.
 * ------------------------------------------------------------------------- */
struct vnet_pcpu_stats {
	u64 rx_packets;
	u64 rx_bytes;
	u64 tx_packets;
	u64 tx_bytes;
	struct u64_stats_sync syncp;
};

/* ---------------------------------------------------------------------------
 * Private per-device state. It lives in the tail of the net_device allocation
 * (alloc_netdev reserves sizeof(struct vnet_priv) bytes there) and is reached
 * with netdev_priv(dev). Keeping it inline avoids a second allocation and a
 * pointer chase on the hot path.
 * ------------------------------------------------------------------------- */
struct vnet_priv {
	struct net_device *dev;      /* back-pointer, so poll() can find the netdev  */
	struct napi_struct napi;     /* the NAPI context the poll loop runs under    */

	/* The software RX ring. Producer = ndo_start_xmit (possibly several CPUs);
	 * consumer = NAPI poll (one CPU). head and tail are FREE-RUNNING unsigned
	 * counters (they only increment and wrap at 2^32); the physical slot is
	 * `counter & VNET_RING_MASK`. Occupancy is `head - tail` in modular u32
	 * arithmetic — correct across the wrap. See asm/demo.c. */
	spinlock_t ring_lock;        /* serializes producers vs the consumer         */
	u32 head;                    /* producer cursor (next slot to fill)          */
	u32 tail;                    /* consumer cursor (next slot to drain)         */
	struct sk_buff *ring[VNET_RING_SIZE];

	struct vnet_pcpu_stats __percpu *pcpu_stats;
};

/* Forward declarations for the ops tables further down. */
static const struct net_device_ops vnet_netdev_ops;
static const struct ethtool_ops   vnet_ethtool_ops;

/* The single device this module manages. A real driver keyed off a bus/probe
 * would track many; one global keeps the teardown path obvious. */
static struct net_device *vnet_dev;

/* ===========================================================================
 * ndo_init / ndo_uninit — allocate and free the per-CPU stats.
 *
 * register_netdev() calls ndo_init() exactly once, early, while it still holds
 * the RTNL lock and before the device is visible or up. If it fails, register
 * unwinds and ndo_uninit() is NOT called, so ndo_init must clean up after
 * itself on error. ndo_uninit() is the mirror, called during unregister.
 * ===========================================================================
 */
static int vnet_dev_init(struct net_device *dev)
{
	struct vnet_priv *priv = netdev_priv(dev);

	/* netdev_alloc_pcpu_stats allocates one struct per possible CPU, zeroes
	 * them, and calls u64_stats_init(&copy->syncp) on each. Returns NULL on
	 * OOM — the only failure register_netdev must be able to unwind from here. */
	priv->pcpu_stats = netdev_alloc_pcpu_stats(struct vnet_pcpu_stats);
	if (!priv->pcpu_stats)
		return -ENOMEM;

	return 0;
}

static void vnet_dev_uninit(struct net_device *dev)
{
	struct vnet_priv *priv = netdev_priv(dev);

	free_percpu(priv->pcpu_stats);   /* release every CPU's counter block */
}

/* ===========================================================================
 * ndo_open / ndo_stop — the interface's up/down transitions (`ip link set up`).
 * ===========================================================================
 */
static int vnet_open(struct net_device *dev)
{
	struct vnet_priv *priv = netdev_priv(dev);

	/* Arm NAPI: after this the core may call our poll() when we napi_schedule.
	 * Enabling BEFORE we start the TX queue avoids the window where a frame is
	 * queued but poll() is not yet allowed to run. */
	napi_enable(&priv->napi);

	/* Tell the stack it may hand us frames: the TX queue leaves the STOPPED
	 * state and dev_queue_xmit will start calling ndo_start_xmit. */
	netif_start_queue(dev);

	/* Assert link/carrier so `ip link` shows the interface as running and the
	 * routing layer will actually use it. get_link (ethtool) reflects this. */
	netif_carrier_on(dev);

	netdev_info(dev, "up: NAPI armed, TX queue started\n");
	return 0;
}

static int vnet_stop(struct net_device *dev)
{
	struct vnet_priv *priv = netdev_priv(dev);
	unsigned long flags;

	/* Bring the interface down in the SAFE order:
	 *  1. carrier off + stop the TX queue  -> no NEW producers enter xmit. */
	netif_carrier_off(dev);
	netif_stop_queue(dev);

	/*  2. napi_disable() -> waits for any in-flight poll() to finish and blocks
	 *     future scheduling, so the CONSUMER is now quiesced too. After this,
	 *     nothing touches the ring concurrently and it is safe to drain. */
	napi_disable(&priv->napi);

	/*  3. Free any sk_buffs still sitting in the ring. If we skipped this, every
	 *     queued-but-undelivered frame would leak its skb (and its pages) on
	 *     interface-down. The lock is not strictly needed now (both ends are
	 *     quiesced) but taking it keeps the access pattern uniform and cheap. */
	spin_lock_irqsave(&priv->ring_lock, flags);
	while (priv->head != priv->tail) {
		struct sk_buff *skb = priv->ring[priv->tail & VNET_RING_MASK];

		priv->tail++;
		dev_kfree_skb(skb);   /* drop a reference; frees when it hits zero */
	}
	spin_unlock_irqrestore(&priv->ring_lock, flags);

	netdev_info(dev, "down: TX stopped, NAPI disabled, ring drained\n");
	return 0;
}

/* ===========================================================================
 * ndo_start_xmit — the transmit path, and the PRODUCER for the RX ring.
 *
 * The stack calls this to send `skb` out of `dev`. Ownership of the skb passes
 * TO us: we must either free it or hand it onward, and we must return an
 * NETDEV_TX_* code (never a negative errno). We NEVER free the skb on the
 * success path here — we pass it to the RX ring, and poll() delivers it up the
 * stack, which frees it when it is done.
 *
 * CONTEXT: called with the transmit queue's __xmit_lock held and BH disabled,
 * so this_cpu_ptr() is safe (we cannot migrate CPUs mid-update) and we must not
 * sleep.
 * ===========================================================================
 */
static netdev_tx_t vnet_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct vnet_priv *priv = netdev_priv(dev);
	struct vnet_pcpu_stats *st;
	unsigned long flags;

	/* Snapshot the frame length NOW, before we surrender the skb to the ring.
	 * The moment we unlock, a poll() on another CPU may dequeue and even free
	 * this skb; reading skb->len afterwards would be a use-after-free. Copying
	 * the scalar first is the invariant that makes the stats update safe. */
	unsigned int len = skb->len;

	/* Detach the skb from the sending socket. skb->destructor holds a charge
	 * against the sender's send-buffer budget (sk_wmem_alloc); until it runs the
	 * sender can be throttled. For a loopback we want the sender to keep going
	 * regardless of how deep our ring is, so we release that charge up front —
	 * exactly what drivers/net/loopback.c does. */
	skb_orphan(skb);

	/* PRODUCE: reserve a ring slot under the lock. Occupancy is head - tail in
	 * modular u32 arithmetic; >= VNET_RING_SIZE means full. See asm/demo.c for
	 * this exact comparison compiled to a single subtract + unsigned compare. */
	spin_lock_irqsave(&priv->ring_lock, flags);
	if ((u32)(priv->head - priv->tail) >= VNET_RING_SIZE) {
		/* Ring full: we cannot accept the frame. Drop it. This is the software
		 * analogue of a NIC's RX FIFO overrun. */
		spin_unlock_irqrestore(&priv->ring_lock, flags);
		dev_kfree_skb_any(skb);          /* free in any context (may be softirq) */

		/* tx_dropped lives in the netdev core's lazily-allocated per-CPU stats;
		 * dev_get_stats() merges it into the counters userspace sees, so we do
		 * not need to report it ourselves in ndo_get_stats64. */
		dev_core_stats_tx_dropped_inc(dev);
		return NETDEV_TX_OK;             /* a drop is still a "handled" xmit */
	}
	priv->ring[priv->head & VNET_RING_MASK] = skb;  /* store at head's slot */
	priv->head++;                                   /* publish the advance  */
	spin_unlock_irqrestore(&priv->ring_lock, flags);

	/* TX accounting on this CPU's private counters (torn-free via syncp). */
	st = this_cpu_ptr(priv->pcpu_stats);
	u64_stats_update_begin(&st->syncp);
	st->tx_packets++;
	st->tx_bytes += len;                 /* the length we snapshotted above */
	u64_stats_update_end(&st->syncp);

	/* Raise our "interrupt": ask the core to run poll() in NET_RX softirq. If
	 * NAPI is already scheduled this is a cheap no-op. This is where a real NIC
	 * would instead get a hardware RX IRQ. */
	napi_schedule(&priv->napi);

	return NETDEV_TX_OK;
}

/* ===========================================================================
 * vnet_poll — the NAPI poll callback, and the CONSUMER of the RX ring.
 *
 * The core calls this from the NET_RX softirq after napi_schedule(). Contract:
 *   - Deliver AT MOST `budget` packets, then return the number actually
 *     delivered (`work_done`).
 *   - If we delivered FEWER than budget (i.e. we drained the ring), we must call
 *     napi_complete_done() to take ourselves off the poll list and re-arm.
 *   - If we hit budget exactly, we return budget WITHOUT completing, and the
 *     core will poll us again — this is the fairness mechanism that stops one
 *     device from monopolizing the softirq.
 * ===========================================================================
 */
static int vnet_poll(struct napi_struct *napi, int budget)
{
	/* Recover our private state from the embedded napi_struct. container_of
	 * does pointer arithmetic: (priv) = (address of napi) - offsetof(napi). */
	struct vnet_priv *priv = container_of(napi, struct vnet_priv, napi);
	struct vnet_pcpu_stats *st;
	unsigned long flags;
	int work_done = 0;

	/* CONSUME up to `budget` frames from the ring. */
	while (work_done < budget) {
		struct sk_buff *skb;
		unsigned int len;

		/* Pop one skb under the lock. Empty == head == tail. */
		spin_lock_irqsave(&priv->ring_lock, flags);
		if (priv->head == priv->tail) {
			spin_unlock_irqrestore(&priv->ring_lock, flags);
			break;                        /* ring drained; leave the loop */
		}
		skb = priv->ring[priv->tail & VNET_RING_MASK];
		priv->tail++;
		spin_unlock_irqrestore(&priv->ring_lock, flags);

		/* Count the full on-the-wire length (L2 header included) BEFORE
		 * eth_type_trans strips it, mirroring what a real NIC would report. */
		len = skb->len;

		/* Turn a just-"transmitted" frame into a "received" one. eth_type_trans:
		 *   - reads the 14-byte Ethernet header at skb->data,
		 *   - pulls it off the front (skb->data += ETH_HLEN, skb->len -= ETH_HLEN),
		 *   - sets skb->dev = priv->dev and skb->pkt_type (HOST/BROADCAST/...),
		 *   - returns the EtherType for skb->protocol so IP/ARP/etc. get it.
		 * This is THE canonical "prepare an RX skb" call every Ethernet driver
		 * makes; skipping it would leave the stack unable to demux the frame. */
		skb->protocol = eth_type_trans(skb, priv->dev);

		/* We produced this frame locally and never corrupted it, so its checksum
		 * is trivially valid. Telling the stack CHECKSUM_UNNECESSARY skips a
		 * pointless verification pass — the same shortcut loopback takes. */
		skb->ip_summed = CHECKSUM_UNNECESSARY;

		/* RX accounting on this CPU's counters. */
		st = this_cpu_ptr(priv->pcpu_stats);
		u64_stats_update_begin(&st->syncp);
		st->rx_packets++;
		st->rx_bytes += len;
		u64_stats_update_end(&st->syncp);

		/* Hand the skb up the receive stack, GRO-aware. napi_gro_receive may
		 * coalesce consecutive same-flow segments before pushing them to IP;
		 * ownership of the skb passes to the stack here — we must not touch it
		 * again. This is the last stop on this sk_buff's life inside the driver. */
		napi_gro_receive(napi, skb);
		work_done++;
	}

	/* We left the loop with fewer than budget delivered, so the ring was empty
	 * when we looked. Complete NAPI: take ourselves off the poll list. */
	if (work_done < budget) {
		/* napi_complete_done returns false if it decided to keep us scheduled
		 * (e.g. busy-poll); when it returns true we are truly off the list and
		 * must guard against the lost-wakeup race: a producer that enqueued and
		 * called napi_schedule() in the tiny window between our empty-check and
		 * napi_complete_done() could otherwise be missed forever. Re-check the
		 * ring and reschedule ourselves if it refilled. */
		if (napi_complete_done(napi, work_done)) {
			bool more;

			spin_lock_irqsave(&priv->ring_lock, flags);
			more = (priv->head != priv->tail);
			spin_unlock_irqrestore(&priv->ring_lock, flags);

			if (more)
				napi_schedule(napi);
		}
	}

	return work_done;
}

/* ===========================================================================
 * ndo_get_stats64 — report counters to userspace (`ip -s link`, /proc/net/dev).
 *
 * Sum every CPU's private counters into the 64-bit rtnl_link_stats64 the core
 * hands us. We read each CPU's block inside a u64_stats_fetch retry loop so a
 * concurrent writer on that CPU (32-bit torn write) is never observed halfway.
 * tx_dropped is intentionally absent: it lives in dev->core_stats and is merged
 * for us by dev_get_stats() after this returns.
 * ===========================================================================
 */
static void vnet_get_stats64(struct net_device *dev,
			     struct rtnl_link_stats64 *stats)
{
	struct vnet_priv *priv = netdev_priv(dev);
	int cpu;

	for_each_possible_cpu(cpu) {
		const struct vnet_pcpu_stats *st = per_cpu_ptr(priv->pcpu_stats, cpu);
		u64 rx_packets, rx_bytes, tx_packets, tx_bytes;
		unsigned int start;

		do {
			/* Take a consistent snapshot: if a writer bumped the seqcount
			 * between begin and retry, loop and read again. */
			start = u64_stats_fetch_begin(&st->syncp);
			rx_packets = st->rx_packets;
			rx_bytes   = st->rx_bytes;
			tx_packets = st->tx_packets;
			tx_bytes   = st->tx_bytes;
		} while (u64_stats_fetch_retry(&st->syncp, start));

		stats->rx_packets += rx_packets;
		stats->rx_bytes   += rx_bytes;
		stats->tx_packets += tx_packets;
		stats->tx_bytes   += tx_bytes;
	}
}

/* The operations table the networking core dispatches through. Leaving a slot
 * NULL means "not supported"; the core has sane fallbacks for the optional ones. */
static const struct net_device_ops vnet_netdev_ops = {
	.ndo_init            = vnet_dev_init,     /* alloc per-CPU stats           */
	.ndo_uninit          = vnet_dev_uninit,   /* free them                     */
	.ndo_open            = vnet_open,         /* `ip link set vnet0 up`        */
	.ndo_stop            = vnet_stop,         /* `ip link set vnet0 down`      */
	.ndo_start_xmit      = vnet_start_xmit,   /* TX -> loop into the RX ring   */
	.ndo_get_stats64     = vnet_get_stats64,  /* counters for userspace        */
	.ndo_set_mac_address = eth_mac_addr,      /* generic, validates + copies   */
	.ndo_validate_addr   = eth_validate_addr, /* reject a bad MAC on open      */
};

/* ===========================================================================
 * ethtool_ops — what `ethtool vnet0` / `ethtool -i vnet0` see.
 * ===========================================================================
 */
static void vnet_get_drvinfo(struct net_device *dev,
			     struct ethtool_drvinfo *info)
{
	/* strscpy is the bounded, always-NUL-terminating string copy (the safe
	 * replacement for strlcpy/strncpy). These fields feed `ethtool -i`. */
	strscpy(info->driver,  DRV_NAME,    sizeof(info->driver));
	strscpy(info->version, DRV_VERSION, sizeof(info->version));
	strscpy(info->bus_info, "software", sizeof(info->bus_info));
}

/* Expose the software RX ring depth through the standard ring-parameter API, so
 * `ethtool -g vnet0` reports it. It is fixed, so we advertise max == current and
 * offer no set_ringparam (the ring cannot be resized at runtime here). */
static void vnet_get_ringparam(struct net_device *dev,
			       struct ethtool_ringparam *ring,
			       struct kernel_ethtool_ringparam *kring,
			       struct netlink_ext_ack *extack)
{
	ring->rx_max_pending = VNET_RING_SIZE;
	ring->rx_pending     = VNET_RING_SIZE;
	/* TX has no separate device ring in this design (we loop immediately). */
	ring->tx_max_pending = 0;
	ring->tx_pending     = 0;
}

static const struct ethtool_ops vnet_ethtool_ops = {
	.get_drvinfo  = vnet_get_drvinfo,
	.get_link     = ethtool_op_get_link,   /* generic: returns netif_carrier_ok */
	.get_ringparam = vnet_get_ringparam,
};

/* ===========================================================================
 * vnet_setup — the constructor alloc_netdev() runs on the fresh net_device.
 *
 * alloc_netdev() zeroes the device and then calls this to fill in the type-
 * specific defaults BEFORE the private area is touched. We start from the
 * Ethernet template and then override the fields that make us a virtual NIC.
 * ===========================================================================
 */
static void vnet_setup(struct net_device *dev)
{
	/* ether_setup() stamps in the Ethernet defaults: type ARPHRD_ETHER,
	 * hard_header_len = ETH_HLEN (14), addr_len = ETH_ALEN (6), mtu = 1500,
	 * a broadcast address of all-ones, and the Ethernet header_ops. This is why
	 * the stack knows to build 14-byte L2 headers for us. */
	ether_setup(dev);

	dev->netdev_ops  = &vnet_netdev_ops;   /* our ndo_* dispatch table       */
	dev->ethtool_ops = &vnet_ethtool_ops;  /* our ethtool dispatch table     */

	/* Loopback semantics: there is no peer to ARP for, so suppress ARP. Frames
	 * we send come straight back regardless of L2 addressing. */
	dev->flags |= IFF_NOARP;

	/* Advertise a few honest offloads. Because we loop skbs in-kernel without
	 * ever serializing them onto a wire, we can accept scatter-gather and
	 * fraglist skbs and "checksum" everything for free. These let the stack
	 * hand us larger, un-linearized skbs — less copying on the send side. */
	dev->features    |= NETIF_F_SG | NETIF_F_FRAGLIST | NETIF_F_HW_CSUM |
			    NETIF_F_HIGHDMA;
	dev->hw_features |= dev->features;

	/* MTU bounds for `ip link set vnet0 mtu N`. ETH_MIN_MTU (68) .. 1500. */
	dev->min_mtu = ETH_MIN_MTU;
	dev->max_mtu = ETH_DATA_LEN;

	/* Give the device a random, locally-administered, valid unicast MAC. Without
	 * a real address the stack would refuse to bring the interface up. */
	eth_hw_addr_random(dev);
}

/* ===========================================================================
 * Module init / exit — register the device with the networking core.
 * ===========================================================================
 */
static int __init vnet_init(void)
{
	struct vnet_priv *priv;
	int err;

	/* Allocate the net_device plus sizeof(struct vnet_priv) of private tail.
	 *   - name template "vnet%d" -> the core fills in the lowest free unit
	 *     (vnet0) because we pass NET_NAME_ENUM.
	 *   - vnet_setup is our constructor (runs before we get the pointer back).
	 * On success `dev` is fully formed but NOT yet registered/visible. */
	vnet_dev = alloc_netdev(sizeof(struct vnet_priv), "vnet%d",
				NET_NAME_ENUM, vnet_setup);
	if (!vnet_dev)
		return -ENOMEM;

	priv = netdev_priv(vnet_dev);
	priv->dev = vnet_dev;                 /* back-pointer for poll()          */
	spin_lock_init(&priv->ring_lock);
	priv->head = 0;
	priv->tail = 0;

	/* Attach our NAPI context to the device. The 3-argument form (kernel >= 6.1)
	 * uses the default weight NAPI_POLL_WEIGHT (64) as the poll budget. On older
	 * kernels this took a 4th `weight` argument — see the README note. */
	netif_napi_add(vnet_dev, &priv->napi, vnet_poll);

	/* Publish the device: assigns the final name, creates sysfs entries, calls
	 * ndo_init (our per-CPU stats alloc), and makes vnet0 visible to userspace.
	 * After this the interface exists but is administratively DOWN. */
	err = register_netdev(vnet_dev);
	if (err) {
		netif_napi_del(&priv->napi);   /* undo the napi_add             */
		free_netdev(vnet_dev);         /* free the net_device + priv    */
		return err;
	}

	pr_info("%s: registered %s (loopback NIC, %u-slot RX ring, NAPI budget %d)\n",
		DRV_NAME, vnet_dev->name, VNET_RING_SIZE, NAPI_POLL_WEIGHT);
	return 0;
}

static void __exit vnet_exit(void)
{
	struct vnet_priv *priv = netdev_priv(vnet_dev);

	/* Reverse of init. unregister_netdev() brings the device down (calling
	 * ndo_stop if it was up, then ndo_uninit) and removes it from every list;
	 * it blocks until the device is quiescent and unreferenced. */
	unregister_netdev(vnet_dev);

	/* The device is now offline and no poll() can be in flight, so it is safe to
	 * tear down NAPI and free the memory. Order matters: napi_del BEFORE the
	 * free_netdev that backs it. */
	netif_napi_del(&priv->napi);
	free_netdev(vnet_dev);

	pr_info("%s: unregistered\n", DRV_NAME);
}

module_init(vnet_init);
module_exit(vnet_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("low-level-linux-lab");
MODULE_DESCRIPTION("Loopback-style virtual NIC: net_device_ops + NAPI + ethtool teaching core");
MODULE_VERSION(DRV_VERSION);
