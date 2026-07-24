	.file	"demo.c"
	.text
	.globl	dhcp_opt_find                   # -- Begin function dhcp_opt_find
	.p2align	4
	.type	dhcp_opt_find,@function
dhcp_opt_find:                          # @dhcp_opt_find
# %bb.0:
	testq	%rsi, %rsi
	je	.LBB0_1
# %bb.3:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%rbx
                                        # implicit-def: $rax
	xorl	%r8d, %r8d
	jmp	.LBB0_4
	.p2align	4
.LBB0_15:                               #   in Loop: Header=BB0_4 Depth=1
	testl	%r9d, %r9d
	jne	.LBB0_19
.LBB0_16:                               #   in Loop: Header=BB0_4 Depth=1
	cmpq	%rsi, %r8
	jae	.LBB0_18
.LBB0_4:                                # =>This Inner Loop Header: Depth=1
	movzbl	(%rdi,%r8), %r10d
	movl	$3, %r9d
	cmpl	$255, %r10d
	je	.LBB0_14
# %bb.5:                                #   in Loop: Header=BB0_4 Depth=1
	testl	%r10d, %r10d
	jne	.LBB0_7
# %bb.6:                                #   in Loop: Header=BB0_4 Depth=1
	incq	%r8
	movl	$2, %r9d
	cmpl	$1, %r9d
	jle	.LBB0_15
	jmp	.LBB0_17
	.p2align	4
.LBB0_7:                                #   in Loop: Header=BB0_4 Depth=1
	leaq	1(%r8), %r11
	cmpq	%rsi, %r11
	jae	.LBB0_14
# %bb.8:                                #   in Loop: Header=BB0_4 Depth=1
	movzbl	1(%rdi,%r8), %r11d
	movzbl	%r11b, %ebx
	addq	%r8, %rbx
	addq	$2, %rbx
	cmpq	%rsi, %rbx
	ja	.LBB0_14
# %bb.9:                                #   in Loop: Header=BB0_4 Depth=1
	cmpb	%dl, %r10b
	jne	.LBB0_13
# %bb.10:                               #   in Loop: Header=BB0_4 Depth=1
	testq	%rcx, %rcx
	je	.LBB0_12
# %bb.11:                               #   in Loop: Header=BB0_4 Depth=1
	movb	%r11b, (%rcx)
.LBB0_12:                               #   in Loop: Header=BB0_4 Depth=1
	leaq	2(%r8), %rax
	addq	%rdi, %rax
	movl	$1, %r9d
	.p2align	4
.LBB0_14:                               #   in Loop: Header=BB0_4 Depth=1
	cmpl	$1, %r9d
	jle	.LBB0_15
.LBB0_17:                               #   in Loop: Header=BB0_4 Depth=1
	cmpl	$2, %r9d
	je	.LBB0_16
	jmp	.LBB0_18
.LBB0_13:                               #   in Loop: Header=BB0_4 Depth=1
	xorl	%r9d, %r9d
	movq	%rbx, %r8
	cmpl	$1, %r9d
	jle	.LBB0_15
	jmp	.LBB0_17
.LBB0_18:
	xorl	%eax, %eax
.LBB0_19:
	popq	%rbx
	popq	%rbp
	retq
.LBB0_1:
	xorl	%eax, %eax
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
	movl	%edi, %eax
	shrl	$24, %eax
	movl	%esi, %r9d
	shrl	$24, %r9d
	addl	%eax, %r9d
	movl	%edi, %eax
	rolw	$8, %ax
	movzwl	%ax, %eax
	shrl	$8, %edi
	andl	$65280, %edi                    # imm = 0xFF00
	movl	%esi, %r10d
	rolw	$8, %r10w
	addl	%r9d, %edi
	movzwl	%r10w, %r9d
	addl	%eax, %r9d
	shrl	$8, %esi
	andl	$65280, %esi                    # imm = 0xFF00
	addl	%edi, %esi
	addl	%r9d, %esi
	movzbl	4(%rdx), %eax
	movzbl	5(%rdx), %edi
	movzwl	(%rdx), %r9d
	rolw	$8, %r9w
	movzwl	2(%rdx), %r10d
	movzwl	%r9w, %r9d
	rolw	$8, %r10w
	movzwl	%r10w, %r10d
	movzwl	6(%rdx), %edx
	rolw	$8, %dx
	movzwl	%dx, %edx
	shll	$9, %eax
	leal	(%rax,%rdi,2), %eax
	addl	%esi, %eax
	addl	%r9d, %eax
	addl	%r10d, %eax
	addl	%edx, %eax
	addl	$17, %eax
	cmpq	$2, %r8
	jb	.LBB1_2
	.p2align	4
.LBB1_1:                                # =>This Inner Loop Header: Depth=1
	movzwl	(%rcx), %edx
	rolw	$8, %dx
	movzwl	%dx, %edx
	addl	%edx, %eax
	addq	$2, %rcx
	addq	$-2, %r8
	cmpq	$1, %r8
	ja	.LBB1_1
.LBB1_2:
	testq	%r8, %r8
	je	.LBB1_4
# %bb.3:
	movzbl	(%rcx), %ecx
	shll	$8, %ecx
	addl	%ecx, %eax
.LBB1_4:
	movzwl	%ax, %ecx
	shrl	$16, %eax
	addl	%ecx, %eax
	movl	%eax, %ecx
	shrl	$16, %ecx
	addl	%eax, %ecx
	cmpw	$-1, %cx
	notl	%ecx
	movl	$65535, %eax                    # imm = 0xFFFF
	cmovnel	%ecx, %eax
                                        # kill: def $ax killed $ax killed $eax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	udp_checksum, .Lfunc_end1-udp_checksum
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
