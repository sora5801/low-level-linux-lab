/* ===========================================================================
 * greetlib.c — a freestanding shared object (libgreet.so) for the loader test.
 * ===========================================================================
 *
 * It has NO libc so our loader can bind it without needing to load glibc. It
 * exports exactly the shapes of symbol our loader must resolve:
 *
 *   greet_message()  — a FUNCTION. The main program calls it through the PLT,
 *                      which the linker records as an R_X86_64_JUMP_SLOT
 *                      relocation in the executable.
 *   greet_answer     — a DATA object. Reading it from another object goes
 *                      through the GOT, recorded as R_X86_64_GLOB_DAT.
 *   greet_name       — a global POINTER *inside* this library. Its initializer
 *                      (the address of a string literal) is position-dependent,
 *                      so the linker emits an R_X86_64_RELATIVE for it here.
 *
 * A constructor prints a line so you can see DT_INIT_ARRAY run before main.
 *
 * Build (Linux/WSL):
 *   clang -O2 -fPIC -nostdlib -shared -Wl,--hash-style=both \
 *         -Wl,-soname,libgreet.so greetlib.c -o libgreet.so
 * ========================================================================= */

/* Minimal syscall write, so the library can speak without libc. */
static long sys_write(int fd, const void *buf, unsigned long n)
{
    long ret;
    __asm__ volatile("syscall"
                     : "=a"(ret)
                     : "a"(1L), "D"((long)fd), "S"((long)buf), "d"((long)n)
                     : "rcx", "r11", "memory");
    return ret;
}
static unsigned long slen(const char *s)
{ const char *p = s; while (*p) p++; return (unsigned long)(p - s); }

/* Exported data symbol (GLOB_DAT when read from another object). Default
 * visibility + external linkage puts it in .dynsym. */
int greet_answer = 42;

/* Exported global pointer. The initializer needs an R_X86_64_RELATIVE here so
 * that, wherever the library is loaded, greet_name points at the string. */
const char *greet_name = "dynamic-linker-lab";

/* Exported function (JUMP_SLOT when called from another object). */
const char *greet_message(void)
{
    return "Hello from greetlib.so!";
}

/* A constructor: its pointer lands in this library's .init_array, and our
 * loader runs it (dependencies first) before jumping to the program. */
__attribute__((constructor))
static void greetlib_ctor(void)
{
    static const char m[] = "[greetlib] constructor ran (DT_INIT_ARRAY)\n";
    sys_write(2, m, slen(m));
}
