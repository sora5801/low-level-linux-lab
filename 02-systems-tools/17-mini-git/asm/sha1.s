	.file	"sha1.c"
	.section	.rodata.cst16,"aM",@progbits,16
	.p2align	4, 0x0                          # -- Begin function sha1_init
.LCPI0_0:
	.long	1732584193                      # 0x67452301
	.long	4023233417                      # 0xefcdab89
	.long	2562383102                      # 0x98badcfe
	.long	271733878                       # 0x10325476
	.text
	.globl	sha1_init
	.p2align	4
	.type	sha1_init,@function
sha1_init:                              # @sha1_init
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movaps	.LCPI0_0(%rip), %xmm0           # xmm0 = [1732584193,4023233417,2562383102,271733878]
	movups	%xmm0, (%rdi)
	movl	$-1009589776, 16(%rdi)          # imm = 0xC3D2E1F0
	movq	$0, 24(%rdi)
	movl	$0, 96(%rdi)
	popq	%rbp
	retq
.Lfunc_end0:
	.size	sha1_init, .Lfunc_end0-sha1_init
                                        # -- End function
	.globl	sha1_update                     # -- Begin function sha1_update
	.p2align	4
	.type	sha1_update,@function
sha1_update:                            # @sha1_update
# %bb.0:
	leaq	(,%rdx,8), %rax
	addq	%rax, 24(%rdi)
	testq	%rdx, %rdx
	je	.LBB1_21
# %bb.1:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	subq	$216, %rsp
	jmp	.LBB1_2
	.p2align	4
.LBB1_19:                               #   in Loop: Header=BB1_2 Depth=1
	addq	%rax, %rsi
	subq	%rax, %rdx
	je	.LBB1_20
.LBB1_2:                                # =>This Loop Header: Depth=1
                                        #     Child Loop BB1_4 Depth 2
                                        #     Child Loop BB1_7 Depth 2
                                        #     Child Loop BB1_9 Depth 2
                                        #     Child Loop BB1_11 Depth 2
                                        #     Child Loop BB1_13 Depth 2
                                        #     Child Loop BB1_15 Depth 2
                                        #     Child Loop BB1_17 Depth 2
	movl	96(%rdi), %ecx
	leal	-64(%rcx), %eax
	negl	%eax
	cmpq	%rax, %rdx
	cmovbq	%rdx, %rax
	cmpl	$64, %ecx
	je	.LBB1_5
# %bb.3:                                #   in Loop: Header=BB1_2 Depth=1
	cmpl	$1, %eax
	movl	%eax, %ecx
	adcl	$0, %ecx
	xorl	%r8d, %r8d
	.p2align	4
.LBB1_4:                                #   Parent Loop BB1_2 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movzbl	(%rsi,%r8), %r9d
	movl	96(%rdi), %r10d
	addl	%r8d, %r10d
	movb	%r9b, 32(%rdi,%r10)
	incq	%r8
	cmpq	%r8, %rcx
	jne	.LBB1_4
.LBB1_5:                                #   in Loop: Header=BB1_2 Depth=1
	movl	96(%rdi), %ecx
	addl	%eax, %ecx
	movl	%ecx, 96(%rdi)
	cmpl	$64, %ecx
	jne	.LBB1_19
# %bb.6:                                #   in Loop: Header=BB1_2 Depth=1
	movl	$8, %ecx
	.p2align	4
.LBB1_7:                                #   Parent Loop BB1_2 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movl	(%rdi,%rcx,4), %r8d
	bswapl	%r8d
	movl	%r8d, -416(%rbp,%rcx,4)
	incq	%rcx
	cmpq	$24, %rcx
	jne	.LBB1_7
# %bb.8:                                #   in Loop: Header=BB1_2 Depth=1
	movl	$16, %ecx
	.p2align	4
.LBB1_9:                                #   Parent Loop BB1_2 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movl	-416(%rbp,%rcx,4), %r8d
	xorl	-396(%rbp,%rcx,4), %r8d
	xorl	-440(%rbp,%rcx,4), %r8d
	xorl	-448(%rbp,%rcx,4), %r8d
	roll	%r8d
	movl	%r8d, -384(%rbp,%rcx,4)
	incq	%rcx
	cmpq	$80, %rcx
	jne	.LBB1_9
# %bb.10:                               #   in Loop: Header=BB1_2 Depth=1
	movl	(%rdi), %r11d
	movl	4(%rdi), %r10d
	movl	8(%rdi), %r15d
	movl	12(%rdi), %r14d
	movl	16(%rdi), %ebx
	xorl	%ecx, %ecx
	movl	%r11d, %r13d
	movl	%r10d, %r12d
	movl	%r15d, -44(%rbp)                # 4-byte Spill
	movl	%r14d, -48(%rbp)                # 4-byte Spill
	movl	%ebx, -52(%rbp)                 # 4-byte Spill
	.p2align	4
.LBB1_11:                               #   Parent Loop BB1_2 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movl	%ebx, %r8d
	movl	%r14d, %ebx
	movl	%r15d, %r14d
	movl	%r12d, %r15d
	movl	%r13d, %r12d
	movl	%r14d, %r13d
	xorl	%ebx, %r13d
	andl	%r15d, %r13d
	xorl	%ebx, %r13d
	movl	%r12d, %r9d
	roll	$5, %r9d
	addl	%r8d, %r9d
	addl	%r13d, %r9d
	movl	-384(%rbp,%rcx,4), %r8d
	leal	(%r8,%r9), %r13d
	addl	$1518500249, %r13d              # imm = 0x5A827999
	roll	$30, %r15d
	incq	%rcx
	cmpq	$20, %rcx
	jne	.LBB1_11
# %bb.12:                               #   in Loop: Header=BB1_2 Depth=1
	movl	$20, %ecx
	.p2align	4
.LBB1_13:                               #   Parent Loop BB1_2 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movl	%ebx, %r8d
	movl	%r14d, %ebx
	movl	%r15d, %r14d
	movl	%r12d, %r15d
	movl	%r13d, %r12d
	movl	%r14d, %r9d
	xorl	%r15d, %r9d
	xorl	%ebx, %r9d
	roll	$5, %r13d
	addl	%r8d, %r13d
	addl	%r9d, %r13d
	movl	-384(%rbp,%rcx,4), %r8d
	addl	%r8d, %r13d
	addl	$1859775393, %r13d              # imm = 0x6ED9EBA1
	roll	$30, %r15d
	incq	%rcx
	cmpq	$40, %rcx
	jne	.LBB1_13
# %bb.14:                               #   in Loop: Header=BB1_2 Depth=1
	movl	$40, %ecx
	.p2align	4
.LBB1_15:                               #   Parent Loop BB1_2 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movl	%ebx, %r8d
	movl	%r14d, %ebx
	movl	%r15d, %r14d
	movl	%r12d, %r15d
	movl	%r13d, %r12d
	movl	%ebx, %r9d
	orl	%r14d, %r9d
	andl	%r15d, %r9d
	movl	%ebx, %r13d
	andl	%r14d, %r13d
	orl	%r9d, %r13d
	movl	%r12d, %r9d
	roll	$5, %r9d
	addl	%r8d, %r9d
	addl	-384(%rbp,%rcx,4), %r9d
	addl	%r9d, %r13d
	addl	$-1894007588, %r13d             # imm = 0x8F1BBCDC
	roll	$30, %r15d
	incq	%rcx
	cmpq	$60, %rcx
	jne	.LBB1_15
# %bb.16:                               #   in Loop: Header=BB1_2 Depth=1
	movl	$60, %ecx
	.p2align	4
.LBB1_17:                               #   Parent Loop BB1_2 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movl	%ebx, %r8d
	movl	%r14d, %ebx
	movl	%r15d, %r14d
	movl	%r12d, %r15d
	movl	%r13d, %r12d
	movl	%r14d, %r9d
	xorl	%r15d, %r9d
	xorl	%ebx, %r9d
	roll	$5, %r13d
	addl	%r8d, %r13d
	addl	%r9d, %r13d
	movl	-384(%rbp,%rcx,4), %r8d
	addl	%r8d, %r13d
	addl	$-899497514, %r13d              # imm = 0xCA62C1D6
	roll	$30, %r15d
	incq	%rcx
	cmpq	$80, %rcx
	jne	.LBB1_17
# %bb.18:                               #   in Loop: Header=BB1_2 Depth=1
	addl	%r11d, %r13d
	movl	%r13d, (%rdi)
	addl	%r10d, %r12d
	movl	%r12d, 4(%rdi)
	addl	-44(%rbp), %r15d                # 4-byte Folded Reload
	movl	%r15d, 8(%rdi)
	addl	-48(%rbp), %r14d                # 4-byte Folded Reload
	movl	%r14d, 12(%rdi)
	addl	-52(%rbp), %ebx                 # 4-byte Folded Reload
	movl	%ebx, 16(%rdi)
	movl	$0, 96(%rdi)
	jmp	.LBB1_19
.LBB1_20:
	addq	$216, %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
.LBB1_21:
	retq
.Lfunc_end1:
	.size	sha1_update, .Lfunc_end1-sha1_update
                                        # -- End function
	.globl	sha1_final                      # -- Begin function sha1_final
	.p2align	4
	.type	sha1_final,@function
sha1_final:                             # @sha1_final
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%r15
	pushq	%r14
	pushq	%r12
	pushq	%rbx
	subq	$16, %rsp
	movq	%rsi, %rbx
	movq	%rdi, %r14
	movq	24(%rdi), %r12
	movb	$-128, -34(%rbp)
	leaq	-34(%rbp), %rsi
	movl	$1, %edx
	callq	sha1_update
	movb	$0, -33(%rbp)
	cmpl	$56, 96(%r14)
	je	.LBB2_3
# %bb.1:
	leaq	-33(%rbp), %r15
	.p2align	4
.LBB2_2:                                # =>This Inner Loop Header: Depth=1
	movl	$1, %edx
	movq	%r14, %rdi
	movq	%r15, %rsi
	callq	sha1_update
	cmpl	$56, 96(%r14)
	jne	.LBB2_2
.LBB2_3:
	movl	$56, %ecx
	leaq	-42(%rbp), %rax
	.p2align	4
.LBB2_4:                                # =>This Inner Loop Header: Depth=1
	movq	%r12, %rdx
	shrq	%cl, %rdx
	movb	%dl, (%rax)
	addq	$-8, %rcx
	incq	%rax
	cmpq	$-8, %rcx
	jne	.LBB2_4
# %bb.5:
	leaq	-42(%rbp), %rsi
	movl	$8, %edx
	movq	%r14, %rdi
	callq	sha1_update
	xorl	%eax, %eax
	.p2align	4
.LBB2_6:                                # =>This Inner Loop Header: Depth=1
	movzbl	3(%r14,%rax,4), %ecx
	movb	%cl, (%rbx,%rax,4)
	movzbl	2(%r14,%rax,4), %ecx
	movb	%cl, 1(%rbx,%rax,4)
	movzbl	1(%r14,%rax,4), %ecx
	movb	%cl, 2(%rbx,%rax,4)
	movzbl	(%r14,%rax,4), %ecx
	movb	%cl, 3(%rbx,%rax,4)
	incq	%rax
	cmpq	$5, %rax
	jne	.LBB2_6
# %bb.7:
	addq	$16, %rsp
	popq	%rbx
	popq	%r12
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
.Lfunc_end2:
	.size	sha1_final, .Lfunc_end2-sha1_final
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
