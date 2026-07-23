/* ===========================================================================
 * asm/demo.c — the profiler's TWO pure-logic hot loops, standalone.
 * ===========================================================================
 *
 * WHY THIS FILE EXISTS
 * --------------------
 * The real profiler (perf_sampler.c, sigprof_sampler.c, stackmap.c) is genuine
 * Linux userspace C, but every one of those files #includes kernel/system
 * headers — <linux/perf_event.h>, <sys/mman.h>, <ucontext.h>, <dlfcn.h> — none
 * of which exist on the Windows host this repo may live on. clang therefore
 * cannot turn those translation units into assembly here.
 *
 * The *instructive heart* of a sampling profiler, though, is pure integer and
 * pointer arithmetic with zero kernel dependency. Two loops matter most, and
 * both are named explicitly in this project's spec:
 *
 *   1. fp_unwind()  — the FRAME-POINTER STACK-WALK loop. Given the value of a
 *      %rbp register captured at a sample, chase the linked list of saved frame
 *      pointers up the stack, harvesting one return address per frame. This is
 *      exactly what sigprof_sampler.c runs *inside the SIGPROF handler*, and
 *      what the kernel runs to build a PERF_SAMPLE_CALLCHAIN when your code was
 *      compiled with -fno-omit-frame-pointer.
 *
 *   2. the RING-BUFFER INDEX-MASKING helpers — rb_avail / rb_offset /
 *      rb_first_chunk. The perf mmap ring buffer is addressed by two free-
 *      running 64-bit byte counters (data_head, data_tail); turning those
 *      counters into an index into a power-of-two buffer, and splitting a record
 *      that straddles the wrap point into two contiguous copies, is the whole
 *      head/tail protocol in miniature. This is byte-for-byte the arithmetic
 *      perf_sampler.c's drain loop performs.
 *
 * A third routine, fnv1a(), is the exact hash stackmap.c keys its fold table on;
 * it is here so the assembly shows the classic multiply/xor mixing loop.
 *
 * We include NOTHING and declare our own fixed-width types, so this file
 * compiles to assembly for --target=x86_64-pc-linux-gnu on any host. Read the
 * generated asm to SEE: a pointer-chasing load loop (fp_unwind), an AND-with-a-
 * mask used as a modulo (rb_offset), branchless unsigned wrap arithmetic
 * (rb_avail), and a multiply-heavy hash.
 * =========================================================================== */

/* Our own fixed-width types, so this file needs no <stdint.h>. On the x86-64
 * SysV (LP64) target this file is always compiled for, `unsigned long` is 64-bit
 * and the SAME width as a pointer — which is why casting a u64 address to a
 * pointer below is exact and warning-free. */
typedef unsigned long u64;
typedef unsigned int  u32;
typedef unsigned char u8;

/* A cap on unwind depth. Real profilers cap too (the kernel's default is the
 * sysctl kernel.perf_event_max_stack = 127) so a corrupt or hostile stack can
 * never spin us forever. */
#define MAX_FRAMES 64

/* ===========================================================================
 * PART 1 — the frame-pointer stack walk.
 * ===========================================================================
 *
 * THE ABI FACT THAT MAKES THIS WORK
 * ---------------------------------
 * When a function is compiled WITH a frame pointer (-fno-omit-frame-pointer),
 * its prologue is:
 *
 *       push %rbp            ; save caller's frame pointer
 *       mov  %rsp, %rbp      ; establish our own
 *
 * So at any instant, %rbp points at a two-word record on the stack:
 *
 *       [ %rbp + 0 ]  =  the CALLER's saved %rbp   (the next frame up)
 *       [ %rbp + 8 ]  =  the return address into the caller (what we want)
 *
 * That turns the whole call stack into a singly-linked list threaded through
 * the %rbp slots. Walking it is: read the return address, follow the saved-rbp
 * pointer to the next node, repeat. This function is that walk.
 *
 * WHY THE GUARDS ARE NOT OPTIONAL
 * -------------------------------
 * We are dereferencing addresses taken from *runtime stack memory*. If the
 * sampled function omitted its frame pointer, or we caught it mid-prologue, or
 * the stack is simply garbage, `fp` can be anything. Every load through `fp`
 * could fault or, worse, silently read wild memory. So before each load we
 * require fp to be: inside the known stack bounds [lo, hi), 8-byte aligned (the
 * ABI keeps saved rbp 8-aligned), and — critically — STRICTLY GREATER than the
 * previous fp. Stacks grow downward, so walking UP means addresses must
 * increase monotonically; that single check both terminates the loop and makes
 * a cyclic/corrupt chain impossible to loop on forever.
 *
 * ABI: fp in %rdi, lo in %rsi, hi in %rdx, out in %rcx, max in %r8; count in %eax.
 * ------------------------------------------------------------------------- */
int fp_unwind(u64 fp, u64 lo, u64 hi, u64 *out, int max)
{
    int n = 0;                              /* frames harvested so far          */

    /* Loop while the current frame pointer is plausible. Each `&&` below is one
     * of the guards described above; clang lowers them to a short chain of
     * compares + conditional branches you can trace one-to-one in the asm. */
    while (fp >= lo &&                       /* inside the stack, low end        */
           fp <  hi &&                       /* inside the stack, high end       */
           (fp & 7u) == 0u &&                /* 8-byte aligned (ABI invariant)   */
           n < max) {                        /* respect the caller's depth cap   */

        /* Read the two words the frame-pointer record points at. These are the
         * loads the whole exercise is about; in the asm they are two `movq`
         * off %rdi/%rsi-style base registers. The (const u64 *) cast is exact:
         * u64 == pointer width on LP64. */
        u64 saved = *(const u64 *)fp;        /* [fp+0] = caller's saved %rbp     */
        u64 ret   = *(const u64 *)(fp + 8);  /* [fp+8] = return address          */

        out[n++] = ret;                      /* record this frame's return addr  */

        /* Terminate unless the chain keeps climbing. `saved <= fp` catches the
         * outermost frame (saved rbp is 0 or unwritten there) AND any corrupt
         * back-edge that would otherwise loop us. This is the loop's only exit
         * besides the bounds/cap guards. */
        if (saved <= fp)
            break;
        fp = saved;                          /* step to the next frame up        */
    }
    return n;                                /* innermost-first list of returns  */
}

/* ===========================================================================
 * PART 2 — the perf ring-buffer head/tail index math.
 * ===========================================================================
 *
 * THE PROTOCOL IN ONE PARAGRAPH
 * -----------------------------
 * The kernel and userspace share a mmap'd byte ring. The kernel publishes a
 * monotonically increasing byte counter `data_head` (total bytes it has ever
 * written); userspace publishes `data_tail` (total bytes it has ever consumed).
 * Neither is ever masked in the shared page — they free-run forever and wrap at
 * 2^64. The buffer itself is `size` bytes where `size` is a power of two, so the
 * physical slot for any counter value is `counter & (size - 1)`. Everything
 * below is that idea, split into three tiny, individually-testable pieces.
 * =========================================================================== */

/* rb_avail — how many unconsumed bytes are in the ring right now.
 *
 * It is JUST `head - tail`, computed in unsigned 64-bit. The subtlety worth the
 * comment: because both counters free-run and wrap at 2^64, a naive signed view
 * would go negative after a wrap. Unsigned modular subtraction gives the correct
 * distance regardless of wrap, as long as the true backlog never exceeds `size`
 * (which the protocol guarantees — the kernel stops writing when the ring is
 * full). No branch, no compare: one `sub`. */
u64 rb_avail(u64 head, u64 tail)
{
    return head - tail;
}

/* rb_offset — turn a free-running counter into a physical index into the ring.
 *
 * `size` is a power of two, so `counter % size` is exactly `counter & (size-1)`.
 * The mask form is why perf mandates a power-of-two data area: modulo becomes a
 * single-cycle AND with no division. In the asm this is one `and`. */
u64 rb_offset(u64 counter, u64 size)
{
    return counter & (size - 1);
}

/* rb_first_chunk — length of the first contiguous copy for a record of `need`
 * bytes that begins at physical offset `off` in a `size`-byte ring.
 *
 * A record can straddle the end of the buffer: bytes [off, size) sit at the top,
 * the remainder wraps to [0, ...). This returns how many bytes are contiguous
 * from `off` before the wrap — i.e. `min(need, size - off)`. The drain loop
 * copies that many, then copies `need - first` from offset 0. Written as a
 * branch here so the two cases are legible; the optimizer may turn it into a
 * branchless `cmov`, which is itself worth spotting in demo.O2.s. */
u64 rb_first_chunk(u64 off, u64 need, u64 size)
{
    u64 to_end = size - off;                 /* bytes from off up to the wrap    */
    return (need <= to_end) ? need : to_end; /* min(need, to_end)                */
}

/* ===========================================================================
 * PART 3 — the fold hash (mirrors stackmap.c).
 * ===========================================================================
 * FNV-1a: for each byte, XOR then multiply by the 64-bit FNV prime. It is the
 * hash stackmap.c uses to bucket a collapsed-stack string like
 * "main;wl_compute;wl_hash". Included so the asm shows the tight xor/imul mix
 * loop — note how -O2 may unroll it. */
u64 fnv1a(const u8 *p, u64 n)
{
    u64 h = 14695981039346656037UL;          /* FNV offset basis (64-bit)        */
    for (u64 i = 0; i < n; i++) {
        h ^= (u64)p[i];                      /* mix the byte in first (the "a")  */
        h *= 1099511628211UL;                /* then multiply by the FNV prime   */
    }
    return h;
}

/* ===========================================================================
 * demo_selftest — prove the three cores are correct with no kernel in sight.
 * ===========================================================================
 * Returns 0 on success or a small nonzero code naming the first failing check
 * (inspect with `echo $?` or a debugger). This is scaffolding, not core logic,
 * so it is kept OUT of the annotated walkthrough. Because every input here is a
 * compile-time constant, -O2 folds most of it away — watch demo.O2.s collapse
 * the whole function toward `xor %eax,%eax`.
 * ------------------------------------------------------------------------- */
int demo_selftest(void)
{
    /* --- Build a synthetic 3-deep frame chain in a real array, then walk it.
     * Each even slot is a "saved rbp" pointer to the next frame's record; each
     * odd slot is that frame's "return address". The last saved-rbp is 0, which
     * trips the `saved <= fp` terminator. Using addresses of a live array means
     * fp_unwind actually dereferences valid memory here. */
    u64 stk[6];
    u64 base = (u64)(const u64 *)stk;        /* address of stk[0]                */
    stk[0] = base + 2 * 8;  stk[1] = 0xAAAA; /* frame 0 -> next at stk[2]        */
    stk[2] = base + 4 * 8;  stk[3] = 0xBBBB; /* frame 1 -> next at stk[4]        */
    stk[4] = 0;             stk[5] = 0xCCCC; /* frame 2 -> saved rbp 0 = stop    */

    u64 got[MAX_FRAMES];
    int n = fp_unwind(base, base, base + sizeof stk, got, MAX_FRAMES);
    if (n != 3)          return 1;           /* three return addresses expected  */
    if (got[0] != 0xAAAA) return 2;          /* innermost first                  */
    if (got[1] != 0xBBBB) return 3;
    if (got[2] != 0xCCCC) return 4;

    /* An unaligned fp must be rejected immediately (zero frames). */
    if (fp_unwind(base + 1, base, base + sizeof stk, got, MAX_FRAMES) != 0) return 5;

    /* --- Ring-buffer math. size = 4096 (a power of two), mask = 0xFFF. */
    if (rb_avail(1000, 40) != 960)            return 6;   /* simple backlog       */
    if (rb_avail(5, 0xFFFFFFFFFFFFFFF0UL) != 21) return 7;/* across the 2^64 wrap */
    if (rb_offset(4096 + 7, 4096) != 7)       return 8;   /* mask == modulo       */
    if (rb_offset(0xDEAD000 + 0x123, 4096) != 0x123) return 9;

    /* A record wholly before the wrap copies in one chunk; one that straddles
     * the end splits. off=4090, need=12, size=4096 -> first chunk is 6 bytes. */
    if (rb_first_chunk(100, 12, 4096) != 12)  return 10;  /* fits, no split       */
    if (rb_first_chunk(4090, 12, 4096) != 6)  return 11;  /* 4096-4090 = 6        */

    /* --- Hash: FNV-1a of "ab" is a known constant; check the mixing works. */
    if (fnv1a((const u8 *)"ab", 2) != 0x089C4407B545986AUL) return 12;

    return 0;                                 /* all cores verified               */
}
