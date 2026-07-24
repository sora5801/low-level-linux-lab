/* ===========================================================================
 * asm/demo.c — the serial-validation transform, SELF-CONTAINED for codegen.
 * ===========================================================================
 *
 * This file exists so the committed assembly (demo.s / demo.O0.s / demo.O2.s /
 * demo.annotated.s) is EXACTLY the codegen a reverser would face when opening
 * the crackme in objdump or Ghidra. It is a byte-for-byte logical mirror of
 * serial.h, but with:
 *   - no system headers (own fixed-width typedefs), so it compiles anywhere
 *     with `clang --target=x86_64-pc-linux-gnu -S` and produces clean Linux asm;
 *   - the two hot routines (`key_from_name`, `validate`) as EXTERNAL symbols so
 *     the optimizer must emit them at -O1/-O2 (a `static` unused function would
 *     be dropped, leaving nothing to annotate).
 *
 * What to look for in the generated asm (see demo.annotated.s for the full
 * line-by-line tour):
 *   - `movabsq $0x100000001b3, %rXX`   the FNV prime — your landmark for
 *                                      "this is the hash multiply."
 *   - `imulq`                          the multiply-mod-2^64 step.
 *   - `rolq $7, %rXX`                  the rotate — the single most recognizable
 *                                      instruction of the whole transform.
 *   - the three `shr`/`xor`/`imul` pairs of the fmix64 avalanche finalizer.
 *   - in `validate`: the shift/mask group extraction and the branch-free
 *     constant-time comparison loop.
 *
 * There is a tiny `main` at the bottom so the file also links into a standalone
 * self-test (see the Makefile `demo` target); it is not part of what you
 * annotate, but it makes the routines callable for a sanity check.
 * ===========================================================================
 */

/* --- our own fixed-width types (no <stdint.h>) ---------------------------- */
typedef unsigned long long u64;   /* 64-bit on the LP64 Linux x86-64 target    */
typedef unsigned int       u32;   /* 32-bit                                    */
typedef unsigned char      u8;    /* 8-bit byte                                */

/* Same named constants as serial.h, so the immediates in the asm match the
 * crackme's. A reverser greps the disassembly for these to *find* the routine. */
#define FNV_OFFSET 0xCBF29CE484222325ULL
#define FNV_PRIME  0x00000100000001B3ULL
#define XOR_CONST  0x000000005DEECE66DULL
#define FMIX_C1    0xFF51AFD7ED558CCDULL
#define FMIX_C2    0xC4CEB9FE1A85EC53ULL

/* rotl64 — rotate left; compiles to a single `rolq $7` when r is the constant 7.
 * static+inline so it folds into key_from_name rather than emitting a call. */
static inline u64 rotl64(u64 x, unsigned r)
{
    return (x << r) | (x >> (64 - r));
}

/* key_from_name — THE transform. EXTERNAL so it is emitted at every -O level. */
u64 key_from_name(const char *name)
{
    u64 h = FNV_OFFSET;

    /* per-byte mixing loop: xor-in, multiply, rotate, xor-const */
    for (const u8 *p = (const u8 *)name; *p; ++p) {
        h ^= (u64)*p;          /* fold in the byte                             */
        h *= FNV_PRIME;        /* imulq by the odd prime (invertible mod 2^64) */
        h  = rotl64(h, 7);     /* rolq $7 — diffuse                            */
        h ^= XOR_CONST;        /* constant xor                                */
    }

    /* fmix64 avalanche: shr/xor/imul x2 then a final shr/xor */
    h ^= h >> 33;
    h *= FMIX_C1;
    h ^= h >> 29;
    h *= FMIX_C2;
    h ^= h >> 33;
    return h;
}

/* format_serial — render key as GGGG-GGGG-GGGG-GGGG into out[20] (see serial.h
 * for the full commentary; identical logic here). static: inlined into validate. */
static void format_serial(u64 key, char *out)
{
    static const char HEX[] = "0123456789ABCDEF";
    unsigned oi = 0;
    for (unsigned gi = 0; gi < 4; ++gi) {
        unsigned group = (unsigned)((key >> (48 - 16 * gi)) & 0xFFFFu);
        out[oi++] = HEX[(group >> 12) & 0xF];
        out[oi++] = HEX[(group >>  8) & 0xF];
        out[oi++] = HEX[(group >>  4) & 0xF];
        out[oi++] = HEX[ group        & 0xF];
        if (gi != 3) out[oi++] = '-';
    }
    out[oi] = '\0';
}

/* slen — strlen without <string.h>. static: inlined. */
static unsigned slen(const char *s)
{
    const char *p = s;
    while (*p) ++p;
    return (unsigned)(p - s);
}

/* ct_equal — constant-time n-byte compare; branch-free zero test (see serial.h). */
static int ct_equal(const char *a, const char *b, unsigned n)
{
    unsigned diff = 0;
    for (unsigned i = 0; i < n; ++i)
        diff |= (unsigned)((u8)a[i] ^ (u8)b[i]);
    return (int)(1u ^ ((diff | (0u - diff)) >> 31));
}

/* validate — the whole license check as one function: 1 == accept. EXTERNAL so
 * it is emitted; this is the routine whose CONTROL FLOW (compute expected,
 * length-gate, constant-time compare) a reverser reconstructs. */
int validate(const char *name, const char *entered)
{
    char expected[20];
    u64 key = key_from_name(name);
    format_serial(key, expected);
    if (slen(entered) != 19) return 0;    /* length gate                       */
    return ct_equal(entered, expected, 19);
}

/* --- optional standalone self-test ---------------------------------------- *
 * Compiled only when DEMO_MAIN is defined (Makefile `demo` target). It is NOT
 * part of the annotated assembly; it just lets you run the transform natively.
 * Uses a hand-rolled writer so the file still needs no libc headers to compile
 * to asm; the self-test path uses write(2) via a tiny inline syscall.          */
#ifdef DEMO_MAIN
static long sys_write(long fd, const void *buf, unsigned long len)
{
    long ret;
    __asm__ volatile ("syscall"
        : "=a"(ret)
        : "a"(1L), "D"(fd), "S"(buf), "d"(len)   /* SYS_write=1; fd,buf,len     */
        : "rcx", "r11", "memory");
    return ret;
}
int main(int argc, char **argv)
{
    if (argc != 2) { sys_write(2, "need a name\n", 12); return 2; }
    char serial[20];
    format_serial(key_from_name(argv[1]), serial);
    serial[19] = '\n';                     /* replace NUL with newline for print */
    sys_write(1, serial, 20);
    /* prove validate() agrees with the freshly-formatted serial */
    serial[19] = '\0';
    return validate(argv[1], serial) ? 0 : 1;
}
#endif
