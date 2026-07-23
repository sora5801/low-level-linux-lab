	.file	"demo.c"
	.text
	.globl	invoke_hello                    # -- Begin function invoke_hello
	.p2align	4
	.type	invoke_hello,@function
invoke_hello:                           # @invoke_hello
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	-8(%rbp), %rsi
	movq	-16(%rbp), %rdx
	movl	$463, %edi                      # imm = 0x1CF
	xorl	%eax, %eax
	movl	%eax, %ecx
	callq	raw_syscall3
	addq	$16, %rsp
	popq	%rbp
	retq
.Lfunc_end0:
	.size	invoke_hello, .Lfunc_end0-invoke_hello
                                        # -- End function
	.p2align	4                               # -- Begin function raw_syscall3
	.type	raw_syscall3,@function
raw_syscall3:                           # @raw_syscall3
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	%rdx, -24(%rbp)
	movq	%rcx, -32(%rbp)
	movq	-8(%rbp), %rax
	movq	-16(%rbp), %rdi
	movq	-24(%rbp), %rsi
	movq	-32(%rbp), %rdx
	#APP
	syscall
	#NO_APP
	movq	%rax, -40(%rbp)
	movq	-40(%rbp), %rax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	raw_syscall3, .Lfunc_end1-raw_syscall3
                                        # -- End function
	.globl	hello_or_errno                  # -- Begin function hello_or_errno
	.p2align	4
	.type	hello_or_errno,@function
hello_or_errno:                         # @hello_or_errno
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$48, %rsp
	movq	%rdi, -16(%rbp)
	movq	%rsi, -24(%rbp)
	movq	%rdx, -32(%rbp)
	movq	-16(%rbp), %rsi
	movq	-24(%rbp), %rdx
	movl	$463, %edi                      # imm = 0x1CF
	xorl	%eax, %eax
	movl	%eax, %ecx
	callq	raw_syscall3
	movq	%rax, -40(%rbp)
	cmpq	$0, -40(%rbp)
	jge	.LBB2_3
# %bb.1:
	cmpq	$-4095, -40(%rbp)               # imm = 0xF001
	jl	.LBB2_3
# %bb.2:
	xorl	%eax, %eax
                                        # kill: def $rax killed $eax
	subq	-40(%rbp), %rax
	movl	%eax, %ecx
	movq	-32(%rbp), %rax
	movl	%ecx, (%rax)
	movq	$-1, -8(%rbp)
	jmp	.LBB2_4
.LBB2_3:
	movq	-32(%rbp), %rax
	movl	$0, (%rax)
	movq	-40(%rbp), %rax
	movq	%rax, -8(%rbp)
.LBB2_4:
	movq	-8(%rbp), %rax
	addq	$48, %rsp
	popq	%rbp
	retq
.Lfunc_end2:
	.size	hello_or_errno, .Lfunc_end2-hello_or_errno
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym raw_syscall3
