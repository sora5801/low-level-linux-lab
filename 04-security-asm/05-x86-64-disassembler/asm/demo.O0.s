	.file	"demo.c"
	.text
	.globl	decode_modrm                    # -- Begin function decode_modrm
	.p2align	4
	.type	decode_modrm,@function
decode_modrm:                           # @decode_modrm
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$80, %rsp
	movq	%rdi, -80(%rbp)                 # 8-byte Spill
	movq	%rdi, %rax
	movq	%rax, -72(%rbp)                 # 8-byte Spill
	movq	%rsi, -8(%rbp)
	movl	%edx, -12(%rbp)
	movl	%ecx, -16(%rbp)
	movl	%r8d, -20(%rbp)
	movl	%r9d, -24(%rbp)
	movl	$0, (%rdi)
	movl	$0, 4(%rdi)
	movl	$0, 8(%rdi)
	movl	$0, 12(%rdi)
	movl	$0, 16(%rdi)
	movl	$0, 20(%rdi)
	movl	$0, 24(%rdi)
	movl	$1, 28(%rdi)
	movl	$0, 32(%rdi)
	movl	$0, 36(%rdi)
	movl	$0, 40(%rdi)
	movq	$0, 48(%rdi)
	movl	$0, 56(%rdi)
	cmpl	$1, -12(%rbp)
	jge	.LBB0_2
# %bb.1:
	movq	-80(%rbp), %rax                 # 8-byte Reload
	movl	$-1, 56(%rax)
	jmp	.LBB0_34
.LBB0_2:
	movq	-80(%rbp), %rax                 # 8-byte Reload
	movq	-8(%rbp), %rcx
	movb	(%rcx), %cl
	movb	%cl, -25(%rbp)
	movzbl	-25(%rbp), %ecx
	sarl	$6, %ecx
	andl	$3, %ecx
	movl	%ecx, -32(%rbp)
	movzbl	-25(%rbp), %ecx
	sarl	$3, %ecx
	andl	$7, %ecx
	movl	%ecx, -36(%rbp)
	movzbl	-25(%rbp), %ecx
	andl	$7, %ecx
	movl	%ecx, -40(%rbp)
	movl	$1, -44(%rbp)
	movl	-36(%rbp), %ecx
	movl	-16(%rbp), %edx
	shll	$3, %edx
	orl	%edx, %ecx
	movl	%ecx, 4(%rax)
	cmpl	$3, -32(%rbp)
	jne	.LBB0_4
# %bb.3:
	movq	-80(%rbp), %rax                 # 8-byte Reload
	movl	$1, (%rax)
	movl	-40(%rbp), %ecx
	movl	-24(%rbp), %edx
	shll	$3, %edx
	orl	%edx, %ecx
	movl	%ecx, 8(%rax)
	movl	-44(%rbp), %ecx
	movl	%ecx, 56(%rax)
	jmp	.LBB0_34
.LBB0_4:
	cmpl	$4, -40(%rbp)
	sete	%al
	andb	$1, %al
	movzbl	%al, %eax
	movl	%eax, -48(%rbp)
	cmpl	$0, -48(%rbp)
	je	.LBB0_17
# %bb.5:
	movl	-44(%rbp), %eax
	cmpl	-12(%rbp), %eax
	jl	.LBB0_7
# %bb.6:
	movq	-80(%rbp), %rax                 # 8-byte Reload
	movl	$-1, 56(%rax)
	jmp	.LBB0_34
.LBB0_7:
	movq	-8(%rbp), %rax
	movl	-44(%rbp), %ecx
	movl	%ecx, %edx
	addl	$1, %edx
	movl	%edx, -44(%rbp)
	movslq	%ecx, %rcx
	movb	(%rax,%rcx), %al
	movb	%al, -49(%rbp)
	movzbl	-49(%rbp), %eax
	sarl	$6, %eax
	andl	$3, %eax
	movl	%eax, -56(%rbp)
	movzbl	-49(%rbp), %eax
	sarl	$3, %eax
	andl	$7, %eax
	movl	%eax, -60(%rbp)
	movzbl	-49(%rbp), %eax
	andl	$7, %eax
	movl	%eax, -64(%rbp)
	cmpl	$4, -60(%rbp)
	jne	.LBB0_10
# %bb.8:
	cmpl	$0, -20(%rbp)
	jne	.LBB0_10
# %bb.9:
	movq	-80(%rbp), %rax                 # 8-byte Reload
	movl	$0, 20(%rax)
	jmp	.LBB0_11
.LBB0_10:
	movq	-80(%rbp), %rax                 # 8-byte Reload
	movl	$1, 20(%rax)
	movl	-60(%rbp), %ecx
	movl	-20(%rbp), %edx
	shll	$3, %edx
	orl	%edx, %ecx
	movl	%ecx, 24(%rax)
	movl	-56(%rbp), %ecx
	movl	$1, %edx
                                        # kill: def $cl killed $ecx
	shll	%cl, %edx
	movl	%edx, %ecx
	movl	%ecx, 28(%rax)
.LBB0_11:
	cmpl	$5, -64(%rbp)
	jne	.LBB0_16
# %bb.12:
	cmpl	$0, -32(%rbp)
	jne	.LBB0_16
# %bb.13:
	movq	-80(%rbp), %rax                 # 8-byte Reload
	movl	$0, 12(%rax)
	movl	-44(%rbp), %eax
	addl	$4, %eax
	cmpl	-12(%rbp), %eax
	jle	.LBB0_15
# %bb.14:
	movq	-80(%rbp), %rax                 # 8-byte Reload
	movl	$-1, 56(%rax)
	jmp	.LBB0_34
.LBB0_15:
	movq	-8(%rbp), %rdi
	movslq	-44(%rbp), %rax
	addq	%rax, %rdi
	movl	$4, %esi
	callq	read_disp
	movq	%rax, %rcx
	movq	-80(%rbp), %rax                 # 8-byte Reload
	movq	%rcx, 48(%rax)
	movl	$4, 40(%rax)
	movl	$1, 36(%rax)
	movl	-44(%rbp), %ecx
	addl	$4, %ecx
	movl	%ecx, -44(%rbp)
	movl	-44(%rbp), %ecx
	movl	%ecx, 56(%rax)
	jmp	.LBB0_34
.LBB0_16:
	movq	-80(%rbp), %rax                 # 8-byte Reload
	movl	$1, 12(%rax)
	movl	-64(%rbp), %ecx
	movl	-24(%rbp), %edx
	shll	$3, %edx
	orl	%edx, %ecx
	movl	%ecx, 16(%rax)
	jmp	.LBB0_24
.LBB0_17:
	cmpl	$0, -32(%rbp)
	jne	.LBB0_22
# %bb.18:
	cmpl	$5, -40(%rbp)
	jne	.LBB0_22
# %bb.19:
	movq	-80(%rbp), %rax                 # 8-byte Reload
	movl	$1, 32(%rax)
	movl	-44(%rbp), %eax
	addl	$4, %eax
	cmpl	-12(%rbp), %eax
	jle	.LBB0_21
# %bb.20:
	movq	-80(%rbp), %rax                 # 8-byte Reload
	movl	$-1, 56(%rax)
	jmp	.LBB0_34
.LBB0_21:
	movq	-8(%rbp), %rdi
	movslq	-44(%rbp), %rax
	addq	%rax, %rdi
	movl	$4, %esi
	callq	read_disp
	movq	%rax, %rcx
	movq	-80(%rbp), %rax                 # 8-byte Reload
	movq	%rcx, 48(%rax)
	movl	$4, 40(%rax)
	movl	$1, 36(%rax)
	movl	-44(%rbp), %ecx
	addl	$4, %ecx
	movl	%ecx, -44(%rbp)
	movl	-44(%rbp), %ecx
	movl	%ecx, 56(%rax)
	jmp	.LBB0_34
.LBB0_22:
	movq	-80(%rbp), %rax                 # 8-byte Reload
	movl	$1, 12(%rax)
	movl	-40(%rbp), %ecx
	movl	-24(%rbp), %edx
	shll	$3, %edx
	orl	%edx, %ecx
	movl	%ecx, 16(%rax)
# %bb.23:
	jmp	.LBB0_24
.LBB0_24:
	cmpl	$1, -32(%rbp)
	jne	.LBB0_28
# %bb.25:
	movl	-44(%rbp), %eax
	addl	$1, %eax
	cmpl	-12(%rbp), %eax
	jle	.LBB0_27
# %bb.26:
	movq	-80(%rbp), %rax                 # 8-byte Reload
	movl	$-1, 56(%rax)
	jmp	.LBB0_34
.LBB0_27:
	movq	-8(%rbp), %rdi
	movslq	-44(%rbp), %rax
	addq	%rax, %rdi
	movl	$1, %esi
	callq	read_disp
	movq	%rax, %rcx
	movq	-80(%rbp), %rax                 # 8-byte Reload
	movq	%rcx, 48(%rax)
	movl	$1, 40(%rax)
	movl	$1, 36(%rax)
	movl	-44(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -44(%rbp)
	jmp	.LBB0_33
.LBB0_28:
	cmpl	$2, -32(%rbp)
	jne	.LBB0_32
# %bb.29:
	movl	-44(%rbp), %eax
	addl	$4, %eax
	cmpl	-12(%rbp), %eax
	jle	.LBB0_31
# %bb.30:
	movq	-80(%rbp), %rax                 # 8-byte Reload
	movl	$-1, 56(%rax)
	jmp	.LBB0_34
.LBB0_31:
	movq	-8(%rbp), %rdi
	movslq	-44(%rbp), %rax
	addq	%rax, %rdi
	movl	$4, %esi
	callq	read_disp
	movq	%rax, %rcx
	movq	-80(%rbp), %rax                 # 8-byte Reload
	movq	%rcx, 48(%rax)
	movl	$4, 40(%rax)
	movl	$1, 36(%rax)
	movl	-44(%rbp), %eax
	addl	$4, %eax
	movl	%eax, -44(%rbp)
.LBB0_32:
	jmp	.LBB0_33
.LBB0_33:
	movq	-80(%rbp), %rax                 # 8-byte Reload
	movl	-44(%rbp), %ecx
	movl	%ecx, 56(%rax)
.LBB0_34:
	movq	-72(%rbp), %rax                 # 8-byte Reload
	addq	$80, %rsp
	popq	%rbp
	retq
.Lfunc_end0:
	.size	decode_modrm, .Lfunc_end0-decode_modrm
                                        # -- End function
	.p2align	4                               # -- Begin function read_disp
	.type	read_disp,@function
read_disp:                              # @read_disp
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movq	$0, -24(%rbp)
	movl	$0, -28(%rbp)
.LBB1_1:                                # =>This Inner Loop Header: Depth=1
	movl	-28(%rbp), %eax
	cmpl	-12(%rbp), %eax
	jge	.LBB1_4
# %bb.2:                                #   in Loop: Header=BB1_1 Depth=1
	movq	-8(%rbp), %rax
	movslq	-28(%rbp), %rcx
	movzbl	(%rax,%rcx), %eax
                                        # kill: def $rax killed $eax
	movl	-28(%rbp), %ecx
	shll	$3, %ecx
	movl	%ecx, %ecx
                                        # kill: def $rcx killed $ecx
                                        # kill: def $cl killed $rcx
	shlq	%cl, %rax
	orq	-24(%rbp), %rax
	movq	%rax, -24(%rbp)
# %bb.3:                                #   in Loop: Header=BB1_1 Depth=1
	movl	-28(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -28(%rbp)
	jmp	.LBB1_1
.LBB1_4:
	movl	-12(%rbp), %eax
	shll	$3, %eax
	subl	$1, %eax
	movl	%eax, %eax
	movl	%eax, %ecx
	movl	$1, %eax
                                        # kill: def $cl killed $rcx
	shlq	%cl, %rax
	movq	%rax, -40(%rbp)
	movq	-40(%rbp), %rax
	shlq	%rax
	subq	$1, %rax
	movq	%rax, -48(%rbp)
	movq	-24(%rbp), %rax
	andq	-40(%rbp), %rax
	cmpq	$0, %rax
	je	.LBB1_6
# %bb.5:
	movq	-48(%rbp), %rax
	xorq	$-1, %rax
	orq	-24(%rbp), %rax
	movq	%rax, -24(%rbp)
.LBB1_6:
	movq	-24(%rbp), %rax
	popq	%rbp
	retq
.Lfunc_end1:
	.size	read_disp, .Lfunc_end1-read_disp
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym read_disp
