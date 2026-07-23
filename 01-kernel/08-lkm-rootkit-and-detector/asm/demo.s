	.file	"demo.c"
	.text
	.globl	fnv1a64                         # -- Begin function fnv1a64
	.p2align	4
	.type	fnv1a64,@function
fnv1a64:                                # @fnv1a64
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movabsq	$-3750763034362895579, %rdx     # imm = 0xCBF29CE484222325
	testq	%rsi, %rsi
	je	.LBB0_1
# %bb.2:
	xorl	%ecx, %ecx
	movabsq	$1099511628211, %r8             # imm = 0x100000001B3
	.p2align	4
.LBB0_3:                                # =>This Inner Loop Header: Depth=1
	movzbl	(%rdi,%rcx), %eax
	xorq	%rdx, %rax
	imulq	%r8, %rax
	incq	%rcx
	movq	%rax, %rdx
	cmpq	%rcx, %rsi
	jne	.LBB0_3
# %bb.4:
	popq	%rbp
	retq
.LBB0_1:
	movq	%rdx, %rax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	fnv1a64, .Lfunc_end0-fnv1a64
                                        # -- End function
	.globl	region_first_diff               # -- Begin function region_first_diff
	.p2align	4
	.type	region_first_diff,@function
region_first_diff:                      # @region_first_diff
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	$-1, %rax
	testq	%rdx, %rdx
	je	.LBB1_5
# %bb.1:
	xorl	%ecx, %ecx
	.p2align	4
.LBB1_2:                                # =>This Inner Loop Header: Depth=1
	movzbl	(%rdi,%rcx), %r8d
	cmpb	(%rsi,%rcx), %r8b
	jne	.LBB1_3
# %bb.4:                                #   in Loop: Header=BB1_2 Depth=1
	incq	%rcx
	cmpq	%rcx, %rdx
	jne	.LBB1_2
.LBB1_5:
	popq	%rbp
	retq
.LBB1_3:
	movq	%rcx, %rax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	region_first_diff, .Lfunc_end1-region_first_diff
                                        # -- End function
	.globl	table_fingerprint               # -- Begin function table_fingerprint
	.p2align	4
	.type	table_fingerprint,@function
table_fingerprint:                      # @table_fingerprint
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movabsq	$-3750763034362895579, %rcx     # imm = 0xCBF29CE484222325
	shlq	$3, %rsi
	testq	%rsi, %rsi
	je	.LBB2_1
# %bb.2:
	xorl	%edx, %edx
	movabsq	$1099511628211, %r8             # imm = 0x100000001B3
	.p2align	4
.LBB2_3:                                # =>This Inner Loop Header: Depth=1
	movzbl	(%rdi,%rdx), %eax
	xorq	%rcx, %rax
	imulq	%r8, %rax
	incq	%rdx
	movq	%rax, %rcx
	cmpq	%rdx, %rsi
	jne	.LBB2_3
# %bb.4:
	popq	%rbp
	retq
.LBB2_1:
	movq	%rcx, %rax
	popq	%rbp
	retq
.Lfunc_end2:
	.size	table_fingerprint, .Lfunc_end2-table_fingerprint
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
