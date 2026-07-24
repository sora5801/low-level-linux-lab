	.file	"demo.c"
	.text
	.globl	mk_needs_rebuild                # -- Begin function mk_needs_rebuild
	.p2align	4
	.type	mk_needs_rebuild,@function
mk_needs_rebuild:                       # @mk_needs_rebuild
# %bb.0:
	orl	%esi, %edi
	movl	$1, %eax
	orl	%edx, %edi
	je	.LBB0_1
.LBB0_7:
	retq
.LBB0_1:
	movl	8(%rsp), %edx
	testl	%edx, %edx
	jle	.LBB0_6
# %bb.2:
	movl	%edx, %edx
	xorl	%esi, %esi
	.p2align	4
.LBB0_3:                                # =>This Inner Loop Header: Depth=1
	cmpl	$0, (%r9,%rsi,4)
	jne	.LBB0_7
# %bb.4:                                #   in Loop: Header=BB0_3 Depth=1
	cmpq	%rcx, (%r8,%rsi,8)
	jg	.LBB0_7
# %bb.5:                                #   in Loop: Header=BB0_3 Depth=1
	incq	%rsi
	cmpq	%rsi, %rdx
	jne	.LBB0_3
.LBB0_6:
	xorl	%eax, %eax
	retq
.Lfunc_end0:
	.size	mk_needs_rebuild, .Lfunc_end0-mk_needs_rebuild
                                        # -- End function
	.globl	mk_toposort                     # -- Begin function mk_toposort
	.p2align	4
	.type	mk_toposort,@function
mk_toposort:                            # @mk_toposort
# %bb.0:
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	subq	$128, %rsp
	testl	%edi, %edi
	jle	.LBB1_1
# %bb.2:
	movq	%rdx, %r14
	movq	%rsi, %r15
	movl	%edi, %ebx
	movl	%edi, %r12d
	movq	%rsp, %rdi
	xorl	%r13d, %r13d
	xorl	%esi, %esi
	movq	%r12, %rdx
	callq	memset@PLT
	movl	%r12d, %eax
	andl	$2147483616, %eax               # imm = 0x7FFFFFE0
	movl	%r12d, %ecx
	andl	$2147483644, %ecx               # imm = 0x7FFFFFFC
	leaq	16(%r15), %rdx
	pxor	%xmm0, %xmm0
	pcmpeqd	%xmm1, %xmm1
	movq	%r15, %rsi
	jmp	.LBB1_3
	.p2align	4
.LBB1_15:                               #   in Loop: Header=BB1_3 Depth=1
	movb	%dil, 64(%rsp,%r13)
	incq	%r13
	addq	%r12, %rdx
	addq	%r12, %rsi
	cmpq	%r12, %r13
	je	.LBB1_16
.LBB1_3:                                # =>This Loop Header: Depth=1
                                        #     Child Loop BB1_8 Depth 2
                                        #     Child Loop BB1_12 Depth 2
                                        #     Child Loop BB1_14 Depth 2
	cmpl	$4, %ebx
	jae	.LBB1_5
# %bb.4:                                #   in Loop: Header=BB1_3 Depth=1
	xorl	%r9d, %r9d
	xorl	%edi, %edi
	jmp	.LBB1_14
	.p2align	4
.LBB1_5:                                #   in Loop: Header=BB1_3 Depth=1
	cmpl	$32, %ebx
	jae	.LBB1_7
# %bb.6:                                #   in Loop: Header=BB1_3 Depth=1
	xorl	%r8d, %r8d
	xorl	%edi, %edi
	jmp	.LBB1_11
	.p2align	4
.LBB1_7:                                #   in Loop: Header=BB1_3 Depth=1
	pxor	%xmm2, %xmm2
	xorl	%edi, %edi
	pxor	%xmm3, %xmm3
	.p2align	4
.LBB1_8:                                #   Parent Loop BB1_3 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movdqu	-16(%rdx,%rdi), %xmm4
	movdqu	(%rdx,%rdi), %xmm5
	pcmpeqb	%xmm0, %xmm4
	paddb	%xmm4, %xmm2
	pcmpeqb	%xmm0, %xmm5
	paddb	%xmm5, %xmm3
	psubb	%xmm1, %xmm2
	psubb	%xmm1, %xmm3
	addq	$32, %rdi
	cmpq	%rdi, %rax
	jne	.LBB1_8
# %bb.9:                                #   in Loop: Header=BB1_3 Depth=1
	paddb	%xmm2, %xmm3
	pshufd	$238, %xmm3, %xmm2              # xmm2 = xmm3[2,3,2,3]
	paddb	%xmm3, %xmm2
	psadbw	%xmm0, %xmm2
	movd	%xmm2, %edi
	cmpl	%r12d, %eax
	je	.LBB1_15
# %bb.10:                               #   in Loop: Header=BB1_3 Depth=1
	movq	%rax, %r8
	movq	%rax, %r9
	testb	$28, %r12b
	je	.LBB1_14
.LBB1_11:                               #   in Loop: Header=BB1_3 Depth=1
	movzbl	%dil, %edi
	movd	%edi, %xmm2
	.p2align	4
.LBB1_12:                               #   Parent Loop BB1_3 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movd	(%rsi,%r8), %xmm3               # xmm3 = mem[0],zero,zero,zero
	pcmpeqb	%xmm0, %xmm3
	paddb	%xmm3, %xmm2
	psubb	%xmm1, %xmm2
	addq	$4, %r8
	cmpq	%r8, %rcx
	jne	.LBB1_12
# %bb.13:                               #   in Loop: Header=BB1_3 Depth=1
	punpckldq	%xmm0, %xmm2            # xmm2 = xmm2[0],xmm0[0],xmm2[1],xmm0[1]
	psadbw	%xmm0, %xmm2
	movd	%xmm2, %edi
	movq	%rcx, %r9
	cmpl	%r12d, %ecx
	je	.LBB1_15
	.p2align	4
.LBB1_14:                               #   Parent Loop BB1_3 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	cmpb	$1, (%rsi,%r9)
	sbbb	$-1, %dil
	incq	%r9
	cmpq	%r9, %r12
	jne	.LBB1_14
	jmp	.LBB1_15
.LBB1_16:
	xorl	%eax, %eax
.LBB1_17:                               # =>This Loop Header: Depth=1
                                        #     Child Loop BB1_18 Depth 2
                                        #     Child Loop BB1_23 Depth 2
	movq	%r15, %rcx
	xorl	%edx, %edx
	jmp	.LBB1_18
	.p2align	4
.LBB1_20:                               #   in Loop: Header=BB1_18 Depth=2
	incq	%rdx
	incq	%rcx
	cmpq	%r12, %rdx
	je	.LBB1_21
.LBB1_18:                               #   Parent Loop BB1_17 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	cmpb	$0, (%rsp,%rdx)
	jne	.LBB1_20
# %bb.19:                               #   in Loop: Header=BB1_18 Depth=2
	cmpb	$0, 64(%rsp,%rdx)
	jne	.LBB1_20
# %bb.22:                               #   in Loop: Header=BB1_17 Depth=1
	movl	%edx, (%r14,%rax,4)
	incq	%rax
	movl	%edx, %edx
	movb	$1, (%rsp,%rdx)
	xorl	%edx, %edx
	jmp	.LBB1_23
	.p2align	4
.LBB1_27:                               #   in Loop: Header=BB1_23 Depth=2
	incq	%rdx
	addq	%r12, %rcx
	cmpq	%rdx, %r12
	je	.LBB1_28
.LBB1_23:                               #   Parent Loop BB1_17 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	cmpb	$0, (%rsp,%rdx)
	jne	.LBB1_27
# %bb.24:                               #   in Loop: Header=BB1_23 Depth=2
	cmpb	$0, (%rcx)
	je	.LBB1_27
# %bb.25:                               #   in Loop: Header=BB1_23 Depth=2
	movzbl	64(%rsp,%rdx), %esi
	testb	%sil, %sil
	je	.LBB1_27
# %bb.26:                               #   in Loop: Header=BB1_23 Depth=2
	decb	%sil
	movb	%sil, 64(%rsp,%rdx)
	jmp	.LBB1_27
	.p2align	4
.LBB1_28:                               #   in Loop: Header=BB1_17 Depth=1
	cmpq	%r12, %rax
	jne	.LBB1_17
	jmp	.LBB1_29
.LBB1_21:
	movl	%eax, %ebx
	jmp	.LBB1_29
.LBB1_1:
	xorl	%ebx, %ebx
.LBB1_29:
	movl	%ebx, %eax
	addq	$128, %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	retq
.Lfunc_end1:
	.size	mk_toposort, .Lfunc_end1-mk_toposort
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
