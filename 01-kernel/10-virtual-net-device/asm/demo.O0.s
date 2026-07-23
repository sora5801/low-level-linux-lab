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
	.globl	rb_is_full                      # -- Begin function rb_is_full
	.p2align	4
	.type	rb_is_full,@function
rb_is_full:                             # @rb_is_full
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	%edi, -4(%rbp)
	movl	%esi, -8(%rbp)
	movl	-4(%rbp), %eax
	subl	-8(%rbp), %eax
	cmpl	$64, %eax
	setae	%al
	andb	$1, %al
	movzbl	%al, %eax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	rb_is_full, .Lfunc_end1-rb_is_full
                                        # -- End function
	.globl	rb_is_empty                     # -- Begin function rb_is_empty
	.p2align	4
	.type	rb_is_empty,@function
rb_is_empty:                            # @rb_is_empty
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	%edi, -4(%rbp)
	movl	%esi, -8(%rbp)
	movl	-4(%rbp), %eax
	cmpl	-8(%rbp), %eax
	sete	%al
	andb	$1, %al
	movzbl	%al, %eax
	popq	%rbp
	retq
.Lfunc_end2:
	.size	rb_is_empty, .Lfunc_end2-rb_is_empty
                                        # -- End function
	.globl	rb_slot                         # -- Begin function rb_slot
	.p2align	4
	.type	rb_slot,@function
rb_slot:                                # @rb_slot
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	%edi, -4(%rbp)
	movl	-4(%rbp), %eax
	andl	$63, %eax
	popq	%rbp
	retq
.Lfunc_end3:
	.size	rb_slot, .Lfunc_end3-rb_slot
                                        # -- End function
	.globl	rb_reserve                      # -- Begin function rb_reserve
	.p2align	4
	.type	rb_reserve,@function
rb_reserve:                             # @rb_reserve
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -16(%rbp)
	movl	%esi, -20(%rbp)
	movq	-16(%rbp), %rax
	movl	(%rax), %eax
	movl	%eax, -24(%rbp)
	movl	-24(%rbp), %eax
	subl	-20(%rbp), %eax
	cmpl	$64, %eax
	jb	.LBB4_2
# %bb.1:
	movl	$-1, -4(%rbp)
	jmp	.LBB4_3
.LBB4_2:
	movl	-24(%rbp), %ecx
	addl	$1, %ecx
	movq	-16(%rbp), %rax
	movl	%ecx, (%rax)
	movl	-24(%rbp), %eax
	andl	$63, %eax
	movl	%eax, -4(%rbp)
.LBB4_3:
	movl	-4(%rbp), %eax
	popq	%rbp
	retq
.Lfunc_end4:
	.size	rb_reserve, .Lfunc_end4-rb_reserve
                                        # -- End function
	.globl	skb_has_headroom                # -- Begin function skb_has_headroom
	.p2align	4
	.type	skb_has_headroom,@function
skb_has_headroom:                       # @skb_has_headroom
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	%edi, -4(%rbp)
	movl	%esi, -8(%rbp)
	movl	%edx, -12(%rbp)
	movl	-8(%rbp), %eax
	subl	-4(%rbp), %eax
	cmpl	-12(%rbp), %eax
	setae	%al
	andb	$1, %al
	movzbl	%al, %eax
	popq	%rbp
	retq
.Lfunc_end5:
	.size	skb_has_headroom, .Lfunc_end5-skb_has_headroom
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
