	.file	"demo.c"
	.text
	.globl	dns_decode_name                 # -- Begin function dns_decode_name
	.p2align	4
	.type	dns_decode_name,@function
dns_decode_name:                        # @dns_decode_name
# %bb.0:
                                        # kill: def $edx killed $edx def $rdx
                                        # kill: def $esi killed $esi def $rsi
	testl	%r8d, %r8d
	je	.LBB0_43
# %bb.1:
	movb	$0, (%rcx)
	cmpl	%esi, %edx
	jae	.LBB0_43
# %bb.2:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	leal	1(%rsi), %eax
	movl	%eax, -4(%rsp)                  # 4-byte Spill
	xorl	%eax, %eax
	xorl	%r10d, %r10d
	movl	%r8d, -12(%rsp)                 # 4-byte Spill
	jmp	.LBB0_5
	.p2align	4
.LBB0_3:                                #   in Loop: Header=BB0_5 Depth=1
	movl	-8(%rsp), %edx                  # 4-byte Reload
                                        # kill: def $edx killed $edx def $rdx
	movl	-12(%rsp), %r8d                 # 4-byte Reload
.LBB0_4:                                #   in Loop: Header=BB0_5 Depth=1
	cmpl	%esi, %edx
	jae	.LBB0_54
.LBB0_5:                                # =>This Loop Header: Depth=1
                                        #     Child Loop BB0_36 Depth 2
                                        #     Child Loop BB0_40 Depth 2
                                        #     Child Loop BB0_23 Depth 2
                                        #     Child Loop BB0_26 Depth 2
	movl	%edx, %r9d
	movzbl	(%rdi,%r9), %r13d
	movl	%r13d, %r11d
	andl	$192, %r11d
	je	.LBB0_10
# %bb.6:                                #   in Loop: Header=BB0_5 Depth=1
	cmpl	$192, %r11d
	jne	.LBB0_54
# %bb.7:                                #   in Loop: Header=BB0_5 Depth=1
	leal	2(%rdx), %r11d
	cmpl	%esi, %r11d
	ja	.LBB0_54
# %bb.8:                                #   in Loop: Header=BB0_5 Depth=1
	andl	$63, %r13d
	shll	$8, %r13d
	movzbl	1(%r9,%rdi), %r9d
	orl	%r13d, %r9d
	incl	%r10d
	cmpl	%esi, %r9d
	cmovbl	%r9d, %edx
	cmpl	-4(%rsp), %r10d                 # 4-byte Folded Reload
	ja	.LBB0_54
# %bb.9:                                #   in Loop: Header=BB0_5 Depth=1
	cmpl	%esi, %r9d
	jb	.LBB0_4
	jmp	.LBB0_54
	.p2align	4
.LBB0_10:                               #   in Loop: Header=BB0_5 Depth=1
	testl	%r13d, %r13d
	je	.LBB0_47
# %bb.11:                               #   in Loop: Header=BB0_5 Depth=1
	cmpb	$63, %r13b
	ja	.LBB0_54
# %bb.12:                               #   in Loop: Header=BB0_5 Depth=1
	leal	(%rdx,%r13), %r11d
	incl	%r11d
	cmpl	%esi, %r11d
	ja	.LBB0_54
# %bb.13:                               #   in Loop: Header=BB0_5 Depth=1
	testl	%eax, %eax
	je	.LBB0_16
# %bb.14:                               #   in Loop: Header=BB0_5 Depth=1
	leal	1(%rax), %r14d
	cmpl	%r8d, %r14d
	jae	.LBB0_54
# %bb.15:                               #   in Loop: Header=BB0_5 Depth=1
	movl	%eax, %eax
	movb	$46, (%rcx,%rax)
	jmp	.LBB0_17
.LBB0_16:                               #   in Loop: Header=BB0_5 Depth=1
	xorl	%r14d, %r14d
.LBB0_17:                               #   in Loop: Header=BB0_5 Depth=1
	leal	(%r14,%r13), %eax
	cmpl	%r8d, %eax
	jae	.LBB0_54
# %bb.18:                               #   in Loop: Header=BB0_5 Depth=1
	cmpl	$255, %eax
	ja	.LBB0_54
# %bb.19:                               #   in Loop: Header=BB0_5 Depth=1
	leal	1(%rdx), %r8d
	cmpl	$1, %r13d
	movl	%r13d, %ebx
	adcl	$0, %ebx
	cmpb	$4, %r13b
	movl	%r11d, -8(%rsp)                 # 4-byte Spill
	jae	.LBB0_27
.LBB0_20:                               #   in Loop: Header=BB0_5 Depth=1
	xorl	%r12d, %r12d
.LBB0_21:                               #   in Loop: Header=BB0_5 Depth=1
	movq	%r8, %r11
	movq	%rbx, %rbp
	movq	%r12, %r13
	andq	$3, %rbp
	je	.LBB0_24
# %bb.22:                               #   in Loop: Header=BB0_5 Depth=1
	movl	%r14d, %r9d
	incl	%edx
	movq	%r12, %r13
	.p2align	4
.LBB0_23:                               #   Parent Loop BB0_5 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	leal	(%rdx,%r13), %r8d
	movzbl	(%rdi,%r8), %r8d
	leal	(%r9,%r13), %r15d
	movb	%r8b, (%rcx,%r15)
	incq	%r13
	decq	%rbp
	jne	.LBB0_23
.LBB0_24:                               #   in Loop: Header=BB0_5 Depth=1
	subq	%rbx, %r12
	cmpq	$-4, %r12
	ja	.LBB0_3
# %bb.25:                               #   in Loop: Header=BB0_5 Depth=1
	leal	1(%r11), %edx
	leal	2(%r11), %r12d
	leal	3(%r11), %ebp
	movl	%r14d, %r14d
	movl	%r11d, %r15d
	.p2align	4
.LBB0_26:                               #   Parent Loop BB0_5 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	leal	(%r15,%r13), %r8d
	movzbl	(%rdi,%r8), %r8d
	leal	(%r14,%r13), %r9d
	movb	%r8b, (%rcx,%r9)
	leal	(%rdx,%r13), %r8d
	movzbl	(%rdi,%r8), %r8d
	leal	1(%r13,%r14), %r9d
	movb	%r8b, (%rcx,%r9)
	leal	(%r12,%r13), %r8d
	movzbl	(%rdi,%r8), %r8d
	leal	2(%r13,%r14), %r9d
	movb	%r8b, (%rcx,%r9)
	leal	(%rbp,%r13), %r8d
	movzbl	(%rdi,%r8), %r8d
	leal	(%r14,%r13), %r9d
	addl	$3, %r9d
	movb	%r8b, (%rcx,%r9)
	addq	$4, %r14
	addq	$4, %rbp
	addq	$4, %r12
	addq	$4, %rdx
	addq	$-4, %rbx
	addq	$4, %r15
	cmpq	%rbx, %r13
	jne	.LBB0_26
	jmp	.LBB0_3
	.p2align	4
.LBB0_27:                               #   in Loop: Header=BB0_5 Depth=1
	leaq	-1(%rbx), %r9
	movl	%r14d, %ebp
	addl	%r9d, %ebp
	jb	.LBB0_20
# %bb.28:                               #   in Loop: Header=BB0_5 Depth=1
	movl	$-2, %ebp
	subl	%edx, %ebp
	cmpl	%r9d, %ebp
	jb	.LBB0_20
# %bb.29:                               #   in Loop: Header=BB0_5 Depth=1
	shrq	$32, %r9
	movl	$0, %r12d
	jne	.LBB0_21
# %bb.30:                               #   in Loop: Header=BB0_5 Depth=1
	movl	%r14d, %r9d
	addq	%rcx, %r9
	subq	%rdi, %r9
	subq	%r8, %r9
	cmpq	$32, %r9
	jb	.LBB0_20
# %bb.32:                               #   in Loop: Header=BB0_5 Depth=1
	cmpb	$32, %r13b
	jae	.LBB0_35
# %bb.33:                               #   in Loop: Header=BB0_5 Depth=1
	xorl	%r12d, %r12d
	jmp	.LBB0_39
.LBB0_35:                               #   in Loop: Header=BB0_5 Depth=1
	movl	%ebx, %r12d
	andl	$32, %r12d
	movl	%r14d, %r9d
	movl	%r8d, %ebp
	movq	%r12, %r13
	.p2align	4
.LBB0_36:                               #   Parent Loop BB0_5 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movl	%ebp, %r15d
	movups	(%rdi,%r15), %xmm0
	movups	16(%rdi,%r15), %xmm1
	movl	%r9d, %r15d
	movups	%xmm0, (%rcx,%r15)
	movups	%xmm1, 16(%rcx,%r15)
	addl	$32, %ebp
	addl	$32, %r9d
	addq	$-32, %r13
	jne	.LBB0_36
# %bb.37:                               #   in Loop: Header=BB0_5 Depth=1
	cmpl	%ebx, %r12d
	je	.LBB0_3
# %bb.38:                               #   in Loop: Header=BB0_5 Depth=1
	testb	$28, %bl
	je	.LBB0_21
.LBB0_39:                               #   in Loop: Header=BB0_5 Depth=1
	movq	%r8, %r11
	movq	%r12, %r13
	movl	%ebx, %r12d
	andl	$60, %r12d
	movq	%r13, %rbp
	subq	%r12, %rbp
	leal	(%rdx,%r13), %r9d
	incl	%r9d
	addl	%r14d, %r13d
	.p2align	4
.LBB0_40:                               #   Parent Loop BB0_5 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movl	%r9d, %r15d
	movl	(%rdi,%r15), %r15d
	movl	%r13d, %r8d
	movl	%r15d, (%rcx,%r8)
	addl	$4, %r9d
	addl	$4, %r13d
	addq	$4, %rbp
	jne	.LBB0_40
# %bb.41:                               #   in Loop: Header=BB0_5 Depth=1
	cmpl	%ebx, %r12d
	movq	%r11, %r8
	je	.LBB0_3
	jmp	.LBB0_21
.LBB0_43:
	movl	$-1, %eax
                                        # kill: def $eax killed $eax killed $rax
	retq
.LBB0_54:
	movl	$-1, %eax
.LBB0_55:
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
                                        # kill: def $eax killed $eax killed $rax
	retq
.LBB0_47:
	movl	%eax, %edx
	movb	$0, (%rcx,%rdx)
	jmp	.LBB0_55
.Lfunc_end0:
	.size	dns_decode_name, .Lfunc_end0-dns_decode_name
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
