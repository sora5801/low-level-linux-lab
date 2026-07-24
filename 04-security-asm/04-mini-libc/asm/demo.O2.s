	.file	"demo.c"
	.text
	.globl	u64_to_dec                      # -- Begin function u64_to_dec
	.p2align	4
	.type	u64_to_dec,@function
u64_to_dec:                             # @u64_to_dec
# %bb.0:
	testq	%rdi, %rdi
	je	.LBB0_22
# %bb.1:
	movl	$-2, %ecx
	movabsq	$-3689348814741910323, %r8      # imm = 0xCCCCCCCCCCCCCCCD
	.p2align	4
.LBB0_2:                                # =>This Inner Loop Header: Depth=1
	leal	2(%rcx), %r9d
	movq	%rdi, %rax
	mulq	%r8
	shrq	$3, %rdx
	imull	$246, %edx, %eax
	addl	%edi, %eax
	addb	$48, %al
	movb	%al, -24(%rsp,%r9)
	incl	%ecx
	cmpq	$10, %rdi
	movq	%rdx, %rdi
	jae	.LBB0_2
# %bb.3:
	xorl	%eax, %eax
	cmpl	$-2, %ecx
	je	.LBB0_23
# %bb.4:
	leal	2(%rcx), %eax
	movl	%eax, %edx
	cmpl	$3, %eax
	jbe	.LBB0_5
# %bb.12:
	leal	1(%rcx), %r8d
	leaq	-1(%rdx), %rdi
	cmpl	%edi, %r8d
	setb	%r9b
	shrq	$32, %rdi
	setne	%dil
	orb	%r9b, %dil
	je	.LBB0_13
.LBB0_5:
	xorl	%edi, %edi
.LBB0_6:
	movq	%rdi, %r8
	testb	$3, %dl
	je	.LBB0_9
# %bb.7:
	leal	-2(%rcx), %r9d
	movl	%ecx, %r10d
	subl	%edi, %r10d
	incl	%r10d
	andl	$3, %r9d
	movq	%rdi, %r8
	.p2align	4
.LBB0_8:                                # =>This Inner Loop Header: Depth=1
	movl	%r10d, %r11d
	movzbl	-24(%rsp,%r11), %r11d
	movb	%r11b, (%rsi,%r8)
	incq	%r8
	decl	%r10d
	decq	%r9
	jne	.LBB0_8
.LBB0_9:
	subq	%rdx, %rdi
	cmpq	$-4, %rdi
	ja	.LBB0_23
# %bb.10:
	movq	%r8, %rdi
	subq	%rdx, %rdi
	leaq	(%r8,%rsi), %rdx
	addq	$3, %rdx
	subl	%r8d, %ecx
	leal	-2(%rcx), %esi
	leal	-1(%rcx), %r8d
	leal	1(%rcx), %r9d
	xorl	%r10d, %r10d
	.p2align	4
.LBB0_11:                               # =>This Inner Loop Header: Depth=1
	leal	(%r9,%r10), %r11d
	movzbl	-24(%rsp,%r11), %r11d
	movb	%r11b, -3(%rdx)
	leal	(%rcx,%r10), %r11d
	movzbl	-24(%rsp,%r11), %r11d
	movb	%r11b, -2(%rdx)
	leal	(%r8,%r10), %r11d
	movzbl	-24(%rsp,%r11), %r11d
	movb	%r11b, -1(%rdx)
	leal	(%rsi,%r10), %r11d
	movzbl	-24(%rsp,%r11), %r11d
	movb	%r11b, (%rdx)
	addq	$-4, %r10
	addq	$4, %rdx
	cmpq	%r10, %rdi
	jne	.LBB0_11
.LBB0_23:
	retq
.LBB0_22:
	movb	$48, (%rsi)
	movl	$1, %eax
	retq
.LBB0_13:
	cmpl	$32, %eax
	jae	.LBB0_15
# %bb.14:
	xorl	%edi, %edi
	jmp	.LBB0_19
.LBB0_15:
	movl	%edx, %edi
	andl	$-32, %edi
	movl	%eax, %r9d
	andl	$-32, %r9d
	xorl	%r10d, %r10d
	pxor	%xmm0, %xmm0
	.p2align	4
.LBB0_16:                               # =>This Inner Loop Header: Depth=1
	movl	%r8d, %r11d
	movdqu	-55(%rsp,%r11), %xmm1
	movdqu	-39(%rsp,%r11), %xmm2
	movdqa	%xmm2, %xmm3
	punpcklbw	%xmm0, %xmm3            # xmm3 = xmm3[0],xmm0[0],xmm3[1],xmm0[1],xmm3[2],xmm0[2],xmm3[3],xmm0[3],xmm3[4],xmm0[4],xmm3[5],xmm0[5],xmm3[6],xmm0[6],xmm3[7],xmm0[7]
	pshufd	$78, %xmm3, %xmm3               # xmm3 = xmm3[2,3,0,1]
	pshuflw	$27, %xmm3, %xmm3               # xmm3 = xmm3[3,2,1,0,4,5,6,7]
	pshufhw	$27, %xmm3, %xmm3               # xmm3 = xmm3[0,1,2,3,7,6,5,4]
	punpckhbw	%xmm0, %xmm2            # xmm2 = xmm2[8],xmm0[8],xmm2[9],xmm0[9],xmm2[10],xmm0[10],xmm2[11],xmm0[11],xmm2[12],xmm0[12],xmm2[13],xmm0[13],xmm2[14],xmm0[14],xmm2[15],xmm0[15]
	pshufd	$78, %xmm2, %xmm2               # xmm2 = xmm2[2,3,0,1]
	pshuflw	$27, %xmm2, %xmm2               # xmm2 = xmm2[3,2,1,0,4,5,6,7]
	pshufhw	$27, %xmm2, %xmm2               # xmm2 = xmm2[0,1,2,3,7,6,5,4]
	packuswb	%xmm3, %xmm2
	movdqa	%xmm1, %xmm3
	punpcklbw	%xmm0, %xmm3            # xmm3 = xmm3[0],xmm0[0],xmm3[1],xmm0[1],xmm3[2],xmm0[2],xmm3[3],xmm0[3],xmm3[4],xmm0[4],xmm3[5],xmm0[5],xmm3[6],xmm0[6],xmm3[7],xmm0[7]
	pshufd	$78, %xmm3, %xmm3               # xmm3 = xmm3[2,3,0,1]
	pshuflw	$27, %xmm3, %xmm3               # xmm3 = xmm3[3,2,1,0,4,5,6,7]
	pshufhw	$27, %xmm3, %xmm3               # xmm3 = xmm3[0,1,2,3,7,6,5,4]
	punpckhbw	%xmm0, %xmm1            # xmm1 = xmm1[8],xmm0[8],xmm1[9],xmm0[9],xmm1[10],xmm0[10],xmm1[11],xmm0[11],xmm1[12],xmm0[12],xmm1[13],xmm0[13],xmm1[14],xmm0[14],xmm1[15],xmm0[15]
	pshufd	$78, %xmm1, %xmm1               # xmm1 = xmm1[2,3,0,1]
	pshuflw	$27, %xmm1, %xmm1               # xmm1 = xmm1[3,2,1,0,4,5,6,7]
	pshufhw	$27, %xmm1, %xmm1               # xmm1 = xmm1[0,1,2,3,7,6,5,4]
	packuswb	%xmm3, %xmm1
	movdqu	%xmm2, (%rsi,%r10)
	movdqu	%xmm1, 16(%rsi,%r10)
	addq	$32, %r10
	addl	$-32, %r8d
	cmpq	%r10, %r9
	jne	.LBB0_16
# %bb.17:
	cmpl	%edx, %edi
	je	.LBB0_23
# %bb.18:
	testb	$28, %dl
	je	.LBB0_6
.LBB0_19:
	movq	%rdi, %r8
	movl	%edx, %edi
	andl	$-4, %edi
	movl	%eax, %r9d
	andl	$-4, %r9d
	movl	%ecx, %r10d
	subl	%r8d, %r10d
	incl	%r10d
	pxor	%xmm0, %xmm0
	.p2align	4
.LBB0_20:                               # =>This Inner Loop Header: Depth=1
	movl	%r10d, %r11d
	movd	-27(%rsp,%r11), %xmm1           # xmm1 = mem[0],zero,zero,zero
	punpcklbw	%xmm0, %xmm1            # xmm1 = xmm1[0],xmm0[0],xmm1[1],xmm0[1],xmm1[2],xmm0[2],xmm1[3],xmm0[3],xmm1[4],xmm0[4],xmm1[5],xmm0[5],xmm1[6],xmm0[6],xmm1[7],xmm0[7]
	pshuflw	$27, %xmm1, %xmm1               # xmm1 = xmm1[3,2,1,0,4,5,6,7]
	packuswb	%xmm1, %xmm1
	movd	%xmm1, (%rsi,%r8)
	addq	$4, %r8
	addl	$-4, %r10d
	cmpq	%r8, %r9
	jne	.LBB0_20
# %bb.21:
	cmpl	%edx, %edi
	jne	.LBB0_6
	jmp	.LBB0_23
.Lfunc_end0:
	.size	u64_to_dec, .Lfunc_end0-u64_to_dec
                                        # -- End function
	.globl	i64_to_dec                      # -- Begin function i64_to_dec
	.p2align	4
	.type	i64_to_dec,@function
i64_to_dec:                             # @i64_to_dec
# %bb.0:
	testq	%rdi, %rdi
	js	.LBB1_1
# %bb.23:
	je	.LBB1_46
# %bb.24:
	movl	$-2, %r8d
	movabsq	$-3689348814741910323, %rcx     # imm = 0xCCCCCCCCCCCCCCCD
	.p2align	4
.LBB1_25:                               # =>This Inner Loop Header: Depth=1
	leal	2(%r8), %r9d
	movq	%rdi, %rax
	mulq	%rcx
	shrq	$3, %rdx
	imull	$246, %edx, %eax
	addl	%edi, %eax
	addb	$48, %al
	movb	%al, -24(%rsp,%r9)
	incl	%r8d
	cmpq	$10, %rdi
	movq	%rdx, %rdi
	jae	.LBB1_25
# %bb.26:
	xorl	%ecx, %ecx
	cmpl	$-2, %r8d
	je	.LBB1_45
# %bb.27:
	leal	2(%r8), %ecx
	movl	%ecx, %eax
	cmpl	$3, %ecx
	jbe	.LBB1_28
# %bb.35:
	leal	1(%r8), %edi
	leaq	-1(%rax), %rdx
	cmpl	%edx, %edi
	setb	%r9b
	shrq	$32, %rdx
	setne	%dl
	orb	%r9b, %dl
	je	.LBB1_36
.LBB1_28:
	xorl	%edx, %edx
.LBB1_29:
	movq	%rdx, %rdi
	testb	$3, %al
	je	.LBB1_32
# %bb.30:
	leal	-2(%r8), %r9d
	movl	%r8d, %r10d
	subl	%edx, %r10d
	incl	%r10d
	andl	$3, %r9d
	movq	%rdx, %rdi
	.p2align	4
.LBB1_31:                               # =>This Inner Loop Header: Depth=1
	movl	%r10d, %r11d
	movzbl	-24(%rsp,%r11), %r11d
	movb	%r11b, (%rsi,%rdi)
	incq	%rdi
	decl	%r10d
	decq	%r9
	jne	.LBB1_31
.LBB1_32:
	subq	%rax, %rdx
	cmpq	$-4, %rdx
	ja	.LBB1_45
# %bb.33:
	movq	%rdi, %rdx
	subq	%rax, %rdx
	leaq	(%rdi,%rsi), %rax
	addq	$3, %rax
	subl	%edi, %r8d
	leal	-2(%r8), %esi
	leal	-1(%r8), %edi
	movl	%r8d, %r9d
	incl	%r9d
	xorl	%r10d, %r10d
	.p2align	4
.LBB1_34:                               # =>This Inner Loop Header: Depth=1
	leal	(%r9,%r10), %r11d
	movzbl	-24(%rsp,%r11), %r11d
	movb	%r11b, -3(%rax)
	leal	(%r8,%r10), %r11d
	movzbl	-24(%rsp,%r11), %r11d
	movb	%r11b, -2(%rax)
	leal	(%rdi,%r10), %r11d
	movzbl	-24(%rsp,%r11), %r11d
	movb	%r11b, -1(%rax)
	leal	(%rsi,%r10), %r11d
	movzbl	-24(%rsp,%r11), %r11d
	movb	%r11b, (%rax)
	addq	$-4, %r10
	addq	$4, %rax
	cmpq	%r10, %rdx
	jne	.LBB1_34
.LBB1_45:
	movl	%ecx, %eax
	retq
.LBB1_1:
	movb	$45, (%rsi)
	negq	%rdi
	movl	$-1, %ecx
	movabsq	$-3689348814741910323, %r8      # imm = 0xCCCCCCCCCCCCCCCD
	.p2align	4
.LBB1_2:                                # =>This Inner Loop Header: Depth=1
	incl	%ecx
	movq	%rdi, %rax
	mulq	%r8
	shrq	$3, %rdx
	imull	$246, %edx, %eax
	addl	%edi, %eax
	addb	$48, %al
	movb	%al, -24(%rsp,%rcx)
	cmpq	$10, %rdi
	movq	%rdx, %rdi
	jae	.LBB1_2
# %bb.3:
	movl	%ecx, %edi
	incl	%edi
	je	.LBB1_22
# %bb.4:
	movl	%edi, %eax
	cmpl	$3, %edi
	jbe	.LBB1_5
# %bb.12:
	leaq	-1(%rax), %rdx
	cmpl	%edx, %ecx
	setb	%r8b
	shrq	$32, %rdx
	setne	%dl
	orb	%r8b, %dl
	je	.LBB1_13
.LBB1_5:
	xorl	%edx, %edx
.LBB1_6:
	movq	%rdx, %rdi
	testb	$3, %al
	je	.LBB1_9
# %bb.7:
	leal	-3(%rcx), %r8d
	movl	%ecx, %r9d
	subl	%edx, %r9d
	andl	$3, %r8d
	movq	%rdx, %rdi
	.p2align	4
.LBB1_8:                                # =>This Inner Loop Header: Depth=1
	movl	%r9d, %r10d
	movzbl	-24(%rsp,%r10), %r10d
	movb	%r10b, 1(%rsi,%rdi)
	incq	%rdi
	decl	%r9d
	decq	%r8
	jne	.LBB1_8
.LBB1_9:
	subq	%rax, %rdx
	cmpq	$-4, %rdx
	ja	.LBB1_22
# %bb.10:
	movq	%rdi, %rdx
	subq	%rax, %rdx
	leaq	(%rdi,%rsi), %rax
	addq	$4, %rax
	movl	%ecx, %esi
	subl	%edi, %esi
	leal	-3(%rsi), %edi
	leal	-2(%rsi), %r8d
	leal	-1(%rsi), %r9d
	xorl	%r10d, %r10d
	.p2align	4
.LBB1_11:                               # =>This Inner Loop Header: Depth=1
	leal	(%rsi,%r10), %r11d
	movzbl	-24(%rsp,%r11), %r11d
	movb	%r11b, -3(%rax)
	leal	(%r9,%r10), %r11d
	movzbl	-24(%rsp,%r11), %r11d
	movb	%r11b, -2(%rax)
	leal	(%r8,%r10), %r11d
	movzbl	-24(%rsp,%r11), %r11d
	movb	%r11b, -1(%rax)
	leal	(%rdi,%r10), %r11d
	movzbl	-24(%rsp,%r11), %r11d
	movb	%r11b, (%rax)
	addq	$-4, %r10
	addq	$4, %rax
	cmpq	%r10, %rdx
	jne	.LBB1_11
.LBB1_22:
	addl	$2, %ecx
	movl	%ecx, %eax
	retq
.LBB1_46:
	movb	$48, (%rsi)
	movl	$1, %ecx
	movl	%ecx, %eax
	retq
.LBB1_36:
	cmpl	$32, %ecx
	jae	.LBB1_38
# %bb.37:
	xorl	%edx, %edx
	jmp	.LBB1_42
.LBB1_13:
	cmpl	$32, %edi
	jae	.LBB1_15
# %bb.14:
	xorl	%edx, %edx
	jmp	.LBB1_19
.LBB1_38:
	movl	%eax, %edx
	andl	$-32, %edx
	movl	%ecx, %r9d
	andl	$-32, %r9d
	xorl	%r10d, %r10d
	pxor	%xmm0, %xmm0
	.p2align	4
.LBB1_39:                               # =>This Inner Loop Header: Depth=1
	movl	%edi, %r11d
	movdqu	-55(%rsp,%r11), %xmm1
	movdqu	-39(%rsp,%r11), %xmm2
	movdqa	%xmm2, %xmm3
	punpcklbw	%xmm0, %xmm3            # xmm3 = xmm3[0],xmm0[0],xmm3[1],xmm0[1],xmm3[2],xmm0[2],xmm3[3],xmm0[3],xmm3[4],xmm0[4],xmm3[5],xmm0[5],xmm3[6],xmm0[6],xmm3[7],xmm0[7]
	pshufd	$78, %xmm3, %xmm3               # xmm3 = xmm3[2,3,0,1]
	pshuflw	$27, %xmm3, %xmm3               # xmm3 = xmm3[3,2,1,0,4,5,6,7]
	pshufhw	$27, %xmm3, %xmm3               # xmm3 = xmm3[0,1,2,3,7,6,5,4]
	punpckhbw	%xmm0, %xmm2            # xmm2 = xmm2[8],xmm0[8],xmm2[9],xmm0[9],xmm2[10],xmm0[10],xmm2[11],xmm0[11],xmm2[12],xmm0[12],xmm2[13],xmm0[13],xmm2[14],xmm0[14],xmm2[15],xmm0[15]
	pshufd	$78, %xmm2, %xmm2               # xmm2 = xmm2[2,3,0,1]
	pshuflw	$27, %xmm2, %xmm2               # xmm2 = xmm2[3,2,1,0,4,5,6,7]
	pshufhw	$27, %xmm2, %xmm2               # xmm2 = xmm2[0,1,2,3,7,6,5,4]
	packuswb	%xmm3, %xmm2
	movdqa	%xmm1, %xmm3
	punpcklbw	%xmm0, %xmm3            # xmm3 = xmm3[0],xmm0[0],xmm3[1],xmm0[1],xmm3[2],xmm0[2],xmm3[3],xmm0[3],xmm3[4],xmm0[4],xmm3[5],xmm0[5],xmm3[6],xmm0[6],xmm3[7],xmm0[7]
	pshufd	$78, %xmm3, %xmm3               # xmm3 = xmm3[2,3,0,1]
	pshuflw	$27, %xmm3, %xmm3               # xmm3 = xmm3[3,2,1,0,4,5,6,7]
	pshufhw	$27, %xmm3, %xmm3               # xmm3 = xmm3[0,1,2,3,7,6,5,4]
	punpckhbw	%xmm0, %xmm1            # xmm1 = xmm1[8],xmm0[8],xmm1[9],xmm0[9],xmm1[10],xmm0[10],xmm1[11],xmm0[11],xmm1[12],xmm0[12],xmm1[13],xmm0[13],xmm1[14],xmm0[14],xmm1[15],xmm0[15]
	pshufd	$78, %xmm1, %xmm1               # xmm1 = xmm1[2,3,0,1]
	pshuflw	$27, %xmm1, %xmm1               # xmm1 = xmm1[3,2,1,0,4,5,6,7]
	pshufhw	$27, %xmm1, %xmm1               # xmm1 = xmm1[0,1,2,3,7,6,5,4]
	packuswb	%xmm3, %xmm1
	movdqu	%xmm2, (%rsi,%r10)
	movdqu	%xmm1, 16(%rsi,%r10)
	addq	$32, %r10
	addl	$-32, %edi
	cmpq	%r10, %r9
	jne	.LBB1_39
# %bb.40:
	cmpl	%eax, %edx
	je	.LBB1_45
# %bb.41:
	testb	$28, %al
	je	.LBB1_29
.LBB1_42:
	movq	%rdx, %rdi
	movl	%eax, %edx
	andl	$-4, %edx
	movl	%ecx, %r9d
	andl	$-4, %r9d
	movl	%r8d, %r10d
	subl	%edi, %r10d
	incl	%r10d
	pxor	%xmm0, %xmm0
	.p2align	4
.LBB1_43:                               # =>This Inner Loop Header: Depth=1
	movl	%r10d, %r11d
	movd	-27(%rsp,%r11), %xmm1           # xmm1 = mem[0],zero,zero,zero
	punpcklbw	%xmm0, %xmm1            # xmm1 = xmm1[0],xmm0[0],xmm1[1],xmm0[1],xmm1[2],xmm0[2],xmm1[3],xmm0[3],xmm1[4],xmm0[4],xmm1[5],xmm0[5],xmm1[6],xmm0[6],xmm1[7],xmm0[7]
	pshuflw	$27, %xmm1, %xmm1               # xmm1 = xmm1[3,2,1,0,4,5,6,7]
	packuswb	%xmm1, %xmm1
	movd	%xmm1, (%rsi,%rdi)
	addq	$4, %rdi
	addl	$-4, %r10d
	cmpq	%rdi, %r9
	jne	.LBB1_43
# %bb.44:
	cmpl	%eax, %edx
	jne	.LBB1_29
	jmp	.LBB1_45
.LBB1_15:
	movl	%eax, %edx
	andl	$-32, %edx
	movl	%edi, %r8d
	andl	$-32, %r8d
	xorl	%r9d, %r9d
	pxor	%xmm0, %xmm0
	movl	%ecx, %r10d
	.p2align	4
.LBB1_16:                               # =>This Inner Loop Header: Depth=1
	movl	%r10d, %r11d
	movdqu	-55(%rsp,%r11), %xmm1
	movdqu	-39(%rsp,%r11), %xmm2
	movdqa	%xmm2, %xmm3
	punpcklbw	%xmm0, %xmm3            # xmm3 = xmm3[0],xmm0[0],xmm3[1],xmm0[1],xmm3[2],xmm0[2],xmm3[3],xmm0[3],xmm3[4],xmm0[4],xmm3[5],xmm0[5],xmm3[6],xmm0[6],xmm3[7],xmm0[7]
	pshufd	$78, %xmm3, %xmm3               # xmm3 = xmm3[2,3,0,1]
	pshuflw	$27, %xmm3, %xmm3               # xmm3 = xmm3[3,2,1,0,4,5,6,7]
	pshufhw	$27, %xmm3, %xmm3               # xmm3 = xmm3[0,1,2,3,7,6,5,4]
	punpckhbw	%xmm0, %xmm2            # xmm2 = xmm2[8],xmm0[8],xmm2[9],xmm0[9],xmm2[10],xmm0[10],xmm2[11],xmm0[11],xmm2[12],xmm0[12],xmm2[13],xmm0[13],xmm2[14],xmm0[14],xmm2[15],xmm0[15]
	pshufd	$78, %xmm2, %xmm2               # xmm2 = xmm2[2,3,0,1]
	pshuflw	$27, %xmm2, %xmm2               # xmm2 = xmm2[3,2,1,0,4,5,6,7]
	pshufhw	$27, %xmm2, %xmm2               # xmm2 = xmm2[0,1,2,3,7,6,5,4]
	packuswb	%xmm3, %xmm2
	movdqa	%xmm1, %xmm3
	punpcklbw	%xmm0, %xmm3            # xmm3 = xmm3[0],xmm0[0],xmm3[1],xmm0[1],xmm3[2],xmm0[2],xmm3[3],xmm0[3],xmm3[4],xmm0[4],xmm3[5],xmm0[5],xmm3[6],xmm0[6],xmm3[7],xmm0[7]
	pshufd	$78, %xmm3, %xmm3               # xmm3 = xmm3[2,3,0,1]
	pshuflw	$27, %xmm3, %xmm3               # xmm3 = xmm3[3,2,1,0,4,5,6,7]
	pshufhw	$27, %xmm3, %xmm3               # xmm3 = xmm3[0,1,2,3,7,6,5,4]
	punpckhbw	%xmm0, %xmm1            # xmm1 = xmm1[8],xmm0[8],xmm1[9],xmm0[9],xmm1[10],xmm0[10],xmm1[11],xmm0[11],xmm1[12],xmm0[12],xmm1[13],xmm0[13],xmm1[14],xmm0[14],xmm1[15],xmm0[15]
	pshufd	$78, %xmm1, %xmm1               # xmm1 = xmm1[2,3,0,1]
	pshuflw	$27, %xmm1, %xmm1               # xmm1 = xmm1[3,2,1,0,4,5,6,7]
	pshufhw	$27, %xmm1, %xmm1               # xmm1 = xmm1[0,1,2,3,7,6,5,4]
	packuswb	%xmm3, %xmm1
	movdqu	%xmm2, 1(%rsi,%r9)
	movdqu	%xmm1, 17(%rsi,%r9)
	addq	$32, %r9
	addl	$-32, %r10d
	cmpq	%r9, %r8
	jne	.LBB1_16
# %bb.17:
	cmpl	%eax, %edx
	je	.LBB1_22
# %bb.18:
	testb	$28, %al
	je	.LBB1_6
.LBB1_19:
	movq	%rdx, %r8
	movl	%eax, %edx
	andl	$-4, %edx
	andl	$-4, %edi
	movl	%ecx, %r9d
	subl	%r8d, %r9d
	pxor	%xmm0, %xmm0
	.p2align	4
.LBB1_20:                               # =>This Inner Loop Header: Depth=1
	movl	%r9d, %r10d
	movd	-27(%rsp,%r10), %xmm1           # xmm1 = mem[0],zero,zero,zero
	punpcklbw	%xmm0, %xmm1            # xmm1 = xmm1[0],xmm0[0],xmm1[1],xmm0[1],xmm1[2],xmm0[2],xmm1[3],xmm0[3],xmm1[4],xmm0[4],xmm1[5],xmm0[5],xmm1[6],xmm0[6],xmm1[7],xmm0[7]
	pshuflw	$27, %xmm1, %xmm1               # xmm1 = xmm1[3,2,1,0,4,5,6,7]
	packuswb	%xmm1, %xmm1
	movd	%xmm1, 1(%rsi,%r8)
	addq	$4, %r8
	addl	$-4, %r9d
	cmpq	%r8, %rdi
	jne	.LBB1_20
# %bb.21:
	cmpl	%eax, %edx
	jne	.LBB1_6
	jmp	.LBB1_22
.Lfunc_end1:
	.size	i64_to_dec, .Lfunc_end1-i64_to_dec
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
