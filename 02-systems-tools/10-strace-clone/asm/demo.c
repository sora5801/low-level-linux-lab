/* ===========================================================================
 * asm/demo.c — the two pure-logic cores of the strace clone, standing alone.
 * ===========================================================================
 *
 * WHY THIS FILE EXISTS
 * --------------------
 * The real tracer (ministrace.c, decode.c) cannot be compiled to assembly on a
 * non-Linux host: it needs <sys/ptrace.h> and friends. So we extract its two
 * most instructive PURE routines — the ones that are all arithmetic and no
 * syscalls — into this self-contained file, declaring our own types so it pulls
 * in NOTHING. `clang --target=x86_64-pc-linux-gnu -S` then turns it into the
 * teaching assembly in this directory, and asm/demo.annotated.s explains it.
 *
 * The two routines are the heart of "decode a raw trace into text":
 *
 *   syscall_args()  — the register->argument mapping. When the kernel stops a
 *                     tracee at a syscall, the arguments sit in specific
 *                     registers defined by the System V AMD64 *syscall* ABI:
 *                     rdi, rsi, rdx, r10, r8, r9 (note r10, NOT rcx — the
 *                     `syscall` instruction overwrites rcx with the return RIP,
 *                     so the kernel takes arg4 from r10 instead). This function
 *                     is that mapping, and the -O1 assembly is a clean row of
 *                     loads you can read straight off.
 *
 *   decode_flags()  — the flag-decode table walk. open()/mmap() arguments are
 *                     bitmasks; we AND each candidate bit against the value,
 *                     emit its name when present, clear it, and print whatever
 *                     bits are left over as hex. This loop + the "consume the
 *                     bits" AND-NOT is the single most reused idea in the
 *                     decoder, and it is what the annotated assembly dwells on.
 *
 * These mirror ministrace.c::arg_regs and decode.c::decode_flags byte-for-byte
 * in spirit, so the assembly you read here is the assembly the real tool runs.
 * ===========================================================================
 */

/* Our own fixed-width type: on the LP64 model Linux uses for x86-64, `unsigned
 * long` is 64 bits — wide enough for a register, a pointer, or a syscall arg. */
typedef unsigned long u64;

/* The subset of `struct user_regs_struct` the tracer actually reads. Laid out
 * in the order the kernel stores them is not required here — we name fields, and
 * the compiler addresses them by offset — but we keep the syscall-arg registers
 * grouped so the mapping below reads naturally. */
struct regs {
    u64 orig_rax;   /* syscall number, latched by the kernel at entry          */
    u64 rdi, rsi, rdx, r10, r8, r9;   /* the six syscall-argument registers     */
    u64 rax;        /* return value, valid at the exit stop                    */
};

/* ---------------------------------------------------------------------------
 * syscall_args — copy the six argument registers into args[0..5] in ABI order
 * and return the syscall number. THIS is the register-to-argument mapping.
 *
 * The whole point is the r10 line: a user program calls a libc wrapper with
 * arg4 in rcx (the normal SysV *function* ABI), but the wrapper moves it to r10
 * before executing `syscall`, because the instruction clobbers rcx. So when we
 * inspect a stopped tracee, arg4 lives in r10. Get this wrong and every 4-plus-
 * argument syscall (mmap, pread64, futex, ...) decodes garbage.
 * --------------------------------------------------------------------------- */
u64 syscall_args(const struct regs *r, u64 args[6])
{
    args[0] = r->rdi;   /* arg1 */
    args[1] = r->rsi;   /* arg2 */
    args[2] = r->rdx;   /* arg3 */
    args[3] = r->r10;   /* arg4 — r10, the ABI substitution for rcx            */
    args[4] = r->r8;    /* arg5 */
    args[5] = r->r9;    /* arg6 */
    return r->orig_rax; /* the number that selects which syscall this is       */
}

/* A single decodable bit (or bit-group) and the symbol it prints as. */
struct flag { u64 mask; const char *name; };

/* ---------------------------------------------------------------------------
 * put_hex — append "0x…" of `v` to out[pos..cap), returning the new position.
 * A tiny freestanding hex formatter so decode_flags needs no libc. We build the
 * digits least-significant-first into a scratch buffer, then reverse them out.
 * --------------------------------------------------------------------------- */
static int put_hex(char *out, int cap, int pos, u64 v)
{
    static const char digits[] = "0123456789abcdef";
    if (pos < cap - 1) out[pos++] = '0';
    if (pos < cap - 1) out[pos++] = 'x';
    if (v == 0) {                       /* special-case zero: "0x0"            */
        if (pos < cap - 1) out[pos++] = '0';
        return pos;
    }
    char tmp[16];
    int n = 0;
    while (v != 0 && n < 16) {          /* extract nibbles low-to-high         */
        tmp[n++] = digits[v & 0xf];
        v >>= 4;
    }
    while (n > 0 && pos < cap - 1)      /* emit them high-to-low               */
        out[pos++] = tmp[--n];
    return pos;
}

/* ---------------------------------------------------------------------------
 * decode_flags — the table walk. For each row whose bits are ALL set in `value`,
 * append its name (|-separated) and CLEAR those bits from `value`. Print any
 * remaining bits as hex so a flag we lack a name for is never silently dropped.
 * Returns the number of bytes written (NUL-terminated within cap).
 *
 * Two subtleties the assembly makes visible:
 *   - `(value & mask) == mask` tests "all of this group's bits are present,"
 *     which correctly handles multi-bit flags, not just single bits.
 *   - `value &= ~mask` is the "consume" step; without it a later row sharing a
 *     bit could match again, and the leftover-hex would be wrong.
 * --------------------------------------------------------------------------- */
int decode_flags(u64 value, const struct flag *table, int n, char *out, int cap)
{
    int pos = 0;          /* write cursor                                       */
    int wrote_any = 0;    /* emitted at least one name? (controls the '|')      */

    for (int i = 0; i < n && pos < cap - 1; i++) {
        if (table[i].mask != 0 && (value & table[i].mask) == table[i].mask) {
            if (wrote_any && pos < cap - 1)
                out[pos++] = '|';
            const char *nm = table[i].name;
            int k = 0;
            while (nm[k] != '\0' && pos < cap - 1)
                out[pos++] = nm[k++];
            value &= ~table[i].mask;    /* consume the bits we just named        */
            wrote_any = 1;
        }
    }

    if (value != 0) {                   /* unnamed leftover bits -> show as hex  */
        if (wrote_any && pos < cap - 1)
            out[pos++] = '|';
        pos = put_hex(out, cap, pos, value);
    } else if (!wrote_any && pos < cap - 1) {
        out[pos++] = '0';               /* nothing set at all -> literal "0"     */
    }

    if (pos < cap)
        out[pos] = '\0';
    else if (cap > 0)
        out[cap - 1] = '\0';
    return pos;
}
