	.file	"demo.c"
	.text
	.globl	tinyfs_name_hash                # -- Begin function tinyfs_name_hash
	.p2align	4
	.type	tinyfs_name_hash,@function
tinyfs_name_hash:                       # @tinyfs_name_hash
# %bb.0:
	testl	%esi, %esi
	je	.LBB0_1
# %bb.3:
	cmpl	$1, %esi
	jne	.LBB0_5
# %bb.4:
	xorl	%eax, %eax
	jmp	.LBB0_7
.LBB0_1:
	xorl	%eax, %eax
	retq
.LBB0_5:
	movl	%esi, %ecx
	andl	$-2, %ecx
	xorl	%eax, %eax
	.p2align	4
.LBB0_6:                                # =>This Inner Loop Header: Depth=1
	movzbl	(%rdi), %edx
	movl	%edx, %r8d
	shll	$4, %r8d
	addl	%eax, %r8d
	shrl	$4, %edx
	addl	%r8d, %edx
	leal	(%rdx,%rdx,4), %eax
	leal	(%rdx,%rax,2), %eax
	movzbl	1(%rdi), %edx
	addq	$2, %rdi
	movl	%edx, %r8d
	shll	$4, %r8d
	shrl	$4, %edx
	addl	%r8d, %edx
	addl	%eax, %edx
	leal	(%rdx,%rdx,4), %eax
	leal	(%rdx,%rax,2), %eax
	addl	$-2, %ecx
	jne	.LBB0_6
.LBB0_7:
	testb	$1, %sil
	jne	.LBB0_8
# %bb.2:
	retq
.LBB0_8:
	movzbl	(%rdi), %ecx
	movl	%ecx, %edx
	shll	$4, %edx
	shrl	$4, %ecx
	addl	%edx, %ecx
	addl	%eax, %ecx
	leal	(%rcx,%rcx,4), %eax
	leal	(%rcx,%rax,2), %eax
	retq
.Lfunc_end0:
	.size	tinyfs_name_hash, .Lfunc_end0-tinyfs_name_hash
                                        # -- End function
	.globl	tinyfs_next_ino                 # -- Begin function tinyfs_next_ino
	.p2align	4
	.type	tinyfs_next_ino,@function
tinyfs_next_ino:                        # @tinyfs_next_ino
# %bb.0:
	movl	(%rdi), %eax
	leal	1(%rax), %ecx
	cmpl	$1, %ecx
	adcl	$1, %eax
	movl	%eax, (%rdi)
                                        # kill: def $eax killed $eax killed $rax
	retq
.Lfunc_end1:
	.size	tinyfs_next_ino, .Lfunc_end1-tinyfs_next_ino
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
