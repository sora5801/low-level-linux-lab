	.file	"match.c"
	.text
	.globl	wildcard_match                  # -- Begin function wildcard_match
	.p2align	4
	.type	wildcard_match,@function
wildcard_match:                         # @wildcard_match
# %bb.0:
	pushq	%rbp
	pushq	%r14
	pushq	%rbx
	movzbl	(%rsi), %r8d
	testb	%r8b, %r8b
	je	.LBB0_33
# %bb.1:
	xorl	%ecx, %ecx
	movl	$1, %eax
	xorl	%edx, %edx
	jmp	.LBB0_5
.LBB0_2:                                #   in Loop: Header=BB0_5 Depth=1
	incq	%rdi
	movq	%rdi, %rdx
	movq	%rsi, %rcx
.LBB0_3:                                #   in Loop: Header=BB0_5 Depth=1
	movq	%rdi, %r9
.LBB0_4:                                #   in Loop: Header=BB0_5 Depth=1
	movzbl	(%rsi), %r8d
	movq	%r9, %rdi
	testb	%r8b, %r8b
	je	.LBB0_34
.LBB0_5:                                # =>This Loop Header: Depth=1
                                        #     Child Loop BB0_21 Depth 2
	movzbl	(%rdi), %r9d
	movzbl	%r9b, %r10d
	cmpl	$90, %r10d
	jg	.LBB0_10
# %bb.6:                                #   in Loop: Header=BB0_5 Depth=1
	cmpl	$42, %r10d
	je	.LBB0_2
# %bb.7:                                #   in Loop: Header=BB0_5 Depth=1
	cmpl	$63, %r10d
	je	.LBB0_9
	jmp	.LBB0_8
	.p2align	4
.LBB0_10:                               #   in Loop: Header=BB0_5 Depth=1
	cmpl	$91, %r10d
	je	.LBB0_16
# %bb.11:                               #   in Loop: Header=BB0_5 Depth=1
	cmpl	$92, %r10d
	jne	.LBB0_8
# %bb.12:                               #   in Loop: Header=BB0_5 Depth=1
	movzbl	1(%rdi), %r10d
	testb	%r10b, %r10b
	je	.LBB0_8
# %bb.13:                               #   in Loop: Header=BB0_5 Depth=1
	cmpb	%r8b, %r10b
	jne	.LBB0_31
# %bb.14:                               #   in Loop: Header=BB0_5 Depth=1
	addq	$2, %rdi
	jmp	.LBB0_15
	.p2align	4
.LBB0_8:                                #   in Loop: Header=BB0_5 Depth=1
	cmpb	%r8b, %r9b
	jne	.LBB0_31
.LBB0_9:                                #   in Loop: Header=BB0_5 Depth=1
	incq	%rdi
.LBB0_15:                               #   in Loop: Header=BB0_5 Depth=1
	incq	%rsi
	jmp	.LBB0_3
.LBB0_16:                               #   in Loop: Header=BB0_5 Depth=1
	leaq	1(%rdi), %r9
	movzbl	1(%rdi), %r11d
	cmpl	$94, %r11d
	je	.LBB0_18
# %bb.17:                               #   in Loop: Header=BB0_5 Depth=1
	cmpl	$33, %r11d
	jne	.LBB0_19
.LBB0_18:                               #   in Loop: Header=BB0_5 Depth=1
	movzbl	2(%rdi), %r11d
	addq	$2, %rdi
	xorl	%r10d, %r10d
	testb	%r11b, %r11b
	jne	.LBB0_20
	jmp	.LBB0_29
.LBB0_19:                               #   in Loop: Header=BB0_5 Depth=1
	movl	$1, %r10d
	movq	%r9, %rdi
	testb	%r11b, %r11b
	je	.LBB0_29
.LBB0_20:                               #   in Loop: Header=BB0_5 Depth=1
	xorl	%ebx, %ebx
	jmp	.LBB0_21
	.p2align	4
.LBB0_24:                               #   in Loop: Header=BB0_21 Depth=2
	incq	%rdi
	cmpb	%r8b, %r11b
	cmovel	%eax, %ebx
	testb	%bpl, %bpl
	je	.LBB0_29
.LBB0_25:                               #   in Loop: Header=BB0_21 Depth=2
	movzbl	%bpl, %r14d
	movl	%ebp, %r11d
	cmpl	$93, %r14d
	je	.LBB0_26
.LBB0_21:                               #   Parent Loop BB0_5 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movzbl	1(%rdi), %ebp
	cmpb	$45, %bpl
	jne	.LBB0_24
# %bb.22:                               #   in Loop: Header=BB0_21 Depth=2
	movzbl	2(%rdi), %r14d
	testl	%r14d, %r14d
	je	.LBB0_24
# %bb.23:                               #   in Loop: Header=BB0_21 Depth=2
	cmpl	$93, %r14d
	je	.LBB0_24
# %bb.28:                               #   in Loop: Header=BB0_21 Depth=2
	cmpb	%r14b, %r8b
	movl	$1, %ebp
	cmoval	%ebx, %ebp
	cmpb	%r8b, %r11b
	cmovbel	%ebp, %ebx
	movzbl	3(%rdi), %ebp
	addq	$3, %rdi
	testb	%bpl, %bpl
	jne	.LBB0_25
.LBB0_29:                               #   in Loop: Header=BB0_5 Depth=1
	cmpb	$91, %r8b
	jne	.LBB0_31
# %bb.30:                               #   in Loop: Header=BB0_5 Depth=1
	incq	%rsi
	jmp	.LBB0_4
.LBB0_26:                               #   in Loop: Header=BB0_5 Depth=1
	cmpl	%r10d, %ebx
	je	.LBB0_9
	.p2align	4
.LBB0_31:                               #   in Loop: Header=BB0_5 Depth=1
	testq	%rdx, %rdx
	je	.LBB0_39
# %bb.32:                               #   in Loop: Header=BB0_5 Depth=1
	incq	%rcx
	movq	%rcx, %rsi
	movq	%rdx, %r9
	jmp	.LBB0_4
.LBB0_33:
	movq	%rdi, %r9
	.p2align	4
.LBB0_34:                               # =>This Inner Loop Header: Depth=1
	movzbl	(%r9), %ecx
	incq	%r9
	cmpb	$42, %cl
	je	.LBB0_34
# %bb.35:
	xorl	%eax, %eax
	testb	%cl, %cl
	sete	%al
.LBB0_36:
	popq	%rbx
	popq	%r14
	popq	%rbp
	retq
.LBB0_39:
	xorl	%eax, %eax
	jmp	.LBB0_36
.Lfunc_end0:
	.size	wildcard_match, .Lfunc_end0-wildcard_match
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
