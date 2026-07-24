	.file	"demo.c"
	.text
	.globl	patch_int3                      # -- Begin function patch_int3
	.p2align	4
	.type	patch_int3,@function
patch_int3:                             # @patch_int3
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	andq	$-256, %rdi
	leaq	204(%rdi), %rax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	patch_int3, .Lfunc_end0-patch_int3
                                        # -- End function
	.globl	saved_byte_of                   # -- Begin function saved_byte_of
	.p2align	4
	.type	saved_byte_of,@function
saved_byte_of:                          # @saved_byte_of
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, %rax
                                        # kill: def $al killed $al killed $rax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	saved_byte_of, .Lfunc_end1-saved_byte_of
                                        # -- End function
	.globl	unpatch_byte                    # -- Begin function unpatch_byte
	.p2align	4
	.type	unpatch_byte,@function
unpatch_byte:                           # @unpatch_byte
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	andq	$-256, %rdi
	movl	%esi, %eax
	orq	%rdi, %rax
	popq	%rbp
	retq
.Lfunc_end2:
	.size	unpatch_byte, .Lfunc_end2-unpatch_byte
                                        # -- End function
	.globl	rewind_rip                      # -- Begin function rewind_rip
	.p2align	4
	.type	rewind_rip,@function
rewind_rip:                             # @rewind_rip
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	leaq	-1(%rdi), %rax
	popq	%rbp
	retq
.Lfunc_end3:
	.size	rewind_rip, .Lfunc_end3-rewind_rip
                                        # -- End function
	.globl	addr_to_line                    # -- Begin function addr_to_line
	.p2align	4
	.type	addr_to_line,@function
addr_to_line:                           # @addr_to_line
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	xorl	%eax, %eax
	testl	%esi, %esi
	jg	.LBB4_2
	jmp	.LBB4_4
	.p2align	4
.LBB4_1:                                #   in Loop: Header=BB4_2 Depth=1
	addl	%ecx, %eax
	incl	%eax
	cmpl	%esi, %eax
	jge	.LBB4_4
.LBB4_2:                                # =>This Inner Loop Header: Depth=1
	movl	%esi, %r8d
	subl	%eax, %r8d
	movl	%r8d, %ecx
	shrl	$31, %ecx
	addl	%r8d, %ecx
	sarl	%ecx
	leal	(%rcx,%rax), %r8d
	movslq	%r8d, %r9
	leaq	(%r9,%r9,2), %r9
	cmpq	%rdx, (%rdi,%r9,8)
	jbe	.LBB4_1
# %bb.3:                                #   in Loop: Header=BB4_2 Depth=1
	movl	%r8d, %esi
	cmpl	%esi, %eax
	jl	.LBB4_2
.LBB4_4:
	testl	%eax, %eax
	je	.LBB4_6
# %bb.5:
	decl	%eax
	cltq
	leaq	(%rax,%rax,2), %rcx
	xorl	%edx, %edx
	cmpl	16(%rdi,%rcx,8), %edx
	sbbl	%edx, %edx
	orl	%edx, %eax
                                        # kill: def $eax killed $eax killed $rax
	popq	%rbp
	retq
.LBB4_6:
	movl	$-1, %eax
                                        # kill: def $eax killed $eax killed $rax
	popq	%rbp
	retq
.Lfunc_end4:
	.size	addr_to_line, .Lfunc_end4-addr_to_line
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
