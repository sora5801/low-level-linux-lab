	.file	"demo.c"
	.text
	.globl	ct_eq                           # -- Begin function ct_eq
	.p2align	4
	.type	ct_eq,@function
ct_eq:                                  # @ct_eq
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	%edi, -4(%rbp)
	movl	%esi, -8(%rbp)
	movl	-4(%rbp), %eax
	xorl	-8(%rbp), %eax
	movl	%eax, -12(%rbp)
	movl	-12(%rbp), %eax
	xorl	%ecx, %ecx
	subl	-12(%rbp), %ecx
	orl	%ecx, %eax
	shrl	$31, %eax
	movl	%eax, -16(%rbp)
	movl	-16(%rbp), %eax
	subl	$1, %eax
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
	movl	%edi, -4(%rbp)
	movl	%esi, -8(%rbp)
	movl	%edx, -12(%rbp)
	movl	-8(%rbp), %eax
	andl	-4(%rbp), %eax
	movl	-12(%rbp), %ecx
	movl	-4(%rbp), %edx
	xorl	$-1, %edx
	andl	%edx, %ecx
	orl	%ecx, %eax
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
	subq	$48, %rsp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	%rdx, -24(%rbp)
	movb	$0, -25(%rbp)
	movq	$0, -40(%rbp)
.LBB2_1:                                # =>This Inner Loop Header: Depth=1
	movq	-40(%rbp), %rax
	cmpq	-24(%rbp), %rax
	jae	.LBB2_4
# %bb.2:                                #   in Loop: Header=BB2_1 Depth=1
	movq	-8(%rbp), %rax
	movq	-40(%rbp), %rcx
	movzbl	(%rax,%rcx), %eax
	movq	-16(%rbp), %rcx
	movq	-40(%rbp), %rdx
	movzbl	(%rcx,%rdx), %ecx
	xorl	%ecx, %eax
                                        # kill: def $al killed $al killed $eax
	movzbl	%al, %ecx
	movzbl	-25(%rbp), %eax
	orl	%ecx, %eax
                                        # kill: def $al killed $al killed $eax
	movb	%al, -25(%rbp)
# %bb.3:                                #   in Loop: Header=BB2_1 Depth=1
	movq	-40(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -40(%rbp)
	jmp	.LBB2_1
.LBB2_4:
	movzbl	-25(%rbp), %edi
	xorl	%esi, %esi
	callq	ct_eq
	addq	$48, %rsp
	popq	%rbp
	retq
.Lfunc_end2:
	.size	ct_memeq, .Lfunc_end2-ct_memeq
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym ct_eq
