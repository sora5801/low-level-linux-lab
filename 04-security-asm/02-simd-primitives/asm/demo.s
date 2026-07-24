	.file	"demo.c"
	.text
	.globl	scalar_strlen                   # -- Begin function scalar_strlen
	.p2align	4
	.type	scalar_strlen,@function
scalar_strlen:                          # @scalar_strlen
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	$-1, %rax
	.p2align	4
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	cmpb	$0, 1(%rdi,%rax)
	leaq	1(%rax), %rax
	jne	.LBB0_1
# %bb.2:
	popq	%rbp
	retq
.Lfunc_end0:
	.size	scalar_strlen, .Lfunc_end0-scalar_strlen
                                        # -- End function
	.globl	count_nonzero_bounded           # -- Begin function count_nonzero_bounded
	.p2align	4
	.type	count_nonzero_bounded,@function
count_nonzero_bounded:                  # @count_nonzero_bounded
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	testq	%rsi, %rsi
	je	.LBB1_1
# %bb.3:
	xorl	%ecx, %ecx
	xorl	%eax, %eax
	.p2align	4
.LBB1_4:                                # =>This Inner Loop Header: Depth=1
	cmpb	$1, (%rdi,%rcx)
	sbbq	$-1, %rax
	incq	%rcx
	cmpq	%rcx, %rsi
	jne	.LBB1_4
# %bb.2:
	popq	%rbp
	retq
.LBB1_1:
	xorl	%eax, %eax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	count_nonzero_bounded, .Lfunc_end1-count_nonzero_bounded
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
