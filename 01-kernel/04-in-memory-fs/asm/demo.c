/* ===========================================================================
 * asm/demo.c — the PURE-LOGIC core of tinyfs, extracted so it can be compiled
 *              to standalone x86-64 assembly and hand-annotated.
 * ===========================================================================
 *
 * WHY THIS FILE EXISTS
 * --------------------
 * tinyfs.c is a Linux kernel module: it #includes <linux/fs.h> and a dozen more
 * kernel-only headers, so it can only be built inside a configured kernel tree
 * on Linux — you cannot run `clang -S tinyfs.c` on a normal host to see its
 * assembly. To honor the repo rule that *every C project ships annotated
 * assembly*, we lift out the two most instructive pieces of pure computation
 * that tinyfs relies on — routines that touch no kernel type at all — into this
 * self-contained file. It declares its own integer types, includes nothing, and
 * compiles anywhere clang runs.
 *
 * The two routines are exactly the algorithms tinyfs.c leans on:
 *
 *   1. tinyfs_name_hash() — the kernel's dcache string hash (the byte-at-a-time
 *      form of partial_name_hash/end_name_hash from <linux/dcache.h>). When the
 *      VFS looks up a name in one of our directories, this is the arithmetic
 *      that decides which hash bucket the dentry lands in. It is nothing but
 *      shifts, an add, and a multiply — ideal for reading in assembly.
 *
 *   2. tinyfs_next_ino() — the single-CPU essence of the kernel's get_next_ino()
 *      (fs/inode.c), which tinyfs calls to number every new inode. The whole
 *      point is the wraparound guard: inode number 0 is reserved ("no inode"),
 *      so the counter must skip it. That one branch is the interesting part.
 *
 * These are byte-for-byte the same operations the kernel performs; only the
 * surrounding types are simplified. Read asm/demo.annotated.s to see how the
 * multiply-by-11 becomes an LEA chain and how the "skip zero" guard becomes a
 * single conditional move or branch.
 * ===========================================================================
 */

/* Freestanding integer typedefs. On the x86-64 SysV LP64 model that clang emits
 * for --target=x86_64-pc-linux-gnu, `int` is 32 bits and `long` is 64 bits, so
 * these match the kernel's u32/u64 without needing <stdint.h>. */
typedef unsigned int  u32;      /* 32-bit: an inode number, a folded hash      */
typedef unsigned long u64;      /* 64-bit: the wide hash accumulator           */

/* ---------------------------------------------------------------------------
 * partial_name_hash — mix one more character into the running hash.
 *
 * This is verbatim the kernel's <linux/dcache.h> definition:
 *     (prevhash + (c << 4) + (c >> 4)) * 11
 * The shifts spread each byte's bits into a wider range before the multiply by
 * 11 (a cheap prime the compiler turns into shift-and-add), so that similar
 * names such as "file1" and "file2" scatter to distant buckets. It is marked
 * static so the optimizer inlines it into the loop below — watch it disappear
 * into an LEA chain in the -O1/-O2 assembly.
 * --------------------------------------------------------------------------- */
static u64 partial_name_hash(u64 c, u64 prevhash)
{
	return (prevhash + (c << 4) + (c >> 4)) * 11;
}

/* ---------------------------------------------------------------------------
 * end_name_hash — fold the 64-bit accumulator down to the 32-bit value the
 * dcache actually stores in dentry->d_name.hash. The kernel runs a final
 * hash_long() mix here; we take the low 32 bits, which is enough to see the
 * truncation in assembly (a plain 32-bit register move that drops the top half).
 * --------------------------------------------------------------------------- */
static u32 end_name_hash(u64 hash)
{
	return (u32)hash;
}

/* ---------------------------------------------------------------------------
 * tinyfs_name_hash — hash a directory-entry name of `len` bytes.
 *
 * This is the whole point of the file: a tight loop that reads a byte, mixes it
 * with partial_name_hash, and repeats. In the annotated assembly you can see
 * the loop counter in a register, the byte load with zero-extension, and the
 * inlined multiply-by-11 becoming `lea (%reg,%reg,4)` style address arithmetic
 * used purely for its arithmetic side effect.
 *
 * Cast to unsigned char before widening so bytes >= 0x80 are treated as 128..255
 * (a value, not a sign-extended negative) — the same choice the kernel makes.
 * --------------------------------------------------------------------------- */
u32 tinyfs_name_hash(const char *name, u32 len)
{
	u64 hash = 0;

	while (len--)
		hash = partial_name_hash((unsigned char)*name++, hash);

	return end_name_hash(hash);
}

/* ---------------------------------------------------------------------------
 * tinyfs_next_ino — allocate the next inode number, skipping 0.
 *
 * Mirrors the kernel's get_next_ino() reserved-value guard: inode number 0 is
 * special (it means "no inode" in many contexts), so a counter that wraps past
 * the 32-bit maximum back to 0 must step over it. On this host `res == 0` after
 * the ++ can only happen on wraparound; the branch that adds one more is the
 * instructive bit — in the -O2 assembly the compiler may fold it into a
 * branchless `cmp`/`adc` or `cmove`. `counter` is a caller-owned cell (in the
 * real kernel it is a per-CPU variable, which is how it stays lock-free).
 * --------------------------------------------------------------------------- */
u32 tinyfs_next_ino(u32 *counter)
{
	u32 res = *counter;

	res++;                  /* hand out the next number                        */
	if (res == 0)           /* wrapped past UINT_MAX: 0 is reserved            */
		res++;          /* so skip it and hand out 1 instead              */

	*counter = res;         /* remember where we are for next time            */
	return res;
}
