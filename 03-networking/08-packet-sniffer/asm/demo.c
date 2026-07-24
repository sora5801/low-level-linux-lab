/* ===========================================================================
 * asm/demo.c — the classic-BPF interpreter step, extracted and self-contained.
 * ===========================================================================
 *
 * This is the pure-logic HEART of the whole project: the little accept/reject
 * virtual machine the Linux kernel runs on every captured packet to decide
 * whether userspace wants a copy. filter.c COMPILES an expression into these
 * instructions; here we EXECUTE them, exactly as the kernel's cBPF interpreter
 * (net/core/filter.c, __bpf_prog_run for the classic path) does.
 *
 * It is deliberately self-contained — NO system headers, its own integer types
 * — so `clang -S` turns it into clean, teachable assembly with nothing but this
 * routine in it. See asm/demo.annotated.s for the line-by-line ABI walkthrough.
 *
 * THE MACHINE (cBPF)
 * ------------------
 *   A       : 32-bit accumulator — every load lands here, every test reads it.
 *   X       : 32-bit index register — holds variable offsets (e.g. IP hdr len).
 *   M[0..15]: 16 words of scratch memory.
 *   P[]     : the input packet, `plen` bytes. Loads read big-endian from it.
 *
 * Each instruction is 8 bytes: { u16 code; u8 jt; u8 jf; u32 k; }. `code` is a
 * bitfield — low 3 bits are the CLASS, higher bits select size/mode/operator.
 *
 * TWO INVARIANTS THAT MAKE THIS SAFE TO RUN ON UNTRUSTED PACKETS
 * -------------------------------------------------------------
 *   1. Every packet access is BOUNDS-CHECKED. If a load would read past `plen`,
 *      the program immediately returns 0 (drop). This is why a hostile packet
 *      that lies about its lengths cannot make the VM read out of bounds — the
 *      real kernel enforces the same rule.
 *   2. Jumps are FORWARD-ONLY and relative, so `pc` strictly increases and the
 *      program always terminates. We still loop `while (pc < len)` defensively.
 * ===========================================================================
 */

/* Our own fixed-width types — no <stdint.h> (this must stay freestanding). On
 * every LP64 target clang supports, these widths hold. */
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

/* One classic-BPF instruction, byte-for-byte identical to the kernel's
 * `struct sock_filter`. Layout: 2 + 1 + 1 + 4 = 8 bytes, naturally aligned. */
struct bpf_insn {
    u16 code;   /* opcode bitfield (class | size/mode | op | source)          */
    u8  jt;     /* branch offset if the test is TRUE  (instructions to skip)  */
    u8  jf;     /* branch offset if the test is FALSE                         */
    u32 k;      /* generic operand: an immediate, an offset, or a jump target */
};

/* ---- opcode bitfield constants (values match <linux/bpf_common.h>) -------- */
/* Instruction CLASS — the low 3 bits of `code`. */
#define BPF_LD   0x00   /* load into A                                        */
#define BPF_LDX  0x01   /* load into X                                        */
#define BPF_ST   0x02   /* store A into M[k]                                  */
#define BPF_STX  0x03   /* store X into M[k]                                  */
#define BPF_ALU  0x04   /* arithmetic/logic on A                              */
#define BPF_JMP  0x05   /* conditional/uncond. branch                         */
#define BPF_RET  0x06   /* return a value (0 = drop, else = accept length)    */
#define BPF_MISC 0x07   /* register moves (A<->X)                             */
#define BPF_CLASS(code) ((code) & 0x07)

/* Operand SIZE for loads (bits 3-4). */
#define BPF_W 0x00      /* 32-bit word                                        */
#define BPF_H 0x08      /* 16-bit half                                        */
#define BPF_B 0x10      /* 8-bit byte                                         */
#define BPF_SIZE(code) ((code) & 0x18)

/* Addressing MODE for loads (bits 5-7). */
#define BPF_IMM 0x00    /* A = k (an immediate)                               */
#define BPF_ABS 0x20    /* A = P[k]        (fixed packet offset)              */
#define BPF_IND 0x40    /* A = P[X + k]    (variable packet offset)           */
#define BPF_MEM 0x60    /* A = M[k]        (scratch memory)                   */
#define BPF_LEN 0x80    /* A = packet length                                  */
#define BPF_MSH 0xa0    /* X = 4 * (P[k] & 0xf)   — the IP-header-length trick */
#define BPF_MODE(code) ((code) & 0xe0)

/* ALU operator (bits 4-7, reusing the high nibble). */
#define BPF_ADD 0x00
#define BPF_SUB 0x10
#define BPF_MUL 0x20
#define BPF_DIV 0x30
#define BPF_OR  0x40
#define BPF_AND 0x50
#define BPF_LSH 0x60
#define BPF_RSH 0x70

/* JMP operator (bits 4-7). */
#define BPF_JA   0x00   /* unconditional: pc += k                             */
#define BPF_JEQ  0x10   /* if A == operand                                    */
#define BPF_JGT  0x20   /* if A >  operand                                    */
#define BPF_JGE  0x30   /* if A >= operand                                    */
#define BPF_JSET 0x40   /* if A &  operand  (bit test)                        */
#define BPF_OP(code) ((code) & 0xf0)

/* SOURCE of the second operand (bit 3). K = the immediate `k`; X = register X. */
#define BPF_K 0x00
#define BPF_X 0x08
#define BPF_SRC(code) ((code) & 0x08)

/* RET value source lives in a DIFFERENT field than ALU/JMP's operand source:
 * it is BPF_RVAL = code & 0x18, with BPF_K = 0x00 (return the immediate k) and
 * BPF_A = 0x10 (return the accumulator). Do NOT reuse BPF_SRC (code & 0x08)
 * here — that is the ALU/JMP operand selector and would misclassify RET. */
#define BPF_A 0x10
#define BPF_RVAL(code) ((code) & 0x18)

/* ---------------------------------------------------------------------------
 * Bounds-checked big-endian packet loads.
 *
 * Packets are big-endian (network byte order), so we assemble multi-byte values
 * MOST-SIGNIFICANT-BYTE-FIRST by hand — that is portable and makes the byte math
 * explicit. `*oob` (out-of-bounds) is set to 1 if the access would read past the
 * captured length; the caller turns that into an immediate "drop" (return 0),
 * mirroring the kernel. We return 0 on OOB but the flag is what matters.
 * --------------------------------------------------------------------------- */
static u32 load_byte(const u8 *p, u32 plen, u32 off, int *oob)
{
    if (off + 1 > plen || off + 1 < off) { *oob = 1; return 0; }  /* +overflow */
    return p[off];
}

static u32 load_half(const u8 *p, u32 plen, u32 off, int *oob)
{
    if (off + 2 > plen || off + 2 < off) { *oob = 1; return 0; }
    return ((u32)p[off] << 8) | (u32)p[off + 1];        /* MSB first           */
}

static u32 load_word(const u8 *p, u32 plen, u32 off, int *oob)
{
    if (off + 4 > plen || off + 4 < off) { *oob = 1; return 0; }
    return ((u32)p[off]     << 24) | ((u32)p[off + 1] << 16) |
           ((u32)p[off + 2] <<  8) | ((u32)p[off + 3]);
}

/* ---------------------------------------------------------------------------
 * bpf_run — execute a classic-BPF program against one packet.
 *
 *   prog/len : the instruction array and its length.
 *   pkt/plen : the packet bytes and how many are valid.
 *
 * Returns the program's RET value: 0 means "drop", any non-zero means "accept
 * and copy up to this many bytes to userspace" (the snaplen). This is the exact
 * contract SO_ATTACH_FILTER establishes with the kernel.
 * --------------------------------------------------------------------------- */
u32 bpf_run(const struct bpf_insn *prog, u32 len, const u8 *pkt, u32 plen)
{
    u32 A = 0;              /* accumulator                                     */
    u32 X = 0;              /* index register                                  */
    u32 mem[16] = {0};      /* M[] scratch                                     */
    u32 pc = 0;             /* program counter (instruction index)             */
    int oob = 0;            /* set by a load that ran off the end of the packet*/

    while (pc < len) {
        const struct bpf_insn *in = &prog[pc];
        u16 code = in->code;
        u32 k    = in->k;

        switch (BPF_CLASS(code)) {

        /* ---- LOAD into A ------------------------------------------------- */
        case BPF_LD:
            switch (BPF_MODE(code)) {
            case BPF_IMM: A = k; break;                     /* A = k           */
            case BPF_LEN: A = plen; break;                  /* A = packet len  */
            case BPF_MEM: A = mem[k & 0xf]; break;          /* A = M[k]        */
            case BPF_ABS:                                   /* A = P[k]        */
                if (BPF_SIZE(code) == BPF_W) A = load_word(pkt, plen, k, &oob);
                else if (BPF_SIZE(code) == BPF_H) A = load_half(pkt, plen, k, &oob);
                else A = load_byte(pkt, plen, k, &oob);
                if (oob) return 0;                          /* OOB => drop     */
                break;
            case BPF_IND:                                   /* A = P[X + k]    */
                if (BPF_SIZE(code) == BPF_W) A = load_word(pkt, plen, X + k, &oob);
                else if (BPF_SIZE(code) == BPF_H) A = load_half(pkt, plen, X + k, &oob);
                else A = load_byte(pkt, plen, X + k, &oob);
                if (oob) return 0;
                break;
            default: return 0;                              /* unknown mode    */
            }
            pc++;
            break;

        /* ---- LOAD into X ------------------------------------------------- */
        case BPF_LDX:
            switch (BPF_MODE(code)) {
            case BPF_IMM: X = k; break;                     /* X = k           */
            case BPF_LEN: X = plen; break;                  /* X = packet len  */
            case BPF_MEM: X = mem[k & 0xf]; break;          /* X = M[k]        */
            case BPF_MSH:                                   /* X = 4*(P[k]&0xf)*/
                /* The IPv4 header-length trick: low nibble of the version/IHL
                 * byte is the header length in 32-bit words; *4 gives bytes.
                 * Used so indexed loads can reach past variable IP options. */
                X = 4u * (load_byte(pkt, plen, k, &oob) & 0xf);
                if (oob) return 0;
                break;
            default: return 0;
            }
            pc++;
            break;

        /* ---- STORE scratch ---------------------------------------------- */
        case BPF_ST:  mem[k & 0xf] = A; pc++; break;        /* M[k] = A        */
        case BPF_STX: mem[k & 0xf] = X; pc++; break;        /* M[k] = X        */

        /* ---- ALU on A --------------------------------------------------- */
        case BPF_ALU: {
            /* Second operand is either the immediate k or register X. */
            u32 b = (BPF_SRC(code) == BPF_X) ? X : k;
            switch (BPF_OP(code)) {
            case BPF_ADD: A += b; break;
            case BPF_SUB: A -= b; break;
            case BPF_MUL: A *= b; break;
            case BPF_DIV: if (b == 0) return 0; A /= b; break;  /* div0 => drop */
            case BPF_OR:  A |= b; break;
            case BPF_AND: A &= b; break;
            case BPF_LSH: A <<= (b & 31); break;
            case BPF_RSH: A >>= (b & 31); break;
            default: return 0;
            }
            pc++;
            break;
        }

        /* ---- JMP -------------------------------------------------------- *
         * jt/jf are how many instructions to SKIP past pc+1. A well-formed
         * program never jumps backward, so pc always advances. */
        case BPF_JMP: {
            if (BPF_OP(code) == BPF_JA) { pc += 1 + k; break; }
            u32 b = (BPF_SRC(code) == BPF_X) ? X : k;
            int t;
            switch (BPF_OP(code)) {
            case BPF_JEQ:  t = (A == b); break;
            case BPF_JGT:  t = (A >  b); break;
            case BPF_JGE:  t = (A >= b); break;
            case BPF_JSET: t = (A &  b) != 0; break;        /* bitmask test    */
            default: return 0;
            }
            /* +1 because offsets are measured from the NEXT instruction. */
            pc += 1 + (t ? in->jt : in->jf);
            break;
        }

        /* ---- RET: the only exit --------------------------------------- */
        case BPF_RET:
            /* Value is A (BPF_A) or the immediate k (BPF_K). Non-zero = accept
             * that many bytes; zero = drop. The selector is the RVAL field. */
            return (BPF_RVAL(code) == BPF_A) ? A : k;

        /* ---- MISC: register moves ------------------------------------- */
        case BPF_MISC:
            /* Bit 0x80 distinguishes TXA (A=X) from TAX (X=A). */
            if (code & 0x80) A = X;                         /* TXA             */
            else             X = A;                         /* TAX             */
            pc++;
            break;

        default:
            return 0;                                       /* unknown class   */
        }
    }

    /* Falling off the end without a RET is a malformed program; the kernel's
     * verifier forbids it, so we treat it as "drop". */
    return 0;
}
