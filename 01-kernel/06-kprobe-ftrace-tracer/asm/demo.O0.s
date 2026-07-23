	.file	"demo.c"
	.text
	.globl	lat_bucket                      # -- Begin function lat_bucket
	.p2align	4
	.type	lat_bucket,@function
lat_bucket:                             # @lat_bucket
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movl	$0, -12(%rbp)
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	cmpq	$0, -8(%rbp)
	je	.LBB0_3
# %bb.2:                                #   in Loop: Header=BB0_1 Depth=1
	movl	-12(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -12(%rbp)
	movq	-8(%rbp), %rax
	shrq	%rax
	movq	%rax, -8(%rbp)
	jmp	.LBB0_1
.LBB0_3:
	movl	-12(%rbp), %eax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	lat_bucket, .Lfunc_end0-lat_bucket
                                        # -- End function
	.globl	bucket_low                      # -- Begin function bucket_low
	.p2align	4
	.type	bucket_low,@function
bucket_low:                             # @bucket_low
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	%edi, -12(%rbp)
	cmpl	$0, -12(%rbp)
	jne	.LBB1_2
# %bb.1:
	movq	$0, -8(%rbp)
	jmp	.LBB1_3
.LBB1_2:
	movl	-12(%rbp), %eax
	subl	$1, %eax
	movl	%eax, %eax
	movl	%eax, %ecx
	movl	$1, %eax
                                        # kill: def $cl killed $rcx
	shlq	%cl, %rax
	movq	%rax, -8(%rbp)
.LBB1_3:
	movq	-8(%rbp), %rax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	bucket_low, .Lfunc_end1-bucket_low
                                        # -- End function
	.globl	bucket_high                     # -- Begin function bucket_high
	.p2align	4
	.type	bucket_high,@function
bucket_high:                            # @bucket_high
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	%edi, -12(%rbp)
	cmpl	$0, -12(%rbp)
	jne	.LBB2_2
# %bb.1:
	movq	$0, -8(%rbp)
	jmp	.LBB2_3
.LBB2_2:
	movl	-12(%rbp), %eax
	movl	%eax, %ecx
	movl	$1, %eax
                                        # kill: def $cl killed $rcx
	shlq	%cl, %rax
	subq	$1, %rax
	movq	%rax, -8(%rbp)
.LBB2_3:
	movq	-8(%rbp), %rax
	popq	%rbp
	retq
.Lfunc_end2:
	.size	bucket_high, .Lfunc_end2-bucket_high
                                        # -- End function
	.globl	hist_record                     # -- Begin function hist_record
	.p2align	4
	.type	hist_record,@function
hist_record:                            # @hist_record
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$32, %rsp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	-16(%rbp), %rdi
	callq	lat_bucket
	movl	%eax, -20(%rbp)
	cmpl	$65, -20(%rbp)
	jb	.LBB3_2
# %bb.1:
	movl	$64, -20(%rbp)
.LBB3_2:
	movq	-8(%rbp), %rax
	movl	-20(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movq	(%rax,%rcx,8), %rdx
	addq	$1, %rdx
	movq	%rdx, (%rax,%rcx,8)
	addq	$32, %rsp
	popq	%rbp
	retq
.Lfunc_end3:
	.size	hist_record, .Lfunc_end3-hist_record
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym lat_bucket
