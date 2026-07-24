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
	movl	$-1, %edi
	testl	%edx, %edx
	jle	.LBB0_22
# %bb.1:
	movzbl	(%rsi), %ebx
	movl	%ebx, %r10d
	shrl	$6, %r10d
	movl	%ebx, %r14d
	shrl	$3, %r14d
	andl	$7, %r14d
	movl	%ebx, %r11d
	andl	$7, %r11d
	leal	(%r14,%rcx,8), %ecx
	movl	%ecx, 4(%rax)
	cmpl	$3, %r10d
	jne	.LBB0_3
# %bb.2:
	movl	$1, (%rax)
	leal	(%r11,%r9,8), %ecx
	movl	%ecx, 8(%rax)
	movl	$1, %edi
.LBB0_22:
	movl	%edi, 56(%rax)
	popq	%rbx
	popq	%r14
	popq	%rbp
	retq
.LBB0_3:
	cmpl	$4, %r11d
	jne	.LBB0_11
# %bb.4:
	cmpl	$1, %edx
	je	.LBB0_22
# %bb.5:
	movzbl	1(%rsi), %ecx
	movl	%ecx, %r14d
	shrl	$3, %r14d
	andl	$7, %r14d
	movl	%ecx, %r11d
	andl	$7, %r11d
	testl	%r8d, %r8d
	jne	.LBB0_7
# %bb.6:
	xorl	%ebp, %ebp
	cmpl	$4, %r14d
	je	.LBB0_8
.LBB0_7:
	shrl	$6, %ecx
	leal	(%r14,%r8,8), %r8d
	movl	%r8d, 24(%rax)
	movl	$1, %ebp
	movl	$1, %r8d
                                        # kill: def $cl killed $cl killed $ecx
	shll	%cl, %r8d
	movl	%r8d, 28(%rax)
.LBB0_8:
	movl	%ebp, 20(%rax)
	cmpl	$5, %r11d
	setne	%cl
	cmpb	$64, %bl
	setae	%r8b
	orb	%cl, %r8b
	movl	$2, %ecx
	jne	.LBB0_14
# %bb.9:
	movl	$0, 12(%rax)
	cmpl	$6, %edx
	jb	.LBB0_22
# %bb.10:
	movzwl	2(%rsi), %ecx
	movzbl	4(%rsi), %edx
	shll	$16, %edx
	orq	%rcx, %rdx
	movzbl	5(%rsi), %ecx
	movl	%ecx, %esi
	shll	$24, %esi
	orq	%rdx, %rsi
	xorl	%edx, %edx
	testb	%cl, %cl
	setns	%dl
	shlq	$32, %rdx
	orq	%rsi, %rdx
	movabsq	$-4294967296, %rcx              # imm = 0xFFFFFFFF00000000
	addq	%rdx, %rcx
	movq	%rcx, 48(%rax)
	movabsq	$17179869185, %rcx              # imm = 0x400000001
	movq	%rcx, 36(%rax)
	movl	$6, %edi
	jmp	.LBB0_22
.LBB0_11:
	cmpb	$64, %bl
	setae	%r8b
	cmpl	$5, %r11d
	setne	%bl
	movl	$1, %ecx
	orb	%r8b, %bl
	jne	.LBB0_14
# %bb.12:
	movl	$1, 32(%rax)
	cmpl	$5, %edx
	jb	.LBB0_22
# %bb.13:
	movzwl	1(%rsi), %ecx
	movzbl	3(%rsi), %edx
	shll	$16, %edx
	orq	%rcx, %rdx
	movzbl	4(%rsi), %ecx
	movl	%ecx, %esi
	shll	$24, %esi
	orq	%rdx, %rsi
	xorl	%edx, %edx
	testb	%cl, %cl
	setns	%dl
	shlq	$32, %rdx
	orq	%rsi, %rdx
	movabsq	$-4294967296, %rcx              # imm = 0xFFFFFFFF00000000
	addq	%rdx, %rcx
	movq	%rcx, 48(%rax)
	movabsq	$17179869185, %rcx              # imm = 0x400000001
	movq	%rcx, 36(%rax)
	movl	$5, %edi
	jmp	.LBB0_22
.LBB0_14:
	movl	$1, 12(%rax)
	leal	(%r11,%r9,8), %r8d
	movl	%r8d, 16(%rax)
	cmpl	$2, %r10d
	je	.LBB0_19
# %bb.15:
	cmpl	$1, %r10d
	jne	.LBB0_16
# %bb.17:
	cmpl	%edx, %ecx
	jae	.LBB0_22
# %bb.18:
	movl	%ecx, %edx
	movzbl	(%rsi,%rdx), %edi
	movzbl	%dil, %esi
	incl	%ecx
	movl	$1, %r9d
	movq	$-256, %rdx
	movl	%ecx, %r8d
	jmp	.LBB0_21
.LBB0_19:
	leal	4(%rcx), %r8d
	cmpl	%edx, %r8d
	ja	.LBB0_22
# %bb.20:
	movabsq	$-4294967296, %rdx              # imm = 0xFFFFFFFF00000000
	movl	%ecx, %ecx
	movzwl	(%rsi,%rcx), %edi
	movzbl	2(%rsi,%rcx), %r9d
	shll	$16, %r9d
	orq	%rdi, %r9
	movzbl	3(%rsi,%rcx), %edi
	movl	%edi, %esi
	shll	$24, %esi
	orq	%r9, %rsi
	movl	$4, %r9d
.LBB0_21:
	xorl	%ecx, %ecx
	testb	%dil, %dil
	cmovsq	%rdx, %rcx
	orq	%rsi, %rcx
	movq	%rcx, 48(%rax)
	movl	%r9d, 40(%rax)
	movl	$1, 36(%rax)
	movl	%r8d, %edi
	jmp	.LBB0_22
.LBB0_16:
	movl	%ecx, %edi
	jmp	.LBB0_22
.Lfunc_end0:
	.size	decode_modrm, .Lfunc_end0-decode_modrm
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
