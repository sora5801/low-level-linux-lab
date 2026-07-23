	.file	"demo.c"
	.text
	.globl	lat_bucket                      # -- Begin function lat_bucket
	.p2align	4
	.type	lat_bucket,@function
lat_bucket:                             # @lat_bucket
# %bb.0:
	bsrq	%rdi, %rcx
	incl	%ecx
	xorl	%eax, %eax
	testq	%rdi, %rdi
	cmovnel	%ecx, %eax
	retq
.Lfunc_end0:
	.size	lat_bucket, .Lfunc_end0-lat_bucket
                                        # -- End function
	.globl	bucket_low                      # -- Begin function bucket_low
	.p2align	4
	.type	bucket_low,@function
bucket_low:                             # @bucket_low
# %bb.0:
                                        # kill: def $edi killed $edi def $rdi
	leal	-1(%rdi), %ecx
	movl	$1, %edx
                                        # kill: def $cl killed $cl killed $ecx
	shlq	%cl, %rdx
	xorl	%eax, %eax
	cmpl	$1, %edi
	cmovaeq	%rdx, %rax
	retq
.Lfunc_end1:
	.size	bucket_low, .Lfunc_end1-bucket_low
                                        # -- End function
	.globl	bucket_high                     # -- Begin function bucket_high
	.p2align	4
	.type	bucket_high,@function
bucket_high:                            # @bucket_high
# %bb.0:
	movl	%edi, %ecx
	movq	$-1, %rax
                                        # kill: def $cl killed $cl killed $ecx
	shlq	%cl, %rax
	notq	%rax
	retq
.Lfunc_end2:
	.size	bucket_high, .Lfunc_end2-bucket_high
                                        # -- End function
	.globl	hist_record                     # -- Begin function hist_record
	.p2align	4
	.type	hist_record,@function
hist_record:                            # @hist_record
# %bb.0:
	bsrq	%rsi, %rax
	incl	%eax
	xorl	%ecx, %ecx
	testq	%rsi, %rsi
	cmovnel	%eax, %ecx
	incq	(%rdi,%rcx,8)
	retq
.Lfunc_end3:
	.size	hist_record, .Lfunc_end3-hist_record
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
