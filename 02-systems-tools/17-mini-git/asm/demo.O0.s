	.file	"demo.c"
	.text
	.globl	sha1_block                      # -- Begin function sha1_block
	.p2align	4
	.type	sha1_block,@function
sha1_block:                             # @sha1_block
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$272, %rsp                      # imm = 0x110
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movl	$0, -340(%rbp)
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	cmpl	$16, -340(%rbp)
	jge	.LBB0_4
# %bb.2:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rax
	movl	-340(%rbp), %ecx
	shll	$2, %ecx
	addl	$0, %ecx
	movslq	%ecx, %rcx
	movzbl	(%rax,%rcx), %ecx
	shll	$24, %ecx
	movq	-16(%rbp), %rax
	movl	-340(%rbp), %edx
	shll	$2, %edx
	addl	$1, %edx
	movslq	%edx, %rdx
	movzbl	(%rax,%rdx), %eax
	shll	$16, %eax
	orl	%eax, %ecx
	movq	-16(%rbp), %rax
	movl	-340(%rbp), %edx
	shll	$2, %edx
	addl	$2, %edx
	movslq	%edx, %rdx
	movzbl	(%rax,%rdx), %eax
	shll	$8, %eax
	orl	%eax, %ecx
	movq	-16(%rbp), %rax
	movl	-340(%rbp), %edx
	shll	$2, %edx
	addl	$3, %edx
	movslq	%edx, %rdx
	movzbl	(%rax,%rdx), %eax
	shll	$0, %eax
	orl	%eax, %ecx
	movslq	-340(%rbp), %rax
	movl	%ecx, -336(%rbp,%rax,4)
# %bb.3:                                #   in Loop: Header=BB0_1 Depth=1
	movl	-340(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -340(%rbp)
	jmp	.LBB0_1
.LBB0_4:
	movl	$16, -340(%rbp)
.LBB0_5:                                # =>This Inner Loop Header: Depth=1
	cmpl	$80, -340(%rbp)
	jge	.LBB0_8
# %bb.6:                                #   in Loop: Header=BB0_5 Depth=1
	movl	-340(%rbp), %eax
	subl	$3, %eax
	cltq
	movl	-336(%rbp,%rax,4), %ecx
	movl	-340(%rbp), %eax
	subl	$8, %eax
	cltq
	xorl	-336(%rbp,%rax,4), %ecx
	movl	-340(%rbp), %eax
	subl	$14, %eax
	cltq
	xorl	-336(%rbp,%rax,4), %ecx
	movl	-340(%rbp), %eax
	subl	$16, %eax
	cltq
	xorl	-336(%rbp,%rax,4), %ecx
	shll	%ecx
	movl	-340(%rbp), %eax
	subl	$3, %eax
	cltq
	movl	-336(%rbp,%rax,4), %eax
	movl	-340(%rbp), %edx
	subl	$8, %edx
	movslq	%edx, %rdx
	xorl	-336(%rbp,%rdx,4), %eax
	movl	-340(%rbp), %edx
	subl	$14, %edx
	movslq	%edx, %rdx
	xorl	-336(%rbp,%rdx,4), %eax
	movl	-340(%rbp), %edx
	subl	$16, %edx
	movslq	%edx, %rdx
	xorl	-336(%rbp,%rdx,4), %eax
	shrl	$31, %eax
	orl	%eax, %ecx
	movslq	-340(%rbp), %rax
	movl	%ecx, -336(%rbp,%rax,4)
# %bb.7:                                #   in Loop: Header=BB0_5 Depth=1
	movl	-340(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -340(%rbp)
	jmp	.LBB0_5
.LBB0_8:
	movq	-8(%rbp), %rax
	movl	(%rax), %eax
	movl	%eax, -344(%rbp)
	movq	-8(%rbp), %rax
	movl	4(%rax), %eax
	movl	%eax, -348(%rbp)
	movq	-8(%rbp), %rax
	movl	8(%rax), %eax
	movl	%eax, -352(%rbp)
	movq	-8(%rbp), %rax
	movl	12(%rax), %eax
	movl	%eax, -356(%rbp)
	movq	-8(%rbp), %rax
	movl	16(%rax), %eax
	movl	%eax, -360(%rbp)
	movl	$0, -340(%rbp)
.LBB0_9:                                # =>This Inner Loop Header: Depth=1
	cmpl	$20, -340(%rbp)
	jge	.LBB0_12
# %bb.10:                               #   in Loop: Header=BB0_9 Depth=1
	movl	-348(%rbp), %eax
	andl	-352(%rbp), %eax
	movl	-348(%rbp), %ecx
	xorl	$-1, %ecx
	andl	-356(%rbp), %ecx
	orl	%ecx, %eax
	movl	%eax, -364(%rbp)
	movl	-344(%rbp), %eax
	shll	$5, %eax
	movl	-344(%rbp), %ecx
	shrl	$27, %ecx
	orl	%ecx, %eax
	addl	-364(%rbp), %eax
	addl	-360(%rbp), %eax
	movslq	-340(%rbp), %rcx
	addl	-336(%rbp,%rcx,4), %eax
	addl	$1518500249, %eax               # imm = 0x5A827999
	movl	%eax, -368(%rbp)
	movl	-356(%rbp), %eax
	movl	%eax, -360(%rbp)
	movl	-352(%rbp), %eax
	movl	%eax, -356(%rbp)
	movl	-348(%rbp), %eax
	shll	$30, %eax
	movl	-348(%rbp), %ecx
	shrl	$2, %ecx
	orl	%ecx, %eax
	movl	%eax, -352(%rbp)
	movl	-344(%rbp), %eax
	movl	%eax, -348(%rbp)
	movl	-368(%rbp), %eax
	movl	%eax, -344(%rbp)
# %bb.11:                               #   in Loop: Header=BB0_9 Depth=1
	movl	-340(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -340(%rbp)
	jmp	.LBB0_9
.LBB0_12:
	movl	$20, -340(%rbp)
.LBB0_13:                               # =>This Inner Loop Header: Depth=1
	cmpl	$40, -340(%rbp)
	jge	.LBB0_16
# %bb.14:                               #   in Loop: Header=BB0_13 Depth=1
	movl	-348(%rbp), %eax
	xorl	-352(%rbp), %eax
	xorl	-356(%rbp), %eax
	movl	%eax, -372(%rbp)
	movl	-344(%rbp), %eax
	shll	$5, %eax
	movl	-344(%rbp), %ecx
	shrl	$27, %ecx
	orl	%ecx, %eax
	addl	-372(%rbp), %eax
	addl	-360(%rbp), %eax
	movslq	-340(%rbp), %rcx
	addl	-336(%rbp,%rcx,4), %eax
	addl	$1859775393, %eax               # imm = 0x6ED9EBA1
	movl	%eax, -376(%rbp)
	movl	-356(%rbp), %eax
	movl	%eax, -360(%rbp)
	movl	-352(%rbp), %eax
	movl	%eax, -356(%rbp)
	movl	-348(%rbp), %eax
	shll	$30, %eax
	movl	-348(%rbp), %ecx
	shrl	$2, %ecx
	orl	%ecx, %eax
	movl	%eax, -352(%rbp)
	movl	-344(%rbp), %eax
	movl	%eax, -348(%rbp)
	movl	-376(%rbp), %eax
	movl	%eax, -344(%rbp)
# %bb.15:                               #   in Loop: Header=BB0_13 Depth=1
	movl	-340(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -340(%rbp)
	jmp	.LBB0_13
.LBB0_16:
	movl	$40, -340(%rbp)
.LBB0_17:                               # =>This Inner Loop Header: Depth=1
	cmpl	$60, -340(%rbp)
	jge	.LBB0_20
# %bb.18:                               #   in Loop: Header=BB0_17 Depth=1
	movl	-348(%rbp), %eax
	andl	-352(%rbp), %eax
	movl	-348(%rbp), %ecx
	andl	-356(%rbp), %ecx
	orl	%ecx, %eax
	movl	-352(%rbp), %ecx
	andl	-356(%rbp), %ecx
	orl	%ecx, %eax
	movl	%eax, -380(%rbp)
	movl	-344(%rbp), %eax
	shll	$5, %eax
	movl	-344(%rbp), %ecx
	shrl	$27, %ecx
	orl	%ecx, %eax
	addl	-380(%rbp), %eax
	addl	-360(%rbp), %eax
	movslq	-340(%rbp), %rcx
	addl	-336(%rbp,%rcx,4), %eax
	addl	$-1894007588, %eax              # imm = 0x8F1BBCDC
	movl	%eax, -384(%rbp)
	movl	-356(%rbp), %eax
	movl	%eax, -360(%rbp)
	movl	-352(%rbp), %eax
	movl	%eax, -356(%rbp)
	movl	-348(%rbp), %eax
	shll	$30, %eax
	movl	-348(%rbp), %ecx
	shrl	$2, %ecx
	orl	%ecx, %eax
	movl	%eax, -352(%rbp)
	movl	-344(%rbp), %eax
	movl	%eax, -348(%rbp)
	movl	-384(%rbp), %eax
	movl	%eax, -344(%rbp)
# %bb.19:                               #   in Loop: Header=BB0_17 Depth=1
	movl	-340(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -340(%rbp)
	jmp	.LBB0_17
.LBB0_20:
	movl	$60, -340(%rbp)
.LBB0_21:                               # =>This Inner Loop Header: Depth=1
	cmpl	$80, -340(%rbp)
	jge	.LBB0_24
# %bb.22:                               #   in Loop: Header=BB0_21 Depth=1
	movl	-348(%rbp), %eax
	xorl	-352(%rbp), %eax
	xorl	-356(%rbp), %eax
	movl	%eax, -388(%rbp)
	movl	-344(%rbp), %eax
	shll	$5, %eax
	movl	-344(%rbp), %ecx
	shrl	$27, %ecx
	orl	%ecx, %eax
	addl	-388(%rbp), %eax
	addl	-360(%rbp), %eax
	movslq	-340(%rbp), %rcx
	addl	-336(%rbp,%rcx,4), %eax
	addl	$-899497514, %eax               # imm = 0xCA62C1D6
	movl	%eax, -392(%rbp)
	movl	-356(%rbp), %eax
	movl	%eax, -360(%rbp)
	movl	-352(%rbp), %eax
	movl	%eax, -356(%rbp)
	movl	-348(%rbp), %eax
	shll	$30, %eax
	movl	-348(%rbp), %ecx
	shrl	$2, %ecx
	orl	%ecx, %eax
	movl	%eax, -352(%rbp)
	movl	-344(%rbp), %eax
	movl	%eax, -348(%rbp)
	movl	-392(%rbp), %eax
	movl	%eax, -344(%rbp)
# %bb.23:                               #   in Loop: Header=BB0_21 Depth=1
	movl	-340(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -340(%rbp)
	jmp	.LBB0_21
.LBB0_24:
	movl	-344(%rbp), %ecx
	movq	-8(%rbp), %rax
	addl	(%rax), %ecx
	movl	%ecx, (%rax)
	movl	-348(%rbp), %ecx
	movq	-8(%rbp), %rax
	addl	4(%rax), %ecx
	movl	%ecx, 4(%rax)
	movl	-352(%rbp), %ecx
	movq	-8(%rbp), %rax
	addl	8(%rax), %ecx
	movl	%ecx, 8(%rax)
	movl	-356(%rbp), %ecx
	movq	-8(%rbp), %rax
	addl	12(%rax), %ecx
	movl	%ecx, 12(%rax)
	movl	-360(%rbp), %ecx
	movq	-8(%rbp), %rax
	addl	16(%rax), %ecx
	movl	%ecx, 16(%rax)
	addq	$272, %rsp                      # imm = 0x110
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
	movq	%rdi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movq	%rdx, -24(%rbp)
	movl	$0, -28(%rbp)
.LBB1_1:                                # =>This Inner Loop Header: Depth=1
	movl	-28(%rbp), %eax
	cmpl	-12(%rbp), %eax
	jge	.LBB1_4
# %bb.2:                                #   in Loop: Header=BB1_1 Depth=1
	movq	-8(%rbp), %rax
	movslq	-28(%rbp), %rcx
	movzbl	(%rax,%rcx), %eax
	sarl	$4, %eax
	movslq	%eax, %rcx
	leaq	hex_encode.d(%rip), %rax
	movb	(%rax,%rcx), %dl
	movq	-24(%rbp), %rax
	movl	-28(%rbp), %ecx
	shll	%ecx
	movslq	%ecx, %rcx
	movb	%dl, (%rax,%rcx)
	movq	-8(%rbp), %rax
	movslq	-28(%rbp), %rcx
	movzbl	(%rax,%rcx), %eax
	andl	$15, %eax
	movslq	%eax, %rcx
	leaq	hex_encode.d(%rip), %rax
	movb	(%rax,%rcx), %dl
	movq	-24(%rbp), %rax
	movl	-28(%rbp), %ecx
	shll	%ecx
	addl	$1, %ecx
	movslq	%ecx, %rcx
	movb	%dl, (%rax,%rcx)
# %bb.3:                                #   in Loop: Header=BB1_1 Depth=1
	movl	-28(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -28(%rbp)
	jmp	.LBB1_1
.LBB1_4:
	movq	-24(%rbp), %rax
	movl	-12(%rbp), %ecx
	shll	%ecx
	movslq	%ecx, %rcx
	movb	$0, (%rax,%rcx)
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
	movl	$0, -68(%rbp)
.LBB2_1:                                # =>This Inner Loop Header: Depth=1
	cmpl	$64, -68(%rbp)
	jge	.LBB2_4
# %bb.2:                                #   in Loop: Header=BB2_1 Depth=1
	movslq	-68(%rbp), %rax
	movb	$0, -64(%rbp,%rax)
# %bb.3:                                #   in Loop: Header=BB2_1 Depth=1
	movl	-68(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -68(%rbp)
	jmp	.LBB2_1
.LBB2_4:
	movb	$97, -64(%rbp)
	movb	$98, -63(%rbp)
	movb	$99, -62(%rbp)
	movb	$-128, -61(%rbp)
	movb	$24, -1(%rbp)
	movq	.L__const.demo_run.h(%rip), %rax
	movq	%rax, -96(%rbp)
	movq	.L__const.demo_run.h+8(%rip), %rax
	movq	%rax, -88(%rbp)
	movl	.L__const.demo_run.h+16(%rip), %eax
	movl	%eax, -80(%rbp)
	leaq	-96(%rbp), %rdi
	leaq	-64(%rbp), %rsi
	callq	sha1_block
	movl	$0, -132(%rbp)
.LBB2_5:                                # =>This Inner Loop Header: Depth=1
	cmpl	$5, -132(%rbp)
	jge	.LBB2_8
# %bb.6:                                #   in Loop: Header=BB2_5 Depth=1
	movslq	-132(%rbp), %rax
	movl	-96(%rbp,%rax,4), %eax
	shrl	$24, %eax
	movb	%al, %cl
	movl	-132(%rbp), %eax
	shll	$2, %eax
	addl	$0, %eax
	cltq
	movb	%cl, -128(%rbp,%rax)
	movslq	-132(%rbp), %rax
	movl	-96(%rbp,%rax,4), %eax
	shrl	$16, %eax
	movb	%al, %cl
	movl	-132(%rbp), %eax
	shll	$2, %eax
	addl	$1, %eax
	cltq
	movb	%cl, -128(%rbp,%rax)
	movslq	-132(%rbp), %rax
	movl	-96(%rbp,%rax,4), %eax
	shrl	$8, %eax
	movb	%al, %cl
	movl	-132(%rbp), %eax
	shll	$2, %eax
	addl	$2, %eax
	cltq
	movb	%cl, -128(%rbp,%rax)
	movslq	-132(%rbp), %rax
	movl	-96(%rbp,%rax,4), %eax
	shrl	$0, %eax
	movb	%al, %cl
	movl	-132(%rbp), %eax
	shll	$2, %eax
	addl	$3, %eax
	cltq
	movb	%cl, -128(%rbp,%rax)
# %bb.7:                                #   in Loop: Header=BB2_5 Depth=1
	movl	-132(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -132(%rbp)
	jmp	.LBB2_5
.LBB2_8:
	leaq	-128(%rbp), %rdi
	leaq	-176(%rbp), %rdx
	movl	$20, %esi
	callq	hex_encode
	movzbl	-176(%rbp), %eax
	addq	$176, %rsp
	popq	%rbp
	retq
.Lfunc_end2:
	.size	demo_run, .Lfunc_end2-demo_run
                                        # -- End function
	.type	hex_encode.d,@object            # @hex_encode.d
	.section	.rodata,"a",@progbits
	.p2align	4, 0x0
hex_encode.d:
	.ascii	"0123456789abcdef"
	.size	hex_encode.d, 16

	.type	.L__const.demo_run.h,@object    # @__const.demo_run.h
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
	.addrsig_sym sha1_block
	.addrsig_sym hex_encode
	.addrsig_sym hex_encode.d
