# =============================================================================
# demo.annotated.s — clang -O1 output for asm/demo.c, explained line by line.
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# This is the exact assembly clang emits for demo.c at -O1 (see demo.s for the
# untouched original), commented on essentially every instruction. AT&T syntax:
#
#     op   src, dst          # movl %esi, %eax   =>  eax = esi
#     %reg                   # a register        $imm  => a literal constant
#     N(%base,%idx)          # memory at [base + idx + N]
#
# SysV AMD64 ABI (the target we generate): integer args arrive in
#     rdi, rsi, rdx, rcx, r8, r9   and the return value goes in rax (eax).
# 32-bit ops (eax) zero-extend into the 64-bit register.
#
# THE BIG LESSON
# --------------
# demo.c is written with constant-time IDIOMS (xor/or/shift/mask, never an `if`
# over a secret). Watch what the optimizer does with them: it recognizes the
# idioms and lowers them to BRANCHLESS instructions — `sete`, `neg`, `sbb`,
# `and`/`xor` — every one of which runs in a fixed number of cycles regardless
# of the operand values. There is not a single conditional jump that depends on
# a secret. THAT is what "constant time" looks like once compiled. (The only
# `je` in the whole file is on the PUBLIC buffer length, not on secret bytes.)
# =============================================================================

	.file	"demo.c"
	.text

# =============================================================================
# u32 ct_eq(u32 x, u32 y)   — return 0xFFFFFFFF if x==y else 0, branch-free.
#   args:   x = edi,  y = esi        return: eax
# The C computed (x^y), folded it to a 0/1 "nonzero" flag, and spread it to a
# full mask. clang saw through all of that and emitted the canonical branchless
# equality: compare, set-if-equal, negate.
# =============================================================================
	.globl	ct_eq
	.p2align	4                       # 16-byte align the function entry
	.type	ct_eq,@function
ct_eq:
	pushq	%rbp                    # PROLOGUE: save caller's frame pointer
	movq	%rsp, %rbp              # establish our frame (kept at -O1 for
	                                #   debuggability; this is a leaf function)
	xorl	%eax, %eax              # eax = 0. Zeroing first so `sete %al` below
	                                #   leaves the upper 24 bits clean.
	cmpl	%esi, %edi              # compare y(esi) with x(edi); sets ZF if equal
	sete	%al                     # al = (x == y) ? 1 : 0   — reads ZF, no jump
	negl	%eax                    # eax = -eax: 1 -> 0xFFFFFFFF, 0 -> 0x00000000
	                                #   i.e. spread the 0/1 to an all-ones/zeros
	                                #   mask. Exactly ct_eq's contract, branchless.
	popq	%rbp                    # EPILOGUE
	retq
.Lfunc_end0:
	.size	ct_eq, .Lfunc_end0-ct_eq

# =============================================================================
# u32 ct_select(u32 mask, u32 a, u32 b)  — return mask ? a : b (mask all-1/all-0)
#   args:   mask = edi,  a = esi,  b = edx        return: eax
# Source was (a & mask) | (b & ~mask). clang rewrote it to the cheaper but
# equivalent  b ^ ((a ^ b) & mask), which is 3 ALU ops and still branchless:
#   mask all-ones  -> b ^ (a^b) = a
#   mask all-zeros -> b ^ 0     = b
# =============================================================================
	.globl	ct_select
	.p2align	4
	.type	ct_select,@function
ct_select:
	pushq	%rbp                    # PROLOGUE
	movq	%rsp, %rbp
	movl	%esi, %eax              # eax = a
	xorl	%edx, %eax              # eax = a ^ b
	andl	%edi, %eax              # eax = (a ^ b) & mask
	xorl	%edx, %eax              # eax = b ^ ((a ^ b) & mask)  == mask?a:b
	popq	%rbp                    # EPILOGUE
	retq
.Lfunc_end1:
	.size	ct_select, .Lfunc_end1-ct_select

# =============================================================================
# u32 ct_memeq(const u8 *a, const u8 *b, u64 n) — constant-time buffer compare.
#   args:   a = rdi,  b = rsi,  n = rdx          return: eax
# The loop ORs together every byte's XOR and NEVER returns early, so its timing
# does not reveal where two buffers first differ (the MAC/tag-compare property).
# =============================================================================
	.globl	ct_memeq
	.p2align	4
	.type	ct_memeq,@function
ct_memeq:
	pushq	%rbp                    # PROLOGUE
	movq	%rsp, %rbp
	testq	%rdx, %rdx              # n == 0 ?
	je	.LBB2_1                     # if so, jump to the "vacuously equal" path.
	                                #   NOTE: this branch is on n, the PUBLIC
	                                #   length, not on any secret byte — so it
	                                #   does not leak secret-dependent timing.

# ---- loop setup -------------------------------------------------------------
	xorl	%eax, %eax              # rax = 0  : loop index i
	xorl	%ecx, %ecx              # cl  = 0  : the running OR accumulator `diff`
	.p2align	4                   # align the hot loop head for the fetcher
.LBB2_5:                                # do { ... } while (i != n)
	movzbl	(%rsi,%rax), %r8d       # r8b = b[i]  (zero-extended load)
	xorb	(%rdi,%rax), %r8b       # r8b = b[i] ^ a[i]
	orb	%r8b, %cl                   # diff |= (a[i] ^ b[i])  — fold into cl.
	                                #   No compare, no branch on the byte value:
	                                #   every iteration does identical work.
	incq	%rax                    # i++
	cmpq	%rax, %rdx              # i == n ?
	jne	.LBB2_5                     # loop over ALL bytes (data-independent count)

# ---- reduce diff to a mask (the inlined ct_eq(diff,0)) ----------------------
# %bb.2:
	xorl	%eax, %eax              # eax = 0
	cmpb	$1, %cl                 # sets CF (borrow) iff cl < 1, i.e. iff cl==0
	sbbl	%eax, %eax              # eax = eax - eax - CF = -CF:
	                                #   diff==0 (equal)     -> CF=1 -> eax=0xFFFFFFFF
	                                #   diff!=0 (differ)    -> CF=0 -> eax=0
	                                #   the classic branchless "is-zero -> mask".
	popq	%rbp
	retq

.LBB2_1:                                # n == 0: no bytes differ, return all-ones
	movl	$-1, %eax               # eax = 0xFFFFFFFF (equal by convention)
	popq	%rbp
	retq
.Lfunc_end2:
	.size	ct_memeq, .Lfunc_end2-ct_memeq

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits   # non-executable stack (safe default)
# =============================================================================
# WHAT TO TAKE AWAY
#   * ct_eq   -> cmp/sete/neg      : equality with no branch on the values.
#   * ct_select -> xor/and/xor     : a data-flow multiplexer, no branch.
#   * ct_memeq -> or-fold + sbb    : compare every byte, no early exit, then turn
#                                    "is zero?" into a mask with sbb — all timing
#                                    is independent of the secret contents.
#   * The one conditional jump is on the public length n. Constant-time code is
#     allowed to branch on PUBLIC values; what it must never do is branch on, or
#     index memory with, a SECRET. Compare with demo.O2.s to see clang unroll and
#     vectorize the loop while keeping it branch-free on the data.
# =============================================================================
