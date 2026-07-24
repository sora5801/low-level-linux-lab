	.file	"demo.c"
	.text
	.globl	contains_badchar                # -- Begin function contains_badchar
	.p2align	4
	.type	contains_badchar,@function
contains_badchar:                       # @contains_badchar
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$48, %rsp
	movq	%rdi, -16(%rbp)
	movq	%rsi, -24(%rbp)
	movq	%rdx, -32(%rbp)
	movq	$0, -40(%rbp)
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	movq	-40(%rbp), %rax
	cmpq	-24(%rbp), %rax
	jae	.LBB0_6
# %bb.2:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-32(%rbp), %rdi
	movq	-16(%rbp), %rax
	movq	-40(%rbp), %rcx
	movzbl	(%rax,%rcx), %esi
	callq	badset_has
	cmpl	$0, %eax
	je	.LBB0_4
# %bb.3:
	movq	-40(%rbp), %rax
	movq	%rax, -8(%rbp)
	jmp	.LBB0_7
.LBB0_4:                                #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_5
.LBB0_5:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-40(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -40(%rbp)
	jmp	.LBB0_1
.LBB0_6:
	movq	$-1, -8(%rbp)
.LBB0_7:
	movq	-8(%rbp), %rax
	addq	$48, %rsp
	popq	%rbp
	retq
.Lfunc_end0:
	.size	contains_badchar, .Lfunc_end0-contains_badchar
                                        # -- End function
	.p2align	4                               # -- Begin function badset_has
	.type	badset_has,@function
badset_has:                             # @badset_has
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movb	%sil, %al
	movq	%rdi, -8(%rbp)
	movb	%al, -9(%rbp)
	movq	-8(%rbp), %rax
	movzbl	-9(%rbp), %ecx
	sarl	$3, %ecx
	movslq	%ecx, %rcx
	movzbl	(%rax,%rcx), %eax
	movzbl	-9(%rbp), %ecx
	andl	$7, %ecx
                                        # kill: def $cl killed $ecx
	sarl	%cl, %eax
	andl	$1, %eax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	badset_has, .Lfunc_end1-badset_has
                                        # -- End function
	.globl	is_nul_free                     # -- Begin function is_nul_free
	.p2align	4
	.type	is_nul_free,@function
is_nul_free:                            # @is_nul_free
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -16(%rbp)
	movq	%rsi, -24(%rbp)
	movq	$0, -32(%rbp)
.LBB2_1:                                # =>This Inner Loop Header: Depth=1
	movq	-32(%rbp), %rax
	cmpq	-24(%rbp), %rax
	jae	.LBB2_6
# %bb.2:                                #   in Loop: Header=BB2_1 Depth=1
	movq	-16(%rbp), %rax
	movq	-32(%rbp), %rcx
	movzbl	(%rax,%rcx), %eax
	cmpl	$0, %eax
	jne	.LBB2_4
# %bb.3:
	movl	$0, -4(%rbp)
	jmp	.LBB2_7
.LBB2_4:                                #   in Loop: Header=BB2_1 Depth=1
	jmp	.LBB2_5
.LBB2_5:                                #   in Loop: Header=BB2_1 Depth=1
	movq	-32(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -32(%rbp)
	jmp	.LBB2_1
.LBB2_6:
	movl	$1, -4(%rbp)
.LBB2_7:
	movl	-4(%rbp), %eax
	popq	%rbp
	retq
.Lfunc_end2:
	.size	is_nul_free, .Lfunc_end2-is_nul_free
                                        # -- End function
	.globl	demo_selftest                   # -- Begin function demo_selftest
	.p2align	4
	.type	demo_selftest,@function
demo_selftest:                          # @demo_selftest
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$48, %rsp
	leaq	-36(%rbp), %rdi
	xorl	%esi, %esi
	movl	$32, %edx
	callq	memset@PLT
	leaq	-36(%rbp), %rdi
	xorl	%esi, %esi
	callq	badset_add
	leaq	-36(%rbp), %rdi
	movl	$10, %esi
	callq	badset_add
	movl	.L__const.demo_selftest.clean(%rip), %eax
	movl	%eax, -40(%rbp)
	movw	.L__const.demo_selftest.dirty(%rip), %ax
	movw	%ax, -43(%rbp)
	movb	.L__const.demo_selftest.dirty+2(%rip), %al
	movb	%al, -41(%rbp)
	leaq	-40(%rbp), %rdi
	movl	$4, %esi
	leaq	-36(%rbp), %rdx
	callq	contains_badchar
	cmpq	$-1, %rax
	je	.LBB3_2
# %bb.1:
	movl	$1, -4(%rbp)
	jmp	.LBB3_9
.LBB3_2:
	leaq	-43(%rbp), %rdi
	movl	$3, %esi
	leaq	-36(%rbp), %rdx
	callq	contains_badchar
	cmpq	$1, %rax
	je	.LBB3_4
# %bb.3:
	movl	$2, -4(%rbp)
	jmp	.LBB3_9
.LBB3_4:
	leaq	-40(%rbp), %rdi
	movl	$4, %esi
	callq	is_nul_free
	cmpl	$1, %eax
	je	.LBB3_6
# %bb.5:
	movl	$3, -4(%rbp)
	jmp	.LBB3_9
.LBB3_6:
	movw	.L__const.demo_selftest.hasnul(%rip), %ax
	movw	%ax, -46(%rbp)
	movb	.L__const.demo_selftest.hasnul+2(%rip), %al
	movb	%al, -44(%rbp)
	leaq	-46(%rbp), %rdi
	movl	$3, %esi
	callq	is_nul_free
	cmpl	$0, %eax
	je	.LBB3_8
# %bb.7:
	movl	$4, -4(%rbp)
	jmp	.LBB3_9
.LBB3_8:
	movl	$0, -4(%rbp)
.LBB3_9:
	movl	-4(%rbp), %eax
	addq	$48, %rsp
	popq	%rbp
	retq
.Lfunc_end3:
	.size	demo_selftest, .Lfunc_end3-demo_selftest
                                        # -- End function
	.p2align	4                               # -- Begin function badset_add
	.type	badset_add,@function
badset_add:                             # @badset_add
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movb	%sil, %al
	movq	%rdi, -8(%rbp)
	movb	%al, -9(%rbp)
	movzbl	-9(%rbp), %ecx
	andl	$7, %ecx
	movl	$1, %eax
                                        # kill: def $cl killed $ecx
	shll	%cl, %eax
                                        # kill: def $al killed $al killed $eax
	movzbl	%al, %esi
	movq	-8(%rbp), %rax
	movzbl	-9(%rbp), %ecx
	sarl	$3, %ecx
	movslq	%ecx, %rcx
	movzbl	(%rax,%rcx), %edx
	orl	%esi, %edx
                                        # kill: def $dl killed $dl killed $edx
	movb	%dl, (%rax,%rcx)
	popq	%rbp
	retq
.Lfunc_end4:
	.size	badset_add, .Lfunc_end4-badset_add
                                        # -- End function
	.type	.L__const.demo_selftest.clean,@object # @__const.demo_selftest.clean
	.section	.rodata.cst4,"aM",@progbits,4
.L__const.demo_selftest.clean:
	.ascii	"ABC\177"
	.size	.L__const.demo_selftest.clean, 4

	.type	.L__const.demo_selftest.dirty,@object # @__const.demo_selftest.dirty
	.section	.rodata,"a",@progbits
.L__const.demo_selftest.dirty:
	.ascii	"A\nC"
	.size	.L__const.demo_selftest.dirty, 3

	.type	.L__const.demo_selftest.hasnul,@object # @__const.demo_selftest.hasnul
.L__const.demo_selftest.hasnul:
	.ascii	"X\000Z"
	.size	.L__const.demo_selftest.hasnul, 3

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym contains_badchar
	.addrsig_sym badset_has
	.addrsig_sym is_nul_free
	.addrsig_sym badset_add
