/* ===========================================================================
 * asm/demo.c — the flow-hash core, extracted so it can be compiled to assembly.
 * ===========================================================================
 *
 * WHY THIS FILE EXISTS
 * --------------------
 * The real program, ../xdp_filter.bpf.c, is BPF C: it needs <linux/bpf.h>, the
 * BPF map macros, and `clang -target bpf`. You cannot compile it to *x86-64*
 * assembly on this host, and its BPF bytecode is a different instruction set
 * anyway. So, per the lab convention, we lift out the single most instructive
 * piece of PURE LOGIC — the 5-tuple flow hash that a load balancer computes for
 * every packet — into this self-contained file that depends on NOTHING (it
 * declares its own integer types and struct). We then generate real x86-64
 * assembly from it and hand-annotate the -O1 output in demo.annotated.s.
 *
 * The function below is byte-for-byte the same algorithm as flow_hash() in
 * xdp_filter.bpf.c. Reading its assembly shows you exactly what the CPU does per
 * packet in an XDP load balancer's fast path: a chain of XOR-then-multiply.
 *
 * (Bonus: `clang -target bpf -S demo.c` — well, this file has no BPF deps, so
 * you can also emit the *BPF* bytecode for it and compare instruction sets. The
 * Makefile's `make bpf-asm` target does exactly that.)
 * ===========================================================================
 */

/* We declare our own fixed-width types so this file needs no headers at all —
 * that is what makes it standalone-compilable to assembly on any host. On the
 * LP64 targets we care about these widths are correct; the assembly we generate
 * targets x86-64 Linux explicitly (see the clang commands in the README). */
typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;

/* The connection 5-tuple — the mirror of struct flow_key in ../xdp_filter.h.
 * Layout: 4 + 4 + 2 + 2 + 1 = 13 bytes, padded to 16. We hash the fields
 * explicitly (never a raw byte loop over sizeof), so the pad byte never enters
 * the hash and the result is deterministic. */
struct flow_key {
	u32 saddr;	/* source IPv4,      network byte order */
	u32 daddr;	/* destination IPv4, network byte order */
	u16 sport;	/* source port,      network byte order */
	u16 dport;	/* dest port,        network byte order */
	u8  proto;	/* IP protocol number                   */
};

/* ---------------------------------------------------------------------------
 * flow_hash — 32-bit FNV-1a over the 5-tuple.
 *
 * FNV-1a per input byte:   hash = (hash XOR byte) * FNV_PRIME
 * Determinism is the whole point: every packet of one connection shares the
 * 5-tuple, so it always hashes to the same value, so a hash-based LB pins the
 * connection to one backend with zero per-flow state. In the generated
 * assembly each `* 16777619` becomes a single `imull $0x01000193` (clang keeps
 * the hardware multiply at every level — a 3-cycle imul beats a long shift/add
 * chain for this constant) and each XOR a single-byte `movzbl` + `xorl`.
 *
 * Non-static so clang is forced to emit it (a static, unreferenced function
 * would be optimized away and produce no assembly to read). Marked with the
 * SysV calling convention explicitly in the annotation: the `const struct
 * flow_key *k` argument arrives in %rdi, the u32 result leaves in %eax.
 * --------------------------------------------------------------------------- */
u32 flow_hash(const struct flow_key *k)
{
	const u32 FNV_OFFSET = 2166136261u;	/* 0x811c9dc5 */
	const u32 FNV_PRIME  = 16777619u;	/* 0x01000193 */

	u32 h = FNV_OFFSET;
	const u8 *b;

	b = (const u8 *)&k->saddr;		/* 4 bytes: source IP */
	h = (h ^ b[0]) * FNV_PRIME;
	h = (h ^ b[1]) * FNV_PRIME;
	h = (h ^ b[2]) * FNV_PRIME;
	h = (h ^ b[3]) * FNV_PRIME;

	b = (const u8 *)&k->daddr;		/* 4 bytes: dest IP */
	h = (h ^ b[0]) * FNV_PRIME;
	h = (h ^ b[1]) * FNV_PRIME;
	h = (h ^ b[2]) * FNV_PRIME;
	h = (h ^ b[3]) * FNV_PRIME;

	b = (const u8 *)&k->sport;		/* 2 bytes: source port */
	h = (h ^ b[0]) * FNV_PRIME;
	h = (h ^ b[1]) * FNV_PRIME;

	b = (const u8 *)&k->dport;		/* 2 bytes: dest port */
	h = (h ^ b[0]) * FNV_PRIME;
	h = (h ^ b[1]) * FNV_PRIME;

	h = (h ^ k->proto) * FNV_PRIME;		/* 1 byte: protocol */
	return h;
}

/* ---------------------------------------------------------------------------
 * flow_bucket — fold the hash into one of `nbuckets` buckets.
 *
 * This is the actual backend-selection step. `nbuckets` MUST be a power of two
 * for the `& (nbuckets - 1)` trick to equal a modulo; the assembly shows it as
 * a single `andl`, far cheaper than the `div` a general modulo would need. This
 * is why LB_BUCKETS is 8 in the real program.
 * --------------------------------------------------------------------------- */
u32 flow_bucket(const struct flow_key *k, u32 nbuckets)
{
	return flow_hash(k) & (nbuckets - 1);
}
