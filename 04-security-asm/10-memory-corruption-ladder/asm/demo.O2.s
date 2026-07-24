	.file	"demo.c"
	.text
	.globl	my_strcpy                       # -- Begin function my_strcpy
	.p2align	4
	.type	my_strcpy,@function
my_strcpy:                              # @my_strcpy
# %bb.0:
	movq	%rdi, %rax
	xorl	%ecx, %ecx
	.p2align	4
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	movzbl	(%rsi,%rcx), %edx
	movb	%dl, (%rax,%rcx)
	incq	%rcx
	testb	%dl, %dl
	jne	.LBB0_1
# %bb.2:
	retq
.Lfunc_end0:
	.size	my_strcpy, .Lfunc_end0-my_strcpy
                                        # -- End function
	.globl	vulnerable                      # -- Begin function vulnerable
	.p2align	4
	.type	vulnerable,@function
vulnerable:                             # @vulnerable
# %bb.0:
	pushq	%rbx
	subq	$64, %rsp
	movq	%rdi, %rsi
	movq	%rsp, %rbx
	movq	%rbx, %rdi
	callq	my_strcpy
	movq	%rbx, %rdi
	callq	consume@PLT
	xorl	%eax, %eax
	addq	$64, %rsp
	popq	%rbx
	retq
.Lfunc_end1:
	.size	vulnerable, .Lfunc_end1-vulnerable
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
