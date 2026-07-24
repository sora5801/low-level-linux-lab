	.file	"demo.c"
	.text
	.globl	dict_hash                       # -- Begin function dict_hash
	.p2align	4
	.type	dict_hash,@function
dict_hash:                              # @dict_hash
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movabsq	$-4132994306676758123, %rax     # imm = 0xC6A4A7935BD1E995
	movq	%rsi, %r8
	imulq	%rax, %r8
	xorq	%rdx, %r8
	cmpq	$8, %rsi
	jae	.LBB0_17
# %bb.1:
	movq	%r8, %rcx
	jmp	.LBB0_2
.LBB0_17:
	movq	%rsi, %rdx
	shrq	$3, %rdx
	xorl	%r9d, %r9d
	.p2align	4
.LBB0_18:                               # =>This Inner Loop Header: Depth=1
	movq	(%rdi,%r9,8), %r10
	imulq	%rax, %r10
	movq	%r10, %rcx
	shrq	$47, %rcx
	xorq	%r10, %rcx
	imulq	%rax, %rcx
	xorq	%r8, %rcx
	imulq	%rax, %rcx
	incq	%r9
	movq	%rcx, %r8
	cmpq	%r9, %rdx
	jne	.LBB0_18
.LBB0_2:
	movq	%rsi, %rdx
	andq	$-8, %rdx
	andl	$7, %esi
	cmpq	$3, %rsi
	jg	.LBB0_9
# %bb.3:
	cmpq	$1, %rsi
	jg	.LBB0_6
# %bb.4:
	testq	%rsi, %rsi
	jne	.LBB0_8
	jmp	.LBB0_5
.LBB0_9:
	cmpq	$5, %rsi
	jg	.LBB0_13
# %bb.10:
	cmpl	$4, %esi
	jne	.LBB0_16
	jmp	.LBB0_11
.LBB0_6:
	cmpl	$2, %esi
	jne	.LBB0_12
	jmp	.LBB0_7
.LBB0_13:
	cmpl	$6, %esi
	je	.LBB0_15
# %bb.14:
	movzbl	6(%rdi,%rdx), %esi
	shlq	$48, %rsi
	xorq	%rsi, %rcx
.LBB0_15:
	movzbl	5(%rdi,%rdx), %esi
	shlq	$40, %rsi
	xorq	%rsi, %rcx
.LBB0_16:
	movzbl	4(%rdi,%rdx), %esi
	shlq	$32, %rsi
	xorq	%rsi, %rcx
.LBB0_11:
	movzbl	3(%rdi,%rdx), %esi
	shll	$24, %esi
	xorq	%rsi, %rcx
.LBB0_12:
	movzbl	2(%rdi,%rdx), %esi
	shll	$16, %esi
	xorq	%rsi, %rcx
.LBB0_7:
	movzbl	1(%rdi,%rdx), %esi
	shll	$8, %esi
	xorq	%rsi, %rcx
.LBB0_8:
	movzbl	(%rdi,%rdx), %edx
	xorq	%rcx, %rdx
	imulq	%rax, %rdx
	movq	%rdx, %rcx
.LBB0_5:
	movq	%rcx, %rdx
	shrq	$47, %rdx
	xorq	%rcx, %rdx
	imulq	%rax, %rdx
	movq	%rdx, %rax
	shrq	$47, %rax
	xorq	%rdx, %rax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	dict_hash, .Lfunc_end0-dict_hash
                                        # -- End function
	.globl	rehash_target_index             # -- Begin function rehash_target_index
	.p2align	4
	.type	rehash_target_index,@function
rehash_target_index:                    # @rehash_target_index
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rsi, %rax
	testq	%rcx, %rcx
	cmovnsq	%rdx, %rax
	andq	%rdi, %rax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	rehash_target_index, .Lfunc_end1-rehash_target_index
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
