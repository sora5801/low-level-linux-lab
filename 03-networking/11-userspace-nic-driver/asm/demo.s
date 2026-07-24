	.file	"demo.c"
	.text
	.globl	ring_next                       # -- Begin function ring_next
	.p2align	4
	.type	ring_next,@function
ring_next:                              # @ring_next
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
                                        # kill: def $esi killed $esi def $rsi
                                        # kill: def $edi killed $edi def $rdi
	leal	1(%rdi), %ecx
	leal	-1(%rsi), %eax
	andl	%ecx, %eax
                                        # kill: def $ax killed $ax killed $eax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	ring_next, .Lfunc_end0-ring_next
                                        # -- End function
	.globl	rx_poll                         # -- Begin function rx_poll
	.p2align	4
	.type	rx_poll,@function
rx_poll:                                # @rx_poll
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%rbx
	movzwl	(%rdx), %r9d
	decl	%esi
	xorl	%eax, %eax
	xorl	%r10d, %r10d
	jmp	.LBB1_1
	.p2align	4
.LBB1_4:                                #   in Loop: Header=BB1_1 Depth=1
	testb	$1, %r11b
	je	.LBB1_5
.LBB1_1:                                # =>This Inner Loop Header: Depth=1
	cmpl	%ecx, %eax
	jae	.LBB1_5
# %bb.2:                                #   in Loop: Header=BB1_1 Depth=1
	movzwl	%r9w, %ebx
	shll	$4, %ebx
	movl	8(%rdi,%rbx), %r11d
	testb	$1, %r11b
	je	.LBB1_4
# %bb.3:                                #   in Loop: Header=BB1_1 Depth=1
	addq	%rdi, %rbx
	#APP
	#NO_APP
	addl	12(%rbx), %r10d
	incl	%r9d
	andl	%esi, %r9d
	incl	%eax
	jmp	.LBB1_4
.LBB1_5:
	movw	%r9w, (%rdx)
	movl	%r10d, (%r8)
	popq	%rbx
	popq	%rbp
	retq
.Lfunc_end1:
	.size	rx_poll, .Lfunc_end1-rx_poll
                                        # -- End function
	.globl	tx_clean                        # -- Begin function tx_clean
	.p2align	4
	.type	tx_clean,@function
tx_clean:                               # @tx_clean
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%rbx
                                        # kill: def $r8d killed $r8d def $r8
	movzwl	(%rdx), %r9d
	decl	%esi
	leal	-1(%r8), %r10d
	xorl	%eax, %eax
	jmp	.LBB2_1
	.p2align	4
.LBB2_2:                                #   in Loop: Header=BB2_1 Depth=1
	xorl	%r11d, %r11d
	testb	%r11b, %r11b
	je	.LBB2_7
.LBB2_1:                                # =>This Inner Loop Header: Depth=1
	movl	%ecx, %r11d
	subl	%r9d, %r11d
	andl	%esi, %r11d
	cmpw	%r8w, %r11w
	jb	.LBB2_2
# %bb.3:                                #   in Loop: Header=BB2_1 Depth=1
	leal	(%r9,%r10), %r11d
	andl	%esi, %r11d
	movzwl	%r11w, %r11d
	shll	$4, %r11d
	movl	8(%rdi,%r11), %r11d
	andl	$1, %r11d
	je	.LBB2_5
# %bb.4:                                #   in Loop: Header=BB2_1 Depth=1
	addl	%r8d, %r9d
	andl	%esi, %r9d
.LBB2_5:                                #   in Loop: Header=BB2_1 Depth=1
	movl	%r11d, %ebx
	negl	%ebx
	andl	%r8d, %ebx
	addl	%ebx, %eax
	testb	%r11b, %r11b
	jne	.LBB2_1
.LBB2_7:
	movw	%r9w, (%rdx)
	popq	%rbx
	popq	%rbp
	retq
.Lfunc_end2:
	.size	tx_clean, .Lfunc_end2-tx_clean
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
