	.file	"demo.c"
	.text
	.globl	ring_slot                       # -- Begin function ring_slot
	.p2align	4
	.type	ring_slot,@function
ring_slot:                              # @ring_slot
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	%edi, -4(%rbp)
	movl	%esi, -8(%rbp)
	movl	-4(%rbp), %eax
	andl	-8(%rbp), %eax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	ring_slot, .Lfunc_end0-ring_slot
                                        # -- End function
	.globl	sq_space_left                   # -- Begin function sq_space_left
	.p2align	4
	.type	sq_space_left,@function
sq_space_left:                          # @sq_space_left
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rax
	movq	(%rax), %rax
	movl	(%rax), %eax
	movl	%eax, -16(%rbp)
	movl	-16(%rbp), %eax
	movl	%eax, -12(%rbp)
	movq	-8(%rbp), %rax
	movl	20(%rax), %eax
	movq	-8(%rbp), %rcx
	movl	32(%rcx), %ecx
	subl	-12(%rbp), %ecx
	subl	%ecx, %eax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	sq_space_left, .Lfunc_end1-sq_space_left
                                        # -- End function
	.globl	get_sqe                         # -- Begin function get_sqe
	.p2align	4
	.type	get_sqe,@function
get_sqe:                                # @get_sqe
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$48, %rsp
	movq	%rdi, -16(%rbp)
	movl	%esi, -20(%rbp)
	movl	%edx, -24(%rbp)
	movq	%rcx, -32(%rbp)
	movq	-16(%rbp), %rdi
	callq	sq_space_left
	cmpl	$0, %eax
	jne	.LBB2_2
# %bb.1:
	movl	$-1, -4(%rbp)
	jmp	.LBB2_3
.LBB2_2:
	movq	-16(%rbp), %rax
	movl	32(%rax), %edi
	movq	-16(%rbp), %rax
	movl	16(%rax), %esi
	callq	ring_slot
	movl	%eax, -36(%rbp)
	movq	-16(%rbp), %rax
	movq	24(%rax), %rax
	movl	-36(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	shlq	$4, %rcx
	addq	%rcx, %rax
	movq	%rax, -48(%rbp)
	movl	-20(%rbp), %ecx
	movq	-48(%rbp), %rax
	movl	%ecx, (%rax)
	movl	-24(%rbp), %ecx
	movq	-48(%rbp), %rax
	movl	%ecx, 4(%rax)
	movq	-32(%rbp), %rcx
	movq	-48(%rbp), %rax
	movq	%rcx, 8(%rax)
	movq	-16(%rbp), %rax
	movl	32(%rax), %ecx
	addl	$1, %ecx
	movl	%ecx, 32(%rax)
	movl	-36(%rbp), %eax
	movl	%eax, -4(%rbp)
.LBB2_3:
	movl	-4(%rbp), %eax
	addq	$48, %rsp
	popq	%rbp
	retq
.Lfunc_end2:
	.size	get_sqe, .Lfunc_end2-get_sqe
                                        # -- End function
	.globl	submit_one                      # -- Begin function submit_one
	.p2align	4
	.type	submit_one,@function
submit_one:                             # @submit_one
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rcx
	movq	8(%rcx), %rax
	movl	32(%rcx), %ecx
	movl	%ecx, -12(%rbp)
	movl	-12(%rbp), %ecx
	movl	%ecx, (%rax)
	popq	%rbp
	retq
.Lfunc_end3:
	.size	submit_one, .Lfunc_end3-submit_one
                                        # -- End function
	.globl	peek_cqe                        # -- Begin function peek_cqe
	.p2align	4
	.type	peek_cqe,@function
peek_cqe:                               # @peek_cqe
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$48, %rsp
	movq	%rdi, -16(%rbp)
	movq	%rsi, -24(%rbp)
	movq	-16(%rbp), %rax
	movq	(%rax), %rax
	movl	(%rax), %eax
	movl	%eax, -32(%rbp)
	movl	-32(%rbp), %eax
	movl	%eax, -28(%rbp)
	movq	-16(%rbp), %rax
	movq	8(%rax), %rax
	movl	(%rax), %eax
	movl	%eax, -40(%rbp)
	movl	-40(%rbp), %eax
	movl	%eax, -36(%rbp)
	movl	-28(%rbp), %eax
	cmpl	-36(%rbp), %eax
	jne	.LBB4_2
# %bb.1:
	movq	$0, -8(%rbp)
	jmp	.LBB4_3
.LBB4_2:
	movl	-28(%rbp), %ecx
	movq	-24(%rbp), %rax
	movl	%ecx, (%rax)
	movq	-16(%rbp), %rax
	movq	24(%rax), %rax
	movq	%rax, -48(%rbp)                 # 8-byte Spill
	movl	-28(%rbp), %edi
	movq	-16(%rbp), %rax
	movl	16(%rax), %esi
	callq	ring_slot
	movl	%eax, %ecx
	movq	-48(%rbp), %rax                 # 8-byte Reload
	movl	%ecx, %ecx
                                        # kill: def $rcx killed $ecx
	shlq	$4, %rcx
	addq	%rcx, %rax
	movq	%rax, -8(%rbp)
.LBB4_3:
	movq	-8(%rbp), %rax
	addq	$48, %rsp
	popq	%rbp
	retq
.Lfunc_end4:
	.size	peek_cqe, .Lfunc_end4-peek_cqe
                                        # -- End function
	.globl	cq_advance                      # -- Begin function cq_advance
	.p2align	4
	.type	cq_advance,@function
cq_advance:                             # @cq_advance
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movq	-8(%rbp), %rax
	movq	(%rax), %rax
	movl	(%rax), %eax
	movl	%eax, -20(%rbp)
	movl	-20(%rbp), %eax
	movl	%eax, -16(%rbp)
	movq	-8(%rbp), %rax
	movq	(%rax), %rax
	movl	-16(%rbp), %ecx
	movl	-12(%rbp), %edx
	addl	%edx, %ecx
	movl	%ecx, -24(%rbp)
	movl	-24(%rbp), %ecx
	movl	%ecx, (%rax)
	popq	%rbp
	retq
.Lfunc_end5:
	.size	cq_advance, .Lfunc_end5-cq_advance
                                        # -- End function
	.globl	buf_ring_advance                # -- Begin function buf_ring_advance
	.p2align	4
	.type	buf_ring_advance,@function
buf_ring_advance:                       # @buf_ring_advance
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movq	-8(%rbp), %rax
	movl	(%rax), %eax
	movl	%eax, -20(%rbp)
	movl	-20(%rbp), %eax
	movl	-12(%rbp), %ecx
	addl	%ecx, %eax
	movl	%eax, -16(%rbp)
	movq	-8(%rbp), %rax
	movl	-16(%rbp), %ecx
	movl	%ecx, -24(%rbp)
	movl	-24(%rbp), %ecx
	movl	%ecx, (%rax)
	popq	%rbp
	retq
.Lfunc_end6:
	.size	buf_ring_advance, .Lfunc_end6-buf_ring_advance
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym ring_slot
	.addrsig_sym sq_space_left
