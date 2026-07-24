	.file	"demo.c"
	.text
	.globl	dq_push                         # -- Begin function dq_push
	.p2align	4
	.type	dq_push,@function
dq_push:                                # @dq_push
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	-8(%rbp), %rax
	movq	8(%rax), %rax
	movq	%rax, -32(%rbp)
	movq	-32(%rbp), %rax
	movq	%rax, -24(%rbp)
	movq	-8(%rbp), %rax
	movq	16(%rax), %rax
	movq	%rax, -40(%rbp)
	movq	-40(%rbp), %rax
	movq	8(%rax), %rdx
	movq	16(%rax), %rax
	movq	-24(%rbp), %rcx
	andq	%rdx, %rcx
	movq	-16(%rbp), %rdx
	movq	%rdx, -48(%rbp)
	movq	-48(%rbp), %rdx
	movq	%rdx, (%rax,%rcx,8)
	#MEMBARRIER
	movq	-8(%rbp), %rax
	movq	-24(%rbp), %rcx
	incq	%rcx
	movq	%rcx, -56(%rbp)
	movq	-56(%rbp), %rcx
	movq	%rcx, 8(%rax)
	popq	%rbp
	retq
.Lfunc_end0:
	.size	dq_push, .Lfunc_end0-dq_push
                                        # -- End function
	.globl	dq_take                         # -- Begin function dq_take
	.p2align	4
	.type	dq_take,@function
dq_take:                                # @dq_take
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -16(%rbp)
	movq	-16(%rbp), %rax
	movq	8(%rax), %rax
	movq	%rax, -32(%rbp)
	movq	-32(%rbp), %rax
	decq	%rax
	movq	%rax, -24(%rbp)
	movq	-16(%rbp), %rax
	movq	16(%rax), %rax
	movq	%rax, -40(%rbp)
	movq	-16(%rbp), %rax
	movq	-24(%rbp), %rcx
	movq	%rcx, -48(%rbp)
	movq	-48(%rbp), %rcx
	movq	%rcx, 8(%rax)
	mfence
	movq	-16(%rbp), %rax
	movq	(%rax), %rax
	movq	%rax, -64(%rbp)
	movq	-64(%rbp), %rax
	movq	%rax, -56(%rbp)
	movq	-56(%rbp), %rax
	cmpq	-24(%rbp), %rax
	jg	.LBB1_8
# %bb.1:
	movq	-40(%rbp), %rax
	movq	8(%rax), %rdx
	movq	16(%rax), %rax
	movq	-24(%rbp), %rcx
	andq	%rdx, %rcx
	movq	(%rax,%rcx,8), %rax
	movq	%rax, -80(%rbp)
	movq	-80(%rbp), %rax
	movq	%rax, -72(%rbp)
	movq	-56(%rbp), %rax
	cmpq	-24(%rbp), %rax
	jne	.LBB1_7
# %bb.2:
	movq	-16(%rbp), %rcx
	movq	-56(%rbp), %rax
	incq	%rax
	movq	%rax, -88(%rbp)
	movq	-56(%rbp), %rax
	movq	-88(%rbp), %rdx
	lock		cmpxchgq	%rdx, (%rcx)
	movq	%rax, %rcx
	sete	%al
	movb	%al, -121(%rbp)                 # 1-byte Spill
	movq	%rcx, -120(%rbp)                # 8-byte Spill
	testb	$1, %al
	jne	.LBB1_4
# %bb.3:
	movq	-120(%rbp), %rax                # 8-byte Reload
	movq	%rax, -56(%rbp)
.LBB1_4:
	movb	-121(%rbp), %al                 # 1-byte Reload
	andb	$1, %al
	movb	%al, -89(%rbp)
	testb	$1, -89(%rbp)
	jne	.LBB1_6
# %bb.5:
	movq	$0, -72(%rbp)
.LBB1_6:
	movq	-16(%rbp), %rax
	movq	-24(%rbp), %rcx
	incq	%rcx
	movq	%rcx, -104(%rbp)
	movq	-104(%rbp), %rcx
	movq	%rcx, 8(%rax)
.LBB1_7:
	movq	-72(%rbp), %rax
	movq	%rax, -8(%rbp)
	jmp	.LBB1_9
.LBB1_8:
	movq	-16(%rbp), %rax
	movq	-24(%rbp), %rcx
	incq	%rcx
	movq	%rcx, -112(%rbp)
	movq	-112(%rbp), %rcx
	movq	%rcx, 8(%rax)
	movq	$0, -8(%rbp)
.LBB1_9:
	movq	-8(%rbp), %rax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	dq_take, .Lfunc_end1-dq_take
                                        # -- End function
	.globl	dq_steal                        # -- Begin function dq_steal
	.p2align	4
	.type	dq_steal,@function
dq_steal:                               # @dq_steal
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -16(%rbp)
	movq	-16(%rbp), %rax
	movq	(%rax), %rax
	movq	%rax, -32(%rbp)
	movq	-32(%rbp), %rax
	movq	%rax, -24(%rbp)
	mfence
	movq	-16(%rbp), %rax
	movq	8(%rax), %rax
	movq	%rax, -48(%rbp)
	movq	-48(%rbp), %rax
	movq	%rax, -40(%rbp)
	movq	$0, -56(%rbp)
	movq	-24(%rbp), %rax
	cmpq	-40(%rbp), %rax
	jge	.LBB2_6
# %bb.1:
	movq	-16(%rbp), %rax
	movq	16(%rax), %rax
	movq	%rax, -64(%rbp)
	movq	-64(%rbp), %rax
	movq	8(%rax), %rdx
	movq	16(%rax), %rax
	movq	-24(%rbp), %rcx
	andq	%rdx, %rcx
	movq	(%rax,%rcx,8), %rax
	movq	%rax, -72(%rbp)
	movq	-72(%rbp), %rax
	movq	%rax, -56(%rbp)
	movq	-16(%rbp), %rcx
	movq	-24(%rbp), %rax
	incq	%rax
	movq	%rax, -80(%rbp)
	movq	-24(%rbp), %rax
	movq	-80(%rbp), %rdx
	lock		cmpxchgq	%rdx, (%rcx)
	movq	%rax, %rcx
	sete	%al
	movb	%al, -97(%rbp)                  # 1-byte Spill
	movq	%rcx, -96(%rbp)                 # 8-byte Spill
	testb	$1, %al
	jne	.LBB2_3
# %bb.2:
	movq	-96(%rbp), %rax                 # 8-byte Reload
	movq	%rax, -24(%rbp)
.LBB2_3:
	movb	-97(%rbp), %al                  # 1-byte Reload
	andb	$1, %al
	movb	%al, -81(%rbp)
	testb	$1, -81(%rbp)
	jne	.LBB2_5
# %bb.4:
	movq	$-1, %rax
	movq	%rax, -8(%rbp)
	jmp	.LBB2_7
.LBB2_5:
	jmp	.LBB2_6
.LBB2_6:
	movq	-56(%rbp), %rax
	movq	%rax, -8(%rbp)
.LBB2_7:
	movq	-8(%rbp), %rax
	popq	%rbp
	retq
.Lfunc_end2:
	.size	dq_steal, .Lfunc_end2-dq_steal
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
