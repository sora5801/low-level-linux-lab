	.file	"demo.c"
	.text
	.globl	sha1_block                      # -- Begin function sha1_block
	.p2align	4
	.type	sha1_block,@function
sha1_block:                             # @sha1_block
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	subq	$200, %rsp
	xorl	%eax, %eax
	.p2align	4
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	movl	(%rsi,%rax,4), %ecx
	bswapl	%ecx
	movl	%ecx, -368(%rbp,%rax,4)
	incq	%rax
	cmpq	$16, %rax
	jne	.LBB0_1
# %bb.2:
	movl	$16, %eax
	.p2align	4
.LBB0_3:                                # =>This Inner Loop Header: Depth=1
	movl	-400(%rbp,%rax,4), %ecx
	xorl	-380(%rbp,%rax,4), %ecx
	xorl	-424(%rbp,%rax,4), %ecx
	xorl	-432(%rbp,%rax,4), %ecx
	roll	%ecx
	movl	%ecx, -368(%rbp,%rax,4)
	incq	%rax
	cmpq	$80, %rax
	jne	.LBB0_3
# %bb.4:
	movl	(%rdi), %r8d
	movl	4(%rdi), %esi
	movl	8(%rdi), %edx
	movl	12(%rdi), %ecx
	movl	16(%rdi), %eax
	xorl	%r15d, %r15d
	movl	%r8d, %r14d
	movl	%esi, %ebx
	movl	%edx, %r11d
	movl	%ecx, %r10d
	movl	%eax, %r9d
	.p2align	4
.LBB0_5:                                # =>This Inner Loop Header: Depth=1
	movl	%r9d, %r12d
	movl	%r10d, %r9d
	movl	%r11d, %r10d
	movl	%ebx, %r11d
	movl	%r14d, %ebx
	movl	%r10d, %r14d
	xorl	%r9d, %r14d
	andl	%r11d, %r14d
	xorl	%r9d, %r14d
	movl	%ebx, %r13d
	roll	$5, %r13d
	addl	%r12d, %r13d
	addl	%r14d, %r13d
	movl	-368(%rbp,%r15,4), %r14d
	addl	%r13d, %r14d
	addl	$1518500249, %r14d              # imm = 0x5A827999
	roll	$30, %r11d
	incq	%r15
	cmpq	$20, %r15
	jne	.LBB0_5
# %bb.6:
	movl	$20, %r15d
	.p2align	4
.LBB0_7:                                # =>This Inner Loop Header: Depth=1
	movl	%r9d, %r12d
	movl	%r10d, %r9d
	movl	%r11d, %r10d
	movl	%ebx, %r11d
	movl	%r14d, %ebx
	movl	%r10d, %r14d
	xorl	%r9d, %r14d
	xorl	%r11d, %r14d
	movl	%ebx, %r13d
	roll	$5, %r13d
	addl	%r12d, %r14d
	addl	%r14d, %r13d
	movl	-368(%rbp,%r15,4), %r14d
	addl	%r13d, %r14d
	addl	$1859775393, %r14d              # imm = 0x6ED9EBA1
	roll	$30, %r11d
	incq	%r15
	cmpq	$40, %r15
	jne	.LBB0_7
# %bb.8:
	movl	$40, %r15d
	.p2align	4
.LBB0_9:                                # =>This Inner Loop Header: Depth=1
	movl	%r9d, %r12d
	movl	%r10d, %r9d
	movl	%r11d, %r10d
	movl	%ebx, %r11d
	movl	%r14d, %ebx
	movl	%r10d, %r14d
	orl	%r9d, %r14d
	andl	%r11d, %r14d
	movl	%r10d, %r13d
	andl	%r9d, %r13d
	orl	%r14d, %r13d
	movl	%ebx, %r14d
	roll	$5, %r14d
	addl	%r12d, %r14d
	addl	%r13d, %r14d
	movl	-368(%rbp,%r15,4), %r12d
	addl	%r12d, %r14d
	addl	$-1894007588, %r14d             # imm = 0x8F1BBCDC
	roll	$30, %r11d
	incq	%r15
	cmpq	$60, %r15
	jne	.LBB0_9
# %bb.10:
	movl	$60, %r15d
	.p2align	4
.LBB0_11:                               # =>This Inner Loop Header: Depth=1
	movl	%r9d, %r12d
	movl	%r10d, %r9d
	movl	%r11d, %r10d
	movl	%ebx, %r11d
	movl	%r14d, %ebx
	movl	%r10d, %r14d
	xorl	%r9d, %r14d
	xorl	%r11d, %r14d
	movl	%ebx, %r13d
	roll	$5, %r13d
	addl	%r12d, %r14d
	addl	%r14d, %r13d
	movl	-368(%rbp,%r15,4), %r14d
	addl	%r13d, %r14d
	addl	$-899497514, %r14d              # imm = 0xCA62C1D6
	roll	$30, %r11d
	incq	%r15
	cmpq	$80, %r15
	jne	.LBB0_11
# %bb.12:
	addl	%r8d, %r14d
	movl	%r14d, (%rdi)
	addl	%esi, %ebx
	movl	%ebx, 4(%rdi)
	addl	%edx, %r11d
	movl	%r11d, 8(%rdi)
	addl	%ecx, %r10d
	movl	%r10d, 12(%rdi)
	addl	%eax, %r9d
	movl	%r9d, 16(%rdi)
	addq	$200, %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
.Lfunc_end0:
	.size	sha1_block, .Lfunc_end0-sha1_block
                                        # -- End function
	.globl	hex_encode                      # -- Begin function hex_encode
	.p2align	4
	.type	hex_encode,@function
hex_encode:                             # @hex_encode
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	testl	%esi, %esi
	jle	.LBB1_3
# %bb.1:
	movl	%esi, %eax
	xorl	%ecx, %ecx
	leaq	hex_encode.d(%rip), %r8
	.p2align	4
.LBB1_2:                                # =>This Inner Loop Header: Depth=1
	movzbl	(%rdi,%rcx), %r9d
	shrl	$4, %r9d
	movzbl	(%r9,%r8), %r9d
	movb	%r9b, (%rdx,%rcx,2)
	movzbl	(%rdi,%rcx), %r9d
	andl	$15, %r9d
	movzbl	(%r9,%r8), %r9d
	movb	%r9b, 1(%rdx,%rcx,2)
	incq	%rcx
	cmpq	%rcx, %rax
	jne	.LBB1_2
.LBB1_3:
	movslq	%esi, %rax
	movb	$0, (%rdx,%rax,2)
	popq	%rbp
	retq
.Lfunc_end1:
	.size	hex_encode, .Lfunc_end1-hex_encode
                                        # -- End function
	.globl	demo_run                        # -- Begin function demo_run
	.p2align	4
	.type	demo_run,@function
demo_run:                               # @demo_run
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$176, %rsp
	xorps	%xmm0, %xmm0
	movaps	%xmm0, -96(%rbp)
	movaps	%xmm0, -48(%rbp)
	movaps	%xmm0, -64(%rbp)
	movaps	%xmm0, -80(%rbp)
	movl	$-2140970399, -96(%rbp)         # imm = 0x80636261
	movb	$24, -33(%rbp)
	movaps	.L__const.demo_run.h(%rip), %xmm0
	movaps	%xmm0, -32(%rbp)
	movl	$-1009589776, -16(%rbp)         # imm = 0xC3D2E1F0
	leaq	-32(%rbp), %rdi
	leaq	-96(%rbp), %rsi
	callq	sha1_block
	xorl	%eax, %eax
	.p2align	4
.LBB2_1:                                # =>This Inner Loop Header: Depth=1
	movl	-32(%rbp,%rax,4), %ecx
	bswapl	%ecx
	movl	%ecx, -128(%rbp,%rax,4)
	incq	%rax
	cmpq	$5, %rax
	jne	.LBB2_1
# %bb.2:
	xorl	%eax, %eax
	leaq	hex_encode.d(%rip), %rcx
	.p2align	4
.LBB2_3:                                # =>This Inner Loop Header: Depth=1
	movzbl	-128(%rbp,%rax), %edx
	movl	%edx, %esi
	shrl	$4, %esi
	movzbl	(%rsi,%rcx), %esi
	movb	%sil, -176(%rbp,%rax,2)
	andl	$15, %edx
	movzbl	(%rdx,%rcx), %edx
	movb	%dl, -175(%rbp,%rax,2)
	incq	%rax
	cmpq	$20, %rax
	jne	.LBB2_3
# %bb.4:
	movzbl	-176(%rbp), %eax
	addq	$176, %rsp
	popq	%rbp
	retq
.Lfunc_end2:
	.size	demo_run, .Lfunc_end2-demo_run
                                        # -- End function
	.type	hex_encode.d,@object            # @hex_encode.d
	.section	.rodata.cst16,"aM",@progbits,16
	.p2align	4, 0x0
hex_encode.d:
	.ascii	"0123456789abcdef"
	.size	hex_encode.d, 16

	.type	.L__const.demo_run.h,@object    # @__const.demo_run.h
	.section	.rodata,"a",@progbits
	.p2align	4, 0x0
.L__const.demo_run.h:
	.long	1732584193                      # 0x67452301
	.long	4023233417                      # 0xefcdab89
	.long	2562383102                      # 0x98badcfe
	.long	271733878                       # 0x10325476
	.long	3285377520                      # 0xc3d2e1f0
	.size	.L__const.demo_run.h, 20

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
