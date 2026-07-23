	.file	"demo.c"
	.text
	.globl	wc_count                        # -- Begin function wc_count
	.p2align	4
	.type	wc_count,@function
wc_count:                               # @wc_count
# %bb.0:
	movl	32(%rdi), %r10d
	testq	%rdx, %rdx
	je	.LBB0_1
# %bb.3:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	xorl	%ecx, %ecx
	xorl	%r9d, %r9d
	movl	%r10d, %r11d
	xorl	%r8d, %r8d
	xorl	%eax, %eax
	jmp	.LBB0_4
	.p2align	4
.LBB0_5:                                #   in Loop: Header=BB0_4 Depth=1
	xorl	%r12d, %r12d
	xorl	%r10d, %r10d
.LBB0_8:                                #   in Loop: Header=BB0_4 Depth=1
	addq	%r12, %rax
	movb	%r15b, %r14b
	addq	%r14, %rcx
	movb	%bpl, %bl
	addq	%rbx, %r8
	incq	%r9
	movl	%r10d, %r11d
	cmpq	%r9, %rdx
	je	.LBB0_9
.LBB0_4:                                # =>This Inner Loop Header: Depth=1
	movzbl	(%rsi,%r9), %r13d
	xorl	%r14d, %r14d
	cmpb	$10, %r13b
	sete	%r15b
	xorl	%ebx, %ebx
	cmpb	$-64, %r13b
	setge	%bpl
	leal	-9(%r13), %r10d
	cmpl	$5, %r10d
	jb	.LBB0_5
# %bb.6:                                #   in Loop: Header=BB0_4 Depth=1
	movl	$0, %r12d
	movl	$0, %r10d
	cmpl	$32, %r13d
	je	.LBB0_8
# %bb.7:                                #   in Loop: Header=BB0_4 Depth=1
	xorl	%r12d, %r12d
	testl	%r11d, %r11d
	sete	%r12b
	cmpl	$1, %r11d
	adcl	$0, %r11d
	movl	%r11d, %r10d
	jmp	.LBB0_8
.LBB0_9:
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	jmp	.LBB0_2
.LBB0_1:
	xorl	%eax, %eax
	xorl	%r8d, %r8d
	xorl	%ecx, %ecx
.LBB0_2:
	addq	%rcx, (%rdi)
	addq	%rax, 8(%rdi)
	addq	%r8, 16(%rdi)
	addq	%rdx, 24(%rdi)
	movl	%r10d, 32(%rdi)
	retq
.Lfunc_end0:
	.size	wc_count, .Lfunc_end0-wc_count
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
