	.file	"demo.c"
	.text
	.globl	tinyfs_name_hash                # -- Begin function tinyfs_name_hash
	.p2align	4
	.type	tinyfs_name_hash,@function
tinyfs_name_hash:                       # @tinyfs_name_hash
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	testl	%esi, %esi
	je	.LBB0_4
# %bb.1:
	movl	%esi, %ecx
	xorl	%edx, %edx
	xorl	%eax, %eax
	.p2align	4
.LBB0_2:                                # =>This Inner Loop Header: Depth=1
	movzbl	(%rdi,%rdx), %esi
	movl	%esi, %r8d
	shll	$4, %r8d
	addq	%rax, %r8
	shrl	$4, %esi
	addq	%r8, %rsi
	leaq	(%rsi,%rsi,4), %rax
	leaq	(%rsi,%rax,2), %rax
	incq	%rdx
	cmpl	%edx, %ecx
	jne	.LBB0_2
# %bb.3:
                                        # kill: def $eax killed $eax killed $rax
	popq	%rbp
	retq
.LBB0_4:
	xorl	%eax, %eax
                                        # kill: def $eax killed $eax killed $rax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	tinyfs_name_hash, .Lfunc_end0-tinyfs_name_hash
                                        # -- End function
	.globl	tinyfs_next_ino                 # -- Begin function tinyfs_next_ino
	.p2align	4
	.type	tinyfs_next_ino,@function
tinyfs_next_ino:                        # @tinyfs_next_ino
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	(%rdi), %eax
	leal	1(%rax), %ecx
	cmpl	$1, %ecx
	adcl	$1, %eax
	movl	%eax, (%rdi)
                                        # kill: def $eax killed $eax killed $rax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	tinyfs_next_ino, .Lfunc_end1-tinyfs_next_ino
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
