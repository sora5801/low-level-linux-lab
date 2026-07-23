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
	.globl	rb_is_full                      # -- Begin function rb_is_full
	.p2align	4
	.type	rb_is_full,@function
rb_is_full:                             # @rb_is_full
# %bb.0:
	subl	%esi, %edi
	xorl	%eax, %eax
	cmpl	$64, %edi
	setae	%al
	retq
.Lfunc_end1:
	.size	rb_is_full, .Lfunc_end1-rb_is_full
                                        # -- End function
	.globl	rb_is_empty                     # -- Begin function rb_is_empty
	.p2align	4
	.type	rb_is_empty,@function
rb_is_empty:                            # @rb_is_empty
# %bb.0:
	xorl	%eax, %eax
	cmpl	%esi, %edi
	sete	%al
	retq
.Lfunc_end2:
	.size	rb_is_empty, .Lfunc_end2-rb_is_empty
                                        # -- End function
	.globl	rb_slot                         # -- Begin function rb_slot
	.p2align	4
	.type	rb_slot,@function
rb_slot:                                # @rb_slot
# %bb.0:
	movl	%edi, %eax
	andl	$63, %eax
	retq
.Lfunc_end3:
	.size	rb_slot, .Lfunc_end3-rb_slot
                                        # -- End function
	.globl	rb_reserve                      # -- Begin function rb_reserve
	.p2align	4
	.type	rb_reserve,@function
rb_reserve:                             # @rb_reserve
# %bb.0:
	movl	(%rdi), %ecx
	movl	%ecx, %edx
	subl	%esi, %edx
	movl	$-1, %eax
	cmpl	$63, %edx
	ja	.LBB4_2
# %bb.1:
	leal	1(%rcx), %eax
	movl	%eax, (%rdi)
	andl	$63, %ecx
	movl	%ecx, %eax
.LBB4_2:
	retq
.Lfunc_end4:
	.size	rb_reserve, .Lfunc_end4-rb_reserve
                                        # -- End function
	.globl	skb_has_headroom                # -- Begin function skb_has_headroom
	.p2align	4
	.type	skb_has_headroom,@function
skb_has_headroom:                       # @skb_has_headroom
# %bb.0:
	subl	%edi, %esi
	xorl	%eax, %eax
	cmpl	%edx, %esi
	setae	%al
	retq
.Lfunc_end5:
	.size	skb_has_headroom, .Lfunc_end5-skb_has_headroom
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
