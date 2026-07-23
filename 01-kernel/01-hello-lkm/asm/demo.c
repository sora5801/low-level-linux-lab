/* ===========================================================================
 * demo.c — the pure-logic core of hello_lkm.c, extracted so it can be compiled
 *          to standalone assembly on ANY host.
 * ===========================================================================
 *
 * WHY THIS FILE EXISTS
 * --------------------
 * hello_lkm.c is a kernel module. You cannot run `clang -S hello_lkm.c` to see
 * its assembly, because it #includes <linux/module.h> and dozens of headers
 * that only exist inside a configured kernel build tree — the compile would die
 * on the very first include. So, following the lab convention, we lift the
 * module's dependency-free "brain" — the parameter-validation clamp and the
 * greeting hash — into this self-contained file. It includes NO headers and
 * declares its own types, so clang can compile it to real x86-64 assembly that
 * we then hand-annotate in demo.annotated.s.
 *
 * The two functions below are byte-for-byte the same logic as the `clamp_count`
 * and `fnv1a32` helpers in hello_lkm.c. Reading their assembly teaches you what
 * the CPU actually does for a bounds check and a hash loop — knowledge that
 * transfers directly to the module, whose in-kernel machine code is identical
 * in shape.
 * ===========================================================================
 */

/* We have no <stdint.h> here (that would be "using system headers"), so we
 * spell out the one width we need. On the x86-64 SysV target this file is
 * compiled for, `int` is 32 bits, so `unsigned int` is our 32-bit type. */
typedef unsigned int u32;

/* The legal range for the module's greeting count. Kept as macros (not enum)
 * so they are plain immediates in the assembly, exactly as in the module. */
#define COUNT_MIN 1
#define COUNT_MAX 10

/* ---------------------------------------------------------------------------
 * clamp_count — force an arbitrary integer into [COUNT_MIN, COUNT_MAX].
 * ---------------------------------------------------------------------------
 * SysV AMD64 ABI: the single int argument arrives in %edi (the low 32 bits of
 * %rdi), and the int result leaves in %eax. This is the module's range check;
 * in assembly it becomes a pair of compares that the optimizer collapses into
 * conditional moves — no branches, which is why it is worth staring at.
 */
int clamp_count(int count)
{
	if (count < COUNT_MIN)
		return COUNT_MIN;
	if (count > COUNT_MAX)
		return COUNT_MAX;
	return count;
}

/* ---------------------------------------------------------------------------
 * fnv1a32 — 32-bit FNV-1a hash of a NUL-terminated string.
 * ---------------------------------------------------------------------------
 * ABI: the `const char *` argument arrives in %rdi; the u32 result leaves in
 * %eax. This is the classic xor-then-multiply hash loop. The interesting thing
 * to watch in the assembly is how clang implements `h *= 16777619` — at higher
 * optimization levels it may strength-reduce the multiply into a few shifts and
 * adds (16777619 = 0x1000193), or just emit `imul`, depending on the level.
 */
u32 fnv1a32(const char *s)
{
	u32 h = 2166136261u;          /* FNV-1a 32-bit offset basis */

	while (*s) {
		h ^= (unsigned char)*s++; /* fold in next byte (unsigned: no sign
					   * extension for bytes >= 0x80)          */
		h *= 16777619u;           /* FNV 32-bit prime, 0x01000193          */
	}
	return h;
}

/* A tiny driver so the file links into a runnable program too (handy if you
 * want to `clang demo.c -o demo && ./demo`). It also gives the assembly a
 * `main` whose calls to the two helpers show the ABI in action: arguments
 * loaded into %edi/%rdi, results read from %eax. It is deliberately trivial and
 * kept out of the annotation's spotlight. */
int main(void)
{
	/* Return a value derived from both helpers so the optimizer cannot
	 * discard them as dead code. 5 is in range (clamp returns 5); the hash
	 * of "world" is mixed in and masked to a small exit status. */
	int c = clamp_count(5);
	u32 h = fnv1a32("world");

	return (int)((h ^ (u32)c) & 0x7f);
}
