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
	movaps	.LCPI0_0(%rip), %xmm0           # xmm0 = [1732584193,4023233417,2562383102,271733878]
	movups	%xmm0, (%rdi)
	movl	$-1009589776, 16(%rdi)          # imm = 0xC3D2E1F0
	movq	$0, 24(%rdi)
	movl	$0, 96(%rdi)
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
	je	.LBB1_24
# %bb.1:
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	subq	$216, %rsp
	movl	96(%rdi), %ecx
	jmp	.LBB1_2
	.p2align	4
.LBB1_22:                               #   in Loop: Header=BB1_2 Depth=1
	addq	%rax, %rsi
	subq	%rax, %rdx
	je	.LBB1_23
.LBB1_2:                                # =>This Loop Header: Depth=1
                                        #     Child Loop BB1_26 Depth 2
                                        #     Child Loop BB1_6 Depth 2
                                        #     Child Loop BB1_10 Depth 2
                                        #     Child Loop BB1_12 Depth 2
                                        #     Child Loop BB1_14 Depth 2
                                        #     Child Loop BB1_16 Depth 2
                                        #     Child Loop BB1_18 Depth 2
                                        #     Child Loop BB1_20 Depth 2
	leal	-64(%rcx), %eax
	negl	%eax
	cmpq	%rax, %rdx
	cmovbq	%rdx, %rax
	cmpl	$64, %ecx
	movl	$64, %ecx
	je	.LBB1_8
# %bb.3:                                #   in Loop: Header=BB1_2 Depth=1
	cmpl	$1, %eax
	movl	%eax, %ecx
	adcl	$0, %ecx
	cmpq	$4, %rax
	jae	.LBB1_25
# %bb.4:                                #   in Loop: Header=BB1_2 Depth=1
	xorl	%r8d, %r8d
	jmp	.LBB1_5
	.p2align	4
.LBB1_25:                               #   in Loop: Header=BB1_2 Depth=1
	movl	%ecx, %r9d
	andl	$-4, %r9d
	xorl	%r8d, %r8d
	.p2align	4
.LBB1_26:                               #   Parent Loop BB1_2 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movzbl	(%rsi,%r8), %r10d
	movl	96(%rdi), %r11d
	addl	%r8d, %r11d
	movb	%r10b, 32(%rdi,%r11)
	movzbl	1(%rsi,%r8), %r10d
	movl	96(%rdi), %r11d
	addl	%r8d, %r11d
	incl	%r11d
	movb	%r10b, 32(%rdi,%r11)
	movzbl	2(%rsi,%r8), %r10d
	movl	96(%rdi), %r11d
	leal	2(%r8,%r11), %r11d
	movb	%r10b, 32(%rdi,%r11)
	movzbl	3(%rsi,%r8), %r10d
	movl	96(%rdi), %r11d
	addl	%r8d, %r11d
	addl	$3, %r11d
	movb	%r10b, 32(%rdi,%r11)
	addq	$4, %r8
	cmpq	%r8, %r9
	jne	.LBB1_26
.LBB1_5:                                #   in Loop: Header=BB1_2 Depth=1
	andl	$3, %ecx
	je	.LBB1_7
	.p2align	4
.LBB1_6:                                #   Parent Loop BB1_2 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movzbl	(%rsi,%r8), %r9d
	movl	96(%rdi), %r10d
	addl	%r8d, %r10d
	movb	%r9b, 32(%rdi,%r10)
	incq	%r8
	decq	%rcx
	jne	.LBB1_6
.LBB1_7:                                #   in Loop: Header=BB1_2 Depth=1
	movl	96(%rdi), %ecx
.LBB1_8:                                #   in Loop: Header=BB1_2 Depth=1
	addl	%eax, %ecx
	movl	%ecx, 96(%rdi)
	cmpl	$64, %ecx
	jne	.LBB1_22
# %bb.9:                                #   in Loop: Header=BB1_2 Depth=1
	movl	$8, %ecx
	.p2align	4
.LBB1_10:                               #   Parent Loop BB1_2 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movl	(%rdi,%rcx,4), %r8d
	bswapl	%r8d
	movl	%r8d, -144(%rsp,%rcx,4)
	incq	%rcx
	cmpq	$24, %rcx
	jne	.LBB1_10
# %bb.11:                               #   in Loop: Header=BB1_2 Depth=1
	movl	$16, %ecx
	.p2align	4
.LBB1_12:                               #   Parent Loop BB1_2 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movl	-144(%rsp,%rcx,4), %r8d
	xorl	-124(%rsp,%rcx,4), %r8d
	xorl	-168(%rsp,%rcx,4), %r8d
	xorl	-176(%rsp,%rcx,4), %r8d
	roll	%r8d
	movl	%r8d, -112(%rsp,%rcx,4)
	incq	%rcx
	cmpq	$80, %rcx
	jne	.LBB1_12
# %bb.13:                               #   in Loop: Header=BB1_2 Depth=1
	movl	(%rdi), %r11d
	movl	4(%rdi), %r10d
	movl	8(%rdi), %r14d
	movl	12(%rdi), %ebp
	movl	16(%rdi), %ebx
	movl	$1, %r13d
	movl	%r11d, %r12d
	movl	%r10d, %r15d
	movl	%r14d, -124(%rsp)               # 4-byte Spill
	movl	%ebp, -120(%rsp)                # 4-byte Spill
	movl	%ebx, -116(%rsp)                # 4-byte Spill
	.p2align	4
.LBB1_14:                               #   Parent Loop BB1_2 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movl	%ebx, %r8d
	movl	%ebp, %ecx
	movl	%r14d, %ebx
	movl	%r15d, %ebp
	movl	%r12d, %r14d
	movl	%ebx, %r15d
	xorl	%ecx, %r15d
	andl	%ebp, %r15d
	xorl	%ecx, %r15d
	roll	$5, %r12d
	addl	%r8d, %r12d
	addl	%r15d, %r12d
	movl	-116(%rsp,%r13,4), %r8d
	movl	-112(%rsp,%r13,4), %r9d
	leal	(%r8,%r12), %r15d
	addl	$1518500249, %r15d              # imm = 0x5A827999
	roll	$30, %ebp
	movl	%ebp, %r8d
	xorl	%ebx, %r8d
	andl	%r14d, %r8d
	movl	%r15d, %r12d
	roll	$5, %r12d
	xorl	%ebx, %r8d
	addl	%ecx, %r8d
	addl	%r12d, %r8d
	leal	(%r9,%r8), %r12d
	addl	$1518500249, %r12d              # imm = 0x5A827999
	roll	$30, %r14d
	addq	$2, %r13
	cmpq	$21, %r13
	jne	.LBB1_14
# %bb.15:                               #   in Loop: Header=BB1_2 Depth=1
	movl	$21, %r13d
	.p2align	4
.LBB1_16:                               #   Parent Loop BB1_2 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movl	%ebx, %ecx
	movl	%ebp, %r8d
	movl	%r14d, %ebx
	movl	%r15d, %ebp
	movl	%r12d, %r14d
	movl	%ebx, %r9d
	xorl	%r15d, %r9d
	xorl	%r8d, %r9d
	movl	%r12d, %r15d
	roll	$5, %r15d
	addl	%ecx, %r15d
	addl	%r9d, %r15d
	movl	-116(%rsp,%r13,4), %ecx
	movl	-112(%rsp,%r13,4), %r9d
	addl	%ecx, %r15d
	addl	$1859775393, %r15d              # imm = 0x6ED9EBA1
	roll	$30, %ebp
	movl	%r12d, %ecx
	xorl	%ebx, %ecx
	movl	%r15d, %r12d
	roll	$5, %r12d
	xorl	%ebp, %ecx
	addl	%r8d, %ecx
	addl	%r12d, %ecx
	leal	(%r9,%rcx), %r12d
	addl	$1859775393, %r12d              # imm = 0x6ED9EBA1
	roll	$30, %r14d
	addq	$2, %r13
	cmpq	$41, %r13
	jne	.LBB1_16
# %bb.17:                               #   in Loop: Header=BB1_2 Depth=1
	movl	$41, %r13d
	.p2align	4
.LBB1_18:                               #   Parent Loop BB1_2 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movl	%ebx, %r8d
	movl	%ebp, %ecx
	movl	%r14d, %ebx
	movl	%r15d, %ebp
	movl	%r12d, %r14d
	movl	%ecx, %r9d
	orl	%ebx, %r9d
	andl	%r15d, %r9d
	movl	%ecx, %r15d
	andl	%ebx, %r15d
	roll	$5, %r12d
	orl	%r9d, %r15d
	addl	%r8d, %r12d
	addl	-116(%rsp,%r13,4), %r12d
	addl	%r12d, %r15d
	addl	$-1894007588, %r15d             # imm = 0x8F1BBCDC
	roll	$30, %ebp
	movl	%ebx, %r8d
	orl	%ebp, %r8d
	andl	%r14d, %r8d
	movl	%ebx, %r9d
	andl	%ebp, %r9d
	orl	%r8d, %r9d
	movl	%r15d, %r8d
	roll	$5, %r8d
	addl	%ecx, %r8d
	addl	-112(%rsp,%r13,4), %r8d
	leal	(%r9,%r8), %r12d
	addl	$-1894007588, %r12d             # imm = 0x8F1BBCDC
	roll	$30, %r14d
	addq	$2, %r13
	cmpq	$61, %r13
	jne	.LBB1_18
# %bb.19:                               #   in Loop: Header=BB1_2 Depth=1
	movl	$61, %r13d
	.p2align	4
.LBB1_20:                               #   Parent Loop BB1_2 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movl	%ebx, %ecx
	movl	%ebp, %r8d
	movl	%r14d, %ebx
	movl	%r15d, %ebp
	movl	%r12d, %r14d
	movl	%ebx, %r9d
	xorl	%r15d, %r9d
	xorl	%r8d, %r9d
	movl	%r12d, %r15d
	roll	$5, %r15d
	addl	%ecx, %r15d
	addl	%r9d, %r15d
	movl	-116(%rsp,%r13,4), %ecx
	movl	-112(%rsp,%r13,4), %r9d
	addl	%ecx, %r15d
	addl	$-899497514, %r15d              # imm = 0xCA62C1D6
	roll	$30, %ebp
	movl	%r12d, %ecx
	xorl	%ebx, %ecx
	movl	%r15d, %r12d
	roll	$5, %r12d
	xorl	%ebp, %ecx
	addl	%r8d, %ecx
	addl	%r12d, %ecx
	leal	(%r9,%rcx), %r12d
	addl	$-899497514, %r12d              # imm = 0xCA62C1D6
	roll	$30, %r14d
	addq	$2, %r13
	cmpq	$81, %r13
	jne	.LBB1_20
# %bb.21:                               #   in Loop: Header=BB1_2 Depth=1
	addl	%r11d, %r12d
	movl	%r12d, (%rdi)
	addl	%r10d, %r15d
	movl	%r15d, 4(%rdi)
	addl	-124(%rsp), %r14d               # 4-byte Folded Reload
	movl	%r14d, 8(%rdi)
	addl	-120(%rsp), %ebp                # 4-byte Folded Reload
	movl	%ebp, 12(%rdi)
	addl	-116(%rsp), %ebx                # 4-byte Folded Reload
	movl	%ebx, 16(%rdi)
	movl	$0, 96(%rdi)
	xorl	%ecx, %ecx
	jmp	.LBB1_22
.LBB1_23:
	addq	$216, %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
.LBB1_24:
	retq
.Lfunc_end1:
	.size	sha1_update, .Lfunc_end1-sha1_update
                                        # -- End function
	.globl	sha1_final                      # -- Begin function sha1_final
	.p2align	4
	.type	sha1_final,@function
sha1_final:                             # @sha1_final
# %bb.0:
	pushq	%r15
	pushq	%r14
	pushq	%r12
	pushq	%rbx
	subq	$24, %rsp
	movq	%rsi, %rbx
	movq	%rdi, %r14
	movq	24(%rdi), %r12
	movb	$-128, 15(%rsp)
	leaq	15(%rsp), %rsi
	movl	$1, %edx
	callq	sha1_update
	movb	$0, 14(%rsp)
	cmpl	$56, 96(%r14)
	je	.LBB2_3
# %bb.1:
	leaq	14(%rsp), %r15
	.p2align	4
.LBB2_2:                                # =>This Inner Loop Header: Depth=1
	movl	$1, %edx
	movq	%r14, %rdi
	movq	%r15, %rsi
	callq	sha1_update
	cmpl	$56, 96(%r14)
	jne	.LBB2_2
.LBB2_3:
	bswapq	%r12
	movq	%r12, 16(%rsp)
	leaq	16(%rsp), %rsi
	movl	$8, %edx
	movq	%r14, %rdi
	callq	sha1_update
	movzbl	3(%r14), %eax
	movb	%al, (%rbx)
	movzbl	2(%r14), %eax
	movb	%al, 1(%rbx)
	movzbl	1(%r14), %eax
	movb	%al, 2(%rbx)
	movzbl	(%r14), %eax
	movb	%al, 3(%rbx)
	movzbl	7(%r14), %eax
	movb	%al, 4(%rbx)
	movzbl	6(%r14), %eax
	movb	%al, 5(%rbx)
	movzbl	5(%r14), %eax
	movb	%al, 6(%rbx)
	movzbl	4(%r14), %eax
	movb	%al, 7(%rbx)
	movzbl	11(%r14), %eax
	movb	%al, 8(%rbx)
	movzbl	10(%r14), %eax
	movb	%al, 9(%rbx)
	movzbl	9(%r14), %eax
	movb	%al, 10(%rbx)
	movzbl	8(%r14), %eax
	movb	%al, 11(%rbx)
	movzbl	15(%r14), %eax
	movb	%al, 12(%rbx)
	movzbl	14(%r14), %eax
	movb	%al, 13(%rbx)
	movzbl	13(%r14), %eax
	movb	%al, 14(%rbx)
	movzbl	12(%r14), %eax
	movb	%al, 15(%rbx)
	movzbl	19(%r14), %eax
	movb	%al, 16(%rbx)
	movzbl	18(%r14), %eax
	movb	%al, 17(%rbx)
	movzbl	17(%r14), %eax
	movb	%al, 18(%rbx)
	movzbl	16(%r14), %eax
	movb	%al, 19(%rbx)
	addq	$24, %rsp
	popq	%rbx
	popq	%r12
	popq	%r14
	popq	%r15
	retq
.Lfunc_end2:
	.size	sha1_final, .Lfunc_end2-sha1_final
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
