/* ===========================================================================
 * xdp_filter.h — the ABI shared by the BPF program and its userspace loader.
 * ===========================================================================
 *
 * WHY A SHARED HEADER AT ALL?
 * ---------------------------
 * A BPF program and the process that loads it are two separately-compiled
 * worlds that meet only through BPF maps. A map is a typed key/value store that
 * lives in the kernel; the BPF side writes it from inside the XDP hook, the
 * userspace side reads it over the bpf(2) syscall. For that to work, BOTH sides
 * must agree, byte-for-byte, on the layout of every key and value struct and on
 * the meaning of every index. If they disagree — a padding byte here, a __u32
 * vs __u64 there — userspace will silently read garbage. So the single most
 * important discipline in a libbpf project is: define the map key/value types
 * and the index enums EXACTLY ONCE, here, and include this file from both the
 * .bpf.c and the loader .c. There is then no way for the two sides to drift.
 *
 * WHY THE FIXED-WIDTH TYPES?
 * --------------------------
 * The BPF side is compiled by `clang -target bpf`, whose `long` is 64-bit; the
 * loader is compiled for the host. `int`/`long` widths could in principle
 * differ, and struct padding is ABI-specific. Using only __u8/__u16/__u32/__u64
 * (which are the same everywhere) and keeping fields naturally aligned makes the
 * layout identical on both sides with no compiler-inserted padding surprises.
 *
 * This header is included by BOTH kernel-space BPF C and userspace C, so it must
 * not pull in anything that only one side has. It uses only fixed-width integer
 * typedefs, which the BPF side gets from <linux/types.h> (via bpf_helpers) and
 * the userspace side gets from <stdint.h>-compatible kernel headers. To be safe
 * we provide the __uNN names ourselves only if they are missing.
 * ===========================================================================
 */
#ifndef XDP_FILTER_H
#define XDP_FILTER_H

/* Both sides already have __u8..__u64 from the kernel UAPI headers they include
 * before this file (linux/types.h on BPF side, linux/bpf.h on the loader). We do
 * not redefine them — doing so would conflict. This header is included AFTER
 * those, by contract (see the top of each .c). */

/* ---------------------------------------------------------------------------
 * datarec — one accounting cell: a packet counter and a byte counter.
 *
 * This is the value type for our per-CPU statistics arrays. We keep BYTES as
 * well as PACKETS because "packets per second" and "bits per second" are
 * different signals: a SYN flood is many tiny packets (high pps, low bps),
 * while a volumetric flood is few huge packets (low pps, high bps). A real
 * scrubber needs both to classify an attack.
 *
 * 8-byte fields keep the struct 8-byte aligned with zero padding, so its layout
 * is identical in the BPF and userspace compilers.
 * --------------------------------------------------------------------------- */
struct datarec {
	__u64 packets;
	__u64 bytes;
};

/* ---------------------------------------------------------------------------
 * Protocol index space for the per-protocol stats array.
 *
 * We map the 8-bit IP protocol number (6=TCP, 17=UDP, 1=ICMP, ...) down to a
 * tiny dense index so it can be an ARRAY key. An array map is O(1) and needs no
 * hashing; the trade-off is that the key space must be small and contiguous,
 * which is why we bucket "everything else" into PROTO_IDX_OTHER instead of
 * indexing by the raw 0..255 protocol number (that would need a 256-entry map
 * and still miss the point). PROTO_IDX__MAX is the array length — keeping it as
 * the last enumerator means the map size updates automatically if we add a row.
 * --------------------------------------------------------------------------- */
enum proto_idx {
	PROTO_IDX_OTHER = 0,	/* anything not broken out below (e.g. GRE, ESP)   */
	PROTO_IDX_TCP,		/* IPPROTO_TCP  == 6                               */
	PROTO_IDX_UDP,		/* IPPROTO_UDP  == 17                              */
	PROTO_IDX_ICMP,		/* IPPROTO_ICMP == 1                               */
	PROTO_IDX__MAX,		/* NOT a protocol: the number of rows in the array */
};

/* ---------------------------------------------------------------------------
 * Number of XDP verdict codes we account for.
 *
 * The XDP action codes are a small enum in the kernel UAPI:
 *   XDP_ABORTED=0, XDP_DROP=1, XDP_PASS=2, XDP_TX=3, XDP_REDIRECT=4.
 * We size an array map to (XDP_REDIRECT + 1) so we can bump stats[action]
 * directly with the verdict as the key — the cheapest possible histogram.
 * We spell the literal 5 here rather than referencing XDP_REDIRECT so the
 * header stays include-order independent; a _Static_assert on the BPF side
 * (where the enum is in scope) pins it to the real value.
 * --------------------------------------------------------------------------- */
#define XDP_ACTION_MAX 5

/* ---------------------------------------------------------------------------
 * Number of load-balancer buckets the flow-hash maps a packet into.
 *
 * A hash-based L4 load balancer (Katran/Maglev style) does exactly one thing in
 * the fast path: compute a hash over the connection's 5-tuple and use it to pick
 * a backend, so that every packet of a given TCP connection lands on the SAME
 * backend (per-connection consistency) without storing any per-connection state.
 * We don't forward here — we just COUNT how many packets each bucket would get,
 * which visualises how evenly the flow-hash spreads real traffic. 8 is a power
 * of two so `hash & (LB_BUCKETS-1)` replaces an expensive modulo.
 * --------------------------------------------------------------------------- */
#define LB_BUCKETS 8

/* ---------------------------------------------------------------------------
 * flow_key — the 5-tuple that identifies a transport flow.
 *
 * This is the input to flow_hash() (see xdp_filter.bpf.c and asm/demo.c). It is
 * also the natural key for a per-flow hash map in the load-balancer stretch.
 *
 * FIELD ORDER AND PADDING MATTER: we place the two __u32 addresses first, then
 * the two __u16 ports, then the __u8 protocol. That is 4+4+2+2+1 = 13 bytes.
 * The compiler will pad the struct to 16 for alignment, and — critically — the
 * padding bytes are UNINITIALISED stack unless we zero them. When such a struct
 * is used as a HASH map key, the kernel hashes the raw bytes INCLUDING padding,
 * so two logically-equal keys with different garbage in the pad would miss each
 * other. The fix (shown in the .bpf.c) is to zero the whole struct first, e.g.
 * `struct flow_key k = {};`, before filling fields. This is a classic BPF bug.
 * --------------------------------------------------------------------------- */
struct flow_key {
	__u32 saddr;	/* source IPv4 address,      network byte order */
	__u32 daddr;	/* destination IPv4 address, network byte order */
	__u16 sport;	/* source L4 port,           network byte order */
	__u16 dport;	/* destination L4 port,      network byte order */
	__u8  proto;	/* IP protocol number (6/17/1/...)              */
};

#endif /* XDP_FILTER_H */
