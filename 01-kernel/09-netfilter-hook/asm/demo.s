	.file	"demo.c"
	.text
	.globl	ip_checksum                     # -- Begin function ip_checksum
	.p2align	4
	.type	ip_checksum,@function
ip_checksum:                            # @ip_checksum
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	xorl	%eax, %eax
	cmpl	$2, %esi
	jb	.LBB0_3
# %bb.1:
	xorl	%eax, %eax
	.p2align	4
.LBB0_2:                                # =>This Inner Loop Header: Depth=1
	movzwl	(%rdi), %ecx
	addl	%ecx, %eax
	addq	$2, %rdi
	addl	$-2, %esi
	cmpl	$1, %esi
	ja	.LBB0_2
.LBB0_3:
	testl	%esi, %esi
	je	.LBB0_5
# %bb.4:
	movzbl	(%rdi), %ecx
	addl	%ecx, %eax
.LBB0_5:
	cmpl	$65536, %eax                    # imm = 0x10000
	jb	.LBB0_7
	.p2align	4
.LBB0_6:                                # =>This Inner Loop Header: Depth=1
	movl	%eax, %ecx
	shrl	$16, %ecx
	movzwl	%ax, %eax
	addl	%ecx, %eax
	cmpl	$65535, %eax                    # imm = 0xFFFF
	ja	.LBB0_6
.LBB0_7:
	notl	%eax
                                        # kill: def $ax killed $ax killed $eax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	ip_checksum, .Lfunc_end0-ip_checksum
                                        # -- End function
	.globl	ip_checksum_valid               # -- Begin function ip_checksum_valid
	.p2align	4
	.type	ip_checksum_valid,@function
ip_checksum_valid:                      # @ip_checksum_valid
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	xorl	%ecx, %ecx
	cmpl	$2, %esi
	jb	.LBB1_3
# %bb.1:
	xorl	%ecx, %ecx
	.p2align	4
.LBB1_2:                                # =>This Inner Loop Header: Depth=1
	movzwl	(%rdi), %eax
	addl	%eax, %ecx
	addq	$2, %rdi
	addl	$-2, %esi
	cmpl	$1, %esi
	ja	.LBB1_2
.LBB1_3:
	testl	%esi, %esi
	je	.LBB1_5
# %bb.4:
	movzbl	(%rdi), %eax
	addl	%eax, %ecx
.LBB1_5:
	cmpl	$65536, %ecx                    # imm = 0x10000
	jb	.LBB1_7
	.p2align	4
.LBB1_6:                                # =>This Inner Loop Header: Depth=1
	movl	%ecx, %eax
	shrl	$16, %eax
	movzwl	%cx, %ecx
	addl	%eax, %ecx
	cmpl	$65535, %ecx                    # imm = 0xFFFF
	ja	.LBB1_6
.LBB1_7:
	xorl	%eax, %eax
	cmpl	$65535, %ecx                    # imm = 0xFFFF
	sete	%al
	popq	%rbp
	retq
.Lfunc_end1:
	.size	ip_checksum_valid, .Lfunc_end1-ip_checksum_valid
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
