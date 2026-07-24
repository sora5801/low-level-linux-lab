	.file	"demo.c"
	.text
	.globl	in_heap                         # -- Begin function in_heap
	.p2align	4
	.type	in_heap,@function
in_heap:                                # @in_heap
# %bb.0:
	cmpq	%rsi, %rdi
	setae	%al
	cmpq	%rdx, %rdi
	setb	%cl
	andb	%al, %cl
	movzbl	%cl, %eax
	retq
.Lfunc_end0:
	.size	in_heap, .Lfunc_end0-in_heap
                                        # -- End function
	.globl	granule_of                      # -- Begin function granule_of
	.p2align	4
	.type	granule_of,@function
granule_of:                             # @granule_of
# %bb.0:
	movq	%rdi, %rax
	subq	%rsi, %rax
	shrq	$4, %rax
	retq
.Lfunc_end1:
	.size	granule_of, .Lfunc_end1-granule_of
                                        # -- End function
	.globl	mark_set                        # -- Begin function mark_set
	.p2align	4
	.type	mark_set,@function
mark_set:                               # @mark_set
# %bb.0:
	movq	%rsi, %rcx
	movl	$1, %eax
	shlq	%cl, %rax
	shrq	$6, %rcx
	orq	%rax, (%rdi,%rcx,8)
	retq
.Lfunc_end2:
	.size	mark_set, .Lfunc_end2-mark_set
                                        # -- End function
	.globl	mark_test                       # -- Begin function mark_test
	.p2align	4
	.type	mark_test,@function
mark_test:                              # @mark_test
# %bb.0:
	movq	%rsi, %rax
	shrq	$6, %rax
	movq	(%rdi,%rax,8), %rcx
	xorl	%eax, %eax
	btq	%rsi, %rcx
	setb	%al
	retq
.Lfunc_end3:
	.size	mark_test, .Lfunc_end3-mark_test
                                        # -- End function
	.globl	obj_containing                  # -- Begin function obj_containing
	.p2align	4
	.type	obj_containing,@function
obj_containing:                         # @obj_containing
# %bb.0:
	movq	$-1, %rax
	testq	%rdx, %rdx
	je	.LBB4_8
# %bb.1:
	xorl	%ecx, %ecx
	jmp	.LBB4_2
	.p2align	4
.LBB4_4:                                #   in Loop: Header=BB4_2 Depth=1
	addq	%r8, %rcx
	incq	%rcx
	cmpq	%rdx, %rcx
	jae	.LBB4_6
.LBB4_2:                                # =>This Inner Loop Header: Depth=1
	movq	%rdx, %r8
	subq	%rcx, %r8
	shrq	%r8
	leaq	(%r8,%rcx), %r9
	leaq	(%r9,%r9,2), %r10
	cmpq	(%rsi,%r10,8), %rdi
	jae	.LBB4_4
# %bb.3:                                #   in Loop: Header=BB4_2 Depth=1
	movq	%r9, %rdx
	cmpq	%rdx, %rcx
	jb	.LBB4_2
.LBB4_6:
	testq	%rcx, %rcx
	je	.LBB4_8
# %bb.7:
	decq	%rcx
	leaq	(%rcx,%rcx,2), %rax
	movq	8(%rsi,%rax,8), %rdx
	addq	(%rsi,%rax,8), %rdx
	cmpq	%rdx, %rdi
	movq	$-1, %rax
	cmovbq	%rcx, %rax
.LBB4_8:
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
	setb	%al
	cmpq	%rdx, %rdi
	setae	%dl
	orb	%al, %dl
	testq	%r8, %r8
	sete	%r10b
	xorl	%eax, %eax
	orb	%dl, %r10b
	jne	.LBB5_11
# %bb.1:
	pushq	%rbx
	movq	16(%rsp), %rdx
	xorl	%r10d, %r10d
	jmp	.LBB5_2
	.p2align	4
.LBB5_4:                                #   in Loop: Header=BB5_2 Depth=1
	addq	%rax, %r10
	incq	%r10
	cmpq	%r8, %r10
	jae	.LBB5_6
.LBB5_2:                                # =>This Inner Loop Header: Depth=1
	movq	%r8, %rax
	subq	%r10, %rax
	shrq	%rax
	leaq	(%rax,%r10), %r11
	leaq	(%r11,%r11,2), %rbx
	cmpq	(%rcx,%rbx,8), %rdi
	jae	.LBB5_4
# %bb.3:                                #   in Loop: Header=BB5_2 Depth=1
	movq	%r11, %r8
	cmpq	%r8, %r10
	jb	.LBB5_2
.LBB5_6:
	xorl	%eax, %eax
	testq	%r10, %r10
	popq	%rbx
	je	.LBB5_11
# %bb.7:
	decq	%r10
	js	.LBB5_11
# %bb.8:
	leaq	(%r10,%r10,2), %r11
	movq	(%rcx,%r11,8), %r8
	movq	8(%rcx,%r11,8), %rcx
	addq	%r8, %rcx
	cmpq	%rcx, %rdi
	jae	.LBB5_11
# %bb.9:
	subq	%rsi, %r8
	movl	%r8d, %ecx
	shrl	$4, %ecx
	shrq	$10, %r8
	movq	(%r9,%r8,8), %rsi
	movl	$1, %edi
	shlq	%cl, %rdi
	btq	%rcx, %rsi
	jae	.LBB5_10
.LBB5_11:
	retq
.LBB5_10:
	orq	%rsi, %rdi
	movq	%rdi, (%r9,%r8,8)
	movq	%r10, (%rdx)
	movl	$1, %eax
	retq
.Lfunc_end5:
	.size	mark_word, .Lfunc_end5-mark_word
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
