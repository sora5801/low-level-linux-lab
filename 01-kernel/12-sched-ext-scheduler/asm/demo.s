	.file	"demo.c"
	.text
	.globl	vtime_before                    # -- Begin function vtime_before
	.p2align	4
	.type	vtime_before,@function
vtime_before:                           # @vtime_before
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, %rax
	subq	%rsi, %rax
	shrq	$63, %rax
                                        # kill: def $eax killed $eax killed $rax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	vtime_before, .Lfunc_end0-vtime_before
                                        # -- End function
	.globl	vtime_charge                    # -- Begin function vtime_charge
	.p2align	4
	.type	vtime_charge,@function
vtime_charge:                           # @vtime_charge
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	$20000000, %eax                 # imm = 0x1312D00
	subq	%rsi, %rax
	imulq	$100, %rax, %rax
	movl	%edx, %ecx
	xorl	%edx, %edx
	divq	%rcx
	addq	%rdi, %rax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	vtime_charge, .Lfunc_end1-vtime_charge
                                        # -- End function
	.globl	vtime_clamp                     # -- Begin function vtime_clamp
	.p2align	4
	.type	vtime_clamp,@function
vtime_clamp:                            # @vtime_clamp
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	leaq	-20000000(%rsi), %rax
	cmpq	%rax, %rdi
	cmovnsq	%rdi, %rax
	popq	%rbp
	retq
.Lfunc_end2:
	.size	vtime_clamp, .Lfunc_end2-vtime_clamp
                                        # -- End function
	.globl	vtime_on_stop                   # -- Begin function vtime_on_stop
	.p2align	4
	.type	vtime_on_stop,@function
vtime_on_stop:                          # @vtime_on_stop
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	$20000000, %eax                 # imm = 0x1312D00
	subq	%rsi, %rax
	imulq	$100, %rax, %rax
	movl	%edx, %esi
	xorl	%edx, %edx
	divq	%rsi
	addq	%rdi, %rax
	addq	$-20000000, %rcx                # imm = 0xFECED300
	cmpq	%rcx, %rax
	cmovsq	%rcx, %rax
	popq	%rbp
	retq
.Lfunc_end3:
	.size	vtime_on_stop, .Lfunc_end3-vtime_on_stop
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
