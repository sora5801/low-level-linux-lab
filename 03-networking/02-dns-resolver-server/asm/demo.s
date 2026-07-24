	.file	"demo.c"
	.text
	.globl	dns_decode_name                 # -- Begin function dns_decode_name
	.p2align	4
	.type	dns_decode_name,@function
dns_decode_name:                        # @dns_decode_name
# %bb.0:
                                        # kill: def $edx killed $edx def $rdx
                                        # kill: def $esi killed $esi def $rsi
	movl	$-1, %eax
	testl	%r8d, %r8d
	je	.LBB0_30
# %bb.1:
	movb	$0, (%rcx)
	cmpl	%esi, %edx
	jae	.LBB0_30
# %bb.2:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	leal	1(%rsi), %r9d
	xorl	%ebx, %ebx
	movl	$1, %r10d
	xorl	%r11d, %r11d
	jmp	.LBB0_4
.LBB0_21:                               #   in Loop: Header=BB0_4 Depth=1
	movl	%r15d, %ebx
	.p2align	4
.LBB0_11:                               #   in Loop: Header=BB0_4 Depth=1
	movl	$1, %r14d
	testl	%r14d, %r14d
	je	.LBB0_3
.LBB0_26:                               #   in Loop: Header=BB0_4 Depth=1
	cmpl	$3, %r14d
	jne	.LBB0_27
.LBB0_3:                                #   in Loop: Header=BB0_4 Depth=1
	cmpl	%esi, %edx
	jae	.LBB0_29
.LBB0_4:                                # =>This Loop Header: Depth=1
                                        #     Child Loop BB0_23 Depth 2
	movl	%edx, %r14d
	movzbl	(%rdi,%r14), %r14d
	movl	%r14d, %r15d
	andl	$192, %r15d
	je	.LBB0_8
# %bb.5:                                #   in Loop: Header=BB0_4 Depth=1
	cmpl	$192, %r15d
	jne	.LBB0_11
# %bb.6:                                #   in Loop: Header=BB0_4 Depth=1
	leal	2(%rdx), %r15d
	cmpl	%esi, %r15d
	ja	.LBB0_11
# %bb.12:                               #   in Loop: Header=BB0_4 Depth=1
	andl	$63, %r14d
	shll	$8, %r14d
	leal	1(%rdx), %r15d
	movzbl	(%rdi,%r15), %r15d
	orl	%r14d, %r15d
	incl	%r11d
	xorl	%r14d, %r14d
	cmpl	%esi, %r15d
	setb	%r14b
	cmovael	%edx, %r15d
	cmpl	%r9d, %r11d
	leal	1(%r14,%r14), %r14d
	cmoval	%r10d, %r14d
	cmoval	%edx, %r15d
	movl	%r15d, %edx
	testl	%r14d, %r14d
	jne	.LBB0_26
	jmp	.LBB0_3
	.p2align	4
.LBB0_8:                                #   in Loop: Header=BB0_4 Depth=1
	testl	%r14d, %r14d
	je	.LBB0_13
# %bb.9:                                #   in Loop: Header=BB0_4 Depth=1
	cmpb	$63, %r14b
	ja	.LBB0_11
# %bb.14:                               #   in Loop: Header=BB0_4 Depth=1
	leal	(%rdx,%r14), %r15d
	incl	%r15d
	cmpl	%esi, %r15d
	ja	.LBB0_11
# %bb.16:                               #   in Loop: Header=BB0_4 Depth=1
	testl	%ebx, %ebx
	je	.LBB0_19
# %bb.17:                               #   in Loop: Header=BB0_4 Depth=1
	leal	1(%rbx), %r15d
	cmpl	%r8d, %r15d
	jae	.LBB0_11
# %bb.18:                               #   in Loop: Header=BB0_4 Depth=1
	movl	%ebx, %ebx
	movb	$46, (%rcx,%rbx)
	jmp	.LBB0_20
.LBB0_13:                               #   in Loop: Header=BB0_4 Depth=1
	movl	$2, %r14d
	testl	%r14d, %r14d
	jne	.LBB0_26
	jmp	.LBB0_3
.LBB0_19:                               #   in Loop: Header=BB0_4 Depth=1
	xorl	%r15d, %r15d
.LBB0_20:                               #   in Loop: Header=BB0_4 Depth=1
	leal	(%r15,%r14), %ebx
	cmpl	%r8d, %ebx
	setae	%r12b
	cmpl	$256, %ebx                      # imm = 0x100
	setae	%r13b
	orb	%r12b, %r13b
	jne	.LBB0_21
# %bb.22:                               #   in Loop: Header=BB0_4 Depth=1
	movl	%r9d, -44(%rbp)                 # 4-byte Spill
	cmpl	$1, %r14d
	movl	%r14d, %r12d
	adcl	$0, %r12d
	leal	1(%rdx), %r13d
	.p2align	4
.LBB0_23:                               #   Parent Loop BB0_4 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movl	%r13d, %r10d
	movzbl	(%rdi,%r10), %r10d
	movl	%r15d, %r9d
	movb	%r10b, (%rcx,%r9)
	incl	%r15d
	incl	%r13d
	decq	%r12
	jne	.LBB0_23
# %bb.24:                               #   in Loop: Header=BB0_4 Depth=1
	addl	%r14d, %edx
	incl	%edx
	xorl	%r14d, %r14d
	movl	-44(%rbp), %r9d                 # 4-byte Reload
	movl	$1, %r10d
	testl	%r14d, %r14d
	jne	.LBB0_26
	jmp	.LBB0_3
.LBB0_27:
	cmpl	$2, %r14d
	jne	.LBB0_29
# %bb.28:
	movl	%ebx, %eax
	movb	$0, (%rcx,%rax)
	movl	%ebx, %eax
.LBB0_29:
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
.LBB0_30:
	retq
.Lfunc_end0:
	.size	dns_decode_name, .Lfunc_end0-dns_decode_name
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
