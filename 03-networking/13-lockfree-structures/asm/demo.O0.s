	.file	"demo.c"
	.text
	.globl	demo_push                       # -- Begin function demo_push
	.p2align	4
	.type	demo_push,@function
demo_push:                              # @demo_push
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$80, %rsp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	-8(%rbp), %rax
	movq	(%rax), %rax
	movq	%rax, -32(%rbp)
	movq	-32(%rbp), %rax
	movq	%rax, -24(%rbp)
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	movq	-24(%rbp), %rdi
	callq	ptr_of
	movq	%rax, %rcx
	movq	-16(%rbp), %rax
	movq	%rcx, (%rax)
	movq	-16(%rbp), %rax
	movq	%rax, -64(%rbp)                 # 8-byte Spill
	movq	-24(%rbp), %rdi
	callq	tag_of
	movq	-64(%rbp), %rdi                 # 8-byte Reload
	movq	%rax, %rsi
	addq	$1, %rsi
	callq	pack
	movq	%rax, -40(%rbp)
# %bb.2:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-8(%rbp), %rcx
	movq	-40(%rbp), %rax
	movq	%rax, -48(%rbp)
	movq	-24(%rbp), %rax
	movq	-48(%rbp), %rdx
	lock		cmpxchgq	%rdx, (%rcx)
	movq	%rax, %rcx
	sete	%al
	movb	%al, -73(%rbp)                  # 1-byte Spill
	movq	%rcx, -72(%rbp)                 # 8-byte Spill
	testb	$1, %al
	jne	.LBB0_4
# %bb.3:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-72(%rbp), %rax                 # 8-byte Reload
	movq	%rax, -24(%rbp)
.LBB0_4:                                #   in Loop: Header=BB0_1 Depth=1
	movb	-73(%rbp), %al                  # 1-byte Reload
	andb	$1, %al
	movb	%al, -49(%rbp)
	movb	-49(%rbp), %al
	xorb	$-1, %al
	testb	$1, %al
	jne	.LBB0_1
# %bb.5:
	addq	$80, %rsp
	popq	%rbp
	retq
.Lfunc_end0:
	.size	demo_push, .Lfunc_end0-demo_push
                                        # -- End function
	.p2align	4                               # -- Begin function ptr_of
	.type	ptr_of,@function
ptr_of:                                 # @ptr_of
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movabsq	$281474976710655, %rax          # imm = 0xFFFFFFFFFFFF
	andq	-8(%rbp), %rax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	ptr_of, .Lfunc_end1-ptr_of
                                        # -- End function
	.p2align	4                               # -- Begin function pack
	.type	pack,@function
pack:                                   # @pack
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	-16(%rbp), %rax
	shlq	$48, %rax
	movq	-8(%rbp), %rcx
	movabsq	$281474976710655, %rdx          # imm = 0xFFFFFFFFFFFF
	andq	%rdx, %rcx
	orq	%rcx, %rax
	popq	%rbp
	retq
.Lfunc_end2:
	.size	pack, .Lfunc_end2-pack
                                        # -- End function
	.p2align	4                               # -- Begin function tag_of
	.type	tag_of,@function
tag_of:                                 # @tag_of
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rax
	shrq	$48, %rax
	popq	%rbp
	retq
.Lfunc_end3:
	.size	tag_of, .Lfunc_end3-tag_of
                                        # -- End function
	.globl	demo_pop                        # -- Begin function demo_pop
	.p2align	4
	.type	demo_pop,@function
demo_pop:                               # @demo_pop
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$96, %rsp
	movq	%rdi, -16(%rbp)
	movq	-16(%rbp), %rax
	movq	(%rax), %rax
	movq	%rax, -32(%rbp)
	movq	-32(%rbp), %rax
	movq	%rax, -24(%rbp)
.LBB4_1:                                # =>This Inner Loop Header: Depth=1
	movq	-24(%rbp), %rdi
	callq	ptr_of
	movq	%rax, -48(%rbp)
	cmpq	$0, -48(%rbp)
	jne	.LBB4_3
# %bb.2:
	movq	$0, -8(%rbp)
	jmp	.LBB4_8
.LBB4_3:                                #   in Loop: Header=BB4_1 Depth=1
	movq	-48(%rbp), %rax
	movq	(%rax), %rax
	movq	%rax, -72(%rbp)                 # 8-byte Spill
	movq	-24(%rbp), %rdi
	callq	tag_of
	movq	-72(%rbp), %rdi                 # 8-byte Reload
	movq	%rax, %rsi
	addq	$1, %rsi
	callq	pack
	movq	%rax, -40(%rbp)
# %bb.4:                                #   in Loop: Header=BB4_1 Depth=1
	movq	-16(%rbp), %rcx
	movq	-40(%rbp), %rax
	movq	%rax, -56(%rbp)
	movq	-24(%rbp), %rax
	movq	-56(%rbp), %rdx
	lock		cmpxchgq	%rdx, (%rcx)
	movq	%rax, %rcx
	sete	%al
	movb	%al, -81(%rbp)                  # 1-byte Spill
	movq	%rcx, -80(%rbp)                 # 8-byte Spill
	testb	$1, %al
	jne	.LBB4_6
# %bb.5:                                #   in Loop: Header=BB4_1 Depth=1
	movq	-80(%rbp), %rax                 # 8-byte Reload
	movq	%rax, -24(%rbp)
.LBB4_6:                                #   in Loop: Header=BB4_1 Depth=1
	movb	-81(%rbp), %al                  # 1-byte Reload
	andb	$1, %al
	movb	%al, -57(%rbp)
	movb	-57(%rbp), %al
	xorb	$-1, %al
	testb	$1, %al
	jne	.LBB4_1
# %bb.7:
	movq	-48(%rbp), %rax
	movq	%rax, -8(%rbp)
.LBB4_8:
	movq	-8(%rbp), %rax
	addq	$96, %rsp
	popq	%rbp
	retq
.Lfunc_end4:
	.size	demo_pop, .Lfunc_end4-demo_pop
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym ptr_of
	.addrsig_sym pack
	.addrsig_sym tag_of
