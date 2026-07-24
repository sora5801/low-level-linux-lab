# =============================================================================
# demo.annotated.s — clang's -O1 output for demo.c, explained line by line.
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# This is the EXACT assembly clang 20 emits for asm/demo.c at -O1 (see demo.s
# for the untouched original), with a comment on essentially every instruction.
# AT&T syntax: `op src, dst`; %reg is a register; $imm an immediate; N(%r,%i)
# addresses memory at [r + i + N]. Register widths are the same register: rax
# (64) / eax (32) / al (8).
#
# THE POINT OF THIS FILE
# ----------------------
# demo.c holds TWO loops that do the same per-byte work but differ in one way:
#   * scalar_strlen()        has an UNBOUNDED trip count (stop at the NUL).
#   * count_nonzero_bounded() has a KNOWN trip count n.
# That single difference decides whether the optimizer may use SIMD. Read the
# two functions below, then open demo.O2.s and watch it play out:
#   - scalar_strlen stays a scalar byte loop even at -O2/-O3, because a wide
#     vector load could fault by reading past the array; the compiler may not
#     introduce a fault the C program wouldn't have. THIS is why glibc and our
#     simd_asm.S must hand-roll the aligned-load strlen — the compiler won't.
#   - count_nonzero_bounded auto-vectorizes at -O2 (pcmpeqb + a paddq reduce),
#     because with a known n there is no over-read to fear.
# =============================================================================

	.file	"demo.c"
	.text

# =============================================================================
# usize scalar_strlen(const char *s)          [rdi = s]  ->  rax = length
# =============================================================================
	.globl	scalar_strlen
	.p2align	4                       # 16-byte-align the entry (fetch friendly)
	.type	scalar_strlen,@function
scalar_strlen:
# ---- PROLOGUE (kept at -O1 for debuggability; this leaf needs no frame) ------
	pushq	%rbp                            # save caller's frame pointer
	movq	%rsp, %rbp                      # rbp = frame base for backtraces

# ---- THE IDIOM: fuse "current index" with the return value -------------------
# rax will hold BOTH the loop index and, at exit, the length. Starting it at -1
# and testing the byte at [s + rax + 1] means: iteration k tests s[k], and after
# the post-increment rax == (index of the byte just tested). So when we stop on
# the NUL, rax already equals its index == the string length. One register, no
# separate pointer subtraction at the end.
	movq	$-1, %rax                       # rax = -1  (so first test hits s[0])
	.p2align	4                       # align the hot loop top
.LBB0_1:                                        # <- loop: "while (s[rax+1] != 0)"
	cmpb	$0, 1(%rdi,%rax)                # compare the byte at [s + rax + 1]
                                                #   against 0x00. `b` = 8-bit compare.
	leaq	1(%rax), %rax                   # rax++ WITHOUT touching flags (LEA does
                                                #   arithmetic but never sets EFLAGS, so
                                                #   the cmpb result below is preserved)
	jne	.LBB0_1                         # not the NUL yet -> keep scanning.
                                                #   (jne reads ZF set by cmpb, above)

# ---- EXIT: rax is the index of the NUL = the length --------------------------
	popq	%rbp                            # restore caller's frame pointer
	retq                                    # return length in rax
.Lfunc_end0:
	.size	scalar_strlen, .Lfunc_end0-scalar_strlen


# =============================================================================
# usize count_nonzero_bounded(const char *s, usize n)  [rdi=s, rsi=n] -> rax
# =============================================================================
# Same byte test, KNOWN count. At -O1 clang emits a scalar loop but already uses
# a BRANCHLESS counter (the sbb trick). At -O2 it vectorizes this same loop —
# compare this body with demo.O2.s to see the transformation.
	.globl	count_nonzero_bounded
	.p2align	4
	.type	count_nonzero_bounded,@function
count_nonzero_bounded:
	pushq	%rbp                            # PROLOGUE: save frame pointer
	movq	%rsp, %rbp                      # establish frame

	testq	%rsi, %rsi                      # n == 0 ?
	je	.LBB1_1                         #   yes -> return 0 (skip the loop)

	xorl	%ecx, %ecx                      # rcx = i = 0 (xor zeroes 64 bits too)
	xorl	%eax, %eax                      # rax = count = 0
	.p2align	4
.LBB1_4:                                        # <- for (i=0; i<n; i++)
	cmpb	$1, (%rdi,%rcx)                 # compare s[i] with 1. For an UNSIGNED
                                                #   byte, s[i] < 1  <=>  s[i] == 0, and
                                                #   "below" sets CF. So CF = (s[i]==0).
	sbbq	$-1, %rax                       # rax = rax - (-1) - CF = rax + 1 - CF.
                                                #   byte!=0 (CF=0): count += 1
                                                #   byte==0 (CF=1): count += 0
                                                #   => branchless "count nonzero bytes".
	incq	%rcx                            # i++
	cmpq	%rcx, %rsi                      # i == n ?
	jne	.LBB1_4                         #   no -> loop

	popq	%rbp                            # EPILOGUE
	retq                                    # return count in rax

.LBB1_1:                                        # n == 0 fast path
	xorl	%eax, %eax                      # count = 0
	popq	%rbp
	retq
.Lfunc_end1:
	.size	count_nonzero_bounded, .Lfunc_end1-count_nonzero_bounded

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits  # non-executable stack (W^X default)
# =============================================================================
# WHAT TO TAKE AWAY
#   * The compiler will NOT auto-vectorize an unbounded search loop (strlen),
#     because a speculative wide load could fault past the buffer. That refusal
#     is the entire reason fast strlen/memchr are hand-written with the aligned-
#     load-plus-pcmpeqb/pmovmskb trick you see in ../simd_asm.S.
#   * A bounded loop with the same work DOES vectorize (see demo.O2.s). The lit-
#     mus test the optimizer applies is "can I prove I won't read out of bounds?"
#   * Idioms worth memorizing: rax=-1 to fuse index+return in strlen; and
#     `cmp $1,byte` + `sbb $-1,acc` to count zero/nonzero bytes branchlessly.
# =============================================================================
