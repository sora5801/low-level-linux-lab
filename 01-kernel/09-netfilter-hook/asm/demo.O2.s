	.file	"demo.c"
	.text
	.globl	ip_checksum                     # -- Begin function ip_checksum
	.p2align	4
	.type	ip_checksum,@function
ip_checksum:                            # @ip_checksum
# %bb.0:
                                        # kill: def $esi killed $esi def $rsi
	xorl	%eax, %eax
	cmpl	$2, %esi
	jb	.LBB0_1
# %bb.2:
	leal	-2(%rsi), %edx
	xorl	%eax, %eax
	cmpl	$14, %edx
	jae	.LBB0_4
# %bb.3:
	movq	%rdi, %rcx
	jmp	.LBB0_7
.LBB0_1:
	movq	%rdi, %rcx
	jmp	.LBB0_8
.LBB0_4:
	shrl	%edx
	incl	%edx
	movl	%edx, %r8d
	andl	$-8, %r8d
	subl	%r8d, %esi
	subl	%r8d, %esi
	leaq	(%rdi,%r8,2), %rcx
	pxor	%xmm1, %xmm1
	xorl	%eax, %eax
	pxor	%xmm2, %xmm2
	pxor	%xmm0, %xmm0
	.p2align	4
.LBB0_5:                                # =>This Inner Loop Header: Depth=1
	movq	(%rdi,%rax,2), %xmm3            # xmm3 = mem[0],zero
	movq	8(%rdi,%rax,2), %xmm4           # xmm4 = mem[0],zero
	punpcklwd	%xmm1, %xmm3            # xmm3 = xmm3[0],xmm1[0],xmm3[1],xmm1[1],xmm3[2],xmm1[2],xmm3[3],xmm1[3]
	paddd	%xmm3, %xmm2
	punpcklwd	%xmm1, %xmm4            # xmm4 = xmm4[0],xmm1[0],xmm4[1],xmm1[1],xmm4[2],xmm1[2],xmm4[3],xmm1[3]
	paddd	%xmm4, %xmm0
	addq	$8, %rax
	cmpq	%rax, %r8
	jne	.LBB0_5
# %bb.6:
	paddd	%xmm2, %xmm0
	pshufd	$238, %xmm0, %xmm1              # xmm1 = xmm0[2,3,2,3]
	paddd	%xmm0, %xmm1
	pshufd	$85, %xmm1, %xmm0               # xmm0 = xmm1[1,1,1,1]
	paddd	%xmm1, %xmm0
	movd	%xmm0, %eax
	cmpl	%edx, %r8d
	je	.LBB0_8
	.p2align	4
.LBB0_7:                                # =>This Inner Loop Header: Depth=1
	movzwl	(%rcx), %edx
	addl	%edx, %eax
	addq	$2, %rcx
	addl	$-2, %esi
	cmpl	$1, %esi
	ja	.LBB0_7
.LBB0_8:
	testl	%esi, %esi
	je	.LBB0_10
# %bb.9:
	movzbl	(%rcx), %ecx
	addl	%ecx, %eax
.LBB0_10:
	cmpl	$65536, %eax                    # imm = 0x10000
	jb	.LBB0_12
	.p2align	4
.LBB0_11:                               # =>This Inner Loop Header: Depth=1
	movl	%eax, %ecx
	shrl	$16, %ecx
	movzwl	%ax, %eax
	addl	%ecx, %eax
	cmpl	$65535, %eax                    # imm = 0xFFFF
	ja	.LBB0_11
.LBB0_12:
	notl	%eax
                                        # kill: def $ax killed $ax killed $eax
	retq
.Lfunc_end0:
	.size	ip_checksum, .Lfunc_end0-ip_checksum
                                        # -- End function
	.globl	ip_checksum_valid               # -- Begin function ip_checksum_valid
	.p2align	4
	.type	ip_checksum_valid,@function
ip_checksum_valid:                      # @ip_checksum_valid
# %bb.0:
                                        # kill: def $esi killed $esi def $rsi
	xorl	%ecx, %ecx
	cmpl	$2, %esi
	jb	.LBB1_1
# %bb.2:
	leal	-2(%rsi), %edx
	xorl	%ecx, %ecx
	cmpl	$14, %edx
	jae	.LBB1_4
# %bb.3:
	movq	%rdi, %rax
	jmp	.LBB1_7
.LBB1_1:
	movq	%rdi, %rax
	jmp	.LBB1_8
.LBB1_4:
	shrl	%edx
	incl	%edx
	movl	%edx, %r8d
	andl	$-8, %r8d
	subl	%r8d, %esi
	subl	%r8d, %esi
	leaq	(%rdi,%r8,2), %rax
	pxor	%xmm1, %xmm1
	xorl	%ecx, %ecx
	pxor	%xmm2, %xmm2
	pxor	%xmm0, %xmm0
	.p2align	4
.LBB1_5:                                # =>This Inner Loop Header: Depth=1
	movq	(%rdi,%rcx,2), %xmm3            # xmm3 = mem[0],zero
	movq	8(%rdi,%rcx,2), %xmm4           # xmm4 = mem[0],zero
	punpcklwd	%xmm1, %xmm3            # xmm3 = xmm3[0],xmm1[0],xmm3[1],xmm1[1],xmm3[2],xmm1[2],xmm3[3],xmm1[3]
	paddd	%xmm3, %xmm2
	punpcklwd	%xmm1, %xmm4            # xmm4 = xmm4[0],xmm1[0],xmm4[1],xmm1[1],xmm4[2],xmm1[2],xmm4[3],xmm1[3]
	paddd	%xmm4, %xmm0
	addq	$8, %rcx
	cmpq	%rcx, %r8
	jne	.LBB1_5
# %bb.6:
	paddd	%xmm2, %xmm0
	pshufd	$238, %xmm0, %xmm1              # xmm1 = xmm0[2,3,2,3]
	paddd	%xmm0, %xmm1
	pshufd	$85, %xmm1, %xmm0               # xmm0 = xmm1[1,1,1,1]
	paddd	%xmm1, %xmm0
	movd	%xmm0, %ecx
	cmpl	%edx, %r8d
	je	.LBB1_8
	.p2align	4
.LBB1_7:                                # =>This Inner Loop Header: Depth=1
	movzwl	(%rax), %edx
	addl	%edx, %ecx
	addq	$2, %rax
	addl	$-2, %esi
	cmpl	$1, %esi
	ja	.LBB1_7
.LBB1_8:
	testl	%esi, %esi
	je	.LBB1_10
# %bb.9:
	movzbl	(%rax), %eax
	addl	%eax, %ecx
.LBB1_10:
	cmpl	$65536, %ecx                    # imm = 0x10000
	jb	.LBB1_12
	.p2align	4
.LBB1_11:                               # =>This Inner Loop Header: Depth=1
	movl	%ecx, %eax
	shrl	$16, %eax
	movzwl	%cx, %ecx
	addl	%eax, %ecx
	cmpl	$65535, %ecx                    # imm = 0xFFFF
	ja	.LBB1_11
.LBB1_12:
	xorl	%eax, %eax
	cmpl	$65535, %ecx                    # imm = 0xFFFF
	sete	%al
	retq
.Lfunc_end1:
	.size	ip_checksum_valid, .Lfunc_end1-ip_checksum_valid
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
