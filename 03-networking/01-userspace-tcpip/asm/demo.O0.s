	.file	"demo.c"
	.text
	.globl	csum_accumulate                 # -- Begin function csum_accumulate
	.p2align	4
	.type	csum_accumulate,@function
csum_accumulate:                        # @csum_accumulate
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movl	%edx, -20(%rbp)
	movq	-8(%rbp), %rax
	movq	%rax, -32(%rbp)
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	cmpq	$2, -16(%rbp)
	jb	.LBB0_3
# %bb.2:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-32(%rbp), %rax
	movzbl	(%rax), %eax
	shll	$8, %eax
	movq	-32(%rbp), %rcx
	movzbl	1(%rcx), %ecx
	orl	%ecx, %eax
	addl	-20(%rbp), %eax
	movl	%eax, -20(%rbp)
	movq	-32(%rbp), %rax
	addq	$2, %rax
	movq	%rax, -32(%rbp)
	movq	-16(%rbp), %rax
	subq	$2, %rax
	movq	%rax, -16(%rbp)
	jmp	.LBB0_1
.LBB0_3:
	cmpq	$1, -16(%rbp)
	jne	.LBB0_5
# %bb.4:
	movq	-32(%rbp), %rax
	movzbl	(%rax), %eax
	shll	$8, %eax
	addl	-20(%rbp), %eax
	movl	%eax, -20(%rbp)
.LBB0_5:
	movl	-20(%rbp), %eax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	csum_accumulate, .Lfunc_end0-csum_accumulate
                                        # -- End function
	.globl	csum_fold                       # -- Begin function csum_fold
	.p2align	4
	.type	csum_fold,@function
csum_fold:                              # @csum_fold
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
	andl	$65535, %eax                    # imm = 0xFFFF
                                        # kill: def $ax killed $ax killed $eax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	csum_fold, .Lfunc_end1-csum_fold
                                        # -- End function
	.globl	inet_checksum                   # -- Begin function inet_checksum
	.p2align	4
	.type	inet_checksum,@function
inet_checksum:                          # @inet_checksum
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	-8(%rbp), %rdi
	movq	-16(%rbp), %rsi
	xorl	%edx, %edx
	callq	csum_accumulate
	movl	%eax, %edi
	callq	csum_fold
	addq	$16, %rsp
	popq	%rbp
	retq
.Lfunc_end2:
	.size	inet_checksum, .Lfunc_end2-inet_checksum
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym csum_accumulate
	.addrsig_sym csum_fold
