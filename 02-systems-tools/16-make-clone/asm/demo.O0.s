	.file	"demo.c"
	.text
	.globl	mk_needs_rebuild                # -- Begin function mk_needs_rebuild
	.p2align	4
	.type	mk_needs_rebuild,@function
mk_needs_rebuild:                       # @mk_needs_rebuild
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	16(%rbp), %eax
	movl	%edi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movl	%edx, -16(%rbp)
	movq	%rcx, -24(%rbp)
	movq	%r8, -32(%rbp)
	movq	%r9, -40(%rbp)
	cmpl	$0, -8(%rbp)
	jne	.LBB0_3
# %bb.1:
	cmpl	$0, -12(%rbp)
	jne	.LBB0_3
# %bb.2:
	cmpl	$0, -16(%rbp)
	je	.LBB0_4
.LBB0_3:
	movl	$1, -4(%rbp)
	jmp	.LBB0_13
.LBB0_4:
	movl	$0, -44(%rbp)
.LBB0_5:                                # =>This Inner Loop Header: Depth=1
	movl	-44(%rbp), %eax
	cmpl	16(%rbp), %eax
	jge	.LBB0_12
# %bb.6:                                #   in Loop: Header=BB0_5 Depth=1
	movq	-40(%rbp), %rax
	movslq	-44(%rbp), %rcx
	cmpl	$0, (%rax,%rcx,4)
	je	.LBB0_8
# %bb.7:
	movl	$1, -4(%rbp)
	jmp	.LBB0_13
.LBB0_8:                                #   in Loop: Header=BB0_5 Depth=1
	movq	-32(%rbp), %rax
	movslq	-44(%rbp), %rcx
	movq	(%rax,%rcx,8), %rax
	cmpq	-24(%rbp), %rax
	jle	.LBB0_10
# %bb.9:
	movl	$1, -4(%rbp)
	jmp	.LBB0_13
.LBB0_10:                               #   in Loop: Header=BB0_5 Depth=1
	jmp	.LBB0_11
.LBB0_11:                               #   in Loop: Header=BB0_5 Depth=1
	movl	-44(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -44(%rbp)
	jmp	.LBB0_5
.LBB0_12:
	movl	$0, -4(%rbp)
.LBB0_13:
	movl	-4(%rbp), %eax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	mk_needs_rebuild, .Lfunc_end0-mk_needs_rebuild
                                        # -- End function
	.globl	mk_toposort                     # -- Begin function mk_toposort
	.p2align	4
	.type	mk_toposort,@function
mk_toposort:                            # @mk_toposort
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$80, %rsp
	movl	%edi, -4(%rbp)
	movq	%rsi, -16(%rbp)
	movq	%rdx, -24(%rbp)
	movl	$0, -172(%rbp)
	movl	$0, -164(%rbp)
.LBB1_1:                                # =>This Loop Header: Depth=1
                                        #     Child Loop BB1_3 Depth 2
	movl	-164(%rbp), %eax
	cmpl	-4(%rbp), %eax
	jge	.LBB1_8
# %bb.2:                                #   in Loop: Header=BB1_1 Depth=1
	movl	$0, -176(%rbp)
	movq	-16(%rbp), %rax
	movslq	-164(%rbp), %rcx
	movslq	-4(%rbp), %rdx
	imulq	%rdx, %rcx
	addq	%rcx, %rax
	movq	%rax, -184(%rbp)
	movl	$0, -168(%rbp)
.LBB1_3:                                #   Parent Loop BB1_1 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movl	-168(%rbp), %eax
	cmpl	-4(%rbp), %eax
	jge	.LBB1_6
# %bb.4:                                #   in Loop: Header=BB1_3 Depth=2
	movq	-184(%rbp), %rax
	movslq	-168(%rbp), %rcx
	movzbl	(%rax,%rcx), %eax
	cmpl	$0, %eax
	setne	%al
	andb	$1, %al
	movzbl	%al, %eax
	addl	-176(%rbp), %eax
	movl	%eax, -176(%rbp)
# %bb.5:                                #   in Loop: Header=BB1_3 Depth=2
	movl	-168(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -168(%rbp)
	jmp	.LBB1_3
.LBB1_6:                                #   in Loop: Header=BB1_1 Depth=1
	movl	-176(%rbp), %eax
	movb	%al, %cl
	movslq	-164(%rbp), %rax
	movb	%cl, -96(%rbp,%rax)
	movslq	-164(%rbp), %rax
	movb	$0, -160(%rbp,%rax)
# %bb.7:                                #   in Loop: Header=BB1_1 Depth=1
	movl	-164(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -164(%rbp)
	jmp	.LBB1_1
.LBB1_8:
	movl	$0, -164(%rbp)
.LBB1_9:                                # =>This Loop Header: Depth=1
                                        #     Child Loop BB1_11 Depth 2
                                        #     Child Loop BB1_20 Depth 2
	movl	-164(%rbp), %eax
	cmpl	-4(%rbp), %eax
	jge	.LBB1_29
# %bb.10:                               #   in Loop: Header=BB1_9 Depth=1
	movl	$-1, -188(%rbp)
	movl	$0, -168(%rbp)
.LBB1_11:                               #   Parent Loop BB1_9 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movl	-168(%rbp), %eax
	cmpl	-4(%rbp), %eax
	jge	.LBB1_17
# %bb.12:                               #   in Loop: Header=BB1_11 Depth=2
	movslq	-168(%rbp), %rax
	cmpb	$0, -160(%rbp,%rax)
	jne	.LBB1_15
# %bb.13:                               #   in Loop: Header=BB1_11 Depth=2
	movslq	-168(%rbp), %rax
	movzbl	-96(%rbp,%rax), %eax
	cmpl	$0, %eax
	jne	.LBB1_15
# %bb.14:                               #   in Loop: Header=BB1_9 Depth=1
	movl	-168(%rbp), %eax
	movl	%eax, -188(%rbp)
	jmp	.LBB1_17
.LBB1_15:                               #   in Loop: Header=BB1_11 Depth=2
	jmp	.LBB1_16
.LBB1_16:                               #   in Loop: Header=BB1_11 Depth=2
	movl	-168(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -168(%rbp)
	jmp	.LBB1_11
.LBB1_17:                               #   in Loop: Header=BB1_9 Depth=1
	cmpl	$0, -188(%rbp)
	jge	.LBB1_19
# %bb.18:
	jmp	.LBB1_29
.LBB1_19:                               #   in Loop: Header=BB1_9 Depth=1
	movl	-188(%rbp), %edx
	movq	-24(%rbp), %rax
	movl	-172(%rbp), %ecx
	movl	%ecx, %esi
	addl	$1, %esi
	movl	%esi, -172(%rbp)
	movslq	%ecx, %rcx
	movl	%edx, (%rax,%rcx,4)
	movslq	-188(%rbp), %rax
	movb	$1, -160(%rbp,%rax)
	movl	$0, -168(%rbp)
.LBB1_20:                               #   Parent Loop BB1_9 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movl	-168(%rbp), %eax
	cmpl	-4(%rbp), %eax
	jge	.LBB1_27
# %bb.21:                               #   in Loop: Header=BB1_20 Depth=2
	movq	-16(%rbp), %rax
	movslq	-168(%rbp), %rcx
	movslq	-4(%rbp), %rdx
	imulq	%rdx, %rcx
	addq	%rcx, %rax
	movq	%rax, -200(%rbp)
	movslq	-168(%rbp), %rax
	cmpb	$0, -160(%rbp,%rax)
	jne	.LBB1_25
# %bb.22:                               #   in Loop: Header=BB1_20 Depth=2
	movq	-200(%rbp), %rax
	movslq	-188(%rbp), %rcx
	movzbl	(%rax,%rcx), %eax
	cmpl	$0, %eax
	je	.LBB1_25
# %bb.23:                               #   in Loop: Header=BB1_20 Depth=2
	movslq	-168(%rbp), %rax
	movzbl	-96(%rbp,%rax), %eax
	cmpl	$0, %eax
	jle	.LBB1_25
# %bb.24:                               #   in Loop: Header=BB1_20 Depth=2
	movslq	-168(%rbp), %rax
	movb	-96(%rbp,%rax), %cl
	addb	$-1, %cl
	movb	%cl, -96(%rbp,%rax)
.LBB1_25:                               #   in Loop: Header=BB1_20 Depth=2
	jmp	.LBB1_26
.LBB1_26:                               #   in Loop: Header=BB1_20 Depth=2
	movl	-168(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -168(%rbp)
	jmp	.LBB1_20
.LBB1_27:                               #   in Loop: Header=BB1_9 Depth=1
	jmp	.LBB1_28
.LBB1_28:                               #   in Loop: Header=BB1_9 Depth=1
	movl	-164(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -164(%rbp)
	jmp	.LBB1_9
.LBB1_29:
	movl	-172(%rbp), %eax
	addq	$80, %rsp
	popq	%rbp
	retq
.Lfunc_end1:
	.size	mk_toposort, .Lfunc_end1-mk_toposort
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
