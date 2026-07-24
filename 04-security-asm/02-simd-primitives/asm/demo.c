/* ===========================================================================
 * asm/demo.c — the scalar strlen, isolated so you can DIFF scalar vs vector.
 * ===========================================================================
 *
 * This file exists purely to be compiled to assembly at several -O levels and
 * read. It is self-contained: NO system headers, its OWN size type, so the
 * emitted .s is nothing but the loop you wrote — perfect for correlating C to
 * machine code. Generate the three committed listings with:
 *
 *   clang --target=x86_64-pc-linux-gnu -S -O0 -fno-asynchronous-unwind-tables \
 *         -fno-jump-tables -fno-omit-frame-pointer demo.c -o demo.O0.s
 *   clang --target=x86_64-pc-linux-gnu -S -O1 ... -o demo.s        (annotated)
 *   clang --target=x86_64-pc-linux-gnu -S -O2 ... -o demo.O2.s
 *
 * THE LESSON we want you to SEE in the output:
 *
 *   scalar_strlen() has an UNBOUNDED loop — it stops at a NUL whose position it
 *   cannot know in advance. The compiler therefore CANNOT auto-vectorize it: a
 *   16/32-byte vector load might read past the array end into an unmapped page
 *   and fault, and the compiler is not allowed to introduce a fault the source
 *   program would not have. So even at -O2/-O3 it stays a byte-at-a-time scan
 *   (maybe lightly unrolled). This is exactly WHY glibc and our simd_asm.S must
 *   hand-write the aligned-load trick: the compiler won't do it for you.
 *
 *   count_nonzero_bounded() has the SAME per-byte work but a KNOWN trip count
 *   (n). With no risk of over-reading, the compiler happily auto-vectorizes it
 *   (pcmpeqb/por + a horizontal reduce) at -O2/-O3. Diff the two functions in
 *   demo.O2.s to watch the optimizer make precisely that distinction.
 * ===========================================================================
 */

/* Our own size type so this file needs no <stddef.h>. On the LP64 model Linux
 * uses for x86-64, unsigned long is 64-bit — wide enough for any object size. */
typedef unsigned long usize;

/* ---------------------------------------------------------------------------
 * scalar_strlen — the classic. Walk to the NUL; return the distance.
 * UNBOUNDED loop => the compiler must NOT speculate wide loads => stays scalar.
 * --------------------------------------------------------------------------- */
usize scalar_strlen(const char *s)
{
    const char *p = s;          /* p advances to the terminator              */
    while (*p)                  /* read one byte; stop when it is 0x00        */
        p++;
    return (usize)(p - s);       /* pointer difference = length               */
}

/* ---------------------------------------------------------------------------
 * count_nonzero_bounded — same byte test, but a KNOWN count `n`.
 * BOUNDED loop => safe to vectorize => the optimizer will use SIMD at -O2/-O3.
 * Included only as the contrast case for the diff; not used by the library.
 * --------------------------------------------------------------------------- */
usize count_nonzero_bounded(const char *s, usize n)
{
    usize count = 0;
    for (usize i = 0; i < n; i++)   /* trip count known up front             */
        if (s[i] != 0)              /* the optimizer can prove no over-read  */
            count++;
    return count;
}
