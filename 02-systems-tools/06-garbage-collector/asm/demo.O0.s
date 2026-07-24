	.file	"demo.c"
	.text
	.globl	in_heap                         # -- Begin function in_heap
	.p2align	4
	.type	in_heap,@function
in_heap:                                # @in_heap
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	%rdx, -24(%rbp)
	movq	-8(%rbp), %rcx
	xorl	%eax, %eax
                                        # kill: def $al killed $al killed $eax
	cmpq	-16(%rbp), %rcx
	movb	%al, -25(%rbp)                  # 1-byte Spill
	jb	.LBB0_2
# %bb.1:
	movq	-8(%rbp), %rax
	cmpq	-24(%rbp), %rax
	setb	%al
	movb	%al, -25(%rbp)                  # 1-byte Spill
.LBB0_2:
	movb	-25(%rbp), %al                  # 1-byte Reload
	andb	$1, %al
	movzbl	%al, %eax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	in_heap, .Lfunc_end0-in_heap
                                        # -- End function
	.globl	granule_of                      # -- Begin function granule_of
	.p2align	4
	.type	granule_of,@function
granule_of:                             # @granule_of
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	-8(%rbp), %rax
	movq	-16(%rbp), %rcx
	subq	%rcx, %rax
	shrq	$4, %rax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	granule_of, .Lfunc_end1-granule_of
                                        # -- End function
	.globl	mark_set                        # -- Begin function mark_set
	.p2align	4
	.type	mark_set,@function
mark_set:                               # @mark_set
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	-16(%rbp), %rcx
	andq	$63, %rcx
	movl	$1, %edx
                                        # kill: def $cl killed $rcx
	shlq	%cl, %rdx
	movq	-8(%rbp), %rax
	movq	-16(%rbp), %rcx
	shrq	$6, %rcx
	orq	(%rax,%rcx,8), %rdx
	movq	%rdx, (%rax,%rcx,8)
	popq	%rbp
	retq
.Lfunc_end2:
	.size	mark_set, .Lfunc_end2-mark_set
                                        # -- End function
	.globl	mark_test                       # -- Begin function mark_test
	.p2align	4
	.type	mark_test,@function
mark_test:                              # @mark_test
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	-8(%rbp), %rax
	movq	-16(%rbp), %rcx
	shrq	$6, %rcx
	movq	(%rax,%rcx,8), %rax
	movq	-16(%rbp), %rcx
	andq	$63, %rcx
                                        # kill: def $cl killed $rcx
	shrq	%cl, %rax
	andq	$1, %rax
                                        # kill: def $eax killed $eax killed $rax
	popq	%rbp
	retq
.Lfunc_end3:
	.size	mark_test, .Lfunc_end3-mark_test
                                        # -- End function
	.globl	obj_containing                  # -- Begin function obj_containing
	.p2align	4
	.type	obj_containing,@function
obj_containing:                         # @obj_containing
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -16(%rbp)
	movq	%rsi, -24(%rbp)
	movq	%rdx, -32(%rbp)
	movq	$0, -40(%rbp)
	movq	-32(%rbp), %rax
	movq	%rax, -48(%rbp)
.LBB4_1:                                # =>This Inner Loop Header: Depth=1
	movq	-40(%rbp), %rax
	cmpq	-48(%rbp), %rax
	jae	.LBB4_6
# %bb.2:                                #   in Loop: Header=BB4_1 Depth=1
	movq	-40(%rbp), %rax
	movq	-48(%rbp), %rcx
	subq	-40(%rbp), %rcx
	shrq	%rcx
	addq	%rcx, %rax
	movq	%rax, -56(%rbp)
	movq	-24(%rbp), %rax
	imulq	$24, -56(%rbp), %rcx
	addq	%rcx, %rax
	movq	(%rax), %rax
	cmpq	-16(%rbp), %rax
	ja	.LBB4_4
# %bb.3:                                #   in Loop: Header=BB4_1 Depth=1
	movq	-56(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -40(%rbp)
	jmp	.LBB4_5
.LBB4_4:                                #   in Loop: Header=BB4_1 Depth=1
	movq	-56(%rbp), %rax
	movq	%rax, -48(%rbp)
.LBB4_5:                                #   in Loop: Header=BB4_1 Depth=1
	jmp	.LBB4_1
.LBB4_6:
	cmpq	$0, -40(%rbp)
	jne	.LBB4_8
# %bb.7:
	movq	$-1, -8(%rbp)
	jmp	.LBB4_11
.LBB4_8:
	movq	-40(%rbp), %rax
	subq	$1, %rax
	movq	%rax, -64(%rbp)
	movq	-16(%rbp), %rax
	movq	-24(%rbp), %rcx
	imulq	$24, -64(%rbp), %rdx
	addq	%rdx, %rcx
	movq	(%rcx), %rcx
	movq	-24(%rbp), %rdx
	imulq	$24, -64(%rbp), %rsi
	addq	%rsi, %rdx
	addq	8(%rdx), %rcx
	cmpq	%rcx, %rax
	jae	.LBB4_10
# %bb.9:
	movq	-64(%rbp), %rax
	movq	%rax, -8(%rbp)
	jmp	.LBB4_11
.LBB4_10:
	movq	$-1, -8(%rbp)
.LBB4_11:
	movq	-8(%rbp), %rax
	popq	%rbp
	retq
.Lfunc_end4:
	.size	obj_containing, .Lfunc_end4-obj_containing
                                        # -- End function
	.globl	mark_word                       # -- Begin function mark_word
	.p2align	4
	.type	mark_word,@function
mark_word:                              # @mark_word
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$80, %rsp
	movq	16(%rbp), %rax
	movq	%rdi, -16(%rbp)
	movq	%rsi, -24(%rbp)
	movq	%rdx, -32(%rbp)
	movq	%rcx, -40(%rbp)
	movq	%r8, -48(%rbp)
	movq	%r9, -56(%rbp)
	movq	-16(%rbp), %rdi
	movq	-24(%rbp), %rsi
	movq	-32(%rbp), %rdx
	callq	in_heap
	cmpl	$0, %eax
	jne	.LBB5_2
# %bb.1:
	movl	$0, -4(%rbp)
	jmp	.LBB5_7
.LBB5_2:
	movq	-16(%rbp), %rdi
	movq	-40(%rbp), %rsi
	movq	-48(%rbp), %rdx
	callq	obj_containing
	movq	%rax, -64(%rbp)
	cmpq	$0, -64(%rbp)
	jge	.LBB5_4
# %bb.3:
	movl	$0, -4(%rbp)
	jmp	.LBB5_7
.LBB5_4:
	movq	-40(%rbp), %rax
	imulq	$24, -64(%rbp), %rcx
	addq	%rcx, %rax
	movq	(%rax), %rdi
	movq	-24(%rbp), %rsi
	callq	granule_of
	movq	%rax, -72(%rbp)
	movq	-56(%rbp), %rdi
	movq	-72(%rbp), %rsi
	callq	mark_test
	cmpl	$0, %eax
	je	.LBB5_6
# %bb.5:
	movl	$0, -4(%rbp)
	jmp	.LBB5_7
.LBB5_6:
	movq	-56(%rbp), %rdi
	movq	-72(%rbp), %rsi
	callq	mark_set
	movq	-64(%rbp), %rcx
	movq	16(%rbp), %rax
	movq	%rcx, (%rax)
	movl	$1, -4(%rbp)
.LBB5_7:
	movl	-4(%rbp), %eax
	addq	$80, %rsp
	popq	%rbp
	retq
.Lfunc_end5:
	.size	mark_word, .Lfunc_end5-mark_word
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym in_heap
	.addrsig_sym granule_of
	.addrsig_sym mark_set
	.addrsig_sym mark_test
	.addrsig_sym obj_containing
