	.file	"demo.c"
	.text
	.globl	syscall_args                    # -- Begin function syscall_args
	.p2align	4
	.type	syscall_args,@function
syscall_args:                           # @syscall_args
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	-8(%rbp), %rax
	movq	8(%rax), %rcx
	movq	-16(%rbp), %rax
	movq	%rcx, (%rax)
	movq	-8(%rbp), %rax
	movq	16(%rax), %rcx
	movq	-16(%rbp), %rax
	movq	%rcx, 8(%rax)
	movq	-8(%rbp), %rax
	movq	24(%rax), %rcx
	movq	-16(%rbp), %rax
	movq	%rcx, 16(%rax)
	movq	-8(%rbp), %rax
	movq	32(%rax), %rcx
	movq	-16(%rbp), %rax
	movq	%rcx, 24(%rax)
	movq	-8(%rbp), %rax
	movq	40(%rax), %rcx
	movq	-16(%rbp), %rax
	movq	%rcx, 32(%rax)
	movq	-8(%rbp), %rax
	movq	48(%rax), %rcx
	movq	-16(%rbp), %rax
	movq	%rcx, 40(%rax)
	movq	-8(%rbp), %rax
	movq	(%rax), %rax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	syscall_args, .Lfunc_end0-syscall_args
                                        # -- End function
	.globl	decode_flags                    # -- Begin function decode_flags
	.p2align	4
	.type	decode_flags,@function
decode_flags:                           # @decode_flags
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$64, %rsp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movl	%edx, -20(%rbp)
	movq	%rcx, -32(%rbp)
	movl	%r8d, -36(%rbp)
	movl	$0, -40(%rbp)
	movl	$0, -44(%rbp)
	movl	$0, -48(%rbp)
.LBB1_1:                                # =>This Loop Header: Depth=1
                                        #     Child Loop BB1_10 Depth 2
	movl	-48(%rbp), %ecx
	xorl	%eax, %eax
                                        # kill: def $al killed $al killed $eax
	cmpl	-20(%rbp), %ecx
	movb	%al, -61(%rbp)                  # 1-byte Spill
	jge	.LBB1_3
# %bb.2:                                #   in Loop: Header=BB1_1 Depth=1
	movl	-40(%rbp), %eax
	movl	-36(%rbp), %ecx
	subl	$1, %ecx
	cmpl	%ecx, %eax
	setl	%al
	movb	%al, -61(%rbp)                  # 1-byte Spill
.LBB1_3:                                #   in Loop: Header=BB1_1 Depth=1
	movb	-61(%rbp), %al                  # 1-byte Reload
	testb	$1, %al
	jne	.LBB1_4
	jmp	.LBB1_17
.LBB1_4:                                #   in Loop: Header=BB1_1 Depth=1
	movq	-16(%rbp), %rax
	movslq	-48(%rbp), %rcx
	shlq	$4, %rcx
	addq	%rcx, %rax
	cmpq	$0, (%rax)
	je	.LBB1_15
# %bb.5:                                #   in Loop: Header=BB1_1 Depth=1
	movq	-8(%rbp), %rax
	movq	-16(%rbp), %rcx
	movslq	-48(%rbp), %rdx
	shlq	$4, %rdx
	addq	%rdx, %rcx
	andq	(%rcx), %rax
	movq	-16(%rbp), %rcx
	movslq	-48(%rbp), %rdx
	shlq	$4, %rdx
	addq	%rdx, %rcx
	cmpq	(%rcx), %rax
	jne	.LBB1_15
# %bb.6:                                #   in Loop: Header=BB1_1 Depth=1
	cmpl	$0, -44(%rbp)
	je	.LBB1_9
# %bb.7:                                #   in Loop: Header=BB1_1 Depth=1
	movl	-40(%rbp), %eax
	movl	-36(%rbp), %ecx
	subl	$1, %ecx
	cmpl	%ecx, %eax
	jge	.LBB1_9
# %bb.8:                                #   in Loop: Header=BB1_1 Depth=1
	movq	-32(%rbp), %rax
	movl	-40(%rbp), %ecx
	movl	%ecx, %edx
	addl	$1, %edx
	movl	%edx, -40(%rbp)
	movslq	%ecx, %rcx
	movb	$124, (%rax,%rcx)
.LBB1_9:                                #   in Loop: Header=BB1_1 Depth=1
	movq	-16(%rbp), %rax
	movslq	-48(%rbp), %rcx
	shlq	$4, %rcx
	addq	%rcx, %rax
	movq	8(%rax), %rax
	movq	%rax, -56(%rbp)
	movl	$0, -60(%rbp)
.LBB1_10:                               #   Parent Loop BB1_1 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movq	-56(%rbp), %rax
	movslq	-60(%rbp), %rcx
	movsbl	(%rax,%rcx), %ecx
	xorl	%eax, %eax
                                        # kill: def $al killed $al killed $eax
	cmpl	$0, %ecx
	movb	%al, -62(%rbp)                  # 1-byte Spill
	je	.LBB1_12
# %bb.11:                               #   in Loop: Header=BB1_10 Depth=2
	movl	-40(%rbp), %eax
	movl	-36(%rbp), %ecx
	subl	$1, %ecx
	cmpl	%ecx, %eax
	setl	%al
	movb	%al, -62(%rbp)                  # 1-byte Spill
.LBB1_12:                               #   in Loop: Header=BB1_10 Depth=2
	movb	-62(%rbp), %al                  # 1-byte Reload
	testb	$1, %al
	jne	.LBB1_13
	jmp	.LBB1_14
.LBB1_13:                               #   in Loop: Header=BB1_10 Depth=2
	movq	-56(%rbp), %rax
	movl	-60(%rbp), %ecx
	movl	%ecx, %edx
	addl	$1, %edx
	movl	%edx, -60(%rbp)
	movslq	%ecx, %rcx
	movb	(%rax,%rcx), %dl
	movq	-32(%rbp), %rax
	movl	-40(%rbp), %ecx
	movl	%ecx, %esi
	addl	$1, %esi
	movl	%esi, -40(%rbp)
	movslq	%ecx, %rcx
	movb	%dl, (%rax,%rcx)
	jmp	.LBB1_10
.LBB1_14:                               #   in Loop: Header=BB1_1 Depth=1
	movq	-16(%rbp), %rax
	movslq	-48(%rbp), %rcx
	shlq	$4, %rcx
	addq	%rcx, %rax
	movq	(%rax), %rax
	xorq	$-1, %rax
	andq	-8(%rbp), %rax
	movq	%rax, -8(%rbp)
	movl	$1, -44(%rbp)
.LBB1_15:                               #   in Loop: Header=BB1_1 Depth=1
	jmp	.LBB1_16
.LBB1_16:                               #   in Loop: Header=BB1_1 Depth=1
	movl	-48(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -48(%rbp)
	jmp	.LBB1_1
.LBB1_17:
	cmpq	$0, -8(%rbp)
	je	.LBB1_22
# %bb.18:
	cmpl	$0, -44(%rbp)
	je	.LBB1_21
# %bb.19:
	movl	-40(%rbp), %eax
	movl	-36(%rbp), %ecx
	subl	$1, %ecx
	cmpl	%ecx, %eax
	jge	.LBB1_21
# %bb.20:
	movq	-32(%rbp), %rax
	movl	-40(%rbp), %ecx
	movl	%ecx, %edx
	addl	$1, %edx
	movl	%edx, -40(%rbp)
	movslq	%ecx, %rcx
	movb	$124, (%rax,%rcx)
.LBB1_21:
	movq	-32(%rbp), %rdi
	movl	-36(%rbp), %esi
	movl	-40(%rbp), %edx
	movq	-8(%rbp), %rcx
	callq	put_hex
	movl	%eax, -40(%rbp)
	jmp	.LBB1_26
.LBB1_22:
	cmpl	$0, -44(%rbp)
	jne	.LBB1_25
# %bb.23:
	movl	-40(%rbp), %eax
	movl	-36(%rbp), %ecx
	subl	$1, %ecx
	cmpl	%ecx, %eax
	jge	.LBB1_25
# %bb.24:
	movq	-32(%rbp), %rax
	movl	-40(%rbp), %ecx
	movl	%ecx, %edx
	addl	$1, %edx
	movl	%edx, -40(%rbp)
	movslq	%ecx, %rcx
	movb	$48, (%rax,%rcx)
.LBB1_25:
	jmp	.LBB1_26
.LBB1_26:
	movl	-40(%rbp), %eax
	cmpl	-36(%rbp), %eax
	jge	.LBB1_28
# %bb.27:
	movq	-32(%rbp), %rax
	movslq	-40(%rbp), %rcx
	movb	$0, (%rax,%rcx)
	jmp	.LBB1_31
.LBB1_28:
	cmpl	$0, -36(%rbp)
	jle	.LBB1_30
# %bb.29:
	movq	-32(%rbp), %rax
	movl	-36(%rbp), %ecx
	subl	$1, %ecx
	movslq	%ecx, %rcx
	movb	$0, (%rax,%rcx)
.LBB1_30:
	jmp	.LBB1_31
.LBB1_31:
	movl	-40(%rbp), %eax
	addq	$64, %rsp
	popq	%rbp
	retq
.Lfunc_end1:
	.size	decode_flags, .Lfunc_end1-decode_flags
                                        # -- End function
	.p2align	4                               # -- Begin function put_hex
	.type	put_hex,@function
put_hex:                                # @put_hex
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -16(%rbp)
	movl	%esi, -20(%rbp)
	movl	%edx, -24(%rbp)
	movq	%rcx, -32(%rbp)
	movl	-24(%rbp), %eax
	movl	-20(%rbp), %ecx
	subl	$1, %ecx
	cmpl	%ecx, %eax
	jge	.LBB2_2
# %bb.1:
	movq	-16(%rbp), %rax
	movl	-24(%rbp), %ecx
	movl	%ecx, %edx
	addl	$1, %edx
	movl	%edx, -24(%rbp)
	movslq	%ecx, %rcx
	movb	$48, (%rax,%rcx)
.LBB2_2:
	movl	-24(%rbp), %eax
	movl	-20(%rbp), %ecx
	subl	$1, %ecx
	cmpl	%ecx, %eax
	jge	.LBB2_4
# %bb.3:
	movq	-16(%rbp), %rax
	movl	-24(%rbp), %ecx
	movl	%ecx, %edx
	addl	$1, %edx
	movl	%edx, -24(%rbp)
	movslq	%ecx, %rcx
	movb	$120, (%rax,%rcx)
.LBB2_4:
	cmpq	$0, -32(%rbp)
	jne	.LBB2_8
# %bb.5:
	movl	-24(%rbp), %eax
	movl	-20(%rbp), %ecx
	subl	$1, %ecx
	cmpl	%ecx, %eax
	jge	.LBB2_7
# %bb.6:
	movq	-16(%rbp), %rax
	movl	-24(%rbp), %ecx
	movl	%ecx, %edx
	addl	$1, %edx
	movl	%edx, -24(%rbp)
	movslq	%ecx, %rcx
	movb	$48, (%rax,%rcx)
.LBB2_7:
	movl	-24(%rbp), %eax
	movl	%eax, -4(%rbp)
	jmp	.LBB2_19
.LBB2_8:
	movl	$0, -52(%rbp)
.LBB2_9:                                # =>This Inner Loop Header: Depth=1
	xorl	%eax, %eax
                                        # kill: def $al killed $al killed $eax
	cmpq	$0, -32(%rbp)
	movb	%al, -53(%rbp)                  # 1-byte Spill
	je	.LBB2_11
# %bb.10:                               #   in Loop: Header=BB2_9 Depth=1
	cmpl	$16, -52(%rbp)
	setl	%al
	movb	%al, -53(%rbp)                  # 1-byte Spill
.LBB2_11:                               #   in Loop: Header=BB2_9 Depth=1
	movb	-53(%rbp), %al                  # 1-byte Reload
	testb	$1, %al
	jne	.LBB2_12
	jmp	.LBB2_13
.LBB2_12:                               #   in Loop: Header=BB2_9 Depth=1
	movq	-32(%rbp), %rcx
	andq	$15, %rcx
	leaq	put_hex.digits(%rip), %rax
	movb	(%rax,%rcx), %cl
	movl	-52(%rbp), %eax
	movl	%eax, %edx
	addl	$1, %edx
	movl	%edx, -52(%rbp)
	cltq
	movb	%cl, -48(%rbp,%rax)
	movq	-32(%rbp), %rax
	shrq	$4, %rax
	movq	%rax, -32(%rbp)
	jmp	.LBB2_9
.LBB2_13:
	jmp	.LBB2_14
.LBB2_14:                               # =>This Inner Loop Header: Depth=1
	xorl	%eax, %eax
                                        # kill: def $al killed $al killed $eax
	cmpl	$0, -52(%rbp)
	movb	%al, -54(%rbp)                  # 1-byte Spill
	jle	.LBB2_16
# %bb.15:                               #   in Loop: Header=BB2_14 Depth=1
	movl	-24(%rbp), %eax
	movl	-20(%rbp), %ecx
	subl	$1, %ecx
	cmpl	%ecx, %eax
	setl	%al
	movb	%al, -54(%rbp)                  # 1-byte Spill
.LBB2_16:                               #   in Loop: Header=BB2_14 Depth=1
	movb	-54(%rbp), %al                  # 1-byte Reload
	testb	$1, %al
	jne	.LBB2_17
	jmp	.LBB2_18
.LBB2_17:                               #   in Loop: Header=BB2_14 Depth=1
	movl	-52(%rbp), %eax
	addl	$-1, %eax
	movl	%eax, -52(%rbp)
	cltq
	movb	-48(%rbp,%rax), %dl
	movq	-16(%rbp), %rax
	movl	-24(%rbp), %ecx
	movl	%ecx, %esi
	addl	$1, %esi
	movl	%esi, -24(%rbp)
	movslq	%ecx, %rcx
	movb	%dl, (%rax,%rcx)
	jmp	.LBB2_14
.LBB2_18:
	movl	-24(%rbp), %eax
	movl	%eax, -4(%rbp)
.LBB2_19:
	movl	-4(%rbp), %eax
	popq	%rbp
	retq
.Lfunc_end2:
	.size	put_hex, .Lfunc_end2-put_hex
                                        # -- End function
	.type	put_hex.digits,@object          # @put_hex.digits
	.section	.rodata,"a",@progbits
	.p2align	4, 0x0
put_hex.digits:
	.asciz	"0123456789abcdef"
	.size	put_hex.digits, 17

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym put_hex
	.addrsig_sym put_hex.digits
