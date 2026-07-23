	.file	"demo.c"
	.text
	.globl	apply_relocations               # -- Begin function apply_relocations
	.p2align	4
	.type	apply_relocations,@function
apply_relocations:                      # @apply_relocations
# %bb.0:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	pushq	%rax
	testq	%rdx, %rdx
	je	.LBB0_1
# %bb.2:
	movq	%rcx, %r14
	movq	%rdx, %r15
	movq	%rsi, %r12
	movq	%rdi, %r13
	addq	$16, %r12
	xorl	%ebx, %ebx
	jmp	.LBB0_3
	.p2align	4
.LBB0_6:                                #   in Loop: Header=BB0_3 Depth=1
	shrq	$32, %rcx
	leaq	(%rcx,%rcx,2), %rcx
	addq	%r13, %rax
	addq	8(%r14,%rcx,8), %rax
.LBB0_12:                               #   in Loop: Header=BB0_3 Depth=1
	movq	%rax, (%rbp,%r13)
	incq	%rbx
.LBB0_13:                               #   in Loop: Header=BB0_3 Depth=1
	addq	$24, %r12
	decq	%r15
	je	.LBB0_14
.LBB0_3:                                # =>This Inner Loop Header: Depth=1
	movq	-16(%r12), %rbp
	movq	-8(%r12), %rcx
	movq	(%r12), %rax
	cmpl	$5, %ecx
	jle	.LBB0_4
# %bb.7:                                #   in Loop: Header=BB0_3 Depth=1
	leal	-6(%rcx), %edx
	cmpl	$2, %edx
	jb	.LBB0_6
# %bb.8:                                #   in Loop: Header=BB0_3 Depth=1
	cmpl	$37, %ecx
	je	.LBB0_11
# %bb.9:                                #   in Loop: Header=BB0_3 Depth=1
	cmpl	$8, %ecx
	jne	.LBB0_14
# %bb.10:                               #   in Loop: Header=BB0_3 Depth=1
	addq	%r13, %rax
	jmp	.LBB0_12
	.p2align	4
.LBB0_4:                                #   in Loop: Header=BB0_3 Depth=1
	testl	%ecx, %ecx
	je	.LBB0_13
# %bb.5:                                #   in Loop: Header=BB0_3 Depth=1
	cmpl	$1, %ecx
	je	.LBB0_6
	jmp	.LBB0_14
.LBB0_11:                               #   in Loop: Header=BB0_3 Depth=1
	addq	%r13, %rax
	callq	*%rax
	jmp	.LBB0_12
.LBB0_1:
	xorl	%ebx, %ebx
.LBB0_14:
	movq	%rbx, %rax
	addq	$8, %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
.Lfunc_end0:
	.size	apply_relocations, .Lfunc_end0-apply_relocations
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
