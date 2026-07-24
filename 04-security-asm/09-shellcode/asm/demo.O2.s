	.file	"demo.c"
	.text
	.globl	contains_badchar                # -- Begin function contains_badchar
	.p2align	4
	.type	contains_badchar,@function
contains_badchar:                       # @contains_badchar
# %bb.0:
	movq	$-1, %rax
	testq	%rsi, %rsi
	je	.LBB0_5
# %bb.1:
	xorl	%ecx, %ecx
	.p2align	4
.LBB0_2:                                # =>This Inner Loop Header: Depth=1
	movzbl	(%rdi,%rcx), %r8d
	movl	%r8d, %r9d
	shrl	$3, %r9d
	movzbl	(%rdx,%r9), %r9d
	andl	$7, %r8d
	btl	%r8d, %r9d
	jb	.LBB0_3
# %bb.4:                                #   in Loop: Header=BB0_2 Depth=1
	incq	%rcx
	cmpq	%rcx, %rsi
	jne	.LBB0_2
.LBB0_5:
	retq
.LBB0_3:
	movq	%rcx, %rax
	retq
.Lfunc_end0:
	.size	contains_badchar, .Lfunc_end0-contains_badchar
                                        # -- End function
	.globl	is_nul_free                     # -- Begin function is_nul_free
	.p2align	4
	.type	is_nul_free,@function
is_nul_free:                            # @is_nul_free
# %bb.0:
	movl	$1, %eax
	testq	%rsi, %rsi
	je	.LBB1_5
# %bb.1:
	xorl	%ecx, %ecx
	.p2align	4
.LBB1_3:                                # =>This Inner Loop Header: Depth=1
	cmpb	$0, (%rdi,%rcx)
	je	.LBB1_4
# %bb.2:                                #   in Loop: Header=BB1_3 Depth=1
	incq	%rcx
	cmpq	%rcx, %rsi
	jne	.LBB1_3
.LBB1_5:
	retq
.LBB1_4:
	xorl	%eax, %eax
	retq
.Lfunc_end1:
	.size	is_nul_free, .Lfunc_end1-is_nul_free
                                        # -- End function
	.globl	demo_selftest                   # -- Begin function demo_selftest
	.p2align	4
	.type	demo_selftest,@function
demo_selftest:                          # @demo_selftest
# %bb.0:
	xorl	%eax, %eax
	retq
.Lfunc_end2:
	.size	demo_selftest, .Lfunc_end2-demo_selftest
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
