	.file	"demo.c"
	.text
	.globl	align_up                        # -- Begin function align_up
	.p2align	4
	.type	align_up,@function
align_up:                               # @align_up
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	-8(%rbp), %rax
	movq	-16(%rbp), %rcx
	subq	$1, %rcx
	addq	%rcx, %rax
	movq	-16(%rbp), %rcx
	subq	$1, %rcx
	xorq	$-1, %rcx
	andq	%rcx, %rax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	align_up, .Lfunc_end0-align_up
                                        # -- End function
	.globl	bin_index                       # -- Begin function bin_index
	.p2align	4
	.type	bin_index,@function
bin_index:                              # @bin_index
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	cmpq	$48, -8(%rbp)
	jae	.LBB1_2
# %bb.1:
	movq	$48, -8(%rbp)
.LBB1_2:
	movq	-8(%rbp), %rcx
                                        # implicit-def: $rax
	bsrq	%rcx, %rax
	xorq	$63, %rax
	movl	%eax, %ecx
	movl	$63, %eax
	subl	%ecx, %eax
	movl	%eax, -12(%rbp)
	movl	-12(%rbp), %eax
	subl	$5, %eax
	movl	%eax, -16(%rbp)
	cmpl	$0, -16(%rbp)
	jge	.LBB1_4
# %bb.3:
	movl	$0, -16(%rbp)
.LBB1_4:
	cmpl	$16, -16(%rbp)
	jl	.LBB1_6
# %bb.5:
	movl	$15, -16(%rbp)
.LBB1_6:
	movl	-16(%rbp), %eax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	bin_index, .Lfunc_end1-bin_index
                                        # -- End function
	.globl	round_total                     # -- Begin function round_total
	.p2align	4
	.type	round_total,@function
round_total:                            # @round_total
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$32, %rsp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rax
	addq	$16, %rax
	addq	$8, %rax
	movq	%rax, -16(%rbp)
	movq	-16(%rbp), %rdi
	movl	$16, %esi
	callq	align_up
	movq	%rax, -16(%rbp)
	cmpq	$48, -16(%rbp)
	jae	.LBB2_2
# %bb.1:
	movl	$48, %eax
	movq	%rax, -24(%rbp)                 # 8-byte Spill
	jmp	.LBB2_3
.LBB2_2:
	movq	-16(%rbp), %rax
	movq	%rax, -24(%rbp)                 # 8-byte Spill
.LBB2_3:
	movq	-24(%rbp), %rax                 # 8-byte Reload
	addq	$32, %rsp
	popq	%rbp
	retq
.Lfunc_end2:
	.size	round_total, .Lfunc_end2-round_total
                                        # -- End function
	.globl	split_block                     # -- Begin function split_block
	.p2align	4
	.type	split_block,@function
split_block:                            # @split_block
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$48, %rsp
	movq	%rdi, -16(%rbp)
	movq	%rsi, -24(%rbp)
	movq	-16(%rbp), %rdi
	callq	blk_size
	movq	%rax, -32(%rbp)
	movq	-32(%rbp), %rax
	subq	-24(%rbp), %rax
	movq	%rax, -40(%rbp)
	cmpq	$48, -40(%rbp)
	jae	.LBB3_2
# %bb.1:
	movq	-32(%rbp), %rcx
	orq	$1, %rcx
	movq	-16(%rbp), %rax
	movq	%rcx, (%rax)
	movq	$0, -8(%rbp)
	jmp	.LBB3_3
.LBB3_2:
	movq	-24(%rbp), %rcx
	orq	$1, %rcx
	movq	-16(%rbp), %rax
	movq	%rcx, (%rax)
	movq	-16(%rbp), %rax
	addq	-24(%rbp), %rax
	movq	%rax, -48(%rbp)
	movq	-40(%rbp), %rcx
	movq	-48(%rbp), %rax
	movq	%rcx, (%rax)
	movq	-48(%rbp), %rax
	movq	%rax, -8(%rbp)
.LBB3_3:
	movq	-8(%rbp), %rax
	addq	$48, %rsp
	popq	%rbp
	retq
.Lfunc_end3:
	.size	split_block, .Lfunc_end3-split_block
                                        # -- End function
	.p2align	4                               # -- Begin function blk_size
	.type	blk_size,@function
blk_size:                               # @blk_size
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rax
	movq	(%rax), %rax
	andq	$-16, %rax
	popq	%rbp
	retq
.Lfunc_end4:
	.size	blk_size, .Lfunc_end4-blk_size
                                        # -- End function
	.globl	coalesce_next                   # -- Begin function coalesce_next
	.p2align	4
	.type	coalesce_next,@function
coalesce_next:                          # @coalesce_next
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$48, %rsp
	movq	%rdi, -16(%rbp)
	movq	%rsi, -24(%rbp)
	movq	-16(%rbp), %rax
	movq	%rax, -40(%rbp)                 # 8-byte Spill
	movq	-16(%rbp), %rdi
	callq	blk_size
	movq	%rax, %rcx
	movq	-40(%rbp), %rax                 # 8-byte Reload
	addq	%rcx, %rax
	movq	%rax, -32(%rbp)
	movq	-32(%rbp), %rax
	cmpq	-24(%rbp), %rax
	jb	.LBB5_2
# %bb.1:
	movq	-16(%rbp), %rax
	movq	%rax, -8(%rbp)
	jmp	.LBB5_5
.LBB5_2:
	movq	-32(%rbp), %rax
	movq	(%rax), %rax
	andq	$1, %rax
	cmpq	$0, %rax
	je	.LBB5_4
# %bb.3:
	movq	-16(%rbp), %rax
	movq	%rax, -8(%rbp)
	jmp	.LBB5_5
.LBB5_4:
	movq	-16(%rbp), %rdi
	callq	blk_size
	movq	%rax, -48(%rbp)                 # 8-byte Spill
	movq	-32(%rbp), %rdi
	callq	blk_size
	movq	-48(%rbp), %rcx                 # 8-byte Reload
	addq	%rax, %rcx
	movq	-16(%rbp), %rax
	movq	%rcx, (%rax)
	movq	-16(%rbp), %rax
	movq	%rax, -8(%rbp)
.LBB5_5:
	movq	-8(%rbp), %rax
	addq	$48, %rsp
	popq	%rbp
	retq
.Lfunc_end5:
	.size	coalesce_next, .Lfunc_end5-coalesce_next
                                        # -- End function
	.globl	coalesce_prev                   # -- Begin function coalesce_prev
	.p2align	4
	.type	coalesce_prev,@function
coalesce_prev:                          # @coalesce_prev
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$64, %rsp
	movq	%rdi, -16(%rbp)
	movq	%rsi, -24(%rbp)
	movq	-16(%rbp), %rax
	cmpq	-24(%rbp), %rax
	ja	.LBB6_2
# %bb.1:
	movq	-16(%rbp), %rax
	movq	%rax, -8(%rbp)
	jmp	.LBB6_5
.LBB6_2:
	movq	-16(%rbp), %rax
	addq	$-8, %rax
	movq	%rax, -32(%rbp)
	movq	-32(%rbp), %rax
	movq	(%rax), %rax
	andq	$1, %rax
	cmpq	$0, %rax
	je	.LBB6_4
# %bb.3:
	movq	-16(%rbp), %rax
	movq	%rax, -8(%rbp)
	jmp	.LBB6_5
.LBB6_4:
	movq	-32(%rbp), %rax
	movq	(%rax), %rax
	andq	$-16, %rax
	movq	%rax, -40(%rbp)
	movq	-16(%rbp), %rax
	xorl	%ecx, %ecx
                                        # kill: def $rcx killed $ecx
	subq	-40(%rbp), %rcx
	addq	%rcx, %rax
	movq	%rax, -48(%rbp)
	movq	-48(%rbp), %rdi
	callq	blk_size
	movq	%rax, -56(%rbp)                 # 8-byte Spill
	movq	-16(%rbp), %rdi
	callq	blk_size
	movq	-56(%rbp), %rcx                 # 8-byte Reload
	addq	%rax, %rcx
	movq	-48(%rbp), %rax
	movq	%rcx, (%rax)
	movq	-48(%rbp), %rax
	movq	%rax, -8(%rbp)
.LBB6_5:
	movq	-8(%rbp), %rax
	addq	$64, %rsp
	popq	%rbp
	retq
.Lfunc_end6:
	.size	coalesce_prev, .Lfunc_end6-coalesce_prev
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym align_up
	.addrsig_sym blk_size
