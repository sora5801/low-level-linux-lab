	.file	"demo.c"
	.text
	.globl	sum16                           # -- Begin function sum16
	.p2align	4
	.type	sum16,@function
sum16:                                  # @sum16
# %bb.0:
	movl	%edx, %eax
                                        # kill: def $esi killed $esi def $rsi
	cmpl	$2, %esi
	jb	.LBB0_6
# %bb.1:
	leal	-2(%rsi), %ecx
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
	addl	$-2, %esi
	decl	%edx
	jne	.LBB0_3
.LBB0_4:
	cmpl	$6, %ecx
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
	addl	$-8, %esi
	cmpl	$1, %esi
	ja	.LBB0_5
.LBB0_6:
	testl	%esi, %esi
	je	.LBB0_8
# %bb.7:
	movzbl	(%rdi), %ecx
	shll	$8, %ecx
	addl	%ecx, %eax
.LBB0_8:
	retq
.Lfunc_end0:
	.size	sum16, .Lfunc_end0-sum16
                                        # -- End function
	.globl	fold_csum                       # -- Begin function fold_csum
	.p2align	4
	.type	fold_csum,@function
fold_csum:                              # @fold_csum
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
	.size	fold_csum, .Lfunc_end1-fold_csum
                                        # -- End function
	.globl	tcp_checksum                    # -- Begin function tcp_checksum
	.p2align	4
	.type	tcp_checksum,@function
tcp_checksum:                           # @tcp_checksum
# %bb.0:
                                        # kill: def $edx killed $edx def $rdx
	movzwl	(%rdi), %eax
	movzwl	2(%rdi), %ecx
	rolw	$8, %ax
	rolw	$8, %cx
	movzwl	%ax, %eax
	movzwl	%cx, %ecx
	addl	%eax, %ecx
	movzwl	4(%rdi), %eax
	rolw	$8, %ax
	movzwl	%ax, %eax
	movzwl	6(%rdi), %r8d
	rolw	$8, %r8w
	movzwl	%r8w, %r8d
	addl	%eax, %r8d
	addl	%ecx, %r8d
	movzwl	8(%rdi), %eax
	rolw	$8, %ax
	movzwl	%ax, %ecx
	movzwl	10(%rdi), %eax
	rolw	$8, %ax
	movzwl	%ax, %eax
	addl	%ecx, %eax
	addl	%r8d, %eax
	cmpl	$2, %edx
	jb	.LBB2_6
# %bb.1:
	leal	-2(%rdx), %ecx
	movl	%ecx, %edi
	notl	%edi
	testb	$6, %dil
	je	.LBB2_4
# %bb.2:
	movl	%ecx, %edi
	shrl	%edi
	incl	%edi
	andl	$3, %edi
	.p2align	4
.LBB2_3:                                # =>This Inner Loop Header: Depth=1
	movzwl	(%rsi), %r8d
	rolw	$8, %r8w
	movzwl	%r8w, %r8d
	addl	%r8d, %eax
	addq	$2, %rsi
	addl	$-2, %edx
	decl	%edi
	jne	.LBB2_3
.LBB2_4:
	cmpl	$6, %ecx
	jb	.LBB2_6
	.p2align	4
.LBB2_5:                                # =>This Inner Loop Header: Depth=1
	movzwl	(%rsi), %ecx
	rolw	$8, %cx
	movzwl	2(%rsi), %edi
	movzwl	%cx, %ecx
	rolw	$8, %di
	addl	%eax, %ecx
	movzwl	4(%rsi), %eax
	rolw	$8, %ax
	movzwl	%di, %edi
	movzwl	%ax, %r8d
	addl	%edi, %r8d
	addl	%ecx, %r8d
	movzwl	6(%rsi), %eax
	rolw	$8, %ax
	movzwl	%ax, %eax
	addl	%r8d, %eax
	addq	$8, %rsi
	addl	$-8, %edx
	cmpl	$1, %edx
	ja	.LBB2_5
.LBB2_6:
	testl	%edx, %edx
	je	.LBB2_8
# %bb.7:
	movzbl	(%rsi), %ecx
	shll	$8, %ecx
	addl	%ecx, %eax
.LBB2_8:
	cmpl	$65536, %eax                    # imm = 0x10000
	jb	.LBB2_10
	.p2align	4
.LBB2_9:                                # =>This Inner Loop Header: Depth=1
	movl	%eax, %ecx
	shrl	$16, %ecx
	movzwl	%ax, %eax
	addl	%ecx, %eax
	cmpl	$65535, %eax                    # imm = 0xFFFF
	ja	.LBB2_9
.LBB2_10:
	notl	%eax
                                        # kill: def $ax killed $ax killed $eax
	retq
.Lfunc_end2:
	.size	tcp_checksum, .Lfunc_end2-tcp_checksum
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
