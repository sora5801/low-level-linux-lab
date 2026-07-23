	.file	"demo.c"
	.text
	.globl	ip_checksum                     # -- Begin function ip_checksum
	.p2align	4
	.type	ip_checksum,@function
ip_checksum:                            # @ip_checksum
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$32, %rsp
	movq	%rdi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movq	-8(%rbp), %rax
	movq	%rax, -24(%rbp)
	movl	$0, -28(%rbp)
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	cmpl	$1, -12(%rbp)
	jbe	.LBB0_3
# %bb.2:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-24(%rbp), %rax
	movzbl	(%rax), %eax
	movq	-24(%rbp), %rcx
	movzbl	1(%rcx), %ecx
	shll	$8, %ecx
	orl	%ecx, %eax
                                        # kill: def $ax killed $ax killed $eax
	movw	%ax, -30(%rbp)
	movzwl	-30(%rbp), %eax
	addl	-28(%rbp), %eax
	movl	%eax, -28(%rbp)
	movq	-24(%rbp), %rax
	addq	$2, %rax
	movq	%rax, -24(%rbp)
	movl	-12(%rbp), %eax
	subl	$2, %eax
	movl	%eax, -12(%rbp)
	jmp	.LBB0_1
.LBB0_3:
	cmpl	$1, -12(%rbp)
	jne	.LBB0_5
# %bb.4:
	movq	-24(%rbp), %rax
	movzbl	(%rax), %eax
	addl	-28(%rbp), %eax
	movl	%eax, -28(%rbp)
.LBB0_5:
	movl	-28(%rbp), %edi
	callq	csum_fold
	movzwl	%ax, %eax
	xorl	$-1, %eax
                                        # kill: def $ax killed $ax killed $eax
	addq	$32, %rsp
	popq	%rbp
	retq
.Lfunc_end0:
	.size	ip_checksum, .Lfunc_end0-ip_checksum
                                        # -- End function
	.p2align	4                               # -- Begin function csum_fold
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
                                        # kill: def $ax killed $ax killed $eax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	csum_fold, .Lfunc_end1-csum_fold
                                        # -- End function
	.globl	ip_checksum_valid               # -- Begin function ip_checksum_valid
	.p2align	4
	.type	ip_checksum_valid,@function
ip_checksum_valid:                      # @ip_checksum_valid
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movq	%rdi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movq	-8(%rbp), %rdi
	movl	-12(%rbp), %esi
	callq	ip_checksum
	movzwl	%ax, %eax
	cmpl	$0, %eax
	sete	%al
	andb	$1, %al
	movzbl	%al, %eax
	addq	$16, %rsp
	popq	%rbp
	retq
.Lfunc_end2:
	.size	ip_checksum_valid, .Lfunc_end2-ip_checksum_valid
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym ip_checksum
	.addrsig_sym csum_fold
