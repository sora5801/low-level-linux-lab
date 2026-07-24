	.file	"demo.c"
	.text
	.globl	bpf_run                         # -- Begin function bpf_run
	.p2align	4
	.type	bpf_run,@function
bpf_run:                                # @bpf_run
# %bb.0:
	pushq	%rbp
	pushq	%r14
	pushq	%rbx
	xorps	%xmm0, %xmm0
	movaps	%xmm0, -16(%rsp)
	movaps	%xmm0, -32(%rsp)
	movaps	%xmm0, -48(%rsp)
	movaps	%xmm0, -64(%rsp)
	testl	%esi, %esi
	je	.LBB0_83
# %bb.1:
	movl	%ecx, %r8d
	xorl	%eax, %eax
	xorl	%r11d, %r11d
	xorl	%r10d, %r10d
	jmp	.LBB0_2
.LBB0_23:                               #   in Loop: Header=BB0_2 Depth=1
	andl	$15, %ecx
	movl	-64(%rsp,%rcx,4), %ecx
	.p2align	4
.LBB0_39:                               #   in Loop: Header=BB0_2 Depth=1
	incl	%r10d
	movl	%ecx, %eax
.LBB0_82:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	%esi, %r10d
	jae	.LBB0_83
.LBB0_2:                                # =>This Inner Loop Header: Depth=1
	movl	%r10d, %r14d
	movzwl	(%rdi,%r14,8), %ebx
	movl	4(%rdi,%r14,8), %ecx
	movl	%ebx, %r9d
	andl	$7, %r9d
	cmpl	$3, %r9d
	jg	.LBB0_13
# %bb.3:                                #   in Loop: Header=BB0_2 Depth=1
	cmpl	$1, %r9d
	jg	.LBB0_50
# %bb.4:                                #   in Loop: Header=BB0_2 Depth=1
	testl	%r9d, %r9d
	jne	.LBB0_40
# %bb.5:                                #   in Loop: Header=BB0_2 Depth=1
	movl	%ebx, %eax
	shrl	$5, %eax
	andl	$7, %eax
	xorl	%r9d, %r9d
	cmpl	$1, %eax
	jle	.LBB0_6
# %bb.20:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$2, %eax
	je	.LBB0_29
# %bb.21:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$3, %eax
	je	.LBB0_23
# %bb.22:                               #   in Loop: Header=BB0_2 Depth=1
	movl	%r8d, %ecx
	cmpl	$4, %eax
	je	.LBB0_39
	jmp	.LBB0_84
	.p2align	4
.LBB0_13:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$5, %r9d
	jg	.LBB0_79
# %bb.14:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$4, %r9d
	jne	.LBB0_67
# %bb.15:                               #   in Loop: Header=BB0_2 Depth=1
	testb	$8, %bl
	cmovnel	%r11d, %ecx
	shrl	$4, %ebx
	andl	$15, %ebx
	xorl	%r9d, %r9d
	cmpl	$3, %ebx
	jg	.LBB0_56
# %bb.16:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$1, %ebx
	jg	.LBB0_52
# %bb.17:                               #   in Loop: Header=BB0_2 Depth=1
	testl	%ebx, %ebx
	je	.LBB0_63
# %bb.18:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$1, %ebx
	jne	.LBB0_84
# %bb.19:                               #   in Loop: Header=BB0_2 Depth=1
	subl	%ecx, %eax
	incl	%r10d
	jmp	.LBB0_82
	.p2align	4
.LBB0_50:                               #   in Loop: Header=BB0_2 Depth=1
	andl	$15, %ecx
	cmpl	$2, %r9d
	jne	.LBB0_51
# %bb.49:                               #   in Loop: Header=BB0_2 Depth=1
	movl	%eax, -64(%rsp,%rcx,4)
	incl	%r10d
	jmp	.LBB0_82
.LBB0_79:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$7, %r9d
	jne	.LBB0_80
# %bb.81:                               #   in Loop: Header=BB0_2 Depth=1
	testb	%bl, %bl
	cmovnsl	%eax, %r11d
	incl	%r10d
	movl	%r11d, %eax
	jmp	.LBB0_82
.LBB0_40:                               #   in Loop: Header=BB0_2 Depth=1
	shrl	$5, %ebx
	andl	$7, %ebx
	xorl	%r9d, %r9d
	cmpl	$3, %ebx
	jg	.LBB0_44
# %bb.41:                               #   in Loop: Header=BB0_2 Depth=1
	testl	%ebx, %ebx
	je	.LBB0_48
# %bb.42:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$3, %ebx
	jne	.LBB0_84
# %bb.43:                               #   in Loop: Header=BB0_2 Depth=1
	andl	$15, %ecx
	movl	-64(%rsp,%rcx,4), %ecx
	jmp	.LBB0_48
.LBB0_51:                               #   in Loop: Header=BB0_2 Depth=1
	movl	%r11d, -64(%rsp,%rcx,4)
	incl	%r10d
	jmp	.LBB0_82
.LBB0_67:                               #   in Loop: Header=BB0_2 Depth=1
	movl	%ebx, %ebp
	andl	$240, %ebp
	je	.LBB0_68
# %bb.69:                               #   in Loop: Header=BB0_2 Depth=1
	testb	$8, %bl
	cmovnel	%r11d, %ecx
	addl	$-16, %ebp
	shrl	$4, %ebp
	xorl	%r9d, %r9d
	cmpl	$1, %ebp
	jg	.LBB0_73
# %bb.70:                               #   in Loop: Header=BB0_2 Depth=1
	testl	%ebp, %ebp
	je	.LBB0_76
# %bb.71:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$1, %ebp
	jne	.LBB0_84
# %bb.72:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	%ecx, %eax
	seta	%cl
	jmp	.LBB0_78
.LBB0_44:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$5, %ebx
	je	.LBB0_46
# %bb.45:                               #   in Loop: Header=BB0_2 Depth=1
	movl	%r8d, %ecx
	cmpl	$4, %ebx
	je	.LBB0_48
	jmp	.LBB0_84
.LBB0_56:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$5, %ebx
	jg	.LBB0_60
# %bb.57:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$4, %ebx
	je	.LBB0_65
# %bb.58:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$5, %ebx
	jne	.LBB0_84
# %bb.59:                               #   in Loop: Header=BB0_2 Depth=1
	andl	%eax, %ecx
	jmp	.LBB0_39
.LBB0_6:                                #   in Loop: Header=BB0_2 Depth=1
	testl	%eax, %eax
	je	.LBB0_39
# %bb.7:                                #   in Loop: Header=BB0_2 Depth=1
	cmpl	$1, %eax
	jne	.LBB0_84
# %bb.8:                                #   in Loop: Header=BB0_2 Depth=1
	andl	$24, %ebx
	cmpl	$8, %ebx
	je	.LBB0_24
# %bb.9:                                #   in Loop: Header=BB0_2 Depth=1
	testl	%ebx, %ebx
	jne	.LBB0_27
# %bb.10:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$-5, %ecx
	ja	.LBB0_84
# %bb.11:                               #   in Loop: Header=BB0_2 Depth=1
	movl	%ecx, %eax
	addl	$4, %eax
	cmpl	%r8d, %eax
	ja	.LBB0_84
# %bb.12:                               #   in Loop: Header=BB0_2 Depth=1
	movzbl	(%rdx,%rcx), %eax
	shll	$24, %eax
	movzbl	1(%rcx,%rdx), %r9d
	shll	$16, %r9d
	orl	%eax, %r9d
	movzbl	2(%rcx,%rdx), %eax
	shll	$8, %eax
	orl	%r9d, %eax
	movzbl	3(%rcx,%rdx), %ecx
	orl	%eax, %ecx
	jmp	.LBB0_39
.LBB0_68:                               #   in Loop: Header=BB0_2 Depth=1
	addl	%ecx, %r10d
	incl	%r10d
	jmp	.LBB0_82
.LBB0_73:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$2, %ebp
	je	.LBB0_77
# %bb.74:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$3, %ebp
	jne	.LBB0_84
# %bb.75:                               #   in Loop: Header=BB0_2 Depth=1
	testl	%eax, %ecx
	setne	%cl
	jmp	.LBB0_78
.LBB0_52:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$2, %ebx
	je	.LBB0_64
# %bb.53:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$3, %ebx
	jne	.LBB0_84
# %bb.54:                               #   in Loop: Header=BB0_2 Depth=1
	testl	%ecx, %ecx
	je	.LBB0_84
# %bb.55:                               #   in Loop: Header=BB0_2 Depth=1
	movq	%rdx, %r9
	xorl	%edx, %edx
	divl	%ecx
	movq	%r9, %rdx
	incl	%r10d
	jmp	.LBB0_82
.LBB0_60:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$6, %ebx
	je	.LBB0_66
# %bb.61:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$7, %ebx
	jne	.LBB0_84
# %bb.62:                               #   in Loop: Header=BB0_2 Depth=1
                                        # kill: def $cl killed $cl killed $rcx
	shrl	%cl, %eax
	incl	%r10d
	jmp	.LBB0_82
.LBB0_46:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	%r8d, %ecx
	jae	.LBB0_84
# %bb.47:                               #   in Loop: Header=BB0_2 Depth=1
	movzbl	(%rdx,%rcx), %ecx
	shll	$2, %ecx
	andl	$60, %ecx
.LBB0_48:                               #   in Loop: Header=BB0_2 Depth=1
	incl	%r10d
	movl	%ecx, %r11d
	jmp	.LBB0_82
.LBB0_29:                               #   in Loop: Header=BB0_2 Depth=1
	andl	$24, %ebx
	addl	%r11d, %ecx
	cmpl	$8, %ebx
	je	.LBB0_34
# %bb.30:                               #   in Loop: Header=BB0_2 Depth=1
	testl	%ebx, %ebx
	jne	.LBB0_37
# %bb.31:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$-5, %ecx
	ja	.LBB0_84
# %bb.32:                               #   in Loop: Header=BB0_2 Depth=1
	leal	4(%rcx), %eax
	cmpl	%r8d, %eax
	ja	.LBB0_84
# %bb.33:                               #   in Loop: Header=BB0_2 Depth=1
	movl	%ecx, %eax
	movl	(%rdx,%rax), %ecx
	bswapl	%ecx
	jmp	.LBB0_39
.LBB0_76:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	%ecx, %eax
	sete	%cl
	jmp	.LBB0_78
.LBB0_77:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	%ecx, %eax
	setae	%cl
.LBB0_78:                               #   in Loop: Header=BB0_2 Depth=1
	leaq	(%rdi,%r14,8), %r9
	movzbl	%cl, %ecx
	xorq	$3, %rcx
	movzbl	(%r9,%rcx), %ecx
	addl	%ecx, %r10d
	incl	%r10d
	jmp	.LBB0_82
.LBB0_63:                               #   in Loop: Header=BB0_2 Depth=1
	addl	%eax, %ecx
	jmp	.LBB0_39
.LBB0_65:                               #   in Loop: Header=BB0_2 Depth=1
	orl	%eax, %ecx
	jmp	.LBB0_39
.LBB0_64:                               #   in Loop: Header=BB0_2 Depth=1
	imull	%eax, %ecx
	jmp	.LBB0_39
.LBB0_66:                               #   in Loop: Header=BB0_2 Depth=1
                                        # kill: def $cl killed $cl killed $rcx
	shll	%cl, %eax
	incl	%r10d
	jmp	.LBB0_82
.LBB0_34:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$-3, %ecx
	ja	.LBB0_84
# %bb.35:                               #   in Loop: Header=BB0_2 Depth=1
	leal	2(%rcx), %eax
	cmpl	%r8d, %eax
	ja	.LBB0_84
# %bb.36:                               #   in Loop: Header=BB0_2 Depth=1
	movl	%ecx, %eax
	movzwl	(%rdx,%rax), %eax
	rolw	$8, %ax
	movzwl	%ax, %ecx
	jmp	.LBB0_39
.LBB0_24:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	$-3, %ecx
	ja	.LBB0_84
# %bb.25:                               #   in Loop: Header=BB0_2 Depth=1
	movl	%ecx, %eax
	addl	$2, %eax
	cmpl	%r8d, %eax
	ja	.LBB0_84
# %bb.26:                               #   in Loop: Header=BB0_2 Depth=1
	movzbl	(%rdx,%rcx), %eax
	shll	$8, %eax
	movzbl	1(%rcx,%rdx), %ecx
	orl	%eax, %ecx
	jmp	.LBB0_39
.LBB0_37:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	%r8d, %ecx
	jae	.LBB0_84
# %bb.38:                               #   in Loop: Header=BB0_2 Depth=1
	movl	%ecx, %eax
	movzbl	(%rdx,%rax), %ecx
	jmp	.LBB0_39
.LBB0_27:                               #   in Loop: Header=BB0_2 Depth=1
	cmpl	%r8d, %ecx
	jae	.LBB0_84
# %bb.28:                               #   in Loop: Header=BB0_2 Depth=1
	movzbl	(%rdx,%rcx), %ecx
	jmp	.LBB0_39
.LBB0_83:
	xorl	%r9d, %r9d
.LBB0_84:
	movl	%r9d, %eax
	popq	%rbx
	popq	%r14
	popq	%rbp
	retq
.LBB0_80:
	andl	$24, %ebx
	cmpl	$16, %ebx
	cmovel	%eax, %ecx
	movl	%ecx, %r9d
	jmp	.LBB0_84
.Lfunc_end0:
	.size	bpf_run, .Lfunc_end0-bpf_run
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
