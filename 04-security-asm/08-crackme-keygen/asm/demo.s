	.file	"demo.c"
	.text
	.globl	key_from_name                   # -- Begin function key_from_name
	.p2align	4
	.type	key_from_name,@function
key_from_name:                          # @key_from_name
# %bb.0:
	movabsq	$-3750763034362895579, %r8      # imm = 0xCBF29CE484222325
	movzbl	(%rdi), %esi
	testb	%sil, %sil
	je	.LBB0_1
# %bb.3:
	pushq	%rbp
	movq	%rsp, %rbp
	incq	%rdi
	movabsq	$1099511628211, %rax            # imm = 0x100000001B3
	movabsq	$25214903917, %rcx              # imm = 0x5DEECE66D
	.p2align	4
.LBB0_4:                                # =>This Inner Loop Header: Depth=1
	movzbl	%sil, %edx
	xorq	%r8, %rdx
	imulq	%rax, %rdx
	rolq	$7, %rdx
	xorq	%rcx, %rdx
	movzbl	(%rdi), %esi
	incq	%rdi
	movq	%rdx, %r8
	testb	%sil, %sil
	jne	.LBB0_4
# %bb.5:
	popq	%rbp
	jmp	.LBB0_2
.LBB0_1:
	movq	%r8, %rdx
.LBB0_2:
	movq	%rdx, %rax
	shrq	$33, %rax
	xorq	%rdx, %rax
	movabsq	$-49064778989728563, %rcx       # imm = 0xFF51AFD7ED558CCD
	imulq	%rax, %rcx
	movq	%rcx, %rax
	shrq	$29, %rax
	xorq	%rcx, %rax
	movabsq	$-4265267296055464877, %rcx     # imm = 0xC4CEB9FE1A85EC53
	imulq	%rax, %rcx
	movq	%rcx, %rax
	shrq	$33, %rax
	xorq	%rcx, %rax
	retq
.Lfunc_end0:
	.size	key_from_name, .Lfunc_end0-key_from_name
                                        # -- End function
	.globl	validate                        # -- Begin function validate
	.p2align	4
	.type	validate,@function
validate:                               # @validate
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movabsq	$-3750763034362895579, %r9      # imm = 0xCBF29CE484222325
	movzbl	(%rdi), %r8d
	testb	%r8b, %r8b
	je	.LBB1_1
# %bb.2:
	incq	%rdi
	movabsq	$1099511628211, %rax            # imm = 0x100000001B3
	movabsq	$25214903917, %rcx              # imm = 0x5DEECE66D
	.p2align	4
.LBB1_3:                                # =>This Inner Loop Header: Depth=1
	movzbl	%r8b, %edx
	xorq	%r9, %rdx
	imulq	%rax, %rdx
	rolq	$7, %rdx
	xorq	%rcx, %rdx
	movzbl	(%rdi), %r8d
	incq	%rdi
	movq	%rdx, %r9
	testb	%r8b, %r8b
	jne	.LBB1_3
	jmp	.LBB1_4
.LBB1_1:
	movq	%r9, %rdx
.LBB1_4:
	movq	%rdx, %rax
	shrq	$33, %rax
	xorq	%rdx, %rax
	movabsq	$-49064778989728563, %rcx       # imm = 0xFF51AFD7ED558CCD
	imulq	%rax, %rcx
	movq	%rcx, %rax
	shrq	$29, %rax
	xorq	%rcx, %rax
	movabsq	$-4265267296055464877, %rcx     # imm = 0xC4CEB9FE1A85EC53
	imulq	%rax, %rcx
	movq	%rcx, %rax
	shrq	$33, %rax
	xorq	%rcx, %rax
	xorl	%edx, %edx
	movl	$48, %ecx
	leaq	format_serial.HEX(%rip), %rdi
	jmp	.LBB1_5
	.p2align	4
.LBB1_7:                                #   in Loop: Header=BB1_5 Depth=1
	addl	$5, %edx
	movl	%r8d, %r8d
	movb	$45, -32(%rbp,%r8)
.LBB1_8:                                #   in Loop: Header=BB1_5 Depth=1
	addq	$-16, %rcx
	cmpq	$-16, %rcx
	je	.LBB1_9
.LBB1_5:                                # =>This Inner Loop Header: Depth=1
	movq	%rax, %r8
	shrq	%cl, %r8
	movl	%r8d, %r9d
	shrl	$12, %r9d
	andl	$15, %r9d
	movzbl	(%r9,%rdi), %r9d
	leal	1(%rdx), %r10d
	movl	%edx, %r11d
	movb	%r9b, -32(%rbp,%r11)
	movl	%r8d, %r9d
	shrl	$8, %r9d
	andl	$15, %r9d
	movzbl	(%r9,%rdi), %r9d
	leal	2(%rdx), %r11d
	movb	%r9b, -32(%rbp,%r10)
	movl	%r8d, %r9d
	shrl	$4, %r9d
	andl	$15, %r9d
	movzbl	(%r9,%rdi), %r9d
	leal	3(%rdx), %r10d
	movb	%r9b, -32(%rbp,%r11)
	andl	$15, %r8d
	movzbl	(%r8,%rdi), %r9d
	leal	4(%rdx), %r8d
	movb	%r9b, -32(%rbp,%r10)
	testq	%rcx, %rcx
	jne	.LBB1_7
# %bb.6:                                #   in Loop: Header=BB1_5 Depth=1
	movl	%r8d, %edx
	jmp	.LBB1_8
.LBB1_9:
	movl	%edx, %eax
	movb	$0, -32(%rbp,%rax)
	movq	$-1, %rcx
	.p2align	4
.LBB1_10:                               # =>This Inner Loop Header: Depth=1
	cmpb	$0, 1(%rsi,%rcx)
	leaq	1(%rcx), %rcx
	jne	.LBB1_10
# %bb.11:
	xorl	%eax, %eax
	cmpl	$19, %ecx
	jne	.LBB1_15
# %bb.12:
	xorl	%eax, %eax
	xorl	%ecx, %ecx
	.p2align	4
.LBB1_13:                               # =>This Inner Loop Header: Depth=1
	movzbl	-32(%rbp,%rax), %edx
	xorb	(%rsi,%rax), %dl
	movzbl	%dl, %edx
	orl	%edx, %ecx
	incq	%rax
	cmpq	$19, %rax
	jne	.LBB1_13
# %bb.14:
	xorl	%eax, %eax
	testl	%ecx, %ecx
	sete	%al
.LBB1_15:
	popq	%rbp
	retq
.Lfunc_end1:
	.size	validate, .Lfunc_end1-validate
                                        # -- End function
	.type	format_serial.HEX,@object       # @format_serial.HEX
	.section	.rodata.str1.16,"aMS",@progbits,1
	.p2align	4, 0x0
format_serial.HEX:
	.asciz	"0123456789ABCDEF"
	.size	format_serial.HEX, 17

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
