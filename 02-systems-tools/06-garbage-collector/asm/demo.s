	.file	"demo.c"
	.text
	.globl	in_heap                         # -- Begin function in_heap
	.p2align	4
	.type	in_heap,@function
in_heap:                                # @in_heap
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	cmpq	%rsi, %rdi
	setae	%al
	cmpq	%rdx, %rdi
	setb	%cl
	andb	%al, %cl
	movzbl	%cl, %eax
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
	movq	%rdi, %rax
	subq	%rsi, %rax
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
	movq	%rsi, %rcx
	movl	$1, %eax
	shlq	%cl, %rax
	shrq	$6, %rcx
	orq	%rax, (%rdi,%rcx,8)
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
	movq	%rsi, %rax
	shrq	$6, %rax
	movq	(%rdi,%rax,8), %rcx
	xorl	%eax, %eax
	btq	%rsi, %rcx
	setb	%al
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
	xorl	%ecx, %ecx
	testq	%rdx, %rdx
	jne	.LBB4_1
	jmp	.LBB4_5
	.p2align	4
.LBB4_3:                                #   in Loop: Header=BB4_1 Depth=1
	addq	%rax, %rcx
	incq	%rcx
	cmpq	%rdx, %rcx
	jae	.LBB4_5
.LBB4_1:                                # =>This Inner Loop Header: Depth=1
	movq	%rdx, %rax
	subq	%rcx, %rax
	shrq	%rax
	leaq	(%rax,%rcx), %r8
	leaq	(%r8,%r8,2), %r9
	cmpq	(%rsi,%r9,8), %rdi
	jae	.LBB4_3
# %bb.2:                                #   in Loop: Header=BB4_1 Depth=1
	movq	%r8, %rdx
	cmpq	%rdx, %rcx
	jb	.LBB4_1
.LBB4_5:
	testq	%rcx, %rcx
	je	.LBB4_6
# %bb.7:
	decq	%rcx
	leaq	(%rcx,%rcx,2), %rax
	movq	8(%rsi,%rax,8), %rdx
	addq	(%rsi,%rax,8), %rdx
	cmpq	%rdx, %rdi
	movq	$-1, %rax
	cmovbq	%rcx, %rax
	popq	%rbp
	retq
.LBB4_6:
	movq	$-1, %rax
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
	cmpq	%rsi, %rdi
	setb	%r10b
	cmpq	%rdx, %rdi
	setae	%dl
	xorl	%eax, %eax
	orb	%r10b, %dl
	jne	.LBB5_12
# %bb.1:
	xorl	%eax, %eax
	testq	%r8, %r8
	jne	.LBB5_2
	jmp	.LBB5_6
	.p2align	4
.LBB5_4:                                #   in Loop: Header=BB5_2 Depth=1
	addq	%rdx, %rax
	incq	%rax
	cmpq	%r8, %rax
	jae	.LBB5_6
.LBB5_2:                                # =>This Inner Loop Header: Depth=1
	movq	%r8, %rdx
	subq	%rax, %rdx
	shrq	%rdx
	leaq	(%rdx,%rax), %r10
	leaq	(%r10,%r10,2), %r11
	cmpq	(%rcx,%r11,8), %rdi
	jae	.LBB5_4
# %bb.3:                                #   in Loop: Header=BB5_2 Depth=1
	movq	%r10, %r8
	cmpq	%r8, %rax
	jb	.LBB5_2
.LBB5_6:
	testq	%rax, %rax
	je	.LBB5_7
# %bb.8:
	decq	%rax
	leaq	(%rax,%rax,2), %rdx
	movq	8(%rcx,%rdx,8), %r8
	addq	(%rcx,%rdx,8), %r8
	cmpq	%r8, %rdi
	movq	$-1, %rdx
	cmovbq	%rax, %rdx
	jmp	.LBB5_9
.LBB5_7:
	movq	$-1, %rdx
.LBB5_9:
	xorl	%eax, %eax
	testq	%rdx, %rdx
	js	.LBB5_12
# %bb.10:
	leaq	(%rdx,%rdx,2), %rdi
	movq	(%rcx,%rdi,8), %rcx
	subq	%rsi, %rcx
	movl	%ecx, %esi
	shrl	$4, %esi
	shrq	$10, %rcx
	movq	(%r9,%rcx,8), %rdi
	btq	%rsi, %rdi
	jae	.LBB5_11
.LBB5_12:
	retq
.LBB5_11:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	16(%rbp), %rax
	andl	$63, %esi
	btsq	%rsi, %rdi
	movq	%rdi, (%r9,%rcx,8)
	movq	%rdx, (%rax)
	movl	$1, %eax
	popq	%rbp
	retq
.Lfunc_end5:
	.size	mark_word, .Lfunc_end5-mark_word
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
