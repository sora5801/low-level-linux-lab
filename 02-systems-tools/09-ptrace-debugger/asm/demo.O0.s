	.file	"demo.c"
	.text
	.globl	patch_int3                      # -- Begin function patch_int3
	.p2align	4
	.type	patch_int3,@function
patch_int3:                             # @patch_int3
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rax
	andq	$-256, %rax
	orq	$204, %rax
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
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rax
	andq	$255, %rax
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
	movb	%sil, %al
	movq	%rdi, -8(%rbp)
	movb	%al, -9(%rbp)
	movq	-8(%rbp), %rax
	andq	$-256, %rax
	movzbl	-9(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	orq	%rcx, %rax
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
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rax
	subq	$1, %rax
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
	movq	%rdi, -16(%rbp)
	movl	%esi, -20(%rbp)
	movq	%rdx, -32(%rbp)
	movl	$0, -36(%rbp)
	movl	-20(%rbp), %eax
	movl	%eax, -40(%rbp)
.LBB4_1:                                # =>This Inner Loop Header: Depth=1
	movl	-36(%rbp), %eax
	cmpl	-40(%rbp), %eax
	jge	.LBB4_6
# %bb.2:                                #   in Loop: Header=BB4_1 Depth=1
	movl	-36(%rbp), %eax
	movl	%eax, -48(%rbp)                 # 4-byte Spill
	movl	-40(%rbp), %eax
	subl	-36(%rbp), %eax
	movl	$2, %ecx
	cltd
	idivl	%ecx
	movl	%eax, %ecx
	movl	-48(%rbp), %eax                 # 4-byte Reload
	addl	%ecx, %eax
	movl	%eax, -44(%rbp)
	movq	-16(%rbp), %rax
	movslq	-44(%rbp), %rcx
	imulq	$24, %rcx, %rcx
	addq	%rcx, %rax
	movq	(%rax), %rax
	cmpq	-32(%rbp), %rax
	ja	.LBB4_4
# %bb.3:                                #   in Loop: Header=BB4_1 Depth=1
	movl	-44(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -36(%rbp)
	jmp	.LBB4_5
.LBB4_4:                                #   in Loop: Header=BB4_1 Depth=1
	movl	-44(%rbp), %eax
	movl	%eax, -40(%rbp)
.LBB4_5:                                #   in Loop: Header=BB4_1 Depth=1
	jmp	.LBB4_1
.LBB4_6:
	cmpl	$0, -36(%rbp)
	jne	.LBB4_8
# %bb.7:
	movl	$-1, -4(%rbp)
	jmp	.LBB4_11
.LBB4_8:
	movq	-16(%rbp), %rax
	movl	-36(%rbp), %ecx
	subl	$1, %ecx
	movslq	%ecx, %rcx
	imulq	$24, %rcx, %rcx
	addq	%rcx, %rax
	cmpl	$0, 16(%rax)
	je	.LBB4_10
# %bb.9:
	movl	$-1, -4(%rbp)
	jmp	.LBB4_11
.LBB4_10:
	movl	-36(%rbp), %eax
	subl	$1, %eax
	movl	%eax, -4(%rbp)
.LBB4_11:
	movl	-4(%rbp), %eax
	popq	%rbp
	retq
.Lfunc_end4:
	.size	addr_to_line, .Lfunc_end4-addr_to_line
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
