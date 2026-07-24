	.file	"demo.c"
	.text
	.globl	patch_int3                      # -- Begin function patch_int3
	.p2align	4
	.type	patch_int3,@function
patch_int3:                             # @patch_int3
# %bb.0:
	andq	$-256, %rdi
	leaq	204(%rdi), %rax
	retq
.Lfunc_end0:
	.size	patch_int3, .Lfunc_end0-patch_int3
                                        # -- End function
	.globl	saved_byte_of                   # -- Begin function saved_byte_of
	.p2align	4
	.type	saved_byte_of,@function
saved_byte_of:                          # @saved_byte_of
# %bb.0:
	movq	%rdi, %rax
                                        # kill: def $al killed $al killed $rax
	retq
.Lfunc_end1:
	.size	saved_byte_of, .Lfunc_end1-saved_byte_of
                                        # -- End function
	.globl	unpatch_byte                    # -- Begin function unpatch_byte
	.p2align	4
	.type	unpatch_byte,@function
unpatch_byte:                           # @unpatch_byte
# %bb.0:
	andq	$-256, %rdi
	movl	%esi, %eax
	orq	%rdi, %rax
	retq
.Lfunc_end2:
	.size	unpatch_byte, .Lfunc_end2-unpatch_byte
                                        # -- End function
	.globl	rewind_rip                      # -- Begin function rewind_rip
	.p2align	4
	.type	rewind_rip,@function
rewind_rip:                             # @rewind_rip
# %bb.0:
	leaq	-1(%rdi), %rax
	retq
.Lfunc_end3:
	.size	rewind_rip, .Lfunc_end3-rewind_rip
                                        # -- End function
	.globl	addr_to_line                    # -- Begin function addr_to_line
	.p2align	4
	.type	addr_to_line,@function
addr_to_line:                           # @addr_to_line
# %bb.0:
	movl	$-1, %eax
	testl	%esi, %esi
	jle	.LBB4_8
# %bb.1:
	xorl	%ecx, %ecx
	jmp	.LBB4_2
	.p2align	4
.LBB4_4:                                #   in Loop: Header=BB4_2 Depth=1
	addl	%r8d, %ecx
	incl	%ecx
	cmpl	%esi, %ecx
	jge	.LBB4_6
.LBB4_2:                                # =>This Inner Loop Header: Depth=1
	movl	%esi, %r9d
	subl	%ecx, %r9d
	movl	%r9d, %r8d
	shrl	$31, %r8d
	addl	%r9d, %r8d
	sarl	%r8d
	leal	(%r8,%rcx), %r9d
	movslq	%r9d, %r10
	leaq	(%r10,%r10,2), %r10
	cmpq	%rdx, (%rdi,%r10,8)
	jbe	.LBB4_4
# %bb.3:                                #   in Loop: Header=BB4_2 Depth=1
	movl	%r9d, %esi
	cmpl	%esi, %ecx
	jl	.LBB4_2
.LBB4_6:
	testl	%ecx, %ecx
	je	.LBB4_8
# %bb.7:
	decl	%ecx
	movslq	%ecx, %rax
	leaq	(%rax,%rax,2), %rcx
	xorl	%edx, %edx
	cmpl	16(%rdi,%rcx,8), %edx
	sbbl	%edx, %edx
	orl	%edx, %eax
.LBB4_8:
                                        # kill: def $eax killed $eax killed $rax
	retq
.Lfunc_end4:
	.size	addr_to_line, .Lfunc_end4-addr_to_line
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
