	.file	"demo.c"
	.text
	.globl	u64_to_dec                      # -- Begin function u64_to_dec
	.p2align	4
	.type	u64_to_dec,@function
u64_to_dec:                             # @u64_to_dec
# %bb.0:
	testq	%rdi, %rdi
	je	.LBB0_24
# %bb.1:
	pushq	%rbp
	pushq	%r14
	pushq	%rbx
	movl	$-2, %ecx
	movabsq	$-3689348814741910323, %r8      # imm = 0xCCCCCCCCCCCCCCCD
	.p2align	4
.LBB0_2:                                # =>This Inner Loop Header: Depth=1
	movq	%rdi, %rax
	mulq	%r8
	leal	2(%rcx), %eax
	shrq	$3, %rdx
	leal	(%rdx,%rdx), %r9d
	leal	(%r9,%r9,4), %r9d
	movl	%edi, %r10d
	subl	%r9d, %r10d
	orb	$48, %r10b
	movb	%r10b, -32(%rsp,%rax)
	incl	%ecx
	cmpq	$10, %rdi
	movq	%rdx, %rdi
	jae	.LBB0_2
# %bb.3:
	leal	2(%rcx), %eax
	movl	%eax, %edx
	cmpl	$-2, %ecx
	je	.LBB0_22
# %bb.4:
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
	movq	%rdi, %r9
	testb	$3, %dl
	je	.LBB0_9
# %bb.7:
	leal	-2(%rcx), %r8d
	movl	%ecx, %r10d
	subl	%edi, %r10d
	incl	%r10d
	andl	$3, %r8d
	movq	%rdi, %r9
	.p2align	4
.LBB0_8:                                # =>This Inner Loop Header: Depth=1
	movl	%r10d, %r11d
	movzbl	-32(%rsp,%r11), %r11d
	movb	%r11b, (%rsi,%r9)
	incq	%r9
	decl	%r10d
	decq	%r8
	jne	.LBB0_8
.LBB0_9:
	subq	%rdx, %rdi
	cmpq	$-4, %rdi
	ja	.LBB0_22
# %bb.10:
	movq	%r9, %rdi
	subq	%rdx, %rdi
	leaq	(%r9,%rsi), %r8
	addq	$3, %r8
	subl	%r9d, %ecx
	leal	-2(%rcx), %r9d
	leal	-1(%rcx), %r10d
	leal	1(%rcx), %r11d
	xorl	%ebx, %ebx
	.p2align	4
.LBB0_11:                               # =>This Inner Loop Header: Depth=1
	leal	(%r11,%rbx), %r14d
	movzbl	-32(%rsp,%r14), %ebp
	movb	%bpl, -3(%r8)
	leal	(%rcx,%rbx), %r14d
	movzbl	-32(%rsp,%r14), %ebp
	movb	%bpl, -2(%r8)
	leal	(%r10,%rbx), %r14d
	movzbl	-32(%rsp,%r14), %ebp
	movb	%bpl, -1(%r8)
	leal	(%r9,%rbx), %r14d
	movzbl	-32(%rsp,%r14), %ebp
	movb	%bpl, (%r8)
	addq	$-4, %rbx
	addq	$4, %r8
	cmpq	%rbx, %rdi
	jne	.LBB0_11
.LBB0_22:
	popq	%rbx
	popq	%r14
	popq	%rbp
	movb	$0, (%rsi,%rdx)
	retq
.LBB0_24:
	movb	$48, (%rsi)
	movl	$1, %eax
	movl	$1, %edx
	movb	$0, (%rsi,%rdx)
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
	movdqu	-63(%rsp,%r11), %xmm1
	movdqu	-47(%rsp,%r11), %xmm2
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
	je	.LBB0_22
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
	movd	-35(%rsp,%r11), %xmm1           # xmm1 = mem[0],zero,zero,zero
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
	jmp	.LBB0_22
.Lfunc_end0:
	.size	u64_to_dec, .Lfunc_end0-u64_to_dec
                                        # -- End function
	.globl	format_field                    # -- Begin function format_field
	.p2align	4
	.type	format_field,@function
format_field:                           # @format_field
# %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r12
	pushq	%rbx
	subq	$64, %rsp
	movq	%rdx, %rbx
	movl	%esi, %r14d
	testq	%rdi, %rdi
	je	.LBB1_51
# %bb.1:
	movl	$-2, %ecx
	movabsq	$-3689348814741910323, %rsi     # imm = 0xCCCCCCCCCCCCCCCD
	.p2align	4
.LBB1_2:                                # =>This Inner Loop Header: Depth=1
	movq	%rdi, %rax
	mulq	%rsi
	leal	2(%rcx), %eax
	shrq	$3, %rdx
	leal	(%rdx,%rdx), %r8d
	leal	(%r8,%r8,4), %r8d
	movl	%edi, %r9d
	subl	%r8d, %r9d
	orb	$48, %r9b
	movb	%r9b, 32(%rsp,%rax)
	incl	%ecx
	cmpq	$10, %rdi
	movq	%rdx, %rdi
	jae	.LBB1_2
# %bb.3:
	leal	2(%rcx), %ebp
	movl	%ebp, %eax
	cmpl	$-2, %ecx
	je	.LBB1_22
# %bb.4:
	cmpl	$3, %ebp
	jbe	.LBB1_5
# %bb.12:
	leal	1(%rcx), %esi
	leaq	-1(%rax), %rdx
	cmpl	%edx, %esi
	setb	%dil
	shrq	$32, %rdx
	setne	%dl
	orb	%dil, %dl
	je	.LBB1_13
.LBB1_5:
	xorl	%edx, %edx
.LBB1_6:
	movq	%rdx, %rdi
	testb	$3, %al
	je	.LBB1_9
# %bb.7:
	leal	-2(%rcx), %esi
	movl	%ecx, %r8d
	subl	%edx, %r8d
	incl	%r8d
	andl	$3, %esi
	movq	%rdx, %rdi
	.p2align	4
.LBB1_8:                                # =>This Inner Loop Header: Depth=1
	movl	%r8d, %r9d
	movzbl	32(%rsp,%r9), %r9d
	movb	%r9b, (%rsp,%rdi)
	incq	%rdi
	decl	%r8d
	decq	%rsi
	jne	.LBB1_8
.LBB1_9:
	subq	%rax, %rdx
	cmpq	$-4, %rdx
	ja	.LBB1_22
# %bb.10:
	movq	%rdi, %rdx
	subq	%rax, %rdx
	movq	%rsp, %rsi
	addq	%rdi, %rsi
	addq	$3, %rsi
	subl	%edi, %ecx
	leal	-2(%rcx), %edi
	leal	-1(%rcx), %r8d
	movl	%ecx, %r9d
	incl	%r9d
	xorl	%r10d, %r10d
	.p2align	4
.LBB1_11:                               # =>This Inner Loop Header: Depth=1
	leal	(%r9,%r10), %r11d
	movzbl	32(%rsp,%r11), %r11d
	movb	%r11b, -3(%rsi)
	leal	(%rcx,%r10), %r11d
	movzbl	32(%rsp,%r11), %r11d
	movb	%r11b, -2(%rsi)
	leal	(%r8,%r10), %r11d
	movzbl	32(%rsp,%r11), %r11d
	movb	%r11b, -1(%rsi)
	leal	(%rdi,%r10), %r11d
	movzbl	32(%rsp,%r11), %r11d
	movb	%r11b, (%rsi)
	addq	$-4, %r10
	addq	$4, %rsi
	cmpq	%r10, %rdx
	jne	.LBB1_11
	jmp	.LBB1_22
.LBB1_51:
	movb	$48, (%rsp)
	movl	$1, %ebp
	movl	$1, %eax
.LBB1_22:
	movb	$0, (%rsp,%rax)
	xorl	%eax, %eax
	movl	%r14d, %r12d
	subl	%ebp, %r12d
	cmovbl	%eax, %r12d
	jbe	.LBB1_38
# %bb.23:
	movl	%ebp, %eax
	notl	%eax
	addl	%eax, %r14d
	leaq	1(%r14), %r15
	movq	%rbx, %rdi
	movl	$32, %esi
	movq	%r15, %rdx
	callq	memset@PLT
	cmpl	$2, %r14d
	ja	.LBB1_26
# %bb.24:
	xorl	%eax, %eax
	jmp	.LBB1_25
.LBB1_26:
	movabsq	$8589934560, %rcx               # imm = 0x1FFFFFFE0
	cmpl	$31, %r14d
	jae	.LBB1_28
# %bb.27:
	xorl	%eax, %eax
	jmp	.LBB1_33
.LBB1_28:
	movq	%r15, %rax
	andq	%rcx, %rax
	movq	%r15, %rsi
	andq	$-32, %rsi
	negq	%rsi
	xorl	%edx, %edx
	.p2align	4
.LBB1_29:                               # =>This Inner Loop Header: Depth=1
	addq	$-32, %rdx
	cmpq	%rdx, %rsi
	jne	.LBB1_29
# %bb.30:
	cmpq	%rax, %r15
	jne	.LBB1_32
# %bb.31:
	negq	%rdx
	jmp	.LBB1_37
.LBB1_13:
	cmpl	$32, %ebp
	jae	.LBB1_15
# %bb.14:
	xorl	%edx, %edx
	jmp	.LBB1_19
.LBB1_32:
	testb	$28, %r15b
	je	.LBB1_25
.LBB1_33:
	movq	%rax, %rdx
	addq	$28, %rcx
	movq	%rcx, %rax
	andq	%r15, %rax
	movq	%rdx, %rsi
	subq	%rax, %rsi
	xorl	%ecx, %ecx
	.p2align	4
.LBB1_34:                               # =>This Inner Loop Header: Depth=1
	addq	$-4, %rcx
	cmpq	%rcx, %rsi
	jne	.LBB1_34
# %bb.35:
	cmpq	%rax, %r15
	jne	.LBB1_25
# %bb.36:
	subq	%rcx, %rdx
.LBB1_37:
	movl	%edx, %eax
	jmp	.LBB1_38
	.p2align	4
.LBB1_25:                               # =>This Inner Loop Header: Depth=1
	incl	%eax
	cmpl	%eax, %r12d
	ja	.LBB1_25
.LBB1_38:
	testl	%ebp, %ebp
	je	.LBB1_50
# %bb.39:
	movl	%ebp, %ecx
	cmpl	$32, %ebp
	jae	.LBB1_41
# %bb.40:
	xorl	%edx, %edx
.LBB1_46:
	movq	%rcx, %rdi
	movq	%rdx, %rsi
	andq	$3, %rdi
	je	.LBB1_48
	.p2align	4
.LBB1_47:                               # =>This Inner Loop Header: Depth=1
	movzbl	(%rsp,%rsi), %r8d
	movl	%eax, %r9d
	incl	%eax
	movb	%r8b, (%rbx,%r9)
	incq	%rsi
	decq	%rdi
	jne	.LBB1_47
.LBB1_48:
	subq	%rcx, %rdx
	cmpq	$-4, %rdx
	ja	.LBB1_50
	.p2align	4
.LBB1_49:                               # =>This Inner Loop Header: Depth=1
	movzbl	(%rsp,%rsi), %edx
	leal	1(%rax), %edi
	movl	%eax, %r8d
	movb	%dl, (%rbx,%r8)
	movzbl	1(%rsp,%rsi), %edx
	leal	2(%rax), %r8d
	movb	%dl, (%rbx,%rdi)
	movzbl	2(%rsp,%rsi), %edx
	leal	3(%rax), %edi
	movb	%dl, (%rbx,%r8)
	movzbl	3(%rsp,%rsi), %edx
	addl	$4, %eax
	movb	%dl, (%rbx,%rdi)
	addq	$4, %rsi
	cmpq	%rsi, %rcx
	jne	.LBB1_49
	jmp	.LBB1_50
.LBB1_41:
	leaq	-1(%rcx), %rsi
	xorl	%edx, %edx
	movl	%eax, %edi
	addl	%esi, %edi
	jb	.LBB1_46
# %bb.42:
	shrq	$32, %rsi
	jne	.LBB1_46
# %bb.43:
	movl	%ecx, %edx
	andl	$-32, %edx
	movl	%eax, %esi
	addl	%edx, %eax
	xorl	%edi, %edi
	.p2align	4
.LBB1_44:                               # =>This Inner Loop Header: Depth=1
	leal	(%rsi,%rdi), %r8d
	movdqa	(%rsp,%rdi), %xmm0
	movdqa	16(%rsp,%rdi), %xmm1
	movdqu	%xmm0, (%rbx,%r8)
	movdqu	%xmm1, 16(%rbx,%r8)
	addq	$32, %rdi
	cmpq	%rdi, %rdx
	jne	.LBB1_44
# %bb.45:
	cmpl	%ecx, %edx
	jne	.LBB1_46
.LBB1_50:
	movl	%eax, %ecx
	movb	$0, (%rbx,%rcx)
                                        # kill: def $eax killed $eax killed $rax
	addq	$64, %rsp
	popq	%rbx
	popq	%r12
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
.LBB1_15:
	movl	%eax, %edx
	andl	$-32, %edx
	movl	%ebp, %edi
	andl	$-32, %edi
	xorl	%r8d, %r8d
	pxor	%xmm0, %xmm0
	.p2align	4
.LBB1_16:                               # =>This Inner Loop Header: Depth=1
	movl	%esi, %r9d
	movdqu	1(%rsp,%r9), %xmm1
	movdqu	17(%rsp,%r9), %xmm2
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
	movdqa	%xmm2, (%rsp,%r8)
	movdqa	%xmm1, 16(%rsp,%r8)
	addq	$32, %r8
	addl	$-32, %esi
	cmpq	%r8, %rdi
	jne	.LBB1_16
# %bb.17:
	cmpl	%eax, %edx
	je	.LBB1_22
# %bb.18:
	testb	$28, %al
	je	.LBB1_6
.LBB1_19:
	movq	%rdx, %rsi
	movl	%eax, %edx
	andl	$-4, %edx
	movl	%ebp, %edi
	andl	$-4, %edi
	movl	%ecx, %r8d
	subl	%esi, %r8d
	incl	%r8d
	pxor	%xmm0, %xmm0
	.p2align	4
.LBB1_20:                               # =>This Inner Loop Header: Depth=1
	movl	%r8d, %r9d
	movd	29(%rsp,%r9), %xmm1             # xmm1 = mem[0],zero,zero,zero
	punpcklbw	%xmm0, %xmm1            # xmm1 = xmm1[0],xmm0[0],xmm1[1],xmm0[1],xmm1[2],xmm0[2],xmm1[3],xmm0[3],xmm1[4],xmm0[4],xmm1[5],xmm0[5],xmm1[6],xmm0[6],xmm1[7],xmm0[7]
	pshuflw	$27, %xmm1, %xmm1               # xmm1 = xmm1[3,2,1,0,4,5,6,7]
	packuswb	%xmm1, %xmm1
	movd	%xmm1, (%rsp,%rsi)
	addq	$4, %rsi
	addl	$-4, %r8d
	cmpq	%rsi, %rdi
	jne	.LBB1_20
# %bb.21:
	cmpl	%eax, %edx
	jne	.LBB1_6
	jmp	.LBB1_22
.Lfunc_end1:
	.size	format_field, .Lfunc_end1-format_field
                                        # -- End function
	.globl	checksum8                       # -- Begin function checksum8
	.p2align	4
	.type	checksum8,@function
checksum8:                              # @checksum8
# %bb.0:
	testl	%esi, %esi
	je	.LBB2_1
# %bb.2:
	movl	%esi, %eax
	cmpl	$8, %esi
	jae	.LBB2_4
# %bb.3:
	xorl	%ecx, %ecx
	xorl	%edx, %edx
	jmp	.LBB2_7
.LBB2_1:
	xorl	%eax, %eax
	retq
.LBB2_4:
	movl	%eax, %ecx
	andl	$-8, %ecx
	pxor	%xmm1, %xmm1
	xorl	%edx, %edx
	pxor	%xmm2, %xmm2
	pxor	%xmm0, %xmm0
	.p2align	4
.LBB2_5:                                # =>This Inner Loop Header: Depth=1
	movd	(%rdi,%rdx), %xmm3              # xmm3 = mem[0],zero,zero,zero
	movd	4(%rdi,%rdx), %xmm4             # xmm4 = mem[0],zero,zero,zero
	punpcklbw	%xmm1, %xmm3            # xmm3 = xmm3[0],xmm1[0],xmm3[1],xmm1[1],xmm3[2],xmm1[2],xmm3[3],xmm1[3],xmm3[4],xmm1[4],xmm3[5],xmm1[5],xmm3[6],xmm1[6],xmm3[7],xmm1[7]
	punpcklwd	%xmm1, %xmm3            # xmm3 = xmm3[0],xmm1[0],xmm3[1],xmm1[1],xmm3[2],xmm1[2],xmm3[3],xmm1[3]
	paddd	%xmm3, %xmm2
	punpcklbw	%xmm1, %xmm4            # xmm4 = xmm4[0],xmm1[0],xmm4[1],xmm1[1],xmm4[2],xmm1[2],xmm4[3],xmm1[3],xmm4[4],xmm1[4],xmm4[5],xmm1[5],xmm4[6],xmm1[6],xmm4[7],xmm1[7]
	punpcklwd	%xmm1, %xmm4            # xmm4 = xmm4[0],xmm1[0],xmm4[1],xmm1[1],xmm4[2],xmm1[2],xmm4[3],xmm1[3]
	paddd	%xmm4, %xmm0
	addq	$8, %rdx
	cmpq	%rdx, %rcx
	jne	.LBB2_5
# %bb.6:
	paddd	%xmm2, %xmm0
	pshufd	$238, %xmm0, %xmm1              # xmm1 = xmm0[2,3,2,3]
	paddd	%xmm0, %xmm1
	pshufd	$85, %xmm1, %xmm0               # xmm0 = xmm1[1,1,1,1]
	paddd	%xmm1, %xmm0
	movd	%xmm0, %edx
	cmpl	%eax, %ecx
	je	.LBB2_8
	.p2align	4
.LBB2_7:                                # =>This Inner Loop Header: Depth=1
	movzbl	(%rdi,%rcx), %esi
	addl	%esi, %edx
	incq	%rcx
	cmpq	%rcx, %rax
	jne	.LBB2_7
.LBB2_8:
	movzbl	%dl, %eax
	retq
.Lfunc_end2:
	.size	checksum8, .Lfunc_end2-checksum8
                                        # -- End function
	.globl	render_line                     # -- Begin function render_line
	.p2align	4
	.type	render_line,@function
render_line:                            # @render_line
# %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r12
	pushq	%rbx
	subq	$64, %rsp
	testl	%edi, %edi
	je	.LBB3_63
# %bb.1:
	movl	%edi, %ecx
	movl	$-2, %eax
	movl	$3435973837, %edi               # imm = 0xCCCCCCCD
	.p2align	4
.LBB3_2:                                # =>This Inner Loop Header: Depth=1
	leal	2(%rax), %r8d
	movl	%ecx, %r9d
	imulq	%rdi, %r9
	shrq	$35, %r9
	leal	(%r9,%r9), %r10d
	leal	(%r10,%r10,4), %r10d
	movl	%ecx, %r11d
	subl	%r10d, %r11d
	orb	$48, %r11b
	movb	%r11b, (%rsp,%r8)
	incl	%eax
	cmpq	$10, %rcx
	movq	%r9, %rcx
	jae	.LBB3_2
# %bb.3:
	leal	2(%rax), %ebx
	movl	%ebx, %ecx
	cmpl	$-2, %eax
	je	.LBB3_22
# %bb.4:
	cmpl	$3, %ebx
	jbe	.LBB3_5
# %bb.12:
	leal	1(%rax), %r8d
	leaq	-1(%rcx), %rdi
	cmpl	%edi, %r8d
	setb	%r9b
	shrq	$32, %rdi
	setne	%dil
	orb	%r9b, %dil
	je	.LBB3_13
.LBB3_5:
	xorl	%edi, %edi
.LBB3_6:
	movq	%rdi, %r9
	testb	$3, %cl
	je	.LBB3_9
# %bb.7:
	leal	-2(%rax), %r8d
	movl	%eax, %r10d
	subl	%edi, %r10d
	incl	%r10d
	andl	$3, %r8d
	movq	%rdi, %r9
	.p2align	4
.LBB3_8:                                # =>This Inner Loop Header: Depth=1
	movl	%r10d, %r11d
	movzbl	(%rsp,%r11), %r11d
	movb	%r11b, (%rdx,%r9)
	incq	%r9
	decl	%r10d
	decq	%r8
	jne	.LBB3_8
.LBB3_9:
	subq	%rcx, %rdi
	cmpq	$-4, %rdi
	ja	.LBB3_22
# %bb.10:
	movq	%r9, %rdi
	subq	%rcx, %rdi
	leaq	(%r9,%rdx), %r8
	addq	$3, %r8
	subl	%r9d, %eax
	leal	-2(%rax), %r9d
	leal	-1(%rax), %r10d
	movl	%eax, %r11d
	incl	%r11d
	xorl	%r14d, %r14d
	.p2align	4
.LBB3_11:                               # =>This Inner Loop Header: Depth=1
	leal	(%r11,%r14), %r15d
	movzbl	(%rsp,%r15), %ebp
	movb	%bpl, -3(%r8)
	leal	(%rax,%r14), %r15d
	movzbl	(%rsp,%r15), %ebp
	movb	%bpl, -2(%r8)
	leal	(%r10,%r14), %r15d
	movzbl	(%rsp,%r15), %ebp
	movb	%bpl, -1(%r8)
	leal	(%r9,%r14), %r15d
	movzbl	(%rsp,%r15), %ebp
	movb	%bpl, (%r8)
	addq	$-4, %r14
	addq	$4, %r8
	cmpq	%r14, %rdi
	jne	.LBB3_11
	jmp	.LBB3_22
.LBB3_63:
	movb	$48, (%rdx)
	movl	$1, %ebx
	movl	$1, %ecx
.LBB3_22:
	movb	$0, (%rdx,%rcx)
	movl	%ebx, %eax
	incl	%ebx
	movb	$32, (%rdx,%rax)
	leaq	(%rbx,%rdx), %r14
	testq	%rsi, %rsi
	je	.LBB3_47
# %bb.23:
	xorl	%r15d, %r15d
	movabsq	$-3689348814741910323, %rcx     # imm = 0xCCCCCCCCCCCCCCCD
	.p2align	4
.LBB3_24:                               # =>This Inner Loop Header: Depth=1
	movq	%rsi, %rax
	mulq	%rcx
	shrq	$3, %rdx
	leal	(%rdx,%rdx), %eax
	leal	(%rax,%rax,4), %eax
	movl	%esi, %edi
	subl	%eax, %edi
	orb	$48, %dil
	movl	%r15d, %eax
	incl	%r15d
	movb	%dil, 32(%rsp,%rax)
	cmpq	$10, %rsi
	movq	%rdx, %rsi
	jae	.LBB3_24
# %bb.25:
	movl	%r15d, %edx
	subl	$1, %edx
	jae	.LBB3_27
# %bb.26:
	movl	%r15d, %eax
	xorl	%r15d, %r15d
	movl	$20, %r12d
	jmp	.LBB3_48
.LBB3_47:
	movb	$48, (%rsp)
	movl	$1, %r15d
	movl	$19, %r12d
	movl	$1, %eax
.LBB3_48:
	movb	$0, (%rsp,%rax)
.LBB3_49:
	movl	$20, %edx
	subl	%r15d, %edx
	movq	%r14, %rdi
	movl	$32, %esi
	callq	memset@PLT
	cmpl	$1, %r12d
	adcl	$0, %r12d
	testl	%r15d, %r15d
	je	.LBB3_62
# %bb.50:
	movl	%r15d, %eax
	jmp	.LBB3_51
.LBB3_27:
	movl	%r15d, %eax
	cmpl	$3, %r15d
	jbe	.LBB3_28
# %bb.35:
	leaq	-1(%rax), %rcx
	cmpl	%ecx, %edx
	setb	%sil
	shrq	$32, %rcx
	setne	%cl
	orb	%sil, %cl
	je	.LBB3_36
.LBB3_28:
	xorl	%ecx, %ecx
.LBB3_29:
	movq	%rcx, %rdi
	testb	$3, %al
	je	.LBB3_32
# %bb.30:
	leal	-4(%r15), %edx
	movl	%ecx, %esi
	notl	%esi
	addl	%r15d, %esi
	andl	$3, %edx
	movq	%rcx, %rdi
	.p2align	4
.LBB3_31:                               # =>This Inner Loop Header: Depth=1
	movl	%esi, %r8d
	movzbl	32(%rsp,%r8), %r8d
	movb	%r8b, (%rsp,%rdi)
	incq	%rdi
	decl	%esi
	decq	%rdx
	jne	.LBB3_31
.LBB3_32:
	subq	%rax, %rcx
	cmpq	$-4, %rcx
	ja	.LBB3_45
# %bb.33:
	movq	%rdi, %rcx
	subq	%rax, %rcx
	movq	%rsp, %rdx
	addq	%rdi, %rdx
	addq	$3, %rdx
	movl	%r15d, %esi
	subl	%edi, %esi
	leal	-4(%rsi), %edi
	leal	-3(%rsi), %r8d
	leal	-2(%rsi), %r9d
	decl	%esi
	xorl	%r10d, %r10d
	.p2align	4
.LBB3_34:                               # =>This Inner Loop Header: Depth=1
	leal	(%rsi,%r10), %r11d
	movzbl	32(%rsp,%r11), %r11d
	movb	%r11b, -3(%rdx)
	leal	(%r9,%r10), %r11d
	movzbl	32(%rsp,%r11), %r11d
	movb	%r11b, -2(%rdx)
	leal	(%r8,%r10), %r11d
	movzbl	32(%rsp,%r11), %r11d
	movb	%r11b, -1(%rdx)
	leal	(%rdi,%r10), %r11d
	movzbl	32(%rsp,%r11), %r11d
	movb	%r11b, (%rdx)
	addq	$-4, %r10
	addq	$4, %rdx
	cmpq	%r10, %rcx
	jne	.LBB3_34
.LBB3_45:
	xorl	%r12d, %r12d
	movl	%r15d, %ecx
	movb	$0, (%rsp,%rax)
	subl	$20, %ecx
	jae	.LBB3_51
# %bb.46:
	negl	%ecx
	movl	%ecx, %r12d
	jmp	.LBB3_49
.LBB3_51:
	cmpl	$32, %eax
	jae	.LBB3_53
# %bb.52:
	xorl	%ecx, %ecx
.LBB3_58:
	movq	%rax, %rsi
	movq	%rcx, %rdx
	andq	$3, %rsi
	je	.LBB3_60
	.p2align	4
.LBB3_59:                               # =>This Inner Loop Header: Depth=1
	movzbl	(%rsp,%rdx), %edi
	movl	%r12d, %r8d
	incl	%r12d
	movb	%dil, (%r14,%r8)
	incq	%rdx
	decq	%rsi
	jne	.LBB3_59
.LBB3_60:
	subq	%rax, %rcx
	cmpq	$-4, %rcx
	ja	.LBB3_62
	.p2align	4
.LBB3_61:                               # =>This Inner Loop Header: Depth=1
	movzbl	(%rsp,%rdx), %ecx
	leal	1(%r12), %esi
	movl	%r12d, %edi
	movb	%cl, (%r14,%rdi)
	movzbl	1(%rsp,%rdx), %ecx
	leal	2(%r12), %edi
	movb	%cl, (%r14,%rsi)
	movzbl	2(%rsp,%rdx), %ecx
	leal	3(%r12), %esi
	movb	%cl, (%r14,%rdi)
	movzbl	3(%rsp,%rdx), %ecx
	addl	$4, %r12d
	movb	%cl, (%r14,%rsi)
	addq	$4, %rdx
	cmpq	%rdx, %rax
	jne	.LBB3_61
	jmp	.LBB3_62
.LBB3_53:
	leaq	-1(%rax), %rdx
	xorl	%ecx, %ecx
	movl	%r12d, %esi
	addl	%edx, %esi
	jb	.LBB3_58
# %bb.54:
	shrq	$32, %rdx
	jne	.LBB3_58
# %bb.55:
	movl	%eax, %ecx
	andl	$-32, %ecx
	movl	%r12d, %edx
	addl	%ecx, %r12d
	xorl	%esi, %esi
	.p2align	4
.LBB3_56:                               # =>This Inner Loop Header: Depth=1
	leal	(%rdx,%rsi), %edi
	movdqa	(%rsp,%rsi), %xmm0
	movdqa	16(%rsp,%rsi), %xmm1
	movdqu	%xmm0, (%r14,%rdi)
	movdqu	%xmm1, 16(%r14,%rdi)
	addq	$32, %rsi
	cmpq	%rsi, %rcx
	jne	.LBB3_56
# %bb.57:
	cmpl	%ecx, %eax
	jne	.LBB3_58
.LBB3_62:
	movl	%r12d, %eax
	movb	$0, (%r14,%rax)
	addl	%r12d, %ebx
	movl	%ebx, %eax
	addq	$64, %rsp
	popq	%rbx
	popq	%r12
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
.LBB3_13:
	cmpl	$32, %ebx
	jae	.LBB3_15
# %bb.14:
	xorl	%edi, %edi
	jmp	.LBB3_19
.LBB3_36:
	cmpl	$32, %r15d
	jae	.LBB3_38
# %bb.37:
	xorl	%ecx, %ecx
	jmp	.LBB3_42
.LBB3_15:
	movl	%ecx, %edi
	andl	$-32, %edi
	movl	%ebx, %r9d
	andl	$-32, %r9d
	xorl	%r10d, %r10d
	pxor	%xmm0, %xmm0
	.p2align	4
.LBB3_16:                               # =>This Inner Loop Header: Depth=1
	movl	%r8d, %r11d
	movdqu	-31(%rsp,%r11), %xmm1
	movdqu	-15(%rsp,%r11), %xmm2
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
	movdqu	%xmm2, (%rdx,%r10)
	movdqu	%xmm1, 16(%rdx,%r10)
	addq	$32, %r10
	addl	$-32, %r8d
	cmpq	%r10, %r9
	jne	.LBB3_16
# %bb.17:
	cmpl	%ecx, %edi
	je	.LBB3_22
# %bb.18:
	testb	$28, %cl
	je	.LBB3_6
.LBB3_19:
	movq	%rdi, %r8
	movl	%ecx, %edi
	andl	$-4, %edi
	movl	%ebx, %r9d
	andl	$-4, %r9d
	movl	%eax, %r10d
	subl	%r8d, %r10d
	incl	%r10d
	pxor	%xmm0, %xmm0
	.p2align	4
.LBB3_20:                               # =>This Inner Loop Header: Depth=1
	movl	%r10d, %r11d
	movd	-3(%rsp,%r11), %xmm1            # xmm1 = mem[0],zero,zero,zero
	punpcklbw	%xmm0, %xmm1            # xmm1 = xmm1[0],xmm0[0],xmm1[1],xmm0[1],xmm1[2],xmm0[2],xmm1[3],xmm0[3],xmm1[4],xmm0[4],xmm1[5],xmm0[5],xmm1[6],xmm0[6],xmm1[7],xmm0[7]
	pshuflw	$27, %xmm1, %xmm1               # xmm1 = xmm1[3,2,1,0,4,5,6,7]
	packuswb	%xmm1, %xmm1
	movd	%xmm1, (%rdx,%r8)
	addq	$4, %r8
	addl	$-4, %r10d
	cmpq	%r8, %r9
	jne	.LBB3_20
# %bb.21:
	cmpl	%ecx, %edi
	jne	.LBB3_6
	jmp	.LBB3_22
.LBB3_38:
	movl	%eax, %ecx
	andl	$-32, %ecx
	movl	%r15d, %esi
	andl	$-32, %esi
	xorl	%edi, %edi
	pxor	%xmm0, %xmm0
	.p2align	4
.LBB3_39:                               # =>This Inner Loop Header: Depth=1
	movl	%edx, %r8d
	movdqu	1(%rsp,%r8), %xmm1
	movdqu	17(%rsp,%r8), %xmm2
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
	movdqa	%xmm2, (%rsp,%rdi)
	movdqa	%xmm1, 16(%rsp,%rdi)
	addq	$32, %rdi
	addl	$-32, %edx
	cmpq	%rdi, %rsi
	jne	.LBB3_39
# %bb.40:
	cmpl	%eax, %ecx
	je	.LBB3_45
# %bb.41:
	testb	$28, %al
	je	.LBB3_29
.LBB3_42:
	movq	%rcx, %rdx
	movl	%eax, %ecx
	andl	$-4, %ecx
	movl	%r15d, %esi
	andl	$-4, %esi
	movl	%edx, %edi
	notl	%edi
	addl	%r15d, %edi
	pxor	%xmm0, %xmm0
	.p2align	4
.LBB3_43:                               # =>This Inner Loop Header: Depth=1
	movl	%edi, %r8d
	movd	29(%rsp,%r8), %xmm1             # xmm1 = mem[0],zero,zero,zero
	punpcklbw	%xmm0, %xmm1            # xmm1 = xmm1[0],xmm0[0],xmm1[1],xmm0[1],xmm1[2],xmm0[2],xmm1[3],xmm0[3],xmm1[4],xmm0[4],xmm1[5],xmm0[5],xmm1[6],xmm0[6],xmm1[7],xmm0[7]
	pshuflw	$27, %xmm1, %xmm1               # xmm1 = xmm1[3,2,1,0,4,5,6,7]
	packuswb	%xmm1, %xmm1
	movd	%xmm1, (%rsp,%rdx)
	addq	$4, %rdx
	addl	$-4, %edi
	cmpq	%rdx, %rsi
	jne	.LBB3_43
# %bb.44:
	cmpl	%eax, %ecx
	jne	.LBB3_29
	jmp	.LBB3_45
.Lfunc_end3:
	.size	render_line, .Lfunc_end3-render_line
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
