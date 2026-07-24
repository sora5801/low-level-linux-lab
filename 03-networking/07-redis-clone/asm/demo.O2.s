	.file	"demo.c"
	.text
	.globl	dict_hash                       # -- Begin function dict_hash
	.p2align	4
	.type	dict_hash,@function
dict_hash:                              # @dict_hash
# %bb.0:
	movabsq	$-4132994306676758123, %rax     # imm = 0xC6A4A7935BD1E995
	movq	%rsi, %rcx
	imulq	%rax, %rcx
	xorq	%rdx, %rcx
	cmpq	$8, %rsi
	jb	.LBB0_5
# %bb.1:
	movq	%rsi, %r8
	shrq	$3, %r8
	cmpq	$1, %r8
	jne	.LBB0_20
# %bb.2:
	xorl	%edx, %edx
	jmp	.LBB0_3
.LBB0_20:
	andq	$-2, %r8
	xorl	%edx, %edx
	.p2align	4
.LBB0_21:                               # =>This Inner Loop Header: Depth=1
	movq	(%rdi,%rdx), %r9
	imulq	%rax, %r9
	movq	%r9, %r10
	shrq	$47, %r10
	xorq	%r9, %r10
	imulq	%rax, %r10
	xorq	%rcx, %r10
	imulq	%rax, %r10
	movq	8(%rdi,%rdx), %r9
	imulq	%rax, %r9
	movq	%r9, %rcx
	shrq	$47, %rcx
	xorq	%r9, %rcx
	imulq	%rax, %rcx
	xorq	%r10, %rcx
	imulq	%rax, %rcx
	addq	$16, %rdx
	addq	$-2, %r8
	jne	.LBB0_21
.LBB0_3:
	testb	$8, %sil
	je	.LBB0_5
# %bb.4:
	movq	(%rdi,%rdx), %rdx
	imulq	%rax, %rdx
	movq	%rdx, %r8
	shrq	$47, %r8
	xorq	%rdx, %r8
	imulq	%rax, %r8
	xorq	%rcx, %r8
	imulq	%rax, %r8
	movq	%r8, %rcx
.LBB0_5:
	movq	%rsi, %rdx
	andq	$-8, %rdx
	andl	$7, %esi
	cmpq	$3, %rsi
	jg	.LBB0_12
# %bb.6:
	cmpq	$1, %rsi
	jg	.LBB0_9
# %bb.7:
	testq	%rsi, %rsi
	jne	.LBB0_11
	jmp	.LBB0_8
.LBB0_12:
	cmpq	$5, %rsi
	jg	.LBB0_16
# %bb.13:
	cmpl	$4, %esi
	jne	.LBB0_19
	jmp	.LBB0_14
.LBB0_9:
	cmpl	$2, %esi
	jne	.LBB0_15
	jmp	.LBB0_10
.LBB0_16:
	cmpl	$6, %esi
	je	.LBB0_18
# %bb.17:
	movzbl	6(%rdi,%rdx), %esi
	shlq	$48, %rsi
	xorq	%rsi, %rcx
.LBB0_18:
	movzbl	5(%rdi,%rdx), %esi
	shlq	$40, %rsi
	xorq	%rsi, %rcx
.LBB0_19:
	movzbl	4(%rdi,%rdx), %esi
	shlq	$32, %rsi
	xorq	%rsi, %rcx
.LBB0_14:
	movzbl	3(%rdi,%rdx), %esi
	shll	$24, %esi
	xorq	%rsi, %rcx
.LBB0_15:
	movzbl	2(%rdi,%rdx), %esi
	shll	$16, %esi
	xorq	%rsi, %rcx
.LBB0_10:
	movzbl	1(%rdi,%rdx), %esi
	shll	$8, %esi
	xorq	%rsi, %rcx
.LBB0_11:
	movzbl	(%rdi,%rdx), %edx
	xorq	%rcx, %rdx
	imulq	%rax, %rdx
	movq	%rdx, %rcx
.LBB0_8:
	movq	%rcx, %rdx
	shrq	$47, %rdx
	xorq	%rcx, %rdx
	imulq	%rax, %rdx
	movq	%rdx, %rax
	shrq	$47, %rax
	xorq	%rdx, %rax
	retq
.Lfunc_end0:
	.size	dict_hash, .Lfunc_end0-dict_hash
                                        # -- End function
	.globl	rehash_target_index             # -- Begin function rehash_target_index
	.p2align	4
	.type	rehash_target_index,@function
rehash_target_index:                    # @rehash_target_index
# %bb.0:
	movq	%rsi, %rax
	testq	%rcx, %rcx
	cmovnsq	%rdx, %rax
	andq	%rdi, %rax
	retq
.Lfunc_end1:
	.size	rehash_target_index, .Lfunc_end1-rehash_target_index
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
