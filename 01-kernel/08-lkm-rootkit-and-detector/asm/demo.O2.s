	.file	"demo.c"
	.text
	.globl	fnv1a64                         # -- Begin function fnv1a64
	.p2align	4
	.type	fnv1a64,@function
fnv1a64:                                # @fnv1a64
# %bb.0:
	movabsq	$-3750763034362895579, %r8      # imm = 0xCBF29CE484222325
	testq	%rsi, %rsi
	je	.LBB0_1
# %bb.2:
	movabsq	$1099511628211, %rcx            # imm = 0x100000001B3
	movl	%esi, %edx
	andl	$3, %edx
	cmpq	$4, %rsi
	jae	.LBB0_4
# %bb.3:
	xorl	%r9d, %r9d
	jmp	.LBB0_6
.LBB0_1:
	movq	%r8, %rax
	retq
.LBB0_4:
	andq	$-4, %rsi
	xorl	%r9d, %r9d
	.p2align	4
.LBB0_5:                                # =>This Inner Loop Header: Depth=1
	movzbl	(%rdi,%r9), %eax
	xorq	%r8, %rax
	imulq	%rcx, %rax
	movzbl	1(%rdi,%r9), %r8d
	xorq	%rax, %r8
	imulq	%rcx, %r8
	movzbl	2(%rdi,%r9), %eax
	xorq	%r8, %rax
	imulq	%rcx, %rax
	movzbl	3(%rdi,%r9), %r8d
	xorq	%rax, %r8
	imulq	%rcx, %r8
	addq	$4, %r9
	cmpq	%r9, %rsi
	jne	.LBB0_5
.LBB0_6:
	movq	%r8, %rax
	testq	%rdx, %rdx
	je	.LBB0_9
# %bb.7:
	addq	%r9, %rdi
	xorl	%esi, %esi
	.p2align	4
.LBB0_8:                                # =>This Inner Loop Header: Depth=1
	movzbl	(%rdi,%rsi), %eax
	xorq	%r8, %rax
	imulq	%rcx, %rax
	incq	%rsi
	movq	%rax, %r8
	cmpq	%rsi, %rdx
	jne	.LBB0_8
.LBB0_9:
	retq
.Lfunc_end0:
	.size	fnv1a64, .Lfunc_end0-fnv1a64
                                        # -- End function
	.globl	region_first_diff               # -- Begin function region_first_diff
	.p2align	4
	.type	region_first_diff,@function
region_first_diff:                      # @region_first_diff
# %bb.0:
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
	retq
.LBB1_3:
	movq	%rcx, %rax
	retq
.Lfunc_end1:
	.size	region_first_diff, .Lfunc_end1-region_first_diff
                                        # -- End function
	.globl	table_fingerprint               # -- Begin function table_fingerprint
	.p2align	4
	.type	table_fingerprint,@function
table_fingerprint:                      # @table_fingerprint
# %bb.0:
	movabsq	$-3750763034362895579, %rax     # imm = 0xCBF29CE484222325
	shlq	$3, %rsi
	testq	%rsi, %rsi
	je	.LBB2_3
# %bb.1:
	movabsq	$1099511628211, %rcx            # imm = 0x100000001B3
	xorl	%edx, %edx
	.p2align	4
.LBB2_2:                                # =>This Inner Loop Header: Depth=1
	movzbl	(%rdi,%rdx), %r8d
	xorq	%rax, %r8
	imulq	%rcx, %r8
	movzbl	1(%rdi,%rdx), %eax
	xorq	%r8, %rax
	imulq	%rcx, %rax
	movzbl	2(%rdi,%rdx), %r8d
	xorq	%rax, %r8
	imulq	%rcx, %r8
	movzbl	3(%rdi,%rdx), %eax
	xorq	%r8, %rax
	imulq	%rcx, %rax
	addq	$4, %rdx
	cmpq	%rdx, %rsi
	jne	.LBB2_2
.LBB2_3:
	retq
.Lfunc_end2:
	.size	table_fingerprint, .Lfunc_end2-table_fingerprint
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
