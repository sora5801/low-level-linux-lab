/* ===========================================================================
 * demo.c — the seccomp decision core, extracted as pure, self-contained logic.
 * ===========================================================================
 *
 * The real runtime (seccomp.c) BUILDS a classic-BPF program and hands it to the
 * kernel; the KERNEL is what actually interprets it on every syscall. This file
 * extracts that interpreter — the "bytecode dispatch core" — so we can compile
 * it to assembly and read exactly how a filter decision becomes machine code.
 *
 * It is deliberately freestanding: no #includes, no libc, no system headers. It
 * declares its own fixed-width types and its own copies of the handful of BPF
 * opcodes seccomp uses. That is what lets `clang --target=x86_64-pc-linux-gnu`
 * turn it into Linux assembly on ANY host (see the Makefile `asm` target).
 *
 * Classic BPF, in one paragraph: a program is an array of instructions, each
 * { code, jt, jf, k }. There is ONE 32-bit accumulator `A`. Instructions either
 * load a word from the input into A, compare A against the immediate `k` and
 * jump (jt if true, jf if false — both are *relative* instruction offsets), or
 * return a value and halt. No backward jumps, no loops: it always terminates,
 * which is why the kernel can safely run a filter supplied by an unprivileged
 * process. `bpf_run` below is that whole machine.
 * ===========================================================================
 */

/* --- our own minimal type + opcode vocabulary (no headers) --------------- */
typedef unsigned int   u32;
typedef unsigned short u16;
typedef unsigned char  u8;

/* One classic-BPF instruction. Byte-for-byte the shape of the kernel's
 * `struct sock_filter`: a 16-bit opcode, two 8-bit relative jump offsets, and a
 * 32-bit immediate operand. */
struct binsn {
    u16 code;   /* operation selector (see opcodes below)                      */
    u8  jt;     /* if a conditional test is TRUE, skip this many instructions   */
    u8  jf;     /* if it is FALSE, skip this many                               */
    u32 k;      /* immediate: a load offset, a compare value, or a return code  */
};

/* The read-only input the filter inspects. A trimmed `struct seccomp_data`:
 * the two fields our allowlist actually reads. Offsets match the real struct
 * (nr at 0, arch at 4), so the same load offsets used in seccomp.c work here. */
struct sdata {
    u32 nr;     /* the syscall number being attempted (offset 0)               */
    u32 arch;   /* the calling ABI, e.g. AUDIT_ARCH_X86_64 (offset 4)          */
};

/* The subset of BPF opcodes a seccomp allowlist needs. These are the exact
 * bit patterns the kernel's BPF_LD|BPF_W|BPF_ABS etc. macros produce. */
#define OP_LD_ABS  0x20u    /* BPF_LD | BPF_W | BPF_ABS : A = input[k]         */
#define OP_JEQ_K   0x15u    /* BPF_JMP| BPF_JEQ| BPF_K   : if A==k jt else jf   */
#define OP_JA      0x05u    /* BPF_JMP| BPF_JA           : pc += k (uncond.)    */
#define OP_RET_K   0x06u    /* BPF_RET| BPF_K            : return k             */

/* Field offsets within `struct sdata`, matching the real seccomp_data layout. */
#define OFF_NR    0u
#define OFF_ARCH  4u

/* Sentinel returned for a malformed program (runaway pc / unknown opcode). The
 * real kernel rejects such programs at load time; we fold that into "kill". */
#define RET_KILL  0u

/* ---------------------------------------------------------------------------
 * bpf_run — interpret one classic-BPF filter against one syscall event.
 *
 *   prog : the instruction array (the compiled allowlist)
 *   len  : number of instructions in prog
 *   in   : the syscall event to judge (nr + arch)
 *   ->   : the filter's return word (an ALLOW / ERRNO / KILL action code)
 *
 * This is the whole seccomp virtual machine. Note the shape the compiler sees:
 * an accumulator held in a register, a program counter, and a switch that the
 * optimizer turns into a chain of compares (we build with -fno-jump-tables so
 * the dispatch is explicit branches — read it in the .s files).
 *
 * We mark it `noinline` on purpose. Without it, the optimizer inlines this whole
 * interpreter into `classify` and then into `main` three times, and the emitted
 * assembly becomes three specialized copies — great for a lesson on inlining,
 * poor for reading the interpreter itself. Keeping it standalone mirrors the
 * REAL kernel, which has exactly ONE classic-BPF interpreter that every filter
 * flows through, and gives us one clean function to annotate line by line. */
__attribute__((noinline))
u32 bpf_run(const struct binsn *prog, u32 len, const struct sdata *in)
{
    u32 A  = 0;             /* the single 32-bit accumulator                    */
    u32 pc = 0;             /* program counter (index into prog)                */

    for (;;) {
        if (pc >= len)
            return RET_KILL;            /* ran off the end: treat as kill        */

        struct binsn ins = prog[pc];    /* fetch the current instruction         */

        switch (ins.code) {
        case OP_LD_ABS:
            /* Load a 32-bit word from the input at absolute offset k into A.
             * Only the two offsets a seccomp filter uses are meaningful here. */
            if (ins.k == OFF_NR)
                A = in->nr;
            else if (ins.k == OFF_ARCH)
                A = in->arch;
            else
                A = 0;                  /* out-of-range load reads as zero        */
            pc = pc + 1;                /* fall through to the next instruction   */
            break;

        case OP_JEQ_K:
            /* Conditional branch. jt/jf are counts of instructions to skip,
             * measured from the instruction AFTER this one — hence the +1. */
            if (A == ins.k)
                pc = pc + 1 + ins.jt;
            else
                pc = pc + 1 + ins.jf;
            break;

        case OP_JA:
            pc = pc + 1 + ins.k;        /* unconditional relative jump            */
            break;

        case OP_RET_K:
            return ins.k;              /* halt: k is the verdict                 */

        default:
            return RET_KILL;           /* unknown opcode: refuse safely          */
        }
    }
}

/* ---------------------------------------------------------------------------
 * A tiny hand-built allowlist, exactly the shape seccomp.c emits:
 *   [0] load arch
 *   [1] if arch == X86_64 skip the next (kill) insn
 *   [2] return KILL
 *   [3] load nr
 *   [4] if nr == read  -> allow
 *   [5] return ALLOW
 *   [6] if nr == write -> allow
 *   [7] return ALLOW
 *   [8] return EPERM (default deny)
 * --------------------------------------------------------------------------- */
#define ARCH_X86_64 0xC000003Eu         /* AUDIT_ARCH_X86_64's actual value      */
#define RET_ALLOW   0x7fff0000u
#define RET_EPERM   0x00050001u          /* SECCOMP_RET_ERRNO | EPERM(1)          */
#define NR_READ     0u
#define NR_WRITE    1u

static const struct binsn allowlist[] = {
    { OP_LD_ABS, 0, 0, OFF_ARCH },
    { OP_JEQ_K,  1, 0, ARCH_X86_64 },
    { OP_RET_K,  0, 0, RET_KILL },
    { OP_LD_ABS, 0, 0, OFF_NR },
    { OP_JEQ_K,  0, 1, NR_READ },
    { OP_RET_K,  0, 0, RET_ALLOW },
    { OP_JEQ_K,  0, 1, NR_WRITE },
    { OP_RET_K,  0, 0, RET_ALLOW },
    { OP_RET_K,  0, 0, RET_EPERM },
};

#define ALLOWLIST_LEN (sizeof allowlist / sizeof allowlist[0])

/* Classify a syscall on x86-64 through the allowlist above. Returns the verdict
 * word — the same thing the kernel would use. */
u32 classify(u32 sysno)
{
    struct sdata ev;
    ev.nr   = sysno;
    ev.arch = ARCH_X86_64;
    return bpf_run(allowlist, (u32)ALLOWLIST_LEN, &ev);
}

/* main lets the file compile and link into a runnable probe. The exit status
 * encodes three probes so you can check the interpreter without a debugger:
 *   bit 0: read  is allowed   (expect 1)
 *   bit 1: write is allowed   (expect 1)
 *   bit 2: a blocked call (nr=101, ptrace) is NOT allowed (expect 1)
 * So a correct build exits with status 7. */
int main(void)
{
    int ok = 0;
    ok |= (classify(NR_READ)  == RET_ALLOW) ? 1 : 0;
    ok |= (classify(NR_WRITE) == RET_ALLOW) ? 2 : 0;
    ok |= (classify(101u)     != RET_ALLOW) ? 4 : 0;   /* ptrace: must be denied */
    return ok;
}
