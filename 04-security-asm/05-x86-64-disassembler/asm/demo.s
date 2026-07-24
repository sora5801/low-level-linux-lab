	.file	"demo.c"
	.section	.rodata.cst16,"aM",@progbits,16
	.p2align	4, 0x0                          # -- Begin function decode_modrm
.LCPI0_0:
	.long	1                               # 0x1
	.long	0                               # 0x0
	.long	0                               # 0x0
	.long	0                               # 0x0
	.section	.rodata.cst4,"aM",@progbits,4
	.p2align	2, 0x0
.LCPI0_1:
	.long	1                               # 0x1
	.text
	.globl	decode_modrm
	.p2align	4
	.type	decode_modrm,@function
decode_modrm:                           # @decode_modrm
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%r14
	pushq	%rbx
                                        # kill: def $r9d killed $r9d def $r9
                                        # kill: def $r8d killed $r8d def $r8
                                        # kill: def $ecx killed $ecx def $rcx
	movq	%rdi, %rax
	xorps	%xmm0, %xmm0
	movups	%xmm0, 12(%rdi)
	movups	%xmm0, (%rdi)
	movss	.LCPI0_1(%rip), %xmm0           # xmm0 = [1,0,0,0]
	movups	%xmm0, 28(%rdi)
	movq	$0, 48(%rdi)
	movl	$0, 56(%rdi)
	movl	$-1, %edi
	testl	%edx, %edx
	jle	.LBB0_34
# %bb.1:
	movzbl	(%rsi), %r11d
	movl	%r11d, %r10d
	shrl	$6, %r10d
	movl	%r11d, %r14d
	shrl	$3, %r14d
	andl	$7, %r14d
	movl	%r11d, %ebx
	andl	$7, %ebx
	leal	(%r14,%rcx,8), %ecx
	movl	%ecx, 4(%rax)
	cmpl	$3, %r10d
	jne	.LBB0_3
# %bb.2:
	movl	$1, (%rax)
	leal	(%rbx,%r9,8), %ecx
	movl	%ecx, 8(%rax)
	movl	$1, %edi
.LBB0_34:
	movl	%edi, 56(%rax)
.LBB0_35:
	popq	%rbx
	popq	%r14
	popq	%rbp
	retq
.LBB0_3:
	cmpl	$4, %ebx
	jne	.LBB0_18
# %bb.4:
	cmpl	$1, %edx
	je	.LBB0_34
# %bb.5:
	movzbl	1(%rsi), %ecx
	movl	%ecx, %r14d
	shrl	$3, %r14d
	andl	$7, %r14d
	movl	%ecx, %ebx
	andl	$7, %ebx
	testl	%r8d, %r8d
	jne	.LBB0_8
# %bb.6:
	cmpl	$4, %r14d
	jne	.LBB0_8
# %bb.7:
	movl	$0, 20(%rax)
	jmp	.LBB0_9
.LBB0_18:
	cmpb	$64, %r11b
	setae	%cl
	cmpl	$5, %ebx
	setne	%r8b
	orb	%cl, %r8b
	jne	.LBB0_23
# %bb.19:
	movl	$1, 32(%rax)
	cmpl	$5, %edx
	jl	.LBB0_34
# %bb.20:
	incq	%rsi
	xorl	%ecx, %ecx
	xorl	%edx, %edx
	.p2align	4
.LBB0_21:                               # =>This Inner Loop Header: Depth=1
	movzbl	(%rsi), %edi
	shlq	%cl, %rdi
	orq	%rdi, %rdx
	addq	$8, %rcx
	incq	%rsi
	cmpq	$32, %rcx
	jne	.LBB0_21
# %bb.22:
	movslq	%edx, %rcx
	movabsq	$-4294967296, %rsi              # imm = 0xFFFFFFFF00000000
	andq	%rcx, %rsi
	orq	%rdx, %rsi
	movq	%rsi, 48(%rax)
	movabsq	$17179869185, %rcx              # imm = 0x400000001
	movq	%rcx, 36(%rax)
	movl	$5, %edi
	jmp	.LBB0_34
.LBB0_23:
	movl	$1, 12(%rax)
	leal	(%rbx,%r9,8), %ecx
	movl	%ecx, 16(%rax)
	movl	$1, %ecx
	jmp	.LBB0_24
.LBB0_8:
	shrl	$6, %ecx
	movl	$1, 20(%rax)
	leal	(%r14,%r8,8), %r8d
	movl	%r8d, 24(%rax)
	movl	$1, %r8d
                                        # kill: def $cl killed $cl killed $ecx
	shll	%cl, %r8d
	movl	%r8d, 28(%rax)
.LBB0_9:
	cmpl	$5, %ebx
	setne	%cl
	cmpb	$64, %r11b
	setae	%r8b
	orb	%cl, %r8b
	je	.LBB0_10
# %bb.15:
	movl	$1, 12(%rax)
	leal	(%rbx,%r9,8), %ecx
	movl	%ecx, 16(%rax)
	jmp	.LBB0_16
.LBB0_10:
	movl	$0, 12(%rax)
	cmpl	$5, %edx
	jg	.LBB0_12
# %bb.11:
	movl	$-1, 56(%rax)
.LBB0_16:
	movl	$2, %ecx
	jmp	.LBB0_17
.LBB0_12:
	leaq	2(%rsi), %r11
	xorl	%ecx, %ecx
	xorl	%r9d, %r9d
	.p2align	4
.LBB0_13:                               # =>This Inner Loop Header: Depth=1
	movzbl	(%r11), %ebx
	shlq	%cl, %rbx
	orq	%rbx, %r9
	addq	$8, %rcx
	incq	%r11
	cmpq	$32, %rcx
	jne	.LBB0_13
# %bb.14:
	movslq	%r9d, %rcx
	movabsq	$-4294967296, %r11              # imm = 0xFFFFFFFF00000000
	andq	%rcx, %r11
	orq	%r9, %r11
	movq	%r11, 48(%rax)
	movabsq	$17179869185, %rcx              # imm = 0x400000001
	movq	%rcx, 36(%rax)
	movl	$6, 56(%rax)
	movl	$6, %ecx
.LBB0_17:
	testb	%r8b, %r8b
	je	.LBB0_35
.LBB0_24:
	cmpl	$2, %r10d
	je	.LBB0_29
# %bb.25:
	cmpl	$1, %r10d
	jne	.LBB0_26
# %bb.27:
	cmpl	%edx, %ecx
	jge	.LBB0_34
# %bb.28:
	movl	%ecx, %edx
	movzbl	(%rsi,%rdx), %edx
	incl	%ecx
	xorl	%esi, %esi
	testb	%dl, %dl
	setns	%sil
	shll	$8, %esi
	addq	%rdx, %rsi
	addq	$-256, %rsi
	movl	$1, %edx
	movl	%ecx, %r8d
	jmp	.LBB0_33
.LBB0_29:
	leal	4(%rcx), %r8d
	cmpl	%edx, %r8d
	jg	.LBB0_34
# %bb.30:
	movl	%ecx, %ecx
	addq	%rcx, %rsi
	xorl	%ecx, %ecx
	xorl	%edx, %edx
	.p2align	4
.LBB0_31:                               # =>This Inner Loop Header: Depth=1
	movzbl	(%rsi), %edi
	shlq	%cl, %rdi
	orq	%rdi, %rdx
	addq	$8, %rcx
	incq	%rsi
	cmpq	$32, %rcx
	jne	.LBB0_31
# %bb.32:
	movslq	%edx, %rcx
	movabsq	$-4294967296, %rsi              # imm = 0xFFFFFFFF00000000
	andq	%rcx, %rsi
	orq	%rdx, %rsi
	movl	$4, %edx
.LBB0_33:
	movq	%rsi, 48(%rax)
	movl	%edx, 40(%rax)
	movl	$1, 36(%rax)
	movl	%r8d, %edi
	jmp	.LBB0_34
.LBB0_26:
	movl	%ecx, %edi
	jmp	.LBB0_34
.Lfunc_end0:
	.size	decode_modrm, .Lfunc_end0-decode_modrm
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
