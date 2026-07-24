	.file	"demo.c"
	.text
	.globl	my_strcpy                       # -- Begin function my_strcpy
	.p2align	4
	.type	my_strcpy,@function
my_strcpy:                              # @my_strcpy
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	-8(%rbp), %rax
	movq	%rax, -24(%rbp)
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	movq	-16(%rbp), %rax
	movq	%rax, %rcx
	addq	$1, %rcx
	movq	%rcx, -16(%rbp)
	movb	(%rax), %al
	movq	-24(%rbp), %rcx
	movq	%rcx, %rdx
	addq	$1, %rdx
	movq	%rdx, -24(%rbp)
	movb	%al, (%rcx)
	movsbl	%al, %eax
	cmpl	$0, %eax
	je	.LBB0_3
# %bb.2:                                #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_1
.LBB0_3:
	movq	-8(%rbp), %rax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	my_strcpy, .Lfunc_end0-my_strcpy
                                        # -- End function
	.globl	vulnerable                      # -- Begin function vulnerable
	.p2align	4
	.type	vulnerable,@function
vulnerable:                             # @vulnerable
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$80, %rsp
	movq	%rdi, -8(%rbp)
	leaq	-80(%rbp), %rdi
	movq	-8(%rbp), %rsi
	callq	my_strcpy
	leaq	-80(%rbp), %rdi
	callq	consume@PLT
	xorl	%eax, %eax
	addq	$80, %rsp
	popq	%rbp
	retq
.Lfunc_end1:
	.size	vulnerable, .Lfunc_end1-vulnerable
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym my_strcpy
	.addrsig_sym consume
