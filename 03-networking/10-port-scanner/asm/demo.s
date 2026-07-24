	.file	"demo.c"
	.text
	.globl	sum16                           # -- Begin function sum16
	.p2align	4
	.type	sum16,@function
sum16:                                  # @sum16
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	%edx, %eax
	cmpl	$2, %esi
	jb	.LBB0_2
	.p2align	4
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	movzwl	(%rdi), %ecx
	rolw	$8, %cx
	movzwl	%cx, %ecx
	addl	%ecx, %eax
	addq	$2, %rdi
	addl	$-2, %esi
	cmpl	$1, %esi
	ja	.LBB0_1
.LBB0_2:
	testl	%esi, %esi
	je	.LBB0_4
# %bb.3:
	movzbl	(%rdi), %ecx
	shll	$8, %ecx
	addl	%ecx, %eax
.LBB0_4:
	popq	%rbp
	retq
.Lfunc_end0:
	.size	sum16, .Lfunc_end0-sum16
                                        # -- End function
	.globl	fold_csum                       # -- Begin function fold_csum
	.p2align	4
	.type	fold_csum,@function
fold_csum:                              # @fold_csum
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
	.size	fold_csum, .Lfunc_end1-fold_csum
                                        # -- End function
	.globl	tcp_checksum                    # -- Begin function tcp_checksum
	.p2align	4
	.type	tcp_checksum,@function
tcp_checksum:                           # @tcp_checksum
# %bb.0:
	xorl	%ecx, %ecx
	xorl	%eax, %eax
	.p2align	4
.LBB2_1:                                # =>This Inner Loop Header: Depth=1
	movzwl	(%rdi,%rcx), %r8d
	rolw	$8, %r8w
	movzwl	%r8w, %r8d
	addl	%r8d, %eax
	addq	$2, %rcx
	cmpl	$12, %ecx
	jne	.LBB2_1
# %bb.2:
	pushq	%rbp
	movq	%rsp, %rbp
	cmpl	$2, %edx
	jb	.LBB2_4
	.p2align	4
.LBB2_3:                                # =>This Inner Loop Header: Depth=1
	movzwl	(%rsi), %ecx
	rolw	$8, %cx
	movzwl	%cx, %ecx
	addl	%ecx, %eax
	addq	$2, %rsi
	addl	$-2, %edx
	cmpl	$1, %edx
	ja	.LBB2_3
.LBB2_4:
	testl	%edx, %edx
	je	.LBB2_6
# %bb.5:
	movzbl	(%rsi), %ecx
	shll	$8, %ecx
	addl	%ecx, %eax
.LBB2_6:
	cmpl	$65536, %eax                    # imm = 0x10000
	popq	%rbp
	jb	.LBB2_8
	.p2align	4
.LBB2_7:                                # =>This Inner Loop Header: Depth=1
	movl	%eax, %ecx
	shrl	$16, %ecx
	movzwl	%ax, %eax
	addl	%ecx, %eax
	cmpl	$65535, %eax                    # imm = 0xFFFF
	ja	.LBB2_7
.LBB2_8:
	notl	%eax
                                        # kill: def $ax killed $ax killed $eax
	retq
.Lfunc_end2:
	.size	tcp_checksum, .Lfunc_end2-tcp_checksum
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
