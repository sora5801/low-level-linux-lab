	.file	"demo.c"
	.text
	.globl	clamp_count                     # -- Begin function clamp_count
	.p2align	4
	.type	clamp_count,@function
clamp_count:                            # @clamp_count
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	%edi, -8(%rbp)
	cmpl	$1, -8(%rbp)
	jge	.LBB0_2
# %bb.1:
	movl	$1, -4(%rbp)
	jmp	.LBB0_5
.LBB0_2:
	cmpl	$10, -8(%rbp)
	jle	.LBB0_4
# %bb.3:
	movl	$10, -4(%rbp)
	jmp	.LBB0_5
.LBB0_4:
	movl	-8(%rbp), %eax
	movl	%eax, -4(%rbp)
.LBB0_5:
	movl	-4(%rbp), %eax
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
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movl	$-2128831035, -12(%rbp)         # imm = 0x811C9DC5
.LBB1_1:                                # =>This Inner Loop Header: Depth=1
	movq	-8(%rbp), %rax
	cmpb	$0, (%rax)
	je	.LBB1_3
# %bb.2:                                #   in Loop: Header=BB1_1 Depth=1
	movq	-8(%rbp), %rax
	movq	%rax, %rcx
	addq	$1, %rcx
	movq	%rcx, -8(%rbp)
	movzbl	(%rax), %eax
	xorl	-12(%rbp), %eax
	movl	%eax, -12(%rbp)
	imull	$16777619, -12(%rbp), %eax      # imm = 0x1000193
	movl	%eax, -12(%rbp)
	jmp	.LBB1_1
.LBB1_3:
	movl	-12(%rbp), %eax
	popq	%rbp
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
	subq	$16, %rsp
	movl	$0, -4(%rbp)
	movl	$5, %edi
	callq	clamp_count
	movl	%eax, -8(%rbp)
	leaq	.L.str(%rip), %rdi
	callq	fnv1a32
	movl	%eax, -12(%rbp)
	movl	-12(%rbp), %eax
	xorl	-8(%rbp), %eax
	andl	$127, %eax
	addq	$16, %rsp
	popq	%rbp
	retq
.Lfunc_end2:
	.size	main, .Lfunc_end2-main
                                        # -- End function
	.type	.L.str,@object                  # @.str
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str:
	.asciz	"world"
	.size	.L.str, 6

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym clamp_count
	.addrsig_sym fnv1a32
