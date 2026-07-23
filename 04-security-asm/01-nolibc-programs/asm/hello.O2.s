	.file	"hello.c"
	.text
	.globl	_start                          # -- Begin function _start
	.p2align	4
	.type	_start,@function
_start:                                 # @_start
# %bb.0:
	leaq	_start.msg(%rip), %rsi
	movl	$1, %eax
	movl	$1, %edi
	movl	$35, %edx
	#APP
	syscall
	#NO_APP
	movl	$231, %eax
	movl	$7, %edi
	xorl	%esi, %esi
	xorl	%edx, %edx
	#APP
	syscall
	#NO_APP
.Lfunc_end0:
	.size	_start, .Lfunc_end0-_start
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
	.addrsig_sym _start.msg
