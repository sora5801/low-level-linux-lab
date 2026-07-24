	.file	"demo.c"
	.text
	.globl	syscall_args                    # -- Begin function syscall_args
	.p2align	4
	.type	syscall_args,@function
syscall_args:                           # @syscall_args
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	8(%rdi), %rax
	movq	%rax, (%rsi)
	movq	16(%rdi), %rax
	movq	%rax, 8(%rsi)
	movq	24(%rdi), %rax
	movq	%rax, 16(%rsi)
	movq	32(%rdi), %rax
	movq	%rax, 24(%rsi)
	movq	40(%rdi), %rax
	movq	%rax, 32(%rsi)
	movq	48(%rdi), %rax
	movq	%rax, 40(%rsi)
	movq	(%rdi), %rax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	syscall_args, .Lfunc_end0-syscall_args
                                        # -- End function
	.globl	decode_flags                    # -- Begin function decode_flags
	.p2align	4
	.type	decode_flags,@function
decode_flags:                           # @decode_flags
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
                                        # kill: def $r8d killed $r8d def $r8
	leal	-1(%r8), %r9d
	testl	%edx, %edx
	setle	%r10b
	cmpl	$2, %r8d
	setge	%bl
	setl	%r11b
	xorl	%eax, %eax
	orb	%r10b, %r11b
	jne	.LBB1_1
# %bb.16:
	movslq	%r9d, %r10
	movl	%edx, %edx
	xorl	%r11d, %r11d
	xorl	%r14d, %r14d
	xorl	%eax, %eax
	.p2align	4
.LBB1_17:                               # =>This Loop Header: Depth=1
                                        #     Child Loop BB1_24 Depth 2
	movq	%r11, %rbx
	shlq	$4, %rbx
	movq	(%rsi,%rbx), %r15
	testq	%r15, %r15
	je	.LBB1_27
# %bb.18:                               #   in Loop: Header=BB1_17 Depth=1
	movq	%r15, %r12
	andq	%rdi, %r12
	cmpq	%r15, %r12
	jne	.LBB1_27
# %bb.19:                               #   in Loop: Header=BB1_17 Depth=1
	testl	%r14d, %r14d
	je	.LBB1_21
# %bb.20:                               #   in Loop: Header=BB1_17 Depth=1
	movslq	%eax, %r14
	incl	%eax
	movb	$124, (%rcx,%r14)
.LBB1_21:                               #   in Loop: Header=BB1_17 Depth=1
	addq	%rsi, %rbx
	movq	8(%rbx), %r14
	movzbl	(%r14), %r15d
	testb	%r15b, %r15b
	je	.LBB1_26
# %bb.22:                               #   in Loop: Header=BB1_17 Depth=1
	cmpl	%r9d, %eax
	jge	.LBB1_26
# %bb.23:                               #   in Loop: Header=BB1_17 Depth=1
	movslq	%eax, %r12
	incq	%r12
	incq	%r14
	.p2align	4
.LBB1_24:                               #   Parent Loop BB1_17 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movb	%r15b, -1(%rcx,%r12)
	movzbl	(%r14), %r15d
	incl	%eax
	testb	%r15b, %r15b
	je	.LBB1_26
# %bb.25:                               #   in Loop: Header=BB1_24 Depth=2
	leaq	1(%r12), %r13
	incq	%r14
	cmpq	%r10, %r12
	movq	%r13, %r12
	jl	.LBB1_24
.LBB1_26:                               #   in Loop: Header=BB1_17 Depth=1
	movq	(%rbx), %rbx
	notq	%rbx
	andq	%rbx, %rdi
	movl	$1, %r14d
.LBB1_27:                               #   in Loop: Header=BB1_17 Depth=1
	incq	%r11
	cmpl	%r9d, %eax
	setl	%bl
	cmpq	%rdx, %r11
	jae	.LBB1_2
# %bb.28:                               #   in Loop: Header=BB1_17 Depth=1
	cmpl	%r9d, %eax
	jl	.LBB1_17
.LBB1_2:
	testq	%rdi, %rdi
	je	.LBB1_29
.LBB1_3:
	testl	%r14d, %r14d
	setne	%dl
	andb	%bl, %dl
	cmpb	$1, %dl
	je	.LBB1_4
# %bb.5:
	cmpl	%r9d, %eax
	jl	.LBB1_6
.LBB1_7:
	cmpl	%r9d, %eax
	jge	.LBB1_9
.LBB1_8:
	movslq	%eax, %rdx
	incl	%eax
	movb	$120, (%rcx,%rdx)
.LBB1_9:
	xorl	%esi, %esi
	leaq	put_hex.digits(%rip), %r10
	.p2align	4
.LBB1_10:                               # =>This Inner Loop Header: Depth=1
	movl	%edi, %edx
	andl	$15, %edx
	movzbl	(%rdx,%r10), %r11d
	leaq	1(%rsi), %rdx
	movb	%r11b, -64(%rbp,%rsi)
	cmpq	$16, %rdi
	jb	.LBB1_12
# %bb.11:                               #   in Loop: Header=BB1_10 Depth=1
	shrq	$4, %rdi
	cmpq	$15, %rsi
	movq	%rdx, %rsi
	jb	.LBB1_10
.LBB1_12:
	cmpl	%r9d, %eax
	jge	.LBB1_31
# %bb.13:
	movslq	%eax, %rdi
	movslq	%r9d, %rsi
	incq	%rdi
	.p2align	4
.LBB1_14:                               # =>This Inner Loop Header: Depth=1
	movzbl	-65(%rbp,%rdx), %r10d
	movb	%r10b, -1(%rcx,%rdi)
	incl	%eax
	cmpq	$2, %rdx
	jl	.LBB1_31
# %bb.15:                               #   in Loop: Header=BB1_14 Depth=1
	decq	%rdx
	leaq	1(%rdi), %r10
	cmpq	%rsi, %rdi
	movq	%r10, %rdi
	jl	.LBB1_14
	jmp	.LBB1_31
.LBB1_1:
	xorl	%r14d, %r14d
	testq	%rdi, %rdi
	jne	.LBB1_3
.LBB1_29:
	testl	%r14d, %r14d
	sete	%dl
	andb	%bl, %dl
	cmpb	$1, %dl
	jne	.LBB1_31
# %bb.30:
	movslq	%eax, %rdx
	incl	%eax
	movb	$48, (%rcx,%rdx)
.LBB1_31:
	movl	%eax, %edx
	cmpl	%r8d, %eax
	jl	.LBB1_33
# %bb.32:
	movl	%r9d, %edx
	testl	%r8d, %r8d
	jle	.LBB1_34
.LBB1_33:
	movslq	%edx, %rdx
	movb	$0, (%rcx,%rdx)
.LBB1_34:
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
.LBB1_4:
	movslq	%eax, %rdx
	incl	%eax
	movb	$124, (%rcx,%rdx)
	cmpl	%r9d, %eax
	jge	.LBB1_7
.LBB1_6:
	movslq	%eax, %rdx
	incl	%eax
	movb	$48, (%rcx,%rdx)
	cmpl	%r9d, %eax
	jl	.LBB1_8
	jmp	.LBB1_9
.Lfunc_end1:
	.size	decode_flags, .Lfunc_end1-decode_flags
                                        # -- End function
	.type	put_hex.digits,@object          # @put_hex.digits
	.section	.rodata.str1.16,"aMS",@progbits,1
	.p2align	4, 0x0
put_hex.digits:
	.asciz	"0123456789abcdef"
	.size	put_hex.digits, 17

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
