	.file	"demo.c"
	.text
	.globl	hp_parse                        # -- Begin function hp_parse
	.p2align	4
	.type	hp_parse,@function
hp_parse:                               # @hp_parse
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%rbx
	movq	8(%rdi), %rax
	cmpq	%rdx, %rax
	jae	.LBB0_84
# %bb.1:
	movl	56(%rdi), %ecx
	incl	%ecx
	movabsq	$-2305843009213680003, %r8      # imm = 0xE00000000000367D
	leaq	hp_parse.V(%rip), %r9
	.p2align	4
.LBB0_2:                                # =>This Inner Loop Header: Depth=1
	movzbl	(%rsi,%rax), %r11d
	movl	%ecx, 56(%rdi)
	cmpl	$8193, %ecx                     # imm = 0x2001
	jb	.LBB0_4
# %bb.3:                                #   in Loop: Header=BB0_2 Depth=1
	movl	$13, (%rdi)
	movl	$2, %r10d
	testl	%r10d, %r10d
	je	.LBB0_75
	jmp	.LBB0_81
	.p2align	4
.LBB0_4:                                #   in Loop: Header=BB0_2 Depth=1
	movl	(%rdi), %r10d
	cmpl	$5, %r10d
	jg	.LBB0_12
# %bb.5:                                #   in Loop: Header=BB0_2 Depth=1
	cmpl	$2, %r10d
	jg	.LBB0_20
# %bb.6:                                #   in Loop: Header=BB0_2 Depth=1
	testl	%r10d, %r10d
	je	.LBB0_31
# %bb.7:                                #   in Loop: Header=BB0_2 Depth=1
	cmpl	$1, %r10d
	je	.LBB0_41
# %bb.8:                                #   in Loop: Header=BB0_2 Depth=1
	cmpl	$2, %r10d
	jne	.LBB0_73
# %bb.9:                                #   in Loop: Header=BB0_2 Depth=1
	movzbl	60(%rdi), %ebx
	cmpb	(%rbx,%r9), %r11b
	jne	.LBB0_73
# %bb.10:                               #   in Loop: Header=BB0_2 Depth=1
	incb	%bl
	movb	%bl, 60(%rdi)
	xorl	%r10d, %r10d
	cmpb	$7, %bl
	jne	.LBB0_74
# %bb.11:                               #   in Loop: Header=BB0_2 Depth=1
	movl	$3, (%rdi)
	testl	%r10d, %r10d
	je	.LBB0_75
	jmp	.LBB0_81
	.p2align	4
.LBB0_12:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$8, %r10d
	jg	.LBB0_23
# %bb.13:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$6, %r10d
	je	.LBB0_34
# %bb.14:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$7, %r10d
	je	.LBB0_44
# %bb.15:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$8, %r10d
	jne	.LBB0_73
# %bb.16:                               #   in Loop: Header=BB0_2 Depth=1
	xorl	%r10d, %r10d
	cmpl	$12, %r11d
	jle	.LBB0_71
# %bb.17:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$127, %r11d
	je	.LBB0_73
# %bb.18:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$32, %r11d
	je	.LBB0_74
# %bb.19:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$13, %r11d
	je	.LBB0_40
	jmp	.LBB0_76
.LBB0_20:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$3, %r10d
	je	.LBB0_36
# %bb.21:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$4, %r10d
	je	.LBB0_46
# %bb.22:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$5, %r10d
	je	.LBB0_28
	jmp	.LBB0_73
.LBB0_23:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$9, %r10d
	je	.LBB0_38
# %bb.24:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$10, %r10d
	je	.LBB0_28
# %bb.25:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$11, %r10d
	jne	.LBB0_73
# %bb.26:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$10, %r11d
	jne	.LBB0_73
# %bb.27:                               #   in Loop: Header=BB0_2 Depth=1
	movl	$12, (%rdi)
	incq	%rax
	movl	$6, %r10d
	testl	%r10d, %r10d
	je	.LBB0_75
	jmp	.LBB0_81
.LBB0_28:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$10, %r11d
	jne	.LBB0_73
# %bb.29:                               #   in Loop: Header=BB0_2 Depth=1
	movl	$6, (%rdi)
	jmp	.LBB0_30
.LBB0_31:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$32, %r11d
	jne	.LBB0_48
# %bb.32:                               #   in Loop: Header=BB0_2 Depth=1
	cmpq	$0, 24(%rdi)
	je	.LBB0_73
# %bb.33:                               #   in Loop: Header=BB0_2 Depth=1
	leaq	1(%rax), %r10
	movq	%r10, 32(%rdi)
	movl	$1, (%rdi)
	jmp	.LBB0_30
.LBB0_34:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$13, %r11d
	jne	.LBB0_55
# %bb.35:                               #   in Loop: Header=BB0_2 Depth=1
	movl	$11, (%rdi)
	jmp	.LBB0_30
.LBB0_36:                               #   in Loop: Header=BB0_2 Depth=1
	leal	-58(%r11), %r10d
	cmpb	$-11, %r10b
	jbe	.LBB0_73
# %bb.37:                               #   in Loop: Header=BB0_2 Depth=1
	addl	$-48, %r11d
	movl	%r11d, 48(%rdi)
	movl	$4, (%rdi)
	jmp	.LBB0_30
.LBB0_38:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$10, %r11d
	je	.LBB0_73
# %bb.39:                               #   in Loop: Header=BB0_2 Depth=1
	xorl	%r10d, %r10d
	cmpl	$13, %r11d
	jne	.LBB0_74
.LBB0_40:                               #   in Loop: Header=BB0_2 Depth=1
	incl	52(%rdi)
	movl	$10, (%rdi)
	testl	%r10d, %r10d
	je	.LBB0_75
	jmp	.LBB0_81
.LBB0_41:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$32, %r11d
	jne	.LBB0_61
# %bb.42:                               #   in Loop: Header=BB0_2 Depth=1
	cmpq	$0, 40(%rdi)
	je	.LBB0_73
# %bb.43:                               #   in Loop: Header=BB0_2 Depth=1
	movb	$0, 60(%rdi)
	movl	$2, (%rdi)
	jmp	.LBB0_30
.LBB0_44:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$58, %r11d
	jne	.LBB0_65
# %bb.45:                               #   in Loop: Header=BB0_2 Depth=1
	movl	$8, (%rdi)
	jmp	.LBB0_30
.LBB0_46:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$13, %r11d
	jne	.LBB0_73
# %bb.47:                               #   in Loop: Header=BB0_2 Depth=1
	movl	$5, (%rdi)
	jmp	.LBB0_30
.LBB0_48:                               #   in Loop: Header=BB0_2 Depth=1
	leal	-48(%r11), %r10d
	cmpb	$10, %r10b
	jb	.LBB0_52
# %bb.49:                               #   in Loop: Header=BB0_2 Depth=1
	movl	%r11d, %r10d
	andb	$-33, %r10b
	addb	$-65, %r10b
	cmpb	$26, %r10b
	jb	.LBB0_52
# %bb.50:                               #   in Loop: Header=BB0_2 Depth=1
	movl	%r11d, %r10d
	addl	$-33, %r10d
	cmpl	$63, %r10d
	ja	.LBB0_77
# %bb.51:                               #   in Loop: Header=BB0_2 Depth=1
	btq	%r10, %r8
	jae	.LBB0_77
.LBB0_52:                               #   in Loop: Header=BB0_2 Depth=1
	movq	24(%rdi), %r11
	testq	%r11, %r11
	jne	.LBB0_54
# %bb.53:                               #   in Loop: Header=BB0_2 Depth=1
	movq	%rax, 16(%rdi)
.LBB0_54:                               #   in Loop: Header=BB0_2 Depth=1
	incq	%r11
	movq	%r11, 24(%rdi)
	xorl	%r10d, %r10d
	cmpq	$17, %r11
	jae	.LBB0_73
	jmp	.LBB0_74
.LBB0_55:                               #   in Loop: Header=BB0_2 Depth=1
	leal	-48(%r11), %r10d
	cmpb	$10, %r10b
	jb	.LBB0_59
# %bb.56:                               #   in Loop: Header=BB0_2 Depth=1
	movl	%r11d, %r10d
	andb	$-33, %r10b
	addb	$-65, %r10b
	cmpb	$26, %r10b
	jb	.LBB0_59
# %bb.57:                               #   in Loop: Header=BB0_2 Depth=1
	leal	-33(%r11), %r10d
	cmpl	$63, %r10d
	ja	.LBB0_79
# %bb.58:                               #   in Loop: Header=BB0_2 Depth=1
	btq	%r10, %r8
	jae	.LBB0_79
.LBB0_59:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$64, 52(%rdi)
	jae	.LBB0_73
# %bb.60:                               #   in Loop: Header=BB0_2 Depth=1
	movl	$7, (%rdi)
	jmp	.LBB0_30
.LBB0_61:                               #   in Loop: Header=BB0_2 Depth=1
	cmpb	$33, %r11b
	setae	%r10b
	cmpl	$127, %r11d
	setne	%r11b
	testb	%r11b, %r10b
	je	.LBB0_73
# %bb.62:                               #   in Loop: Header=BB0_2 Depth=1
	movq	40(%rdi), %r10
	testq	%r10, %r10
	jne	.LBB0_64
# %bb.63:                               #   in Loop: Header=BB0_2 Depth=1
	movq	%rax, 32(%rdi)
.LBB0_64:                               #   in Loop: Header=BB0_2 Depth=1
	incq	%r10
	movq	%r10, 40(%rdi)
.LBB0_30:                               #   in Loop: Header=BB0_2 Depth=1
	xorl	%r10d, %r10d
	testl	%r10d, %r10d
	je	.LBB0_75
	jmp	.LBB0_81
.LBB0_65:                               #   in Loop: Header=BB0_2 Depth=1
	leal	-48(%r11), %ebx
	xorl	%r10d, %r10d
	cmpb	$10, %bl
	jb	.LBB0_74
# %bb.66:                               #   in Loop: Header=BB0_2 Depth=1
	movl	%r11d, %ebx
	andb	$-33, %bl
	addb	$-65, %bl
	cmpb	$26, %bl
	jb	.LBB0_74
# %bb.67:                               #   in Loop: Header=BB0_2 Depth=1
	leal	-33(%r11), %ebx
	cmpl	$63, %ebx
	ja	.LBB0_69
# %bb.68:                               #   in Loop: Header=BB0_2 Depth=1
	btq	%rbx, %r8
	jb	.LBB0_74
.LBB0_69:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$124, %r11d
	je	.LBB0_74
# %bb.70:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$126, %r11d
	jne	.LBB0_73
	jmp	.LBB0_74
.LBB0_71:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$9, %r11d
	je	.LBB0_74
# %bb.72:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$10, %r11d
	jne	.LBB0_76
	.p2align	4
.LBB0_73:                               #   in Loop: Header=BB0_2 Depth=1
	movl	$13, (%rdi)
	movl	$6, %r10d
.LBB0_74:                               #   in Loop: Header=BB0_2 Depth=1
	testl	%r10d, %r10d
	jne	.LBB0_81
.LBB0_75:                               #   in Loop: Header=BB0_2 Depth=1
	incq	%rax
	incl	%ecx
	cmpq	%rdx, %rax
	jb	.LBB0_2
	jmp	.LBB0_84
.LBB0_76:                               #   in Loop: Header=BB0_2 Depth=1
	movl	$9, (%rdi)
	testl	%r10d, %r10d
	je	.LBB0_75
	jmp	.LBB0_81
.LBB0_77:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$124, %r11d
	je	.LBB0_52
# %bb.78:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$126, %r11d
	je	.LBB0_52
	jmp	.LBB0_73
.LBB0_79:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$124, %r11d
	je	.LBB0_59
# %bb.80:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$126, %r11d
	je	.LBB0_59
	jmp	.LBB0_73
.LBB0_81:
	cmpl	$2, %r10d
	je	.LBB0_84
# %bb.82:
	cmpl	$4, %r10d
	jne	.LBB0_84
# %bb.83:
                                        # implicit-def: $eax
	jmp	.LBB0_85
.LBB0_84:
	movq	%rax, 8(%rdi)
	movl	(%rdi), %eax
	xorl	%ecx, %ecx
	cmpl	$12, %eax
	sete	%cl
	cmpl	$13, %eax
	movl	$-1, %eax
	cmovnel	%ecx, %eax
.LBB0_85:
	popq	%rbx
	popq	%rbp
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
