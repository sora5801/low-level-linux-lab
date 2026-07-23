/* ===========================================================================
 * prog.c — a freestanding DYNAMIC executable that depends on libgreet.so.
 * ===========================================================================
 *
 * We run it under our own loader:
 *     ./loader ./test/prog
 *
 * It references three symbols from libgreet.so, one of each relocation shape:
 *   - greet_message()  -> called through the PLT   (R_X86_64_JUMP_SLOT)
 *   - greet_answer     -> read through the GOT      (R_X86_64_GLOB_DAT)
 *   - greet_name       -> its address taken via GOT (R_X86_64_GLOB_DAT)
 * plus its own global pointer (R_X86_64_RELATIVE) and a constructor.
 *
 * Build (Linux/WSL), producing an ET_DYN with a DT_NEEDED of libgreet.so:
 *   clang -O2 -fPIE -nostdlib -pie -Wl,--hash-style=both \
 *         test/prog.c -L test -lgreet -o test/prog
 * ========================================================================= */

static long sys_write(int fd, const void *buf, unsigned long n)
{
    long ret;
    __asm__ volatile("syscall"
                     : "=a"(ret)
                     : "a"(1L), "D"((long)fd), "S"((long)buf), "d"((long)n)
                     : "rcx", "r11", "memory");
    return ret;
}
__attribute__((noreturn))
static void sys_exit(int code)
{
    __asm__ volatile("syscall" : : "a"(231L), "D"((long)code) : "rcx", "r11");
    __builtin_unreachable();
}
static unsigned long slen(const char *s)
{ const char *p = s; while (*p) p++; return (unsigned long)(p - s); }
static void puts2(const char *s) { sys_write(1, s, slen(s)); }

/* These live in libgreet.so; the linker leaves them UNDEF here and records a
 * relocation so our loader can bind them at load time. */
extern const char *greet_message(void);
extern int greet_answer;
extern const char *greet_name;

/* A global pointer of our own — its initializer is an R_X86_64_RELATIVE in
 * THIS object, proving the loader rebased us correctly. */
static const char *banner = "[prog] running under the toy dynamic linker\n";

__attribute__((constructor))
static void prog_ctor(void)
{
    static const char m[] = "[prog] constructor ran (DT_INIT_ARRAY)\n";
    sys_write(2, m, slen(m));
}

/* Print a small non-negative integer in decimal (no libc). */
static void put_uint(unsigned v)
{
    char b[12]; int i = 12;
    b[--i] = '\n';
    if (v == 0) b[--i] = '0';
    while (v) { b[--i] = (char)('0' + v % 10); v /= 10; }
    sys_write(1, b + i, (unsigned long)(12 - i));
}

/* The freestanding entry point. No libc, so we ARE _start. */
__attribute__((noreturn))
void _start(void)
{
    puts2(banner);                              /* our own RELATIVE pointer   */
    puts2(greet_message());                     /* JUMP_SLOT into libgreet    */
    sys_write(1, "\n", 1);
    puts2("name = ");
    puts2(greet_name);                          /* GLOB_DAT: pointer in lib   */
    sys_write(1, "\n", 1);
    puts2("answer = ");
    put_uint((unsigned)greet_answer);           /* GLOB_DAT: int in lib       */
    sys_exit(0);
}
