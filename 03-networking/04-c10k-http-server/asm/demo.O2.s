	.file	"demo.c"
	.text
	.globl	hp_parse                        # -- Begin function hp_parse
	.p2align	4
	.type	hp_parse,@function
hp_parse:                               # @hp_parse
# %bb.0:
	movq	8(%rdi), %rcx
	cmpq	%rdx, %rcx
	jae	.LBB0_2
# %bb.1:
	movl	56(%rdi), %eax
	incl	%eax
	movabsq	$-2305843009213680003, %r8      # imm = 0xE00000000000367D
	leaq	hp_parse.V(%rip), %r9
	jmp	.LBB0_6
.LBB0_2:
	movl	(%rdi), %edx
	xorl	%eax, %eax
	cmpl	$12, %edx
	sete	%cl
	cmpl	$13, %edx
	je	.LBB0_82
# %bb.3:
	movb	%cl, %al
	retq
.LBB0_4:                                #   in Loop: Header=BB0_6 Depth=1
	incl	52(%rdi)
	movl	$10, (%rdi)
	.p2align	4
.LBB0_5:                                #   in Loop: Header=BB0_6 Depth=1
	incq	%rcx
	incl	%eax
	cmpq	%rcx, %rdx
	je	.LBB0_78
.LBB0_6:                                # =>This Inner Loop Header: Depth=1
	movzbl	(%rsi,%rcx), %r10d
	movl	%eax, 56(%rdi)
	cmpl	$8192, %eax                     # imm = 0x2000
	ja	.LBB0_81
# %bb.7:                                #   in Loop: Header=BB0_6 Depth=1
	movl	(%rdi), %r11d
	cmpl	$5, %r11d
	jg	.LBB0_15
# %bb.8:                                #   in Loop: Header=BB0_6 Depth=1
	cmpl	$2, %r11d
	jg	.LBB0_23
# %bb.9:                                #   in Loop: Header=BB0_6 Depth=1
	testl	%r11d, %r11d
	je	.LBB0_42
# %bb.10:                               #   in Loop: Header=BB0_6 Depth=1
	cmpl	$1, %r11d
	je	.LBB0_33
# %bb.11:                               #   in Loop: Header=BB0_6 Depth=1
	cmpl	$2, %r11d
	jne	.LBB0_81
# %bb.12:                               #   in Loop: Header=BB0_6 Depth=1
	movzbl	60(%rdi), %r11d
	cmpb	(%r11,%r9), %r10b
	jne	.LBB0_81
# %bb.13:                               #   in Loop: Header=BB0_6 Depth=1
	incb	%r11b
	movb	%r11b, 60(%rdi)
	cmpb	$7, %r11b
	jne	.LBB0_5
# %bb.14:                               #   in Loop: Header=BB0_6 Depth=1
	movl	$3, (%rdi)
	jmp	.LBB0_5
	.p2align	4
.LBB0_15:                               #   in Loop: Header=BB0_6 Depth=1
	cmpl	$8, %r11d
	jg	.LBB0_28
# %bb.16:                               #   in Loop: Header=BB0_6 Depth=1
	cmpl	$6, %r11d
	je	.LBB0_40
# %bb.17:                               #   in Loop: Header=BB0_6 Depth=1
	cmpl	$7, %r11d
	je	.LBB0_31
# %bb.18:                               #   in Loop: Header=BB0_6 Depth=1
	cmpl	$8, %r11d
	jne	.LBB0_81
# %bb.19:                               #   in Loop: Header=BB0_6 Depth=1
	cmpl	$12, %r10d
	jle	.LBB0_70
# %bb.20:                               #   in Loop: Header=BB0_6 Depth=1
	cmpl	$13, %r10d
	je	.LBB0_4
# %bb.21:                               #   in Loop: Header=BB0_6 Depth=1
	cmpl	$32, %r10d
	je	.LBB0_5
# %bb.22:                               #   in Loop: Header=BB0_6 Depth=1
	cmpl	$127, %r10d
	jne	.LBB0_72
	jmp	.LBB0_81
	.p2align	4
.LBB0_23:                               #   in Loop: Header=BB0_6 Depth=1
	cmpl	$3, %r11d
	je	.LBB0_45
# %bb.24:                               #   in Loop: Header=BB0_6 Depth=1
	cmpl	$4, %r11d
	je	.LBB0_36
# %bb.25:                               #   in Loop: Header=BB0_6 Depth=1
	cmpl	$5, %r11d
	je	.LBB0_26
	jmp	.LBB0_81
.LBB0_28:                               #   in Loop: Header=BB0_6 Depth=1
	cmpl	$9, %r11d
	je	.LBB0_38
# %bb.29:                               #   in Loop: Header=BB0_6 Depth=1
	cmpl	$10, %r11d
	jne	.LBB0_79
.LBB0_26:                               #   in Loop: Header=BB0_6 Depth=1
	cmpl	$10, %r10d
	jne	.LBB0_81
# %bb.27:                               #   in Loop: Header=BB0_6 Depth=1
	movl	$6, (%rdi)
	jmp	.LBB0_5
.LBB0_31:                               #   in Loop: Header=BB0_6 Depth=1
	cmpl	$58, %r10d
	jne	.LBB0_47
# %bb.32:                               #   in Loop: Header=BB0_6 Depth=1
	movl	$8, (%rdi)
	jmp	.LBB0_5
.LBB0_33:                               #   in Loop: Header=BB0_6 Depth=1
	cmpl	$32, %r10d
	jne	.LBB0_53
# %bb.34:                               #   in Loop: Header=BB0_6 Depth=1
	cmpq	$0, 40(%rdi)
	je	.LBB0_81
# %bb.35:                               #   in Loop: Header=BB0_6 Depth=1
	movb	$0, 60(%rdi)
	movl	$2, (%rdi)
	jmp	.LBB0_5
.LBB0_36:                               #   in Loop: Header=BB0_6 Depth=1
	cmpl	$13, %r10d
	jne	.LBB0_81
# %bb.37:                               #   in Loop: Header=BB0_6 Depth=1
	movl	$5, (%rdi)
	jmp	.LBB0_5
.LBB0_38:                               #   in Loop: Header=BB0_6 Depth=1
	cmpl	$13, %r10d
	je	.LBB0_4
# %bb.39:                               #   in Loop: Header=BB0_6 Depth=1
	cmpl	$10, %r10d
	jne	.LBB0_5
	jmp	.LBB0_81
.LBB0_40:                               #   in Loop: Header=BB0_6 Depth=1
	cmpl	$13, %r10d
	jne	.LBB0_58
# %bb.41:                               #   in Loop: Header=BB0_6 Depth=1
	movl	$11, (%rdi)
	jmp	.LBB0_5
.LBB0_42:                               #   in Loop: Header=BB0_6 Depth=1
	cmpl	$32, %r10d
	jne	.LBB0_64
# %bb.43:                               #   in Loop: Header=BB0_6 Depth=1
	cmpq	$0, 24(%rdi)
	je	.LBB0_81
# %bb.44:                               #   in Loop: Header=BB0_6 Depth=1
	leaq	1(%rcx), %r10
	movq	%r10, 32(%rdi)
	movl	$1, (%rdi)
	jmp	.LBB0_5
.LBB0_45:                               #   in Loop: Header=BB0_6 Depth=1
	leal	-58(%r10), %r11d
	cmpb	$-10, %r11b
	jb	.LBB0_81
# %bb.46:                               #   in Loop: Header=BB0_6 Depth=1
	addl	$-48, %r10d
	movl	%r10d, 48(%rdi)
	movl	$4, (%rdi)
	jmp	.LBB0_5
.LBB0_47:                               #   in Loop: Header=BB0_6 Depth=1
	leal	-48(%r10), %r11d
	cmpb	$10, %r11b
	jb	.LBB0_5
# %bb.48:                               #   in Loop: Header=BB0_6 Depth=1
	movl	%r10d, %r11d
	andb	$-33, %r11b
	addb	$-65, %r11b
	cmpb	$26, %r11b
	jb	.LBB0_5
# %bb.49:                               #   in Loop: Header=BB0_6 Depth=1
	leal	-33(%r10), %r11d
	cmpl	$63, %r11d
	ja	.LBB0_51
# %bb.50:                               #   in Loop: Header=BB0_6 Depth=1
	btq	%r11, %r8
	jb	.LBB0_5
.LBB0_51:                               #   in Loop: Header=BB0_6 Depth=1
	cmpl	$124, %r10d
	je	.LBB0_5
# %bb.52:                               #   in Loop: Header=BB0_6 Depth=1
	cmpl	$126, %r10d
	je	.LBB0_5
	jmp	.LBB0_81
.LBB0_53:                               #   in Loop: Header=BB0_6 Depth=1
	cmpb	$33, %r10b
	jb	.LBB0_81
# %bb.54:                               #   in Loop: Header=BB0_6 Depth=1
	cmpb	$127, %r10b
	je	.LBB0_81
# %bb.55:                               #   in Loop: Header=BB0_6 Depth=1
	movq	40(%rdi), %r10
	testq	%r10, %r10
	jne	.LBB0_57
# %bb.56:                               #   in Loop: Header=BB0_6 Depth=1
	movq	%rcx, 32(%rdi)
.LBB0_57:                               #   in Loop: Header=BB0_6 Depth=1
	incq	%r10
	movq	%r10, 40(%rdi)
	jmp	.LBB0_5
.LBB0_58:                               #   in Loop: Header=BB0_6 Depth=1
	leal	-48(%r10), %r11d
	cmpb	$10, %r11b
	jb	.LBB0_62
# %bb.59:                               #   in Loop: Header=BB0_6 Depth=1
	movl	%r10d, %r11d
	andb	$-33, %r11b
	addb	$-65, %r11b
	cmpb	$26, %r11b
	jb	.LBB0_62
# %bb.60:                               #   in Loop: Header=BB0_6 Depth=1
	leal	-33(%r10), %r11d
	cmpl	$63, %r11d
	ja	.LBB0_74
# %bb.61:                               #   in Loop: Header=BB0_6 Depth=1
	btq	%r11, %r8
	jae	.LBB0_74
.LBB0_62:                               #   in Loop: Header=BB0_6 Depth=1
	cmpl	$63, 52(%rdi)
	ja	.LBB0_81
# %bb.63:                               #   in Loop: Header=BB0_6 Depth=1
	movl	$7, (%rdi)
	jmp	.LBB0_5
.LBB0_64:                               #   in Loop: Header=BB0_6 Depth=1
	leal	-48(%r10), %r11d
	cmpb	$10, %r11b
	jb	.LBB0_68
# %bb.65:                               #   in Loop: Header=BB0_6 Depth=1
	movl	%r10d, %r11d
	andb	$-33, %r11b
	addb	$-65, %r11b
	cmpb	$26, %r11b
	jb	.LBB0_68
# %bb.66:                               #   in Loop: Header=BB0_6 Depth=1
	movl	%r10d, %r11d
	addl	$-33, %r11d
	cmpl	$63, %r11d
	ja	.LBB0_76
# %bb.67:                               #   in Loop: Header=BB0_6 Depth=1
	btq	%r11, %r8
	jae	.LBB0_76
.LBB0_68:                               #   in Loop: Header=BB0_6 Depth=1
	movq	24(%rdi), %r10
	testq	%r10, %r10
	je	.LBB0_73
# %bb.69:                               #   in Loop: Header=BB0_6 Depth=1
	incq	%r10
	movq	%r10, 24(%rdi)
	cmpq	$16, %r10
	jbe	.LBB0_5
	jmp	.LBB0_81
.LBB0_70:                               #   in Loop: Header=BB0_6 Depth=1
	cmpl	$9, %r10d
	je	.LBB0_5
# %bb.71:                               #   in Loop: Header=BB0_6 Depth=1
	cmpl	$10, %r10d
	je	.LBB0_81
.LBB0_72:                               #   in Loop: Header=BB0_6 Depth=1
	movl	$9, (%rdi)
	jmp	.LBB0_5
.LBB0_73:                               #   in Loop: Header=BB0_6 Depth=1
	movq	%rcx, 16(%rdi)
	movq	$1, 24(%rdi)
	jmp	.LBB0_5
.LBB0_74:                               #   in Loop: Header=BB0_6 Depth=1
	cmpl	$126, %r10d
	je	.LBB0_62
# %bb.75:                               #   in Loop: Header=BB0_6 Depth=1
	cmpl	$124, %r10d
	je	.LBB0_62
	jmp	.LBB0_81
.LBB0_76:                               #   in Loop: Header=BB0_6 Depth=1
	cmpl	$126, %r10d
	je	.LBB0_68
# %bb.77:                               #   in Loop: Header=BB0_6 Depth=1
	cmpl	$124, %r10d
	je	.LBB0_68
	jmp	.LBB0_81
.LBB0_78:
	xorl	%eax, %eax
	movq	%rdx, 8(%rdi)
	retq
.LBB0_79:
	cmpl	$11, %r11d
	jne	.LBB0_81
# %bb.80:
	cmpl	$10, %r10d
	jne	.LBB0_81
# %bb.83:
	movl	$12, (%rdi)
	incq	%rcx
	movl	$1, %eax
	movq	%rcx, 8(%rdi)
	retq
.LBB0_81:
	movl	$13, (%rdi)
	movq	%rcx, 8(%rdi)
.LBB0_82:
	movl	$-1, %eax
	retq
.Lfunc_end0:
	.size	hp_parse, .Lfunc_end0-hp_parse
                                        # -- End function
	.type	hp_parse.V,@object              # @hp_parse.V
	.section	.rodata.str1.1,"aMS",@progbits,1
hp_parse.V:
	.asciz	"HTTP/1."
	.size	hp_parse.V, 8

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
