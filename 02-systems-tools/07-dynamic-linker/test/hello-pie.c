/* ===========================================================================
 * hello-pie.c — a freestanding static-PIE, the simplest thing the loader runs.
 * ===========================================================================
 *
 * A static-PIE is ET_DYN with no PT_INTERP and no DT_NEEDED: it has no external
 * dependencies, but because it is position-independent it still carries
 * R_X86_64_RELATIVE (and often DT_RELR) relocations that must be applied before
 * it can run. That makes it the perfect first target: our loader maps it,
 * rebases it, runs its constructor, and jumps in — with zero symbol resolution.
 *
 * Run it:  ./loader ./test/hello-pie
 *
 * Build (Linux/WSL):
 *   clang -O2 -fPIE -static-pie -nostdlib hello-pie.c -o test/hello-pie
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

/* A GLOBAL pointer initialized to a string address. In a PIE the string's
 * absolute address isn't known until load time, so the linker emits an
 * R_X86_64_RELATIVE relocation for this pointer — the one our loader applies. */
const char *message = "Hello from a static-PIE, loaded by hand!\n";

__attribute__((constructor))
static void hello_ctor(void)
{
    static const char m[] = "[hello-pie] constructor ran before entry\n";
    sys_write(2, m, slen(m));
}

__attribute__((noreturn))
void _start(void)
{
    sys_write(1, message, slen(message));   /* deref the rebased pointer */
    sys_exit(0);
}
