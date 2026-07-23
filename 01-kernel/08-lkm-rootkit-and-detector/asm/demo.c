/* ===========================================================================
 * demo.c — the detector's PURE-LOGIC core, extracted for standalone assembly.
 * ===========================================================================
 *
 * WHY THIS FILE EXISTS
 * --------------------
 * The real project (../detector.c) is a Linux kernel module. Kernel C cannot be
 * compiled to assembly on its own: it needs the linux/ headers, the kernel config,
 * and the Kbuild machinery, none of which exist on a normal host. So there is no
 * honest way to `clang -S detector.c`.
 *
 * The lab's rule is that EVERY C project still ships real, hand-annotated
 * assembly. To honor that we lift out the one piece of the detector that is
 * pure arithmetic — the routine that *fingerprints and diffs a memory region* —
 * into this self-contained file. It declares its own integer types, pulls in no
 * headers, and touches no kernel APIs, so `clang -S` produces genuine, readable
 * x86-64 assembly (see asm/demo.s and asm/demo.annotated.s).
 *
 * WHAT THE DETECTOR USES THIS FOR
 * -------------------------------
 * The blue-team detector's whole strategy is "diff the live kernel against a
 * known-good baseline." Concretely, at module load it snapshots the syscall
 * table (an array of NR_syscalls function pointers) and hashes it. Later — on
 * demand — it re-hashes the live table and compares. If a rootkit has swapped
 * even one entry (the classic sys_call_table hook), the hash changes, and
 * region_first_diff() points at exactly which syscall number was tampered with.
 *
 * These three functions are that logic, verbatim in spirit:
 *   - fnv1a64()          : a fast, dependency-free 64-bit hash of a byte region.
 *   - region_first_diff(): find the first byte where baseline and live differ.
 *   - table_fingerprint(): hash an array of pointer-sized words (the table).
 *
 * They are the interesting part to read as assembly: a multiply-accumulate hash
 * loop (watch the optimizer strength-reduce and unroll it) and a compare loop
 * with an early-exit branch (watch how the ABI return value is formed).
 * ===========================================================================
 */

/* We are freestanding here: no <stdint.h>. Spell out the widths we rely on.
 * On the LP64 model Linux uses for x86-64, `long` is 64 bits and holds a
 * pointer or a size without truncation — that is why the kernel-side code can
 * treat a syscall-table entry as one `u64`. */
typedef unsigned long  u64;   /* 64-bit unsigned: hash state, table entries    */
typedef unsigned char  u8;    /* a single byte: the unit we hash and compare   */
typedef unsigned long  usize; /* a byte count / array length (never negative)  */

/* ---------------------------------------------------------------------------
 * fnv1a64 — FNV-1a, a non-cryptographic 64-bit hash.
 *
 * We deliberately do NOT use a crypto hash here. The detector runs in kernel
 * context on a hot-ish path and only needs to notice *accidental-looking or
 * malicious* single-word changes to a small table; FNV-1a is a handful of
 * instructions per byte with no tables and no allocations, which is exactly
 * what you want when you cannot afford a heavyweight primitive and cannot fail.
 *
 * The algorithm, per byte b:
 *     hash = (hash XOR b) * FNV_PRIME
 * seeded with the 64-bit FNV offset basis. The XOR-then-multiply order is the
 * "1a" variant, which has slightly better avalanche than the original "1".
 *
 * INVARIANT the caller must uphold: [data, data+len) is entirely readable. In
 * the kernel we only ever call this on memory we already own (our baseline copy)
 * or on the syscall table, whose bounds we validated first. Hand it a bad
 * pointer and you get an oops — the same rule as any raw memory read.
 * --------------------------------------------------------------------------- */
u64 fnv1a64(const u8 *data, usize len)
{
    /* The standard 64-bit FNV offset basis. 0xcbf29ce484222325 is not arbitrary:
     * it is FNV-1 applied to a specific signature string, chosen once by the FNV
     * authors. We write it in hex so it is unmistakably that published constant. */
    u64 hash = 0xcbf29ce484222325u;
    usize i;

    for (i = 0; i < len; i++) {
        hash ^= (u64)data[i];   /* mix the next byte into the low bits         */
        hash *= 1099511628211u; /* FNV prime 0x100000001b3; the multiply is
                                 * what diffuses that byte across all 64 bits.  */
    }
    return hash;
}

/* ---------------------------------------------------------------------------
 * region_first_diff — index of the first differing byte between two regions.
 *
 * Returns the byte offset where a[] and b[] first disagree, or -1 if the two
 * `len`-byte regions are identical. The detector calls this AFTER a hash
 * mismatch to turn "something changed" into "syscall number N changed": with
 * 8-byte entries, a differing byte at offset K means syscall K/8 was hooked.
 *
 * We return a signed `long` precisely so that -1 can mean "equal" — a real
 * offset is always >= 0 and a table is far smaller than LONG_MAX, so there is
 * no ambiguity. Watch in the assembly how the two return paths (`i` vs -1) are
 * materialized in rax.
 * --------------------------------------------------------------------------- */
long region_first_diff(const u8 *a, const u8 *b, usize len)
{
    usize i;

    for (i = 0; i < len; i++) {
        if (a[i] != b[i])       /* early exit on the FIRST mismatch...          */
            return (long)i;     /* ...so cost is O(offset), not O(len)          */
    }
    return -1;                  /* fell through: every byte matched             */
}

/* ---------------------------------------------------------------------------
 * table_fingerprint — hash an array of pointer-sized words.
 *
 * The syscall table is `void *sys_call_table[NR_syscalls]`. Rather than cast it
 * to bytes at the call site, the detector hands the array and a COUNT of words
 * to this helper, which re-expresses it as bytes and reuses fnv1a64. Keeping the
 * "count of entries" units here (not raw bytes) is what lets the caller say
 * "there are 450 syscalls" without thinking about sizeof(void*).
 *
 * Note the single interesting subtlety for the assembly reader: `count *
 * sizeof(u64)` is a shift-left-by-3, and passing &entries[0] as a byte pointer
 * is a no-op reinterpretation — the optimizer will show both.
 * --------------------------------------------------------------------------- */
u64 table_fingerprint(const u64 *entries, usize count)
{
    /* sizeof(u64) is 8 on LP64; count*8 is the region length in bytes. The
     * cast to (const u8 *) is a pure reinterpretation — same address, byte view.
     * We own or have bounds-checked `entries` before calling (see detector.c). */
    return fnv1a64((const u8 *)entries, count * sizeof(u64));
}
