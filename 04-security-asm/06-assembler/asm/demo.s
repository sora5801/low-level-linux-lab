	.file	"demo.c"
	.text
	.globl	encode_mov_rr                   # -- Begin function encode_mov_rr
	.p2align	4
	.type	encode_mov_rr,@function
encode_mov_rr:                          # @encode_mov_rr
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
                                        # kill: def $edx killed $edx def $rdx
                                        # kill: def $esi killed $esi def $rsi
	cmpl	$8, %esi
	setge	%al
	shlb	$2, %al
	cmpl	$8, %edx
	setge	%cl
	orb	%al, %cl
	orb	$72, %cl
	movb	%cl, (%rdi)
	movb	$-119, 1(%rdi)
	andl	$7, %edx
	leal	(%rdx,%rsi,8), %eax
	orb	$-64, %al
	movb	%al, 2(%rdi)
	movl	$3, %eax
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
                                        # kill: def $esi killed $esi def $rsi
	subl	%esi, %edx
	addl	$-4, %edx
	movl	%esi, %eax
	movb	%dl, (%rdi,%rax)
	leal	1(%rsi), %eax
	movb	%dh, (%rdi,%rax)
	movl	%edx, %eax
	shrl	$16, %eax
	leal	2(%rsi), %ecx
	movb	%al, (%rdi,%rcx)
	shrl	$24, %edx
	addl	$3, %esi
	movb	%dl, (%rdi,%rsi)
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
	xorl	%eax, %eax
	popq	%rbp
	retq
.Lfunc_end2:
	.size	main, .Lfunc_end2-main
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
