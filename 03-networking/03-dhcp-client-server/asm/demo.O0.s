	.file	"demo.c"
	.text
	.globl	dhcp_opt_find                   # -- Begin function dhcp_opt_find
	.p2align	4
	.type	dhcp_opt_find,@function
dhcp_opt_find:                          # @dhcp_opt_find
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movb	%dl, %al
	movq	%rdi, -16(%rbp)
	movq	%rsi, -24(%rbp)
	movb	%al, -25(%rbp)
	movq	%rcx, -40(%rbp)
	movq	$0, -48(%rbp)
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	movq	-48(%rbp), %rax
	cmpq	-24(%rbp), %rax
	jae	.LBB0_15
# %bb.2:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rax
	movq	-48(%rbp), %rcx
	movb	(%rax,%rcx), %al
	movb	%al, -49(%rbp)
	movzbl	-49(%rbp), %eax
	cmpl	$0, %eax
	jne	.LBB0_4
# %bb.3:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-48(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -48(%rbp)
	jmp	.LBB0_1
.LBB0_4:                                #   in Loop: Header=BB0_1 Depth=1
	movzbl	-49(%rbp), %eax
	cmpl	$255, %eax
	jne	.LBB0_6
# %bb.5:
	jmp	.LBB0_15
.LBB0_6:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-48(%rbp), %rax
	addq	$1, %rax
	cmpq	-24(%rbp), %rax
	jb	.LBB0_8
# %bb.7:
	jmp	.LBB0_15
.LBB0_8:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rax
	movq	-48(%rbp), %rcx
	movb	1(%rax,%rcx), %al
	movb	%al, -50(%rbp)
	movq	-48(%rbp), %rax
	addq	$2, %rax
	movzbl	-50(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	addq	%rcx, %rax
	cmpq	-24(%rbp), %rax
	jbe	.LBB0_10
# %bb.9:
	jmp	.LBB0_15
.LBB0_10:                               #   in Loop: Header=BB0_1 Depth=1
	movzbl	-49(%rbp), %eax
	movzbl	-25(%rbp), %ecx
	cmpl	%ecx, %eax
	jne	.LBB0_14
# %bb.11:
	cmpq	$0, -40(%rbp)
	je	.LBB0_13
# %bb.12:
	movb	-50(%rbp), %cl
	movq	-40(%rbp), %rax
	movb	%cl, (%rax)
.LBB0_13:
	movq	-16(%rbp), %rax
	movq	-48(%rbp), %rcx
	addq	$2, %rcx
	addq	%rcx, %rax
	movq	%rax, -8(%rbp)
	jmp	.LBB0_16
.LBB0_14:                               #   in Loop: Header=BB0_1 Depth=1
	movzbl	-50(%rbp), %eax
                                        # kill: def $rax killed $eax
	addq	$2, %rax
	addq	-48(%rbp), %rax
	movq	%rax, -48(%rbp)
	jmp	.LBB0_1
.LBB0_15:
	movq	$0, -8(%rbp)
.LBB0_16:
	movq	-8(%rbp), %rax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	dhcp_opt_find, .Lfunc_end0-dhcp_opt_find
                                        # -- End function
	.globl	udp_checksum                    # -- Begin function udp_checksum
	.p2align	4
	.type	udp_checksum,@function
udp_checksum:                           # @udp_checksum
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	%edi, -4(%rbp)
	movl	%esi, -8(%rbp)
	movq	%rdx, -16(%rbp)
	movq	%rcx, -24(%rbp)
	movq	%r8, -32(%rbp)
	movl	$0, -36(%rbp)
	leaq	-4(%rbp), %rax
	movq	%rax, -48(%rbp)
	leaq	-8(%rbp), %rax
	movq	%rax, -56(%rbp)
	movq	-48(%rbp), %rax
	movzbl	(%rax), %eax
	shll	$8, %eax
	movq	-48(%rbp), %rcx
	movzbl	1(%rcx), %ecx
	orl	%ecx, %eax
	addl	-36(%rbp), %eax
	movl	%eax, -36(%rbp)
	movq	-48(%rbp), %rax
	movzbl	2(%rax), %eax
	shll	$8, %eax
	movq	-48(%rbp), %rcx
	movzbl	3(%rcx), %ecx
	orl	%ecx, %eax
	addl	-36(%rbp), %eax
	movl	%eax, -36(%rbp)
	movq	-56(%rbp), %rax
	movzbl	(%rax), %eax
	shll	$8, %eax
	movq	-56(%rbp), %rcx
	movzbl	1(%rcx), %ecx
	orl	%ecx, %eax
	addl	-36(%rbp), %eax
	movl	%eax, -36(%rbp)
	movq	-56(%rbp), %rax
	movzbl	2(%rax), %eax
	shll	$8, %eax
	movq	-56(%rbp), %rcx
	movzbl	3(%rcx), %ecx
	orl	%ecx, %eax
	addl	-36(%rbp), %eax
	movl	%eax, -36(%rbp)
	movl	-36(%rbp), %eax
	addl	$17, %eax
	movl	%eax, -36(%rbp)
	movq	-16(%rbp), %rax
	movzbl	4(%rax), %eax
	shll	$8, %eax
	movq	-16(%rbp), %rcx
	movzbl	5(%rcx), %ecx
	orl	%ecx, %eax
	addl	-36(%rbp), %eax
	movl	%eax, -36(%rbp)
	movq	-16(%rbp), %rax
	movzbl	(%rax), %eax
	shll	$8, %eax
	movq	-16(%rbp), %rcx
	movzbl	1(%rcx), %ecx
	orl	%ecx, %eax
	addl	-36(%rbp), %eax
	movl	%eax, -36(%rbp)
	movq	-16(%rbp), %rax
	movzbl	2(%rax), %eax
	shll	$8, %eax
	movq	-16(%rbp), %rcx
	movzbl	3(%rcx), %ecx
	orl	%ecx, %eax
	addl	-36(%rbp), %eax
	movl	%eax, -36(%rbp)
	movq	-16(%rbp), %rax
	movzbl	4(%rax), %eax
	shll	$8, %eax
	movq	-16(%rbp), %rcx
	movzbl	5(%rcx), %ecx
	orl	%ecx, %eax
	addl	-36(%rbp), %eax
	movl	%eax, -36(%rbp)
	movq	-16(%rbp), %rax
	movzbl	6(%rax), %eax
	shll	$8, %eax
	movq	-16(%rbp), %rcx
	movzbl	7(%rcx), %ecx
	orl	%ecx, %eax
	addl	-36(%rbp), %eax
	movl	%eax, -36(%rbp)
	movq	-32(%rbp), %rax
	movq	%rax, -64(%rbp)
	movq	-24(%rbp), %rax
	movq	%rax, -72(%rbp)
.LBB1_1:                                # =>This Inner Loop Header: Depth=1
	cmpq	$1, -64(%rbp)
	jbe	.LBB1_3
# %bb.2:                                #   in Loop: Header=BB1_1 Depth=1
	movq	-72(%rbp), %rax
	movzbl	(%rax), %eax
	shll	$8, %eax
	movq	-72(%rbp), %rcx
	movzbl	1(%rcx), %ecx
	orl	%ecx, %eax
	addl	-36(%rbp), %eax
	movl	%eax, -36(%rbp)
	movq	-72(%rbp), %rax
	addq	$2, %rax
	movq	%rax, -72(%rbp)
	movq	-64(%rbp), %rax
	subq	$2, %rax
	movq	%rax, -64(%rbp)
	jmp	.LBB1_1
.LBB1_3:
	cmpq	$1, -64(%rbp)
	jne	.LBB1_5
# %bb.4:
	movq	-72(%rbp), %rax
	movzbl	(%rax), %eax
	shll	$8, %eax
	addl	-36(%rbp), %eax
	movl	%eax, -36(%rbp)
.LBB1_5:
	movl	-36(%rbp), %eax
	andl	$65535, %eax                    # imm = 0xFFFF
	movl	-36(%rbp), %ecx
	shrl	$16, %ecx
	addl	%ecx, %eax
	movl	%eax, -36(%rbp)
	movl	-36(%rbp), %eax
	andl	$65535, %eax                    # imm = 0xFFFF
	movl	-36(%rbp), %ecx
	shrl	$16, %ecx
	addl	%ecx, %eax
	movl	%eax, -36(%rbp)
	movl	-36(%rbp), %eax
	xorl	$-1, %eax
	andl	$65535, %eax                    # imm = 0xFFFF
                                        # kill: def $ax killed $ax killed $eax
	movw	%ax, -74(%rbp)
	movzwl	-74(%rbp), %eax
	cmpl	$0, %eax
	jne	.LBB1_7
# %bb.6:
	movl	$65535, %eax                    # imm = 0xFFFF
	movl	%eax, -80(%rbp)                 # 4-byte Spill
	jmp	.LBB1_8
.LBB1_7:
	movzwl	-74(%rbp), %eax
	movl	%eax, -80(%rbp)                 # 4-byte Spill
.LBB1_8:
	movl	-80(%rbp), %eax                 # 4-byte Reload
                                        # kill: def $ax killed $ax killed $eax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	udp_checksum, .Lfunc_end1-udp_checksum
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
