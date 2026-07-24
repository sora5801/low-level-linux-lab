	.file	"demo.c"
	.text
	.globl	ring_slot                       # -- Begin function ring_slot
	.p2align	4
	.type	ring_slot,@function
ring_slot:                              # @ring_slot
# %bb.0:
	movl	%edi, %eax
	andl	%esi, %eax
	retq
.Lfunc_end0:
	.size	ring_slot, .Lfunc_end0-ring_slot
                                        # -- End function
	.globl	sq_space_left                   # -- Begin function sq_space_left
	.p2align	4
	.type	sq_space_left,@function
sq_space_left:                          # @sq_space_left
# %bb.0:
	movq	(%rdi), %rax
	movl	(%rax), %eax
	addl	20(%rdi), %eax
	subl	32(%rdi), %eax
	retq
.Lfunc_end1:
	.size	sq_space_left, .Lfunc_end1-sq_space_left
                                        # -- End function
	.globl	get_sqe                         # -- Begin function get_sqe
	.p2align	4
	.type	get_sqe,@function
get_sqe:                                # @get_sqe
# %bb.0:
	movq	(%rdi), %rax
	movl	(%rax), %r9d
	movl	32(%rdi), %r8d
	addl	20(%rdi), %r9d
	movl	$-1, %eax
	cmpl	%r8d, %r9d
	je	.LBB2_2
# %bb.1:
	movl	16(%rdi), %eax
	andl	%r8d, %eax
	movq	24(%rdi), %r9
	movq	%rax, %r10
	shlq	$4, %r10
	movl	%esi, (%r9,%r10)
	movl	%edx, 4(%r9,%r10)
	movq	%rcx, 8(%r9,%r10)
	incl	%r8d
	movl	%r8d, 32(%rdi)
.LBB2_2:
                                        # kill: def $eax killed $eax killed $rax
	retq
.Lfunc_end2:
	.size	get_sqe, .Lfunc_end2-get_sqe
                                        # -- End function
	.globl	submit_one                      # -- Begin function submit_one
	.p2align	4
	.type	submit_one,@function
submit_one:                             # @submit_one
# %bb.0:
	movq	8(%rdi), %rax
	movl	32(%rdi), %ecx
	movl	%ecx, (%rax)
	retq
.Lfunc_end3:
	.size	submit_one, .Lfunc_end3-submit_one
                                        # -- End function
	.globl	peek_cqe                        # -- Begin function peek_cqe
	.p2align	4
	.type	peek_cqe,@function
peek_cqe:                               # @peek_cqe
# %bb.0:
	movq	(%rdi), %rax
	movl	(%rax), %eax
	movq	8(%rdi), %rcx
	movl	(%rcx), %ecx
	cmpl	%ecx, %eax
	jne	.LBB4_2
# %bb.1:
	xorl	%eax, %eax
	retq
.LBB4_2:
	movl	%eax, (%rsi)
	andl	16(%rdi), %eax
	shlq	$4, %rax
	addq	24(%rdi), %rax
	retq
.Lfunc_end4:
	.size	peek_cqe, .Lfunc_end4-peek_cqe
                                        # -- End function
	.globl	cq_advance                      # -- Begin function cq_advance
	.p2align	4
	.type	cq_advance,@function
cq_advance:                             # @cq_advance
# %bb.0:
	movq	(%rdi), %rax
	addl	%esi, (%rax)
	retq
.Lfunc_end5:
	.size	cq_advance, .Lfunc_end5-cq_advance
                                        # -- End function
	.globl	buf_ring_advance                # -- Begin function buf_ring_advance
	.p2align	4
	.type	buf_ring_advance,@function
buf_ring_advance:                       # @buf_ring_advance
# %bb.0:
	addl	%esi, (%rdi)
	retq
.Lfunc_end6:
	.size	buf_ring_advance, .Lfunc_end6-buf_ring_advance
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
