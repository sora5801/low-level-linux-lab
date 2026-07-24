/* ===========================================================================
 * demo.c — the debugger's two purest routines, extracted for annotated asm.
 * ===========================================================================
 *
 * This file is DELIBERATELY self-contained: no #includes, its own typedefs, so
 * `clang --target=x86_64-pc-linux-gnu -S` produces clean Linux/SysV assembly with
 * nothing from libc in the way. The real debugger sources (../breakpoint.c,
 * ../debuginfo.c) cannot compile to asm standalone — they pull in <sys/ptrace.h>,
 * <elf.h>, <sys/user.h>, etc. — so we lift out the parts that are 100% register-
 * and-pointer logic, which are also the most instructive to read as machine code:
 *
 *   1. int3 byte-splice   — the read-modify-write that plants and lifts a 0xCC
 *                           software breakpoint (mirrors bp_enable/bp_disable).
 *   2. RIP rewind         — the one-byte correction after an int3 fires.
 *   3. addr_to_line       — the sorted-table binary search that maps a PC to a
 *                           source row (mirrors di_addr_to_line): "upper_bound,
 *                           then step back one". Watch it become branchless-ish
 *                           index math with a `cmov` in the -O1/-O2 output.
 *
 * These mirror the same-named logic in the real sources exactly, minus syscalls.
 * ===========================================================================
 */

/* Our own fixed-width types (LP64: long and pointers are 64-bit on x86-64). */
typedef unsigned long  u64;
typedef unsigned int   u32;
typedef unsigned char  u8;

#define INT3          0xCCUL     /* the one-byte `int3` breakpoint opcode          */
#define LOW_BYTE_MASK 0xFFUL     /* selects the single byte we overwrite           */

/* ---------------------------------------------------------------------------
 * 1a. patch_int3 — splice 0xCC into the low byte of a text word.
 *
 * PTRACE_POKETEXT writes 8 bytes at a time, but a software breakpoint only wants
 * to change ONE byte (the first opcode byte of the target instruction). So we
 * read the surrounding 8-byte word, clear its low byte with ~0xFF, and OR in
 * 0xCC. The upper 7 bytes are preserved verbatim.
 *
 *   patched = (word & ~0xFF) | 0xCC
 * ------------------------------------------------------------------------- */
u64 patch_int3(u64 word)
{
    return (word & ~LOW_BYTE_MASK) | INT3;
}

/* ---------------------------------------------------------------------------
 * 1b. saved_byte_of — extract the original opcode byte before we clobber it.
 * This is the byte we must stash so the breakpoint can later be made invisible.
 * ------------------------------------------------------------------------- */
u8 saved_byte_of(u64 word)
{
    return (u8)(word & LOW_BYTE_MASK);
}

/* ---------------------------------------------------------------------------
 * 1c. unpatch_byte — restore a saved opcode byte into a (possibly re-read) word.
 *
 *   restored = (word & ~0xFF) | saved
 *
 * Symmetric with patch_int3: same mask, but we OR the SAVED byte back instead of
 * 0xCC. This is how a breakpoint is temporarily lifted to step over it.
 * ------------------------------------------------------------------------- */
u64 unpatch_byte(u64 word, u8 saved)
{
    return (word & ~LOW_BYTE_MASK) | (u64)saved;
}

/* ---------------------------------------------------------------------------
 * 2. rewind_rip — after a one-byte int3 executes, RIP points at addr+1.
 * To make the breakpoint transparent we set RIP back to addr, i.e. RIP - 1.
 * Trivial, but it is the correction that makes the whole scheme work.
 * ------------------------------------------------------------------------- */
u64 rewind_rip(u64 rip)
{
    return rip - 1;
}

/* ---------------------------------------------------------------------------
 * 3. addr_to_line — map a program counter to a source-line row.
 *
 * `rows` is sorted ascending by `addr`. Each row owns the half-open address
 * range [rows[i].addr, rows[i+1].addr). We binary-search for the FIRST row whose
 * addr is strictly greater than `pc` (an upper bound), then step back one: that
 * predecessor is the row covering `pc`. A row flagged `end` is an end_sequence
 * sentinel (a gap between ranges), so a hit on it means "no line".
 *
 * This is exactly di_addr_to_line() in ../debuginfo.c, reduced to pure logic.
 * Returns the row index, or -1 if pc is before the table or lands in a gap.
 * ------------------------------------------------------------------------- */
typedef struct {
    u64 addr;   /* link-time start address of this row's range */
    u32 file;   /* file-name index                             */
    u32 line;   /* source line number                          */
    int end;    /* 1 = end_sequence sentinel (a gap)           */
} line_row;

int addr_to_line(const line_row *rows, int n, u64 pc)
{
    int lo = 0, hi = n;                 /* search the half-open interval [lo, hi)   */
    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;   /* (hi-lo)/2 avoids lo+hi overflow          */
        if (rows[mid].addr <= pc)
            lo = mid + 1;               /* row starts at/below pc: answer is higher */
        else
            hi = mid;                   /* row starts above pc: search lower half   */
    }
    /* `lo` is now the count of rows with addr <= pc; the covering row is lo-1. */
    if (lo == 0)
        return -1;                      /* pc is below the first row's address      */
    if (rows[lo - 1].end)
        return -1;                      /* pc landed in an end_sequence gap         */
    return lo - 1;
}
