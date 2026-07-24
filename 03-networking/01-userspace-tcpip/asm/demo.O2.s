	.file	"demo.c"
	.text
	.globl	csum_accumulate                 # -- Begin function csum_accumulate
	.p2align	4
	.type	csum_accumulate,@function
csum_accumulate:                        # @csum_accumulate
# %bb.0:
	movl	%edx, %eax
	cmpq	$2, %rsi
	jb	.LBB0_6
# %bb.1:
	leaq	-2(%rsi), %rcx
	movl	%ecx, %edx
	notl	%edx
	testb	$6, %dl
	je	.LBB0_4
# %bb.2:
	movl	%ecx, %edx
	shrl	%edx
	incl	%edx
	andl	$3, %edx
	.p2align	4
.LBB0_3:                                # =>This Inner Loop Header: Depth=1
	movzwl	(%rdi), %r8d
	rolw	$8, %r8w
	movzwl	%r8w, %r8d
	addl	%r8d, %eax
	addq	$2, %rdi
	addq	$-2, %rsi
	decq	%rdx
	jne	.LBB0_3
.LBB0_4:
	cmpq	$6, %rcx
	jb	.LBB0_6
	.p2align	4
.LBB0_5:                                # =>This Inner Loop Header: Depth=1
	movzwl	(%rdi), %ecx
	rolw	$8, %cx
	movzwl	2(%rdi), %edx
	movzwl	%cx, %ecx
	rolw	$8, %dx
	addl	%eax, %ecx
	movzwl	4(%rdi), %eax
	rolw	$8, %ax
	movzwl	%dx, %edx
	movzwl	%ax, %r8d
	addl	%edx, %r8d
	addl	%ecx, %r8d
	movzwl	6(%rdi), %eax
	rolw	$8, %ax
	movzwl	%ax, %eax
	addl	%r8d, %eax
	addq	$8, %rdi
	addq	$-8, %rsi
	cmpq	$1, %rsi
	ja	.LBB0_5
.LBB0_6:
	testq	%rsi, %rsi
	je	.LBB0_8
# %bb.7:
	movzbl	(%rdi), %ecx
	shll	$8, %ecx
	addl	%ecx, %eax
.LBB0_8:
	retq
.Lfunc_end0:
	.size	csum_accumulate, .Lfunc_end0-csum_accumulate
                                        # -- End function
	.globl	csum_fold                       # -- Begin function csum_fold
	.p2align	4
	.type	csum_fold,@function
csum_fold:                              # @csum_fold
# %bb.0:
	movl	%edi, %eax
	cmpl	$65536, %edi                    # imm = 0x10000
	jb	.LBB1_2
	.p2align	4
.LBB1_1:                                # =>This Inner Loop Header: Depth=1
	movl	%eax, %ecx
	shrl	$16, %ecx
	movzwl	%ax, %eax
	addl	%ecx, %eax
	cmpl	$65535, %eax                    # imm = 0xFFFF
	ja	.LBB1_1
.LBB1_2:
	notl	%eax
                                        # kill: def $ax killed $ax killed $eax
	retq
.Lfunc_end1:
	.size	csum_fold, .Lfunc_end1-csum_fold
                                        # -- End function
	.globl	inet_checksum                   # -- Begin function inet_checksum
	.p2align	4
	.type	inet_checksum,@function
inet_checksum:                          # @inet_checksum
# %bb.0:
	xorl	%eax, %eax
	cmpq	$2, %rsi
	jb	.LBB2_9
# %bb.1:
	leaq	-2(%rsi), %rax
	movq	%rax, %rdx
	shrq	%rdx
	incq	%rdx
	movl	%edx, %ecx
	andl	$3, %ecx
	cmpq	$6, %rax
	jae	.LBB2_3
# %bb.2:
	xorl	%eax, %eax
	jmp	.LBB2_5
.LBB2_3:
	andq	$-4, %rdx
	xorl	%eax, %eax
	.p2align	4
.LBB2_4:                                # =>This Inner Loop Header: Depth=1
	movzwl	(%rdi), %r8d
	rolw	$8, %r8w
	movzwl	2(%rdi), %r9d
	movzwl	%r8w, %r8d
	rolw	$8, %r9w
	addl	%eax, %r8d
	movzwl	4(%rdi), %eax
	rolw	$8, %ax
	movzwl	%r9w, %r9d
	movzwl	%ax, %r10d
	addl	%r9d, %r10d
	addl	%r8d, %r10d
	movzwl	6(%rdi), %eax
	rolw	$8, %ax
	movzwl	%ax, %eax
	addl	%r10d, %eax
	addq	$8, %rdi
	addq	$-8, %rsi
	addq	$-4, %rdx
	jne	.LBB2_4
.LBB2_5:
	testq	%rcx, %rcx
	je	.LBB2_9
# %bb.6:
	addl	%ecx, %ecx
	xorl	%edx, %edx
	.p2align	4
.LBB2_7:                                # =>This Inner Loop Header: Depth=1
	movzwl	(%rdi,%rdx), %r8d
	rolw	$8, %r8w
	movzwl	%r8w, %r8d
	addl	%r8d, %eax
	addq	$2, %rdx
	cmpq	%rdx, %rcx
	jne	.LBB2_7
# %bb.8:
	addq	%rdx, %rdi
	subq	%rdx, %rsi
.LBB2_9:
	testq	%rsi, %rsi
	je	.LBB2_11
# %bb.10:
	movzbl	(%rdi), %ecx
	shll	$8, %ecx
	addl	%ecx, %eax
.LBB2_11:
	cmpl	$65536, %eax                    # imm = 0x10000
	jb	.LBB2_13
	.p2align	4
.LBB2_12:                               # =>This Inner Loop Header: Depth=1
	movl	%eax, %ecx
	shrl	$16, %ecx
	movzwl	%ax, %eax
	addl	%ecx, %eax
	cmpl	$65535, %eax                    # imm = 0xFFFF
	ja	.LBB2_12
.LBB2_13:
	notl	%eax
                                        # kill: def $ax killed $ax killed $eax
	retq
.Lfunc_end2:
	.size	inet_checksum, .Lfunc_end2-inet_checksum
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
