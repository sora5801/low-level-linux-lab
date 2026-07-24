	.file	"demo.c"
	.text
	.globl	vq_ring_slot                    # -- Begin function vq_ring_slot
	.p2align	4
	.type	vq_ring_slot,@function
vq_ring_slot:                           # @vq_ring_slot
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movw	%si, %ax
	movw	%di, %cx
	movw	%cx, -2(%rbp)
	movw	%ax, -4(%rbp)
	movzwl	-2(%rbp), %eax
	movzwl	-4(%rbp), %ecx
	subl	$1, %ecx
                                        # kill: def $cx killed $cx killed $ecx
	movzwl	%cx, %ecx
	andl	%ecx, %eax
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
	movw	%si, %ax
	movw	%di, %cx
	movw	%cx, -2(%rbp)
	movw	%ax, -4(%rbp)
	movzwl	-2(%rbp), %eax
	movzwl	-4(%rbp), %ecx
	subl	%ecx, %eax
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
	movw	%di, %ax
	movw	%ax, -2(%rbp)
	movzwl	-2(%rbp), %eax
	addl	$1, %eax
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
	movw	%di, %ax
	movw	%ax, -2(%rbp)
	movzwl	-2(%rbp), %eax
	andl	$2, %eax
	cmpl	$0, %eax
	setne	%al
	andb	$1, %al
	movzbl	%al, %eax
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
	movw	%di, %ax
	movw	%ax, -2(%rbp)
	movzwl	-2(%rbp), %eax
	andl	$1, %eax
	cmpl	$0, %eax
	setne	%al
	andb	$1, %al
	movzbl	%al, %eax
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
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	-8(%rbp), %rax
	subq	-16(%rbp), %rax
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
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	-8(%rbp), %rax
	subq	-16(%rbp), %rax
	andq	$511, %rax                      # imm = 0x1FF
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
	movl	%edi, -8(%rbp)
	movl	-8(%rbp), %eax
	movl	%eax, -12(%rbp)                 # 4-byte Spill
	subl	$2, %eax
	je	.LBB7_2
	jmp	.LBB7_8
.LBB7_8:
	movl	-12(%rbp), %eax                 # 4-byte Reload
	subl	$5, %eax
	je	.LBB7_3
	jmp	.LBB7_9
.LBB7_9:
	movl	-12(%rbp), %eax                 # 4-byte Reload
	subl	$6, %eax
	je	.LBB7_1
	jmp	.LBB7_10
.LBB7_10:
	movl	-12(%rbp), %eax                 # 4-byte Reload
	subl	$7, %eax
	je	.LBB7_5
	jmp	.LBB7_11
.LBB7_11:
	movl	-12(%rbp), %eax                 # 4-byte Reload
	addl	$-8, %eax
	subl	$2, %eax
	jb	.LBB7_4
	jmp	.LBB7_12
.LBB7_12:
	movl	-12(%rbp), %eax                 # 4-byte Reload
	subl	$10, %eax
	je	.LBB7_5
	jmp	.LBB7_13
.LBB7_13:
	movl	-12(%rbp), %eax                 # 4-byte Reload
	subl	$17, %eax
	je	.LBB7_4
	jmp	.LBB7_6
.LBB7_1:
	movl	$1, -4(%rbp)
	jmp	.LBB7_7
.LBB7_2:
	movl	$2, -4(%rbp)
	jmp	.LBB7_7
.LBB7_3:
	movl	$3, -4(%rbp)
	jmp	.LBB7_7
.LBB7_4:
	movl	$4, -4(%rbp)
	jmp	.LBB7_7
.LBB7_5:
	movl	$5, -4(%rbp)
	jmp	.LBB7_7
.LBB7_6:
	movl	$0, -4(%rbp)
.LBB7_7:
	movl	-4(%rbp), %eax
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
	subq	$16, %rsp
	movl	$11, %edi
	movl	$8, %esi
	callq	vq_ring_slot
	movzwl	%ax, %eax
	cmpl	$3, %eax
	je	.LBB8_2
# %bb.1:
	movl	$1, -4(%rbp)
	jmp	.LBB8_29
.LBB8_2:
	movl	$8, %esi
	movl	%esi, %edi
	callq	vq_ring_slot
	movzwl	%ax, %eax
	cmpl	$0, %eax
	je	.LBB8_4
# %bb.3:
	movl	$2, -4(%rbp)
	jmp	.LBB8_29
.LBB8_4:
	movl	$3, %edi
	movl	$65534, %esi                    # imm = 0xFFFE
	callq	vq_pending
	movzwl	%ax, %eax
	cmpl	$5, %eax
	je	.LBB8_6
# %bb.5:
	movl	$3, -4(%rbp)
	jmp	.LBB8_29
.LBB8_6:
	movl	$1, %edi
	xorl	%esi, %esi
	callq	vq_pending
	movzwl	%ax, %eax
	cmpl	$1, %eax
	je	.LBB8_8
# %bb.7:
	movl	$4, -4(%rbp)
	jmp	.LBB8_29
.LBB8_8:
	movl	$65535, %edi                    # imm = 0xFFFF
	callq	vq_next
	movzwl	%ax, %eax
	cmpl	$0, %eax
	je	.LBB8_10
# %bb.9:
	movl	$5, -4(%rbp)
	jmp	.LBB8_29
.LBB8_10:
	movl	$1, %edi
	callq	desc_has_next
	cmpl	$0, %eax
	jne	.LBB8_12
# %bb.11:
	movl	$6, -4(%rbp)
	jmp	.LBB8_29
.LBB8_12:
	movl	$1, %edi
	callq	desc_is_writable
	cmpl	$0, %eax
	je	.LBB8_14
# %bb.13:
	movl	$7, -4(%rbp)
	jmp	.LBB8_29
.LBB8_14:
	movl	$2, %edi
	callq	desc_is_writable
	cmpl	$0, %eax
	jne	.LBB8_16
# %bb.15:
	movl	$8, -4(%rbp)
	jmp	.LBB8_29
.LBB8_16:
	movl	$268436560, %edi                # imm = 0x10000450
	movl	$268435456, %esi                # imm = 0x10000000
	callq	mmio_device_index
	cmpl	$2, %eax
	je	.LBB8_18
# %bb.17:
	movl	$9, -4(%rbp)
	jmp	.LBB8_29
.LBB8_18:
	movl	$268436560, %edi                # imm = 0x10000450
	movl	$268435456, %esi                # imm = 0x10000000
	callq	mmio_reg_offset
	cmpl	$80, %eax
	je	.LBB8_20
# %bb.19:
	movl	$10, -4(%rbp)
	jmp	.LBB8_29
.LBB8_20:
	movl	$6, %edi
	callq	kvm_exit_action
	cmpl	$1, %eax
	je	.LBB8_22
# %bb.21:
	movl	$11, -4(%rbp)
	jmp	.LBB8_29
.LBB8_22:
	movl	$5, %edi
	callq	kvm_exit_action
	cmpl	$3, %eax
	je	.LBB8_24
# %bb.23:
	movl	$12, -4(%rbp)
	jmp	.LBB8_29
.LBB8_24:
	movl	$8, %edi
	callq	kvm_exit_action
	cmpl	$4, %eax
	je	.LBB8_26
# %bb.25:
	movl	$13, -4(%rbp)
	jmp	.LBB8_29
.LBB8_26:
	movl	$10, %edi
	callq	kvm_exit_action
	cmpl	$5, %eax
	je	.LBB8_28
# %bb.27:
	movl	$14, -4(%rbp)
	jmp	.LBB8_29
.LBB8_28:
	movl	$0, -4(%rbp)
.LBB8_29:
	movl	-4(%rbp), %eax
	addq	$16, %rsp
	popq	%rbp
	retq
.Lfunc_end8:
	.size	demo_selftest, .Lfunc_end8-demo_selftest
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym vq_ring_slot
	.addrsig_sym vq_pending
	.addrsig_sym vq_next
	.addrsig_sym desc_is_writable
	.addrsig_sym desc_has_next
	.addrsig_sym mmio_device_index
	.addrsig_sym mmio_reg_offset
	.addrsig_sym kvm_exit_action
