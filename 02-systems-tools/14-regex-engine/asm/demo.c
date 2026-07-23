/* ===========================================================================
 * asm/demo.c — SELF-CONTAINED extraction of the engine's hottest routine:
 *              the NFA state-set transition (a bitset step).
 * ===========================================================================
 *
 * This file exists purely to generate teaching assembly. It has NO #includes and
 * declares its own types, so a bare cross-compiler can turn it into x86-64 SysV
 * assembly with no libc or system headers present. The function nfa_step() below
 * is the SAME logic as the one in ../sim.c — the innermost loop of the whole
 * regex engine — reproduced here in isolation so every instruction it compiles
 * to can be annotated (see demo.annotated.s).
 *
 * WHAT THE ROUTINE DOES (the linear-time secret, in one loop)
 * -----------------------------------------------------------
 * The matcher represents "the set of NFA states the machine could be in" as a
 * BITSET: one bit per compiled instruction. To consume one input byte `c`, it
 * must compute the successor set. For every ACTIVE state that is a byte-consuming
 * instruction whose 256-bit character set contains `c`, it marks that state's
 * successor (pc+1) in the next set. That is all a "step" is — and because each of
 * the <= m states contributes O(1) work, a step is O(m) and a full match O(n*m).
 * No backtracking, no exponential blow-up.
 *
 * The loop is a textbook "iterate the set bits of a bitset":
 *     b     = __builtin_ctzll(bits);   // index of the lowest set bit  (TZCNT)
 *     bits &= bits - 1;                // clear that lowest set bit     (BLSR)
 * so it visits only active states, not all m of them. Watching these two lines
 * become single x86 instructions is the point of the exercise.
 * ===========================================================================
 */

/* --- our own fixed-width types (no <stdint.h>) ----------------------------- */
typedef unsigned long long u64;   /* 64-bit bitset word / 256-bit class chunk  */
typedef unsigned char      u8;    /* opcode byte                               */
typedef int                i32;   /* class id / signed index                   */

/* The four NFA opcodes. CONSUME is 0 on purpose: the hot test below is
 * `op[state] != OP_CONSUME`, and comparing against 0 is the cheapest compare on
 * x86 (it can even fold into a flag from a preceding load). */
enum { OP_CONSUME = 0, OP_SPLIT = 1, OP_JMP = 2, OP_MATCH = 3 };

/* ---------------------------------------------------------------------------
 * nfa_step — pure state-set transition for one input byte.
 *
 * Inputs:
 *   op    : op[i]  = opcode of state i
 *   cls   : cls[i] = class id of state i (only meaningful when op[i]==CONSUME)
 *   sets  : 256-bit character sets, 4 u64 words each; byte c is in class k iff
 *           bit (c & 63) of word sets[k*4 + (c>>6)] is set.
 *   cur   : current active-state bitset (read only)
 *   next  : successor bitset (WRITE; caller pre-clears it to all zero)
 *   nwords: number of 64-bit words in each bitset = ceil(nstate/64)
 *   c     : the input byte, 0..255
 *
 * Contract: reads op/cls/sets/cur, writes only `next`. No allocation, no calls.
 * --------------------------------------------------------------------------- */
void nfa_step(const u8  *op,
              const i32 *cls,
              const u64 *sets,
              const u64 *cur,
              u64       *next,
              int        nwords,
              int        c)
{
    for (int wi = 0; wi < nwords; wi++) {
        u64 bits = cur[wi];                  /* the active states in this word   */
        while (bits) {                       /* loop over ONLY the set bits      */
            int b     = __builtin_ctzll(bits);   /* lowest set bit index (TZCNT) */
            bits     &= bits - 1;                /* clear lowest set bit (BLSR)  */
            int state = (wi << 6) + b;           /* absolute state index (pc)    */

            if (op[state] != OP_CONSUME)         /* only consuming states advance */
                continue;                        /*   on an input byte           */

            /* 256-bit membership: is byte c in this state's class set?
             * word index c>>6 (0..3), bit index c&63 — a shift and an AND. */
            i32 k    = cls[state];
            u64 word = sets[(k << 2) + (c >> 6)];
            if ((word >> (c & 63)) & 1u) {
                int t = state + 1;               /* success edge is always pc+1  */
                next[t >> 6] |= (u64)1 << (t & 63);
            }
        }
    }
}

/* ---------------------------------------------------------------------------
 * demo_run — a tiny self-contained exercise so the translation unit is complete
 * and nfa_step cannot be optimized away when this file is compiled on its own.
 *
 * It hand-builds the 3-state NFA for the regex /a[bc]/ ...
 *     state 0: CONSUME {'a'}   -> 1
 *     state 1: CONSUME {'b','c'} -> 2
 *     state 2: MATCH
 * ... seeds the active set to {state 0}, steps it once on the byte 'a', and
 * returns the resulting successor word (which must have bit 1 set). No libc.
 * --------------------------------------------------------------------------- */
static u64 g_sets[8];   /* two classes * 4 words each (256 bits per class)      */

int demo_run(void)
{
    u8  op[3]  = { OP_CONSUME, OP_CONSUME, OP_MATCH };
    i32 cls[3] = { 0, 1, -1 };

    /* class 0 = {'a'} : byte 0x61 -> word 0x61>>6==1, bit 0x61&63==33. */
    for (int i = 0; i < 8; i++) g_sets[i] = 0;
    g_sets[0 * 4 + ('a' >> 6)] |= (u64)1 << ('a' & 63);
    /* class 1 = {'b','c'} : both live in word 1 as well. */
    g_sets[1 * 4 + ('b' >> 6)] |= (u64)1 << ('b' & 63);
    g_sets[1 * 4 + ('c' >> 6)] |= (u64)1 << ('c' & 63);

    u64 cur[1]  = { 1 };    /* active set = {state 0}                            */
    u64 next[1] = { 0 };    /* cleared, per the contract                        */

    nfa_step(op, cls, g_sets, cur, next, /*nwords=*/1, /*c=*/'a');
    return (int)next[0];    /* expect bit 1 set (state 1 became active) => 2    */
}
