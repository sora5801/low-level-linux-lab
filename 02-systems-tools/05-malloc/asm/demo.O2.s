	.file	"demo.c"
	.text
	.globl	align_up                        # -- Begin function align_up
	.p2align	4
	.type	align_up,@function
align_up:                               # @align_up
# %bb.0:
	leaq	(%rdi,%rsi), %rax
	decq	%rax
	negq	%rsi
	andq	%rsi, %rax
	retq
.Lfunc_end0:
	.size	align_up, .Lfunc_end0-align_up
                                        # -- End function
	.globl	bin_index                       # -- Begin function bin_index
	.p2align	4
	.type	bin_index,@function
bin_index:                              # @bin_index
# %bb.0:
	cmpq	$49, %rdi
	movl	$48, %eax
	cmovaeq	%rdi, %rax
	bsrq	%rax, %rcx
	addl	$-5, %ecx
	cmpl	$15, %ecx
	movl	$15, %eax
	cmovbl	%ecx, %eax
	retq
.Lfunc_end1:
	.size	bin_index, .Lfunc_end1-bin_index
                                        # -- End function
	.globl	round_total                     # -- Begin function round_total
	.p2align	4
	.type	round_total,@function
round_total:                            # @round_total
# %bb.0:
	leaq	39(%rdi), %rcx
	andq	$-16, %rcx
	cmpq	$49, %rcx
	movl	$48, %eax
	cmovaeq	%rcx, %rax
	retq
.Lfunc_end2:
	.size	round_total, .Lfunc_end2-round_total
                                        # -- End function
	.globl	split_block                     # -- Begin function split_block
	.p2align	4
	.type	split_block,@function
split_block:                            # @split_block
# %bb.0:
	movq	(%rdi), %rax
	andq	$-16, %rax
	movq	%rax, %rcx
	subq	%rsi, %rcx
	cmpq	$47, %rcx
	ja	.LBB3_2
# %bb.1:
	orq	$1, %rax
	movq	%rax, (%rdi)
	xorl	%eax, %eax
	retq
.LBB3_2:
	movq	%rsi, %rax
	orq	$1, %rax
	movq	%rax, (%rdi)
	leaq	(%rdi,%rsi), %rax
	movq	%rcx, (%rdi,%rsi)
	retq
.Lfunc_end3:
	.size	split_block, .Lfunc_end3-split_block
                                        # -- End function
	.globl	coalesce_next                   # -- Begin function coalesce_next
	.p2align	4
	.type	coalesce_next,@function
coalesce_next:                          # @coalesce_next
# %bb.0:
	movq	%rdi, %rax
	movq	(%rdi), %rcx
	andq	$-16, %rcx
	leaq	(%rdi,%rcx), %rdx
	cmpq	%rsi, %rdx
	jae	.LBB4_3
# %bb.1:
	movq	(%rdx), %rdx
	testb	$1, %dl
	jne	.LBB4_3
# %bb.2:
	andq	$-16, %rdx
	addq	%rcx, %rdx
	movq	%rdx, (%rax)
.LBB4_3:
	retq
.Lfunc_end4:
	.size	coalesce_next, .Lfunc_end4-coalesce_next
                                        # -- End function
	.globl	coalesce_prev                   # -- Begin function coalesce_prev
	.p2align	4
	.type	coalesce_prev,@function
coalesce_prev:                          # @coalesce_prev
# %bb.0:
	movq	%rdi, %rax
	cmpq	%rsi, %rdi
	jbe	.LBB5_3
# %bb.1:
	movq	-8(%rax), %rcx
	testb	$1, %cl
	jne	.LBB5_3
# %bb.2:
	andq	$-16, %rcx
	movq	%rax, %rdx
	subq	%rcx, %rdx
	negq	%rcx
	movq	(%rdx), %rsi
	andq	$-16, %rsi
	movq	(%rax), %rdi
	andq	$-16, %rdi
	addq	%rsi, %rdi
	movq	%rdi, (%rax,%rcx)
	movq	%rdx, %rax
.LBB5_3:
	retq
.Lfunc_end5:
	.size	coalesce_prev, .Lfunc_end5-coalesce_prev
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
