	.file	"demo.c"
	.text
	.globl	apply_reloc                     # -- Begin function apply_reloc
	.p2align	4
	.type	apply_reloc,@function
apply_reloc:                            # @apply_reloc
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	$2, %eax
	cmpl	$3, %esi
	jle	.LBB0_1
# %bb.4:
	cmpl	$4, %esi
	je	.LBB0_3
# %bb.5:
	cmpl	$10, %esi
	je	.LBB0_10
# %bb.6:
	cmpl	$11, %esi
	jne	.LBB0_13
# %bb.7:
	addq	%rdx, %rcx
	jmp	.LBB0_8
.LBB0_1:
	cmpl	$1, %esi
	je	.LBB0_9
# %bb.2:
	cmpl	$2, %esi
	jne	.LBB0_13
.LBB0_3:
	addq	%rdx, %rcx
	subq	%r8, %rcx
.LBB0_8:
	movslq	%ecx, %rdx
	movl	$1, %eax
	cmpq	%rcx, %rdx
	je	.LBB0_11
.LBB0_13:
	popq	%rbp
	retq
.LBB0_10:
	addq	%rdx, %rcx
	movq	%rcx, %rax
	shrq	$32, %rax
	movl	$1, %eax
	jne	.LBB0_13
.LBB0_11:
	movb	%cl, (%rdi)
	movb	%ch, 1(%rdi)
	movl	%ecx, %eax
	shrl	$16, %eax
	movb	%al, 2(%rdi)
	shrq	$24, %rcx
	movl	$3, %eax
	jmp	.LBB0_12
.LBB0_9:
	addq	%rdx, %rcx
	movb	%cl, (%rdi)
	movb	%ch, 1(%rdi)
	movl	%ecx, %eax
	shrl	$16, %eax
	movb	%al, 2(%rdi)
	movl	%ecx, %eax
	shrl	$24, %eax
	movb	%al, 3(%rdi)
	movq	%rcx, %rax
	shrq	$32, %rax
	movb	%al, 4(%rdi)
	movq	%rcx, %rax
	shrq	$40, %rax
	movb	%al, 5(%rdi)
	movq	%rcx, %rax
	shrq	$48, %rax
	movb	%al, 6(%rdi)
	shrq	$56, %rcx
	movl	$7, %eax
.LBB0_12:
	movb	%cl, (%rdi,%rax)
	xorl	%eax, %eax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	apply_reloc, .Lfunc_end0-apply_reloc
                                        # -- End function
	.globl	demo_selfcheck                  # -- Begin function demo_selfcheck
	.p2align	4
	.type	demo_selfcheck,@function
demo_selfcheck:                         # @demo_selfcheck
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	xorps	%xmm0, %xmm0
	movaps	%xmm0, -16(%rbp)
	movl	$143336, -16(%rbp)              # imm = 0x22FE8
	movb	$0, -12(%rbp)
	movw	$4660, -8(%rbp)                 # imm = 0x1234
	movb	$64, -6(%rbp)
	movl	$0, -5(%rbp)
	movb	$0, -1(%rbp)
	xorl	%ecx, %ecx
	xorl	%eax, %eax
	.p2align	4
.LBB1_1:                                # =>This Inner Loop Header: Depth=1
	imulq	$131, %rax, %rdx
	movzbl	-16(%rbp,%rcx), %eax
	addq	%rdx, %rax
	incq	%rcx
	cmpq	$16, %rcx
	jne	.LBB1_1
# %bb.2:
	popq	%rbp
	retq
.Lfunc_end1:
	.size	demo_selfcheck, .Lfunc_end1-demo_selfcheck
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
