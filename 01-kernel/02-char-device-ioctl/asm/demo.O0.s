	.file	"demo.c"
	.text
	.globl	rb_count                        # -- Begin function rb_count
	.p2align	4
	.type	rb_count,@function
rb_count:                               # @rb_count
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	%edi, -4(%rbp)
	movl	%esi, -8(%rbp)
	movl	-4(%rbp), %eax
	subl	-8(%rbp), %eax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	rb_count, .Lfunc_end0-rb_count
                                        # -- End function
	.globl	rb_space                        # -- Begin function rb_space
	.p2align	4
	.type	rb_space,@function
rb_space:                               # @rb_space
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	%edi, -4(%rbp)
	movl	%esi, -8(%rbp)
	movl	%edx, -12(%rbp)
	movl	-12(%rbp), %eax
	movl	-4(%rbp), %ecx
	subl	-8(%rbp), %ecx
	subl	%ecx, %eax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	rb_space, .Lfunc_end1-rb_space
                                        # -- End function
	.globl	rb_first_chunk                  # -- Begin function rb_first_chunk
	.p2align	4
	.type	rb_first_chunk,@function
rb_first_chunk:                         # @rb_first_chunk
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	%edi, -4(%rbp)
	movl	%esi, -8(%rbp)
	movl	%edx, -12(%rbp)
	movl	-4(%rbp), %eax
	movl	-8(%rbp), %ecx
	subl	$1, %ecx
	andl	%ecx, %eax
	movl	%eax, -16(%rbp)
	movl	-8(%rbp), %eax
	subl	-16(%rbp), %eax
	movl	%eax, -20(%rbp)
	movl	-12(%rbp), %eax
	cmpl	-20(%rbp), %eax
	jae	.LBB2_2
# %bb.1:
	movl	-12(%rbp), %eax
	movl	%eax, -24(%rbp)                 # 4-byte Spill
	jmp	.LBB2_3
.LBB2_2:
	movl	-20(%rbp), %eax
	movl	%eax, -24(%rbp)                 # 4-byte Spill
.LBB2_3:
	movl	-24(%rbp), %eax                 # 4-byte Reload
	popq	%rbp
	retq
.Lfunc_end2:
	.size	rb_first_chunk, .Lfunc_end2-rb_first_chunk
                                        # -- End function
	.globl	rb_plan_xfer                    # -- Begin function rb_plan_xfer
	.p2align	4
	.type	rb_plan_xfer,@function
rb_plan_xfer:                           # @rb_plan_xfer
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	%edi, -12(%rbp)
	movl	%esi, -16(%rbp)
	movl	%edx, -20(%rbp)
	movl	-12(%rbp), %eax
	movl	-16(%rbp), %ecx
	subl	$1, %ecx
	andl	%ecx, %eax
	movl	%eax, -24(%rbp)
	movl	-16(%rbp), %eax
	subl	-24(%rbp), %eax
	movl	%eax, -28(%rbp)
	movl	-20(%rbp), %eax
	cmpl	-28(%rbp), %eax
	jae	.LBB3_2
# %bb.1:
	movl	-20(%rbp), %eax
	movl	%eax, -36(%rbp)                 # 4-byte Spill
	jmp	.LBB3_3
.LBB3_2:
	movl	-28(%rbp), %eax
	movl	%eax, -36(%rbp)                 # 4-byte Spill
.LBB3_3:
	movl	-36(%rbp), %eax                 # 4-byte Reload
	movl	%eax, -32(%rbp)
	movl	-32(%rbp), %eax
	movl	%eax, -8(%rbp)
	movl	-20(%rbp), %eax
	subl	-32(%rbp), %eax
	movl	%eax, -4(%rbp)
	movq	-8(%rbp), %rax
	popq	%rbp
	retq
.Lfunc_end3:
	.size	rb_plan_xfer, .Lfunc_end3-rb_plan_xfer
                                        # -- End function
	.globl	rb_selftest                     # -- Begin function rb_selftest
	.p2align	4
	.type	rb_selftest,@function
rb_selftest:                            # @rb_selftest
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movl	$4096, -8(%rbp)                 # imm = 0x1000
	movl	$10, %esi
	movl	%esi, %edi
	callq	rb_count
	cmpl	$0, %eax
	je	.LBB4_2
# %bb.1:
	movl	$1, -4(%rbp)
	jmp	.LBB4_16
.LBB4_2:
	movl	$10, %edi
	movl	$4, %esi
	callq	rb_count
	cmpl	$6, %eax
	je	.LBB4_4
# %bb.3:
	movl	$2, -4(%rbp)
	jmp	.LBB4_16
.LBB4_4:
	movl	$10, %edi
	movl	$4, %esi
	movl	$4096, %edx                     # imm = 0x1000
	callq	rb_space
	cmpl	$4090, %eax                     # imm = 0xFFA
	je	.LBB4_6
# %bb.5:
	movl	$3, -4(%rbp)
	jmp	.LBB4_16
.LBB4_6:
	movl	$3996, %edi                     # imm = 0xF9C
	movl	$4096, %esi                     # imm = 0x1000
	movl	$40, %edx
	callq	rb_first_chunk
	cmpl	$40, %eax
	je	.LBB4_8
# %bb.7:
	movl	$4, -4(%rbp)
	jmp	.LBB4_16
.LBB4_8:
	movl	$3996, %edi                     # imm = 0xF9C
	movl	$4096, %esi                     # imm = 0x1000
	movl	$250, %edx
	callq	rb_first_chunk
	cmpl	$100, %eax
	je	.LBB4_10
# %bb.9:
	movl	$5, -4(%rbp)
	jmp	.LBB4_16
.LBB4_10:
	movl	$3996, %edi                     # imm = 0xF9C
	movl	$4096, %esi                     # imm = 0x1000
	movl	$250, %edx
	callq	rb_plan_xfer
	movq	%rax, -16(%rbp)
	cmpl	$100, -16(%rbp)
	jne	.LBB4_12
# %bb.11:
	cmpl	$150, -12(%rbp)
	je	.LBB4_13
.LBB4_12:
	movl	$6, -4(%rbp)
	jmp	.LBB4_16
.LBB4_13:
	movl	$3, %edi
	movl	$4294967295, %esi               # imm = 0xFFFFFFFF
	callq	rb_count
	cmpl	$4, %eax
	je	.LBB4_15
# %bb.14:
	movl	$7, -4(%rbp)
	jmp	.LBB4_16
.LBB4_15:
	movl	$0, -4(%rbp)
.LBB4_16:
	movl	-4(%rbp), %eax
	addq	$16, %rsp
	popq	%rbp
	retq
.Lfunc_end4:
	.size	rb_selftest, .Lfunc_end4-rb_selftest
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym rb_count
	.addrsig_sym rb_space
	.addrsig_sym rb_first_chunk
	.addrsig_sym rb_plan_xfer
