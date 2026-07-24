	.file	"demo.c"
	.text
	.globl	syscall_args                    # -- Begin function syscall_args
	.p2align	4
	.type	syscall_args,@function
syscall_args:                           # @syscall_args
# %bb.0:
	movq	8(%rdi), %rax
	movq	%rax, (%rsi)
	movq	16(%rdi), %rax
	movq	%rax, 8(%rsi)
	movq	24(%rdi), %rax
	movq	%rax, 16(%rsi)
	movq	32(%rdi), %rax
	movq	%rax, 24(%rsi)
	movq	40(%rdi), %rax
	movq	%rax, 32(%rsi)
	movq	48(%rdi), %rax
	movq	%rax, 40(%rsi)
	movq	(%rdi), %rax
	retq
.Lfunc_end0:
	.size	syscall_args, .Lfunc_end0-syscall_args
                                        # -- End function
	.globl	decode_flags                    # -- Begin function decode_flags
	.p2align	4
	.type	decode_flags,@function
decode_flags:                           # @decode_flags
# %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r12
	pushq	%rbx
                                        # kill: def $r8d killed $r8d def $r8
	leal	-1(%r8), %r9d
	testl	%edx, %edx
	setle	%r10b
	cmpl	$2, %r8d
	setge	%bl
	setl	%r11b
	xorl	%eax, %eax
	orb	%r10b, %r11b
	jne	.LBB1_1
# %bb.15:
	movl	%r9d, %r10d
	movl	%edx, %edx
	xorl	%r11d, %r11d
	xorl	%ebp, %ebp
	xorl	%eax, %eax
	.p2align	4
.LBB1_16:                               # =>This Loop Header: Depth=1
                                        #     Child Loop BB1_23 Depth 2
	movq	%r11, %rbx
	shlq	$4, %rbx
	movq	(%rsi,%rbx), %r14
	testq	%r14, %r14
	je	.LBB1_26
# %bb.17:                               #   in Loop: Header=BB1_16 Depth=1
	movq	%r14, %r15
	andq	%rdi, %r15
	cmpq	%r14, %r15
	jne	.LBB1_26
# %bb.18:                               #   in Loop: Header=BB1_16 Depth=1
	testl	%ebp, %ebp
	je	.LBB1_20
# %bb.19:                               #   in Loop: Header=BB1_16 Depth=1
	movslq	%eax, %r14
	incl	%eax
	movb	$124, (%rcx,%r14)
.LBB1_20:                               #   in Loop: Header=BB1_16 Depth=1
	addq	%rsi, %rbx
	movq	8(%rbx), %r14
	movzbl	(%r14), %ebp
	testb	%bpl, %bpl
	je	.LBB1_25
# %bb.21:                               #   in Loop: Header=BB1_16 Depth=1
	cmpl	%r9d, %eax
	jge	.LBB1_25
# %bb.22:                               #   in Loop: Header=BB1_16 Depth=1
	movslq	%eax, %r15
	incq	%r15
	incq	%r14
	.p2align	4
.LBB1_23:                               #   Parent Loop BB1_16 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movb	%bpl, -1(%rcx,%r15)
	movzbl	(%r14), %ebp
	incl	%eax
	testb	%bpl, %bpl
	je	.LBB1_25
# %bb.24:                               #   in Loop: Header=BB1_23 Depth=2
	leaq	1(%r15), %r12
	incq	%r14
	cmpq	%r10, %r15
	movq	%r12, %r15
	jl	.LBB1_23
.LBB1_25:                               #   in Loop: Header=BB1_16 Depth=1
	movq	(%rbx), %rbx
	notq	%rbx
	andq	%rbx, %rdi
	movl	$1, %ebp
.LBB1_26:                               #   in Loop: Header=BB1_16 Depth=1
	incq	%r11
	cmpl	%r9d, %eax
	setl	%bl
	cmpq	%rdx, %r11
	jae	.LBB1_2
# %bb.27:                               #   in Loop: Header=BB1_16 Depth=1
	cmpl	%r9d, %eax
	jl	.LBB1_16
.LBB1_2:
	testq	%rdi, %rdi
	je	.LBB1_32
.LBB1_3:
	testl	%ebp, %ebp
	setne	%dl
	andb	%bl, %dl
	cmpb	$1, %dl
	je	.LBB1_4
# %bb.5:
	cmpl	%r9d, %eax
	jl	.LBB1_6
.LBB1_7:
	cmpl	%r9d, %eax
	jge	.LBB1_9
.LBB1_8:
	movslq	%eax, %rdx
	incl	%eax
	movb	$120, (%rcx,%rdx)
.LBB1_9:
	movq	$-1, %r10
	xorl	%r11d, %r11d
	leaq	put_hex.digits(%rip), %rsi
	.p2align	4
.LBB1_10:                               # =>This Inner Loop Header: Depth=1
	movl	%edi, %edx
	andl	$15, %edx
	movzbl	(%rdx,%rsi), %ebx
	leaq	1(%r11), %rdx
	movb	%bl, -16(%rsp,%r11)
	incq	%r10
	cmpq	$16, %rdi
	jb	.LBB1_12
# %bb.11:                               #   in Loop: Header=BB1_10 Depth=1
	shrq	$4, %rdi
	cmpq	$15, %r11
	movq	%rdx, %r11
	jb	.LBB1_10
.LBB1_12:
	cmpl	%r9d, %eax
	jge	.LBB1_34
# %bb.13:
	leaq	-1(%rdx), %r11
	movslq	%eax, %rbx
	movslq	%r9d, %rsi
	movq	%rbx, %rdi
	notq	%rdi
	addq	%rsi, %rdi
	cmpq	%r11, %rdi
	cmovbq	%rdi, %r11
	cmpq	$15, %r11
	jae	.LBB1_28
# %bb.14:
	movq	%rbx, %rax
	jmp	.LBB1_30
.LBB1_1:
	xorl	%ebp, %ebp
	testq	%rdi, %rdi
	jne	.LBB1_3
.LBB1_32:
	testl	%ebp, %ebp
	sete	%dl
	andb	%bl, %dl
	cmpb	$1, %dl
	jne	.LBB1_34
# %bb.33:
	movslq	%eax, %rdx
	incl	%eax
	movb	$48, (%rcx,%rdx)
	jmp	.LBB1_34
.LBB1_4:
	movslq	%eax, %rdx
	incl	%eax
	movb	$124, (%rcx,%rdx)
	cmpl	%r9d, %eax
	jge	.LBB1_7
.LBB1_6:
	movslq	%eax, %rdx
	incl	%eax
	movb	$48, (%rcx,%rdx)
	cmpl	%r9d, %eax
	jl	.LBB1_8
	jmp	.LBB1_9
.LBB1_28:
	incq	%r11
	movabsq	$9223372036854775792, %r14      # imm = 0x7FFFFFFFFFFFFFF0
	andq	%r11, %r14
	leaq	(%r14,%rbx), %rax
	cmpq	%rdi, %r10
	cmovbq	%r10, %rdi
	movdqu	-32(%rsp,%rdx), %xmm0
	pxor	%xmm1, %xmm1
	movdqa	%xmm0, %xmm2
	punpcklbw	%xmm1, %xmm2            # xmm2 = xmm2[0],xmm1[0],xmm2[1],xmm1[1],xmm2[2],xmm1[2],xmm2[3],xmm1[3],xmm2[4],xmm1[4],xmm2[5],xmm1[5],xmm2[6],xmm1[6],xmm2[7],xmm1[7]
	pshufd	$78, %xmm2, %xmm2               # xmm2 = xmm2[2,3,0,1]
	pshuflw	$27, %xmm2, %xmm2               # xmm2 = xmm2[3,2,1,0,4,5,6,7]
	pshufhw	$27, %xmm2, %xmm2               # xmm2 = xmm2[0,1,2,3,7,6,5,4]
	punpckhbw	%xmm1, %xmm0            # xmm0 = xmm0[8],xmm1[8],xmm0[9],xmm1[9],xmm0[10],xmm1[10],xmm0[11],xmm1[11],xmm0[12],xmm1[12],xmm0[13],xmm1[13],xmm0[14],xmm1[14],xmm0[15],xmm1[15]
	pshufd	$78, %xmm0, %xmm0               # xmm0 = xmm0[2,3,0,1]
	pshuflw	$27, %xmm0, %xmm0               # xmm0 = xmm0[3,2,1,0,4,5,6,7]
	pshufhw	$27, %xmm0, %xmm0               # xmm0 = xmm0[0,1,2,3,7,6,5,4]
	packuswb	%xmm2, %xmm0
	movdqu	%xmm0, (%rcx,%rbx)
	cmpq	%r14, %r11
	je	.LBB1_34
# %bb.29:
	incq	%rdi
	andq	$-16, %rdi
	subq	%rdi, %rdx
	.p2align	4
.LBB1_30:                               # =>This Inner Loop Header: Depth=1
	movzbl	-17(%rsp,%rdx), %edi
	movb	%dil, (%rcx,%rax)
	incq	%rax
	cmpq	$2, %rdx
	jl	.LBB1_34
# %bb.31:                               #   in Loop: Header=BB1_30 Depth=1
	decq	%rdx
	cmpq	%rsi, %rax
	jl	.LBB1_30
.LBB1_34:
	cmpl	%r8d, %eax
	jge	.LBB1_36
# %bb.35:
	movslq	%eax, %rdx
	jmp	.LBB1_38
.LBB1_36:
	testl	%r8d, %r8d
	jle	.LBB1_39
# %bb.37:
	movl	%r9d, %edx
.LBB1_38:
	movb	$0, (%rcx,%rdx)
.LBB1_39:
                                        # kill: def $eax killed $eax killed $rax
	popq	%rbx
	popq	%r12
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
.Lfunc_end1:
	.size	decode_flags, .Lfunc_end1-decode_flags
                                        # -- End function
	.type	put_hex.digits,@object          # @put_hex.digits
	.section	.rodata.str1.16,"aMS",@progbits,1
	.p2align	4, 0x0
put_hex.digits:
	.asciz	"0123456789abcdef"
	.size	put_hex.digits, 17

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
