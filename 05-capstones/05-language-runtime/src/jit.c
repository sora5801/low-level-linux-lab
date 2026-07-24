/* ===========================================================================
 * jit.c — hand-emit x86-64 for one hot loop, run it from PROT_EXEC memory.
 * ===========================================================================
 *
 * Platform: Linux/x86-64 only (mmap/mprotect + executing raw bytes).
 *
 * This is the smallest honest JIT: we KNOW the machine code for `sum(1..n)` and
 * emit its exact bytes. A production JIT would generate these bytes from the hot
 * function's bytecode at run time and patch in constants; here the shape is fixed
 * so every byte can be explained. The lifecycle is the real, reusable part:
 *
 *   mmap(RW)  ->  write code  ->  mprotect(RX)  ->  call via fn pointer  ->  munmap
 *
 * W^X: the page is writable while we fill it, then flipped to read+execute with
 * PROT_WRITE dropped. It is never writable-and-executable at the same instant —
 * the invariant hardened kernels (and this code) enforce.
 *
 * The emitted routine, in SysV AMD64 (arg n in %rdi, result in %rax):
 *
 *     xor  %eax,%eax        ; 31 C0        acc = 0   (also zero-extends rax)
 *     xor  %ecx,%ecx        ; 31 C9        i   = 0
 *   L:cmp  %rdi,%rcx        ; 48 39 F9     flags = i - n
 *     jge  E                ; 7D 08        if i >= n, leave the loop
 *     inc  %rcx             ; 48 FF C1     i = i + 1        (now 1..n)
 *     add  %rcx,%rax        ; 48 01 C8     acc = acc + i
 *     jmp  L                ; EB F3        (rel8 = -13)
 *   E:ret                   ; C3           return acc in rax
 *
 * The two REX.W prefixes (0x48) promote the ops to 64-bit; the ModRM bytes encode
 * mod=11 (register-direct) plus the reg/rm register numbers. rel8 displacements
 * are measured from the END of the jump instruction and are little-endian (here
 * one byte). Cross-check any byte with `llvm-mc --show-encoding`.
 */
#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "jit.h"

/* The reference answer, computed EXACTLY as the emitted code does (acc += i for
 * i in 1..n), so a match proves the machine code, not a lucky closed form. */
int64_t jitReferenceSum(int64_t n)
{
    int64_t acc = 0;
    for (int64_t i = 1; i <= n; i++) acc += i;
    return acc;
}

/* Signature of the code we JIT: one int64 in, one int64 out (SysV AMD64). */
typedef int64_t (*JitFn)(int64_t);

int jitDemo(int64_t n)
{
    /* The 18 machine-code bytes from the header comment. */
    static const uint8_t code[] = {
        0x31, 0xC0,             /* xor  %eax,%eax                               */
        0x31, 0xC9,             /* xor  %ecx,%ecx                               */
        0x48, 0x39, 0xF9,       /* cmp  %rdi,%rcx                               */
        0x7D, 0x08,             /* jge  +8  (to ret)                            */
        0x48, 0xFF, 0xC1,       /* inc  %rcx                                    */
        0x48, 0x01, 0xC8,       /* add  %rcx,%rax                               */
        0xEB, 0xF3,             /* jmp  -13 (to cmp)                            */
        0xC3,                   /* ret                                          */
    };
    const size_t len = sizeof(code);

    long pg = sysconf(_SC_PAGESIZE);
    size_t pagesz = (pg > 0) ? (size_t)pg : 4096u;

    /* 1. Anonymous RW page (NOT executable yet). */
    void *mem = mmap(NULL, pagesz, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) { perror("lumen jit: mmap"); return 1; }

    /* 2. Copy the code in while the page is still writable. */
    memcpy(mem, code, len);

    /* 3. Flip to R+X, dropping write. On x86 the instruction cache is kept
     * coherent with the data cache by hardware and mprotect serializes, so no
     * explicit i-cache flush is needed (on ARM/RISC-V you would
     * __builtin___clear_cache(mem, mem+len) here). */
    if (mprotect(mem, pagesz, PROT_READ | PROT_EXEC) != 0) {
        perror("lumen jit: mprotect");
        munmap(mem, pagesz);
        return 1;
    }

    /* 4. Call into it. memcpy the address into a function pointer to sidestep the
     * ISO C "no object<->function pointer cast" rule cleanly. */
    JitFn fn;
    memcpy(&fn, &mem, sizeof(fn));
    int64_t got  = fn(n);
    int64_t want = jitReferenceSum(n);

    munmap(mem, pagesz);

    printf("jit: emitted %zu bytes of x86-64 for sum(1..%lld):\n  ",
           len, (long long)n);
    for (size_t i = 0; i < len; i++) printf("%02x ", code[i]);
    printf("\njit: native result = %lld,  reference = %lld  [%s]\n",
           (long long)got, (long long)want, (got == want) ? "OK" : "MISMATCH");

    return (got == want) ? 0 : 2;
}
