	.file	"demo.c"
	.text
	.globl	u64_to_dec                      # -- Begin function u64_to_dec
	.p2align	4
	.type	u64_to_dec,@function
u64_to_dec:                             # @u64_to_dec
# %bb.0:
	testq	%rdi, %rdi
	je	.LBB0_7
# %bb.1:
	pushq	%rbp
	movq	%rsp, %rbp
	xorl	%ecx, %ecx
	movabsq	$-3689348814741910323, %r8      # imm = 0xCCCCCCCCCCCCCCCD
	.p2align	4
.LBB0_2:                                # =>This Inner Loop Header: Depth=1
	movq	%rdi, %rax
	mulq	%r8
	shrq	$3, %rdx
	imull	$246, %edx, %eax
	addl	%edi, %eax
	addb	$48, %al
	movl	%ecx, %r9d
	incl	%ecx
	movb	%al, -32(%rbp,%r9)
	cmpq	$10, %rdi
	movq	%rdx, %rdi
	jae	.LBB0_2
# %bb.3:
	movl	%ecx, %eax
	subl	$1, %eax
	jb	.LBB0_6
# %bb.4:
	movl	%ecx, %edx
	xorl	%edi, %edi
	.p2align	4
.LBB0_5:                                # =>This Inner Loop Header: Depth=1
	movl	%eax, %r8d
	movzbl	-32(%rbp,%r8), %r8d
	movb	%r8b, (%rsi,%rdi)
	incq	%rdi
	decl	%eax
	cmpq	%rdi, %rdx
	jne	.LBB0_5
.LBB0_6:
	popq	%rbp
	movl	%ecx, %eax
	retq
.LBB0_7:
	movb	$48, (%rsi)
	movl	$1, %eax
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
	testq	%rdi, %rdi
	js	.LBB1_1
# %bb.7:
	je	.LBB1_14
# %bb.8:
	xorl	%ecx, %ecx
	movabsq	$-3689348814741910323, %r8      # imm = 0xCCCCCCCCCCCCCCCD
	.p2align	4
.LBB1_9:                                # =>This Inner Loop Header: Depth=1
	movq	%rdi, %rax
	mulq	%r8
	shrq	$3, %rdx
	imull	$246, %edx, %eax
	addl	%edi, %eax
	addb	$48, %al
	movl	%ecx, %r9d
	incl	%ecx
	movb	%al, -32(%rbp,%r9)
	cmpq	$10, %rdi
	movq	%rdx, %rdi
	jae	.LBB1_9
# %bb.10:
	movl	%ecx, %eax
	subl	$1, %eax
	jb	.LBB1_13
# %bb.11:
	movl	%ecx, %edx
	xorl	%edi, %edi
	.p2align	4
.LBB1_12:                               # =>This Inner Loop Header: Depth=1
	movl	%eax, %r8d
	movzbl	-32(%rbp,%r8), %r8d
	movb	%r8b, (%rsi,%rdi)
	incq	%rdi
	decl	%eax
	cmpq	%rdi, %rdx
	jne	.LBB1_12
	jmp	.LBB1_13
.LBB1_1:
	movb	$45, (%rsi)
	negq	%rdi
	xorl	%ecx, %ecx
	movabsq	$-3689348814741910323, %r8      # imm = 0xCCCCCCCCCCCCCCCD
	.p2align	4
.LBB1_2:                                # =>This Inner Loop Header: Depth=1
	movq	%rdi, %rax
	mulq	%r8
	shrq	$3, %rdx
	imull	$246, %edx, %eax
	addl	%edi, %eax
	addb	$48, %al
	movl	%ecx, %r9d
	incl	%ecx
	movb	%al, -32(%rbp,%r9)
	cmpq	$10, %rdi
	movq	%rdx, %rdi
	jae	.LBB1_2
# %bb.3:
	movl	%ecx, %eax
	subl	$1, %eax
	jb	.LBB1_6
# %bb.4:
	movl	%ecx, %edx
	xorl	%edi, %edi
	.p2align	4
.LBB1_5:                                # =>This Inner Loop Header: Depth=1
	movl	%eax, %r8d
	movzbl	-32(%rbp,%r8), %r8d
	movb	%r8b, 1(%rsi,%rdi)
	incq	%rdi
	decl	%eax
	cmpq	%rdi, %rdx
	jne	.LBB1_5
.LBB1_6:
	incl	%ecx
	jmp	.LBB1_13
.LBB1_14:
	movb	$48, (%rsi)
	movl	$1, %ecx
.LBB1_13:
	movl	%ecx, %eax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	i64_to_dec, .Lfunc_end1-i64_to_dec
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
