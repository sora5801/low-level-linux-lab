	.file	"demo.c"
	.text
	.globl	ht_hash                         # -- Begin function ht_hash
	.p2align	4
	.type	ht_hash,@function
ht_hash:                                # @ht_hash
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	%edi, -4(%rbp)
	imull	$1640531527, -4(%rbp), %eax     # imm = 0x61C88647
	popq	%rbp
	retq
.Lfunc_end0:
	.size	ht_hash, .Lfunc_end0-ht_hash
                                        # -- End function
	.globl	ht_bucket_highbits              # -- Begin function ht_bucket_highbits
	.p2align	4
	.type	ht_bucket_highbits,@function
ht_bucket_highbits:                     # @ht_bucket_highbits
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movl	%edi, -4(%rbp)
	movl	-4(%rbp), %edi
	callq	ht_hash
	shrl	$24, %eax
	addq	$16, %rsp
	popq	%rbp
	retq
.Lfunc_end1:
	.size	ht_bucket_highbits, .Lfunc_end1-ht_bucket_highbits
                                        # -- End function
	.globl	ht_bucket_mask                  # -- Begin function ht_bucket_mask
	.p2align	4
	.type	ht_bucket_mask,@function
ht_bucket_mask:                         # @ht_bucket_mask
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movl	%edi, -4(%rbp)
	movl	-4(%rbp), %edi
	callq	ht_hash
	andl	$255, %eax
	addq	$16, %rsp
	popq	%rbp
	retq
.Lfunc_end2:
	.size	ht_bucket_mask, .Lfunc_end2-ht_bucket_mask
                                        # -- End function
	.globl	ht_bucket                       # -- Begin function ht_bucket
	.p2align	4
	.type	ht_bucket,@function
ht_bucket:                              # @ht_bucket
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	%edi, -4(%rbp)
	imull	$1640531527, -4(%rbp), %eax     # imm = 0x61C88647
	movl	%eax, -8(%rbp)
	movl	-8(%rbp), %eax
	shrl	$16, %eax
	xorl	-8(%rbp), %eax
	movl	%eax, -8(%rbp)
	movl	-8(%rbp), %eax
	andl	$255, %eax
	popq	%rbp
	retq
.Lfunc_end3:
	.size	ht_bucket, .Lfunc_end3-ht_bucket
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
	movl	$0, -8(%rbp)
	movl	$0, -12(%rbp)
.LBB4_1:                                # =>This Inner Loop Header: Depth=1
	cmpl	$8, -12(%rbp)
	jae	.LBB4_4
# %bb.2:                                #   in Loop: Header=BB4_1 Depth=1
	movl	-12(%rbp), %edi
	callq	ht_bucket
	addl	-8(%rbp), %eax
	movl	%eax, -8(%rbp)
# %bb.3:                                #   in Loop: Header=BB4_1 Depth=1
	movl	-12(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -12(%rbp)
	jmp	.LBB4_1
.LBB4_4:
	movl	-8(%rbp), %eax
	andl	$255, %eax
	addq	$16, %rsp
	popq	%rbp
	retq
.Lfunc_end4:
	.size	main, .Lfunc_end4-main
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym ht_hash
	.addrsig_sym ht_bucket
