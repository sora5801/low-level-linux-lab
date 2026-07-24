/* ===========================================================================
 * string.c — the byte-level primitives (<string.h>).
 * ===========================================================================
 *
 * These are the smallest, most-used routines in all of C. Two of them —
 * memcpy and memset — are special: the COMPILER itself emits calls to them
 * for aggregate assignments and array initialization, even in code that never
 * types the names. That is why a `-nostdlib` program still needs them, and why
 * we build with -fno-builtin: otherwise clang could "optimize" the body of our
 * memset into a call to memset (itself), an infinite recursion.
 *
 * These are the deliberately-simple, byte-at-a-time versions. glibc/musl beat
 * them by an order of magnitude using SIMD to move 16/32/64 bytes per iteration
 * and by handling alignment — see this lab's 02-simd-primitives. The lesson
 * here is the CONTRACT (especially the no-overlap rule), not the speed.
 * ===========================================================================
 */
#include "minilibc.h"

/* strlen — number of bytes before the NUL terminator (the NUL is not counted).
 * INVARIANT: `s` must point at a NUL-terminated string; there is no length
 * bound, so a missing terminator walks off into whatever follows in memory —
 * the root of countless overflow bugs. O(n) in the string length. */
size_t strlen(const char *s)
{
	const char *p = s;
	while (*p != '\0')          /* advance until we hit the 0 byte            */
		p++;
	return (size_t)(p - s);     /* pointer difference = count of non-NUL bytes */
}

/* strcpy — copy src (including its NUL) into dst; return dst.
 * INVARIANT: dst must have room for strlen(src)+1 bytes. strcpy CANNOT know
 * dst's size, so it will happily overrun a short buffer — this function is a
 * textbook overflow primitive. Prefer a bounded copy (strlcpy/snprintf) in
 * real code; we provide the classic for teaching and for the demos. */
char *strcpy(char *dst, const char *src)
{
	char *d = dst;
	while ((*d = *src) != '\0') {   /* copy the byte, THEN test if it was NUL  */
		d++;
		src++;
	}
	/* The loop already wrote the terminating NUL (the assignment that tested
	 * false stored a 0), so we are done. */
	return dst;
}

/* strcmp — lexicographic byte compare. Returns <0, 0, or >0 by the first
 * differing byte, compared as UNSIGNED char (the standard's rule; comparing as
 * signed char would order bytes >= 0x80 wrongly). Stops at the first mismatch
 * or at a shared NUL. */
int strcmp(const char *a, const char *b)
{
	const unsigned char *ua = (const unsigned char *)a;
	const unsigned char *ub = (const unsigned char *)b;
	while (*ua != '\0' && *ua == *ub) {   /* walk while equal and not at end   */
		ua++;
		ub++;
	}
	return (int)*ua - (int)*ub;            /* difference of first differing pair */
}

/* memcpy — copy exactly n bytes from src to dst; return dst.
 * INVARIANT (critical): the regions MUST NOT overlap. memcpy is permitted to
 * copy in any order (real ones copy in big aligned chunks and even backwards),
 * so overlapping copies produce garbage. If they can overlap, memmove is the
 * correct tool. We copy forward, one byte at a time. */
void *memcpy(void *dst, const void *src, size_t n)
{
	unsigned char *d = (unsigned char *)dst;
	const unsigned char *s = (const unsigned char *)src;
	while (n-- > 0)             /* post-decrement: run the body exactly n times */
		*d++ = *s++;
	return dst;
}

/* memset — write the low byte of `c` into n bytes starting at dst; return dst.
 * Used to zero buffers (memset(p,0,n)) and to poison freed memory. `c` is an
 * int by the standard's prototype but only its least-significant byte is used,
 * hence the (unsigned char) truncation. */
void *memset(void *dst, int c, size_t n)
{
	unsigned char *d = (unsigned char *)dst;
	unsigned char b = (unsigned char)c;   /* only the low 8 bits matter        */
	while (n-- > 0)
		*d++ = b;
	return dst;
}
