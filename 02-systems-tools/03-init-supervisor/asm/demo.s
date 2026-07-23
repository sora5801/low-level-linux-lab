	.file	"demo.c"
	.text
	.globl	backoff_delay_ms                # -- Begin function backoff_delay_ms
	.p2align	4
	.type	backoff_delay_ms,@function
backoff_delay_ms:                       # @backoff_delay_ms
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdx, %rax
	cmpl	$63, %edi
	movl	$63, %ecx
	cmovbl	%edi, %ecx
	testl	%edi, %edi
	je	.LBB0_3
	.p2align	4
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	cmpq	%rax, %rsi
	jae	.LBB0_4
# %bb.2:                                #   in Loop: Header=BB0_1 Depth=1
	addq	%rsi, %rsi
	decl	%ecx
	jne	.LBB0_1
.LBB0_3:
	cmpq	%rax, %rsi
	cmovbq	%rsi, %rax
.LBB0_4:
	popq	%rbp
	retq
.Lfunc_end0:
	.size	backoff_delay_ms, .Lfunc_end0-backoff_delay_ms
                                        # -- End function
	.globl	toposort                        # -- Begin function toposort
	.p2align	4
	.type	toposort,@function
toposort:                               # @toposort
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
	.size	toposort, .Lfunc_end1-toposort
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
