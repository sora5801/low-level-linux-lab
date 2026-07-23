	.file	"demo.c"
	.text
	.globl	ht_hash                         # -- Begin function ht_hash
	.p2align	4
	.type	ht_hash,@function
ht_hash:                                # @ht_hash
# %bb.0:
	imull	$1640531527, %edi, %eax         # imm = 0x61C88647
	retq
.Lfunc_end0:
	.size	ht_hash, .Lfunc_end0-ht_hash
                                        # -- End function
	.globl	ht_bucket_highbits              # -- Begin function ht_bucket_highbits
	.p2align	4
	.type	ht_bucket_highbits,@function
ht_bucket_highbits:                     # @ht_bucket_highbits
# %bb.0:
	imull	$1640531527, %edi, %eax         # imm = 0x61C88647
	shrl	$24, %eax
	retq
.Lfunc_end1:
	.size	ht_bucket_highbits, .Lfunc_end1-ht_bucket_highbits
                                        # -- End function
	.globl	ht_bucket_mask                  # -- Begin function ht_bucket_mask
	.p2align	4
	.type	ht_bucket_mask,@function
ht_bucket_mask:                         # @ht_bucket_mask
# %bb.0:
	imull	$71, %edi, %eax
	movzbl	%al, %eax
	retq
.Lfunc_end2:
	.size	ht_bucket_mask, .Lfunc_end2-ht_bucket_mask
                                        # -- End function
	.globl	ht_bucket                       # -- Begin function ht_bucket
	.p2align	4
	.type	ht_bucket,@function
ht_bucket:                              # @ht_bucket
# %bb.0:
	imull	$1640531527, %edi, %eax         # imm = 0x61C88647
	movl	%eax, %ecx
	shrl	$16, %ecx
	xorl	%eax, %ecx
	movzbl	%cl, %eax
	retq
.Lfunc_end3:
	.size	ht_bucket, .Lfunc_end3-ht_bucket
                                        # -- End function
	.globl	main                            # -- Begin function main
	.p2align	4
	.type	main,@function
main:                                   # @main
# %bb.0:
	movl	$164, %eax
	retq
.Lfunc_end4:
	.size	main, .Lfunc_end4-main
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
