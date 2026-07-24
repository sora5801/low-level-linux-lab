	.file	"demo.c"
	.text
	.globl	glob_star                       # -- Begin function glob_star
	.p2align	4
	.type	glob_star,@function
glob_star:                              # @glob_star
# %bb.0:
	movzbl	(%rsi), %edx
	testb	%dl, %dl
	je	.LBB0_17
# %bb.1:
	xorl	%eax, %eax
	xorl	%ecx, %ecx
	jmp	.LBB0_5
	.p2align	4
.LBB0_14:                               #   in Loop: Header=BB0_5 Depth=1
	incq	%rdi
	movq	%rdi, %rcx
	movq	%rsi, %rax
.LBB0_4:                                #   in Loop: Header=BB0_5 Depth=1
	movzbl	(%rsi), %edx
	testb	%dl, %dl
	je	.LBB0_17
.LBB0_5:                                # =>This Inner Loop Header: Depth=1
	movzbl	(%rdi), %r8d
	cmpl	$42, %r8d
	je	.LBB0_14
# %bb.6:                                #   in Loop: Header=BB0_5 Depth=1
	cmpl	$92, %r8d
	je	.LBB0_11
# %bb.7:                                #   in Loop: Header=BB0_5 Depth=1
	cmpl	$63, %r8d
	je	.LBB0_2
	jmp	.LBB0_8
	.p2align	4
.LBB0_11:                               #   in Loop: Header=BB0_5 Depth=1
	movzbl	1(%rdi), %r9d
	testb	%r9b, %r9b
	je	.LBB0_8
# %bb.12:                               #   in Loop: Header=BB0_5 Depth=1
	cmpb	%dl, %r9b
	jne	.LBB0_15
# %bb.13:                               #   in Loop: Header=BB0_5 Depth=1
	addq	$2, %rdi
	jmp	.LBB0_3
	.p2align	4
.LBB0_8:                                #   in Loop: Header=BB0_5 Depth=1
	cmpb	%dl, %r8b
	jne	.LBB0_15
.LBB0_2:                                #   in Loop: Header=BB0_5 Depth=1
	incq	%rdi
.LBB0_3:                                #   in Loop: Header=BB0_5 Depth=1
	incq	%rsi
	jmp	.LBB0_4
	.p2align	4
.LBB0_15:                               #   in Loop: Header=BB0_5 Depth=1
	testq	%rcx, %rcx
	je	.LBB0_10
# %bb.16:                               #   in Loop: Header=BB0_5 Depth=1
	incq	%rax
	movq	%rcx, %rdi
	movq	%rax, %rsi
	jmp	.LBB0_4
	.p2align	4
.LBB0_17:                               # =>This Inner Loop Header: Depth=1
	movzbl	(%rdi), %ecx
	incq	%rdi
	cmpb	$42, %cl
	je	.LBB0_17
# %bb.18:
	xorl	%eax, %eax
	testb	%cl, %cl
	sete	%al
	retq
.LBB0_10:
	xorl	%eax, %eax
	retq
.Lfunc_end0:
	.size	glob_star, .Lfunc_end0-glob_star
                                        # -- End function
	.globl	demo_run                        # -- Begin function demo_run
	.p2align	4
	.type	demo_run,@function
demo_run:                               # @demo_run
# %bb.0:
	leaq	.L.str(%rip), %rax
	leaq	.L.str.1(%rip), %rsi
	movb	$109, %dil
	xorl	%ecx, %ecx
	xorl	%edx, %edx
	jmp	.LBB1_4
	.p2align	4
.LBB1_13:                               #   in Loop: Header=BB1_4 Depth=1
	incq	%rax
	movq	%rax, %rdx
	movq	%rsi, %rcx
.LBB1_3:                                #   in Loop: Header=BB1_4 Depth=1
	movzbl	(%rsi), %edi
	testb	%dil, %dil
	je	.LBB1_16
.LBB1_4:                                # =>This Inner Loop Header: Depth=1
	movzbl	(%rax), %r8d
	cmpl	$42, %r8d
	je	.LBB1_13
# %bb.5:                                #   in Loop: Header=BB1_4 Depth=1
	cmpl	$92, %r8d
	je	.LBB1_10
# %bb.6:                                #   in Loop: Header=BB1_4 Depth=1
	cmpl	$63, %r8d
	je	.LBB1_1
	jmp	.LBB1_7
	.p2align	4
.LBB1_10:                               #   in Loop: Header=BB1_4 Depth=1
	movzbl	1(%rax), %r9d
	testb	%r9b, %r9b
	je	.LBB1_7
# %bb.11:                               #   in Loop: Header=BB1_4 Depth=1
	cmpb	%dil, %r9b
	jne	.LBB1_14
# %bb.12:                               #   in Loop: Header=BB1_4 Depth=1
	addq	$2, %rax
	jmp	.LBB1_2
	.p2align	4
.LBB1_7:                                #   in Loop: Header=BB1_4 Depth=1
	cmpb	%dil, %r8b
	jne	.LBB1_14
.LBB1_1:                                #   in Loop: Header=BB1_4 Depth=1
	incq	%rax
.LBB1_2:                                #   in Loop: Header=BB1_4 Depth=1
	incq	%rsi
	jmp	.LBB1_3
	.p2align	4
.LBB1_14:                               #   in Loop: Header=BB1_4 Depth=1
	testq	%rdx, %rdx
	je	.LBB1_9
# %bb.15:                               #   in Loop: Header=BB1_4 Depth=1
	incq	%rcx
	movq	%rdx, %rax
	movq	%rcx, %rsi
	jmp	.LBB1_3
	.p2align	4
.LBB1_16:                               # =>This Inner Loop Header: Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	cmpb	$42, %cl
	je	.LBB1_16
# %bb.17:
	xorl	%eax, %eax
	testb	%cl, %cl
	sete	%al
	jmp	.LBB1_18
.LBB1_9:
	xorl	%eax, %eax
.LBB1_18:
	leaq	.L.str.2(%rip), %rcx
	leaq	.L.str.3(%rip), %rdi
	movb	$97, %r8b
	xorl	%edx, %edx
	xorl	%esi, %esi
	jmp	.LBB1_22
	.p2align	4
.LBB1_31:                               #   in Loop: Header=BB1_22 Depth=1
	incq	%rcx
	movq	%rcx, %rsi
	movq	%rdi, %rdx
.LBB1_21:                               #   in Loop: Header=BB1_22 Depth=1
	movzbl	(%rdi), %r8d
	testb	%r8b, %r8b
	je	.LBB1_34
.LBB1_22:                               # =>This Inner Loop Header: Depth=1
	movzbl	(%rcx), %r9d
	cmpl	$42, %r9d
	je	.LBB1_31
# %bb.23:                               #   in Loop: Header=BB1_22 Depth=1
	cmpl	$92, %r9d
	je	.LBB1_28
# %bb.24:                               #   in Loop: Header=BB1_22 Depth=1
	cmpl	$63, %r9d
	je	.LBB1_19
	jmp	.LBB1_25
	.p2align	4
.LBB1_28:                               #   in Loop: Header=BB1_22 Depth=1
	movzbl	1(%rcx), %r10d
	testb	%r10b, %r10b
	je	.LBB1_25
# %bb.29:                               #   in Loop: Header=BB1_22 Depth=1
	cmpb	%r8b, %r10b
	jne	.LBB1_32
# %bb.30:                               #   in Loop: Header=BB1_22 Depth=1
	addq	$2, %rcx
	jmp	.LBB1_20
	.p2align	4
.LBB1_25:                               #   in Loop: Header=BB1_22 Depth=1
	cmpb	%r8b, %r9b
	jne	.LBB1_32
.LBB1_19:                               #   in Loop: Header=BB1_22 Depth=1
	incq	%rcx
.LBB1_20:                               #   in Loop: Header=BB1_22 Depth=1
	incq	%rdi
	jmp	.LBB1_21
	.p2align	4
.LBB1_32:                               #   in Loop: Header=BB1_22 Depth=1
	testq	%rsi, %rsi
	je	.LBB1_27
# %bb.33:                               #   in Loop: Header=BB1_22 Depth=1
	incq	%rdx
	movq	%rsi, %rcx
	movq	%rdx, %rdi
	jmp	.LBB1_21
	.p2align	4
.LBB1_34:                               # =>This Inner Loop Header: Depth=1
	movzbl	(%rcx), %edx
	incq	%rcx
	cmpb	$42, %dl
	je	.LBB1_34
# %bb.35:
	xorl	%ecx, %ecx
	testb	%dl, %dl
	sete	%cl
	addl	%ecx, %ecx
	jmp	.LBB1_36
.LBB1_27:
	xorl	%ecx, %ecx
.LBB1_36:
	leaq	.L.str.4(%rip), %rdx
	leaq	.L.str.3(%rip), %r8
	movb	$97, %r9b
	xorl	%esi, %esi
	xorl	%edi, %edi
	jmp	.LBB1_40
	.p2align	4
.LBB1_49:                               #   in Loop: Header=BB1_40 Depth=1
	incq	%rdx
	movq	%rdx, %rdi
	movq	%r8, %rsi
.LBB1_39:                               #   in Loop: Header=BB1_40 Depth=1
	movzbl	(%r8), %r9d
	testb	%r9b, %r9b
	je	.LBB1_52
.LBB1_40:                               # =>This Inner Loop Header: Depth=1
	movzbl	(%rdx), %r10d
	cmpl	$42, %r10d
	je	.LBB1_49
# %bb.41:                               #   in Loop: Header=BB1_40 Depth=1
	cmpl	$92, %r10d
	je	.LBB1_46
# %bb.42:                               #   in Loop: Header=BB1_40 Depth=1
	cmpl	$63, %r10d
	je	.LBB1_37
	jmp	.LBB1_43
	.p2align	4
.LBB1_46:                               #   in Loop: Header=BB1_40 Depth=1
	movzbl	1(%rdx), %r11d
	testb	%r11b, %r11b
	je	.LBB1_43
# %bb.47:                               #   in Loop: Header=BB1_40 Depth=1
	cmpb	%r9b, %r11b
	jne	.LBB1_50
# %bb.48:                               #   in Loop: Header=BB1_40 Depth=1
	addq	$2, %rdx
	jmp	.LBB1_38
	.p2align	4
.LBB1_43:                               #   in Loop: Header=BB1_40 Depth=1
	cmpb	%r9b, %r10b
	jne	.LBB1_50
.LBB1_37:                               #   in Loop: Header=BB1_40 Depth=1
	incq	%rdx
.LBB1_38:                               #   in Loop: Header=BB1_40 Depth=1
	incq	%r8
	jmp	.LBB1_39
	.p2align	4
.LBB1_50:                               #   in Loop: Header=BB1_40 Depth=1
	testq	%rdi, %rdi
	je	.LBB1_45
# %bb.51:                               #   in Loop: Header=BB1_40 Depth=1
	incq	%rsi
	movq	%rdi, %rdx
	movq	%rsi, %r8
	jmp	.LBB1_39
	.p2align	4
.LBB1_52:                               # =>This Inner Loop Header: Depth=1
	movzbl	(%rdx), %esi
	incq	%rdx
	cmpb	$42, %sil
	je	.LBB1_52
# %bb.53:
	xorl	%edx, %edx
	testb	%sil, %sil
	sete	%dl
	shll	$2, %edx
	jmp	.LBB1_54
.LBB1_45:
	xorl	%edx, %edx
.LBB1_54:
	orl	%eax, %ecx
	leal	(%rdx,%rcx), %eax
	addl	$8, %eax
	retq
.Lfunc_end1:
	.size	demo_run, .Lfunc_end1-demo_run
                                        # -- End function
	.type	.L.str,@object                  # @.str
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str:
	.asciz	"*.c"
	.size	.L.str, 4

	.type	.L.str.1,@object                # @.str.1
.L.str.1:
	.asciz	"main.c"
	.size	.L.str.1, 7

	.type	.L.str.2,@object                # @.str.2
.L.str.2:
	.asciz	"a?c"
	.size	.L.str.2, 4

	.type	.L.str.3,@object                # @.str.3
.L.str.3:
	.asciz	"abc"
	.size	.L.str.3, 4

	.type	.L.str.4,@object                # @.str.4
.L.str.4:
	.asciz	"a*d"
	.size	.L.str.4, 4

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
