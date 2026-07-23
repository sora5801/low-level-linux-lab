/* ===========================================================================
 * demo.c — a SELF-CONTAINED extraction of the supervisor's pure-logic core.
 * ===========================================================================
 *
 * This file exists purely to be turned into readable teaching assembly. It has
 * NO #includes and declares its own types, so clang can compile it straight to
 * x86-64 Linux assembly on any host:
 *
 *   clang --target=x86_64-pc-linux-gnu -S -O0 ... asm/demo.c -o asm/demo.O0.s
 *   clang --target=x86_64-pc-linux-gnu -S -O1 ... asm/demo.c -o asm/demo.s
 *   clang --target=x86_64-pc-linux-gnu -S -O2 ... asm/demo.c -o asm/demo.O2.s
 *
 * It contains the two decisions a service supervisor makes that are interesting
 * to the CPU rather than to the kernel, lifted verbatim (minus the header) from
 * the real project files order.c / order.h:
 *
 *   1. backoff_delay_ms — how long to wait before the Nth restart of a service
 *      (exponential backoff with an overflow-safe clamp). This is the routine
 *      annotated instruction-by-instruction in demo.annotated.s.
 *
 *   2. toposort — order services so every dependency starts before its dependent
 *      (Kahn's algorithm over a dense adjacency matrix).
 *
 * Because there are no system calls and no libc, the assembly is nothing but the
 * SysV AMD64 ABI, integer math, loads/stores, and branches — exactly what the
 * annotated file dissects.
 * ===========================================================================
 */

/* Our own fixed-width alias: 64-bit under the LP64 model x86-64 Linux uses. */
typedef unsigned long u64;

#define MAX_NODES 64

/* ---------------------------------------------------------------------------
 * backoff_delay_ms — delay = min(base_ms << restart_count, cap_ms), overflow-safe.
 *
 * ABI: restart_count in edi (unsigned, 32-bit), base_ms in rsi, cap_ms in rdx;
 *      result returned in rax. See demo.annotated.s for the full register story.
 * --------------------------------------------------------------------------- */
u64 backoff_delay_ms(unsigned restart_count, u64 base_ms, u64 cap_ms)
{
    u64 delay = base_ms;
    unsigned shift = restart_count;

    /* Clamp the shift so the doublings can never invoke shift UB and settle
     * quickly at the cap. */
    if (shift > 63u)
        shift = 63u;

    /* Double one bit at a time, saturating at the cap. Stopping early keeps the
     * overflow test trivial. */
    for (unsigned k = 0; k < shift; k++) {
        if (delay >= cap_ms)
            return cap_ms;
        delay <<= 1;
    }

    if (delay > cap_ms)
        delay = cap_ms;
    return delay;
}

/* ---------------------------------------------------------------------------
 * toposort — Kahn's algorithm; returns how many of `n` nodes could be ordered.
 * A return value < n means the dependency graph had a cycle. `edges[i*n + j]`
 * != 0 means "node i depends on node j" (j must come first).
 * --------------------------------------------------------------------------- */
int toposort(int n, const unsigned char *edges, int *order)
{
    unsigned char indeg[MAX_NODES];
    unsigned char emitted[MAX_NODES];
    int i, j;
    int out = 0;

    /* Phase 1: in-degree of every node = its number of prerequisites. */
    for (i = 0; i < n; i++) {
        int deg = 0;
        const unsigned char *row = edges + (u64)i * (u64)n;
        for (j = 0; j < n; j++)
            deg += (row[j] != 0);
        indeg[i] = (unsigned char)deg;
        emitted[i] = 0;
    }

    /* Phase 2: emit the lowest ready (in-degree 0) node, relax its dependents. */
    for (i = 0; i < n; i++) {
        int pick = -1;
        for (j = 0; j < n; j++) {
            if (!emitted[j] && indeg[j] == 0) { pick = j; break; }
        }
        if (pick < 0)
            break;                       /* nothing ready but nodes remain: cycle */
        order[out++] = pick;
        emitted[pick] = 1;
        for (j = 0; j < n; j++) {
            const unsigned char *row = edges + (u64)j * (u64)n;
            if (!emitted[j] && row[pick] != 0 && indeg[j] > 0)
                indeg[j]--;
        }
    }
    return out;
}
