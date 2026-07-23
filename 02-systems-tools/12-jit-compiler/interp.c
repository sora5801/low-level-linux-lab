/* ===========================================================================
 * interp.c — the baseline: a tree-walking (really array-walking) interpreter.
 * ===========================================================================
 *
 * This is the "before" in the JIT's "before/after". It executes the SAME
 * optimized IR the JIT compiles, so the only thing we are measuring when we
 * compare them is dispatch cost: the interpreter pays, for every single op, the
 * price of
 *     (1) a memory load of op.kind,
 *     (2) a branch to the right handler (the switch),
 *     (3) a bounds-free but data-dependent loop back to the top.
 * The CPU's branch predictor cannot see through this indirection well because
 * the "next op" is data, so it mispredicts constantly. That per-op overhead —
 * often 5-10x the useful work — is exactly what a JIT deletes by turning each
 * op into inline native instructions with statically-known control flow.
 *
 * A production interpreter would go further (computed-goto "threaded" dispatch,
 * a `void* handlers[]` table indexed by opcode) to cut the switch cost; we keep
 * a plain switch here because it is the clearest baseline to read and to beat.
 * ===========================================================================
 */
#include "bf.h"

void bf_interp(const bf_program *prog, unsigned char *tape, const bf_io *io)
{
    const op *ops = prog->ops;
    size_t    ip  = 0;              /* instruction pointer: index into ops[]   */
    unsigned char *p = tape;        /* the Brainfuck data pointer              */

    /* The classic interpreter loop. `ip` advances by 1 for straight-line ops
     * and is *assigned* a target for taken branches. There is no separate
     * program-counter register in the machine sense — it is just this variable,
     * which is precisely the indirection the JIT removes. */
    for (;;) {
        const op o = ops[ip];
        switch (o.kind) {

        case OP_ADD:
            /* Byte add with wraparound. `arg` is already folded to 0..255, so
             * casting the sum to unsigned char gives BF's mod-256 semantics. */
            *p = (unsigned char)(*p + (unsigned char)o.arg);
            ip++;
            break;

        case OP_MOVE:
            /* Move the data pointer. `arg` is a signed net delta from folding.
             * NOTE: like most teaching Brainfucks we do not bounds-check the
             * tape here — the caller sizes it (see main.c) and well-formed
             * programs stay inside. A hardened VM would clamp or trap. */
            p += o.arg;
            ip++;
            break;

        case OP_CLEAR:
            /* Peephole result of "[-]": the cell is now zero no matter what it
             * held. One store replaces a whole decrement loop. */
            *p = 0;
            ip++;
            break;

        case OP_OUT:
            /* Output the cell `arg` times (folded '.' run). */
            for (int k = 0; k < o.arg; k++)
                io->write_byte((long)*p);
            ip++;
            break;

        case OP_IN:
            /* Read one byte into the cell; read_byte returns 0 at EOF. */
            *p = (unsigned char)io->read_byte();
            ip++;
            break;

        case OP_JZ:
            /* '[': if the cell is zero, skip the loop body by jumping to the
             * op after the matching ']'. Otherwise fall through into the body. */
            if (*p == 0) ip = (size_t)o.arg;
            else         ip++;
            break;

        case OP_JNZ:
            /* ']': if the cell is non-zero, jump back to the op after the
             * matching '[' to run the body again. Otherwise fall through. */
            if (*p != 0) ip = (size_t)o.arg;
            else         ip++;
            break;

        case OP_HALT:
            return;    /* the single, unambiguous exit */
        }
    }
}
