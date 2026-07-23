	.file	"demo.c"
	.text
	.globl	apply_relocations               # -- Begin function apply_relocations
	.p2align	4
	.type	apply_relocations,@function
apply_relocations:                      # @apply_relocations
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	subq	$24, %rsp
	testq	%rdx, %rdx
	je	.LBB0_1
# %bb.2:
	movq	%rdx, %r14
	movq	%rsi, %r12
	movq	%rdi, %r13
	addq	$16, %r12
	xorl	%r15d, %r15d
                                        # implicit-def: $rax
                                        # kill: killed $rax
	movq	%rcx, -56(%rbp)                 # 8-byte Spill
	.p2align	4
.LBB0_3:                                # =>This Inner Loop Header: Depth=1
	movq	-16(%r12), %rbx
	movq	-8(%r12), %rsi
	movq	(%r12), %rax
	cmpl	$5, %esi
	jle	.LBB0_4
# %bb.6:                                #   in Loop: Header=BB0_3 Depth=1
	leal	-6(%rsi), %edx
	cmpl	$2, %edx
	jb	.LBB0_13
# %bb.7:                                #   in Loop: Header=BB0_3 Depth=1
	cmpl	$37, %esi
	je	.LBB0_10
# %bb.8:                                #   in Loop: Header=BB0_3 Depth=1
	cmpl	$8, %esi
	jne	.LBB0_12
# %bb.9:                                #   in Loop: Header=BB0_3 Depth=1
	addq	%r13, %rax
	jmp	.LBB0_14
	.p2align	4
.LBB0_4:                                #   in Loop: Header=BB0_3 Depth=1
	testl	%esi, %esi
	je	.LBB0_5
# %bb.11:                               #   in Loop: Header=BB0_3 Depth=1
	cmpl	$1, %esi
	jne	.LBB0_12
.LBB0_13:                               #   in Loop: Header=BB0_3 Depth=1
	shrq	$32, %rsi
	leaq	(%rsi,%rsi,2), %rdx
	addq	%r13, %rax
	addq	8(%rcx,%rdx,8), %rax
	jmp	.LBB0_14
.LBB0_12:                               #   in Loop: Header=BB0_3 Depth=1
	movl	$1, %eax
	movq	%r15, -48(%rbp)                 # 8-byte Spill
	jmp	.LBB0_15
.LBB0_5:                                #   in Loop: Header=BB0_3 Depth=1
	movl	$4, %eax
	jmp	.LBB0_15
.LBB0_10:                               #   in Loop: Header=BB0_3 Depth=1
	addq	%r13, %rax
	callq	*%rax
	movq	-56(%rbp), %rcx                 # 8-byte Reload
	.p2align	4
.LBB0_14:                               #   in Loop: Header=BB0_3 Depth=1
	movq	%rax, (%rbx,%r13)
	incq	%r15
	xorl	%eax, %eax
.LBB0_15:                               #   in Loop: Header=BB0_3 Depth=1
	testb	$3, %al
	jne	.LBB0_16
# %bb.17:                               #   in Loop: Header=BB0_3 Depth=1
	addq	$24, %r12
	decq	%r14
	jne	.LBB0_3
	jmp	.LBB0_18
.LBB0_1:
	xorl	%r15d, %r15d
	jmp	.LBB0_18
.LBB0_16:
	movq	-48(%rbp), %r15                 # 8-byte Reload
.LBB0_18:
	movq	%r15, %rax
	addq	$24, %rsp
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
