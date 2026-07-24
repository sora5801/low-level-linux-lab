/* ===========================================================================
 * asm/demo.c — the seccomp-BPF ALLOWLIST builder + the classic-BPF matcher,
 *              extracted as SELF-CONTAINED pure logic (no system headers).
 * ===========================================================================
 *
 * WHY THIS FILE EXISTS
 * --------------------
 * The full sandbox (../seccomp_filter.c, ../landlock_fs.c, ../supervisor.c)
 * needs Linux kernel headers and a live kernel, so it cannot compile on an
 * arbitrary host and its "interesting" logic is tangled up with syscalls. This
 * file lifts out the one genuinely instructive, host-portable core:
 *
 *     1. build_allowlist() — emit a classic-BPF program that ALLOWs a set of
 *        syscall numbers and KILLs everything else (fail-closed).
 *     2. seccomp_run()     — the classic-BPF virtual machine the KERNEL runs on
 *        each syscall: a one-register accumulator machine that loads a word from
 *        `seccomp_data`, compares it, and returns an action. THIS is "the
 *        classic-BPF check of the syscall nr against the allowlist."
 *
 * Reproducing (from the project dir) — this is a committed teaching artifact:
 *     clang --target=x86_64-pc-linux-gnu -S -O0 -fno-asynchronous-unwind-tables \
 *           -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
 *     clang --target=x86_64-pc-linux-gnu -S -O1 -fno-asynchronous-unwind-tables \
 *           -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
 *     clang --target=x86_64-pc-linux-gnu -S -O2 -fno-asynchronous-unwind-tables \
 *           -fno-jump-tables asm/demo.c -o asm/demo.O2.s
 * Then asm/demo.annotated.s is hand-written from asm/demo.s (-O1).
 *
 * Everything below uses only its own fixed-width types, so the emitted assembly
 * has no libc calls to distract from the BPF machine itself.
 * ===========================================================================
 */

/* ---- our own fixed-width integer types (no <stdint.h>) -------------------- */
typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

/* ---------------------------------------------------------------------------
 * struct bpf_insn — byte-for-byte identical to the kernel's `struct sock_filter`
 * (8 bytes). A classic-BPF program is just an array of these.
 *   code : the opcode = an instruction CLASS OR'd with mode/operation bits.
 *   jt   : "jump if true"  — how many instructions to SKIP when a test passes.
 *   jf   : "jump if false" — how many to skip when it fails.
 *   k    : the generic 32-bit operand (a load offset, a compare immediate, or a
 *          return value, depending on the class).
 * --------------------------------------------------------------------------- */
struct bpf_insn {
    u16 code;
    u8  jt;
    u8  jf;
    u32 k;
};

/* ---- classic-BPF opcode bits (the subset seccomp filters use) ------------- *
 * An opcode is CLASS | (mode-or-op) | size. We spell out the exact hex the
 * kernel's <linux/bpf_common.h> defines so the numbers are not magic.          */
#define BPF_LD   0x00u   /* class: load into the accumulator A                 */
#define BPF_JMP  0x05u   /* class: conditional/unconditional jump              */
#define BPF_RET  0x06u   /* class: return an action and halt                   */
#define BPF_W    0x00u   /* size:  word = 32 bits                              */
#define BPF_ABS  0x20u   /* mode:  ABSolute offset into the data buffer        */
#define BPF_JEQ  0x10u   /* op:    jump if A == k                              */
#define BPF_JGE  0x30u   /* op:    jump if A >= k                              */
#define BPF_K    0x00u   /* source: the immediate k (vs. the X register)       */

/* ---- SECCOMP_RET_* action words (what a BPF_RET returns) ------------------ *
 * The high bits select behaviour; when filters are stacked the kernel keeps the
 * NUMERICALLY SMALLEST result, so KILL_PROCESS (0x8000_0000, top bit set, i.e.
 * the "most negative") always wins — least privilege by construction.          */
#define RET_KILL_PROCESS 0x80000000u  /* terminate the process with SIGSYS      */
#define RET_TRACE        0x7ff00000u  /* defer to a ptrace supervisor           */
#define RET_ERRNO        0x00050000u  /* fail the call, errno in the low 16 bits*/
#define RET_ALLOW        0x7fff0000u  /* run the syscall unmodified             */

/* ---- seccomp_data layout: byte offsets of the fields BPF loads ------------ *
 * struct seccomp_data { s32 nr; u32 arch; u64 ip; u64 args[6]; }               */
#define OFF_NR    0u    /* the syscall number                                  */
#define OFF_ARCH  4u    /* AUDIT_ARCH_* tag of the calling ABI                 */

/* The x86-64 arch tag, and the bit that marks an x32-ABI syscall number. A
 * filter MUST pin the arch and reject x32, or a caller can present a syscall
 * number that means something different under another ABI. */
#define AUDIT_ARCH_X86_64 0xC000003Eu
#define X32_SYSCALL_BIT   0x40000000u

/* ===========================================================================
 * bpf_load_u32 — read a 32-bit big-value from the data buffer, LITTLE-ENDIAN.
 *
 * Classic BPF indexes `seccomp_data` as a flat byte array; a BPF_LD|W|ABS at
 * offset `off` loads the 32-bit word there. We reconstruct it byte-by-byte so
 * the routine is endianness-explicit and self-contained (x86-64 is LE, so byte
 * 0 is the least-significant). This is a small, clean function to read in asm.
 * =========================================================================== */
u32 bpf_load_u32(const u8 *data, u32 off)
{
    return  (u32)data[off + 0]        |
           ((u32)data[off + 1] <<  8) |
           ((u32)data[off + 2] << 16) |
           ((u32)data[off + 3] << 24);
}

/* store_u32 — the inverse, used only to build test inputs below. */
static void store_u32(u8 *data, u32 off, u32 v)
{
    data[off + 0] = (u8)(v      );
    data[off + 1] = (u8)(v >>  8);
    data[off + 2] = (u8)(v >> 16);
    data[off + 3] = (u8)(v >> 24);
}

/* ===========================================================================
 * build_allowlist — emit the classic-BPF program for an allowlist of nrs.
 *
 * Layout (mirrors ../seccomp_filter.c exactly):
 *     A = arch;   if A != X86_64 -> KILL           (pin the ABI)
 *     A = nr;     if A >= X32_BIT -> KILL           (reject the x32 alias space)
 *     for each allowed nr:  if A == nr -> RET ALLOW
 *     RET KILL                                      (fail-closed default)
 *
 * Every jump is a short local hop (jt/jf of 0 or 1), so the program is correct
 * for any allowlist length with no fragile offset arithmetic. Returns the
 * instruction count, or -1 if it would not fit in `cap`.
 * =========================================================================== */
int build_allowlist(struct bpf_insn *prog, int cap, const u32 *allow, int n)
{
    int k = 0;

    /* Append one instruction, bounds-checking against cap as we go. */
    #define PUT(code_, jt_, jf_, k_) do {                 \
        if (k >= cap) return -1;                          \
        prog[k].code = (u16)(code_);                      \
        prog[k].jt   = (u8)(jt_);                         \
        prog[k].jf   = (u8)(jf_);                         \
        prog[k].k    = (u32)(k_);                         \
        k++;                                              \
    } while (0)

    /* --- arch guard --- */
    PUT(BPF_LD | BPF_W | BPF_ABS, 0, 0, OFF_ARCH);            /* A = data[arch]  */
    PUT(BPF_JMP | BPF_JEQ | BPF_K, 1, 0, AUDIT_ARCH_X86_64);  /* ==x86_64: skip  */
    PUT(BPF_RET | BPF_K,           0, 0, RET_KILL_PROCESS);   /*   else KILL     */

    /* --- load the syscall number --- */
    PUT(BPF_LD | BPF_W | BPF_ABS,  0, 0, OFF_NR);             /* A = data[nr]    */

    /* --- x32 guard: nr with bit 30 set is the x32 alias -> KILL --- */
    PUT(BPF_JMP | BPF_JGE | BPF_K, 0, 1, X32_SYSCALL_BIT);    /* >=bit: next KILL*/
    PUT(BPF_RET | BPF_K,           0, 0, RET_KILL_PROCESS);

    /* --- allowlist rows: if (nr == allow[i]) return ALLOW; --- */
    for (int i = 0; i < n; i++) {
        /* jt=0: on match fall through to the RET_ALLOW below.
         * jf=1: on mismatch skip that RET and test the next number. */
        PUT(BPF_JMP | BPF_JEQ | BPF_K, 0, 1, allow[i]);
        PUT(BPF_RET | BPF_K,           0, 0, RET_ALLOW);
    }

    /* --- default: nothing matched, deny --- */
    PUT(BPF_RET | BPF_K,           0, 0, RET_KILL_PROCESS);

    #undef PUT
    return k;
}

/* ===========================================================================
 * seccomp_run — the classic-BPF VIRTUAL MACHINE (what the kernel runs).
 *
 * This is the heart of the demo and the function the annotated assembly walks
 * through. It is an ACCUMULATOR machine: a single 32-bit register A, a program
 * counter, and a read-only data buffer. It supports exactly the four opcode
 * forms a seccomp allowlist uses. `datalen` lets every load be bounds-checked so
 * a malformed program can never read out of bounds — and any anomaly FAILS
 * CLOSED to KILL, never to ALLOW.
 *
 * jt/jf semantics: they count instructions to SKIP *after* the current one, so
 * "pc += skip" here plus the loop's "pc++" lands on pc + skip + 1 — exactly the
 * classic-BPF rule.
 * =========================================================================== */
u32 seccomp_run(const struct bpf_insn *prog, int len, const u8 *data, int datalen)
{
    u32 A = 0;            /* the one and only accumulator register              */
    int pc;

    for (pc = 0; pc < len; pc++) {
        const struct bpf_insn *ins = &prog[pc];
        switch (ins->code) {

        case BPF_LD | BPF_W | BPF_ABS:
            /* Bounds-check the 4-byte load; OOB is a broken filter -> KILL. */
            if (ins->k + 4u > (u32)datalen)
                return RET_KILL_PROCESS;
            A = bpf_load_u32(data, ins->k);
            break;

        case BPF_JMP | BPF_JEQ | BPF_K:
            /* if (A == k) skip jt instructions, else skip jf. */
            pc += (A == ins->k) ? ins->jt : ins->jf;
            break;

        case BPF_JMP | BPF_JGE | BPF_K:
            pc += (A >= ins->k) ? ins->jt : ins->jf;
            break;

        case BPF_RET | BPF_K:
            /* Halt: this action word is the verdict for the syscall. */
            return ins->k;

        default:
            /* Unknown opcode: a filter we do not understand is not one we can
             * trust, so deny. */
            return RET_KILL_PROCESS;
        }
    }
    /* Ran off the end without a RET — malformed. Fail closed. */
    return RET_KILL_PROCESS;
}

/* Build a seccomp_data image (64 bytes is enough for nr+arch) for testing. */
static void make_data(u8 *buf, u32 nr, u32 arch)
{
    for (int i = 0; i < 64; i++) buf[i] = 0;
    store_u32(buf, OFF_NR,   nr);
    store_u32(buf, OFF_ARCH, arch);
}

/* ===========================================================================
 * demo_selftest — build a filter, then run it against several syscall images
 * and confirm the verdicts. Pure logic: returns 0 on success, or the number of
 * the first failing case (so a nonzero return names what broke). A harness on
 * Linux can call this; here it mainly gives the optimizer real code to chew on.
 * =========================================================================== */
int demo_selftest(void)
{
    struct bpf_insn prog[32];
    /* allow read(0), write(1), exit_group(231) — deny all else. */
    const u32 allow[3] = { 0u, 1u, 231u };
    int len = build_allowlist(prog, 32, allow, 3);
    if (len < 0) return 1;

    u8 data[64];

    /* (1) an allowed syscall on the right arch -> ALLOW */
    make_data(data, 1u, AUDIT_ARCH_X86_64);
    if (seccomp_run(prog, len, data, 64) != RET_ALLOW) return 2;

    /* (2) socket(41) is not listed -> the default KILL */
    make_data(data, 41u, AUDIT_ARCH_X86_64);
    if (seccomp_run(prog, len, data, 64) != RET_KILL_PROCESS) return 3;

    /* (3) right nr but WRONG arch -> KILL (the arch guard fires first) */
    make_data(data, 1u, AUDIT_ARCH_X86_64 ^ 1u);
    if (seccomp_run(prog, len, data, 64) != RET_KILL_PROCESS) return 4;

    /* (4) the x32 alias of write (1 | bit30) -> KILL (the x32 guard) */
    make_data(data, 1u | X32_SYSCALL_BIT, AUDIT_ARCH_X86_64);
    if (seccomp_run(prog, len, data, 64) != RET_KILL_PROCESS) return 5;

    return 0;   /* all verdicts as expected */
}
