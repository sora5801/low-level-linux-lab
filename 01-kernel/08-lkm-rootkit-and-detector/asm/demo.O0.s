	.file	"demo.c"
	.text
	.globl	fnv1a64                         # -- Begin function fnv1a64
	.p2align	4
	.type	fnv1a64,@function
fnv1a64:                                # @fnv1a64
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movabsq	$-3750763034362895579, %rax     # imm = 0xCBF29CE484222325
	movq	%rax, -24(%rbp)
	movq	$0, -32(%rbp)
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	movq	-32(%rbp), %rax
	cmpq	-16(%rbp), %rax
	jae	.LBB0_4
# %bb.2:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-8(%rbp), %rax
	movq	-32(%rbp), %rcx
	movzbl	(%rax,%rcx), %eax
                                        # kill: def $rax killed $eax
	xorq	-24(%rbp), %rax
	movq	%rax, -24(%rbp)
	movabsq	$1099511628211, %rax            # imm = 0x100000001B3
	imulq	-24(%rbp), %rax
	movq	%rax, -24(%rbp)
# %bb.3:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-32(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -32(%rbp)
	jmp	.LBB0_1
.LBB0_4:
	movq	-24(%rbp), %rax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	fnv1a64, .Lfunc_end0-fnv1a64
                                        # -- End function
	.globl	region_first_diff               # -- Begin function region_first_diff
	.p2align	4
	.type	region_first_diff,@function
region_first_diff:                      # @region_first_diff
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -16(%rbp)
	movq	%rsi, -24(%rbp)
	movq	%rdx, -32(%rbp)
	movq	$0, -40(%rbp)
.LBB1_1:                                # =>This Inner Loop Header: Depth=1
	movq	-40(%rbp), %rax
	cmpq	-32(%rbp), %rax
	jae	.LBB1_6
# %bb.2:                                #   in Loop: Header=BB1_1 Depth=1
	movq	-16(%rbp), %rax
	movq	-40(%rbp), %rcx
	movzbl	(%rax,%rcx), %eax
	movq	-24(%rbp), %rcx
	movq	-40(%rbp), %rdx
	movzbl	(%rcx,%rdx), %ecx
	cmpl	%ecx, %eax
	je	.LBB1_4
# %bb.3:
	movq	-40(%rbp), %rax
	movq	%rax, -8(%rbp)
	jmp	.LBB1_7
.LBB1_4:                                #   in Loop: Header=BB1_1 Depth=1
	jmp	.LBB1_5
.LBB1_5:                                #   in Loop: Header=BB1_1 Depth=1
	movq	-40(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -40(%rbp)
	jmp	.LBB1_1
.LBB1_6:
	movq	$-1, -8(%rbp)
.LBB1_7:
	movq	-8(%rbp), %rax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	region_first_diff, .Lfunc_end1-region_first_diff
                                        # -- End function
	.globl	table_fingerprint               # -- Begin function table_fingerprint
	.p2align	4
	.type	table_fingerprint,@function
table_fingerprint:                      # @table_fingerprint
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	-8(%rbp), %rdi
	movq	-16(%rbp), %rsi
	shlq	$3, %rsi
	callq	fnv1a64
	addq	$16, %rsp
	popq	%rbp
	retq
.Lfunc_end2:
	.size	table_fingerprint, .Lfunc_end2-table_fingerprint
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym fnv1a64
