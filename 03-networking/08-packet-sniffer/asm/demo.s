	.file	"demo.c"
	.text
	.globl	bpf_run                         # -- Begin function bpf_run
	.p2align	4
	.type	bpf_run,@function
bpf_run:                                # @bpf_run
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movl	%ecx, %r8d
	xorps	%xmm0, %xmm0
	movaps	%xmm0, -64(%rbp)
	movaps	%xmm0, -80(%rbp)
	movaps	%xmm0, -96(%rbp)
	movaps	%xmm0, -112(%rbp)
	xorl	%r11d, %r11d
	xorl	%r10d, %r10d
	xorl	%ebx, %ebx
	xorl	%eax, %eax
                                        # implicit-def: $r9d
	jmp	.LBB0_4
.LBB0_1:                                #   in Loop: Header=BB0_4 Depth=1
	testb	%r15b, %r15b
	cmovnsl	%eax, %ebx
	incl	%r10d
	movl	%ebx, %eax
	.p2align	4
.LBB0_2:                                #   in Loop: Header=BB0_4 Depth=1
	movb	$1, %r14b
.LBB0_3:                                #   in Loop: Header=BB0_4 Depth=1
	testb	%r14b, %r14b
	je	.LBB0_106
.LBB0_4:                                # =>This Inner Loop Header: Depth=1
	cmpl	%esi, %r10d
	jae	.LBB0_105
# %bb.5:                                #   in Loop: Header=BB0_4 Depth=1
	movl	%r10d, %r14d
	movzwl	(%rdi,%r14,8), %r15d
	movl	4(%rdi,%r14,8), %ecx
	movl	%r15d, %r12d
	andl	$7, %r12d
	cmpl	$3, %r12d
	jg	.LBB0_13
# %bb.6:                                #   in Loop: Header=BB0_4 Depth=1
	cmpl	$1, %r12d
	jg	.LBB0_20
# %bb.7:                                #   in Loop: Header=BB0_4 Depth=1
	testl	%r12d, %r12d
	jne	.LBB0_24
# %bb.8:                                #   in Loop: Header=BB0_4 Depth=1
	movl	%r15d, %r12d
	shrl	$5, %r12d
	andl	$7, %r12d
	xorl	%r14d, %r14d
	cmpl	$1, %r12d
	jle	.LBB0_41
# %bb.9:                                #   in Loop: Header=BB0_4 Depth=1
	cmpl	$2, %r12d
	je	.LBB0_64
# %bb.10:                               #   in Loop: Header=BB0_4 Depth=1
	cmpl	$3, %r12d
	je	.LBB0_69
# %bb.11:                               #   in Loop: Header=BB0_4 Depth=1
	movl	%r8d, %ecx
	cmpl	$4, %r12d
	je	.LBB0_99
	jmp	.LBB0_83
	.p2align	4
.LBB0_13:                               #   in Loop: Header=BB0_4 Depth=1
	cmpl	$5, %r12d
	jg	.LBB0_22
# %bb.14:                               #   in Loop: Header=BB0_4 Depth=1
	cmpl	$4, %r12d
	jne	.LBB0_28
# %bb.15:                               #   in Loop: Header=BB0_4 Depth=1
	testb	$8, %r15b
	cmovnel	%ebx, %ecx
	shrl	$4, %r15d
	andl	$15, %r15d
	xorl	%r14d, %r14d
	cmpl	$3, %r15d
	jg	.LBB0_34
# %bb.16:                               #   in Loop: Header=BB0_4 Depth=1
	cmpl	$1, %r15d
	jg	.LBB0_51
# %bb.17:                               #   in Loop: Header=BB0_4 Depth=1
	testl	%r15d, %r15d
	je	.LBB0_75
# %bb.18:                               #   in Loop: Header=BB0_4 Depth=1
	cmpl	$1, %r15d
	jne	.LBB0_101
# %bb.19:                               #   in Loop: Header=BB0_4 Depth=1
	subl	%ecx, %eax
	jmp	.LBB0_79
	.p2align	4
.LBB0_20:                               #   in Loop: Header=BB0_4 Depth=1
	andl	$15, %ecx
	cmpl	$2, %r12d
	jne	.LBB0_33
# %bb.21:                               #   in Loop: Header=BB0_4 Depth=1
	movl	%eax, -112(%rbp,%rcx,4)
	incl	%r10d
	jmp	.LBB0_2
	.p2align	4
.LBB0_22:                               #   in Loop: Header=BB0_4 Depth=1
	cmpl	$6, %r12d
	jne	.LBB0_1
# %bb.23:                               #   in Loop: Header=BB0_4 Depth=1
	andl	$24, %r15d
	cmpl	$16, %r15d
	cmovel	%eax, %ecx
	xorl	%r14d, %r14d
	movl	%ecx, %r9d
	jmp	.LBB0_3
.LBB0_24:                               #   in Loop: Header=BB0_4 Depth=1
	shrl	$5, %r15d
	andl	$7, %r15d
	xorl	%r14d, %r14d
	cmpl	$3, %r15d
	jg	.LBB0_38
# %bb.25:                               #   in Loop: Header=BB0_4 Depth=1
	testl	%r15d, %r15d
	je	.LBB0_58
# %bb.26:                               #   in Loop: Header=BB0_4 Depth=1
	cmpl	$3, %r15d
	jne	.LBB0_83
# %bb.27:                               #   in Loop: Header=BB0_4 Depth=1
	andl	$15, %ecx
	movl	-112(%rbp,%rcx,4), %r12d
	movl	%r11d, %r13d
	jmp	.LBB0_63
.LBB0_28:                               #   in Loop: Header=BB0_4 Depth=1
	movl	%r15d, %r12d
	andl	$240, %r12d
	je	.LBB0_47
# %bb.29:                               #   in Loop: Header=BB0_4 Depth=1
	testb	$8, %r15b
	cmovnel	%ebx, %ecx
	addl	$-16, %r12d
	shrl	$4, %r12d
	xorl	%r15d, %r15d
	cmpl	$1, %r12d
	jg	.LBB0_48
# %bb.30:                               #   in Loop: Header=BB0_4 Depth=1
	testl	%r12d, %r12d
	je	.LBB0_70
# %bb.31:                               #   in Loop: Header=BB0_4 Depth=1
	cmpl	$1, %r12d
	jne	.LBB0_85
# %bb.32:                               #   in Loop: Header=BB0_4 Depth=1
	cmpl	%ecx, %eax
	seta	%cl
	jmp	.LBB0_72
.LBB0_33:                               #   in Loop: Header=BB0_4 Depth=1
	movl	%ebx, -112(%rbp,%rcx,4)
	incl	%r10d
	jmp	.LBB0_2
.LBB0_34:                               #   in Loop: Header=BB0_4 Depth=1
	cmpl	$5, %r15d
	jg	.LBB0_55
# %bb.35:                               #   in Loop: Header=BB0_4 Depth=1
	cmpl	$4, %r15d
	je	.LBB0_76
# %bb.36:                               #   in Loop: Header=BB0_4 Depth=1
	cmpl	$5, %r15d
	jne	.LBB0_101
# %bb.37:                               #   in Loop: Header=BB0_4 Depth=1
	andl	%eax, %ecx
	movl	%ecx, %eax
	jmp	.LBB0_79
.LBB0_38:                               #   in Loop: Header=BB0_4 Depth=1
	cmpl	$5, %r15d
	je	.LBB0_59
# %bb.39:                               #   in Loop: Header=BB0_4 Depth=1
	movl	%r11d, %r13d
	movl	%r8d, %r12d
	cmpl	$4, %r15d
	je	.LBB0_63
	jmp	.LBB0_83
.LBB0_41:                               #   in Loop: Header=BB0_4 Depth=1
	testl	%r12d, %r12d
	je	.LBB0_99
# %bb.42:                               #   in Loop: Header=BB0_4 Depth=1
	cmpl	$1, %r12d
	jne	.LBB0_83
# %bb.43:                               #   in Loop: Header=BB0_4 Depth=1
	andl	$24, %r15d
	cmpl	$8, %r15d
	je	.LBB0_88
# %bb.44:                               #   in Loop: Header=BB0_4 Depth=1
	testl	%r15d, %r15d
	jne	.LBB0_94
# %bb.45:                               #   in Loop: Header=BB0_4 Depth=1
	leal	4(%rcx), %eax
	cmpl	%r8d, %eax
	seta	%al
	cmpl	$-4, %ecx
	setae	%r14b
	orb	%al, %r14b
	jne	.LBB0_96
# %bb.46:                               #   in Loop: Header=BB0_4 Depth=1
	movzbl	(%rdx,%rcx), %eax
	jmp	.LBB0_68
.LBB0_47:                               #   in Loop: Header=BB0_4 Depth=1
	addl	%ecx, %r10d
	incl	%r10d
	jmp	.LBB0_2
.LBB0_83:                               #   in Loop: Header=BB0_4 Depth=1
	xorl	%r9d, %r9d
	jmp	.LBB0_3
.LBB0_48:                               #   in Loop: Header=BB0_4 Depth=1
	cmpl	$2, %r12d
	je	.LBB0_71
# %bb.49:                               #   in Loop: Header=BB0_4 Depth=1
	cmpl	$3, %r12d
	jne	.LBB0_85
# %bb.50:                               #   in Loop: Header=BB0_4 Depth=1
	testl	%eax, %ecx
	setne	%cl
	jmp	.LBB0_72
.LBB0_51:                               #   in Loop: Header=BB0_4 Depth=1
	cmpl	$2, %r15d
	je	.LBB0_77
# %bb.52:                               #   in Loop: Header=BB0_4 Depth=1
	cmpl	$3, %r15d
	jne	.LBB0_101
# %bb.53:                               #   in Loop: Header=BB0_4 Depth=1
	testl	%ecx, %ecx
	je	.LBB0_100
# %bb.54:                               #   in Loop: Header=BB0_4 Depth=1
	movq	%rdx, %r14
	xorl	%edx, %edx
	divl	%ecx
	movq	%r14, %rdx
	jmp	.LBB0_79
.LBB0_55:                               #   in Loop: Header=BB0_4 Depth=1
	cmpl	$6, %r15d
	je	.LBB0_78
# %bb.56:                               #   in Loop: Header=BB0_4 Depth=1
	cmpl	$7, %r15d
	jne	.LBB0_101
# %bb.57:                               #   in Loop: Header=BB0_4 Depth=1
                                        # kill: def $cl killed $cl killed $rcx
	shrl	%cl, %eax
	jmp	.LBB0_79
.LBB0_58:                               #   in Loop: Header=BB0_4 Depth=1
	movl	%r11d, %r13d
	movl	%ecx, %r12d
	jmp	.LBB0_63
.LBB0_59:                               #   in Loop: Header=BB0_4 Depth=1
	xorl	%r14d, %r14d
	movl	$1, %r13d
	movl	$0, %r12d
	cmpl	%r8d, %ecx
	jae	.LBB0_61
# %bb.60:                               #   in Loop: Header=BB0_4 Depth=1
	movzbl	(%rdx,%rcx), %r12d
	shll	$2, %r12d
	andl	$60, %r12d
	movl	%r11d, %r13d
.LBB0_61:                               #   in Loop: Header=BB0_4 Depth=1
	testl	%r13d, %r13d
	je	.LBB0_63
# %bb.62:                               #   in Loop: Header=BB0_4 Depth=1
	movl	%r13d, %r11d
	movl	%r12d, %ebx
	xorl	%r9d, %r9d
	jmp	.LBB0_3
.LBB0_63:                               #   in Loop: Header=BB0_4 Depth=1
	incl	%r10d
	movl	%r13d, %r11d
	movl	%r12d, %ebx
	jmp	.LBB0_2
.LBB0_64:                               #   in Loop: Header=BB0_4 Depth=1
	andl	$24, %r15d
	addl	%ebx, %ecx
	cmpl	$8, %r15d
	je	.LBB0_86
# %bb.65:                               #   in Loop: Header=BB0_4 Depth=1
	testl	%r15d, %r15d
	jne	.LBB0_92
# %bb.66:                               #   in Loop: Header=BB0_4 Depth=1
	leal	4(%rcx), %eax
	cmpl	%r8d, %eax
	seta	%al
	cmpl	$-4, %ecx
	setae	%r14b
	orb	%al, %r14b
	jne	.LBB0_96
# %bb.67:                               #   in Loop: Header=BB0_4 Depth=1
	movl	%ecx, %eax
	movzbl	(%rdx,%rax), %eax
.LBB0_68:                               #   in Loop: Header=BB0_4 Depth=1
	shll	$24, %eax
	leal	1(%rcx), %r14d
	movzbl	(%rdx,%r14), %r14d
	shll	$16, %r14d
	orl	%eax, %r14d
	leal	2(%rcx), %eax
	movzbl	(%rdx,%rax), %eax
	shll	$8, %eax
	orl	%r14d, %eax
	addl	$3, %ecx
	jmp	.LBB0_91
.LBB0_69:                               #   in Loop: Header=BB0_4 Depth=1
	andl	$15, %ecx
	movl	-112(%rbp,%rcx,4), %ecx
	jmp	.LBB0_99
.LBB0_85:                               #   in Loop: Header=BB0_4 Depth=1
	xorl	%r9d, %r9d
	jmp	.LBB0_73
.LBB0_70:                               #   in Loop: Header=BB0_4 Depth=1
	cmpl	%ecx, %eax
	sete	%cl
	jmp	.LBB0_72
.LBB0_71:                               #   in Loop: Header=BB0_4 Depth=1
	cmpl	%ecx, %eax
	setae	%cl
.LBB0_72:                               #   in Loop: Header=BB0_4 Depth=1
	leaq	(%rdi,%r14,8), %r14
	movzbl	%cl, %ecx
	xorq	$3, %rcx
	movzbl	(%r14,%rcx), %ecx
	addl	%ecx, %r10d
	incl	%r10d
	movb	$1, %r15b
.LBB0_73:                               #   in Loop: Header=BB0_4 Depth=1
	testb	%r15b, %r15b
	jne	.LBB0_2
	jmp	.LBB0_81
.LBB0_75:                               #   in Loop: Header=BB0_4 Depth=1
	addl	%eax, %ecx
	movl	%ecx, %eax
	jmp	.LBB0_79
.LBB0_76:                               #   in Loop: Header=BB0_4 Depth=1
	orl	%eax, %ecx
	movl	%ecx, %eax
	jmp	.LBB0_79
.LBB0_77:                               #   in Loop: Header=BB0_4 Depth=1
	imull	%eax, %ecx
	movl	%ecx, %eax
	jmp	.LBB0_79
.LBB0_78:                               #   in Loop: Header=BB0_4 Depth=1
                                        # kill: def $cl killed $cl killed $rcx
	shll	%cl, %eax
.LBB0_79:                               #   in Loop: Header=BB0_4 Depth=1
	incl	%r10d
	movb	$1, %r14b
	testb	%r14b, %r14b
	jne	.LBB0_2
.LBB0_81:                               #   in Loop: Header=BB0_4 Depth=1
	xorl	%r14d, %r14d
	jmp	.LBB0_3
.LBB0_86:                               #   in Loop: Header=BB0_4 Depth=1
	leal	2(%rcx), %eax
	cmpl	%r8d, %eax
	seta	%al
	cmpl	$-2, %ecx
	setae	%r14b
	orb	%al, %r14b
	jne	.LBB0_96
# %bb.87:                               #   in Loop: Header=BB0_4 Depth=1
	movl	%ecx, %eax
	movzbl	(%rdx,%rax), %eax
	jmp	.LBB0_90
.LBB0_88:                               #   in Loop: Header=BB0_4 Depth=1
	leal	2(%rcx), %eax
	cmpl	%r8d, %eax
	seta	%al
	cmpl	$-2, %ecx
	setae	%r14b
	orb	%al, %r14b
	jne	.LBB0_96
# %bb.89:                               #   in Loop: Header=BB0_4 Depth=1
	movzbl	(%rdx,%rcx), %eax
.LBB0_90:                               #   in Loop: Header=BB0_4 Depth=1
	shll	$8, %eax
	incl	%ecx
.LBB0_91:                               #   in Loop: Header=BB0_4 Depth=1
	movzbl	(%rdx,%rcx), %ecx
	orl	%eax, %ecx
	jmp	.LBB0_97
.LBB0_92:                               #   in Loop: Header=BB0_4 Depth=1
	cmpl	%r8d, %ecx
	jae	.LBB0_96
# %bb.93:                               #   in Loop: Header=BB0_4 Depth=1
	movl	%ecx, %eax
	movzbl	(%rdx,%rax), %ecx
	jmp	.LBB0_97
.LBB0_94:                               #   in Loop: Header=BB0_4 Depth=1
	cmpl	%r8d, %ecx
	jae	.LBB0_96
# %bb.95:                               #   in Loop: Header=BB0_4 Depth=1
	movzbl	(%rdx,%rcx), %ecx
	jmp	.LBB0_97
.LBB0_96:                               #   in Loop: Header=BB0_4 Depth=1
	xorl	%ecx, %ecx
	movl	$1, %r11d
.LBB0_97:                               #   in Loop: Header=BB0_4 Depth=1
	xorl	%r14d, %r14d
	testl	%r11d, %r11d
	je	.LBB0_99
# %bb.98:                               #   in Loop: Header=BB0_4 Depth=1
	movl	%ecx, %eax
	xorl	%r9d, %r9d
	jmp	.LBB0_3
.LBB0_99:                               #   in Loop: Header=BB0_4 Depth=1
	incl	%r10d
	movl	%ecx, %eax
	jmp	.LBB0_2
.LBB0_100:                              #   in Loop: Header=BB0_4 Depth=1
	xorl	%r14d, %r14d
.LBB0_101:                              #   in Loop: Header=BB0_4 Depth=1
	xorl	%r9d, %r9d
	testb	%r14b, %r14b
	jne	.LBB0_2
	jmp	.LBB0_81
.LBB0_105:
	xorl	%r9d, %r9d
.LBB0_106:
	movl	%r9d, %eax
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
.Lfunc_end0:
	.size	bpf_run, .Lfunc_end0-bpf_run
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
