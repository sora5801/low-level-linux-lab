	.file	"demo.c"
	.text
	.globl	demo_push                       # -- Begin function demo_push
	.p2align	4
	.type	demo_push,@function
demo_push:                              # @demo_push
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movabsq	$281474976710655, %rcx          # imm = 0xFFFFFFFFFFFF
	movq	(%rdi), %rax
	movq	%rsi, %rdx
	andq	%rcx, %rdx
	addq	%rcx, %rdx
	incq	%rdx
	movabsq	$-281474976710656, %r8          # imm = 0xFFFF000000000000
	.p2align	4
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	movq	%rax, %r9
	movq	%rax, %r10
	andq	%rcx, %r10
	movq	%r10, (%rsi)
	andq	%r8, %r9
	addq	%rdx, %r9
	lock		cmpxchgq	%r9, (%rdi)
	jne	.LBB0_1
# %bb.2:
	popq	%rbp
	retq
.Lfunc_end0:
	.size	demo_push, .Lfunc_end0-demo_push
                                        # -- End function
	.globl	demo_pop                        # -- Begin function demo_pop
	.p2align	4
	.type	demo_pop,@function
demo_pop:                               # @demo_pop
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movabsq	$281474976710655, %rdx          # imm = 0xFFFFFFFFFFFF
	movq	(%rdi), %rax
	xorl	%ecx, %ecx
	movabsq	$-281474976710656, %rsi         # imm = 0xFFFF000000000000
	.p2align	4
.LBB1_1:                                # =>This Inner Loop Header: Depth=1
	movq	%rax, %r8
	andq	%rdx, %r8
	je	.LBB1_4
# %bb.2:                                #   in Loop: Header=BB1_1 Depth=1
	movq	%rax, %r9
	andq	%rsi, %r9
	orq	%rdx, %r9
	movq	(%r8), %r10
	andq	%rdx, %r10
	addq	%r10, %r9
	incq	%r9
	lock		cmpxchgq	%r9, (%rdi)
	jne	.LBB1_1
# %bb.3:
	movq	%r8, %rcx
.LBB1_4:
	movq	%rcx, %rax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	demo_pop, .Lfunc_end1-demo_pop
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
