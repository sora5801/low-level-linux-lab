	.file	"demo.c"
	.text
	.globl	wc_count                        # -- Begin function wc_count
	.p2align	4
	.type	wc_count,@function
wc_count:                               # @wc_count
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	%rdi, -48(%rbp)                 # 8-byte Spill
	movl	32(%rdi), %eax
	testq	%rdx, %rdx
	je	.LBB0_1
# %bb.3:
	xorl	%r9d, %r9d
	xorl	%r11d, %r11d
	xorl	%ecx, %ecx
	xorl	%r8d, %r8d
	jmp	.LBB0_4
	.p2align	4
.LBB0_7:                                #   in Loop: Header=BB0_4 Depth=1
	movb	%r13b, %r15b
	addq	%r15, %r9
	movb	%r12b, %r14b
	addq	%r14, %rcx
	testl	%eax, %eax
	sete	%dil
	cmpl	$1, %eax
	adcl	$0, %eax
	andb	%bl, %dil
	movzbl	%dil, %edi
	addq	%rdi, %r8
	testb	%bl, %bl
	movl	$0, %edi
	cmovel	%edi, %eax
	incq	%r11
	cmpq	%r11, %rdx
	je	.LBB0_2
.LBB0_4:                                # =>This Inner Loop Header: Depth=1
	movzbl	(%rsi,%r11), %r10d
	xorl	%r15d, %r15d
	cmpb	$10, %r10b
	sete	%r13b
	xorl	%r14d, %r14d
	cmpb	$-64, %r10b
	setge	%r12b
	xorl	%ebx, %ebx
	leal	-9(%r10), %edi
	cmpl	$5, %edi
	jb	.LBB0_7
# %bb.5:                                #   in Loop: Header=BB0_4 Depth=1
	cmpl	$32, %r10d
	je	.LBB0_7
# %bb.6:                                #   in Loop: Header=BB0_4 Depth=1
	movb	$1, %bl
	jmp	.LBB0_7
.LBB0_1:
	xorl	%r8d, %r8d
	xorl	%ecx, %ecx
	xorl	%r9d, %r9d
.LBB0_2:
	movq	-48(%rbp), %rsi                 # 8-byte Reload
	addq	%r9, (%rsi)
	addq	%r8, 8(%rsi)
	addq	%rcx, 16(%rsi)
	addq	%rdx, 24(%rsi)
	movl	%eax, 32(%rsi)
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
.Lfunc_end0:
	.size	wc_count, .Lfunc_end0-wc_count
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
