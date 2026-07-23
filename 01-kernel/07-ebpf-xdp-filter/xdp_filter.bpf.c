// SPDX-License-Identifier: GPL-2.0
/* ===========================================================================
 * xdp_filter.bpf.c — an XDP packet filter that runs INSIDE the kernel.
 * ===========================================================================
 *
 * WHAT THIS IS
 * ------------
 * This is a BPF program compiled with `clang -target bpf`. It is NOT a normal
 * object file: clang emits BPF bytecode (a tiny 64-bit RISC-like instruction
 * set) plus BTF type information, and libbpf later loads that bytecode into the
 * kernel through bpf(2). Before the kernel will run a single instruction, the
 * in-kernel VERIFIER simulates every possible path through the program and
 * proves it is safe: it terminates, never dereferences an out-of-bounds or
 * NULL pointer, and only touches memory it is allowed to. That proof obligation
 * is what shapes every line below — see the "VERIFIER CONTRACT" notes.
 *
 * WHERE IT RUNS: THE XDP HOOK
 * ---------------------------
 * XDP (eXpress Data Path) is the earliest hook in the Linux receive path. The
 * NIC driver calls our program on each received frame BEFORE the kernel has
 * allocated an sk_buff — the packet is still just DMA'd bytes in a page. That
 * is why XDP is fast (millions of pps per core) and why it is the right place
 * to drop a DDoS flood: we spend nothing building socket-buffer metadata for a
 * packet we are about to discard. Our program returns a VERDICT:
 *
 *   XDP_DROP     — free the frame now; the stack never sees it.
 *   XDP_PASS     — hand the frame up to the normal networking stack.
 *   XDP_TX       — bounce it back out the same NIC (basic reflection/LB).
 *   XDP_REDIRECT — send it to another NIC or into an AF_XDP userspace socket.
 *   XDP_ABORTED  — like DROP but fires a tracepoint; means "a bug hit", not
 *                  "policy said drop". We use it only for malformed packets.
 *
 * WHAT THIS PROGRAM DOES
 * ----------------------
 *   1. Parses Ethernet (+ up to two VLAN tags) + IPv4 + TCP/UDP/ICMP, with a
 *      bounds check before every field access (direct packet access).
 *   2. Counts packets/bytes per XDP verdict, per L4 protocol, and per source IP.
 *   3. Drops any packet whose source IP is in a userspace-managed blocklist,
 *      and (optionally, controlled by a compile-time-constant flag the loader
 *      sets) drops all ICMP.
 *   4. Computes a 5-tuple flow hash and tallies which load-balancer bucket the
 *      packet WOULD map to — the exact primitive a Katran/Maglev L4 LB uses to
 *      keep a connection pinned to one backend. (We only count; we don't fwd.)
 *
 * The data plane (this file) is deliberately policy-free where it can be: the
 * blocklist is a MAP that userspace fills, so you change policy without
 * reloading/recompiling the program. Separating a fast, dumb data plane from a
 * slow, smart control plane is the central pattern of production XDP.
 * ===========================================================================
 */

/* linux/bpf.h gives us: the XDP action enum (XDP_DROP...), struct xdp_md (the
 * program context), the BPF_MAP_TYPE_* enum, and the __u8..__u64 typedefs. */
#include <linux/bpf.h>
/* bpf_helpers.h: the SEC()/__uint()/__type() macros for BTF-defined maps, the
 * bpf_map_* helper prototypes, and __always_inline. */
#include <bpf/bpf_helpers.h>
/* bpf_endian.h: bpf_htons/bpf_ntohs. On-the-wire fields are big-endian; x86 is
 * little-endian, so every 16/32-bit header field must be byte-swapped before we
 * compare it against a host constant. Getting this wrong is the #1 XDP bug. */
#include <bpf/bpf_endian.h>

/* These are UAPI (user-facing, on-the-wire) headers. Packet formats — Ethernet,
 * IPv4, TCP — are frozen by protocol standards, so their structs are STABLE ABI
 * and need no CO-RE relocation. That is why we include them directly instead of
 * pulling struct iphdr out of vmlinux.h: CO-RE relocations exist to survive
 * changes in KERNEL-INTERNAL layout across versions, and a wire format never
 * changes layout. (libbpf still gives us CO-RE for anything kernel-internal.) */
#include <linux/if_ether.h>	/* struct ethhdr, ETH_P_IP, ETH_P_8021Q      */
#include <linux/ip.h>		/* struct iphdr                              */
#include <linux/in.h>		/* IPPROTO_TCP / IPPROTO_UDP / IPPROTO_ICMP  */
#include <linux/tcp.h>		/* struct tcphdr                             */
#include <linux/udp.h>		/* struct udphdr                             */

/* Our own shared key/value/index definitions — see xdp_filter.h. Included AFTER
 * linux/bpf.h so the __uNN types it uses are already defined. */
#include "xdp_filter.h"

/* Every program that uses a GPL-only helper (and most of the useful ones are
 * GPL-only) MUST declare a GPL-compatible license in this exact magic section,
 * or the verifier rejects the load. This is a license-compliance gate baked
 * into the kernel, not decoration. */
char LICENSE[] SEC("license") = "GPL";

/* Pin the header's hand-spelled constant to the real kernel enum value now that
 * the enum is in scope. If a future kernel ever adds an action and shifts the
 * numbering, this fails the BUILD instead of silently corrupting stats. */
_Static_assert(XDP_ACTION_MAX == XDP_REDIRECT + 1,
	       "XDP_ACTION_MAX must equal the number of XDP action codes");

/* ---------------------------------------------------------------------------
 * COMPILE-TIME-CONSTANT CONFIG (rodata), set by userspace BEFORE load.
 *
 * `const volatile` is the libbpf idiom for a tunable that the loader patches
 * into the program's read-only data section between open() and load(). Because
 * the verifier then treats it as a KNOWN CONSTANT, an `if (drop_all_icmp)` whose
 * value is 0 gets dead-code-eliminated entirely — you pay zero runtime cost for
 * a feature you turned off. `volatile` stops the BPF compiler from folding the
 * initial 0 in itself (it must read the slot the loader will overwrite).
 * --------------------------------------------------------------------------- */
const volatile __u8 drop_all_icmp = 0;

/* ===========================================================================
 * MAPS — the only channel between this program and userspace.
 * ===========================================================================
 * Each map below is declared in the modern "BTF-defined" style: a struct whose
 * fields are pseudo-annotations the loader reads from BTF to create the map.
 * `__uint`/`__type` are macros that stuff the value into a specially-typed
 * member so libbpf can recover it. Nothing here allocates at runtime — the maps
 * are created by the kernel at load time and pinned to the program's lifetime.
 */

/* xdp_action_map: a histogram of verdicts, keyed by the XDP action code.
 *
 * PERCPU_ARRAY is the workhorse of XDP statistics. "Array" = keys are dense
 * integers 0..max_entries-1, O(1), no hashing. "PERCPU" = the kernel keeps a
 * SEPARATE copy of each value per logical CPU, and a running program only ever
 * sees ITS OWN cpu's copy. That means we can do a plain `value->packets++`
 * with NO atomic and NO lock: no other CPU can touch our slot concurrently.
 * On a 64-core box handling 60 Mpps that is the difference between scaling
 * linearly and melting on cache-line contention. Userspace reads ALL per-cpu
 * copies and sums them (see the loader). */
struct {
	__uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
	__type(key, __u32);
	__type(value, struct datarec);
	__uint(max_entries, XDP_ACTION_MAX);
} xdp_action_map SEC(".maps");

/* proto_map: packets/bytes per L4 protocol, keyed by enum proto_idx. Same
 * per-CPU rationale as above. */
struct {
	__uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
	__type(key, __u32);
	__type(value, struct datarec);
	__uint(max_entries, PROTO_IDX__MAX);
} proto_map SEC(".maps");

/* lb_bucket_map: how many packets each flow-hash bucket would receive. Per-CPU
 * again — this is pure accounting on the hot path. */
struct {
	__uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
	__type(key, __u32);
	__type(value, struct datarec);
	__uint(max_entries, LB_BUCKETS);
} lb_bucket_map SEC(".maps");

/* src_map: per-source-IP packets/bytes — the "top talkers" table.
 *
 * We can't use an array here: source IPs are a sparse 32-bit space, not a dense
 * 0..N index. So this is a HASH map. LRU_HASH means that when the map is full,
 * inserting a new key evicts the least-recently-used entry instead of failing.
 * That is exactly what you want facing the open internet: under a spoofed-source
 * flood you would otherwise fill a fixed HASH and start dropping updates for
 * legitimate hosts; LRU bounds memory and self-cleans. The cost is that an entry
 * can disappear, so these counts are "recent talkers", not an exact ledger.
 *
 * A HASH value CAN be touched by several CPUs at once (two cores may see packets
 * from the same source IP simultaneously), so unlike the per-CPU arrays we MUST
 * use an atomic add here — see account_hash(). */
struct {
	__uint(type, BPF_MAP_TYPE_LRU_HASH);
	__type(key, __u32);		/* source IPv4, network byte order */
	__type(value, struct datarec);
	__uint(max_entries, 1024);
} src_map SEC(".maps");

/* blocklist_map: source IPs to DROP, filled by userspace (the control plane).
 * Key = src IPv4; value = a per-IP drop counter so you can see the block biting.
 * A plain HASH (not LRU): a blocklist entry must never be silently evicted, or
 * a blocked attacker would leak back through. Bounded at 4096 entries; a real
 * scrubber would size this to its threat feed. */
struct {
	__uint(type, BPF_MAP_TYPE_HASH);
	__type(key, __u32);		/* source IPv4, network byte order */
	__type(value, __u64);		/* packets dropped for this source */
	__uint(max_entries, 4096);
} blocklist_map SEC(".maps");

/* ===========================================================================
 * PURE-LOGIC HELPERS
 * ===========================================================================
 */

/* flow_hash — a 32-bit FNV-1a hash over the connection 5-tuple.
 *
 * This is the project's most instructive pure-logic routine, and the one
 * extracted verbatim into asm/demo.c for the assembly deliverable. An L4 load
 * balancer's entire fast-path decision is: hash(5-tuple) -> pick a backend. The
 * hash must be DETERMINISTIC (every packet of a TCP connection shares the same
 * 5-tuple, so it must land in the same bucket — otherwise the connection would
 * be sprayed across backends and reset) and reasonably UNIFORM (so load spreads
 * evenly). FNV-1a is a fine choice: tiny, no table, good avalanche for short
 * keys. Production LBs (Maglev) add consistent hashing so that adding/removing
 * one backend reshuffles only 1/N of flows — that is the stretch goal.
 *
 * __always_inline: the BPF verifier historically could not handle real function
 * calls into a subprogram with a loop, and inlining keeps the whole thing in one
 * verifiable straight-line blob. It also lets the optimizer fold the fixed-size
 * loop. This is a common XDP idiom, not a micro-optimization for its own sake. */
static __always_inline __u32 flow_hash(const struct flow_key *k)
{
	/* FNV-1a: start from the 32-bit offset basis, then for each input byte:
	 *   hash = (hash XOR byte) * FNV_prime
	 * The XOR-before-multiply ordering (that is the "1a" variant) gives better
	 * avalanche than the original FNV-1. Constants are the standardized 32-bit
	 * FNV parameters — do not change them or two hosts would disagree. */
	const __u32 FNV_OFFSET = 2166136261u;	/* 0x811c9dc5 */
	const __u32 FNV_PRIME  = 16777619u;	/* 0x01000193 */

	__u32 h = FNV_OFFSET;
	/* Treat the key as a byte string. We hash the fields explicitly (not via a
	 * byte loop over sizeof(*k)) so that the struct's PADDING byte can never
	 * leak into the hash — padding is uninitialised and would make the hash
	 * non-deterministic. Order is fixed and shared with asm/demo.c. */
	const __u8 *b;

	b = (const __u8 *)&k->saddr;			/* 4 bytes, source IP     */
	h = (h ^ b[0]) * FNV_PRIME;
	h = (h ^ b[1]) * FNV_PRIME;
	h = (h ^ b[2]) * FNV_PRIME;
	h = (h ^ b[3]) * FNV_PRIME;

	b = (const __u8 *)&k->daddr;			/* 4 bytes, dest IP       */
	h = (h ^ b[0]) * FNV_PRIME;
	h = (h ^ b[1]) * FNV_PRIME;
	h = (h ^ b[2]) * FNV_PRIME;
	h = (h ^ b[3]) * FNV_PRIME;

	b = (const __u8 *)&k->sport;			/* 2 bytes, source port   */
	h = (h ^ b[0]) * FNV_PRIME;
	h = (h ^ b[1]) * FNV_PRIME;

	b = (const __u8 *)&k->dport;			/* 2 bytes, dest port     */
	h = (h ^ b[0]) * FNV_PRIME;
	h = (h ^ b[1]) * FNV_PRIME;

	h = (h ^ k->proto) * FNV_PRIME;			/* 1 byte, protocol       */
	return h;
}

/* account_percpu — bump a per-CPU array cell with no atomic.
 *
 * VERIFIER CONTRACT: bpf_map_lookup_elem may return NULL (e.g. key out of
 * range), and the verifier FORBIDS dereferencing its result until we have
 * proven it non-NULL. The `if (!rec) return;` below is not defensive style — it
 * is mandatory; without it the program is rejected at load. Once past the
 * check, rec points at THIS cpu's private copy, so a plain `+=` is race-free. */
static __always_inline void account_percpu(void *map, __u32 key, __u64 bytes)
{
	struct datarec *rec = bpf_map_lookup_elem(map, &key);
	if (!rec)
		return;
	rec->packets += 1;
	rec->bytes   += bytes;
}

/* account_hash — bump (or create) a per-source-IP cell in a shared HASH map.
 *
 * Unlike the per-CPU arrays, a HASH value is shared across CPUs, so we increment
 * with __sync_fetch_and_add (a BPF atomic add). The lookup can miss on the first
 * packet from an IP; then we insert a seed record with BPF_NOEXIST. The insert
 * can race with another CPU inserting the same key — that is fine, one wins and
 * the loser's next packet will find the entry and add atomically. */
static __always_inline void account_hash(void *map, __u32 key, __u64 bytes)
{
	struct datarec *rec = bpf_map_lookup_elem(map, &key);
	if (rec) {
		/* Entry exists: atomically add so a concurrent CPU can't lose an
		 * update via a read-modify-write race on the shared value. */
		__sync_fetch_and_add(&rec->packets, 1);
		__sync_fetch_and_add(&rec->bytes, bytes);
		return;
	}
	/* First sighting of this source IP: seed the entry. BPF_NOEXIST means
	 * "fail if a key already appeared" — harmless here, it just means another
	 * CPU beat us to it. For an LRU map this insert may EVICT a stale entry. */
	struct datarec seed = { .packets = 1, .bytes = bytes };
	bpf_map_update_elem(map, &key, &seed, BPF_NOEXIST);
}

/* The VLAN tag header. We define it ourselves (rather than pull in if_vlan.h,
 * which drags in kernel-internal helpers) because on the wire it is just two
 * big-endian shorts sitting between the Ethernet header and the payload:
 *   [ 802.1Q TCI ][ inner ethertype ]. */
struct vlan_hdr {
	__be16 tci;		/* priority + VLAN id (we don't inspect it) */
	__be16 encap_proto;	/* the ethertype of what the tag wraps      */
};

/* ===========================================================================
 * THE XDP PROGRAM
 * ===========================================================================
 * SEC("xdp") names the program section; libbpf uses it to know this is an XDP
 * program and to attach it correctly. The single argument is the context.
 */
SEC("xdp")
int xdp_filter_prog(struct xdp_md *ctx)
{
	/* struct xdp_md hands us the packet as two integers. We cast them to
	 * pointers. `data` is the first byte of the frame (the Ethernet dst MAC);
	 * `data_end` is one-past-the-last received byte. These are the ONLY bounds
	 * we get — the verifier tracks every pointer we derive from `data` and will
	 * reject any load/store it cannot prove stays < data_end. This discipline
	 * (a bounds check before every dereference) is "direct packet access". */
	void *data     = (void *)(long)ctx->data;
	void *data_end = (void *)(long)ctx->data_end;

	/* The on-wire frame length as seen at the driver hook — used as the byte
	 * count for all our accounting. */
	__u64 pkt_bytes = data_end - data;

	/* The default verdict. We compute the real one and account for it once at
	 * the end, so there is exactly one accounting site per outcome. */
	__u32 action = XDP_PASS;

	/* ---- Ethernet header ------------------------------------------------ */
	struct ethhdr *eth = data;
	/* BOUNDS CHECK: prove all 14 bytes of the Ethernet header are present
	 * before reading eth->h_proto. (void*)(eth + 1) is the address just past
	 * the struct; if that exceeds data_end the frame is truncated. A runt
	 * frame is a bug/attack, not policy, so XDP_ABORTED (fires a tracepoint
	 * you can watch with `bpftool prog tracelog`). */
	if ((void *)(eth + 1) > data_end) {
		action = XDP_ABORTED;
		goto out;
	}

	/* h_proto tells us what the Ethernet frame carries. It is big-endian on
	 * the wire; `nh` ("next header") walks forward as we peel layers. */
	__be16 h_proto = eth->h_proto;
	void *nh = eth + 1;

	/* ---- VLAN tags: a BOUNDED loop (the verifier's cardinal rule) -------
	 * The verifier must prove the program HALTS, so it rejects loops whose
	 * trip count it cannot bound. A `#pragma unroll` over a compile-time
	 * constant count is the classic way to satisfy it: clang physically
	 * unrolls these two iterations into straight-line code, so there is no
	 * back-edge for the verifier to worry about. (Modern kernels also allow
	 * bpf_loop() and some bounded `for` loops, but unroll is the portable,
	 * always-accepted form.) Two iterations handles Q-in-Q double tagging. */
#pragma unroll
	for (int i = 0; i < 2; i++) {
		if (h_proto != bpf_htons(ETH_P_8021Q) &&
		    h_proto != bpf_htons(ETH_P_8021AD))
			break;			/* not a VLAN tag — stop peeling */

		struct vlan_hdr *vh = nh;
		if ((void *)(vh + 1) > data_end) {	/* bounds check again    */
			action = XDP_ABORTED;
			goto out;
		}
		h_proto = vh->encap_proto;	/* the tag reveals the inner type */
		nh = vh + 1;
	}

	/* We only implement IPv4 in this teaching core. Anything else (IPv6, ARP,
	 * ...) we simply PASS untouched — dropping unknown traffic would break the
	 * host. IPv6 support is a mechanical addition noted in the README. */
	if (h_proto != bpf_htons(ETH_P_IP))
		goto out;			/* action stays XDP_PASS */

	/* ---- IPv4 header (VARIABLE length — options make it 20..60 bytes) --- */
	struct iphdr *ip = nh;
	/* First prove the FIXED 20-byte part is present so we can read ihl. */
	if ((void *)(ip + 1) > data_end) {
		action = XDP_ABORTED;
		goto out;
	}
	/* ihl is the header length in 32-bit words. A sane IPv4 header is >= 5
	 * words (20 bytes); anything less is malformed. We also must re-bounds-
	 * check the FULL header including options before stepping past it — an
	 * attacker controls ihl, so trusting it without a check would let them
	 * point our L4 pointer wherever they like. */
	if (ip->ihl < 5) {
		action = XDP_ABORTED;
		goto out;
	}
	__u32 ip_hlen = ip->ihl * 4;			/* bytes of IP header    */
	if ((void *)ip + ip_hlen > data_end) {
		action = XDP_ABORTED;
		goto out;
	}
	void *l4 = (void *)ip + ip_hlen;		/* start of TCP/UDP/ICMP */
	__u8 proto = ip->protocol;

	/* ---- Build the flow 5-tuple ----------------------------------------
	 * ZERO THE STRUCT FIRST: `= {}` clears the padding byte the compiler adds
	 * after `proto`. flow_hash() hashes fields explicitly so padding wouldn't
	 * corrupt it there, but if you ever use this key in a HASH map (the LB
	 * stretch), unzeroed padding makes two equal flows miss each other. Zero
	 * it once, here, and never think about it again. */
	struct flow_key fk = {};
	fk.saddr = ip->saddr;
	fk.daddr = ip->daddr;
	fk.proto = proto;

	/* Per-protocol bucketing + port extraction, each behind its own bounds
	 * check because the L4 header sits at a variable offset (after IP options)
	 * and may be truncated. */
	__u32 proto_idx = PROTO_IDX_OTHER;
	if (proto == IPPROTO_TCP) {
		struct tcphdr *th = l4;
		if ((void *)(th + 1) > data_end) {
			action = XDP_ABORTED;
			goto out;
		}
		fk.sport = th->source;
		fk.dport = th->dest;
		proto_idx = PROTO_IDX_TCP;
	} else if (proto == IPPROTO_UDP) {
		struct udphdr *uh = l4;
		if ((void *)(uh + 1) > data_end) {
			action = XDP_ABORTED;
			goto out;
		}
		fk.sport = uh->source;
		fk.dport = uh->dest;
		proto_idx = PROTO_IDX_UDP;
	} else if (proto == IPPROTO_ICMP) {
		proto_idx = PROTO_IDX_ICMP;
		/* ICMP has no ports; leave sport/dport zero. Optionally drop it. */
		if (drop_all_icmp) {		/* rodata flag — see top of file */
			action = XDP_DROP;
			goto account;		/* still counts the drop below   */
		}
	}

	/* ---- Control-plane policy: the source-IP blocklist -----------------
	 * Look the source IP up in the userspace-managed blocklist. A hit means
	 * "this source is hostile" -> DROP and bump its per-IP drop counter so an
	 * operator can watch the block working. This is the whole point of XDP for
	 * DDoS: the drop happens before any sk_buff exists, at line rate. */
	__u64 *dropped = bpf_map_lookup_elem(&blocklist_map, &fk.saddr);
	if (dropped) {
		__sync_fetch_and_add(dropped, 1);	/* shared HASH -> atomic */
		action = XDP_DROP;
		goto account;
	}

	/* ---- Load-balancer bucket accounting (count-only) ------------------
	 * Compute the flow hash and fold it into LB_BUCKETS buckets. `& (N-1)`
	 * works as modulo ONLY because LB_BUCKETS is a power of two; it is the
	 * cheap bucket-selection a real LB does per packet. We count into the
	 * bucket to visualise hash uniformity; we do not actually redirect. */
	__u32 bucket = flow_hash(&fk) & (LB_BUCKETS - 1);
	account_percpu(&lb_bucket_map, bucket, pkt_bytes);

	/* Per-protocol and per-source-IP accounting for PASSed traffic. */
	account_percpu(&proto_map, proto_idx, pkt_bytes);
	account_hash(&src_map, fk.saddr, pkt_bytes);

account:
	/* Single verdict-accounting site: record the packet against whatever
	 * action we settled on, then return that action to the driver. */
	account_percpu(&xdp_action_map, action, pkt_bytes);
	return action;

out:
	/* Reached by the "pass it through untouched" and malformed-frame paths,
	 * which skipped the per-protocol/src accounting above. Still record the
	 * verdict so the action histogram stays complete. */
	account_percpu(&xdp_action_map, action, pkt_bytes);
	return action;
}
