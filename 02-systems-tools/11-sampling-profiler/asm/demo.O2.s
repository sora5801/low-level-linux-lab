	.file	"demo.c"
	.text
	.globl	fp_unwind                       # -- Begin function fp_unwind
	.p2align	4
	.type	fp_unwind,@function
fp_unwind:                              # @fp_unwind
# %bb.0:
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
	movq	%r9, %rdi
	ja	.LBB0_1
.LBB0_6:
                                        # kill: def $eax killed $eax killed $rax
	retq
.Lfunc_end0:
	.size	fp_unwind, .Lfunc_end0-fp_unwind
                                        # -- End function
	.globl	rb_avail                        # -- Begin function rb_avail
	.p2align	4
	.type	rb_avail,@function
rb_avail:                               # @rb_avail
# %bb.0:
	movq	%rdi, %rax
	subq	%rsi, %rax
	retq
.Lfunc_end1:
	.size	rb_avail, .Lfunc_end1-rb_avail
                                        # -- End function
	.globl	rb_offset                       # -- Begin function rb_offset
	.p2align	4
	.type	rb_offset,@function
rb_offset:                              # @rb_offset
# %bb.0:
	leaq	-1(%rsi), %rax
	andq	%rdi, %rax
	retq
.Lfunc_end2:
	.size	rb_offset, .Lfunc_end2-rb_offset
                                        # -- End function
	.globl	rb_first_chunk                  # -- Begin function rb_first_chunk
	.p2align	4
	.type	rb_first_chunk,@function
rb_first_chunk:                         # @rb_first_chunk
# %bb.0:
	movq	%rdx, %rax
	subq	%rdi, %rax
	cmpq	%rax, %rsi
	cmovbq	%rsi, %rax
	retq
.Lfunc_end3:
	.size	rb_first_chunk, .Lfunc_end3-rb_first_chunk
                                        # -- End function
	.globl	fnv1a                           # -- Begin function fnv1a
	.p2align	4
	.type	fnv1a,@function
fnv1a:                                  # @fnv1a
# %bb.0:
	movabsq	$-3750763034362895579, %r8      # imm = 0xCBF29CE484222325
	testq	%rsi, %rsi
	je	.LBB4_1
# %bb.2:
	movabsq	$1099511628211, %rcx            # imm = 0x100000001B3
	movl	%esi, %edx
	andl	$3, %edx
	cmpq	$4, %rsi
	jae	.LBB4_8
# %bb.3:
	xorl	%r9d, %r9d
	jmp	.LBB4_4
.LBB4_1:
	movq	%r8, %rax
	retq
.LBB4_8:
	andq	$-4, %rsi
	xorl	%r9d, %r9d
	.p2align	4
.LBB4_9:                                # =>This Inner Loop Header: Depth=1
	movzbl	(%rdi,%r9), %eax
	xorq	%r8, %rax
	imulq	%rcx, %rax
	movzbl	1(%rdi,%r9), %r8d
	xorq	%rax, %r8
	imulq	%rcx, %r8
	movzbl	2(%rdi,%r9), %eax
	xorq	%r8, %rax
	imulq	%rcx, %rax
	movzbl	3(%rdi,%r9), %r8d
	xorq	%rax, %r8
	imulq	%rcx, %r8
	addq	$4, %r9
	cmpq	%r9, %rsi
	jne	.LBB4_9
.LBB4_4:
	movq	%r8, %rax
	testq	%rdx, %rdx
	je	.LBB4_7
# %bb.5:
	addq	%r9, %rdi
	xorl	%esi, %esi
	.p2align	4
.LBB4_6:                                # =>This Inner Loop Header: Depth=1
	movzbl	(%rdi,%rsi), %eax
	xorq	%r8, %rax
	imulq	%rcx, %rax
	incq	%rsi
	movq	%rax, %r8
	cmpq	%rsi, %rdx
	jne	.LBB4_6
.LBB4_7:
	retq
.Lfunc_end4:
	.size	fnv1a, .Lfunc_end4-fnv1a
                                        # -- End function
	.globl	demo_selftest                   # -- Begin function demo_selftest
	.p2align	4
	.type	demo_selftest,@function
demo_selftest:                          # @demo_selftest
# %bb.0:
	subq	$440, %rsp                      # imm = 0x1B8
	leaq	-128(%rsp), %rax
	leaq	-112(%rsp), %rcx
	movq	%rcx, -128(%rsp)
	movq	$43690, -120(%rsp)              # imm = 0xAAAA
	leaq	-96(%rsp), %rcx
	movq	%rcx, -112(%rsp)
	movq	$48059, -104(%rsp)              # imm = 0xBBBB
	movq	$0, -96(%rsp)
	movq	$52428, -88(%rsp)               # imm = 0xCCCC
	leaq	-80(%rsp), %rcx
	xorl	%edx, %edx
	movq	%rax, %rsi
	.p2align	4
.LBB5_1:                                # =>This Inner Loop Header: Depth=1
	cmpq	%rax, %rsi
	jb	.LBB5_6
# %bb.2:                                #   in Loop: Header=BB5_1 Depth=1
	cmpq	%rcx, %rsi
	jae	.LBB5_6
# %bb.3:                                #   in Loop: Header=BB5_1 Depth=1
	movl	%esi, %edi
	andl	$7, %edi
	jne	.LBB5_6
# %bb.4:                                #   in Loop: Header=BB5_1 Depth=1
	cmpq	$63, %rdx
	ja	.LBB5_6
# %bb.5:                                #   in Loop: Header=BB5_1 Depth=1
	movq	(%rsi), %rdi
	movq	8(%rsi), %r8
	movq	%r8, -80(%rsp,%rdx,8)
	incq	%rdx
	cmpq	%rsi, %rdi
	movq	%rdi, %rsi
	ja	.LBB5_1
.LBB5_6:
	movl	$1, %eax
	cmpl	$3, %edx
	jne	.LBB5_17
# %bb.7:
	movl	$2, %eax
	cmpq	$43690, -80(%rsp)               # imm = 0xAAAA
	jne	.LBB5_17
# %bb.8:
	movl	$3, %eax
	cmpq	$48059, -72(%rsp)               # imm = 0xBBBB
	jne	.LBB5_17
# %bb.9:
	movl	$4, %eax
	cmpq	$52428, -64(%rsp)               # imm = 0xCCCC
	jne	.LBB5_17
# %bb.10:
	leaq	-127(%rsp), %rdx
	xorl	%eax, %eax
	leaq	-128(%rsp), %rsi
	.p2align	4
.LBB5_11:                               # =>This Inner Loop Header: Depth=1
	cmpq	%rsi, %rdx
	jb	.LBB5_16
# %bb.12:                               #   in Loop: Header=BB5_11 Depth=1
	cmpq	%rcx, %rdx
	jae	.LBB5_16
# %bb.13:                               #   in Loop: Header=BB5_11 Depth=1
	movl	%edx, %edi
	andl	$7, %edi
	jne	.LBB5_16
# %bb.14:                               #   in Loop: Header=BB5_11 Depth=1
	cmpq	$63, %rax
	ja	.LBB5_16
# %bb.15:                               #   in Loop: Header=BB5_11 Depth=1
	movq	(%rdx), %rdi
	movq	8(%rdx), %r8
	movq	%r8, -80(%rsp,%rax,8)
	incq	%rax
	cmpq	%rdx, %rdi
	movq	%rdi, %rdx
	ja	.LBB5_11
.LBB5_16:
	xorl	%ecx, %ecx
	testl	%eax, %eax
	setne	%cl
	leal	(%rcx,%rcx,4), %eax
.LBB5_17:
	addq	$440, %rsp                      # imm = 0x1B8
	retq
.Lfunc_end5:
	.size	demo_selftest, .Lfunc_end5-demo_selftest
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
