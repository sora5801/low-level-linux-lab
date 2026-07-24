	.file	"demo.c"
	.text
	.globl	u64_to_dec                      # -- Begin function u64_to_dec
	.p2align	4
	.type	u64_to_dec,@function
u64_to_dec:                             # @u64_to_dec
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -16(%rbp)
	movq	%rsi, -24(%rbp)
	movl	$0, -52(%rbp)
	cmpq	$0, -16(%rbp)
	jne	.LBB0_2
# %bb.1:
	movq	-24(%rbp), %rax
	movb	$48, (%rax)
	movl	$1, -4(%rbp)
	jmp	.LBB0_10
.LBB0_2:
	jmp	.LBB0_3
.LBB0_3:                                # =>This Inner Loop Header: Depth=1
	cmpq	$0, -16(%rbp)
	je	.LBB0_5
# %bb.4:                                #   in Loop: Header=BB0_3 Depth=1
	movq	-16(%rbp), %rax
	movl	$10, %ecx
	xorl	%edx, %edx
                                        # kill: def $rdx killed $edx
	divq	%rcx
	movq	%rax, -64(%rbp)
	movq	-16(%rbp), %rax
	imulq	$10, -64(%rbp), %rcx
	subq	%rcx, %rax
	movq	%rax, -72(%rbp)
	movq	-72(%rbp), %rax
	addq	$48, %rax
	movb	%al, %cl
	movl	-52(%rbp), %eax
	movl	%eax, %edx
	addl	$1, %edx
	movl	%edx, -52(%rbp)
	movl	%eax, %eax
                                        # kill: def $rax killed $eax
	movb	%cl, -48(%rbp,%rax)
	movq	-64(%rbp), %rax
	movq	%rax, -16(%rbp)
	jmp	.LBB0_3
.LBB0_5:
	movl	$0, -76(%rbp)
.LBB0_6:                                # =>This Inner Loop Header: Depth=1
	movl	-76(%rbp), %eax
	cmpl	-52(%rbp), %eax
	jae	.LBB0_9
# %bb.7:                                #   in Loop: Header=BB0_6 Depth=1
	movl	-52(%rbp), %eax
	subl	$1, %eax
	subl	-76(%rbp), %eax
	movl	%eax, %eax
                                        # kill: def $rax killed $eax
	movb	-48(%rbp,%rax), %dl
	movq	-24(%rbp), %rax
	movl	-76(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	movb	%dl, (%rax,%rcx)
# %bb.8:                                #   in Loop: Header=BB0_6 Depth=1
	movl	-76(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -76(%rbp)
	jmp	.LBB0_6
.LBB0_9:
	movl	-52(%rbp), %eax
	movl	%eax, -4(%rbp)
.LBB0_10:
	movl	-4(%rbp), %eax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	u64_to_dec, .Lfunc_end0-u64_to_dec
                                        # -- End function
	.globl	i64_to_dec                      # -- Begin function i64_to_dec
	.p2align	4
	.type	i64_to_dec,@function
i64_to_dec:                             # @i64_to_dec
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$32, %rsp
	movq	%rdi, -16(%rbp)
	movq	%rsi, -24(%rbp)
	cmpq	$0, -16(%rbp)
	jge	.LBB1_2
# %bb.1:
	movq	-24(%rbp), %rax
	movb	$45, (%rax)
	xorl	%eax, %eax
	movl	%eax, %edi
	subq	-16(%rbp), %rdi
	movq	-24(%rbp), %rsi
	addq	$1, %rsi
	callq	u64_to_dec
	addl	$1, %eax
	movl	%eax, -4(%rbp)
	jmp	.LBB1_3
.LBB1_2:
	movq	-16(%rbp), %rdi
	movq	-24(%rbp), %rsi
	callq	u64_to_dec
	movl	%eax, -4(%rbp)
.LBB1_3:
	movl	-4(%rbp), %eax
	addq	$32, %rsp
	popq	%rbp
	retq
.Lfunc_end1:
	.size	i64_to_dec, .Lfunc_end1-i64_to_dec
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym u64_to_dec
