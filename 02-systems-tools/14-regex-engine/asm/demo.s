	.file	"demo.c"
	.text
	.globl	nfa_step                        # -- Begin function nfa_step
	.p2align	4
	.type	nfa_step,@function
nfa_step:                               # @nfa_step
# %bb.0:
	testl	%r9d, %r9d
	jle	.LBB0_10
# %bb.1:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	movq	%rcx, %rax
	movl	16(%rbp), %ecx
	movl	$1, %r10d
	shlq	%cl, %r10
	movl	%ecx, %r11d
	sarl	$6, %r11d
	movl	%r9d, %r9d
	xorl	%ebx, %ebx
	jmp	.LBB0_2
	.p2align	4
.LBB0_8:                                #   in Loop: Header=BB0_2 Depth=1
	incq	%rbx
	cmpq	%r9, %rbx
	je	.LBB0_9
.LBB0_2:                                # =>This Loop Header: Depth=1
                                        #     Child Loop BB0_4 Depth 2
	movq	(%rax,%rbx,8), %r14
	testq	%r14, %r14
	je	.LBB0_8
# %bb.3:                                #   in Loop: Header=BB0_2 Depth=1
	movl	%ebx, %r15d
	shll	$6, %r15d
	leal	1(%r15), %r12d
	jmp	.LBB0_4
	.p2align	4
.LBB0_7:                                #   in Loop: Header=BB0_4 Depth=2
	leaq	-1(%r14), %rcx
	andq	%rcx, %r14
	je	.LBB0_8
.LBB0_4:                                #   Parent Loop BB0_2 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	rep		bsfq	%r14, %rcx
	movl	%r15d, %r13d
	orl	%ecx, %r13d
	movslq	%r13d, %r13
	cmpb	$0, (%rdi,%r13)
	jne	.LBB0_7
# %bb.5:                                #   in Loop: Header=BB0_4 Depth=2
	movl	(%rsi,%r13,4), %r13d
	leal	(%r11,%r13,4), %r13d
	movslq	%r13d, %r13
	testq	%r10, (%rdx,%r13,8)
	je	.LBB0_7
# %bb.6:                                #   in Loop: Header=BB0_4 Depth=2
	addl	%r12d, %ecx
	movl	$1, %r13d
	shlq	%cl, %r13
	sarl	$6, %ecx
	movslq	%ecx, %rcx
	orq	%r13, (%r8,%rcx,8)
	jmp	.LBB0_7
.LBB0_9:
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
.LBB0_10:
	retq
.Lfunc_end0:
	.size	nfa_step, .Lfunc_end0-nfa_step
                                        # -- End function
	.globl	demo_run                        # -- Begin function demo_run
	.p2align	4
	.type	demo_run,@function
demo_run:                               # @demo_run
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	xorps	%xmm0, %xmm0
	movaps	%xmm0, g_sets(%rip)
	movaps	%xmm0, g_sets+32(%rip)
	movaps	%xmm0, g_sets+48(%rip)
	movaps	%xmm0, g_sets+16(%rip)
	orb	$2, g_sets+12(%rip)
	orb	$12, g_sets+44(%rip)
	movl	$2, %eax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	demo_run, .Lfunc_end1-demo_run
                                        # -- End function
	.type	g_sets,@object                  # @g_sets
	.local	g_sets
	.comm	g_sets,64,16
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
