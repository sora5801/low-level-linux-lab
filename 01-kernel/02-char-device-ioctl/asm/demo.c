/* ===========================================================================
 * asm/demo.c — the ring buffer's PURE-LOGIC core, extracted for assembly study.
 * ===========================================================================
 *
 * WHY THIS FILE EXISTS
 * --------------------
 * foo.c is a kernel module: it #includes <linux/...> and can only be compiled by
 * kbuild against a configured kernel tree, so you cannot run clang on it here to
 * see its assembly. But the *interesting* part of the driver — the ring buffer
 * index arithmetic in foo_read/foo_write — is pure integer math with no kernel
 * dependencies at all. So we lift that math OUT into this standalone file, which
 * declares its own types and includes nothing, and generate real assembly from
 * it. The functions below are byte-for-byte the same computations the driver
 * does; only the copy_to/from_user calls around them are gone.
 *
 * The whole trick rests on TWO facts:
 *   1. `capacity` is a power of two, so the physical offset of a free-running
 *      index is just `index & (capacity - 1)` — a single AND, no divide.
 *   2. head and tail are never wrapped; `head - tail` in unsigned arithmetic is
 *      the exact byte count even across the 2^32 wraparound, as long as the
 *      count never exceeds capacity (which the driver enforces).
 *
 * Read the generated asm to SEE the compiler turn `& (cap-1)` into `and`, turn
 * the `min` into a branchless `cmp`/`cmov`, and return a small struct packed
 * into a single register. That is the payoff of reading assembly.
 * =========================================================================== */

/* Our own fixed-width types so this file needs no headers whatsoever. On the
 * x86-64 SysV target these match the kernel's __u32 / unsigned int exactly. */
typedef unsigned int   u32;
typedef unsigned long  u64;

/* -------------------------------------------------------------------------
 * rb_count — bytes currently stored = producer index minus consumer index.
 *
 * Unsigned subtraction wraps modulo 2^32, which is PRECISELY what we want:
 * once head has advanced past 2^32 and wrapped back to a small value while tail
 * is still large, `head - tail` still yields the true distance between them.
 * The result is valid as long as the live count never exceeds capacity — an
 * invariant the driver upholds by refusing to write into a full buffer.
 * ABI: head in %edi, tail in %esi; returns in %eax. One `sub`. That's it.
 * ------------------------------------------------------------------------- */
u32 rb_count(u32 head, u32 tail)
{
	return head - tail;
}

/* -------------------------------------------------------------------------
 * rb_space — bytes free = capacity minus the current count.
 * ABI: head %edi, tail %esi, cap %edx; returns %eax.
 * ------------------------------------------------------------------------- */
u32 rb_space(u32 head, u32 tail, u32 cap)
{
	return cap - (head - tail);
}

/* -------------------------------------------------------------------------
 * rb_first_chunk — of `n` bytes starting at `index`, how many are contiguous
 * before the buffer wraps back to offset 0.
 *
 *   off       = index & (cap - 1)     physical position of the cursor
 *   tail_room = cap - off             bytes from there to the end of the buffer
 *   result    = min(n, tail_room)     copy at most what fits before the wrap
 *
 * This is the value `first` in foo_read/foo_write. If it equals n, the whole
 * transfer is contiguous; if it is less, the caller does a second copy of
 * (n - first) bytes starting at offset 0. Watch the compiler emit the min() as
 * a branchless compare + conditional move rather than a jump.
 * ABI: index %edi, cap %esi, n %edx; returns %eax.
 * ------------------------------------------------------------------------- */
u32 rb_first_chunk(u32 index, u32 cap, u32 n)
{
	u32 off       = index & (cap - 1u);   /* AND, because cap is a power of two */
	u32 tail_room = cap - off;
	return n < tail_room ? n : tail_room; /* min(n, tail_room) */
}

/* A transfer plan: how to split `n` bytes across the wrap boundary into a
 * contiguous first chunk and a wrapped-around remainder. Two u32s = 8 bytes,
 * which the SysV ABI returns PACKED IN A SINGLE 64-bit register (%rax) — a neat
 * detail the annotated assembly points out. */
struct rb_plan {
	u32 first;    /* bytes to copy at (index & mask), before the wrap  */
	u32 second;   /* remaining bytes to copy at offset 0, after the wrap */
};

/* -------------------------------------------------------------------------
 * rb_plan_xfer — compute the full two-part transfer plan in one shot. This is
 * exactly the decision foo_read/foo_write make before their copy_*_user calls.
 * ABI: index %edi, cap %esi, n %edx; returns the packed struct in %rax.
 * ------------------------------------------------------------------------- */
struct rb_plan rb_plan_xfer(u32 index, u32 cap, u32 n)
{
	u32 off       = index & (cap - 1u);
	u32 tail_room = cap - off;
	u32 first     = n < tail_room ? n : tail_room;   /* contiguous part      */

	struct rb_plan p;
	p.first  = first;
	p.second = n - first;                            /* wrapped remainder    */
	return p;
}

/* -------------------------------------------------------------------------
 * rb_selftest — a tiny driver so the file can be compiled AND run to prove the
 * math is right (independent of the kernel). It has no I/O; it just returns 0 on
 * success and a nonzero code identifying the first failing case, so you can run
 * it under a debugger or check `echo $?`. Kept out of the annotated walkthrough
 * because it is scaffolding, not the core logic.
 * ------------------------------------------------------------------------- */
int rb_selftest(void)
{
	const u32 CAP = 4096;              /* a power of two, like the driver */

	/* empty and full extremes */
	if (rb_count(10, 10) != 0)   return 1;    /* head==tail -> empty      */
	if (rb_count(10, 4)  != 6)   return 2;    /* 6 bytes buffered         */
	if (rb_space(10, 4, CAP) != CAP - 6) return 3;

	/* wrap math: a cursor 100 bytes from the end has 100 bytes of tail room */
	if (rb_first_chunk(CAP - 100, CAP, 40)  != 40)  return 4;  /* fits      */
	if (rb_first_chunk(CAP - 100, CAP, 250) != 100) return 5;  /* clamped   */

	/* the split plan for a wrapping transfer */
	struct rb_plan p = rb_plan_xfer(CAP - 100, CAP, 250);
	if (p.first != 100 || p.second != 150) return 6;

	/* count is correct even when the free-running index wraps past 2^32 */
	if (rb_count(3u, 0xFFFFFFFFu) != 4u) return 7;

	return 0;
}
