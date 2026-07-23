	.file	"demo.c"
	.text
	.globl	invoke_hello                    # -- Begin function invoke_hello
	.p2align	4
	.type	invoke_hello,@function
invoke_hello:                           # @invoke_hello
# %bb.0:
	movl	$463, %eax                      # imm = 0x1CF
	xorl	%edx, %edx
	#APP
	syscall
	#NO_APP
	retq
.Lfunc_end0:
	.size	invoke_hello, .Lfunc_end0-invoke_hello
                                        # -- End function
	.globl	hello_or_errno                  # -- Begin function hello_or_errno
	.p2align	4
	.type	hello_or_errno,@function
hello_or_errno:                         # @hello_or_errno
# %bb.0:
	movq	%rdx, %r8
	xorl	%r9d, %r9d
	movl	$463, %eax                      # imm = 0x1CF
	xorl	%edx, %edx
	#APP
	syscall
	#NO_APP
	movl	%eax, %ecx
	negl	%ecx
	cmpq	$-4095, %rax                    # imm = 0xF001
	cmovbl	%r9d, %ecx
	movq	$-1, %rdx
	cmovaeq	%rdx, %rax
	movl	%ecx, (%r8)
	retq
.Lfunc_end1:
	.size	hello_or_errno, .Lfunc_end1-hello_or_errno
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
