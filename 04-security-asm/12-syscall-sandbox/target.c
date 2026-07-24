/* ===========================================================================
 * target.c — the program we confine, and the one that TRIES TO ESCAPE.
 * ===========================================================================
 *
 * This is a static, NO-LIBC binary (its own _start, raw `syscall`s) for one
 * reason: its trust surface is exactly the syscalls written below, nothing more.
 * A glibc "hello world" secretly makes dozens of startup syscalls (brk,
 * arch_prctl, mmap, the dynamic loader's opens...), which would force the
 * allowlist to be large and muddy the lesson. Here you can read every syscall
 * the confined program makes, and match it against seccomp_filter.c's policy.
 *
 * It walks through one example of EACH seccomp action, ending with a deliberate
 * escape attempt that the allowlist's default-deny turns fatal:
 *
 *   write/getpid   -> ACT_ALLOW : normal, succeeds.
 *   ptrace(TRACEME)-> ACT_ERRNO : neutralized to -EPERM; we survive.
 *   openat(path)   -> ACT_TRACE : the supervisor (or Landlock) rules on the path.
 *   socket(...)    -> ACT_KILL  : not on the allowlist -> SIGSYS, process dies.
 *
 * Build it like the nolibc reference:
 *   clang -nostdlib -static -no-pie -fno-stack-protector -Os target.c -o target
 *
 * On non-Linux hosts this file will not build (there is no Linux `syscall`),
 * which is expected — the whole project is Linux/WSL. The asm/ deliverable is
 * the part that compiles anywhere (clang cross-targets Linux).
 * ===========================================================================
 */

/* ---- x86-64 Linux syscall numbers (kernel ABI; stable) -------------------- */
#define SYS_read         0
#define SYS_write        1
#define SYS_close        3
#define SYS_socket      41
#define SYS_ptrace     101
#define SYS_getpid      39
#define SYS_openat     257
#define SYS_exit_group 231

/* ---- assorted constants we would normally get from headers ---------------- */
#define AT_FDCWD       (-100)   /* openat: resolve relative to CWD             */
#define O_RDONLY          0
#define PTRACE_TRACEME    0
#define AF_INET           2
#define SOCK_STREAM       1

typedef unsigned long usize;
typedef long          ssize;

/* ---------------------------------------------------------------------------
 * Raw syscall wrappers. The x86-64 syscall ABI puts the number in rax and the
 * six args in rdi, rsi, rdx, r10, r8, r9 (note: r10, NOT rcx, for arg4 — the
 * `syscall` instruction destroys rcx and r11). We bind the 4th arg to r10 with
 * a register-local so the compiler places it correctly.
 * --------------------------------------------------------------------------- */
static inline long sc3(long n, long a1, long a2, long a3)
{
    long ret;
    __asm__ volatile("syscall"
        : "=a"(ret)
        : "a"(n), "D"(a1), "S"(a2), "d"(a3)
        : "rcx", "r11", "memory");
    return ret;
}
static inline long sc4(long n, long a1, long a2, long a3, long a4)
{
    long ret;
    register long r10 __asm__("r10") = a4;   /* arg4 MUST be in r10, not rcx  */
    __asm__ volatile("syscall"
        : "=a"(ret)
        : "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10)
        : "rcx", "r11", "memory");
    return ret;
}

/* ---- minimal I/O helpers (no libc) ---------------------------------------- */
static usize kstrlen(const char *s) { const char *p = s; while (*p) p++; return (usize)(p - s); }
static void  puts_(const char *s)   { sc3(SYS_write, 2, (long)s, (long)kstrlen(s)); } /* -> stderr */

/* Print a signed long in decimal to stderr, no libc. */
static void putl_(long v)
{
    char buf[24]; int i = (int)sizeof(buf); int neg = 0;
    unsigned long u = (unsigned long)v;
    if (v < 0) { neg = 1; u = (unsigned long)(-v); }
    buf[--i] = '\n';
    if (u == 0) buf[--i] = '0';
    while (u) { buf[--i] = (char)('0' + u % 10); u /= 10; }
    if (neg) buf[--i] = '-';
    sc3(SYS_write, 2, (long)&buf[i], (long)((int)sizeof(buf) - i));
}

__attribute__((noreturn)) static void die(int code)
{
    sc3(SYS_exit_group, code, 0, 0);
    __builtin_unreachable();
}

/* ---------------------------------------------------------------------------
 * _start — the confined program.
 * --------------------------------------------------------------------------- */
__attribute__((noreturn)) void _start(void)
{
    puts_("[target] hello from inside the sandbox\n");

    /* (A) ACT_ALLOW: getpid always succeeds. */
    long pid = sc3(SYS_getpid, 0, 0, 0);
    puts_("[target] getpid() = ");
    putl_(pid);

    /* (B) ACT_ERRNO: a classic anti-debug probe. Under seccomp this returns
     * -EPERM (errno 1) instead of succeeding OR killing us — the process copes
     * and continues. On Linux a negative syscall return is -errno. */
    long pr = sc4(SYS_ptrace, PTRACE_TRACEME, 0, 0, 0);
    puts_("[target] ptrace(TRACEME) returned ");
    putl_(pr);
    puts_(pr < 0 ? "[target]   -> blocked with an errno; I survived (ACT_ERRNO)\n"
                 : "[target]   -> succeeded (no seccomp ERRNO in effect)\n");

    /* (C) ACT_TRACE / Landlock: open a path the policy MIGHT allow. Whether
     * this succeeds depends on --allow (Landlock) and --trace (supervisor). */
    {
        static const char ok[] = "/etc/hostname";
        long fd = sc4(SYS_openat, AT_FDCWD, (long)ok, O_RDONLY, 0);
        puts_("[target] openat(\"/etc/hostname\") = ");
        putl_(fd);
        if (fd >= 0) {
            char b[64];
            long r = sc3(SYS_read, fd, (long)b, (long)sizeof(b));
            if (r > 0) { puts_("[target]   contents: "); sc3(SYS_write, 2, (long)b, r); }
            sc3(SYS_close, fd, 0, 0);
        } else {
            puts_("[target]   -> denied (Landlock/supervisor said no)\n");
        }
    }

    /* (D) ACT_TRACE / Landlock: a path we expect to be OUTSIDE the allowlist. */
    {
        static const char bad[] = "/etc/shadow";
        long fd = sc4(SYS_openat, AT_FDCWD, (long)bad, O_RDONLY, 0);
        puts_("[target] openat(\"/etc/shadow\") = ");
        putl_(fd);
        if (fd >= 0) { puts_("[target]   -> OPENED (sandbox gap!)\n"); sc3(SYS_close, fd, 0, 0); }
        else         puts_("[target]   -> denied, as intended\n");
    }

    /* (E) ACT_KILL: cross the line. socket() is NOT on the allowlist, so the
     * seccomp default-deny fires and the kernel kills us with SIGSYS. Nothing
     * below this call runs. This is the whole point of an allowlist: the
     * syscall you never thought to list is denied for free. */
    puts_("[target] now crossing the line: calling socket() (not allowlisted)\n");
    long s = sc3(SYS_socket, AF_INET, SOCK_STREAM, 0);

    /* If we reach here, the sandbox FAILED to block socket(). */
    puts_("[target] socket() returned ");
    putl_(s);
    puts_("[target] !! if you can read this, the allowlist did not hold\n");
    die(0);
}
