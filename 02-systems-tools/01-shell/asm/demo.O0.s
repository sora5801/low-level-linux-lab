	.file	"demo.c"
	.text
	.globl	glob_star                       # -- Begin function glob_star
	.p2align	4
	.type	glob_star,@function
glob_star:                              # @glob_star
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -16(%rbp)
	movq	%rsi, -24(%rbp)
	movq	$0, -32(%rbp)
	movq	$0, -40(%rbp)
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	movq	-24(%rbp), %rax
	movsbl	(%rax), %eax
	cmpl	$0, %eax
	je	.LBB0_25
# %bb.2:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-24(%rbp), %rax
	movb	(%rax), %al
	movb	%al, -41(%rbp)
	movq	-16(%rbp), %rax
	movsbl	(%rax), %eax
	cmpl	$63, %eax
	jne	.LBB0_4
# %bb.3:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -16(%rbp)
	movq	-24(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -24(%rbp)
	jmp	.LBB0_24
.LBB0_4:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rax
	movsbl	(%rax), %eax
	cmpl	$42, %eax
	jne	.LBB0_6
# %bb.5:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -16(%rbp)
	movq	%rax, -32(%rbp)
	movq	-24(%rbp), %rax
	movq	%rax, -40(%rbp)
	jmp	.LBB0_23
.LBB0_6:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rax
	movsbl	(%rax), %eax
	cmpl	$92, %eax
	jne	.LBB0_15
# %bb.7:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rax
	movsbl	1(%rax), %eax
	cmpl	$0, %eax
	je	.LBB0_15
# %bb.8:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rax
	movzbl	1(%rax), %eax
	movzbl	-41(%rbp), %ecx
	cmpl	%ecx, %eax
	jne	.LBB0_10
# %bb.9:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rax
	addq	$2, %rax
	movq	%rax, -16(%rbp)
	movq	-24(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -24(%rbp)
	jmp	.LBB0_14
.LBB0_10:                               #   in Loop: Header=BB0_1 Depth=1
	cmpq	$0, -32(%rbp)
	je	.LBB0_12
# %bb.11:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-32(%rbp), %rax
	movq	%rax, -16(%rbp)
	movq	-40(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -40(%rbp)
	movq	%rax, -24(%rbp)
	jmp	.LBB0_13
.LBB0_12:
	movl	$0, -4(%rbp)
	jmp	.LBB0_29
.LBB0_13:                               #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_14
.LBB0_14:                               #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_22
.LBB0_15:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rax
	movzbl	(%rax), %eax
	movzbl	-41(%rbp), %ecx
	cmpl	%ecx, %eax
	jne	.LBB0_17
# %bb.16:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-16(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -16(%rbp)
	movq	-24(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -24(%rbp)
	jmp	.LBB0_21
.LBB0_17:                               #   in Loop: Header=BB0_1 Depth=1
	cmpq	$0, -32(%rbp)
	je	.LBB0_19
# %bb.18:                               #   in Loop: Header=BB0_1 Depth=1
	movq	-32(%rbp), %rax
	movq	%rax, -16(%rbp)
	movq	-40(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -40(%rbp)
	movq	%rax, -24(%rbp)
	jmp	.LBB0_20
.LBB0_19:
	movl	$0, -4(%rbp)
	jmp	.LBB0_29
.LBB0_20:                               #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_21
.LBB0_21:                               #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_22
.LBB0_22:                               #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_23
.LBB0_23:                               #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_24
.LBB0_24:                               #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_1
.LBB0_25:
	jmp	.LBB0_26
.LBB0_26:                               # =>This Inner Loop Header: Depth=1
	movq	-16(%rbp), %rax
	movsbl	(%rax), %eax
	cmpl	$42, %eax
	jne	.LBB0_28
# %bb.27:                               #   in Loop: Header=BB0_26 Depth=1
	movq	-16(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -16(%rbp)
	jmp	.LBB0_26
.LBB0_28:
	movq	-16(%rbp), %rax
	movsbl	(%rax), %eax
	cmpl	$0, %eax
	sete	%al
	andb	$1, %al
	movzbl	%al, %eax
	movl	%eax, -4(%rbp)
.LBB0_29:
	movl	-4(%rbp), %eax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	glob_star, .Lfunc_end0-glob_star
                                        # -- End function
	.globl	demo_run                        # -- Begin function demo_run
	.p2align	4
	.type	demo_run,@function
demo_run:                               # @demo_run
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movl	$0, -4(%rbp)
	leaq	.L.str(%rip), %rdi
	leaq	.L.str.1(%rip), %rsi
	callq	glob_star
	shll	$0, %eax
	orl	-4(%rbp), %eax
	movl	%eax, -4(%rbp)
	leaq	.L.str.2(%rip), %rdi
	leaq	.L.str.3(%rip), %rsi
	callq	glob_star
	shll	%eax
	orl	-4(%rbp), %eax
	movl	%eax, -4(%rbp)
	leaq	.L.str.4(%rip), %rdi
	leaq	.L.str.3(%rip), %rsi
	callq	glob_star
	shll	$2, %eax
	orl	-4(%rbp), %eax
	movl	%eax, -4(%rbp)
	leaq	.L.str.5(%rip), %rdi
	leaq	.L.str.6(%rip), %rsi
	callq	glob_star
	shll	$3, %eax
	orl	-4(%rbp), %eax
	movl	%eax, -4(%rbp)
	movl	-4(%rbp), %eax
	addq	$16, %rsp
	popq	%rbp
	retq
.Lfunc_end1:
	.size	demo_run, .Lfunc_end1-demo_run
                                        # -- End function
	.type	.L.str,@object                  # @.str
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str:
	.asciz	"*.c"
	.size	.L.str, 4

	.type	.L.str.1,@object                # @.str.1
.L.str.1:
	.asciz	"main.c"
	.size	.L.str.1, 7

	.type	.L.str.2,@object                # @.str.2
.L.str.2:
	.asciz	"a?c"
	.size	.L.str.2, 4

	.type	.L.str.3,@object                # @.str.3
.L.str.3:
	.asciz	"abc"
	.size	.L.str.3, 4

	.type	.L.str.4,@object                # @.str.4
.L.str.4:
	.asciz	"a*d"
	.size	.L.str.4, 4

	.type	.L.str.5,@object                # @.str.5
.L.str.5:
	.asciz	"*"
	.size	.L.str.5, 2

	.type	.L.str.6,@object                # @.str.6
.L.str.6:
	.zero	1
	.size	.L.str.6, 1

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym glob_star
