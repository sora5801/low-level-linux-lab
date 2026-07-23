	.file	"demo.c"
	.text
	.globl	backoff_delay_ms                # -- Begin function backoff_delay_ms
	.p2align	4
	.type	backoff_delay_ms,@function
backoff_delay_ms:                       # @backoff_delay_ms
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	%edi, -12(%rbp)
	movq	%rsi, -24(%rbp)
	movq	%rdx, -32(%rbp)
	movq	-24(%rbp), %rax
	movq	%rax, -40(%rbp)
	movl	-12(%rbp), %eax
	movl	%eax, -44(%rbp)
	cmpl	$63, -44(%rbp)
	jbe	.LBB0_2
# %bb.1:
	movl	$63, -44(%rbp)
.LBB0_2:
	movl	$0, -48(%rbp)
.LBB0_3:                                # =>This Inner Loop Header: Depth=1
	movl	-48(%rbp), %eax
	cmpl	-44(%rbp), %eax
	jae	.LBB0_8
# %bb.4:                                #   in Loop: Header=BB0_3 Depth=1
	movq	-40(%rbp), %rax
	cmpq	-32(%rbp), %rax
	jb	.LBB0_6
# %bb.5:
	movq	-32(%rbp), %rax
	movq	%rax, -8(%rbp)
	jmp	.LBB0_11
.LBB0_6:                                #   in Loop: Header=BB0_3 Depth=1
	movq	-40(%rbp), %rax
	shlq	%rax
	movq	%rax, -40(%rbp)
# %bb.7:                                #   in Loop: Header=BB0_3 Depth=1
	movl	-48(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -48(%rbp)
	jmp	.LBB0_3
.LBB0_8:
	movq	-40(%rbp), %rax
	cmpq	-32(%rbp), %rax
	jbe	.LBB0_10
# %bb.9:
	movq	-32(%rbp), %rax
	movq	%rax, -40(%rbp)
.LBB0_10:
	movq	-40(%rbp), %rax
	movq	%rax, -8(%rbp)
.LBB0_11:
	movq	-8(%rbp), %rax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	backoff_delay_ms, .Lfunc_end0-backoff_delay_ms
                                        # -- End function
	.globl	toposort                        # -- Begin function toposort
	.p2align	4
	.type	toposort,@function
toposort:                               # @toposort
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
	.size	toposort, .Lfunc_end1-toposort
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
