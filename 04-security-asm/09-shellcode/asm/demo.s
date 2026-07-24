	.file	"demo.c"
	.text
	.globl	contains_badchar                # -- Begin function contains_badchar
	.p2align	4
	.type	contains_badchar,@function
contains_badchar:                       # @contains_badchar
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
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
	popq	%rbp
	retq
.LBB0_3:
	movq	%rcx, %rax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	contains_badchar, .Lfunc_end0-contains_badchar
                                        # -- End function
	.globl	is_nul_free                     # -- Begin function is_nul_free
	.p2align	4
	.type	is_nul_free,@function
is_nul_free:                            # @is_nul_free
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
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
	popq	%rbp
	retq
.LBB1_4:
	xorl	%eax, %eax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	is_nul_free, .Lfunc_end1-is_nul_free
                                        # -- End function
	.globl	demo_selftest                   # -- Begin function demo_selftest
	.p2align	4
	.type	demo_selftest,@function
demo_selftest:                          # @demo_selftest
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	xorps	%xmm0, %xmm0
	movaps	%xmm0, -48(%rbp)
	movaps	%xmm0, -32(%rbp)
	movb	$1, -48(%rbp)
	orb	$4, -47(%rbp)
	movl	$2135114305, -4(%rbp)           # imm = 0x7F434241
	xorl	%eax, %eax
	.p2align	4
.LBB2_1:                                # =>This Inner Loop Header: Depth=1
	movzbl	-4(%rbp,%rax), %ecx
	movl	%ecx, %edx
	shrl	$3, %edx
	movzbl	-48(%rbp,%rdx), %esi
	andb	$7, %cl
	movl	$1, %edx
                                        # kill: def $cl killed $cl killed $ecx
	shll	%cl, %edx
	andl	%esi, %edx
	jne	.LBB2_3
# %bb.2:                                #   in Loop: Header=BB2_1 Depth=1
	leaq	1(%rax), %rcx
	cmpq	$3, %rax
	movq	%rcx, %rax
	jne	.LBB2_1
.LBB2_3:
	movl	$1, %eax
	testl	%edx, %edx
	je	.LBB2_4
.LBB2_13:
	popq	%rbp
	retq
.LBB2_4:
	xorl	%ecx, %ecx
	leaq	.L__const.demo_selftest.dirty(%rip), %rax
	.p2align	4
.LBB2_5:                                # =>This Inner Loop Header: Depth=1
	movzbl	(%rcx,%rax), %edx
	movl	%edx, %esi
	shrl	$3, %esi
	movzbl	-48(%rbp,%rsi), %esi
	andl	$7, %edx
	btl	%edx, %esi
	jb	.LBB2_8
# %bb.6:                                #   in Loop: Header=BB2_5 Depth=1
	incq	%rcx
	cmpq	$3, %rcx
	jne	.LBB2_5
# %bb.7:
	movq	$-1, %rcx
.LBB2_8:
	movl	$2, %eax
	cmpq	$1, %rcx
	jne	.LBB2_13
# %bb.9:
	xorl	%eax, %eax
	.p2align	4
.LBB2_10:                               # =>This Inner Loop Header: Depth=1
	movzbl	-4(%rbp,%rax), %ecx
	testb	%cl, %cl
	je	.LBB2_12
# %bb.11:                               #   in Loop: Header=BB2_10 Depth=1
	leaq	1(%rax), %rdx
	cmpq	$3, %rax
	movq	%rdx, %rax
	jne	.LBB2_10
.LBB2_12:
	xorl	%eax, %eax
	testb	%cl, %cl
	sete	%al
	leal	(%rax,%rax,2), %eax
	popq	%rbp
	retq
.Lfunc_end2:
	.size	demo_selftest, .Lfunc_end2-demo_selftest
                                        # -- End function
	.type	.L__const.demo_selftest.dirty,@object # @__const.demo_selftest.dirty
	.section	.rodata,"a",@progbits
.L__const.demo_selftest.dirty:
	.ascii	"A\nC"
	.size	.L__const.demo_selftest.dirty, 3

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
