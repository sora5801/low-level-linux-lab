/* ===========================================================================
 * breakpoint.c — software breakpoints with int3 (0xCC).
 * ===========================================================================
 *
 * THE CORE TRICK
 * --------------
 * x86 has a one-byte software-breakpoint instruction: `int3`, opcode 0xCC. To
 * plant a breakpoint at address A we:
 *
 *   1. read the 8-byte word W currently at A                (PTRACE_PEEKTEXT)
 *   2. remember W's low byte  (saved_byte = W & 0xFF)       — the real opcode
 *   3. write back  (W & ~0xFF) | 0xCC                       (PTRACE_POKETEXT)
 *
 * Now when the CPU reaches A it executes 0xCC, faults #BP, and the kernel stops
 * the tracee with SIGTRAP. Crucially, after executing a one-byte int3 the CPU's
 * RIP points at A+1. So on a hit we must:
 *
 *   - recognise that RIP-1 is one of our breakpoints,
 *   - restore the saved opcode byte at A,
 *   - rewind RIP back to A (SETREGS),
 *
 * so the *real* instruction runs next and the breakpoint is invisible to the
 * program. To later resume past A without immediately re-tripping, we single-
 * step the restored instruction and only THEN re-plant the 0xCC — that is what
 * step_over_breakpoint() does.
 *
 * All of the byte-twiddling math (steps 2 and 3, and the reverse) is exactly
 * what asm/demo.c extracts and annotates, because it is pure register logic.
 *
 * WHY WORD GRANULARITY: PTRACE_POKETEXT writes 8 bytes at a time. We only want to
 * change ONE byte, so we read the surrounding word, splice our byte into it, and
 * write the whole word back. That read-modify-write is the reason we keep the
 * word intact and only mask the low 8 bits.
 * ===========================================================================
 */
#include <stdio.h>
#include <sys/user.h>     /* struct user_regs_struct (for the RIP rewind)        */
#include "debugger.h"

/* Masks used by the splice. Kept as named constants so the intent is obvious in
 * both the C and the generated assembly. */
#define INT3_OPCODE   0xCCUL               /* the `int3` breakpoint instruction    */
#define LOW_BYTE_MASK 0xFFUL               /* selects the byte we overwrite         */

/* ---------------------------------------------------------------------------
 * bp_enable — install the 0xCC at bp->addr, saving the byte we clobber.
 *
 * Read-modify-write of a single word. `saved_byte` is captured ONLY the first
 * time (when we transition from disabled to enabled) so that re-enabling after a
 * step-over doesn't accidentally save our own 0xCC as the "original" byte.
 * ------------------------------------------------------------------------- */
void bp_enable(debugger *dbg, breakpoint *bp)
{
    if (bp->enabled) return;               /* already armed: nothing to do         */

    int ok = 0;
    long word = inferior_peek(dbg->pid, bp->addr, &ok);
    if (!ok) {
        fprintf(stderr, "bp_enable: cannot read text at 0x%lx\n",
                (unsigned long)bp->addr);
        return;
    }

    /* Save the true opcode byte so we can restore it, then splice in 0xCC.
     *   patched = (word & ~0xFF) | 0xCC
     * The mask ~0xFF clears the low byte; the OR drops 0xCC into that hole. */
    bp->saved_byte = (uint8_t)((unsigned long)word & LOW_BYTE_MASK);
    unsigned long patched = ((unsigned long)word & ~LOW_BYTE_MASK) | INT3_OPCODE;

    if (inferior_poke(dbg->pid, bp->addr, patched) == 0)
        bp->enabled = 1;
}

/* ---------------------------------------------------------------------------
 * bp_disable — put the original opcode byte back.
 *
 * We re-read the word (its upper 7 bytes may have changed if the program wrote
 * nearby, though for code that is rare) and splice the saved byte back into the
 * low position:  restored = (word & ~0xFF) | saved_byte.
 * ------------------------------------------------------------------------- */
void bp_disable(debugger *dbg, breakpoint *bp)
{
    if (!bp->enabled) return;

    int ok = 0;
    long word = inferior_peek(dbg->pid, bp->addr, &ok);
    if (!ok) {
        fprintf(stderr, "bp_disable: cannot read text at 0x%lx\n",
                (unsigned long)bp->addr);
        return;
    }

    unsigned long restored =
        ((unsigned long)word & ~LOW_BYTE_MASK) | (unsigned long)bp->saved_byte;

    if (inferior_poke(dbg->pid, bp->addr, restored) == 0)
        bp->enabled = 0;
}

/* ---------------------------------------------------------------------------
 * bp_find — linear scan of the fixed table. With <= 64 breakpoints this is
 * trivially fast and needs no ordering; a real debugger uses a hash by address.
 * ------------------------------------------------------------------------- */
breakpoint *bp_find(debugger *dbg, uint64_t addr)
{
    for (int i = 0; i < MAX_BREAKPOINTS; i++)
        if (dbg->bps[i].in_use && dbg->bps[i].addr == addr)
            return &dbg->bps[i];
    return NULL;
}

/* ---------------------------------------------------------------------------
 * bp_add — occupy a free slot, then arm the breakpoint.
 * Returns the slot, or NULL if the table is full / already set at addr.
 * ------------------------------------------------------------------------- */
breakpoint *bp_add(debugger *dbg, uint64_t addr)
{
    if (bp_find(dbg, addr)) {
        fprintf(stderr, "breakpoint already set at 0x%lx\n", (unsigned long)addr);
        return NULL;
    }
    for (int i = 0; i < MAX_BREAKPOINTS; i++) {
        breakpoint *bp = &dbg->bps[i];
        if (bp->in_use) continue;
        bp->in_use     = 1;
        bp->addr       = addr;
        bp->enabled    = 0;
        bp->saved_byte = 0;
        bp->id         = dbg->next_bp_id++;
        bp_enable(dbg, bp);            /* plant the 0xCC immediately              */
        return bp;
    }
    fprintf(stderr, "breakpoint table full (max %d)\n", MAX_BREAKPOINTS);
    return NULL;
}

/* bp_remove — disable (restore the byte) and free the slot by id. */
int bp_remove(debugger *dbg, int id)
{
    for (int i = 0; i < MAX_BREAKPOINTS; i++) {
        breakpoint *bp = &dbg->bps[i];
        if (bp->in_use && bp->id == id) {
            bp_disable(dbg, bp);
            bp->in_use = 0;
            return 0;
        }
    }
    fprintf(stderr, "no breakpoint #%d\n", id);
    return -1;
}

/* ---------------------------------------------------------------------------
 * step_over_breakpoint — resume across the breakpoint we are parked on.
 *
 * Precondition: we just handled a hit, so RIP already points AT the breakpoint
 * address (the caller rewound it) and the original opcode is currently restored
 * or about to be. The dance:
 *
 *   1. If a breakpoint sits at the current RIP and is enabled, disable it (so the
 *      real instruction is in place).
 *   2. PTRACE_SINGLESTEP one instruction; wait for the resulting SIGTRAP.
 *   3. Re-enable the breakpoint (re-plant 0xCC) so future passes still trap.
 *
 * If the child exited during the single step (the stepped instruction was, say,
 * an exit syscall), we report that and don't try to re-arm.
 *
 * Return: 1 = stepped and child still alive, 0 = child exited, -1 = error.
 * ------------------------------------------------------------------------- */
int step_over_breakpoint(debugger *dbg)
{
    struct user_regs_struct regs;
    if (inferior_getregs(dbg->pid, &regs) < 0) return -1;

    /* regs.rip is the current instruction pointer; a breakpoint there is the one
     * we must hop over. If none, there is nothing special to do — a plain single
     * step suffices. */
    breakpoint *bp = bp_find(dbg, regs.rip);
    if (bp && bp->enabled)
        bp_disable(dbg, bp);            /* expose the real opcode for one step     */

    if (inferior_step(dbg->pid, 0) < 0) return -1;

    int stopsig = 0, code = 0;
    int alive = inferior_wait(dbg->pid, &stopsig, &code);
    if (alive != 1) {
        if (alive == 0)
            printf("[inferior exited during step, status %d]\n", code);
        return alive == 0 ? 0 : -1;
    }

    if (bp)
        bp_enable(dbg, bp);             /* re-plant 0xCC now that we're past it     */
    return 1;
}
