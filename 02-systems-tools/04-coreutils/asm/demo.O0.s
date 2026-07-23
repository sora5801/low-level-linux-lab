	.file	"demo.c"
	.text
	.globl	wc_count                        # -- Begin function wc_count
	.p2align	4
	.type	wc_count,@function
wc_count:                               # @wc_count
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$80, %rsp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	%rdx, -24(%rbp)
	movq	$0, -32(%rbp)
	movq	$0, -40(%rbp)
	movq	$0, -48(%rbp)
	movq	-8(%rbp), %rax
	movl	32(%rax), %eax
	movl	%eax, -52(%rbp)
	movq	$0, -64(%rbp)
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	movq	-64(%rbp), %rax
	cmpq	-24(%rbp), %rax
	jae	.LBB0_13
# %bb.2:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rax
	movq	-64(%rbp), %rcx
	movb	(%rax,%rcx), %al
	movb	%al, -65(%rbp)
	movzbl	-65(%rbp), %eax
	cmpl	$10, %eax
	jne	.LBB0_4
# %bb.3:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-32(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -32(%rbp)
.LBB0_4:                                #   in Loop: Header=BB0_1 Depth=1
	movzbl	-65(%rbp), %eax
	andl	$192, %eax
	cmpl	$128, %eax
	je	.LBB0_6
# %bb.5:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-48(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -48(%rbp)
.LBB0_6:                                #   in Loop: Header=BB0_1 Depth=1
	movzbl	-65(%rbp), %edi
	callq	wc_is_space
	cmpl	$0, %eax
	je	.LBB0_8
# %bb.7:                                #   in Loop: Header=BB0_1 Depth=1
	movl	$0, -52(%rbp)
	jmp	.LBB0_11
.LBB0_8:                                #   in Loop: Header=BB0_1 Depth=1
	cmpl	$0, -52(%rbp)
	jne	.LBB0_10
# %bb.9:                                #   in Loop: Header=BB0_1 Depth=1
	movl	$1, -52(%rbp)
	movq	-40(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -40(%rbp)
.LBB0_10:                               #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_11
.LBB0_11:                               #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_12
.LBB0_12:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-64(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -64(%rbp)
	jmp	.LBB0_1
.LBB0_13:
	movq	-32(%rbp), %rcx
	movq	-8(%rbp), %rax
	addq	(%rax), %rcx
	movq	%rcx, (%rax)
	movq	-40(%rbp), %rcx
	movq	-8(%rbp), %rax
	addq	8(%rax), %rcx
	movq	%rcx, 8(%rax)
	movq	-48(%rbp), %rcx
	movq	-8(%rbp), %rax
	addq	16(%rax), %rcx
	movq	%rcx, 16(%rax)
	movq	-24(%rbp), %rcx
	movq	-8(%rbp), %rax
	addq	24(%rax), %rcx
	movq	%rcx, 24(%rax)
	movl	-52(%rbp), %ecx
	movq	-8(%rbp), %rax
	movl	%ecx, 32(%rax)
	addq	$80, %rsp
	popq	%rbp
	retq
.Lfunc_end0:
	.size	wc_count, .Lfunc_end0-wc_count
                                        # -- End function
	.p2align	4                               # -- Begin function wc_is_space
	.type	wc_is_space,@function
wc_is_space:                            # @wc_is_space
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movb	%dil, %al
	movb	%al, -1(%rbp)
	movzbl	-1(%rbp), %ecx
	movb	$1, %al
	cmpl	$32, %ecx
	movb	%al, -2(%rbp)                   # 1-byte Spill
	je	.LBB1_6
# %bb.1:
	movzbl	-1(%rbp), %ecx
	movb	$1, %al
	cmpl	$9, %ecx
	movb	%al, -2(%rbp)                   # 1-byte Spill
	je	.LBB1_6
# %bb.2:
	movzbl	-1(%rbp), %ecx
	movb	$1, %al
	cmpl	$10, %ecx
	movb	%al, -2(%rbp)                   # 1-byte Spill
	je	.LBB1_6
# %bb.3:
	movzbl	-1(%rbp), %ecx
	movb	$1, %al
	cmpl	$13, %ecx
	movb	%al, -2(%rbp)                   # 1-byte Spill
	je	.LBB1_6
# %bb.4:
	movzbl	-1(%rbp), %ecx
	movb	$1, %al
	cmpl	$11, %ecx
	movb	%al, -2(%rbp)                   # 1-byte Spill
	je	.LBB1_6
# %bb.5:
	movzbl	-1(%rbp), %eax
	cmpl	$12, %eax
	sete	%al
	movb	%al, -2(%rbp)                   # 1-byte Spill
.LBB1_6:
	movb	-2(%rbp), %al                   # 1-byte Reload
	andb	$1, %al
	movzbl	%al, %eax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	wc_is_space, .Lfunc_end1-wc_is_space
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym wc_is_space
