	.file	"demo.c"
	.text
	.globl	sha1_block                      # -- Begin function sha1_block
	.p2align	4
	.type	sha1_block,@function
sha1_block:                             # @sha1_block
# %bb.0:
	pushq	%rbp
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
	movl	%ecx, -128(%rsp,%rax,4)
	incq	%rax
	cmpq	$16, %rax
	jne	.LBB0_1
# %bb.2:
	movl	$16, %eax
	.p2align	4
.LBB0_3:                                # =>This Inner Loop Header: Depth=1
	movl	-160(%rsp,%rax,4), %ecx
	xorl	-140(%rsp,%rax,4), %ecx
	xorl	-184(%rsp,%rax,4), %ecx
	xorl	-192(%rsp,%rax,4), %ecx
	roll	%ecx
	movl	%ecx, -128(%rsp,%rax,4)
	incq	%rax
	cmpq	$80, %rax
	jne	.LBB0_3
# %bb.4:
	movl	(%rdi), %r8d
	movl	4(%rdi), %esi
	movl	8(%rdi), %edx
	movl	12(%rdi), %ecx
	movl	16(%rdi), %eax
	movl	$1, %r14d
	movl	%r8d, %ebp
	movl	%esi, %ebx
	movl	%edx, %r11d
	movl	%ecx, %r10d
	movl	%eax, %r9d
	.p2align	4
.LBB0_5:                                # =>This Inner Loop Header: Depth=1
	movl	%r9d, %r12d
	movl	%r10d, %r15d
	movl	%r11d, %r9d
	movl	%ebx, %r10d
	movl	%ebp, %r11d
	movl	%r9d, %ebx
	xorl	%r15d, %ebx
	andl	%r10d, %ebx
	xorl	%r15d, %ebx
	movl	%ebp, %r13d
	roll	$5, %r13d
	addl	%r12d, %r13d
	addl	%ebx, %r13d
	movl	-132(%rsp,%r14,4), %ebx
	movl	-128(%rsp,%r14,4), %r12d
	addl	%r13d, %ebx
	addl	$1518500249, %ebx               # imm = 0x5A827999
	roll	$30, %r10d
	movl	%r10d, %r13d
	xorl	%r9d, %r13d
	andl	%ebp, %r13d
	movl	%ebx, %ebp
	roll	$5, %ebp
	xorl	%r9d, %r13d
	addl	%r15d, %r13d
	addl	%ebp, %r13d
	leal	(%r12,%r13), %ebp
	addl	$1518500249, %ebp               # imm = 0x5A827999
	roll	$30, %r11d
	addq	$2, %r14
	cmpq	$21, %r14
	jne	.LBB0_5
# %bb.6:
	movl	$21, %r14d
	.p2align	4
.LBB0_7:                                # =>This Inner Loop Header: Depth=1
	movl	%r9d, %r12d
	movl	%r10d, %r15d
	movl	%r11d, %r9d
	movl	%ebx, %r10d
	movl	%ebp, %r11d
	movl	%r9d, %ebx
	xorl	%r15d, %ebx
	xorl	%r10d, %ebx
	movl	%ebp, %r13d
	roll	$5, %r13d
	addl	%r12d, %ebx
	addl	%ebx, %r13d
	movl	-132(%rsp,%r14,4), %ebx
	movl	-128(%rsp,%r14,4), %r12d
	addl	%r13d, %ebx
	addl	$1859775393, %ebx               # imm = 0x6ED9EBA1
	roll	$30, %r10d
	movl	%r9d, %r13d
	xorl	%ebp, %r13d
	movl	%ebx, %ebp
	roll	$5, %ebp
	xorl	%r10d, %r13d
	addl	%r15d, %r13d
	addl	%ebp, %r13d
	leal	(%r12,%r13), %ebp
	addl	$1859775393, %ebp               # imm = 0x6ED9EBA1
	roll	$30, %r11d
	addq	$2, %r14
	cmpq	$41, %r14
	jne	.LBB0_7
# %bb.8:
	movl	$41, %r14d
	.p2align	4
.LBB0_9:                                # =>This Inner Loop Header: Depth=1
	movl	%r9d, %r12d
	movl	%r10d, %r15d
	movl	%r11d, %r9d
	movl	%ebx, %r10d
	movl	%ebp, %r11d
	movl	%r9d, %ebx
	orl	%r15d, %ebx
	andl	%r10d, %ebx
	movl	%r9d, %ebp
	andl	%r15d, %ebp
	movl	%r11d, %r13d
	roll	$5, %r13d
	orl	%ebx, %ebp
	addl	%r12d, %r13d
	addl	%ebp, %r13d
	movl	-132(%rsp,%r14,4), %ebx
	addl	%r13d, %ebx
	addl	$-1894007588, %ebx              # imm = 0x8F1BBCDC
	roll	$30, %r10d
	movl	%r10d, %ebp
	orl	%r9d, %ebp
	andl	%r11d, %ebp
	movl	%r10d, %r12d
	andl	%r9d, %r12d
	orl	%ebp, %r12d
	movl	%ebx, %ebp
	roll	$5, %ebp
	movl	-128(%rsp,%r14,4), %r13d
	addl	%r15d, %r12d
	addl	%ebp, %r12d
	leal	(%r12,%r13), %ebp
	addl	$-1894007588, %ebp              # imm = 0x8F1BBCDC
	roll	$30, %r11d
	addq	$2, %r14
	cmpq	$61, %r14
	jne	.LBB0_9
# %bb.10:
	movl	$61, %r14d
	.p2align	4
.LBB0_11:                               # =>This Inner Loop Header: Depth=1
	movl	%r9d, %r12d
	movl	%r10d, %r15d
	movl	%r11d, %r9d
	movl	%ebx, %r10d
	movl	%ebp, %r11d
	movl	%r9d, %ebx
	xorl	%r15d, %ebx
	xorl	%r10d, %ebx
	movl	%ebp, %r13d
	roll	$5, %r13d
	addl	%r12d, %ebx
	addl	%ebx, %r13d
	movl	-132(%rsp,%r14,4), %ebx
	movl	-128(%rsp,%r14,4), %r12d
	addl	%r13d, %ebx
	addl	$-899497514, %ebx               # imm = 0xCA62C1D6
	roll	$30, %r10d
	movl	%r9d, %r13d
	xorl	%ebp, %r13d
	movl	%ebx, %ebp
	roll	$5, %ebp
	xorl	%r10d, %r13d
	addl	%r15d, %r13d
	addl	%ebp, %r13d
	leal	(%r12,%r13), %ebp
	addl	$-899497514, %ebp               # imm = 0xCA62C1D6
	roll	$30, %r11d
	addq	$2, %r14
	cmpq	$81, %r14
	jne	.LBB0_11
# %bb.12:
	addl	%r8d, %ebp
	movl	%ebp, (%rdi)
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
	retq
.Lfunc_end1:
	.size	hex_encode, .Lfunc_end1-hex_encode
                                        # -- End function
	.globl	demo_run                        # -- Begin function demo_run
	.p2align	4
	.type	demo_run,@function
demo_run:                               # @demo_run
# %bb.0:
	subq	$184, %rsp
	xorps	%xmm0, %xmm0
	movaps	%xmm0, 64(%rsp)
	movaps	%xmm0, 112(%rsp)
	movaps	%xmm0, 96(%rsp)
	movaps	%xmm0, 80(%rsp)
	movl	$-2140970399, 64(%rsp)          # imm = 0x80636261
	movb	$24, 127(%rsp)
	movaps	.L__const.demo_run.h(%rip), %xmm0
	movaps	%xmm0, (%rsp)
	movl	$-1009589776, 16(%rsp)          # imm = 0xC3D2E1F0
	movq	%rsp, %rdi
	leaq	64(%rsp), %rsi
	callq	sha1_block
	movl	(%rsp), %eax
	movl	4(%rsp), %ecx
	bswapl	%eax
	movl	%eax, 32(%rsp)
	bswapl	%ecx
	movl	%ecx, 36(%rsp)
	movl	8(%rsp), %eax
	bswapl	%eax
	movl	%eax, 40(%rsp)
	movl	12(%rsp), %eax
	bswapl	%eax
	movl	%eax, 44(%rsp)
	movl	16(%rsp), %eax
	bswapl	%eax
	movl	%eax, 48(%rsp)
	xorl	%eax, %eax
	leaq	hex_encode.d(%rip), %rcx
	.p2align	4
.LBB2_1:                                # =>This Inner Loop Header: Depth=1
	movzbl	32(%rsp,%rax), %edx
	movl	%edx, %esi
	shrl	$4, %esi
	movzbl	(%rsi,%rcx), %esi
	movb	%sil, 128(%rsp,%rax,2)
	andl	$15, %edx
	movzbl	(%rdx,%rcx), %edx
	movb	%dl, 129(%rsp,%rax,2)
	incq	%rax
	cmpq	$20, %rax
	jne	.LBB2_1
# %bb.2:
	movzbl	128(%rsp), %eax
	addq	$184, %rsp
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
