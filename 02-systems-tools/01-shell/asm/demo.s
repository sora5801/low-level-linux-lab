	.file	"demo.c"
	.text
	.globl	glob_star                       # -- Begin function glob_star
	.p2align	4
	.type	glob_star,@function
glob_star:                              # @glob_star
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	xorl	%eax, %eax
	xorl	%ecx, %ecx
	jmp	.LBB0_1
	.p2align	4
.LBB0_7:                                #   in Loop: Header=BB0_1 Depth=1
	incq	%rdi
	movb	$1, %dl
	movq	%rdi, %rax
	movq	%rsi, %rcx
.LBB0_15:                               #   in Loop: Header=BB0_1 Depth=1
	testb	%dl, %dl
	je	.LBB0_16
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	movzbl	(%rsi), %edx
	testb	%dl, %dl
	je	.LBB0_17
# %bb.2:                                #   in Loop: Header=BB0_1 Depth=1
	movzbl	(%rdi), %r8d
	cmpl	$42, %r8d
	je	.LBB0_7
# %bb.3:                                #   in Loop: Header=BB0_1 Depth=1
	cmpl	$92, %r8d
	je	.LBB0_8
# %bb.4:                                #   in Loop: Header=BB0_1 Depth=1
	cmpl	$63, %r8d
	jne	.LBB0_11
	jmp	.LBB0_5
	.p2align	4
.LBB0_8:                                #   in Loop: Header=BB0_1 Depth=1
	movzbl	1(%rdi), %r9d
	testb	%r9b, %r9b
	je	.LBB0_11
# %bb.9:                                #   in Loop: Header=BB0_1 Depth=1
	cmpb	%dl, %r9b
	jne	.LBB0_12
# %bb.10:                               #   in Loop: Header=BB0_1 Depth=1
	addq	$2, %rdi
	jmp	.LBB0_6
	.p2align	4
.LBB0_11:                               #   in Loop: Header=BB0_1 Depth=1
	cmpb	%dl, %r8b
	jne	.LBB0_12
.LBB0_5:                                #   in Loop: Header=BB0_1 Depth=1
	incq	%rdi
.LBB0_6:                                #   in Loop: Header=BB0_1 Depth=1
	incq	%rsi
	movb	$1, %dl
	jmp	.LBB0_15
	.p2align	4
.LBB0_12:                               #   in Loop: Header=BB0_1 Depth=1
	testq	%rax, %rax
	je	.LBB0_13
# %bb.14:                               #   in Loop: Header=BB0_1 Depth=1
	incq	%rcx
	movb	$1, %dl
	movq	%rax, %rdi
	movq	%rcx, %rsi
	jmp	.LBB0_15
.LBB0_13:                               #   in Loop: Header=BB0_1 Depth=1
	xorl	%eax, %eax
	xorl	%edx, %edx
	jmp	.LBB0_15
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
	popq	%rbp
	retq
.LBB0_16:
	xorl	%eax, %eax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	glob_star, .Lfunc_end0-glob_star
                                        # -- End function
	.globl	demo_run                        # -- Begin function demo_run
	.p2align	4
	.type	demo_run,@function
demo_run:                               # @demo_run
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	leaq	.L.str.1(%rip), %rcx
	leaq	.L.str(%rip), %rax
	xorl	%edx, %edx
	xorl	%esi, %esi
	jmp	.LBB1_1
	.p2align	4
.LBB1_7:                                #   in Loop: Header=BB1_1 Depth=1
	incq	%rax
	movb	$1, %dil
	movq	%rax, %rdx
	movq	%rcx, %rsi
.LBB1_15:                               #   in Loop: Header=BB1_1 Depth=1
	testb	%dil, %dil
	je	.LBB1_16
.LBB1_1:                                # =>This Inner Loop Header: Depth=1
	movzbl	(%rcx), %edi
	testb	%dil, %dil
	je	.LBB1_17
# %bb.2:                                #   in Loop: Header=BB1_1 Depth=1
	movzbl	(%rax), %r8d
	cmpl	$42, %r8d
	je	.LBB1_7
# %bb.3:                                #   in Loop: Header=BB1_1 Depth=1
	cmpl	$92, %r8d
	je	.LBB1_8
# %bb.4:                                #   in Loop: Header=BB1_1 Depth=1
	cmpl	$63, %r8d
	jne	.LBB1_11
	jmp	.LBB1_5
	.p2align	4
.LBB1_8:                                #   in Loop: Header=BB1_1 Depth=1
	movzbl	1(%rax), %r9d
	testb	%r9b, %r9b
	je	.LBB1_11
# %bb.9:                                #   in Loop: Header=BB1_1 Depth=1
	cmpb	%dil, %r9b
	jne	.LBB1_12
# %bb.10:                               #   in Loop: Header=BB1_1 Depth=1
	addq	$2, %rax
	jmp	.LBB1_6
	.p2align	4
.LBB1_11:                               #   in Loop: Header=BB1_1 Depth=1
	cmpb	%dil, %r8b
	jne	.LBB1_12
.LBB1_5:                                #   in Loop: Header=BB1_1 Depth=1
	incq	%rax
.LBB1_6:                                #   in Loop: Header=BB1_1 Depth=1
	incq	%rcx
	movb	$1, %dil
	jmp	.LBB1_15
	.p2align	4
.LBB1_12:                               #   in Loop: Header=BB1_1 Depth=1
	testq	%rdx, %rdx
	je	.LBB1_13
# %bb.14:                               #   in Loop: Header=BB1_1 Depth=1
	incq	%rsi
	movb	$1, %dil
	movq	%rdx, %rax
	movq	%rsi, %rcx
	jmp	.LBB1_15
.LBB1_13:                               #   in Loop: Header=BB1_1 Depth=1
	xorl	%edx, %edx
	xorl	%edi, %edi
	jmp	.LBB1_15
	.p2align	4
.LBB1_17:                               # =>This Inner Loop Header: Depth=1
	movzbl	(%rax), %ecx
	incq	%rax
	cmpb	$42, %cl
	je	.LBB1_17
# %bb.18:
	xorl	%eax, %eax
	testb	%cl, %cl
	sete	%al
	jmp	.LBB1_19
.LBB1_16:
	xorl	%eax, %eax
.LBB1_19:
	leaq	.L.str.3(%rip), %rdx
	leaq	.L.str.2(%rip), %rcx
	xorl	%esi, %esi
	xorl	%edi, %edi
	jmp	.LBB1_20
	.p2align	4
.LBB1_26:                               #   in Loop: Header=BB1_20 Depth=1
	incq	%rcx
	movb	$1, %r8b
	movq	%rcx, %rsi
	movq	%rdx, %rdi
.LBB1_34:                               #   in Loop: Header=BB1_20 Depth=1
	testb	%r8b, %r8b
	je	.LBB1_35
.LBB1_20:                               # =>This Inner Loop Header: Depth=1
	movzbl	(%rdx), %r8d
	testb	%r8b, %r8b
	je	.LBB1_36
# %bb.21:                               #   in Loop: Header=BB1_20 Depth=1
	movzbl	(%rcx), %r9d
	cmpl	$42, %r9d
	je	.LBB1_26
# %bb.22:                               #   in Loop: Header=BB1_20 Depth=1
	cmpl	$92, %r9d
	je	.LBB1_27
# %bb.23:                               #   in Loop: Header=BB1_20 Depth=1
	cmpl	$63, %r9d
	jne	.LBB1_30
	jmp	.LBB1_24
	.p2align	4
.LBB1_27:                               #   in Loop: Header=BB1_20 Depth=1
	movzbl	1(%rcx), %r10d
	testb	%r10b, %r10b
	je	.LBB1_30
# %bb.28:                               #   in Loop: Header=BB1_20 Depth=1
	cmpb	%r8b, %r10b
	jne	.LBB1_31
# %bb.29:                               #   in Loop: Header=BB1_20 Depth=1
	addq	$2, %rcx
	jmp	.LBB1_25
	.p2align	4
.LBB1_30:                               #   in Loop: Header=BB1_20 Depth=1
	cmpb	%r8b, %r9b
	jne	.LBB1_31
.LBB1_24:                               #   in Loop: Header=BB1_20 Depth=1
	incq	%rcx
.LBB1_25:                               #   in Loop: Header=BB1_20 Depth=1
	incq	%rdx
	movb	$1, %r8b
	jmp	.LBB1_34
	.p2align	4
.LBB1_31:                               #   in Loop: Header=BB1_20 Depth=1
	testq	%rsi, %rsi
	je	.LBB1_32
# %bb.33:                               #   in Loop: Header=BB1_20 Depth=1
	incq	%rdi
	movb	$1, %r8b
	movq	%rsi, %rcx
	movq	%rdi, %rdx
	jmp	.LBB1_34
.LBB1_32:                               #   in Loop: Header=BB1_20 Depth=1
	xorl	%esi, %esi
	xorl	%r8d, %r8d
	jmp	.LBB1_34
	.p2align	4
.LBB1_36:                               # =>This Inner Loop Header: Depth=1
	movzbl	(%rcx), %edx
	incq	%rcx
	cmpb	$42, %dl
	je	.LBB1_36
# %bb.37:
	xorl	%ecx, %ecx
	testb	%dl, %dl
	sete	%cl
	addl	%ecx, %ecx
	jmp	.LBB1_38
.LBB1_35:
	xorl	%ecx, %ecx
.LBB1_38:
	leaq	.L.str.3(%rip), %rsi
	leaq	.L.str.4(%rip), %rdx
	xorl	%edi, %edi
	xorl	%r8d, %r8d
	jmp	.LBB1_39
	.p2align	4
.LBB1_45:                               #   in Loop: Header=BB1_39 Depth=1
	incq	%rdx
	movb	$1, %r9b
	movq	%rdx, %rdi
	movq	%rsi, %r8
.LBB1_53:                               #   in Loop: Header=BB1_39 Depth=1
	testb	%r9b, %r9b
	je	.LBB1_54
.LBB1_39:                               # =>This Inner Loop Header: Depth=1
	movzbl	(%rsi), %r9d
	testb	%r9b, %r9b
	je	.LBB1_55
# %bb.40:                               #   in Loop: Header=BB1_39 Depth=1
	movzbl	(%rdx), %r10d
	cmpl	$42, %r10d
	je	.LBB1_45
# %bb.41:                               #   in Loop: Header=BB1_39 Depth=1
	cmpl	$92, %r10d
	je	.LBB1_46
# %bb.42:                               #   in Loop: Header=BB1_39 Depth=1
	cmpl	$63, %r10d
	jne	.LBB1_49
	jmp	.LBB1_43
	.p2align	4
.LBB1_46:                               #   in Loop: Header=BB1_39 Depth=1
	movzbl	1(%rdx), %r11d
	testb	%r11b, %r11b
	je	.LBB1_49
# %bb.47:                               #   in Loop: Header=BB1_39 Depth=1
	cmpb	%r9b, %r11b
	jne	.LBB1_50
# %bb.48:                               #   in Loop: Header=BB1_39 Depth=1
	addq	$2, %rdx
	jmp	.LBB1_44
	.p2align	4
.LBB1_49:                               #   in Loop: Header=BB1_39 Depth=1
	cmpb	%r9b, %r10b
	jne	.LBB1_50
.LBB1_43:                               #   in Loop: Header=BB1_39 Depth=1
	incq	%rdx
.LBB1_44:                               #   in Loop: Header=BB1_39 Depth=1
	incq	%rsi
	movb	$1, %r9b
	jmp	.LBB1_53
	.p2align	4
.LBB1_50:                               #   in Loop: Header=BB1_39 Depth=1
	testq	%rdi, %rdi
	je	.LBB1_51
# %bb.52:                               #   in Loop: Header=BB1_39 Depth=1
	incq	%r8
	movb	$1, %r9b
	movq	%rdi, %rdx
	movq	%r8, %rsi
	jmp	.LBB1_53
.LBB1_51:                               #   in Loop: Header=BB1_39 Depth=1
	xorl	%edi, %edi
	xorl	%r9d, %r9d
	jmp	.LBB1_53
	.p2align	4
.LBB1_55:                               # =>This Inner Loop Header: Depth=1
	movzbl	(%rdx), %esi
	incq	%rdx
	cmpb	$42, %sil
	je	.LBB1_55
# %bb.56:
	xorl	%edx, %edx
	testb	%sil, %sil
	sete	%dl
	shll	$2, %edx
	jmp	.LBB1_57
.LBB1_54:
	xorl	%edx, %edx
.LBB1_57:
	leaq	.L.str.5(%rip), %rsi
	.p2align	4
.LBB1_58:                               # =>This Inner Loop Header: Depth=1
	movzbl	(%rsi), %edi
	incq	%rsi
	cmpb	$42, %dil
	je	.LBB1_58
# %bb.59:
	xorl	%esi, %esi
	testb	%dil, %dil
	sete	%sil
	orl	%eax, %ecx
	orl	%edx, %ecx
	leal	(%rcx,%rsi,8), %eax
	popq	%rbp
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

	.type	.L.str.5,@object                # @.str.5
.L.str.5:
	.asciz	"*"
	.size	.L.str.5, 2

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
