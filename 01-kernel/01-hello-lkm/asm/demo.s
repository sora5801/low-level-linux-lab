	.file	"demo.c"
	.text
	.globl	clamp_count                     # -- Begin function clamp_count
	.p2align	4
	.type	clamp_count,@function
clamp_count:                            # @clamp_count
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	cmpl	$10, %edi
	movl	$10, %ecx
	cmovll	%edi, %ecx
	cmpl	$2, %ecx
	movl	$1, %eax
	cmovgel	%ecx, %eax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	clamp_count, .Lfunc_end0-clamp_count
                                        # -- End function
	.globl	fnv1a32                         # -- Begin function fnv1a32
	.p2align	4
	.type	fnv1a32,@function
fnv1a32:                                # @fnv1a32
# %bb.0:
	movzbl	(%rdi), %ecx
	testb	%cl, %cl
	je	.LBB1_1
# %bb.3:
	pushq	%rbp
	movq	%rsp, %rbp
	incq	%rdi
	movl	$-2128831035, %eax              # imm = 0x811C9DC5
	.p2align	4
.LBB1_4:                                # =>This Inner Loop Header: Depth=1
	movzbl	%cl, %ecx
	xorl	%eax, %ecx
	imull	$16777619, %ecx, %eax           # imm = 0x1000193
	movzbl	(%rdi), %ecx
	incq	%rdi
	testb	%cl, %cl
	jne	.LBB1_4
# %bb.5:
	popq	%rbp
	retq
.LBB1_1:
	movl	$-2128831035, %eax              # imm = 0x811C9DC5
	retq
.Lfunc_end1:
	.size	fnv1a32, .Lfunc_end1-fnv1a32
                                        # -- End function
	.globl	main                            # -- Begin function main
	.p2align	4
	.type	main,@function
main:                                   # @main
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	$22, %eax
	popq	%rbp
	retq
.Lfunc_end2:
	.size	main, .Lfunc_end2-main
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
