	.file	"demo.c"
	.text
	.globl	ct_eq                           # -- Begin function ct_eq
	.p2align	4
	.type	ct_eq,@function
ct_eq:                                  # @ct_eq
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	xorl	%eax, %eax
	cmpl	%esi, %edi
	sete	%al
	negl	%eax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	ct_eq, .Lfunc_end0-ct_eq
                                        # -- End function
	.globl	ct_select                       # -- Begin function ct_select
	.p2align	4
	.type	ct_select,@function
ct_select:                              # @ct_select
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	%esi, %eax
	xorl	%edx, %eax
	andl	%edi, %eax
	xorl	%edx, %eax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	ct_select, .Lfunc_end1-ct_select
                                        # -- End function
	.globl	ct_memeq                        # -- Begin function ct_memeq
	.p2align	4
	.type	ct_memeq,@function
ct_memeq:                               # @ct_memeq
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	testq	%rdx, %rdx
	je	.LBB2_1
# %bb.4:
	xorl	%eax, %eax
	xorl	%ecx, %ecx
	.p2align	4
.LBB2_5:                                # =>This Inner Loop Header: Depth=1
	movzbl	(%rsi,%rax), %r8d
	xorb	(%rdi,%rax), %r8b
	orb	%r8b, %cl
	incq	%rax
	cmpq	%rax, %rdx
	jne	.LBB2_5
# %bb.2:
	xorl	%eax, %eax
	cmpb	$1, %cl
	sbbl	%eax, %eax
	popq	%rbp
	retq
.LBB2_1:
	movl	$-1, %eax
	popq	%rbp
	retq
.Lfunc_end2:
	.size	ct_memeq, .Lfunc_end2-ct_memeq
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
