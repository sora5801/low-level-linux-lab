/* ===========================================================================
 * switch.S — the hand-written user-space context switch. THIS is the star.
 * ===========================================================================
 *
 * Everything else in this project (the run queue, spawn/yield/join) is ordinary
 * C. The one thing C cannot express is "stop running on THIS stack, start
 * running on THAT one." That is what these ~20 instructions do, and doing it
 * correctly is the entire art of a green-thread library.
 *
 * -------------------------------------------------------------------------
 * THE ABI CONTRACT WE EXPLOIT (System V AMD64, the Linux/x86-64 convention)
 * -------------------------------------------------------------------------
 *   Integer/pointer ARGUMENTS, in order:  rdi, rsi, rdx, rcx, r8, r9
 *   RETURN value:                          rax  (rdx:rax for 128-bit)
 *   CALLER-saved (volatile) registers:     rax, rcx, rdx, rsi, rdi, r8-r11
 *   CALLEE-saved (non-volatile) registers: rbx, rbp, r12, r13, r14, r15, rsp
 *   Stack alignment:  rsp % 16 == 0 at the point of a `call` (so a callee sees
 *                     rsp % 16 == 8 on entry, because `call` pushed 8 bytes).
 *   The red zone:     128 bytes below rsp that a LEAF function may scribble in
 *                     without adjusting rsp. gt_switch is NOT a leaf (it is the
 *                     thing being switched through), so we never rely on it.
 *
 * -------------------------------------------------------------------------
 * WHY WE ONLY SAVE THE CALLEE-SAVED REGISTERS (the key insight)
 * -------------------------------------------------------------------------
 * gt_switch is, to the C compiler, an ordinary function call. The compiler
 * therefore already assumes that *caller-saved* registers (rax, rcx, rdx, rsi,
 * rdi, r8-r11) may be destroyed across the call — so before calling us it has
 * ALREADY spilled anything live in them to its own stack. We get that saving
 * for free. The only registers a well-behaved function must hand back UNCHANGED
 * are the callee-saved ones. So to make the caller unable to tell it was ever
 * suspended, we must preserve exactly:  rbx, rbp, r12, r13, r14, r15  (+ rsp,
 * which we swap on purpose, and the return address, which lives on the stack).
 * Save those six and the switch is transparent. Save more and you waste cycles;
 * save fewer and you get memory corruption that only bites after a switch.
 *
 * -------------------------------------------------------------------------
 * HOW A CONTEXT IS REPRESENTED
 * -------------------------------------------------------------------------
 * A gt_context is a single word: the saved rsp (see gt.h). Everything else is
 * pushed onto the task's OWN stack. So "resume state" = "the six saved
 * registers + a return address, sitting at the top of the parked stack" and the
 * context only has to remember where that top is.
 * ===========================================================================
 */

	.text

/* ===========================================================================
 * void gt_switch(gt_context *from, gt_context *to);
 *
 *   from -> rdi   (where to SAVE the outgoing task's rsp)
 *   to   -> rsi   (where to LOAD  the incoming task's rsp from)
 *
 * Contract: on return, the CALLER (whichever task we switch back INTO later)
 * sees all its callee-saved registers exactly as they were, as if gt_switch
 * had simply returned immediately. The trick is that "later" may be a totally
 * different point in time on a totally different stack.
 * ===========================================================================
 */
	.globl	gt_switch
	.p2align 4                       /* 16-byte align the entry for the fetcher */
	.type	gt_switch,@function
gt_switch:
	/* --- SAVE outgoing context onto the CURRENT stack -------------------
	 * Push the six callee-saved registers. Order matters only in that the
	 * restore side below must pop them in the mirror order. After these six
	 * pushes, the current stack top holds a complete snapshot of the
	 * registers we owe the caller, sitting just above the return address
	 * that `call gt_switch` already pushed. */
	pushq	%rbp                     /* save frame pointer                     */
	pushq	%rbx                     /* save rbx                               */
	pushq	%r12                     /* save r12                               */
	pushq	%r13                     /* save r13                               */
	pushq	%r14                     /* save r14                               */
	pushq	%r15                     /* save r15  (rsp now points at this word)*/

	/* Record where that snapshot lives: from->rsp = current rsp.
	 * gt_context.rsp is at offset 0, so no displacement is needed. This is
	 * the ONE store that "parks" the outgoing task — after it, the task can
	 * be resumed at any time just by reloading rsp from this word. */
	movq	%rsp, (%rdi)             /* from->rsp = rsp                        */

	/* --- LOAD incoming context: switch stacks --------------------------
	 * The instant this executes, we are running on a DIFFERENT stack. The
	 * six registers we are about to pop belong to `to`, not to us. */
	movq	(%rsi), %rsp             /* rsp = to->rsp   (STACK SWAP happens here)*/

	/* --- RESTORE incoming context from the NEW stack -------------------
	 * Pop in the mirror order of the pushes above. These values were placed
	 * here either by a previous gt_switch that parked `to`, or (for a
	 * brand-new task) synthesised by gt_spawn's fake initial frame. */
	popq	%r15                     /* restore r15                            */
	popq	%r14                     /* restore r14                            */
	popq	%r13                     /* restore r13                            */
	popq	%r12                     /* restore r12                            */
	popq	%rbx                     /* restore rbx                            */
	popq	%rbp                     /* restore rbp                            */

	/* `ret` pops the return address that is now on top of the incoming
	 * stack and jumps to it. For a task that was parked mid-gt_yield, that
	 * address is the instruction right after its own `call gt_switch`, so
	 * control resumes exactly where it left off. For a brand-new task, the
	 * fake frame put gt_trampoline's address here, so we "return" into the
	 * trampoline that launches the task. Either way: same instruction. */
	ret
.Lgt_switch_end:
	.size	gt_switch, .Lgt_switch_end-gt_switch

/* ===========================================================================
 * gt_trampoline — the first thing a BRAND-NEW task ever executes.
 *
 * A new task has no "return address after a yield" to come back to, so gt_spawn
 * fabricates an initial stack frame such that the very first gt_switch INTO the
 * task `ret`s here. gt_spawn also plants two values in the fake frame's saved
 * r12/r13 slots, so after gt_switch's pops they arrive in those registers:
 *
 *     r12 = the task's entry function   (void (*fn)(void *))
 *     r13 = the argument to pass it     (void *arg)
 *
 * We choose r12/r13 precisely because they are callee-saved and therefore ARE
 * the values gt_switch restores from the stack — there is no other legal way to
 * hand data across the `ret` that lands us here.
 *
 * ALIGNMENT: gt_spawn arranges the fake frame so that on entry here rsp is
 * 16-byte aligned. We make no push before the `call` below, so the ABI's
 * "rsp % 16 == 0 at a call" rule is satisfied and fn() sees a proper stack.
 * ===========================================================================
 */
	.globl	gt_trampoline
	.p2align 4
	.type	gt_trampoline,@function
gt_trampoline:
	movq	%r13, %rdi               /* arg -> first integer argument register */
	callq	*%r12                    /* fn(arg); rsp is 16-aligned here (correct)*/

	/* fn returned normally => this task is finished. Hand control to the C
	 * side, which marks us GT_DEAD, wakes any joiner, and switches back to
	 * the scheduler. gt_coro_exit is declared noreturn, so control never
	 * comes back here. @PLT keeps the call correct under PIE linking. */
	callq	gt_coro_exit@PLT
	ud2                              /* trap loudly if the "noreturn" ever does */
.Lgt_trampoline_end:
	.size	gt_trampoline, .Lgt_trampoline_end-gt_trampoline

/* Mark the stack as NON-executable. Without this note the linker may
 * conservatively request an executable stack for the whole program — a
 * needless security downgrade. We only ever DATA on our stacks, never code. */
	.section	.note.GNU-stack,"",@progbits
