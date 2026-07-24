	.file	"demo.c"
	.text
	.globl	ct_eq                           # -- Begin function ct_eq
	.p2align	4
	.type	ct_eq,@function
ct_eq:                                  # @ct_eq
# %bb.0:
	xorl	%eax, %eax
	cmpl	%esi, %edi
	sete	%al
	negl	%eax
	retq
.Lfunc_end0:
	.size	ct_eq, .Lfunc_end0-ct_eq
                                        # -- End function
	.globl	ct_select                       # -- Begin function ct_select
	.p2align	4
	.type	ct_select,@function
ct_select:                              # @ct_select
# %bb.0:
	movl	%esi, %eax
	xorl	%edx, %eax
	andl	%edi, %eax
	xorl	%edx, %eax
	retq
.Lfunc_end1:
	.size	ct_select, .Lfunc_end1-ct_select
                                        # -- End function
	.globl	ct_memeq                        # -- Begin function ct_memeq
	.p2align	4
	.type	ct_memeq,@function
ct_memeq:                               # @ct_memeq
# %bb.0:
	testq	%rdx, %rdx
	je	.LBB2_1
# %bb.2:
	cmpq	$3, %rdx
	ja	.LBB2_5
# %bb.3:
	xorl	%eax, %eax
	xorl	%ecx, %ecx
	jmp	.LBB2_4
.LBB2_1:
	movl	$-1, %eax
	retq
.LBB2_5:
	cmpq	$32, %rdx
	jae	.LBB2_7
# %bb.6:
	xorl	%eax, %eax
	xorl	%ecx, %ecx
	jmp	.LBB2_11
.LBB2_7:
	movq	%rdx, %rax
	andq	$-32, %rax
	pxor	%xmm0, %xmm0
	xorl	%ecx, %ecx
	pxor	%xmm1, %xmm1
	.p2align	4
.LBB2_8:                                # =>This Inner Loop Header: Depth=1
	movdqu	(%rdi,%rcx), %xmm2
	movdqu	16(%rdi,%rcx), %xmm3
	movdqu	(%rsi,%rcx), %xmm4
	pxor	%xmm2, %xmm4
	por	%xmm4, %xmm0
	movdqu	16(%rsi,%rcx), %xmm2
	pxor	%xmm3, %xmm2
	por	%xmm2, %xmm1
	addq	$32, %rcx
	cmpq	%rcx, %rax
	jne	.LBB2_8
# %bb.9:
	por	%xmm0, %xmm1
	pshufd	$238, %xmm1, %xmm0              # xmm0 = xmm1[2,3,2,3]
	por	%xmm1, %xmm0
	pshufd	$85, %xmm0, %xmm1               # xmm1 = xmm0[1,1,1,1]
	por	%xmm0, %xmm1
	movdqa	%xmm1, %xmm0
	psrld	$16, %xmm0
	por	%xmm1, %xmm0
	movdqa	%xmm0, %xmm1
	psrlw	$8, %xmm1
	por	%xmm0, %xmm1
	movd	%xmm1, %ecx
	cmpq	%rax, %rdx
	je	.LBB2_15
# %bb.10:
	testb	$28, %dl
	je	.LBB2_4
.LBB2_11:
	movq	%rax, %r8
	movq	%rdx, %rax
	andq	$-4, %rax
	movzbl	%cl, %ecx
	movd	%ecx, %xmm0
	.p2align	4
.LBB2_12:                               # =>This Inner Loop Header: Depth=1
	movl	(%rsi,%r8), %ecx
	xorl	(%rdi,%r8), %ecx
	movd	%ecx, %xmm1
	por	%xmm1, %xmm0
	addq	$4, %r8
	cmpq	%r8, %rax
	jne	.LBB2_12
# %bb.13:
	movdqa	%xmm0, %xmm1
	psrld	$16, %xmm1
	por	%xmm0, %xmm1
	movdqa	%xmm1, %xmm0
	psrlw	$8, %xmm0
	por	%xmm1, %xmm0
	movd	%xmm0, %ecx
	jmp	.LBB2_14
.LBB2_4:
	movzbl	(%rsi,%rax), %r8d
	xorb	(%rdi,%rax), %r8b
	orb	%r8b, %cl
	incq	%rax
.LBB2_14:
	cmpq	%rax, %rdx
	jne	.LBB2_4
.LBB2_15:
	xorl	%eax, %eax
	cmpb	$1, %cl
	sbbl	%eax, %eax
	retq
.Lfunc_end2:
	.size	ct_memeq, .Lfunc_end2-ct_memeq
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
