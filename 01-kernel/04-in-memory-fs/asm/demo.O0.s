	.file	"demo.c"
	.text
	.globl	tinyfs_name_hash                # -- Begin function tinyfs_name_hash
	.p2align	4
	.type	tinyfs_name_hash,@function
tinyfs_name_hash:                       # @tinyfs_name_hash
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$32, %rsp
	movq	%rdi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movq	$0, -24(%rbp)
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	movl	-12(%rbp), %eax
	movl	%eax, %ecx
	addl	$-1, %ecx
	movl	%ecx, -12(%rbp)
	cmpl	$0, %eax
	je	.LBB0_3
# %bb.2:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-8(%rbp), %rax
	movq	%rax, %rcx
	addq	$1, %rcx
	movq	%rcx, -8(%rbp)
	movzbl	(%rax), %eax
	movl	%eax, %edi
	movq	-24(%rbp), %rsi
	callq	partial_name_hash
	movq	%rax, -24(%rbp)
	jmp	.LBB0_1
.LBB0_3:
	movq	-24(%rbp), %rdi
	callq	end_name_hash
	addq	$32, %rsp
	popq	%rbp
	retq
.Lfunc_end0:
	.size	tinyfs_name_hash, .Lfunc_end0-tinyfs_name_hash
                                        # -- End function
	.p2align	4                               # -- Begin function partial_name_hash
	.type	partial_name_hash,@function
partial_name_hash:                      # @partial_name_hash
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	-16(%rbp), %rax
	movq	-8(%rbp), %rcx
	shlq	$4, %rcx
	addq	%rcx, %rax
	movq	-8(%rbp), %rcx
	shrq	$4, %rcx
	addq	%rcx, %rax
	imulq	$11, %rax, %rax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	partial_name_hash, .Lfunc_end1-partial_name_hash
                                        # -- End function
	.p2align	4                               # -- Begin function end_name_hash
	.type	end_name_hash,@function
end_name_hash:                          # @end_name_hash
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rax
                                        # kill: def $eax killed $eax killed $rax
	popq	%rbp
	retq
.Lfunc_end2:
	.size	end_name_hash, .Lfunc_end2-end_name_hash
                                        # -- End function
	.globl	tinyfs_next_ino                 # -- Begin function tinyfs_next_ino
	.p2align	4
	.type	tinyfs_next_ino,@function
tinyfs_next_ino:                        # @tinyfs_next_ino
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rax
	movl	(%rax), %eax
	movl	%eax, -12(%rbp)
	movl	-12(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -12(%rbp)
	cmpl	$0, -12(%rbp)
	jne	.LBB3_2
# %bb.1:
	movl	-12(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -12(%rbp)
.LBB3_2:
	movl	-12(%rbp), %ecx
	movq	-8(%rbp), %rax
	movl	%ecx, (%rax)
	movl	-12(%rbp), %eax
	popq	%rbp
	retq
.Lfunc_end3:
	.size	tinyfs_next_ino, .Lfunc_end3-tinyfs_next_ino
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym partial_name_hash
	.addrsig_sym end_name_hash
