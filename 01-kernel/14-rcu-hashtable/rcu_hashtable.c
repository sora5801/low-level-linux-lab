// SPDX-License-Identifier: GPL-2.0
/* ===========================================================================
 * rcu_hashtable.c — a lock-free-READ hash table in the Linux kernel.
 * ===========================================================================
 *
 * THE ONE IDEA
 * ------------
 * Readers of this hash table take NO lock. They call rcu_read_lock(), walk a
 * bucket chain, read a value, and call rcu_read_unlock() — and that is it. No
 * atomic, no cmpxchg, no cache-line ping-pong between CPUs on the read path. On
 * a read-mostly structure (a route cache, a dentry cache, /proc lookups, a
 * netfilter connection table) this is enormous: a thousand CPUs can look up
 * concurrently and never contend.
 *
 * The price is paid entirely by WRITERS, and it is paid in TWO currencies:
 *
 *   1. Mutual exclusion between writers — an ordinary spinlock. Only writers
 *      contend on it; readers never touch it.
 *
 *   2. Deferred reclamation — a writer that removes or replaces an object may
 *      NOT free it immediately, because some reader may still be looking at it.
 *      Instead the writer unlinks the object (making it unreachable to *future*
 *      readers) and then waits for a "grace period": a window after which every
 *      reader that could possibly have held a reference has finished its
 *      rcu_read_unlock(). Only then is the memory freed. That wait is spelled
 *      synchronize_rcu() (blocking) or call_rcu()/kfree_rcu() (deferred, via a
 *      callback). This is the whole trick: RCU = "Read-Copy-Update", and the
 *      grace period is what makes the free safe without the reader ever locking.
 *
 * WHY THE BARRIERS MATTER (publish / consume) — the part everyone gets wrong
 * -------------------------------------------------------------------------
 * Linking a fully-initialized object into a list is TWO stores on the writer:
 *
 *      newv->data = 42;                 // (A) initialize the payload
 *      rcu_assign_pointer(n->value, newv); // (B) publish the pointer
 *
 * On a weakly-ordered CPU (arm64, ppc) and even under the compiler's freedom to
 * reorder, nothing *automatically* forces (A) to become visible to other CPUs
 * before (B). If a reader observes the new pointer (B) but the old contents at
 * (A), it dereferences a half-built object — garbage, or a crash.
 *
 *   - rcu_assign_pointer() is the PUBLISH side: it is a store-release. It emits
 *     the barrier that guarantees every prior store (A) is visible to any CPU
 *     that later observes this pointer store (B). "Everything I wrote before I
 *     handed you this pointer, you will see."
 *
 *   - rcu_dereference() is the CONSUME side on the reader: it loads the pointer
 *     and establishes a data dependency so the subsequent load of *pointer is
 *     ordered AFTER the pointer load. "Once I have the pointer, what it points
 *     at is the initialized version." On most CPUs this is free (the address
 *     dependency alone orders it); rcu_dereference exists to (a) stop the
 *     COMPILER from breaking that dependency and (b) document intent + let
 *     sparse verify you only deref __rcu pointers inside a read-side section.
 *
 * Use a plain load instead of rcu_dereference and the code will *appear* to
 * work for years, then corrupt memory once, on one customer's Power box, under
 * load. That is why this file is pedantic about which accessor is used where.
 *
 * WHAT THIS TEACHING CORE COVERS (and what it does not)
 * -----------------------------------------------------
 * Covered, end to end and correct against real kernel headers:
 *   - hlist bucket array + a single writer spinlock
 *   - readers via rcu_read_lock / hlist_for_each_entry_rcu / rcu_dereference
 *   - writers via rcu_assign_pointer / RCU_INIT_POINTER / hlist_add_head_rcu /
 *     hlist_del_rcu, with rcu_dereference_protected on the write side
 *   - deferred free three ways: kfree_rcu, call_rcu (+ a real callback), and
 *     synchronize_rcu, each with the reason it is the right tool there
 *   - rcu_barrier() at module unload — the step whose omission is a classic
 *     use-after-free when a module is removed while callbacks are still queued
 *   - a live concurrency stress test: reader kthreads + writer kthreads
 *
 * NOT covered (called out honestly in the README): resizable/rehashing tables
 * (see kernel rhashtable), per-bucket locks, SRText/SRCU sleepable readers, and
 * NUMA-aware allocation. The structure here is fixed-size on purpose so the RCU
 * mechanics stay front and center.
 *
 * BUILD & RUN: this is a kernel module. It only builds on Linux against kernel
 * headers, and you should only *load* it inside a throwaway QEMU/VM — a bug in
 * an RCU reclaim path is a kernel oops, not a segfault. See the README.
 * ===========================================================================
 */

/* Prefix every pr_info/pr_err with "rcu_hashtable: " automatically. Must come
 * before the includes so the logging headers pick it up. */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>	/* module_init/exit, MODULE_* macros            */
#include <linux/kernel.h>	/* pr_info, container_of                        */
#include <linux/init.h>		/* __init / __exit section annotations          */
#include <linux/slab.h>		/* kmalloc, kfree, kfree_rcu                     */
#include <linux/spinlock.h>	/* spinlock_t — the WRITER-side lock            */
#include <linux/rcupdate.h>	/* rcu_read_lock, synchronize_rcu, call_rcu     */
#include <linux/rculist.h>	/* hlist_*_rcu list helpers (RCU-safe variants) */
#include <linux/list.h>		/* struct hlist_head / hlist_node               */
#include <linux/kthread.h>	/* kthread_run, kthread_stop, kthread_should_stop */
#include <linux/sched.h>	/* cond_resched — voluntary quiescent state     */
#include <linux/delay.h>	/* usleep_range                                 */
#include <linux/atomic.h>	/* atomic_t stats counters                      */
#include <linux/types.h>	/* u32, u64, bool                               */
#include <linux/random.h>	/* get_random_u32 — drive the stress test       */
#include <linux/err.h>		/* IS_ERR / PTR_ERR for kthread_run             */

MODULE_LICENSE("GPL");		/* GPL: call_rcu, kthread_* are GPL-only symbols */
MODULE_AUTHOR("low-level-linux-lab");
MODULE_DESCRIPTION("RCU-protected hash table: lock-free reads, deferred reclaim");
MODULE_VERSION("1.0");

/* ---------------------------------------------------------------------------
 * Geometry of the table.
 *
 * A fixed power-of-two bucket count lets us turn a hash into a bucket index
 * with a single mask (h & HT_MASK) instead of a modulo (h % N), which on the
 * hot path is a real win: AND is one cycle, integer division is ~20+.
 * --------------------------------------------------------------------------- */
#define HT_BITS 8			/* 2^8 = 256 buckets                    */
#define HT_SIZE (1u << HT_BITS)		/* number of hlist heads                */
#define HT_MASK (HT_SIZE - 1u)		/* low-bits mask: index = hash & HT_MASK */

/* GOLDEN_RATIO_32 = round(2^32 / phi). Multiplying a key by this and taking the
 * top bits is "Fibonacci hashing": it scrambles sequential keys (0,1,2,3,...)
 * across all buckets instead of piling them into bucket 0. The kernel's own
 * hash_32() uses the same constant. We spell it out so asm/demo.c can extract
 * this exact routine and show you the `imul` the CPU actually runs. */
#define GOLDEN_RATIO_32 0x61C88647u

/* ---------------------------------------------------------------------------
 * The value object — the RCU-protected PAYLOAD.
 *
 * We separate the payload (ht_value) from the list node (ht_node) so that
 * UPDATING a key's value is a pointer swap: allocate a new ht_value, publish it
 * with rcu_assign_pointer, and defer-free the old one. Readers in flight keep
 * using the old ht_value until their grace period ends; new readers see the new
 * one. Nobody ever observes a torn, half-written value. This is the "Copy" and
 * "Update" of Read-Copy-Update made concrete.
 * --------------------------------------------------------------------------- */
struct ht_value {
	u64 data;		/* the actual payload (kept trivial on purpose) */
	struct rcu_head rcu;	/* threading for kfree_rcu() deferred free      */
};

/* ---------------------------------------------------------------------------
 * The hash-table node — one entry living in a bucket chain.
 *
 * `node`  is the intrusive linkage; hlist (a single-pointer-head list) is what
 *         the kernel uses for hash buckets because the head is half the size of
 *         a list_head, and 256 or 65536 heads is a lot of saved memory.
 *
 * `value` is annotated __rcu: sparse will then ERROR if we ever read it with a
 *         plain load instead of rcu_dereference()/rcu_dereference_protected().
 *         That annotation is a compile-time guardrail against the exact
 *         publish/consume bug described in the file header.
 *
 * `rcu`   lets the whole node be freed a grace period later via call_rcu().
 * --------------------------------------------------------------------------- */
struct ht_node {
	struct hlist_node node;			/* linkage in buckets[b]        */
	u32 key;				/* immutable once published     */
	struct ht_value __rcu *value;		/* RCU-protected payload ptr    */
	struct rcu_head rcu;			/* for call_rcu() node reclaim  */
};

/* ---------------------------------------------------------------------------
 * The table itself.
 *
 * `lock` serializes WRITERS only. Readers never acquire it. Two writers racing
 * to insert the same key, or a delete racing an update, are made mutually
 * exclusive here; the RCU machinery separately protects concurrent readers.
 *
 * `count` is a stat (approximate under concurrency); atomic so the reader-side
 * stress thread can print it without holding the writer lock.
 * --------------------------------------------------------------------------- */
struct hash_table {
	struct hlist_head buckets[HT_SIZE];
	spinlock_t lock;
	atomic_t count;
};

static struct hash_table table;

/* Global op counters so module-exit can print evidence the test ran. atomic_t
 * because many kthreads bump them concurrently. */
static atomic_t stat_lookups_hit;
static atomic_t stat_lookups_miss;
static atomic_t stat_inserts;
static atomic_t stat_deletes;

/* ---------------------------------------------------------------------------
 * ht_bucket — map a key to a bucket index.
 *
 * This is the project's most instructive PURE-LOGIC helper, and it is exactly
 * what asm/demo.c extracts and annotates: a multiply by the golden ratio, an
 * avalanche xor-shift to fold the high entropy down into the low bits, then a
 * mask. There is no kernel API here at all — which is the point: the hash is
 * ordinary arithmetic, and reading its assembly teaches the multiply/shift/mask
 * the CPU runs. The RCU ordering concerns live in the CALLERS, not here.
 * --------------------------------------------------------------------------- */
static u32 ht_bucket(u32 key)
{
	u32 h = key * GOLDEN_RATIO_32;	/* Fibonacci hash: scatter the key      */
	h ^= h >> 16;			/* avalanche high bits into low bits    */
	return h & HT_MASK;		/* fold into [0, HT_SIZE) with one AND  */
}

/* ---------------------------------------------------------------------------
 * ht_node_reclaim — the call_rcu() CALLBACK that frees a deleted node.
 *
 * The kernel invokes this ONCE the grace period following our hlist_del_rcu()
 * has elapsed, i.e. once no reader can still hold a pointer to this node. By
 * then we are the sole owner, so we may read node->value with a plain-ish
 * accessor and free both the payload and the node.
 *
 * INVARIANT this callback relies on: the node was already unlinked with
 * hlist_del_rcu() BEFORE call_rcu() was issued. If you call_rcu() a still-linked
 * node, a reader can find it after we free it — a use-after-free. Unlink first,
 * defer-free second: always.
 *
 * DANGER, the reason rcu_barrier() exists: this function's code lives in THIS
 * MODULE's .text. If the module is unloaded while this callback is still queued
 * but not yet run, the kernel will later jump to an address that has been freed
 * — an instant oops. Module exit MUST rcu_barrier() to drain pending callbacks.
 * --------------------------------------------------------------------------- */
static void ht_node_reclaim(struct rcu_head *head)
{
	struct ht_node *n = container_of(head, struct ht_node, rcu);
	/* We are past the grace period and hold the only reference, so there is
	 * no concurrent updater. rcu_dereference_protected(..., true) asserts
	 * "some condition guarantees exclusivity" and returns the raw pointer
	 * without a barrier — the right accessor for a known-quiescent object. */
	struct ht_value *v = rcu_dereference_protected(n->value, true);

	kfree(v);	/* free the payload  */
	kfree(n);	/* free the node     */
}

/* ---------------------------------------------------------------------------
 * ht_lookup — the LOCK-FREE reader. Returns true and fills *out on a hit.
 *
 * The entire body runs inside rcu_read_lock()/rcu_read_unlock(). On a classic
 * (non-preemptible) RCU build that pair is almost free — it just disables
 * preemption — and it does NOT create mutual exclusion. Ten CPUs can be inside
 * this function on the same bucket simultaneously. What the read-side section
 * DOES promise is: any node/value we observe will not be freed until after we
 * call rcu_read_unlock(). The grace period on the writer side is defined as
 * "long enough that every CPU has passed through a quiescent state", and being
 * inside rcu_read_lock() holds off that grace period on this CPU.
 *
 * HARD RULE: you may not sleep between rcu_read_lock() and rcu_read_unlock() in
 * classic RCU. No kmalloc(GFP_KERNEL), no mutex, no copy_*_user, no
 * usleep_range. Sleeping would let the grace period complete while we still
 * hold a reference — the object could be freed under us. (SRCU lifts this at a
 * cost; not used here.)
 * --------------------------------------------------------------------------- */
static bool ht_lookup(u32 key, u64 *out)
{
	struct ht_node *n;
	u32 b = ht_bucket(key);
	bool found = false;

	rcu_read_lock();
	/* hlist_for_each_entry_rcu() loads each ->next with an implicit
	 * rcu_dereference(): the CONSUME barrier that pairs with the writer's
	 * hlist_add_head_rcu() PUBLISH. Because of it, if we see a node at all,
	 * we also see its correctly-initialized fields — no half-built node. */
	hlist_for_each_entry_rcu(n, &table.buckets[b], node) {
		if (n->key == key) {
			/* rcu_dereference(): CONSUME the value pointer. Pairs
			 * with the writer's rcu_assign_pointer(n->value, ...).
			 * Guarantees v->data below is the fully-written payload,
			 * never a stale/torn one, even if a writer is swapping
			 * the value concurrently on another CPU. */
			struct ht_value *v = rcu_dereference(n->value);

			*out = v->data;	/* safe: v is pinned until unlock  */
			found = true;
			break;
		}
	}
	rcu_read_unlock();

	atomic_inc(found ? &stat_lookups_hit : &stat_lookups_miss);
	return found;
}

/* ---------------------------------------------------------------------------
 * ht_insert — insert a new key, or replace the value of an existing key.
 *
 * All allocation happens BEFORE the spinlock: kmalloc(GFP_KERNEL) may sleep,
 * and you must not sleep holding a spinlock. If it turns out we did not need
 * the pre-allocated node (the key already existed), we free it after dropping
 * the lock. Trading a rare wasted allocation for a never-sleep-under-lock
 * guarantee is the standard kernel pattern.
 * --------------------------------------------------------------------------- */
static int ht_insert(u32 key, u64 data)
{
	struct ht_node *n, *newn;
	struct ht_value *newv, *oldv = NULL;
	u32 b = ht_bucket(key);
	bool replaced = false;

	newv = kmalloc(sizeof(*newv), GFP_KERNEL);
	if (!newv)
		return -ENOMEM;
	newv->data = data;

	newn = kmalloc(sizeof(*newn), GFP_KERNEL);
	if (!newn) {
		kfree(newv);
		return -ENOMEM;
	}

	spin_lock(&table.lock);

	/* Writer-side scan. We hold table.lock, which excludes every other
	 * writer, so the chain is stable under us and we iterate with the PLAIN
	 * hlist_for_each_entry() — no rcu_dereference needed for our own
	 * consistency. (Concurrent READERS are still walking this same chain
	 * with the _rcu variant; that is fine and is the entire point.) */
	hlist_for_each_entry(n, &table.buckets[b], node) {
		if (n->key == key) {
			/* Existing key: swap in the new value. On the write side
			 * we read the current pointer with rcu_dereference_
			 * protected(): "the lock guarantees no other writer",
			 * so no CONSUME barrier is required — but sparse still
			 * sees a legitimate __rcu access. */
			oldv = rcu_dereference_protected(n->value,
					lockdep_is_held(&table.lock));

			/* PUBLISH the new value. rcu_assign_pointer() is the
			 * store-release that makes newv->data (written above,
			 * before the lock) visible to any reader that later
			 * observes this pointer. This single line is the crux of
			 * the whole file — see the header's publish/consume note. */
			rcu_assign_pointer(n->value, newv);
			replaced = true;
			break;
		}
	}

	if (!replaced) {
		newn->key = key;
		/* The node is NOT yet linked, so no reader can reach newn->value
		 * yet. RCU_INIT_POINTER() is the documented optimization for
		 * exactly this "initializing a not-yet-published pointer" case:
		 * it stores WITHOUT the release barrier, because the barrier we
		 * actually need is the one inside hlist_add_head_rcu() below —
		 * that publish orders BOTH the node link AND everything the node
		 * transitively points at (including newv->data). Using the
		 * cheaper init here and letting the list publish do the ordering
		 * is correct and idiomatic. */
		RCU_INIT_POINTER(newn->value, newv);

		/* PUBLISH the node. hlist_add_head_rcu() sets the bucket head's
		 * ->first via rcu_assign_pointer internally: the store-release
		 * that pairs with readers' hlist_for_each_entry_rcu() CONSUME.
		 * After this returns, new readers can find newn — and are
		 * guaranteed to see its initialized key/value. */
		hlist_add_head_rcu(&newn->node, &table.buckets[b]);
		atomic_inc(&table.count);
	}

	spin_unlock(&table.lock);

	if (replaced) {
		kfree(newn);		/* never linked — free immediately   */
		/* Defer-free the OLD value. kfree_rcu() is call_rcu() with a
		 * built-in "just kfree it" callback: non-blocking, and (unlike a
		 * custom callback) it needs no module code to survive unload, so
		 * it is the lightest correct choice for a plain kfree after a
		 * grace period. Readers still holding oldv keep using it until
		 * their read-side sections end. */
		kfree_rcu(oldv, rcu);
	}

	atomic_inc(&stat_inserts);
	return 0;
}

/* ---------------------------------------------------------------------------
 * ht_delete — remove a key. Returns true if it was present.
 *
 * The removal is two conceptual steps with a grace period between them:
 *   (1) hlist_del_rcu() — unlink NOW, under the lock. New readers can no longer
 *       find the node. Readers ALREADY mid-walk may still hold it: hlist_del_rcu
 *       leaves the victim's ->next intact precisely so an in-flight reader can
 *       finish traversing off the end of the (now detached) node safely.
 *   (2) call_rcu(&victim->rcu, ht_node_reclaim) — schedule the free for after
 *       the grace period, when step-(1)'s in-flight readers are guaranteed gone.
 *
 * We use call_rcu (not synchronize_rcu) here because a writer should not BLOCK
 * for a whole grace period on every delete — that could be milliseconds. call_
 * rcu registers the callback and returns immediately; the kernel runs it later.
 * --------------------------------------------------------------------------- */
static bool ht_delete(u32 key)
{
	struct ht_node *n, *victim = NULL;
	u32 b = ht_bucket(key);

	spin_lock(&table.lock);
	hlist_for_each_entry(n, &table.buckets[b], node) {
		if (n->key == key) {
			victim = n;
			/* Unlink from the chain. This is an rcu_assign_pointer on
			 * the predecessor's ->next under the hood: it publishes
			 * the "n is gone" view to future readers with proper
			 * ordering, while leaving n itself readable for readers
			 * that already passed the predecessor. */
			hlist_del_rcu(&n->node);
			atomic_dec(&table.count);
			break;
		}
	}
	spin_unlock(&table.lock);

	if (victim) {
		/* Deferred free of the ENTIRE node (and its value, inside the
		 * callback) after a grace period. See ht_node_reclaim. */
		call_rcu(&victim->rcu, ht_node_reclaim);
		atomic_inc(&stat_deletes);
		return true;
	}
	return false;
}

/* ---------------------------------------------------------------------------
 * ht_destroy — tear the whole table down, CORRECTLY even while readers run.
 *
 * This is where we demonstrate the BLOCKING grace-period primitive,
 * synchronize_rcu(), and where that primitive is genuinely load-bearing: this
 * function is called from module_exit while the reader kthreads are STILL
 * running (they are stopped afterwards). So we cannot just free — a reader may
 * be mid-lookup on any node.
 *
 * synchronize_rcu() blocks the caller until a full grace period elapses. That
 * is legal here — module_exit runs in process context and may sleep. It would
 * be catastrophic on a hot path or while holding a spinlock (it sleeps!), which
 * is why the runtime ht_delete path uses the non-blocking call_rcu instead:
 * same guarantee, opposite ergonomics.
 *
 * THE CORRECTNESS SUBTLETY (why we do NOT reuse the node's list linkage):
 * hlist_del_rcu() deliberately leaves the detached node's ->next pointing at its
 * old successor, so a reader currently positioned ON this node can still walk
 * off the end of the (now-detached) chain safely. If we were to relink the node
 * onto a private "to free" hlist, we would OVERWRITE that ->next and a concurrent
 * reader would follow it into our private list — a bug. So instead we record the
 * detached nodes in a separately-allocated POINTER ARRAY, never touching their
 * linkage, and free from the array after the grace period.
 *
 * The array is sized from table.count, which is stable here because all WRITER
 * threads are already stopped before ht_destroy runs (readers never change it).
 * --------------------------------------------------------------------------- */
static void ht_destroy(void)
{
	struct ht_node *n;
	struct hlist_node *tmp;
	struct ht_node **victims;
	unsigned int b;
	unsigned long i, nr = 0;
	unsigned long cap = atomic_read(&table.count);

	if (cap == 0)
		return;			/* empty table: nothing to retire       */

	/* GFP_KERNEL is fine: module_exit context may sleep. If this rare
	 * allocation fails we fall back to per-node call_rcu below. */
	victims = kmalloc_array(cap, sizeof(*victims), GFP_KERNEL);

	/* Phase 1: unlink every node under the writer lock and record it. We do
	 * NOT touch node->node after hlist_del_rcu — in-flight readers rely on
	 * its ->next staying valid until the grace period ends. */
	spin_lock(&table.lock);
	for (b = 0; b < HT_SIZE; b++) {
		hlist_for_each_entry_safe(n, tmp, &table.buckets[b], node) {
			hlist_del_rcu(&n->node);
			if (victims && nr < cap)
				victims[nr++] = n;	/* remember for phase 3 */
			else
				/* Array full/absent (table grew, or kmalloc
				 * failed): fall back to the always-correct
				 * per-node deferred free. */
				call_rcu(&n->rcu, ht_node_reclaim);
		}
	}
	atomic_set(&table.count, 0);
	spin_unlock(&table.lock);

	/* Phase 2: ONE grace period retires every node recorded in `victims`.
	 * When this returns, every reader that could have been mid-lookup when
	 * we unlinked has finished its rcu_read_unlock(). This is the efficiency
	 * win of one synchronize_rcu over nr separate call_rcu callbacks. */
	synchronize_rcu();

	/* Phase 3: provably unreachable and quiescent — free directly. */
	for (i = 0; i < nr; i++) {
		struct ht_value *v =
			rcu_dereference_protected(victims[i]->value, true);

		kfree(v);
		kfree(victims[i]);
	}
	kfree(victims);

	pr_info("ht_destroy: freed %lu node(s) after one grace period\n", nr);
}

/* ===========================================================================
 * CONCURRENCY STRESS TEST — reader kthreads racing writer kthreads.
 *
 * The point is to exercise the read path and write path at the same time, on
 * different CPUs, under a memory checker (KASAN, if your test kernel has it).
 * If the RCU discipline above is wrong, this is where it shows up: a
 * use-after-free reported by KASAN, or an oops in ht_lookup dereferencing a
 * freed value. If it is right, this runs forever without a peep.
 *
 * Keyspace is deliberately small (KEYSPACE) so inserts, updates, deletes, and
 * lookups collide constantly on the same buckets — maximizing the race we want
 * to prove safe.
 * ===========================================================================
 */
#define KEYSPACE 64		/* keys 0..63: small = lots of collisions       */
#define N_READERS 3
#define N_WRITERS 2

static struct task_struct *reader_threads[N_READERS];
static struct task_struct *writer_threads[N_WRITERS];

/* Reader thread: hammer ht_lookup on random keys until asked to stop. Never
 * takes the writer lock; never sleeps inside the RCU read section. */
static int reader_fn(void *arg)
{
	while (!kthread_should_stop()) {
		u32 key = get_random_u32() % KEYSPACE;
		u64 val;

		ht_lookup(key, &val);	/* result checked via stat counters   */

		/* Yield politely between ops. cond_resched() lets this CPU pass
		 * through a quiescent state so writers' grace periods can end;
		 * without periodic quiescence a busy kthread could stall RCU. */
		cond_resched();
	}
	return 0;
}

/* Writer thread: randomly insert/update or delete keys. Takes the writer lock
 * (inside ht_insert/ht_delete); may sleep (kmalloc, synchronize is not called
 * here). */
static int writer_fn(void *arg)
{
	while (!kthread_should_stop()) {
		u32 key = get_random_u32() % KEYSPACE;

		if (get_random_u32() & 1)
			ht_insert(key, get_random_u32());  /* insert or update */
		else
			ht_delete(key);

		/* Sleep a beat so grace periods actually complete and callbacks
		 * get to run, and so the log is not flooded. 0.5–1.5 ms. */
		usleep_range(500, 1500);
	}
	return 0;
}

/* ---------------------------------------------------------------------------
 * Module init: zero the table, start the threads.
 * --------------------------------------------------------------------------- */
static int __init rcu_ht_init(void)
{
	unsigned int i;

	/* hlist heads are just a NULL pointer each; zero them explicitly. A
	 * static struct is already zeroed, but being explicit documents intent
	 * and is correct even if the table becomes dynamically allocated. */
	for (i = 0; i < HT_SIZE; i++)
		INIT_HLIST_HEAD(&table.buckets[i]);

	spin_lock_init(&table.lock);
	atomic_set(&table.count, 0);
	atomic_set(&stat_lookups_hit, 0);
	atomic_set(&stat_lookups_miss, 0);
	atomic_set(&stat_inserts, 0);
	atomic_set(&stat_deletes, 0);

	pr_info("init: %u buckets, %d reader + %d writer kthreads\n",
		HT_SIZE, N_READERS, N_WRITERS);

	/* Start writers first so there is data to find, then readers. */
	for (i = 0; i < N_WRITERS; i++) {
		writer_threads[i] = kthread_run(writer_fn, NULL,
						"rcuht_wr/%u", i);
		if (IS_ERR(writer_threads[i])) {
			pr_err("failed to start writer %u\n", i);
			writer_threads[i] = NULL;
		}
	}
	for (i = 0; i < N_READERS; i++) {
		reader_threads[i] = kthread_run(reader_fn, NULL,
						"rcuht_rd/%u", i);
		if (IS_ERR(reader_threads[i])) {
			pr_err("failed to start reader %u\n", i);
			reader_threads[i] = NULL;
		}
	}

	return 0;	/* module stays resident, threads run until rmmod */
}

/* ---------------------------------------------------------------------------
 * Module exit: the ORDER here is load-bearing. Get it wrong and you get a
 * use-after-free at unload time — the most common RCU-module bug there is.
 *
 *   1. Stop the WRITER threads. After kthread_stop() returns for each, no new
 *      inserts/deletes happen, so table.count is now stable and no NEW call_rcu
 *      callbacks will be queued. The READER threads are intentionally left
 *      running so that step 2's synchronize_rcu() has real readers to wait for.
 *   2. ht_destroy(): unlink every node, then synchronize_rcu() to wait out any
 *      reader still mid-lookup, then free. Correct even with live readers.
 *   3. Stop the reader threads now that the table is gone.
 *   4. rcu_barrier(): WAIT for every already-queued call_rcu callback (from the
 *      writer threads' earlier ht_delete calls) to actually RUN and complete.
 *      This is the step people forget. synchronize_rcu waits for READERS; it
 *      does NOT wait for CALLBACKS. If a ht_node_reclaim callback is still
 *      pending when this module's code is unmapped, the kernel later calls a
 *      freed function pointer → oops. rcu_barrier is the only thing that closes
 *      that window. (kfree_rcu callbacks use built-in kernel code, but our
 *      call_rcu(ht_node_reclaim) points into THIS module — so this is mandatory.)
 * --------------------------------------------------------------------------- */
static void __exit rcu_ht_exit(void)
{
	unsigned int i;

	/* Step 1: stop writers only — freeze the table, leave readers racing. */
	for (i = 0; i < N_WRITERS; i++)
		if (writer_threads[i])
			kthread_stop(writer_threads[i]);

	/* Step 2: retire the live table with a single grace period, while the
	 * reader kthreads are still hammering lookups. This is what makes the
	 * synchronize_rcu() inside ht_destroy() actually necessary. */
	ht_destroy();

	/* Step 3: now the table is empty and freed — stop the readers. */
	for (i = 0; i < N_READERS; i++)
		if (reader_threads[i])
			kthread_stop(reader_threads[i]);

	/* Step 4: drain in-flight call_rcu callbacks BEFORE our .text vanishes.
	 * Without this line, `rmmod` can race a pending ht_node_reclaim. */
	rcu_barrier();

	pr_info("exit: lookups hit=%d miss=%d, inserts=%d, deletes=%d\n",
		atomic_read(&stat_lookups_hit),
		atomic_read(&stat_lookups_miss),
		atomic_read(&stat_inserts),
		atomic_read(&stat_deletes));
}

module_init(rcu_ht_init);
module_exit(rcu_ht_exit);
