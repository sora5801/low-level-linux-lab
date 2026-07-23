	.file	"demo.c"
	.text
	.globl	fp_unwind                       # -- Begin function fp_unwind
	.p2align	4
	.type	fp_unwind,@function
fp_unwind:                              # @fp_unwind
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movslq	%r8d, %r8
	xorl	%eax, %eax
	.p2align	4
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	cmpq	%rsi, %rdi
	jb	.LBB0_6
# %bb.2:                                #   in Loop: Header=BB0_1 Depth=1
	cmpq	%rdx, %rdi
	jae	.LBB0_6
# %bb.3:                                #   in Loop: Header=BB0_1 Depth=1
	movl	%edi, %r9d
	andl	$7, %r9d
	jne	.LBB0_6
# %bb.4:                                #   in Loop: Header=BB0_1 Depth=1
	cmpq	%r8, %rax
	jge	.LBB0_6
# %bb.5:                                #   in Loop: Header=BB0_1 Depth=1
	movq	(%rdi), %r9
	movq	8(%rdi), %r10
	movq	%r10, (%rcx,%rax,8)
	incq	%rax
	cmpq	%rdi, %r9
	cmovaq	%r9, %rdi
	ja	.LBB0_1
.LBB0_6:
                                        # kill: def $eax killed $eax killed $rax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	fp_unwind, .Lfunc_end0-fp_unwind
                                        # -- End function
	.globl	rb_avail                        # -- Begin function rb_avail
	.p2align	4
	.type	rb_avail,@function
rb_avail:                               # @rb_avail
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, %rax
	subq	%rsi, %rax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	rb_avail, .Lfunc_end1-rb_avail
                                        # -- End function
	.globl	rb_offset                       # -- Begin function rb_offset
	.p2align	4
	.type	rb_offset,@function
rb_offset:                              # @rb_offset
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	leaq	-1(%rsi), %rax
	andq	%rdi, %rax
	popq	%rbp
	retq
.Lfunc_end2:
	.size	rb_offset, .Lfunc_end2-rb_offset
                                        # -- End function
	.globl	rb_first_chunk                  # -- Begin function rb_first_chunk
	.p2align	4
	.type	rb_first_chunk,@function
rb_first_chunk:                         # @rb_first_chunk
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdx, %rax
	subq	%rdi, %rax
	cmpq	%rax, %rsi
	cmovbq	%rsi, %rax
	popq	%rbp
	retq
.Lfunc_end3:
	.size	rb_first_chunk, .Lfunc_end3-rb_first_chunk
                                        # -- End function
	.globl	fnv1a                           # -- Begin function fnv1a
	.p2align	4
	.type	fnv1a,@function
fnv1a:                                  # @fnv1a
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movabsq	$-3750763034362895579, %r8      # imm = 0xCBF29CE484222325
	testq	%rsi, %rsi
	je	.LBB4_1
# %bb.3:
	xorl	%ecx, %ecx
	movabsq	$1099511628211, %rdx            # imm = 0x100000001B3
	.p2align	4
.LBB4_4:                                # =>This Inner Loop Header: Depth=1
	movzbl	(%rdi,%rcx), %eax
	xorq	%r8, %rax
	imulq	%rdx, %rax
	incq	%rcx
	movq	%rax, %r8
	cmpq	%rcx, %rsi
	jne	.LBB4_4
# %bb.2:
	popq	%rbp
	retq
.LBB4_1:
	movq	%r8, %rax
	popq	%rbp
	retq
.Lfunc_end4:
	.size	fnv1a, .Lfunc_end4-fnv1a
                                        # -- End function
	.globl	demo_selftest                   # -- Begin function demo_selftest
	.p2align	4
	.type	demo_selftest,@function
demo_selftest:                          # @demo_selftest
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$432, %rsp                      # imm = 0x1B0
	leaq	-48(%rbp), %rax
	leaq	-32(%rbp), %rcx
	movq	%rcx, -48(%rbp)
	movq	$43690, -40(%rbp)               # imm = 0xAAAA
	leaq	-16(%rbp), %rcx
	movq	%rcx, -32(%rbp)
	movq	$48059, -24(%rbp)               # imm = 0xBBBB
	movq	$0, -16(%rbp)
	movq	$52428, -8(%rbp)                # imm = 0xCCCC
	leaq	(%rbp), %rcx
	xorl	%edx, %edx
	.p2align	4
.LBB5_1:                                # =>This Inner Loop Header: Depth=1
	cmpq	%rcx, %rax
	jae	.LBB5_5
# %bb.2:                                #   in Loop: Header=BB5_1 Depth=1
	movl	%eax, %esi
	andl	$7, %esi
	jne	.LBB5_5
# %bb.3:                                #   in Loop: Header=BB5_1 Depth=1
	cmpq	$63, %rdx
	ja	.LBB5_5
# %bb.4:                                #   in Loop: Header=BB5_1 Depth=1
	movq	(%rax), %rsi
	movq	8(%rax), %rdi
	movq	%rdi, -560(%rbp,%rdx,8)
	incq	%rdx
	cmpq	%rax, %rsi
	cmovaq	%rsi, %rax
	ja	.LBB5_1
.LBB5_5:
	movl	$1, %eax
	cmpl	$3, %edx
	jne	.LBB5_15
# %bb.6:
	movl	$2, %eax
	cmpq	$43690, -560(%rbp)              # imm = 0xAAAA
	jne	.LBB5_15
# %bb.7:
	movl	$3, %eax
	cmpq	$48059, -552(%rbp)              # imm = 0xBBBB
	jne	.LBB5_15
# %bb.8:
	movl	$4, %eax
	cmpq	$52428, -544(%rbp)              # imm = 0xCCCC
	jne	.LBB5_15
# %bb.9:
	leaq	-47(%rbp), %rdx
	xorl	%eax, %eax
	.p2align	4
.LBB5_10:                               # =>This Inner Loop Header: Depth=1
	cmpq	%rcx, %rdx
	jae	.LBB5_14
# %bb.11:                               #   in Loop: Header=BB5_10 Depth=1
	movl	%edx, %esi
	andl	$7, %esi
	jne	.LBB5_14
# %bb.12:                               #   in Loop: Header=BB5_10 Depth=1
	cmpq	$63, %rax
	ja	.LBB5_14
# %bb.13:                               #   in Loop: Header=BB5_10 Depth=1
	movq	(%rdx), %rsi
	movq	8(%rdx), %rdi
	movq	%rdi, -560(%rbp,%rax,8)
	incq	%rax
	cmpq	%rdx, %rsi
	cmovaq	%rsi, %rdx
	ja	.LBB5_10
.LBB5_14:
	xorl	%ecx, %ecx
	testl	%eax, %eax
	setne	%cl
	leal	(%rcx,%rcx,4), %eax
.LBB5_15:
	addq	$432, %rsp                      # imm = 0x1B0
	popq	%rbp
	retq
.Lfunc_end5:
	.size	demo_selftest, .Lfunc_end5-demo_selftest
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
