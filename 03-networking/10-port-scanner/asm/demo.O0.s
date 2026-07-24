	.file	"demo.c"
	.text
	.globl	sum16                           # -- Begin function sum16
	.p2align	4
	.type	sum16,@function
sum16:                                  # @sum16
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movl	%edx, -16(%rbp)
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	cmpl	$1, -12(%rbp)
	jbe	.LBB0_3
# %bb.2:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-8(%rbp), %rax
	movzbl	(%rax), %eax
	shll	$8, %eax
	movq	-8(%rbp), %rcx
	movzbl	1(%rcx), %ecx
	orl	%ecx, %eax
	addl	-16(%rbp), %eax
	movl	%eax, -16(%rbp)
	movq	-8(%rbp), %rax
	addq	$2, %rax
	movq	%rax, -8(%rbp)
	movl	-12(%rbp), %eax
	subl	$2, %eax
	movl	%eax, -12(%rbp)
	jmp	.LBB0_1
.LBB0_3:
	cmpl	$1, -12(%rbp)
	jne	.LBB0_5
# %bb.4:
	movq	-8(%rbp), %rax
	movzbl	(%rax), %eax
	shll	$8, %eax
	addl	-16(%rbp), %eax
	movl	%eax, -16(%rbp)
.LBB0_5:
	movl	-16(%rbp), %eax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	sum16, .Lfunc_end0-sum16
                                        # -- End function
	.globl	fold_csum                       # -- Begin function fold_csum
	.p2align	4
	.type	fold_csum,@function
fold_csum:                              # @fold_csum
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	%edi, -4(%rbp)
.LBB1_1:                                # =>This Inner Loop Header: Depth=1
	movl	-4(%rbp), %eax
	shrl	$16, %eax
	cmpl	$0, %eax
	je	.LBB1_3
# %bb.2:                                #   in Loop: Header=BB1_1 Depth=1
	movl	-4(%rbp), %eax
	andl	$65535, %eax                    # imm = 0xFFFF
	movl	-4(%rbp), %ecx
	shrl	$16, %ecx
	addl	%ecx, %eax
	movl	%eax, -4(%rbp)
	jmp	.LBB1_1
.LBB1_3:
	movl	-4(%rbp), %eax
	xorl	$-1, %eax
                                        # kill: def $ax killed $ax killed $eax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	fold_csum, .Lfunc_end1-fold_csum
                                        # -- End function
	.globl	tcp_checksum                    # -- Begin function tcp_checksum
	.p2align	4
	.type	tcp_checksum,@function
tcp_checksum:                           # @tcp_checksum
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$32, %rsp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movl	%edx, -20(%rbp)
	movl	$0, -24(%rbp)
	movq	-8(%rbp), %rdi
	movl	-24(%rbp), %edx
	movl	$12, %esi
	callq	sum16
	movl	%eax, -24(%rbp)
	movq	-16(%rbp), %rdi
	movl	-20(%rbp), %esi
	movl	-24(%rbp), %edx
	callq	sum16
	movl	%eax, -24(%rbp)
	movl	-24(%rbp), %edi
	callq	fold_csum
	addq	$32, %rsp
	popq	%rbp
	retq
.Lfunc_end2:
	.size	tcp_checksum, .Lfunc_end2-tcp_checksum
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym sum16
	.addrsig_sym fold_csum
