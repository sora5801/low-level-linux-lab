	.file	"demo.c"
	.text
	.globl	dhcp_opt_find                   # -- Begin function dhcp_opt_find
	.p2align	4
	.type	dhcp_opt_find,@function
dhcp_opt_find:                          # @dhcp_opt_find
# %bb.0:
	testq	%rsi, %rsi
	je	.LBB0_12
# %bb.1:
	movq	%rdi, %rax
	xorl	%edi, %edi
	jmp	.LBB0_2
	.p2align	4
.LBB0_10:                               #   in Loop: Header=BB0_2 Depth=1
	incq	%rdi
	movq	%rdi, %r10
.LBB0_11:                               #   in Loop: Header=BB0_2 Depth=1
	movq	%r10, %rdi
	cmpq	%rsi, %r10
	jae	.LBB0_12
.LBB0_2:                                # =>This Inner Loop Header: Depth=1
	movzbl	(%rax,%rdi), %r8d
	testl	%r8d, %r8d
	je	.LBB0_10
# %bb.3:                                #   in Loop: Header=BB0_2 Depth=1
	cmpl	$255, %r8d
	je	.LBB0_12
# %bb.4:                                #   in Loop: Header=BB0_2 Depth=1
	leaq	1(%rdi), %r9
	cmpq	%rsi, %r9
	jae	.LBB0_12
# %bb.5:                                #   in Loop: Header=BB0_2 Depth=1
	movzbl	1(%rax,%rdi), %r9d
	leaq	(%rdi,%r9), %r10
	addq	$2, %r10
	cmpq	%rsi, %r10
	ja	.LBB0_12
# %bb.6:                                #   in Loop: Header=BB0_2 Depth=1
	cmpb	%dl, %r8b
	jne	.LBB0_11
# %bb.7:
	addq	$2, %rdi
	testq	%rcx, %rcx
	je	.LBB0_9
# %bb.8:
	movb	%r9b, (%rcx)
.LBB0_9:
	addq	%rdi, %rax
	retq
.LBB0_12:
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
	jb	.LBB1_6
# %bb.1:
	leaq	-2(%r8), %rdx
	movl	%edx, %esi
	notl	%esi
	testb	$6, %sil
	je	.LBB1_4
# %bb.2:
	movl	%edx, %esi
	shrl	%esi
	incl	%esi
	andl	$3, %esi
	.p2align	4
.LBB1_3:                                # =>This Inner Loop Header: Depth=1
	movzwl	(%rcx), %edi
	rolw	$8, %di
	movzwl	%di, %edi
	addl	%edi, %eax
	addq	$2, %rcx
	addq	$-2, %r8
	decq	%rsi
	jne	.LBB1_3
.LBB1_4:
	cmpq	$6, %rdx
	jb	.LBB1_6
	.p2align	4
.LBB1_5:                                # =>This Inner Loop Header: Depth=1
	movzwl	(%rcx), %edx
	rolw	$8, %dx
	movzwl	2(%rcx), %esi
	movzwl	%dx, %edx
	rolw	$8, %si
	addl	%eax, %edx
	movzwl	4(%rcx), %eax
	rolw	$8, %ax
	movzwl	%si, %esi
	movzwl	%ax, %edi
	addl	%esi, %edi
	addl	%edx, %edi
	movzwl	6(%rcx), %eax
	rolw	$8, %ax
	movzwl	%ax, %eax
	addl	%edi, %eax
	addq	$8, %rcx
	addq	$-8, %r8
	cmpq	$1, %r8
	ja	.LBB1_5
.LBB1_6:
	testq	%r8, %r8
	je	.LBB1_8
# %bb.7:
	movzbl	(%rcx), %ecx
	shll	$8, %ecx
	addl	%ecx, %eax
.LBB1_8:
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
	retq
.Lfunc_end1:
	.size	udp_checksum, .Lfunc_end1-udp_checksum
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
