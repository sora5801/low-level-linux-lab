	.file	"demo.c"
	.text
	.globl	scalar_strlen                   # -- Begin function scalar_strlen
	.p2align	4
	.type	scalar_strlen,@function
scalar_strlen:                          # @scalar_strlen
# %bb.0:
	movq	$-1, %rax
	.p2align	4
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	cmpb	$0, 1(%rdi,%rax)
	leaq	1(%rax), %rax
	jne	.LBB0_1
# %bb.2:
	retq
.Lfunc_end0:
	.size	scalar_strlen, .Lfunc_end0-scalar_strlen
                                        # -- End function
	.section	.rodata.cst16,"aM",@progbits,16
	.p2align	4, 0x0                          # -- Begin function count_nonzero_bounded
.LCPI1_0:
	.quad	1                               # 0x1
	.quad	1                               # 0x1
	.text
	.globl	count_nonzero_bounded
	.p2align	4
	.type	count_nonzero_bounded,@function
count_nonzero_bounded:                  # @count_nonzero_bounded
# %bb.0:
	testq	%rsi, %rsi
	je	.LBB1_1
# %bb.3:
	cmpq	$4, %rsi
	jae	.LBB1_5
# %bb.4:
	xorl	%ecx, %ecx
	xorl	%eax, %eax
	jmp	.LBB1_9
.LBB1_1:
	xorl	%eax, %eax
	jmp	.LBB1_2
.LBB1_5:
	movq	%rsi, %rcx
	andq	$-4, %rcx
	pxor	%xmm0, %xmm0
	xorl	%eax, %eax
	pcmpeqd	%xmm2, %xmm2
	movdqa	.LCPI1_0(%rip), %xmm4           # xmm4 = [1,1]
	pxor	%xmm3, %xmm3
	pxor	%xmm1, %xmm1
	.p2align	4
.LBB1_6:                                # =>This Inner Loop Header: Depth=1
	movzwl	(%rdi,%rax), %edx
	movd	%edx, %xmm5
	movzwl	2(%rdi,%rax), %edx
	movd	%edx, %xmm6
	pcmpeqb	%xmm0, %xmm5
	pxor	%xmm2, %xmm5
	punpcklbw	%xmm5, %xmm5            # xmm5 = xmm5[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
	pshuflw	$212, %xmm5, %xmm5              # xmm5 = xmm5[0,1,1,3,4,5,6,7]
	pshufd	$212, %xmm5, %xmm5              # xmm5 = xmm5[0,1,1,3]
	pand	%xmm4, %xmm5
	paddq	%xmm5, %xmm3
	pcmpeqb	%xmm0, %xmm6
	pxor	%xmm2, %xmm6
	punpcklbw	%xmm6, %xmm6            # xmm6 = xmm6[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
	pshuflw	$212, %xmm6, %xmm5              # xmm5 = xmm6[0,1,1,3,4,5,6,7]
	pshufd	$212, %xmm5, %xmm5              # xmm5 = xmm5[0,1,1,3]
	pand	%xmm4, %xmm5
	paddq	%xmm5, %xmm1
	addq	$4, %rax
	cmpq	%rax, %rcx
	jne	.LBB1_6
# %bb.7:
	paddq	%xmm3, %xmm1
	pshufd	$238, %xmm1, %xmm0              # xmm0 = xmm1[2,3,2,3]
	paddq	%xmm1, %xmm0
	movq	%xmm0, %rax
	jmp	.LBB1_8
.LBB1_2:
	retq
.LBB1_8:
	cmpq	%rcx, %rsi
	je	.LBB1_2
.LBB1_9:
	cmpb	$1, (%rdi,%rcx)
	sbbq	$-1, %rax
	incq	%rcx
	jmp	.LBB1_8
.Lfunc_end1:
	.size	count_nonzero_bounded, .Lfunc_end1-count_nonzero_bounded
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
