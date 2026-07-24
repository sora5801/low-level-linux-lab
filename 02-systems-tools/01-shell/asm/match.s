	.file	"match.c"
	.text
	.globl	wildcard_match                  # -- Begin function wildcard_match
	.p2align	4
	.type	wildcard_match,@function
wildcard_match:                         # @wildcard_match
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	xorl	%r8d, %r8d
	movl	$-1, %eax
	movl	$1, %ecx
                                        # implicit-def: $rdx
	xorl	%r9d, %r9d
	jmp	.LBB0_3
.LBB0_1:                                #   in Loop: Header=BB0_3 Depth=1
	incq	%rdi
	movb	$1, %r10b
	movq	%rdi, %r8
	movq	%rsi, %r9
	.p2align	4
.LBB0_2:                                #   in Loop: Header=BB0_3 Depth=1
	testb	%r10b, %r10b
	je	.LBB0_40
.LBB0_3:                                # =>This Loop Header: Depth=1
                                        #     Child Loop BB0_21 Depth 2
	movzbl	(%rsi), %r10d
	testb	%r10b, %r10b
	je	.LBB0_38
# %bb.4:                                #   in Loop: Header=BB0_3 Depth=1
	movzbl	(%rdi), %r11d
	movzbl	%r11b, %ebx
	cmpl	$90, %ebx
	jg	.LBB0_9
# %bb.5:                                #   in Loop: Header=BB0_3 Depth=1
	cmpl	$42, %ebx
	je	.LBB0_1
# %bb.6:                                #   in Loop: Header=BB0_3 Depth=1
	cmpl	$63, %ebx
	je	.LBB0_8
	jmp	.LBB0_7
	.p2align	4
.LBB0_9:                                #   in Loop: Header=BB0_3 Depth=1
	cmpl	$91, %ebx
	je	.LBB0_17
# %bb.10:                               #   in Loop: Header=BB0_3 Depth=1
	cmpl	$92, %ebx
	jne	.LBB0_7
# %bb.11:                               #   in Loop: Header=BB0_3 Depth=1
	movzbl	1(%rdi), %ebx
	testb	%bl, %bl
	je	.LBB0_7
# %bb.12:                               #   in Loop: Header=BB0_3 Depth=1
	cmpb	%r10b, %bl
	jne	.LBB0_15
# %bb.13:                               #   in Loop: Header=BB0_3 Depth=1
	addq	$2, %rdi
	jmp	.LBB0_14
	.p2align	4
.LBB0_7:                                #   in Loop: Header=BB0_3 Depth=1
	cmpb	%r10b, %r11b
	jne	.LBB0_15
.LBB0_8:                                #   in Loop: Header=BB0_3 Depth=1
	incq	%rdi
.LBB0_14:                               #   in Loop: Header=BB0_3 Depth=1
	incq	%rsi
	movb	$1, %r10b
	jmp	.LBB0_2
.LBB0_15:                               #   in Loop: Header=BB0_3 Depth=1
	testq	%r8, %r8
	jne	.LBB0_37
# %bb.16:                               #   in Loop: Header=BB0_3 Depth=1
	xorl	%r8d, %r8d
	xorl	%r10d, %r10d
	jmp	.LBB0_2
.LBB0_17:                               #   in Loop: Header=BB0_3 Depth=1
	leaq	1(%rdi), %r11
	movzbl	1(%rdi), %r15d
	cmpl	$94, %r15d
	je	.LBB0_19
# %bb.18:                               #   in Loop: Header=BB0_3 Depth=1
	movb	$1, %r14b
	movq	%r11, %rbx
	cmpl	$33, %r15d
	jne	.LBB0_20
.LBB0_19:                               #   in Loop: Header=BB0_3 Depth=1
	leaq	2(%rdi), %rbx
	xorl	%r14d, %r14d
.LBB0_20:                               #   in Loop: Header=BB0_3 Depth=1
	movzbl	(%rbx), %r12d
	xorl	%r15d, %r15d
	testb	%r12b, %r12b
	je	.LBB0_28
	.p2align	4
.LBB0_21:                               #   Parent Loop BB0_3 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	cmpb	$45, 1(%rbx)
	jne	.LBB0_24
# %bb.22:                               #   in Loop: Header=BB0_21 Depth=2
	movzbl	2(%rbx), %r13d
	testl	%r13d, %r13d
	je	.LBB0_24
# %bb.23:                               #   in Loop: Header=BB0_21 Depth=2
	cmpl	$93, %r13d
	jne	.LBB0_42
	.p2align	4
.LBB0_24:                               #   in Loop: Header=BB0_21 Depth=2
	incq	%rbx
	cmpb	%r10b, %r12b
	cmovel	%ecx, %r15d
.LBB0_25:                               #   in Loop: Header=BB0_21 Depth=2
	movzbl	(%rbx), %r12d
	cmpl	$93, %r12d
	je	.LBB0_27
# %bb.26:                               #   in Loop: Header=BB0_21 Depth=2
	testl	%r12d, %r12d
	jne	.LBB0_21
	jmp	.LBB0_27
.LBB0_42:                               #   in Loop: Header=BB0_21 Depth=2
	cmpb	%r13b, %r10b
	movl	$1, %r13d
	cmoval	%r15d, %r13d
	cmpb	%r10b, %r12b
	cmovbel	%r13d, %r15d
	addq	$3, %rbx
	jmp	.LBB0_25
.LBB0_27:                               #   in Loop: Header=BB0_3 Depth=1
	cmpb	$93, %r12b
	sete	%r12b
	jmp	.LBB0_29
.LBB0_28:                               #   in Loop: Header=BB0_3 Depth=1
	xorl	%r12d, %r12d
.LBB0_29:                               #   in Loop: Header=BB0_3 Depth=1
	incq	%rbx
	xorl	%r13d, %r13d
	testl	%r15d, %r15d
	sete	%r13b
	testb	%r14b, %r14b
	cmovnel	%r15d, %r13d
	testb	%r12b, %r12b
	cmovneq	%rbx, %rdx
	cmovel	%eax, %r13d
	testl	%r13d, %r13d
	je	.LBB0_32
# %bb.30:                               #   in Loop: Header=BB0_3 Depth=1
	cmpl	$1, %r13d
	jne	.LBB0_34
# %bb.31:                               #   in Loop: Header=BB0_3 Depth=1
	incq	%rsi
	movb	$1, %r10b
	movq	%rdx, %rdi
	jmp	.LBB0_2
.LBB0_34:                               #   in Loop: Header=BB0_3 Depth=1
	cmpb	$91, %r10b
	jne	.LBB0_32
# %bb.35:                               #   in Loop: Header=BB0_3 Depth=1
	incq	%rsi
	movb	$1, %r10b
	movq	%r11, %rdi
	jmp	.LBB0_2
.LBB0_32:                               #   in Loop: Header=BB0_3 Depth=1
	testq	%r8, %r8
	je	.LBB0_33
.LBB0_37:                               #   in Loop: Header=BB0_3 Depth=1
	incq	%r9
	movb	$1, %r10b
	movq	%r9, %rsi
	movq	%r8, %rdi
	jmp	.LBB0_2
.LBB0_33:                               #   in Loop: Header=BB0_3 Depth=1
	xorl	%r10d, %r10d
	jmp	.LBB0_2
	.p2align	4
.LBB0_38:                               # =>This Inner Loop Header: Depth=1
	movzbl	(%rdi), %ecx
	incq	%rdi
	cmpb	$42, %cl
	je	.LBB0_38
# %bb.39:
	xorl	%eax, %eax
	testb	%cl, %cl
	sete	%al
	jmp	.LBB0_41
.LBB0_40:
	xorl	%eax, %eax
.LBB0_41:
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
.Lfunc_end0:
	.size	wildcard_match, .Lfunc_end0-wildcard_match
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
