	.file	"order.c"
	.text
	.globl	sup_toposort                    # -- Begin function sup_toposort
	.p2align	4
	.type	sup_toposort,@function
sup_toposort:                           # @sup_toposort
# %bb.0:
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	subq	$128, %rsp
	testl	%edi, %edi
	jle	.LBB0_1
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
	jmp	.LBB0_3
	.p2align	4
.LBB0_15:                               #   in Loop: Header=BB0_3 Depth=1
	movb	%dil, 64(%rsp,%r13)
	incq	%r13
	addq	%r12, %rdx
	addq	%r12, %rsi
	cmpq	%r12, %r13
	je	.LBB0_16
.LBB0_3:                                # =>This Loop Header: Depth=1
                                        #     Child Loop BB0_8 Depth 2
                                        #     Child Loop BB0_12 Depth 2
                                        #     Child Loop BB0_14 Depth 2
	cmpl	$4, %ebx
	jae	.LBB0_5
# %bb.4:                                #   in Loop: Header=BB0_3 Depth=1
	xorl	%r9d, %r9d
	xorl	%edi, %edi
	jmp	.LBB0_14
	.p2align	4
.LBB0_5:                                #   in Loop: Header=BB0_3 Depth=1
	cmpl	$32, %ebx
	jae	.LBB0_7
# %bb.6:                                #   in Loop: Header=BB0_3 Depth=1
	xorl	%r8d, %r8d
	xorl	%edi, %edi
	jmp	.LBB0_11
	.p2align	4
.LBB0_7:                                #   in Loop: Header=BB0_3 Depth=1
	pxor	%xmm2, %xmm2
	xorl	%edi, %edi
	pxor	%xmm3, %xmm3
	.p2align	4
.LBB0_8:                                #   Parent Loop BB0_3 Depth=1
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
	jne	.LBB0_8
# %bb.9:                                #   in Loop: Header=BB0_3 Depth=1
	paddb	%xmm2, %xmm3
	pshufd	$238, %xmm3, %xmm2              # xmm2 = xmm3[2,3,2,3]
	paddb	%xmm3, %xmm2
	psadbw	%xmm0, %xmm2
	movd	%xmm2, %edi
	cmpl	%r12d, %eax
	je	.LBB0_15
# %bb.10:                               #   in Loop: Header=BB0_3 Depth=1
	movq	%rax, %r8
	movq	%rax, %r9
	testb	$28, %r12b
	je	.LBB0_14
.LBB0_11:                               #   in Loop: Header=BB0_3 Depth=1
	movzbl	%dil, %edi
	movd	%edi, %xmm2
	.p2align	4
.LBB0_12:                               #   Parent Loop BB0_3 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movd	(%rsi,%r8), %xmm3               # xmm3 = mem[0],zero,zero,zero
	pcmpeqb	%xmm0, %xmm3
	paddb	%xmm3, %xmm2
	psubb	%xmm1, %xmm2
	addq	$4, %r8
	cmpq	%r8, %rcx
	jne	.LBB0_12
# %bb.13:                               #   in Loop: Header=BB0_3 Depth=1
	punpckldq	%xmm0, %xmm2            # xmm2 = xmm2[0],xmm0[0],xmm2[1],xmm0[1]
	psadbw	%xmm0, %xmm2
	movd	%xmm2, %edi
	movq	%rcx, %r9
	cmpl	%r12d, %ecx
	je	.LBB0_15
	.p2align	4
.LBB0_14:                               #   Parent Loop BB0_3 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	cmpb	$1, (%rsi,%r9)
	sbbb	$-1, %dil
	incq	%r9
	cmpq	%r9, %r12
	jne	.LBB0_14
	jmp	.LBB0_15
.LBB0_16:
	xorl	%eax, %eax
.LBB0_17:                               # =>This Loop Header: Depth=1
                                        #     Child Loop BB0_18 Depth 2
                                        #     Child Loop BB0_23 Depth 2
	movq	%r15, %rcx
	xorl	%edx, %edx
	jmp	.LBB0_18
	.p2align	4
.LBB0_20:                               #   in Loop: Header=BB0_18 Depth=2
	incq	%rdx
	incq	%rcx
	cmpq	%r12, %rdx
	je	.LBB0_21
.LBB0_18:                               #   Parent Loop BB0_17 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	cmpb	$0, (%rsp,%rdx)
	jne	.LBB0_20
# %bb.19:                               #   in Loop: Header=BB0_18 Depth=2
	cmpb	$0, 64(%rsp,%rdx)
	jne	.LBB0_20
# %bb.22:                               #   in Loop: Header=BB0_17 Depth=1
	movl	%edx, (%r14,%rax,4)
	incq	%rax
	movl	%edx, %edx
	movb	$1, (%rsp,%rdx)
	xorl	%edx, %edx
	jmp	.LBB0_23
	.p2align	4
.LBB0_27:                               #   in Loop: Header=BB0_23 Depth=2
	incq	%rdx
	addq	%r12, %rcx
	cmpq	%rdx, %r12
	je	.LBB0_28
.LBB0_23:                               #   Parent Loop BB0_17 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	cmpb	$0, (%rsp,%rdx)
	jne	.LBB0_27
# %bb.24:                               #   in Loop: Header=BB0_23 Depth=2
	cmpb	$0, (%rcx)
	je	.LBB0_27
# %bb.25:                               #   in Loop: Header=BB0_23 Depth=2
	movzbl	64(%rsp,%rdx), %esi
	testb	%sil, %sil
	je	.LBB0_27
# %bb.26:                               #   in Loop: Header=BB0_23 Depth=2
	decb	%sil
	movb	%sil, 64(%rsp,%rdx)
	jmp	.LBB0_27
	.p2align	4
.LBB0_28:                               #   in Loop: Header=BB0_17 Depth=1
	cmpq	%r12, %rax
	jne	.LBB0_17
	jmp	.LBB0_29
.LBB0_21:
	movl	%eax, %ebx
	jmp	.LBB0_29
.LBB0_1:
	xorl	%ebx, %ebx
.LBB0_29:
	movl	%ebx, %eax
	addq	$128, %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	retq
.Lfunc_end0:
	.size	sup_toposort, .Lfunc_end0-sup_toposort
                                        # -- End function
	.globl	sup_backoff_delay_ms            # -- Begin function sup_backoff_delay_ms
	.p2align	4
	.type	sup_backoff_delay_ms,@function
sup_backoff_delay_ms:                   # @sup_backoff_delay_ms
# %bb.0:
	movq	%rdx, %rax
	cmpl	$63, %edi
	movl	$63, %ecx
	cmovbl	%edi, %ecx
	testl	%edi, %edi
	je	.LBB1_3
	.p2align	4
.LBB1_1:                                # =>This Inner Loop Header: Depth=1
	cmpq	%rax, %rsi
	jae	.LBB1_4
# %bb.2:                                #   in Loop: Header=BB1_1 Depth=1
	addq	%rsi, %rsi
	decl	%ecx
	jne	.LBB1_1
.LBB1_3:
	cmpq	%rax, %rsi
	cmovbq	%rsi, %rax
.LBB1_4:
	retq
.Lfunc_end1:
	.size	sup_backoff_delay_ms, .Lfunc_end1-sup_backoff_delay_ms
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
