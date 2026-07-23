	.file	"hello.c"
	.text
	.globl	_start                          # -- Begin function _start
	.p2align	4
	.type	_start,@function
_start:                                 # @_start
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	leaq	_start.msg(%rip), %rdi
	callq	kstrlen
	movq	%rax, %rdx
	movl	$1, %edi
	leaq	_start.msg(%rip), %rsi
	callq	sys_write
	movl	$7, %edi
	callq	sys_exit
.Lfunc_end0:
	.size	_start, .Lfunc_end0-_start
                                        # -- End function
	.p2align	4                               # -- Begin function sys_write
	.type	sys_write,@function
sys_write:                              # @sys_write
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$32, %rsp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	%rdx, -24(%rbp)
	movq	-8(%rbp), %rsi
	movq	-16(%rbp), %rdx
	movq	-24(%rbp), %rcx
	movl	$1, %edi
	callq	syscall3
	addq	$32, %rsp
	popq	%rbp
	retq
.Lfunc_end1:
	.size	sys_write, .Lfunc_end1-sys_write
                                        # -- End function
	.p2align	4                               # -- Begin function kstrlen
	.type	kstrlen,@function
kstrlen:                                # @kstrlen
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rax
	movq	%rax, -16(%rbp)
.LBB2_1:                                # =>This Inner Loop Header: Depth=1
	movq	-16(%rbp), %rax
	cmpb	$0, (%rax)
	je	.LBB2_3
# %bb.2:                                #   in Loop: Header=BB2_1 Depth=1
	movq	-16(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -16(%rbp)
	jmp	.LBB2_1
.LBB2_3:
	movq	-16(%rbp), %rax
	movq	-8(%rbp), %rcx
	subq	%rcx, %rax
	popq	%rbp
	retq
.Lfunc_end2:
	.size	kstrlen, .Lfunc_end2-kstrlen
                                        # -- End function
	.p2align	4                               # -- Begin function sys_exit
	.type	sys_exit,@function
sys_exit:                               # @sys_exit
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movl	%edi, -4(%rbp)
	movslq	-4(%rbp), %rsi
	movl	$231, %edi
	xorl	%eax, %eax
	movl	%eax, %ecx
	movq	%rcx, %rdx
	callq	syscall3
.Lfunc_end3:
	.size	sys_exit, .Lfunc_end3-sys_exit
                                        # -- End function
	.p2align	4                               # -- Begin function syscall3
	.type	syscall3,@function
syscall3:                               # @syscall3
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
.Lfunc_end4:
	.size	syscall3, .Lfunc_end4-syscall3
                                        # -- End function
	.type	_start.msg,@object              # @_start.msg
	.section	.rodata,"a",@progbits
	.p2align	4, 0x0
_start.msg:
	.asciz	"Hello from a program with no libc!\n"
	.size	_start.msg, 36

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym sys_write
	.addrsig_sym kstrlen
	.addrsig_sym sys_exit
	.addrsig_sym syscall3
	.addrsig_sym _start.msg
