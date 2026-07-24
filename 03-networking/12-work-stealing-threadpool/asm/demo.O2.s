	.file	"demo.c"
	.text
	.globl	dq_push                         # -- Begin function dq_push
	.p2align	4
	.type	dq_push,@function
dq_push:                                # @dq_push
# %bb.0:
	movq	8(%rdi), %rax
	movq	16(%rdi), %rcx
	movq	16(%rcx), %rdx
	movq	8(%rcx), %rcx
	andq	%rax, %rcx
	movq	%rsi, (%rdx,%rcx,8)
	#MEMBARRIER
	incq	%rax
	movq	%rax, 8(%rdi)
	retq
.Lfunc_end0:
	.size	dq_push, .Lfunc_end0-dq_push
                                        # -- End function
	.globl	dq_take                         # -- Begin function dq_take
	.p2align	4
	.type	dq_take,@function
dq_take:                                # @dq_take
# %bb.0:
	movq	8(%rdi), %rdx
	leaq	-1(%rdx), %rcx
	movq	16(%rdi), %rax
	movq	%rcx, 8(%rdi)
	mfence
	movq	(%rdi), %rsi
	cmpq	%rdx, %rsi
	jge	.LBB1_1
# %bb.2:
	movq	16(%rax), %r8
	movq	8(%rax), %rax
	andq	%rcx, %rax
	movq	(%r8,%rax,8), %rax
	cmpq	%rcx, %rsi
	jne	.LBB1_5
# %bb.3:
	xorl	%r8d, %r8d
	movq	%rax, %rsi
	movq	%rcx, %rax
	lock		cmpxchgq	%rdx, (%rdi)
	movq	%rsi, %rax
	cmovneq	%r8, %rax
	jmp	.LBB1_4
.LBB1_1:
	xorl	%eax, %eax
.LBB1_4:
	movq	%rdx, 8(%rdi)
.LBB1_5:
	retq
.Lfunc_end1:
	.size	dq_take, .Lfunc_end1-dq_take
                                        # -- End function
	.globl	dq_steal                        # -- Begin function dq_steal
	.p2align	4
	.type	dq_steal,@function
dq_steal:                               # @dq_steal
# %bb.0:
	movq	(%rdi), %rax
	mfence
	movq	8(%rdi), %rcx
	cmpq	%rcx, %rax
	jge	.LBB2_1
# %bb.2:
	movq	16(%rdi), %rcx
	movq	16(%rcx), %rdx
	movq	8(%rcx), %rcx
	andq	%rax, %rcx
	movq	(%rdx,%rcx,8), %rcx
	leaq	1(%rax), %rdx
	lock		cmpxchgq	%rdx, (%rdi)
	movq	$-1, %rax
	cmoveq	%rcx, %rax
	retq
.LBB2_1:
	xorl	%eax, %eax
	retq
.Lfunc_end2:
	.size	dq_steal, .Lfunc_end2-dq_steal
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
