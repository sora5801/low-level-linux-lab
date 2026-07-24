	.file	"demo.c"
	.text
	.globl	fnv1a_32                        # -- Begin function fnv1a_32
	.p2align	4
	.type	fnv1a_32,@function
fnv1a_32:                               # @fnv1a_32
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movl	$-2128831035, -20(%rbp)         # imm = 0x811C9DC5
	movq	$0, -32(%rbp)
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	movq	-32(%rbp), %rax
	cmpq	-16(%rbp), %rax
	jae	.LBB0_4
# %bb.2:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-8(%rbp), %rax
	movq	-32(%rbp), %rcx
	movzbl	(%rax,%rcx), %eax
	xorl	-20(%rbp), %eax
	movl	%eax, -20(%rbp)
	imull	$16777619, -20(%rbp), %eax      # imm = 0x1000193
	movl	%eax, -20(%rbp)
# %bb.3:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-32(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -32(%rbp)
	jmp	.LBB0_1
.LBB0_4:
	movl	-20(%rbp), %eax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	fnv1a_32, .Lfunc_end0-fnv1a_32
                                        # -- End function
	.globl	ring_lookup                     # -- Begin function ring_lookup
	.p2align	4
	.type	ring_lookup,@function
ring_lookup:                            # @ring_lookup
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -16(%rbp)
	movl	%esi, -20(%rbp)
	movl	%edx, -24(%rbp)
	cmpl	$0, -20(%rbp)
	jg	.LBB1_2
# %bb.1:
	movl	$-1, -4(%rbp)
	jmp	.LBB1_11
.LBB1_2:
	movl	$0, -28(%rbp)
	movl	-20(%rbp), %eax
	movl	%eax, -32(%rbp)
.LBB1_3:                                # =>This Inner Loop Header: Depth=1
	movl	-28(%rbp), %eax
	cmpl	-32(%rbp), %eax
	jge	.LBB1_8
# %bb.4:                                #   in Loop: Header=BB1_3 Depth=1
	movl	-28(%rbp), %eax
	movl	-32(%rbp), %ecx
	subl	-28(%rbp), %ecx
	sarl	%ecx
	addl	%ecx, %eax
	movl	%eax, -36(%rbp)
	movq	-16(%rbp), %rax
	movslq	-36(%rbp), %rcx
	movl	(%rax,%rcx,8), %eax
	cmpl	-24(%rbp), %eax
	jae	.LBB1_6
# %bb.5:                                #   in Loop: Header=BB1_3 Depth=1
	movl	-36(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -28(%rbp)
	jmp	.LBB1_7
.LBB1_6:                                #   in Loop: Header=BB1_3 Depth=1
	movl	-36(%rbp), %eax
	movl	%eax, -32(%rbp)
.LBB1_7:                                #   in Loop: Header=BB1_3 Depth=1
	jmp	.LBB1_3
.LBB1_8:
	movl	-28(%rbp), %eax
	cmpl	-20(%rbp), %eax
	jne	.LBB1_10
# %bb.9:
	movl	$0, -28(%rbp)
.LBB1_10:
	movq	-16(%rbp), %rax
	movslq	-28(%rbp), %rcx
	movl	4(%rax,%rcx,8), %eax
	movl	%eax, -4(%rbp)
.LBB1_11:
	movl	-4(%rbp), %eax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	ring_lookup, .Lfunc_end1-ring_lookup
                                        # -- End function
	.globl	least_conn_pick                 # -- Begin function least_conn_pick
	.p2align	4
	.type	least_conn_pick,@function
least_conn_pick:                        # @least_conn_pick
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movl	%edx, -20(%rbp)
	movl	$-1, -24(%rbp)
	movl	$0, -28(%rbp)
	movl	$0, -32(%rbp)
.LBB2_1:                                # =>This Inner Loop Header: Depth=1
	movl	-32(%rbp), %eax
	cmpl	-20(%rbp), %eax
	jge	.LBB2_9
# %bb.2:                                #   in Loop: Header=BB2_1 Depth=1
	movq	-16(%rbp), %rax
	movslq	-32(%rbp), %rcx
	cmpb	$0, (%rax,%rcx)
	jne	.LBB2_4
# %bb.3:                                #   in Loop: Header=BB2_1 Depth=1
	jmp	.LBB2_8
.LBB2_4:                                #   in Loop: Header=BB2_1 Depth=1
	cmpl	$0, -24(%rbp)
	jl	.LBB2_6
# %bb.5:                                #   in Loop: Header=BB2_1 Depth=1
	movq	-8(%rbp), %rax
	movslq	-32(%rbp), %rcx
	movl	(%rax,%rcx,4), %eax
	cmpl	-28(%rbp), %eax
	jge	.LBB2_7
.LBB2_6:                                #   in Loop: Header=BB2_1 Depth=1
	movl	-32(%rbp), %eax
	movl	%eax, -24(%rbp)
	movq	-8(%rbp), %rax
	movslq	-32(%rbp), %rcx
	movl	(%rax,%rcx,4), %eax
	movl	%eax, -28(%rbp)
.LBB2_7:                                #   in Loop: Header=BB2_1 Depth=1
	jmp	.LBB2_8
.LBB2_8:                                #   in Loop: Header=BB2_1 Depth=1
	movl	-32(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -32(%rbp)
	jmp	.LBB2_1
.LBB2_9:
	movl	-24(%rbp), %eax
	popq	%rbp
	retq
.Lfunc_end2:
	.size	least_conn_pick, .Lfunc_end2-least_conn_pick
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
