/* ===========================================================================
 * mem.c — the four memory builtins a freestanding kernel must provide itself.
 * ===========================================================================
 *
 * WHY THIS FILE IS NON-NEGOTIABLE
 * -------------------------------
 * Even under `-ffreestanding`, the C standard lets the compiler emit calls to
 * FOUR functions on its own initiative — memcpy, memmove, memset, memcmp — for
 * things like a struct assignment or an array loop it decides to vectorize.
 * There is no libc to satisfy those calls, so if we do not define them the
 * kernel fails to LINK (undefined reference) or, worse, links against nothing.
 * These are the textbook scalar implementations; correctness matters more than
 * speed at this size, and defining them keeps the whole kernel self-contained.
 * =========================================================================== */
#include "types.h"

/* Copy `n` bytes from `src` to `dst`. UB if the ranges overlap — use memmove
 * for that. Returns dst per the C contract. */
void *memcpy(void *dst, const void *src, size_t n)
{
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    for (size_t i = 0; i < n; i++)
        d[i] = s[i];
    return dst;
}

/* Copy `n` bytes even when the ranges overlap. If dst is above src within the
 * overlap we must copy BACKWARD so we read each source byte before it is
 * overwritten; otherwise forward is safe. This direction choice is the whole
 * reason memmove exists separately from memcpy. */
void *memmove(void *dst, const void *src, size_t n)
{
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    if (d < s)
        for (size_t i = 0; i < n; i++) d[i] = s[i];         /* forward  */
    else
        for (size_t i = n; i-- > 0; )   d[i] = s[i];         /* backward */
    return dst;
}

/* Fill `n` bytes of `dst` with byte `c`. Returns dst. */
void *memset(void *dst, int c, size_t n)
{
    uint8_t *d = (uint8_t *)dst;
    for (size_t i = 0; i < n; i++)
        d[i] = (uint8_t)c;
    return dst;
}

/* Compare `n` bytes; return <0/0/>0 like the standard. */
int memcmp(const void *a, const void *b, size_t n)
{
    const uint8_t *x = (const uint8_t *)a, *y = (const uint8_t *)b;
    for (size_t i = 0; i < n; i++)
        if (x[i] != y[i])
            return (int)x[i] - (int)y[i];
    return 0;
}
