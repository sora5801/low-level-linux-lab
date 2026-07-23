	.file	"demo.c"
	.text
	.globl	rb_count                        # -- Begin function rb_count
	.p2align	4
	.type	rb_count,@function
rb_count:                               # @rb_count
# %bb.0:
	movl	%edi, %eax
	subl	%esi, %eax
	retq
.Lfunc_end0:
	.size	rb_count, .Lfunc_end0-rb_count
                                        # -- End function
	.globl	rb_space                        # -- Begin function rb_space
	.p2align	4
	.type	rb_space,@function
rb_space:                               # @rb_space
# %bb.0:
                                        # kill: def $edx killed $edx def $rdx
                                        # kill: def $esi killed $esi def $rsi
	subl	%edi, %esi
	leal	(%rsi,%rdx), %eax
	retq
.Lfunc_end1:
	.size	rb_space, .Lfunc_end1-rb_space
                                        # -- End function
	.globl	rb_first_chunk                  # -- Begin function rb_first_chunk
	.p2align	4
	.type	rb_first_chunk,@function
rb_first_chunk:                         # @rb_first_chunk
# %bb.0:
	movl	%esi, %eax
	leal	-1(%rax), %ecx
	andl	%edi, %ecx
	subl	%ecx, %eax
	cmpl	%eax, %edx
	cmovbl	%edx, %eax
                                        # kill: def $eax killed $eax killed $rax
	retq
.Lfunc_end2:
	.size	rb_first_chunk, .Lfunc_end2-rb_first_chunk
                                        # -- End function
	.globl	rb_plan_xfer                    # -- Begin function rb_plan_xfer
	.p2align	4
	.type	rb_plan_xfer,@function
rb_plan_xfer:                           # @rb_plan_xfer
# %bb.0:
                                        # kill: def $edx killed $edx def $rdx
                                        # kill: def $esi killed $esi def $rsi
	leal	-1(%rsi), %eax
	andl	%edi, %eax
	subl	%eax, %esi
	cmpl	%esi, %edx
	cmovbl	%edx, %esi
	subl	%esi, %edx
	shlq	$32, %rdx
	leaq	(%rsi,%rdx), %rax
	retq
.Lfunc_end3:
	.size	rb_plan_xfer, .Lfunc_end3-rb_plan_xfer
                                        # -- End function
	.globl	rb_selftest                     # -- Begin function rb_selftest
	.p2align	4
	.type	rb_selftest,@function
rb_selftest:                            # @rb_selftest
# %bb.0:
	xorl	%eax, %eax
	retq
.Lfunc_end4:
	.size	rb_selftest, .Lfunc_end4-rb_selftest
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
