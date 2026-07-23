	.file	"demo.c"
	.text
	.globl	apply_relocations               # -- Begin function apply_relocations
	.p2align	4
	.type	apply_relocations,@function
apply_relocations:                      # @apply_relocations
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$112, %rsp
	movq	%rdi, -16(%rbp)
	movq	%rsi, -24(%rbp)
	movq	%rdx, -32(%rbp)
	movq	%rcx, -40(%rbp)
	movq	$0, -48(%rbp)
	movq	$0, -56(%rbp)
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	movq	-56(%rbp), %rax
	cmpq	-32(%rbp), %rax
	jae	.LBB0_10
# %bb.2:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-24(%rbp), %rax
	movq	-56(%rbp), %rcx
	leaq	(%rcx,%rcx,2), %rcx
	leaq	(%rax,%rcx,8), %rax
	movq	%rax, -64(%rbp)
	movq	-64(%rbp), %rax
	movl	8(%rax), %eax
                                        # kill: def $rax killed $eax
	movq	%rax, -72(%rbp)
	movq	-16(%rbp), %rax
	movq	-64(%rbp), %rcx
	movq	(%rcx), %rcx
	addq	%rcx, %rax
	movq	%rax, -80(%rbp)
	movq	-64(%rbp), %rax
	movq	16(%rax), %rax
	movq	%rax, -88(%rbp)
	movq	-72(%rbp), %rax
	movq	%rax, -112(%rbp)                # 8-byte Spill
	testq	%rax, %rax
	je	.LBB0_3
	jmp	.LBB0_12
.LBB0_12:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-112(%rbp), %rax                # 8-byte Reload
	subq	$1, %rax
	je	.LBB0_5
	jmp	.LBB0_13
.LBB0_13:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-112(%rbp), %rax                # 8-byte Reload
	addq	$-6, %rax
	subq	$2, %rax
	jb	.LBB0_5
	jmp	.LBB0_14
.LBB0_14:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-112(%rbp), %rax                # 8-byte Reload
	subq	$8, %rax
	je	.LBB0_4
	jmp	.LBB0_15
.LBB0_15:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-112(%rbp), %rax                # 8-byte Reload
	subq	$37, %rax
	je	.LBB0_6
	jmp	.LBB0_7
.LBB0_3:                                #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_9
.LBB0_4:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rcx
	addq	-88(%rbp), %rcx
	movq	-80(%rbp), %rax
	movq	%rcx, (%rax)
	jmp	.LBB0_8
.LBB0_5:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rdi
	movq	-40(%rbp), %rsi
	movq	-64(%rbp), %rax
	movq	8(%rax), %rdx
	shrq	$32, %rdx
	callq	resolve
	movq	%rax, -96(%rbp)
	movq	-96(%rbp), %rcx
	addq	-88(%rbp), %rcx
	movq	-80(%rbp), %rax
	movq	%rcx, (%rax)
	jmp	.LBB0_8
.LBB0_6:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rax
	addq	-88(%rbp), %rax
	movq	%rax, -104(%rbp)
	callq	*-104(%rbp)
	movq	%rax, %rcx
	movq	-80(%rbp), %rax
	movq	%rcx, (%rax)
	jmp	.LBB0_8
.LBB0_7:
	movq	-48(%rbp), %rax
	movq	%rax, -8(%rbp)
	jmp	.LBB0_11
.LBB0_8:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-48(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -48(%rbp)
.LBB0_9:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-56(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -56(%rbp)
	jmp	.LBB0_1
.LBB0_10:
	movq	-48(%rbp), %rax
	movq	%rax, -8(%rbp)
.LBB0_11:
	movq	-8(%rbp), %rax
	addq	$112, %rsp
	popq	%rbp
	retq
.Lfunc_end0:
	.size	apply_relocations, .Lfunc_end0-apply_relocations
                                        # -- End function
	.p2align	4                               # -- Begin function resolve
	.type	resolve,@function
resolve:                                # @resolve
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	%rdx, -24(%rbp)
	movq	-8(%rbp), %rax
	movq	-16(%rbp), %rcx
	imulq	$24, -24(%rbp), %rdx
	addq	%rdx, %rcx
	addq	8(%rcx), %rax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	resolve, .Lfunc_end1-resolve
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym resolve
