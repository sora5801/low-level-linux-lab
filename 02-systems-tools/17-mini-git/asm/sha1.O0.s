	.file	"sha1.c"
	.text
	.globl	sha1_init                       # -- Begin function sha1_init
	.p2align	4
	.type	sha1_init,@function
sha1_init:                              # @sha1_init
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rax
	movl	$1732584193, (%rax)             # imm = 0x67452301
	movq	-8(%rbp), %rax
	movl	$-271733879, 4(%rax)            # imm = 0xEFCDAB89
	movq	-8(%rbp), %rax
	movl	$-1732584194, 8(%rax)           # imm = 0x98BADCFE
	movq	-8(%rbp), %rax
	movl	$271733878, 12(%rax)            # imm = 0x10325476
	movq	-8(%rbp), %rax
	movl	$-1009589776, 16(%rax)          # imm = 0xC3D2E1F0
	movq	-8(%rbp), %rax
	movq	$0, 24(%rax)
	movq	-8(%rbp), %rax
	movl	$0, 96(%rax)
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
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$48, %rsp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	%rdx, -24(%rbp)
	movq	-16(%rbp), %rax
	movq	%rax, -32(%rbp)
	movq	-24(%rbp), %rcx
	shlq	$3, %rcx
	movq	-8(%rbp), %rax
	addq	24(%rax), %rcx
	movq	%rcx, 24(%rax)
.LBB1_1:                                # =>This Loop Header: Depth=1
                                        #     Child Loop BB1_6 Depth 2
	cmpq	$0, -24(%rbp)
	jbe	.LBB1_12
# %bb.2:                                #   in Loop: Header=BB1_1 Depth=1
	movq	-8(%rbp), %rcx
	movl	$64, %eax
	subl	96(%rcx), %eax
	movl	%eax, -36(%rbp)
	movl	-36(%rbp), %eax
                                        # kill: def $rax killed $eax
	cmpq	-24(%rbp), %rax
	jae	.LBB1_4
# %bb.3:                                #   in Loop: Header=BB1_1 Depth=1
	movl	-36(%rbp), %eax
	movl	%eax, -48(%rbp)                 # 4-byte Spill
	jmp	.LBB1_5
.LBB1_4:                                #   in Loop: Header=BB1_1 Depth=1
	movq	-24(%rbp), %rax
                                        # kill: def $eax killed $eax killed $rax
	movl	%eax, -48(%rbp)                 # 4-byte Spill
.LBB1_5:                                #   in Loop: Header=BB1_1 Depth=1
	movl	-48(%rbp), %eax                 # 4-byte Reload
	movl	%eax, -40(%rbp)
	movl	$0, -44(%rbp)
.LBB1_6:                                #   Parent Loop BB1_1 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movl	-44(%rbp), %eax
	cmpl	-40(%rbp), %eax
	jae	.LBB1_9
# %bb.7:                                #   in Loop: Header=BB1_6 Depth=2
	movq	-32(%rbp), %rax
	movl	-44(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movb	(%rax,%rcx), %dl
	movq	-8(%rbp), %rax
	movq	-8(%rbp), %rcx
	movl	96(%rcx), %ecx
	addl	-44(%rbp), %ecx
	movl	%ecx, %ecx
                                        # kill: def $rcx killed $ecx
	movb	%dl, 32(%rax,%rcx)
# %bb.8:                                #   in Loop: Header=BB1_6 Depth=2
	movl	-44(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -44(%rbp)
	jmp	.LBB1_6
.LBB1_9:                                #   in Loop: Header=BB1_1 Depth=1
	movl	-40(%rbp), %ecx
	movq	-8(%rbp), %rax
	addl	96(%rax), %ecx
	movl	%ecx, 96(%rax)
	movl	-40(%rbp), %ecx
	movq	-32(%rbp), %rax
	movl	%ecx, %ecx
                                        # kill: def $rcx killed $ecx
	addq	%rcx, %rax
	movq	%rax, -32(%rbp)
	movl	-40(%rbp), %eax
	movl	%eax, %ecx
	movq	-24(%rbp), %rax
	subq	%rcx, %rax
	movq	%rax, -24(%rbp)
	movq	-8(%rbp), %rax
	cmpl	$64, 96(%rax)
	jne	.LBB1_11
# %bb.10:                               #   in Loop: Header=BB1_1 Depth=1
	movq	-8(%rbp), %rdi
	movq	-8(%rbp), %rsi
	addq	$32, %rsi
	callq	sha1_block
	movq	-8(%rbp), %rax
	movl	$0, 96(%rax)
.LBB1_11:                               #   in Loop: Header=BB1_1 Depth=1
	jmp	.LBB1_1
.LBB1_12:
	addq	$48, %rsp
	popq	%rbp
	retq
.Lfunc_end1:
	.size	sha1_update, .Lfunc_end1-sha1_update
                                        # -- End function
	.p2align	4                               # -- Begin function sha1_block
	.type	sha1_block,@function
sha1_block:                             # @sha1_block
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$272, %rsp                      # imm = 0x110
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movl	$0, -340(%rbp)
.LBB2_1:                                # =>This Inner Loop Header: Depth=1
	cmpl	$16, -340(%rbp)
	jge	.LBB2_4
# %bb.2:                                #   in Loop: Header=BB2_1 Depth=1
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
# %bb.3:                                #   in Loop: Header=BB2_1 Depth=1
	movl	-340(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -340(%rbp)
	jmp	.LBB2_1
.LBB2_4:
	movl	$16, -340(%rbp)
.LBB2_5:                                # =>This Inner Loop Header: Depth=1
	cmpl	$80, -340(%rbp)
	jge	.LBB2_8
# %bb.6:                                #   in Loop: Header=BB2_5 Depth=1
	movl	-340(%rbp), %eax
	subl	$3, %eax
	cltq
	movl	-336(%rbp,%rax,4), %eax
	movl	-340(%rbp), %ecx
	subl	$8, %ecx
	movslq	%ecx, %rcx
	xorl	-336(%rbp,%rcx,4), %eax
	movl	-340(%rbp), %ecx
	subl	$14, %ecx
	movslq	%ecx, %rcx
	xorl	-336(%rbp,%rcx,4), %eax
	movl	-340(%rbp), %ecx
	subl	$16, %ecx
	movslq	%ecx, %rcx
	xorl	-336(%rbp,%rcx,4), %eax
	movl	%eax, -344(%rbp)
	movl	-344(%rbp), %ecx
	shll	%ecx
	movl	-344(%rbp), %eax
	shrl	$31, %eax
	orl	%eax, %ecx
	movslq	-340(%rbp), %rax
	movl	%ecx, -336(%rbp,%rax,4)
# %bb.7:                                #   in Loop: Header=BB2_5 Depth=1
	movl	-340(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -340(%rbp)
	jmp	.LBB2_5
.LBB2_8:
	movq	-8(%rbp), %rax
	movl	(%rax), %eax
	movl	%eax, -348(%rbp)
	movq	-8(%rbp), %rax
	movl	4(%rax), %eax
	movl	%eax, -352(%rbp)
	movq	-8(%rbp), %rax
	movl	8(%rax), %eax
	movl	%eax, -356(%rbp)
	movq	-8(%rbp), %rax
	movl	12(%rax), %eax
	movl	%eax, -360(%rbp)
	movq	-8(%rbp), %rax
	movl	16(%rax), %eax
	movl	%eax, -364(%rbp)
	movl	$0, -340(%rbp)
.LBB2_9:                                # =>This Inner Loop Header: Depth=1
	cmpl	$20, -340(%rbp)
	jge	.LBB2_12
# %bb.10:                               #   in Loop: Header=BB2_9 Depth=1
	movl	-352(%rbp), %eax
	andl	-356(%rbp), %eax
	movl	-352(%rbp), %ecx
	xorl	$-1, %ecx
	andl	-360(%rbp), %ecx
	orl	%ecx, %eax
	movl	%eax, -368(%rbp)
	movl	-348(%rbp), %eax
	shll	$5, %eax
	movl	-348(%rbp), %ecx
	shrl	$27, %ecx
	orl	%ecx, %eax
	addl	-368(%rbp), %eax
	addl	-364(%rbp), %eax
	movslq	-340(%rbp), %rcx
	addl	-336(%rbp,%rcx,4), %eax
	addl	$1518500249, %eax               # imm = 0x5A827999
	movl	%eax, -372(%rbp)
	movl	-360(%rbp), %eax
	movl	%eax, -364(%rbp)
	movl	-356(%rbp), %eax
	movl	%eax, -360(%rbp)
	movl	-352(%rbp), %eax
	shll	$30, %eax
	movl	-352(%rbp), %ecx
	shrl	$2, %ecx
	orl	%ecx, %eax
	movl	%eax, -356(%rbp)
	movl	-348(%rbp), %eax
	movl	%eax, -352(%rbp)
	movl	-372(%rbp), %eax
	movl	%eax, -348(%rbp)
# %bb.11:                               #   in Loop: Header=BB2_9 Depth=1
	movl	-340(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -340(%rbp)
	jmp	.LBB2_9
.LBB2_12:
	movl	$20, -340(%rbp)
.LBB2_13:                               # =>This Inner Loop Header: Depth=1
	cmpl	$40, -340(%rbp)
	jge	.LBB2_16
# %bb.14:                               #   in Loop: Header=BB2_13 Depth=1
	movl	-352(%rbp), %eax
	xorl	-356(%rbp), %eax
	xorl	-360(%rbp), %eax
	movl	%eax, -376(%rbp)
	movl	-348(%rbp), %eax
	shll	$5, %eax
	movl	-348(%rbp), %ecx
	shrl	$27, %ecx
	orl	%ecx, %eax
	addl	-376(%rbp), %eax
	addl	-364(%rbp), %eax
	movslq	-340(%rbp), %rcx
	addl	-336(%rbp,%rcx,4), %eax
	addl	$1859775393, %eax               # imm = 0x6ED9EBA1
	movl	%eax, -380(%rbp)
	movl	-360(%rbp), %eax
	movl	%eax, -364(%rbp)
	movl	-356(%rbp), %eax
	movl	%eax, -360(%rbp)
	movl	-352(%rbp), %eax
	shll	$30, %eax
	movl	-352(%rbp), %ecx
	shrl	$2, %ecx
	orl	%ecx, %eax
	movl	%eax, -356(%rbp)
	movl	-348(%rbp), %eax
	movl	%eax, -352(%rbp)
	movl	-380(%rbp), %eax
	movl	%eax, -348(%rbp)
# %bb.15:                               #   in Loop: Header=BB2_13 Depth=1
	movl	-340(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -340(%rbp)
	jmp	.LBB2_13
.LBB2_16:
	movl	$40, -340(%rbp)
.LBB2_17:                               # =>This Inner Loop Header: Depth=1
	cmpl	$60, -340(%rbp)
	jge	.LBB2_20
# %bb.18:                               #   in Loop: Header=BB2_17 Depth=1
	movl	-352(%rbp), %eax
	andl	-356(%rbp), %eax
	movl	-352(%rbp), %ecx
	andl	-360(%rbp), %ecx
	orl	%ecx, %eax
	movl	-356(%rbp), %ecx
	andl	-360(%rbp), %ecx
	orl	%ecx, %eax
	movl	%eax, -384(%rbp)
	movl	-348(%rbp), %eax
	shll	$5, %eax
	movl	-348(%rbp), %ecx
	shrl	$27, %ecx
	orl	%ecx, %eax
	addl	-384(%rbp), %eax
	addl	-364(%rbp), %eax
	movslq	-340(%rbp), %rcx
	addl	-336(%rbp,%rcx,4), %eax
	addl	$-1894007588, %eax              # imm = 0x8F1BBCDC
	movl	%eax, -388(%rbp)
	movl	-360(%rbp), %eax
	movl	%eax, -364(%rbp)
	movl	-356(%rbp), %eax
	movl	%eax, -360(%rbp)
	movl	-352(%rbp), %eax
	shll	$30, %eax
	movl	-352(%rbp), %ecx
	shrl	$2, %ecx
	orl	%ecx, %eax
	movl	%eax, -356(%rbp)
	movl	-348(%rbp), %eax
	movl	%eax, -352(%rbp)
	movl	-388(%rbp), %eax
	movl	%eax, -348(%rbp)
# %bb.19:                               #   in Loop: Header=BB2_17 Depth=1
	movl	-340(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -340(%rbp)
	jmp	.LBB2_17
.LBB2_20:
	movl	$60, -340(%rbp)
.LBB2_21:                               # =>This Inner Loop Header: Depth=1
	cmpl	$80, -340(%rbp)
	jge	.LBB2_24
# %bb.22:                               #   in Loop: Header=BB2_21 Depth=1
	movl	-352(%rbp), %eax
	xorl	-356(%rbp), %eax
	xorl	-360(%rbp), %eax
	movl	%eax, -392(%rbp)
	movl	-348(%rbp), %eax
	shll	$5, %eax
	movl	-348(%rbp), %ecx
	shrl	$27, %ecx
	orl	%ecx, %eax
	addl	-392(%rbp), %eax
	addl	-364(%rbp), %eax
	movslq	-340(%rbp), %rcx
	addl	-336(%rbp,%rcx,4), %eax
	addl	$-899497514, %eax               # imm = 0xCA62C1D6
	movl	%eax, -396(%rbp)
	movl	-360(%rbp), %eax
	movl	%eax, -364(%rbp)
	movl	-356(%rbp), %eax
	movl	%eax, -360(%rbp)
	movl	-352(%rbp), %eax
	shll	$30, %eax
	movl	-352(%rbp), %ecx
	shrl	$2, %ecx
	orl	%ecx, %eax
	movl	%eax, -356(%rbp)
	movl	-348(%rbp), %eax
	movl	%eax, -352(%rbp)
	movl	-396(%rbp), %eax
	movl	%eax, -348(%rbp)
# %bb.23:                               #   in Loop: Header=BB2_21 Depth=1
	movl	-340(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -340(%rbp)
	jmp	.LBB2_21
.LBB2_24:
	movl	-348(%rbp), %ecx
	movq	-8(%rbp), %rax
	addl	(%rax), %ecx
	movl	%ecx, (%rax)
	movl	-352(%rbp), %ecx
	movq	-8(%rbp), %rax
	addl	4(%rax), %ecx
	movl	%ecx, 4(%rax)
	movl	-356(%rbp), %ecx
	movq	-8(%rbp), %rax
	addl	8(%rax), %ecx
	movl	%ecx, 8(%rax)
	movl	-360(%rbp), %ecx
	movq	-8(%rbp), %rax
	addl	12(%rax), %ecx
	movl	%ecx, 12(%rax)
	movl	-364(%rbp), %ecx
	movq	-8(%rbp), %rax
	addl	16(%rax), %ecx
	movl	%ecx, 16(%rax)
	addq	$272, %rsp                      # imm = 0x110
	popq	%rbp
	retq
.Lfunc_end2:
	.size	sha1_block, .Lfunc_end2-sha1_block
                                        # -- End function
	.globl	sha1_final                      # -- Begin function sha1_final
	.p2align	4
	.type	sha1_final,@function
sha1_final:                             # @sha1_final
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$48, %rsp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	-8(%rbp), %rax
	movq	24(%rax), %rax
	movq	%rax, -24(%rbp)
	movb	$-128, -25(%rbp)
	movq	-8(%rbp), %rdi
	leaq	-25(%rbp), %rsi
	movl	$1, %edx
	callq	sha1_update
	movb	$0, -26(%rbp)
.LBB3_1:                                # =>This Inner Loop Header: Depth=1
	movq	-8(%rbp), %rax
	cmpl	$56, 96(%rax)
	je	.LBB3_3
# %bb.2:                                #   in Loop: Header=BB3_1 Depth=1
	movq	-8(%rbp), %rdi
	leaq	-26(%rbp), %rsi
	movl	$1, %edx
	callq	sha1_update
	jmp	.LBB3_1
.LBB3_3:
	movl	$0, -40(%rbp)
.LBB3_4:                                # =>This Inner Loop Header: Depth=1
	cmpl	$8, -40(%rbp)
	jge	.LBB3_7
# %bb.5:                                #   in Loop: Header=BB3_4 Depth=1
	movq	-24(%rbp), %rax
	movl	-40(%rbp), %edx
	shll	$3, %edx
	movl	$56, %ecx
	subl	%edx, %ecx
	movl	%ecx, %ecx
                                        # kill: def $rcx killed $ecx
                                        # kill: def $cl killed $rcx
	shrq	%cl, %rax
	movb	%al, %cl
	movslq	-40(%rbp), %rax
	movb	%cl, -34(%rbp,%rax)
# %bb.6:                                #   in Loop: Header=BB3_4 Depth=1
	movl	-40(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -40(%rbp)
	jmp	.LBB3_4
.LBB3_7:
	movq	-8(%rbp), %rdi
	leaq	-34(%rbp), %rsi
	movl	$8, %edx
	callq	sha1_update
	movl	$0, -44(%rbp)
.LBB3_8:                                # =>This Inner Loop Header: Depth=1
	cmpl	$5, -44(%rbp)
	jge	.LBB3_11
# %bb.9:                                #   in Loop: Header=BB3_8 Depth=1
	movq	-8(%rbp), %rax
	movslq	-44(%rbp), %rcx
	movl	(%rax,%rcx,4), %eax
	shrl	$24, %eax
	movb	%al, %dl
	movq	-16(%rbp), %rax
	movl	-44(%rbp), %ecx
	shll	$2, %ecx
	addl	$0, %ecx
	movslq	%ecx, %rcx
	movb	%dl, (%rax,%rcx)
	movq	-8(%rbp), %rax
	movslq	-44(%rbp), %rcx
	movl	(%rax,%rcx,4), %eax
	shrl	$16, %eax
	movb	%al, %dl
	movq	-16(%rbp), %rax
	movl	-44(%rbp), %ecx
	shll	$2, %ecx
	addl	$1, %ecx
	movslq	%ecx, %rcx
	movb	%dl, (%rax,%rcx)
	movq	-8(%rbp), %rax
	movslq	-44(%rbp), %rcx
	movl	(%rax,%rcx,4), %eax
	shrl	$8, %eax
	movb	%al, %dl
	movq	-16(%rbp), %rax
	movl	-44(%rbp), %ecx
	shll	$2, %ecx
	addl	$2, %ecx
	movslq	%ecx, %rcx
	movb	%dl, (%rax,%rcx)
	movq	-8(%rbp), %rax
	movslq	-44(%rbp), %rcx
	movl	(%rax,%rcx,4), %eax
	shrl	$0, %eax
	movb	%al, %dl
	movq	-16(%rbp), %rax
	movl	-44(%rbp), %ecx
	shll	$2, %ecx
	addl	$3, %ecx
	movslq	%ecx, %rcx
	movb	%dl, (%rax,%rcx)
# %bb.10:                               #   in Loop: Header=BB3_8 Depth=1
	movl	-44(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -44(%rbp)
	jmp	.LBB3_8
.LBB3_11:
	addq	$48, %rsp
	popq	%rbp
	retq
.Lfunc_end3:
	.size	sha1_final, .Lfunc_end3-sha1_final
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym sha1_update
	.addrsig_sym sha1_block
