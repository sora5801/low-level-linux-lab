	.file	"demo.c"
	.text
	.globl	kvm_exit_action                 # -- Begin function kvm_exit_action
	.p2align	4
	.type	kvm_exit_action,@function
kvm_exit_action:                        # @kvm_exit_action
# %bb.0:
	cmpl	$17, %edi
	ja	.LBB0_1
# %bb.3:
	movl	$131840, %eax                   # imm = 0x20300
	btl	%edi, %eax
	jb	.LBB0_8
# %bb.4:
	movl	$1152, %eax                     # imm = 0x480
	btl	%edi, %eax
	jae	.LBB0_5
# %bb.9:
	movl	$5, %eax
	retq
.LBB0_8:
	movl	$4, %eax
	retq
.LBB0_5:
	cmpl	$6, %edi
	jne	.LBB0_1
# %bb.6:
	movl	$2, %eax
	retq
.LBB0_1:
	cmpl	$2, %edi
	je	.LBB0_2
# %bb.10:
	cmpl	$5, %edi
	jne	.LBB0_11
# %bb.7:
	movl	$3, %eax
	retq
.LBB0_2:
	movl	$1, %eax
	retq
.LBB0_11:
	xorl	%eax, %eax
	retq
.Lfunc_end0:
	.size	kvm_exit_action, .Lfunc_end0-kvm_exit_action
                                        # -- End function
	.globl	io_batch_bytes                  # -- Begin function io_batch_bytes
	.p2align	4
	.type	io_batch_bytes,@function
io_batch_bytes:                         # @io_batch_bytes
# %bb.0:
	movl	%edi, %eax
	imull	%esi, %eax
	retq
.Lfunc_end1:
	.size	io_batch_bytes, .Lfunc_end1-io_batch_bytes
                                        # -- End function
	.globl	serial_is_console_write         # -- Begin function serial_is_console_write
	.p2align	4
	.type	serial_is_console_write,@function
serial_is_console_write:                # @serial_is_console_write
# %bb.0:
	xorl	$1, %edi
	xorl	%edx, %esi
	xorl	%eax, %eax
	orl	%edi, %esi
	sete	%al
	retq
.Lfunc_end2:
	.size	serial_is_console_write, .Lfunc_end2-serial_is_console_write
                                        # -- End function
	.globl	decode_exit                     # -- Begin function decode_exit
	.p2align	4
	.type	decode_exit,@function
decode_exit:                            # @decode_exit
# %bb.0:
	cmpl	$17, %edi
	ja	.LBB3_1
# %bb.3:
	movl	$131840, %eax                   # imm = 0x20300
	btl	%edi, %eax
	jb	.LBB3_4
# %bb.5:
	movl	$1152, %eax                     # imm = 0x480
	btl	%edi, %eax
	jae	.LBB3_6
# %bb.9:
	movl	$5, %eax
	jmp	.LBB3_12
.LBB3_4:
	movl	$4, %eax
	movabsq	$4294967296, %rcx               # imm = 0x100000000
	orq	%rcx, %rax
	retq
.LBB3_6:
	cmpl	$6, %edi
	jne	.LBB3_1
# %bb.7:
	movl	$2, %eax
	jmp	.LBB3_12
.LBB3_1:
	cmpl	$2, %edi
	je	.LBB3_2
# %bb.10:
	cmpl	$5, %edi
	jne	.LBB3_11
# %bb.8:
	movl	$3, %eax
	jmp	.LBB3_12
.LBB3_2:
	movl	$1, %eax
	jmp	.LBB3_12
.LBB3_11:
	xorl	%eax, %eax
.LBB3_12:
	xorl	%ecx, %ecx
	orq	%rcx, %rax
	retq
.Lfunc_end3:
	.size	decode_exit, .Lfunc_end3-decode_exit
                                        # -- End function
	.globl	demo_selftest                   # -- Begin function demo_selftest
	.p2align	4
	.type	demo_selftest,@function
demo_selftest:                          # @demo_selftest
# %bb.0:
	xorl	%eax, %eax
	retq
.Lfunc_end4:
	.size	demo_selftest, .Lfunc_end4-demo_selftest
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
