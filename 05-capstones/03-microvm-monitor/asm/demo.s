	.file	"demo.c"
	.text
	.globl	vq_ring_slot                    # -- Begin function vq_ring_slot
	.p2align	4
	.type	vq_ring_slot,@function
vq_ring_slot:                           # @vq_ring_slot
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
                                        # kill: def $esi killed $esi def $rsi
	leal	-1(%rsi), %eax
	andl	%edi, %eax
                                        # kill: def $ax killed $ax killed $eax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	vq_ring_slot, .Lfunc_end0-vq_ring_slot
                                        # -- End function
	.globl	vq_pending                      # -- Begin function vq_pending
	.p2align	4
	.type	vq_pending,@function
vq_pending:                             # @vq_pending
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	%edi, %eax
	subl	%esi, %eax
                                        # kill: def $ax killed $ax killed $eax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	vq_pending, .Lfunc_end1-vq_pending
                                        # -- End function
	.globl	vq_next                         # -- Begin function vq_next
	.p2align	4
	.type	vq_next,@function
vq_next:                                # @vq_next
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
                                        # kill: def $edi killed $edi def $rdi
	leal	1(%rdi), %eax
                                        # kill: def $ax killed $ax killed $eax
	popq	%rbp
	retq
.Lfunc_end2:
	.size	vq_next, .Lfunc_end2-vq_next
                                        # -- End function
	.globl	desc_is_writable                # -- Begin function desc_is_writable
	.p2align	4
	.type	desc_is_writable,@function
desc_is_writable:                       # @desc_is_writable
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	%edi, %eax
	shrl	%eax
	andl	$1, %eax
	popq	%rbp
	retq
.Lfunc_end3:
	.size	desc_is_writable, .Lfunc_end3-desc_is_writable
                                        # -- End function
	.globl	desc_has_next                   # -- Begin function desc_has_next
	.p2align	4
	.type	desc_has_next,@function
desc_has_next:                          # @desc_has_next
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	%edi, %eax
	andl	$1, %eax
	popq	%rbp
	retq
.Lfunc_end4:
	.size	desc_has_next, .Lfunc_end4-desc_has_next
                                        # -- End function
	.globl	mmio_device_index               # -- Begin function mmio_device_index
	.p2align	4
	.type	mmio_device_index,@function
mmio_device_index:                      # @mmio_device_index
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, %rax
	subq	%rsi, %rax
	shrq	$9, %rax
                                        # kill: def $eax killed $eax killed $rax
	popq	%rbp
	retq
.Lfunc_end5:
	.size	mmio_device_index, .Lfunc_end5-mmio_device_index
                                        # -- End function
	.globl	mmio_reg_offset                 # -- Begin function mmio_reg_offset
	.p2align	4
	.type	mmio_reg_offset,@function
mmio_reg_offset:                        # @mmio_reg_offset
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, %rax
	subl	%esi, %eax
	andl	$511, %eax                      # imm = 0x1FF
                                        # kill: def $eax killed $eax killed $rax
	popq	%rbp
	retq
.Lfunc_end6:
	.size	mmio_reg_offset, .Lfunc_end6-mmio_reg_offset
                                        # -- End function
	.globl	kvm_exit_action                 # -- Begin function kvm_exit_action
	.p2align	4
	.type	kvm_exit_action,@function
kvm_exit_action:                        # @kvm_exit_action
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	cmpl	$17, %edi
	ja	.LBB7_1
# %bb.4:
	movl	$131840, %eax                   # imm = 0x20300
	btl	%edi, %eax
	jb	.LBB7_8
# %bb.5:
	movl	$1152, %eax                     # imm = 0x480
	btl	%edi, %eax
	jae	.LBB7_6
# %bb.9:
	movl	$5, %eax
	popq	%rbp
	retq
.LBB7_8:
	movl	$4, %eax
	popq	%rbp
	retq
.LBB7_6:
	movl	$1, %eax
	cmpl	$6, %edi
	jne	.LBB7_1
# %bb.11:
	popq	%rbp
	retq
.LBB7_1:
	cmpl	$5, %edi
	je	.LBB7_7
# %bb.2:
	cmpl	$2, %edi
	jne	.LBB7_10
# %bb.3:
	movl	$2, %eax
	popq	%rbp
	retq
.LBB7_7:
	movl	$3, %eax
	popq	%rbp
	retq
.LBB7_10:
	xorl	%eax, %eax
	popq	%rbp
	retq
.Lfunc_end7:
	.size	kvm_exit_action, .Lfunc_end7-kvm_exit_action
                                        # -- End function
	.globl	demo_selftest                   # -- Begin function demo_selftest
	.p2align	4
	.type	demo_selftest,@function
demo_selftest:                          # @demo_selftest
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	xorl	%eax, %eax
	popq	%rbp
	retq
.Lfunc_end8:
	.size	demo_selftest, .Lfunc_end8-demo_selftest
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
