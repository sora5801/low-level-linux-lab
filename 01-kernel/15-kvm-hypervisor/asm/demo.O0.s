	.file	"demo.c"
	.text
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
	je	.LBB0_1
	jmp	.LBB0_8
.LBB0_8:
	movl	-12(%rbp), %eax                 # 4-byte Reload
	subl	$5, %eax
	je	.LBB0_3
	jmp	.LBB0_9
.LBB0_9:
	movl	-12(%rbp), %eax                 # 4-byte Reload
	subl	$6, %eax
	je	.LBB0_2
	jmp	.LBB0_10
.LBB0_10:
	movl	-12(%rbp), %eax                 # 4-byte Reload
	subl	$7, %eax
	je	.LBB0_5
	jmp	.LBB0_11
.LBB0_11:
	movl	-12(%rbp), %eax                 # 4-byte Reload
	addl	$-8, %eax
	subl	$2, %eax
	jb	.LBB0_4
	jmp	.LBB0_12
.LBB0_12:
	movl	-12(%rbp), %eax                 # 4-byte Reload
	subl	$10, %eax
	je	.LBB0_5
	jmp	.LBB0_13
.LBB0_13:
	movl	-12(%rbp), %eax                 # 4-byte Reload
	subl	$17, %eax
	je	.LBB0_4
	jmp	.LBB0_6
.LBB0_1:
	movl	$1, -4(%rbp)
	jmp	.LBB0_7
.LBB0_2:
	movl	$2, -4(%rbp)
	jmp	.LBB0_7
.LBB0_3:
	movl	$3, -4(%rbp)
	jmp	.LBB0_7
.LBB0_4:
	movl	$4, -4(%rbp)
	jmp	.LBB0_7
.LBB0_5:
	movl	$5, -4(%rbp)
	jmp	.LBB0_7
.LBB0_6:
	movl	$0, -4(%rbp)
.LBB0_7:
	movl	-4(%rbp), %eax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	kvm_exit_action, .Lfunc_end0-kvm_exit_action
                                        # -- End function
	.globl	io_batch_bytes                  # -- Begin function io_batch_bytes
	.p2align	4
	.type	io_batch_bytes,@function
io_batch_bytes:                         # @io_batch_bytes
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	%edi, -4(%rbp)
	movl	%esi, -8(%rbp)
	movl	-4(%rbp), %eax
	imull	-8(%rbp), %eax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	io_batch_bytes, .Lfunc_end1-io_batch_bytes
                                        # -- End function
	.globl	serial_is_console_write         # -- Begin function serial_is_console_write
	.p2align	4
	.type	serial_is_console_write,@function
serial_is_console_write:                # @serial_is_console_write
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	%edi, -4(%rbp)
	movl	%esi, -8(%rbp)
	movl	%edx, -12(%rbp)
	cmpl	$1, -4(%rbp)
	sete	%al
	andb	$1, %al
	movzbl	%al, %eax
	movl	-8(%rbp), %ecx
	cmpl	-12(%rbp), %ecx
	sete	%cl
	andb	$1, %cl
	movzbl	%cl, %ecx
	andl	%ecx, %eax
	popq	%rbp
	retq
.Lfunc_end2:
	.size	serial_is_console_write, .Lfunc_end2-serial_is_console_write
                                        # -- End function
	.globl	decode_exit                     # -- Begin function decode_exit
	.p2align	4
	.type	decode_exit,@function
decode_exit:                            # @decode_exit
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movl	%edi, -12(%rbp)
	movl	-12(%rbp), %edi
	callq	kvm_exit_action
	movl	%eax, -16(%rbp)
	movl	-16(%rbp), %eax
	movl	%eax, -8(%rbp)
	cmpl	$4, -16(%rbp)
	sete	%al
	andb	$1, %al
	movzbl	%al, %eax
	movl	%eax, -4(%rbp)
	movq	-8(%rbp), %rax
	addq	$16, %rsp
	popq	%rbp
	retq
.Lfunc_end3:
	.size	decode_exit, .Lfunc_end3-decode_exit
                                        # -- End function
	.globl	demo_selftest                   # -- Begin function demo_selftest
	.p2align	4
	.type	demo_selftest,@function
demo_selftest:                          # @demo_selftest
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movl	$2, %edi
	callq	kvm_exit_action
	cmpl	$1, %eax
	je	.LBB4_2
# %bb.1:
	movl	$1, -4(%rbp)
	jmp	.LBB4_30
.LBB4_2:
	movl	$6, %edi
	callq	kvm_exit_action
	cmpl	$2, %eax
	je	.LBB4_4
# %bb.3:
	movl	$2, -4(%rbp)
	jmp	.LBB4_30
.LBB4_4:
	movl	$5, %edi
	callq	kvm_exit_action
	cmpl	$3, %eax
	je	.LBB4_6
# %bb.5:
	movl	$3, -4(%rbp)
	jmp	.LBB4_30
.LBB4_6:
	movl	$8, %edi
	callq	kvm_exit_action
	cmpl	$4, %eax
	je	.LBB4_8
# %bb.7:
	movl	$4, -4(%rbp)
	jmp	.LBB4_30
.LBB4_8:
	movl	$9, %edi
	callq	kvm_exit_action
	cmpl	$4, %eax
	je	.LBB4_10
# %bb.9:
	movl	$5, -4(%rbp)
	jmp	.LBB4_30
.LBB4_10:
	movl	$17, %edi
	callq	kvm_exit_action
	cmpl	$4, %eax
	je	.LBB4_12
# %bb.11:
	movl	$6, -4(%rbp)
	jmp	.LBB4_30
.LBB4_12:
	movl	$10, %edi
	callq	kvm_exit_action
	cmpl	$5, %eax
	je	.LBB4_14
# %bb.13:
	movl	$7, -4(%rbp)
	jmp	.LBB4_30
.LBB4_14:
	movl	$7, %edi
	callq	kvm_exit_action
	cmpl	$5, %eax
	je	.LBB4_16
# %bb.15:
	movl	$8, -4(%rbp)
	jmp	.LBB4_30
.LBB4_16:
	movl	$3, %edi
	callq	kvm_exit_action
	cmpl	$0, %eax
	je	.LBB4_18
# %bb.17:
	movl	$9, -4(%rbp)
	jmp	.LBB4_30
.LBB4_18:
	movl	$4, %edi
	movl	$8, %esi
	callq	io_batch_bytes
	cmpl	$32, %eax
	je	.LBB4_20
# %bb.19:
	movl	$10, -4(%rbp)
	jmp	.LBB4_30
.LBB4_20:
	movl	$1, %edi
	movl	$1016, %edx                     # imm = 0x3F8
	movl	%edx, %esi
	callq	serial_is_console_write
	cmpl	$0, %eax
	jne	.LBB4_22
# %bb.21:
	movl	$11, -4(%rbp)
	jmp	.LBB4_30
.LBB4_22:
	xorl	%edi, %edi
	movl	$1016, %edx                     # imm = 0x3F8
	movl	%edx, %esi
	callq	serial_is_console_write
	cmpl	$0, %eax
	je	.LBB4_24
# %bb.23:
	movl	$12, -4(%rbp)
	jmp	.LBB4_30
.LBB4_24:
	movl	$1, %edi
	movl	$760, %esi                      # imm = 0x2F8
	movl	$1016, %edx                     # imm = 0x3F8
	callq	serial_is_console_write
	cmpl	$0, %eax
	je	.LBB4_26
# %bb.25:
	movl	$13, -4(%rbp)
	jmp	.LBB4_30
.LBB4_26:
	movl	$8, %edi
	callq	decode_exit
	movq	%rax, -12(%rbp)
	cmpl	$4, -12(%rbp)
	jne	.LBB4_28
# %bb.27:
	cmpl	$1, -8(%rbp)
	je	.LBB4_29
.LBB4_28:
	movl	$14, -4(%rbp)
	jmp	.LBB4_30
.LBB4_29:
	movl	$0, -4(%rbp)
.LBB4_30:
	movl	-4(%rbp), %eax
	addq	$16, %rsp
	popq	%rbp
	retq
.Lfunc_end4:
	.size	demo_selftest, .Lfunc_end4-demo_selftest
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym kvm_exit_action
	.addrsig_sym io_batch_bytes
	.addrsig_sym serial_is_console_write
	.addrsig_sym decode_exit
