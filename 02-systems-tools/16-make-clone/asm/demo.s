	.file	"demo.c"
	.text
	.globl	mk_needs_rebuild                # -- Begin function mk_needs_rebuild
	.p2align	4
	.type	mk_needs_rebuild,@function
mk_needs_rebuild:                       # @mk_needs_rebuild
# %bb.0:
	orl	%esi, %edi
	movl	$1, %eax
	orl	%edx, %edi
	je	.LBB0_1
.LBB0_7:
	retq
.LBB0_1:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	16(%rbp), %edx
	testl	%edx, %edx
	popq	%rbp
	jle	.LBB0_6
# %bb.2:
	movl	%edx, %edx
	xorl	%esi, %esi
	.p2align	4
.LBB0_3:                                # =>This Inner Loop Header: Depth=1
	cmpl	$0, (%r9,%rsi,4)
	jne	.LBB0_7
# %bb.4:                                #   in Loop: Header=BB0_3 Depth=1
	cmpq	%rcx, (%r8,%rsi,8)
	jg	.LBB0_7
# %bb.5:                                #   in Loop: Header=BB0_3 Depth=1
	incq	%rsi
	cmpq	%rsi, %rdx
	jne	.LBB0_3
.LBB0_6:
	xorl	%eax, %eax
	retq
.Lfunc_end0:
	.size	mk_needs_rebuild, .Lfunc_end0-mk_needs_rebuild
                                        # -- End function
	.globl	mk_toposort                     # -- Begin function mk_toposort
	.p2align	4
	.type	mk_toposort,@function
mk_toposort:                            # @mk_toposort
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	subq	$136, %rsp
	movq	%rdx, %rbx
	movq	%rsi, %r14
	movl	%edi, %r12d
	movl	%edi, %r15d
	testl	%edi, %edi
	jle	.LBB1_5
# %bb.1:
	leaq	-112(%rbp), %rdi
	xorl	%r13d, %r13d
	xorl	%esi, %esi
	movq	%r15, %rdx
	callq	memset@PLT
	movq	%r14, %rax
	.p2align	4
.LBB1_2:                                # =>This Loop Header: Depth=1
                                        #     Child Loop BB1_3 Depth 2
	xorl	%edx, %edx
	xorl	%ecx, %ecx
	.p2align	4
.LBB1_3:                                #   Parent Loop BB1_2 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	cmpb	$1, (%rax,%rdx)
	sbbl	$-1, %ecx
	incq	%rdx
	cmpq	%rdx, %r15
	jne	.LBB1_3
# %bb.4:                                #   in Loop: Header=BB1_2 Depth=1
	movb	%cl, -176(%rbp,%r13)
	incq	%r13
	addq	%r15, %rax
	cmpq	%r15, %r13
	jne	.LBB1_2
.LBB1_5:
	testl	%r12d, %r12d
	jle	.LBB1_6
# %bb.7:
	xorl	%ecx, %ecx
	xorl	%eax, %eax
	.p2align	4
.LBB1_8:                                # =>This Loop Header: Depth=1
                                        #     Child Loop BB1_9 Depth 2
                                        #     Child Loop BB1_15 Depth 2
	xorl	%edx, %edx
	jmp	.LBB1_9
	.p2align	4
.LBB1_11:                               #   in Loop: Header=BB1_9 Depth=2
	incq	%rdx
	cmpq	%rdx, %r15
	je	.LBB1_12
.LBB1_9:                                #   Parent Loop BB1_8 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	cmpb	$0, -112(%rbp,%rdx)
	jne	.LBB1_11
# %bb.10:                               #   in Loop: Header=BB1_9 Depth=2
	cmpb	$0, -176(%rbp,%rdx)
	jne	.LBB1_11
# %bb.13:                               #   in Loop: Header=BB1_8 Depth=1
	testl	%edx, %edx
	jns	.LBB1_14
	jmp	.LBB1_20
	.p2align	4
.LBB1_12:                               #   in Loop: Header=BB1_8 Depth=1
	movl	$-1, %edx
	testl	%edx, %edx
	js	.LBB1_20
.LBB1_14:                               #   in Loop: Header=BB1_8 Depth=1
	movslq	%eax, %rsi
	incl	%eax
	movl	%edx, (%rbx,%rsi,4)
	movl	%edx, %esi
	movb	$1, -112(%rbp,%rsi)
	addq	%r14, %rsi
	xorl	%edi, %edi
	jmp	.LBB1_15
	.p2align	4
.LBB1_19:                               #   in Loop: Header=BB1_15 Depth=2
	incq	%rdi
	addq	%r15, %rsi
	cmpq	%rdi, %r15
	je	.LBB1_20
.LBB1_15:                               #   Parent Loop BB1_8 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	cmpb	$0, -112(%rbp,%rdi)
	jne	.LBB1_19
# %bb.16:                               #   in Loop: Header=BB1_15 Depth=2
	cmpb	$0, (%rsi)
	je	.LBB1_19
# %bb.17:                               #   in Loop: Header=BB1_15 Depth=2
	movzbl	-176(%rbp,%rdi), %r8d
	testb	%r8b, %r8b
	je	.LBB1_19
# %bb.18:                               #   in Loop: Header=BB1_15 Depth=2
	decb	%r8b
	movb	%r8b, -176(%rbp,%rdi)
	jmp	.LBB1_19
	.p2align	4
.LBB1_20:                               #   in Loop: Header=BB1_8 Depth=1
	testl	%edx, %edx
	js	.LBB1_22
# %bb.21:                               #   in Loop: Header=BB1_8 Depth=1
	incl	%ecx
	cmpl	%r15d, %ecx
	jne	.LBB1_8
	jmp	.LBB1_22
.LBB1_6:
	xorl	%eax, %eax
.LBB1_22:
	addq	$136, %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
.Lfunc_end1:
	.size	mk_toposort, .Lfunc_end1-mk_toposort
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
