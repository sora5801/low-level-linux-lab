	.file	"demo.c"
	.text
	.globl	encode_mov_rr                   # -- Begin function encode_mov_rr
	.p2align	4
	.type	encode_mov_rr,@function
encode_mov_rr:                          # @encode_mov_rr
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movl	%edx, -16(%rbp)
	movl	$0, -20(%rbp)
	movb	$72, -21(%rbp)
	cmpl	$8, -12(%rbp)
	jl	.LBB0_2
# %bb.1:
	movzbl	-21(%rbp), %eax
	orl	$4, %eax
                                        # kill: def $al killed $al killed $eax
	movb	%al, -21(%rbp)
.LBB0_2:
	cmpl	$8, -16(%rbp)
	jl	.LBB0_4
# %bb.3:
	movzbl	-21(%rbp), %eax
	orl	$1, %eax
                                        # kill: def $al killed $al killed $eax
	movb	%al, -21(%rbp)
.LBB0_4:
	movb	-21(%rbp), %dl
	movq	-8(%rbp), %rax
	movl	-20(%rbp), %ecx
	movl	%ecx, %esi
	addl	$1, %esi
	movl	%esi, -20(%rbp)
	movslq	%ecx, %rcx
	movb	%dl, (%rax,%rcx)
	movq	-8(%rbp), %rax
	movl	-20(%rbp), %ecx
	movl	%ecx, %edx
	addl	$1, %edx
	movl	%edx, -20(%rbp)
	movslq	%ecx, %rcx
	movb	$-119, (%rax,%rcx)
	movl	-12(%rbp), %eax
	andl	$7, %eax
	shll	$3, %eax
	orl	$192, %eax
	movl	-16(%rbp), %ecx
	andl	$7, %ecx
	orl	%ecx, %eax
	movb	%al, %dl
	movq	-8(%rbp), %rax
	movl	-20(%rbp), %ecx
	movl	%ecx, %esi
	addl	$1, %esi
	movl	%esi, -20(%rbp)
	movslq	%ecx, %rcx
	movb	%dl, (%rax,%rcx)
	movl	-20(%rbp), %eax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	encode_mov_rr, .Lfunc_end0-encode_mov_rr
                                        # -- End function
	.globl	backpatch_rel32                 # -- Begin function backpatch_rel32
	.p2align	4
	.type	backpatch_rel32,@function
backpatch_rel32:                        # @backpatch_rel32
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movl	%edx, -16(%rbp)
	movl	-16(%rbp), %eax
	movl	-12(%rbp), %ecx
	addl	$4, %ecx
	subl	%ecx, %eax
	movl	%eax, -20(%rbp)
	movl	-20(%rbp), %eax
	andl	$255, %eax
	movb	%al, %dl
	movq	-8(%rbp), %rax
	movl	-12(%rbp), %ecx
	addl	$0, %ecx
	movl	%ecx, %ecx
                                        # kill: def $rcx killed $ecx
	movb	%dl, (%rax,%rcx)
	movl	-20(%rbp), %eax
	shrl	$8, %eax
	andl	$255, %eax
	movb	%al, %dl
	movq	-8(%rbp), %rax
	movl	-12(%rbp), %ecx
	addl	$1, %ecx
	movl	%ecx, %ecx
                                        # kill: def $rcx killed $ecx
	movb	%dl, (%rax,%rcx)
	movl	-20(%rbp), %eax
	shrl	$16, %eax
	andl	$255, %eax
	movb	%al, %dl
	movq	-8(%rbp), %rax
	movl	-12(%rbp), %ecx
	addl	$2, %ecx
	movl	%ecx, %ecx
                                        # kill: def $rcx killed $ecx
	movb	%dl, (%rax,%rcx)
	movl	-20(%rbp), %eax
	shrl	$24, %eax
	andl	$255, %eax
	movb	%al, %dl
	movq	-8(%rbp), %rax
	movl	-12(%rbp), %ecx
	addl	$3, %ecx
	movl	%ecx, %ecx
                                        # kill: def $rcx killed $ecx
	movb	%dl, (%rax,%rcx)
	popq	%rbp
	retq
.Lfunc_end1:
	.size	backpatch_rel32, .Lfunc_end1-backpatch_rel32
                                        # -- End function
	.globl	main                            # -- Begin function main
	.p2align	4
	.type	main,@function
main:                                   # @main
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$48, %rsp
	movl	$0, -4(%rbp)
	leaq	-32(%rbp), %rdi
	movl	$6, %esi
	movl	$7, %edx
	callq	encode_mov_rr
	movl	%eax, -36(%rbp)
	xorl	%eax, %eax
                                        # kill: def $al killed $al killed $eax
	cmpl	$3, -36(%rbp)
	movb	%al, -41(%rbp)                  # 1-byte Spill
	jne	.LBB2_4
# %bb.1:
	movzbl	-32(%rbp), %ecx
	xorl	%eax, %eax
                                        # kill: def $al killed $al killed $eax
	cmpl	$72, %ecx
	movb	%al, -41(%rbp)                  # 1-byte Spill
	jne	.LBB2_4
# %bb.2:
	movzbl	-31(%rbp), %ecx
	xorl	%eax, %eax
                                        # kill: def $al killed $al killed $eax
	cmpl	$137, %ecx
	movb	%al, -41(%rbp)                  # 1-byte Spill
	jne	.LBB2_4
# %bb.3:
	movzbl	-30(%rbp), %eax
	cmpl	$247, %eax
	sete	%al
	movb	%al, -41(%rbp)                  # 1-byte Spill
.LBB2_4:
	movb	-41(%rbp), %al                  # 1-byte Reload
	andb	$1, %al
	movzbl	%al, %eax
	movl	%eax, -40(%rbp)
	leaq	-32(%rbp), %rdi
	movl	$8, %esi
	movl	$15, %edx
	callq	encode_mov_rr
	movl	%eax, -36(%rbp)
	xorl	%eax, %eax
                                        # kill: def $al killed $al killed $eax
	cmpl	$0, -40(%rbp)
	movb	%al, -42(%rbp)                  # 1-byte Spill
	je	.LBB2_8
# %bb.5:
	movzbl	-32(%rbp), %ecx
	xorl	%eax, %eax
                                        # kill: def $al killed $al killed $eax
	cmpl	$77, %ecx
	movb	%al, -42(%rbp)                  # 1-byte Spill
	jne	.LBB2_8
# %bb.6:
	movzbl	-31(%rbp), %ecx
	xorl	%eax, %eax
                                        # kill: def $al killed $al killed $eax
	cmpl	$137, %ecx
	movb	%al, -42(%rbp)                  # 1-byte Spill
	jne	.LBB2_8
# %bb.7:
	movzbl	-30(%rbp), %eax
	cmpl	$199, %eax
	sete	%al
	movb	%al, -42(%rbp)                  # 1-byte Spill
.LBB2_8:
	movb	-42(%rbp), %al                  # 1-byte Reload
	andb	$1, %al
	movzbl	%al, %eax
	movl	%eax, -40(%rbp)
	movb	$-23, -32(%rbp)
	leaq	-32(%rbp), %rdi
	movl	$1, %esi
	movl	$10, %edx
	callq	backpatch_rel32
	xorl	%eax, %eax
                                        # kill: def $al killed $al killed $eax
	cmpl	$0, -40(%rbp)
	movb	%al, -43(%rbp)                  # 1-byte Spill
	je	.LBB2_13
# %bb.9:
	movzbl	-31(%rbp), %ecx
	xorl	%eax, %eax
                                        # kill: def $al killed $al killed $eax
	cmpl	$5, %ecx
	movb	%al, -43(%rbp)                  # 1-byte Spill
	jne	.LBB2_13
# %bb.10:
	movzbl	-30(%rbp), %ecx
	xorl	%eax, %eax
                                        # kill: def $al killed $al killed $eax
	cmpl	$0, %ecx
	movb	%al, -43(%rbp)                  # 1-byte Spill
	jne	.LBB2_13
# %bb.11:
	movzbl	-29(%rbp), %ecx
	xorl	%eax, %eax
                                        # kill: def $al killed $al killed $eax
	cmpl	$0, %ecx
	movb	%al, -43(%rbp)                  # 1-byte Spill
	jne	.LBB2_13
# %bb.12:
	movzbl	-28(%rbp), %eax
	cmpl	$0, %eax
	sete	%al
	movb	%al, -43(%rbp)                  # 1-byte Spill
.LBB2_13:
	movb	-43(%rbp), %al                  # 1-byte Reload
	andb	$1, %al
	movzbl	%al, %eax
	movl	%eax, -40(%rbp)
	movl	-40(%rbp), %edx
	movl	$1, %eax
	xorl	%ecx, %ecx
	cmpl	$0, %edx
	cmovnel	%ecx, %eax
	addq	$48, %rsp
	popq	%rbp
	retq
.Lfunc_end2:
	.size	main, .Lfunc_end2-main
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym encode_mov_rr
	.addrsig_sym backpatch_rel32
