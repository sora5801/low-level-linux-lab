/* ===========================================================================
 * libfakeptrace.c — an LD_PRELOAD shim that neutralizes the ptrace anti-debug.
 * ===========================================================================
 *
 * HOW LD_PRELOAD HOOKING WORKS
 * ----------------------------
 * When a dynamically-linked program calls `ptrace`, it does not jump to a fixed
 * address — it jumps through the PLT/GOT to whatever the dynamic loader (ld.so)
 * resolved the symbol to. The loader searches libraries in order, and anything
 * named in the LD_PRELOAD environment variable is searched FIRST. So if we ship
 * a library that exports its own `ptrace` symbol and preload it, every call the
 * crackme makes to `ptrace` lands HERE instead of in glibc:
 *
 *     LD_PRELOAD=./libfakeptrace.so ./crackme alice 1A2B-3C4D-5E6F-7089
 *
 * Our fake always reports success (returns 0), so `anti_debug_ptrace()` in the
 * crackme believes it attached to itself cleanly even while gdb holds the real
 * trace slot. The self-ptrace gate is defeated without touching a single byte
 * of the target on disk.
 *
 * WHY THE CRACKME IS HOOKABLE (and how it could resist)
 * -----------------------------------------------------
 * This works ONLY because the crackme calls the libc wrapper `ptrace()` (a
 * dynamic symbol resolved through the PLT). If the target issued the raw
 * `syscall` instruction with rax=101 (SYS_ptrace) itself, there would be no
 * symbol to interpose and LD_PRELOAD would do nothing — you would fall back to
 * byte-patching the branch (docs/writeup.md) or to a seccomp/ptrace syscall
 * filter that rewrites the return value. Knowing *which* bypass a given target
 * admits is the skill this project teaches.
 *
 * DEFENSE FRAMING: interposition cuts both ways. Blue teams use exactly this
 * mechanism (a preloaded shim, or the loader's audit interface) to sandbox,
 * log, and constrain untrusted binaries; the same trick that defeats a toy
 * anti-debug also implements syscall allow-lists and API monitoring. It is
 * defeated in turn by static linking, raw syscalls, and integrity checks — an
 * arms race with no client-side winner, which is the README's whole thesis.
 *
 * Build:   clang -shared -fPIC libfakeptrace.c -o libfakeptrace.so
 * ===========================================================================
 */

#include <stdio.h>       /* fprintf (one-time note to stderr)                  */

/* We intentionally do NOT `#include <sys/ptrace.h>` here: that header declares
 * `ptrace` with glibc's exact prototype (variadic-ish, enum request), and we
 * want to define our own symbol with a simple, ABI-compatible signature. At the
 * call site the crackme passes four integer/pointer args in rdi, rsi, rdx, rcx;
 * a plain 4-argument `long` function reads whichever it needs and ignores the
 * rest. The symbol name `ptrace` is all the dynamic linker matches on. */
long ptrace(long request, long pid, long addr, long data)
{
    (void)pid; (void)addr; (void)data;

    /* Print a one-line note the first time we are called, so the reader can SEE
     * the interposition happening. Not required for the bypass — comment it out
     * for a silent hook. */
    static int announced = 0;
    if (!announced) {
        announced = 1;
        fprintf(stderr,
            "[libfakeptrace] intercepted ptrace(request=%ld) -> returning 0 "
            "(anti-debug neutralized)\n", request);
    }

    /* Always claim success. The crackme's check treats -1 as "debugger present"
     * and anything else as "clear", so 0 makes it believe its self-attach
     * succeeded even under gdb. */
    return 0;
}
