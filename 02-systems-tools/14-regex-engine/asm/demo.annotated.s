# =============================================================================
# demo.annotated.s — clang -O1 output for asm/demo.c, explained instruction by
#                     instruction. This is the NFA state-set transition, the
#                     inner loop that makes the whole regex engine linear-time.
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# This is the EXACT assembly clang 20 emits for asm/demo.c at -O1 (see demo.s for
# the untouched original), with a comment on essentially every instruction. AT&T
# syntax throughout:  op  src, dst    (destination LAST). `%reg` is a register,
# `$imm` an immediate, `N(%base,%index,scale)` is memory at base+index*scale+N.
# Register widths are the same register: rax(64) / eax(32) / ax(16) / al(8);
# writing eax zero-extends into rax, which is why the compiler prefers `movl`.
#
# THE SYSTEM V AMD64 ABI (the contract every function here obeys)
# --------------------------------------------------------------
#   integer/pointer args, in order:  rdi, rsi, rdx, rcx, r8, r9, then the STACK
#   return value:                    rax
#   callee-saved (we must preserve):  rbx, rbp, r12, r13, r14, r15
#   caller-saved (free to clobber):   rax, rcx, rdx, rsi, rdi, r8, r9, r10, r11
#   the "red zone":                  128 bytes below rsp a leaf may use freely
#   stack alignment:                 rsp % 16 == 0 at the point of any `call`
#
# nfa_step takes SEVEN parameters, so the first six are in registers and the
# seventh (`c`) arrives on the stack:
#     rdi = op      (const u8  *)     rcx = cur   (const u64 *)
#     rsi = cls     (const i32 *)     r8  = next  (u64 *)
#     rdx = sets    (const u64 *)     r9d = nwords(int)
#     16(%rbp) = c  (int, the 7th arg, pushed by the caller before the call)
#
# THE SHAPE OF THE CODE
# ---------------------
# Two nested loops:
#   OUTER  (BB0_2)  walks the bitset word by word:            for wi in 0..nwords
#   INNER  (BB0_4)  walks the SET BITS of one word:           while bits != 0
# The inner body does exactly what the C does: skip non-CONSUME states, test the
# input byte against the state's 256-bit class set, and on a hit set the
# successor bit in `next`. The two headline optimizations to notice:
#   (1) SHRINK-WRAPPING: the `nwords <= 0` early-out happens BEFORE the prologue,
#       so the trivial call touches no callee-saved registers and no stack.
#   (2) PER-BYTE CONSTANT HOISTING: the membership mask `1 << (c & 63)` and the
#       word offset `c >> 6` depend only on `c`, so clang computes them ONCE,
#       before the loops, and reuses them for every state. The bit test then
#       becomes a single `test` of the class word against that mask.
# =============================================================================

	.file	"demo.c"
	.text
	.globl	nfa_step                        # export nfa_step for the linker
	.p2align	4                       # 16-byte align the entry (I-fetch)
	.type	nfa_step,@function
nfa_step:                                       # void nfa_step(op,cls,sets,cur,next,nwords,c)

# ---- SHRINK-WRAPPED EARLY EXIT: if (nwords <= 0) return; ---------------------
# Done FIRST, before we spend any instructions saving registers or building a
# frame. If there are no words to scan there is nothing to do, and this path
# leaves the stack exactly as the caller left it.
	testl	%r9d, %r9d              # set flags from nwords (r9d) — is it <= 0?
	jle	.LBB0_10               #   if nwords <= 0, jump straight to `ret`

# ---- PROLOGUE (only reached when there IS work) -----------------------------
# nfa_step needs many long-lived values (pointers, hoisted constants, the two
# loop counters), so it claims all the callee-saved registers and must therefore
# save them for the caller. This is the classic "spill to callee-saved" tradeoff:
# more push/pop at the edges, but no memory traffic inside the hot loop.
	pushq	%rbp                    # save caller's frame pointer
	movq	%rsp, %rbp             # establish our frame; rbp = frame base
	pushq	%r15                    # save callee-saved regs we are about to use
	pushq	%r14                    #   (r15,r14,r13,r12,rbx). After these five
	pushq	%r13                    #   pushes rsp has moved 5*8=40 bytes, but the
	pushq	%r12                    #   7th arg `c` was pushed by the caller BEFORE
	pushq	%rbx                    #   `call`, so it still sits at 16(%rbp).

# ---- HOIST THE PER-BYTE CONSTANTS out of the loops --------------------------
# rcx is needed as the shift-count register (only `cl` can vary a shift), so the
# `cur` pointer is moved out of rcx into rax first.
	movq	%rcx, %rax             # rax = cur   (free rcx for use as `cl`)
	movl	16(%rbp), %ecx         # ecx = c     (7th argument, from the stack)
	movl	$1, %r10d              # r10 = 1
	shlq	%cl, %r10              # r10 = 1 << (c & 63)  == the MEMBERSHIP MASK.
	                                #   (shlq masks the count to 6 bits, so this
	                                #   is exactly bit (c&63) of the class word.)
	movl	%ecx, %r11d            # r11 = c
	sarl	$6, %r11d              # r11 = c >> 6  == the class WORD INDEX (0..3).
	movl	%r9d, %r9d             # zero-extend nwords into the full r9 so the
	                                #   64-bit loop compare below is well-defined.
	xorl	%ebx, %ebx             # rbx = 0  == wi, the OUTER loop counter.
	jmp	.LBB0_2                # enter the outer loop at its header

# ---- OUTER LOOP LATCH: advance wi, test against nwords -----------------------
	.p2align	4
.LBB0_8:                                        # (outer "continue" target)
	incq	%rbx                    # wi++
	cmpq	%r9, %rbx              # wi == nwords ?
	je	.LBB0_9                #   yes -> fall out to the epilogue

# ---- OUTER LOOP HEADER: load one bitset word --------------------------------
.LBB0_2:                                        # for (; wi < nwords; ) {
	movq	(%rax,%rbx,8), %r14    # r14 = cur[wi]  (rax=cur, *8 = 8 bytes/word)
	testq	%r14, %r14             #   are ANY states active in this word?
	je	.LBB0_8                #   no bits set -> skip to next word

# ---- Precompute this word's base state index (wi*64) ------------------------
# state = wi*64 + b, and pc+1 = state+1. Both bases depend only on wi, so hoist
# them out of the inner loop.
	movl	%ebx, %r15d            # r15 = wi
	shll	$6, %r15d              # r15 = wi << 6  == wi*64  (base state of word)
	leal	1(%r15), %r12d         # r12 = wi*64 + 1  == base of the SUCCESSOR pc
	jmp	.LBB0_4                # enter the inner (set-bit) loop

# ---- INNER LOOP LATCH: clear the lowest set bit, test for more ---------------
	.p2align	4
.LBB0_7:                                        # (inner "continue" target)
	leaq	-1(%r14), %rcx         # rcx = bits - 1
	andq	%rcx, %r14             # bits = bits & (bits-1)  == clear lowest set
	                                #   bit (Kernighan / the BLSR idiom). At -O2
	                                #   with BMI this becomes a single `blsr`.
	je	.LBB0_8                # no bits left -> outer continue (next word)

# ---- INNER LOOP HEADER: pick the lowest set bit -----------------------------
.LBB0_4:                                        # while (bits) {
	rep bsfq	%r14, %rcx     # rcx = index of lowest set bit of `bits`.
	                                #   `rep bsf` is the encoding of TZCNT; it
	                                #   counts trailing zeros == that bit's index.
	movl	%r15d, %r13d           # r13 = wi*64            (word base)
	orl	%ecx, %r13d            # r13 = wi*64 | b == wi*64 + b == `state`
	                                #   (OR works because b < 64, so no overlap.)
	movslq	%r13d, %r13            # sign-extend state to 64 bits for addressing

# ---- if (op[state] != OP_CONSUME) continue;  (OP_CONSUME == 0) --------------
	cmpb	$0, (%rdi,%r13)        # compare op[state] (rdi=op) against 0
	jne	.LBB0_7                #   not a CONSUME state -> skip it

# ---- Membership test: is byte c in this state's 256-bit class set? ----------
# word = sets[k*4 + (c>>6)];  hit = (word >> (c&63)) & 1;
# clang realizes the whole test as  word & (1<<(c&63))  == `test word, mask`.
# %bb.5:
	movl	(%rsi,%r13,4), %r13d   # r13 = cls[state] = k  (rsi=cls, 4 bytes/int)
	leal	(%r11,%r13,4), %r13d   # r13 = (c>>6) + k*4 == the WORD INDEX into sets
	                                #   (r11 = c>>6 hoisted earlier; k*4 via scale)
	movslq	%r13d, %r13            # extend to 64 bits for addressing
	testq	%r10, (%rdx,%r13,8)    # test class word (rdx=sets) against r10, the
	                                #   membership mask 1<<(c&63). ZF=0 iff bit set.
	je	.LBB0_7                #   byte not in the set -> skip this state

# ---- HIT: next[t>>6] |= 1 << (t&63), where t = state + 1 --------------------
# %bb.6:
	addl	%r12d, %ecx            # ecx = b + (wi*64 + 1) == state + 1 == t (=pc+1)
	movl	$1, %r13d              # r13 = 1
	shlq	%cl, %r13              # r13 = 1 << (t & 63)  == successor bit mask
	                                #   (uses cl = low bits of t BEFORE the shift
	                                #   below rewrites ecx — order matters).
	sarl	$6, %ecx               # ecx = t >> 6  == successor WORD index
	movslq	%ecx, %rcx             # extend word index for addressing
	orq	%r13, (%r8,%rcx,8)     # next[t>>6] |= (1<<(t&63))   (r8 = next) — the
	                                #   ONLY write this function performs.
	jmp	.LBB0_7                # inner continue: clear this bit, look for more

# ---- EPILOGUE: restore callee-saved regs in reverse push order --------------
.LBB0_9:
	popq	%rbx                    # restore rbx ...
	popq	%r12                    #   ... r12 ...
	popq	%r13                    #   ... r13 ...
	popq	%r14                    #   ... r14 ...
	popq	%r15                    #   ... r15 ...
	popq	%rbp                    # restore caller's frame pointer
.LBB0_10:                                       # (the shrink-wrapped early-out lands here)
	retq                            # return to caller (void; rax is irrelevant)
.Lfunc_end0:
	.size	nfa_step, .Lfunc_end0-nfa_step

# =============================================================================
# demo_run — the self-contained driver. It builds the NFA for /a[bc]/, steps it
# once on 'a', and returns next[0] (which must be 2, i.e. bit 1 set = state 1).
# At -O1 clang CONSTANT-FOLDS almost the whole thing: it recognizes the class
# sets are compile-time constants, precomputes them straight into g_sets with
# vector stores, and even proves the answer is 2 without calling nfa_step.
# That folding is itself the lesson: with all inputs known, the optimizer runs
# your algorithm at compile time. (In the real engine the inputs are runtime
# data, so nfa_step above is what actually executes.)
# =============================================================================
	.globl	demo_run
	.p2align	4
	.type	demo_run,@function
demo_run:
# %bb.0:
	pushq	%rbp                    # PROLOGUE: standard frame (kept for debug)
	movq	%rsp, %rbp             #   rbp = frame base
	xorps	%xmm0, %xmm0           # xmm0 = 0.0 bits == 16 zero bytes, used to...
	movaps	%xmm0, g_sets(%rip)     # ...zero g_sets[0..1]  (16 bytes at once)
	movaps	%xmm0, g_sets+32(%rip)  # ...zero g_sets[4..5]  (the whole 64-byte
	movaps	%xmm0, g_sets+48(%rip)  #    array is cleared with four 16-byte
	movaps	%xmm0, g_sets+16(%rip)  #    aligned SSE stores instead of a loop)
	orb	$2, g_sets+12(%rip)     # set bit for 'a': class 0, word 1 (byte off
	                                #   12), value 0x02 == bit 33 == 'a' (0x61).
	orb	$12, g_sets+44(%rip)    # set bits for 'b','c': class 1, word 1 (byte
	                                #   off 44), 0x0C == bits 34,35 == 'b','c'.
	movl	$2, %eax               # return value 2 — FOLDED at compile time
	                                #   (bit 1 of next == state 1 became active).
	popq	%rbp                    # EPILOGUE
	retq
.Lfunc_end1:
	.size	demo_run, .Lfunc_end1-demo_run

# ---- BSS: the 64-byte class-set table, 16-byte aligned ----------------------
	.type	g_sets,@object
	.local	g_sets
	.comm	g_sets,64,16                    # 64 bytes (2 classes * 4 words * 8),
	                                        #   aligned to 16 so the movaps above
	                                        #   are legal aligned stores.
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits  # non-executable stack (security)
	.addrsig                                # address-significance table (LTO aid)

# =============================================================================
# WHAT TO TAKE AWAY
#   * The linear-time guarantee is THIS loop: visit each active state once per
#     input byte, O(active) work via TZCNT + BLSR bit iteration. Nothing here can
#     backtrack, so nothing here can go exponential.
#   * The optimizer hoisted the two per-byte constants (mask 1<<(c&63) and word
#     index c>>6) out of both loops, turning the 256-bit membership test into a
#     single `test class_word, mask`.
#   * Shrink-wrapping put the nwords<=0 early-out ahead of the prologue: a no-op
#     call costs nothing.
#   * Compare with demo.O0.s (every value spilled to the stack, the two hoisted
#     constants recomputed every iteration) and demo.O2.s (BMI `blsr`, more
#     aggressive scheduling) to watch the optimizer work.
# =============================================================================
