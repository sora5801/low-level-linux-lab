/* ===========================================================================
 * asm/demo.c — the PURE-LOGIC core of rcu_hashtable.c, extracted so it can be
 * compiled to standalone assembly on ANY host.
 * ===========================================================================
 *
 * WHY THIS FILE EXISTS
 * --------------------
 * rcu_hashtable.c is a Linux kernel module. It #includes <linux/rcupdate.h>,
 * <linux/slab.h>, and friends, none of which exist off a configured kernel
 * tree, so `clang -S rcu_hashtable.c` cannot produce assembly here. But the
 * hash-table's HOTTEST, most instructive routine — turning a key into a bucket
 * index — is pure integer arithmetic with no kernel dependency at all. We lift
 * exactly that routine into this self-contained file (it declares its own
 * types, includes nothing) so the repo can still ship real, generated,
 * hand-annotated x86-64 assembly for this project.
 *
 * This is byte-for-byte the same computation ht_bucket() does in the module.
 *
 * WHAT THE ASSEMBLY TEACHES, AND THE RCU TWIST
 * --------------------------------------------
 * The generated asm shows the CPU-level shape of a hash: a single `imul` by the
 * golden-ratio constant, an xor-shift avalanche, and an `and` mask — no division
 * anywhere (a power-of-two table size is what buys that). See asm/demo.annotated.s.
 *
 * The crucial thing the assembly CANNOT show, and which the annotation spells
 * out in prose, is the RCU ordering that lives in the CALLERS of this function:
 * ht_bucket() itself has no memory barriers because it touches no shared memory
 * — it is a leaf that reads its argument and returns a number. The publish
 * (rcu_assign_pointer / store-release) and consume (rcu_dereference /
 * dependent-load) barriers wrap the *bucket access* that uses this index, not
 * the index computation. Reading this clean, barrier-free asm is what makes that
 * separation obvious: hashing is arithmetic; RCU safety is about the loads and
 * stores that come AFTER you have the bucket number.
 * ===========================================================================
 */

/* We are freestanding here — no <stdint.h>. Declare the one width we need.
 * On the x86-64 SysV target `unsigned int` is exactly 32 bits, which matches
 * the kernel's u32 and is what the golden-ratio multiply is defined on. */
typedef unsigned int u32;

/* Table geometry — identical to the module's. A power-of-two bucket count is
 * the whole reason the index is a mask (one AND) instead of a modulo (a slow
 * integer division). */
#define HT_BITS 8
#define HT_SIZE (1u << HT_BITS)		/* 256 buckets                          */
#define HT_MASK (HT_SIZE - 1u)		/* 0xFF: index = hash & HT_MASK         */

/* round(2^32 / phi): the 32-bit golden-ratio constant, same one the kernel's
 * hash_32() uses. Multiplying by it is "Fibonacci hashing" — it maps the
 * sequential keys 0,1,2,3,... to well-spread buckets instead of clustering. */
#define GOLDEN_RATIO_32 0x61C88647u

/* ---------------------------------------------------------------------------
 * ht_hash — scramble a key. Just the multiply; kept separate so you can see the
 * `imul` in isolation in the -O0 assembly.
 * --------------------------------------------------------------------------- */
u32 ht_hash(u32 key)
{
	return key * GOLDEN_RATIO_32;
}

/* ---------------------------------------------------------------------------
 * ht_bucket_highbits — the "take the top bits" school of hashing.
 *
 * Fibonacci hashing puts the best-mixed entropy in the HIGH bits of the
 * product, so shifting right by (32 - HT_BITS) keeps precisely those. Compiles
 * to imul + shr. This is what the kernel's hash_32() returns.
 * --------------------------------------------------------------------------- */
u32 ht_bucket_highbits(u32 key)
{
	return ht_hash(key) >> (32 - HT_BITS);
}

/* ---------------------------------------------------------------------------
 * ht_bucket_mask — the naive "just mask the low bits" school.
 *
 * Shown for contrast: masking the LOW bits of a plain multiply keeps the
 * worst-mixed bits, so this scatters sequential keys less well. Compiles to
 * imul + and. Included so the annotated asm can compare the two index schemes.
 * --------------------------------------------------------------------------- */
u32 ht_bucket_mask(u32 key)
{
	return ht_hash(key) & HT_MASK;
}

/* ---------------------------------------------------------------------------
 * ht_bucket — THE routine the module actually uses.
 *
 * Multiply (scatter) → xor-shift (avalanche the good high bits DOWN into the
 * low bits) → mask (fold to [0, HT_SIZE)). The xor-shift is what lets us safely
 * take the low bits with a cheap AND while still benefiting from the high-bit
 * mixing. Compiles to imul; mov/shr; xor; and — four arithmetic ops, no branch,
 * no memory, no division.
 * --------------------------------------------------------------------------- */
u32 ht_bucket(u32 key)
{
	u32 h = key * GOLDEN_RATIO_32;	/* scatter                              */
	h ^= h >> 16;			/* avalanche high half into low half    */
	return h & HT_MASK;		/* fold into the bucket range           */
}

/* A tiny driver so the file links as a standalone program too (e.g. to compare
 * outputs against the kernel version). Returns a bucket index as the process
 * exit code so `./a.out; echo $?` shows a concrete result — no libc needed for
 * the arithmetic itself, but main() lets -O2 show inlining of all the helpers. */
int main(void)
{
	/* Fold a handful of keys together so the optimizer must actually emit
	 * (and inline) the helpers rather than constant-fold everything away
	 * to nothing — though at -O2 it will fold this whole sum to a literal,
	 * which is itself a lesson visible in demo.O2.s. */
	u32 acc = 0;
	u32 k;

	for (k = 0; k < 8; k++)
		acc += ht_bucket(k);

	return (int)(acc & 0xff);
}
