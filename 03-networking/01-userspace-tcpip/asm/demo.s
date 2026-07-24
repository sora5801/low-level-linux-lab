	.file	"demo.c"
	.text
	.globl	csum_accumulate                 # -- Begin function csum_accumulate
	.p2align	4
	.type	csum_accumulate,@function
csum_accumulate:                        # @csum_accumulate
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	%edx, %eax
	cmpq	$2, %rsi
	jb	.LBB0_2
	.p2align	4
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	movzwl	(%rdi), %ecx
	rolw	$8, %cx
	movzwl	%cx, %ecx
	addl	%ecx, %eax
	addq	$2, %rdi
	addq	$-2, %rsi
	cmpq	$1, %rsi
	ja	.LBB0_1
.LBB0_2:
	testq	%rsi, %rsi
	je	.LBB0_4
# %bb.3:
	movzbl	(%rdi), %ecx
	shll	$8, %ecx
	addl	%ecx, %eax
.LBB0_4:
	popq	%rbp
	retq
.Lfunc_end0:
	.size	csum_accumulate, .Lfunc_end0-csum_accumulate
                                        # -- End function
	.globl	csum_fold                       # -- Begin function csum_fold
	.p2align	4
	.type	csum_fold,@function
csum_fold:                              # @csum_fold
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
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
	popq	%rbp
	retq
.Lfunc_end1:
	.size	csum_fold, .Lfunc_end1-csum_fold
                                        # -- End function
	.globl	inet_checksum                   # -- Begin function inet_checksum
	.p2align	4
	.type	inet_checksum,@function
inet_checksum:                          # @inet_checksum
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	xorl	%eax, %eax
	cmpq	$2, %rsi
	jb	.LBB2_3
# %bb.1:
	xorl	%eax, %eax
	.p2align	4
.LBB2_2:                                # =>This Inner Loop Header: Depth=1
	movzwl	(%rdi), %ecx
	rolw	$8, %cx
	movzwl	%cx, %ecx
	addl	%ecx, %eax
	addq	$2, %rdi
	addq	$-2, %rsi
	cmpq	$1, %rsi
	ja	.LBB2_2
.LBB2_3:
	testq	%rsi, %rsi
	je	.LBB2_5
# %bb.4:
	movzbl	(%rdi), %ecx
	shll	$8, %ecx
	addl	%ecx, %eax
.LBB2_5:
	cmpl	$65536, %eax                    # imm = 0x10000
	jb	.LBB2_7
	.p2align	4
.LBB2_6:                                # =>This Inner Loop Header: Depth=1
	movl	%eax, %ecx
	shrl	$16, %ecx
	movzwl	%ax, %eax
	addl	%ecx, %eax
	cmpl	$65535, %eax                    # imm = 0xFFFF
	ja	.LBB2_6
.LBB2_7:
	notl	%eax
                                        # kill: def $ax killed $ax killed $eax
	popq	%rbp
	retq
.Lfunc_end2:
	.size	inet_checksum, .Lfunc_end2-inet_checksum
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
