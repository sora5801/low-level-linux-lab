/* ===========================================================================
 * demo.c — the DEFENSIVE core of the shellcode project: a bad-char scanner.
 * ===========================================================================
 *
 * This is deliberately the blue-team primitive. Whether you are a defender
 * inspecting a buffer for an injected payload, or a student reasoning about a
 * delivery channel, the fundamental question is the same:
 *
 *     "Does this byte buffer contain any 'bad' characters — bytes that the
 *      channel it travels through will mangle or stop on?"
 *
 * The canonical bad char is 0x00 (NUL): a strcpy-style copy stops there, so a
 * payload delivered through such a copy must be NUL-free. Line-oriented input
 * also chokes on 0x0a (\n) and 0x0d (\r). An intrusion-detection system runs
 * essentially this scan (plus signature matching) over memory and network
 * buffers.
 *
 * Everything here is self-contained (no headers) so it compiles straight to
 * Linux System V assembly for the annotated teaching file. See asm/demo.s.
 * =========================================================================== */

typedef unsigned char  u8;
typedef unsigned long  usize;

/* A 256-bit set of "bad" byte values, stored as 32 bytes = one bit per byte
 * value. bit (c) is set  ->  byte value c is considered bad. Using a bitset
 * (rather than a list) makes the membership test O(1) and branch-light, which
 * is exactly what you want in a hot scanning loop. */
typedef struct { u8 bits[32]; } badset;

/* Mark byte value c as bad: set bit c in the 256-bit set.
 *   c >> 3   selects the byte of the bitset (c / 8)
 *   c & 7    selects the bit within that byte (c % 8) */
static void badset_add(badset *s, u8 c)
{
    s->bits[c >> 3] |= (u8)(1u << (c & 7u));
}

/* Test membership: is byte value c in the bad set? Returns 0 or 1.
 * The shift+mask is the inverse of badset_add. This compiles to a couple of
 * instructions with no data-dependent branch on c's value. */
static int badset_has(const badset *s, u8 c)
{
    return (s->bits[c >> 3] >> (c & 7u)) & 1u;
}

/* ---------------------------------------------------------------------------
 * contains_badchar — the heart of the scanner.
 *
 * Walk `len` bytes of `buf`; return the offset (0-based) of the FIRST byte that
 * is in the bad set, or -1 if the buffer is clean. Returning the offset (not
 * just a bool) is what makes the tool useful: a defender wants to know *where*,
 * and a channel-analysis tells you which byte to re-encode.
 *
 * ABI: buf in rdi, len in rsi, set in rdx; result (long) in rax.
 * --------------------------------------------------------------------------- */
long contains_badchar(const u8 *buf, usize len, const badset *set)
{
    for (usize i = 0; i < len; i++) {   /* linear sweep, O(n)                */
        if (badset_has(set, buf[i]))    /* O(1) bitset membership test       */
            return (long)i;             /* first hit: report its offset      */
    }
    return -1;                          /* clean: no bad byte present         */
}

/* Convenience: the classic "is this buffer NUL-free?" question, which is the
 * single most common bad-char check. Returns 1 if NUL-free, 0 otherwise.
 * This is really just memchr(buf, 0, len) == NULL, and at -O2 clang will
 * recognize the pattern and may lower it to exactly that. */
int is_nul_free(const u8 *buf, usize len)
{
    for (usize i = 0; i < len; i++)
        if (buf[i] == 0)                /* found a NUL -> not NUL-free        */
            return 0;
    return 1;                           /* swept the whole buffer, all nonzero */
}

/* A tiny self-test so `clang demo.c && ./a.out; echo $?` is meaningful.
 * Returns 0 on success (all assertions hold), nonzero otherwise. */
int demo_selftest(void)
{
    badset s = (badset){ .bits = {0} };
    badset_add(&s, 0x00);               /* NUL is bad                         */
    badset_add(&s, 0x0a);               /* newline is bad                     */

    const u8 clean[] = { 'A', 'B', 'C', 0x7f };
    const u8 dirty[] = { 'A', 0x0a, 'C' };          /* newline at offset 1    */

    if (contains_badchar(clean, sizeof clean, &s) != -1) return 1;
    if (contains_badchar(dirty, sizeof dirty, &s) != 1)  return 2;
    if (is_nul_free(clean, sizeof clean) != 1)           return 3;

    const u8 hasnul[] = { 'X', 0x00, 'Z' };
    if (is_nul_free(hasnul, sizeof hasnul) != 0)         return 4;
    return 0;                           /* all good                           */
}
