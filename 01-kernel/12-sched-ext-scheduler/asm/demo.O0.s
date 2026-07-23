	.file	"demo.c"
	.text
	.globl	vtime_before                    # -- Begin function vtime_before
	.p2align	4
	.type	vtime_before,@function
vtime_before:                           # @vtime_before
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	-8(%rbp), %rax
	subq	-16(%rbp), %rax
	cmpq	$0, %rax
	setl	%al
	andb	$1, %al
	movzbl	%al, %eax
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
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movl	%edx, -20(%rbp)
	movl	$20000000, %eax                 # imm = 0x1312D00
	subq	-16(%rbp), %rax
	movq	%rax, -32(%rbp)
	movq	-8(%rbp), %rax
	movq	%rax, -40(%rbp)                 # 8-byte Spill
	imulq	$100, -32(%rbp), %rax
	movl	-20(%rbp), %ecx
                                        # kill: def $rcx killed $ecx
	xorl	%edx, %edx
                                        # kill: def $rdx killed $edx
	divq	%rcx
	movq	%rax, %rcx
	movq	-40(%rbp), %rax                 # 8-byte Reload
	addq	%rcx, %rax
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
	subq	$32, %rsp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	-16(%rbp), %rax
	subq	$20000000, %rax                 # imm = 0x1312D00
	movq	%rax, -24(%rbp)
	movq	-8(%rbp), %rdi
	movq	-24(%rbp), %rsi
	callq	vtime_before
	cmpl	$0, %eax
	je	.LBB2_2
# %bb.1:
	movq	-24(%rbp), %rax
	movq	%rax, -8(%rbp)
.LBB2_2:
	movq	-8(%rbp), %rax
	addq	$32, %rsp
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
	subq	$32, %rsp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movl	%edx, -20(%rbp)
	movq	%rcx, -32(%rbp)
	movq	-8(%rbp), %rdi
	movq	-16(%rbp), %rsi
	movl	-20(%rbp), %edx
	callq	vtime_charge
	movq	%rax, -8(%rbp)
	movq	-8(%rbp), %rdi
	movq	-32(%rbp), %rsi
	callq	vtime_clamp
	movq	%rax, -8(%rbp)
	movq	-8(%rbp), %rax
	addq	$32, %rsp
	popq	%rbp
	retq
.Lfunc_end3:
	.size	vtime_on_stop, .Lfunc_end3-vtime_on_stop
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym vtime_before
	.addrsig_sym vtime_charge
	.addrsig_sym vtime_clamp
